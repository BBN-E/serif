/**
 * Parallel implementation of ElfDocument object based on Python
 * implementation in ELF.py. Uses Xerces-C++ for XML writing.
 *
 * @file ElfDocument.cpp
 * @author nward@bbn.com
 * @date 2010.06.16
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/XmlUtil.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/PatternSet.h"
#include "Generic/state/XMLStrings.h"
#include "Temporal/TemporalAttributeAdder.h"
#include "LearnIt/LearnIt2Matcher.h"
#include "LearnIt/LearnItPattern.h"
#include "PredFinder/common/ContainerTypes.h"
#include "LearnIt/LearnIt2Matcher.h"
#include "LearnIt/MainUtilities.h"
#include "LearnIt/MatchInfo.h"
#include "PredFinder/common/ElfMultiDoc.h"
#include "PredFinder/common/SXMLUtil.h"
#include "PredFinder/inference/EIDocData.h"
#include "PredFinder/inference/ElfInference.h"
#include "PredFinder/elf/ElfDocument.h"
#include "PredFinder/elf/ElfIndividualFactory.h"
#include "PredFinder/elf/ElfRelationFactory.h"
#include "PredFinder/coref/ElfIndividualCoreference.h"

#include "boost/foreach.hpp"
#include "boost/make_shared.hpp"
#include "boost/filesystem.hpp"
#pragma warning(push, 0)
#include "boost/regex.hpp"
#pragma warning(pop)
#include <stdexcept>

using namespace std;

XERCES_CPP_NAMESPACE_USE

using boost::dynamic_pointer_cast;
using boost::make_shared;

std::vector<PatternFeatureSet_ptr> splitFeatureSets(PatternFeatureSet_ptr orig_feature_set){
	typedef std::pair<std::wstring, std::vector<ReturnPatternFeature_ptr> > RoleAndRPFVector;
	size_t n_features = orig_feature_set->getNFeatures();				
	std::vector<PatternFeatureSet_ptr> newFeatureSets;
	std::map<std::wstring, std::vector<ReturnPatternFeature_ptr> > returnFeaturesByRole;
	for (size_t i = 0; i < n_features; i++) {
		if (ReturnPatternFeature_ptr rpf = dynamic_pointer_cast<ReturnPatternFeature>(orig_feature_set->getFeature(i))){
			std::wstring role = rpf->getReturnValue(L"role");
			if(returnFeaturesByRole.find(role) == returnFeaturesByRole.end()){
				returnFeaturesByRole[role];
			}
			returnFeaturesByRole[role].push_back(rpf);
		}
	}
	std::set<ReturnPatternFeature_ptr> separableReturnPatternFeatures;
	std::set<std::wstring> separableRoles;

	BOOST_FOREACH(RoleAndRPFVector p, returnFeaturesByRole){
		//std::wcout<<"Role & counts "<<p.first<<" , "<<p.second.size()<<std::endl;
		if(p.second.size() > 1){
			//std::wcout<<"Role with multiple instances "<<p.first<<" , "<<p.second.size()<<std::endl;
			separableReturnPatternFeatures.insert(p.second.begin(), p.second.end());
			separableRoles.insert(p.first);
		}
	}
	if(separableRoles.size() == 1){	//anything else is too hard
		//for(
		BOOST_FOREACH(ReturnPatternFeature_ptr srpf, separableReturnPatternFeatures){
			PatternFeatureSet_ptr pfs = boost::make_shared<PatternFeatureSet>();
			pfs->setScore(orig_feature_set->getScore());
			pfs->setCoverage(orig_feature_set->getStartSentence(), orig_feature_set->getEndSentence(), orig_feature_set->getStartToken(), orig_feature_set->getEndToken());
			for (size_t i = 0; i < n_features; i++) {
				if (ReturnPatternFeature_ptr rpf = dynamic_pointer_cast<ReturnPatternFeature>(orig_feature_set->getFeature(i))){
					if(separableReturnPatternFeatures.find(rpf) == separableReturnPatternFeatures.end()){
						pfs->addFeature(rpf);
					}
				}
				else{
					pfs->addFeature(orig_feature_set->getFeature(i));
				}
			}
			pfs->addFeature(srpf);						
			newFeatureSets.push_back(pfs);
		}
		
	}
	if(newFeatureSets.size() == 0){
		newFeatureSets.push_back(orig_feature_set);
	}
	return newFeatureSets;
}

/**
 * Construct a document from a particular Serif document,
 * a set of LearnIt Patterns, a set of Distillation
 * PatternSets, and possibly the document's ACE
 * RelationSet and EntitySet.
 *
 * @param docData The return value from a previous call 
 * to ElfInference::prepareDocumentForInference(), invoked on the same Serif document.
 * @param elfInference A pointer to the ElfInference 
 * object whose prepareDocumentForInference() was called.
 * @param learnit_patterns A set of learned patterns to use
 * to find matches.
 * @param manual_patterns A set of manual patterns to use
 * to find matches.
 * @param use_ace_relations Whether ACE Relations should be
 * included as <relation>s.
 * @param use_ace_entities Whether ACE Entitys should be
 * included as <entity>s.
 *
 * @author nward@bbn.com
 * @date 2010.05.14
 **/
ElfDocument::ElfDocument(EIDocData_ptr docData, const ElfInference_ptr elfInference, 
						 const std::vector<LearnItPattern_ptr> & learnit_patterns, 
						 const std::vector<PatternSet_ptr> & manual_patterns,
						 const std::vector<LearnIt2Matcher_ptr>& learnit2_matchers,
						 TemporalAttributeAdder_ptr temporalAttributeAdder) 
{
	// Store the document ID
	_id = docData->getDocID();

	// Initialize the ELF metadata (currently all constants)
	_version = L"2.2";
	_source = L"BBN PredFinder";
	_contents = L"P-ELF";

	if (docData->getDocTheory() == NULL)
		throw UnexpectedInputException("ElfDocument::ElfDocument("
			"EIDocData_ptr,ElfInference_ptr,std::set<Pattern_ptr>,std::set<PatternSet_ptr>)", "doc theory was NULL");

	// For each manual Distillation pattern set, look for matches in this document
	BOOST_FOREACH(PatternSet_ptr pattern, manual_patterns) {
		// The following call, which now should be unnecessary, labeled all entities in this document using any entity label 
		// patterns that might be defined in this pattern set.
		//docData->getDocInfo()->labelPredFinderEntities(pattern.get());

		PatternMatcher_ptr pm = PatternMatcher::makePatternMatcher(docData->getDocTheory(), pattern);
		// Look for matches in each sentence
		for (int sent_index = 0; sent_index < docData->getNSentences(); sent_index++) {
			// Get the theory for this sentence
	        SentenceTheory* sent_theory = docData->getSentenceTheory(sent_index);
			if (SessionLogger::dbg_or_msg_enabled("feat_sets_st")) {
				std::ostringstream ostr;
				ostr << *sent_theory;
				SessionLogger::dbg("feat_sets_st") << ostr.str();
			}
			// Collect all relations and individuals in this sentence
			std::set<ElfRelation_ptr> sentence_relations;
			std::set<ElfIndividual_ptr> sentence_individuals;
	    
			// Look at all of the matches for this manual pattern in this sentence by feature set

			std::vector<PatternFeatureSet_ptr> feature_sets = pm->getSentenceSnippets(sent_theory,
				/*UTF8OutputStream=*/ 0, /*force_multimatches=*/ true);
			size_t feature_set_indices = feature_sets.size(); 
			// avoid doing a BOOST_FOREACH because we want the index number for debugging
			for (size_t feature_set_index = 0; feature_set_index < feature_set_indices; ++feature_set_index) {
				PatternFeatureSet_ptr & orig_feature_set = feature_sets[feature_set_index];

				std::vector<PatternFeatureSet_ptr> separatedFeatureSets = splitFeatureSets(orig_feature_set);
				BOOST_FOREACH(PatternFeatureSet_ptr feature_set, separatedFeatureSets){
					// Find additional arguments for this relation/individual; 
					//   this must be done now, while we have access to the PatternFeatureSet
					elfInference->addInferredLocationForEvent(docData, sent_index, feature_set);
					elfInference->addInferredDateForEvent(docData, sent_index, feature_set);
					elfInference->addInferredDateForKBP(docData, sent_index, pattern, feature_set);

					int n_returns = 0;
					size_t n_features = feature_set->getNFeatures();
					SessionLogger::dbg("feat_sets") << "n_features: " << n_features;
					for (size_t i = 0; i < n_features; i++) {
						if (ReturnPatternFeature_ptr rpf = dynamic_pointer_cast<ReturnPatternFeature>(feature_set->getFeature(i))) {
							if (SessionLogger::dbg_or_msg_enabled("feat_sets")) {
								std::string return_label;
								if (rpf->getReturnLabel() != Symbol()) {
									return_label = rpf->getReturnLabel().to_debug_string();
								}
								SessionLogger::dbg("feat_sets") << "pattern: " << pattern->getPatternSetName().to_debug_string()
									<< "; sent_index: " << sent_index << "; feature_set_index: " << feature_set_index
									<< "; feature: " << i << "; " << "<" << return_label << ">";
							}
							// This is a special type of return feature that discourages one from reporting
							//   a particular ValueMention as a date for an event
							if (rpf->getReturnValue(L"tbd").compare(L"bad_date") == 0) {
								SessionLogger::dbg("feat_sets") << "  bad date";
								continue;
							} else if (SessionLogger::dbg_or_msg_enabled("feat_sets")) { // dump
								std::ostringstream ostr;
								PatternReturn_ptr pr = rpf->getPatternReturn();
								pr->dump(ostr, /*indent=*/2);
								SessionLogger::dbg("feat_sets") << ostr.str();
							}
							n_returns++;
						}
					}
					string i_or_r;
					// Based on the number of return features, create an ElfRelation or an ElfIndividual
					if (n_returns >= 2) {
						i_or_r = "relation";
						// Relations have 2 or more returned values that will become args
						try {
							sentence_relations.insert(boost::make_shared<ElfRelation>(docData->getDocTheory(), pattern, feature_set));
						} catch (UnexpectedInputException& error) {
							SessionLogger::info("no_i_or_r_0") << pattern->getPatternSetName() << " failed to create " << i_or_r << std::endl;
							if ((error.getSource() && strlen(error.getSource()) > 0) || 
								(error.getMessage() && strlen(error.getMessage()) > 0)) {
								SessionLogger::info("no_i_or_r_1") << "    " << error.getSource() << ": " << error.getMessage() << std::endl;
							}
						} catch (std::exception& error) {
							SessionLogger::info("no_i_or_r_2") << pattern->getPatternSetName() << " failed to create " << i_or_r << std::endl;
							if (error.what() && strlen(error.what()) > 0) {
								SessionLogger::info("no_i_or_r_3") << "    " << error.what() << std::endl;
							}
						}
					} else if (n_returns == 1) {
						i_or_r = "individual";
						// Individuals come from a single return value
						try {
							sentence_individuals.insert(ElfIndividualFactory::from_feature_set(docData->getDocTheory(), feature_set));
						} catch (UnexpectedInputException& error) {
							SessionLogger::info("no_i_0") << pattern->getPatternSetName() << " failed to create " << i_or_r << std::endl;
							if ((error.getSource() && strlen(error.getSource()) > 0) || 
								(error.getMessage() && strlen(error.getMessage()) > 0)) {
								SessionLogger::info("no_i_1") << "    " << error.getSource() << ": " << error.getMessage() << std::endl;
							}
						} catch (std::exception& error) {
							SessionLogger::info("no_i_2") << pattern->getPatternSetName() << " failed to create " << i_or_r << std::endl;
							if (!error.what() && strlen(error.what()) > 0) {
								SessionLogger::info("no_i_3") << "    " << error.what() << std::endl;
							}
						}
					} else {
						SessionLogger::info("match_but_no_ret") << "Pattern set <" << pattern->getPatternSetName() << ">: match, but no return features" 
							<< std::endl;
					}
				}
			}
			

			// Trade arguments across various relations
			elfInference->pruneDoubleEntitiesFromEvents(docData, sent_index, sentence_relations);
			elfInference->mergeRelationArguments(docData, sent_index, sentence_relations, sentence_individuals);

			BOOST_FOREACH (ElfRelation_ptr rel, sentence_relations) {
				addTemporalAttributes(temporalAttributeAdder, rel, 
						docData, sent_index);
				_relations.insert(rel);
			}

			BOOST_FOREACH (ElfIndividual_ptr ind, sentence_individuals) {
				insert_individual(ind);
			}
		}

		// Look for document pattern matches
		std::vector<PatternFeatureSet_ptr> feature_sets = pm->getDocumentSnippets();
		BOOST_FOREACH(PatternFeatureSet_ptr feature_set, feature_sets) {
			_relations.insert(boost::make_shared<ElfRelation>(docData->getDocTheory(), 
				pattern, feature_set));
		}
	}

	// For each active LearnIt pattern, look for matches in this document
	BOOST_FOREACH(LearnItPattern_ptr pattern, learnit_patterns) {
		// Look for matches in each sentence
	    for (int sent_index = 0; sent_index < docData->getNSentences(); sent_index++) {
			// Get the theory for this sentence
	        SentenceTheory* sent_theory = docData->getSentenceTheory(sent_index);

			// Look at all of the matches for this LearnIt pattern in this sentence by pattern match
            MatchInfo::PatternMatches matches = MatchInfo::findPatternInSentence(docData->getDocTheory(), 
				sent_theory, pattern);
            BOOST_FOREACH(MatchInfo::PatternMatch match, matches) {
                // Create a new relation for this match
				try {
					ElfRelation_ptr relation = boost::make_shared<ElfRelation>(
						docData->getDocTheory(), sent_theory,
						pattern->getTarget(), match, pattern->precision());
					addTemporalAttributes(temporalAttributeAdder, relation,
							docData, sent_index);
					_relations.insert(relation);
				} catch (UnexpectedInputException& error) {
					SessionLogger::info("LEARNIT") << pattern->getName() << " failed to create relation" << std::endl;
					if ((error.getSource() && strlen(error.getSource()) > 0) || 
						(error.getMessage() && strlen(error.getMessage()) > 0)) {
						SessionLogger::info("LEARNIT") << "    " << error.getSource() << ": " << error.getMessage() << std::endl;
					}
				}
            }
        }
	}

	// for each LearnIt2 matcher, let it match against the document
	BOOST_FOREACH(LearnIt2Matcher_ptr matcher, learnit2_matchers) {
		for (int sent_index = 0; sent_index < docData->getNSentences(); sent_index++) {
			SentenceTheory* sent_theory = docData->getSentenceTheory(sent_index);
			MatchInfo::PatternMatches matches = 
				matcher->match(docData->getDocTheory(), _id, sent_theory);
			BOOST_FOREACH(MatchInfo::PatternMatch match, matches) {
				// Create a new relation for this match
				try {
//					_relations.insert(boost::make_shared<ElfRelation>(pattern, docData->getDocTheory(), sent_theory, match));
					ElfRelation_ptr relation = boost::make_shared<ElfRelation>(
						docData->getDocTheory(), sent_theory,
						matcher->target(), match, match.score, true);

					if (!match.source.empty()) {
						relation->add_source(match.source);
					}

					addTemporalAttributes(temporalAttributeAdder,relation,
							docData, sent_index);

					_relations.insert(relation);

				} catch (UnexpectedInputException& error) {
					SessionLogger::info("LEARNIT") << matcher->name() << " failed to create relation" << std::endl;
					if ((error.getSource() && strlen(error.getSource()) > 0) || 
						(error.getMessage() && strlen(error.getMessage()) > 0)) {
						SessionLogger::info("LEARNIT") << "    " << error.getSource() << ": " << error.getMessage() << std::endl;
					}
				}
			}
		}
	}

	// Generate unique IDs for things we're coreferencing ourselves
	boost::hash<ElfRelation_ptr> relation_hasher;
	BOOST_FOREACH(ElfRelation_ptr relation, _relations) {
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
			std::wstring type_uri = arg->get_type()->get_value();
			if (type_uri == L"nfl:NFLGame" || boost::starts_with(type_uri, L"ic:PharmaceuticalSubstance") || boost::starts_with(type_uri, L"ic:PhysiologicalCondition")) {
				// Extract the prefixless type
				std::wstring local_type_name = type_uri.substr(type_uri.find_first_of(L":") + 1);

				// Generate a new URI using the relation hash and the document ID
				std::wstringstream uri;
				uri << "bbn:" << local_type_name << "-" << _id << "-" << relation_hasher(relation);
				ElfIndividual_ptr individual = arg->get_individual();
				if (individual.get() != NULL) {
					//SessionLogger::info("LEARNIT") << individual->get_generated_uri() << " -> " << uri.str() << std::endl;
					individual->set_generated_uri(uri.str());
				}
			}
		}
	}
	boost::hash<std::wstring> string_hasher;
	boost::hash<int> int_hasher;
	BOOST_FOREACH(ElfIndividual_ptr individual, _individuals) {
		std::wstring type_uri = individual->get_type()->get_value();
		if (boost::starts_with(type_uri, L"ic:PharmaceuticalSubstance") || boost::starts_with(type_uri, L"ic:PhysiologicalCondition")) {
			// Extract the prefixless type
			std::wstring local_type_name = type_uri.substr(type_uri.find_first_of(L":") + 1);

			// Calculate a hash based on just the individual's type
			size_t individual_hash = string_hasher(type_uri);
			EDTOffset start, end;
			individual->get_type()->get_offsets(start, end);
			individual_hash ^= int_hasher(start.value());
			individual_hash ^= int_hasher(end.value());
			ElfString_ptr name_or_desc = individual->get_name_or_desc();
			if (name_or_desc.get() != NULL) {
				individual_hash ^= string_hasher(name_or_desc->get_value());
				name_or_desc->get_offsets(start, end);
				individual_hash ^= int_hasher(start.value());
				individual_hash ^= int_hasher(end.value());
			}

			// Generate a new URI using the relation hash and the document ID
			std::wstringstream uri;
			uri << "bbn:" << local_type_name << "-" << _id << "-" << individual_hash;
			//SessionLogger::info("LEARNIT") << individual->get_generated_uri() << " -> " << uri.str() << std::endl;
			individual->set_generated_uri(uri.str());
		}
	}
}



void ElfDocument::addTemporalAttributes(TemporalAttributeAdder_ptr temporalAttributeAdder,
		ElfRelation_ptr rel, EIDocData_ptr docData, int sent_index) 
{
	try {
		if (temporalAttributeAdder) {
			temporalAttributeAdder->addTemporalAttributes(rel,
					docData->getDocTheory(), sent_index);
		}
	} catch (UnexpectedInputException& e) {
		SessionLogger::err("temp_error") << "Error during temporal "
			<< "processing: \n\tSource: " << e.getSource()
			<< "\n\tMessage: " << e.getMessage();
	}
}

/**
 * Reads this document as an ELF XML file from the specified file.
 *
 * XMLPlatformUtils::Initialize() must be called before this method.
 *
 * @param input_file The path to the ELF XML file to read.
 *
 * @author nward@bbn.com
 * @date 2010.08.30
 **/
ElfDocument::ElfDocument(const std::string & input_file) {
	// Read the file and get the root element
	DOMDocument* xml = XMLUtil::loadXercesDOMFromFilename(input_file.c_str());
	if (xml == NULL)
		throw std::runtime_error("ElfDocument::ElfDocument(std::string): Could not parse XML");
	DOMElement* doc = xml->getDocumentElement();
	if (doc == NULL)
		throw std::runtime_error("ElfDocument::ElfDocument(std::string): Could not get <doc>");

	// Read the document metadata attributes
    _id = SXMLUtil::getAttributeAsStdWString(doc, "id");
	if (_id == L"")
		throw std::runtime_error("ElfDocument::ElfDocument(std::string): No id attribute specified on <doc>");
	_version = SXMLUtil::getAttributeAsStdWString(doc, "elf-version");
	if (_version == L"")
		throw std::runtime_error("ElfDocument::ElfDocument(std::string): No elf-version attribute specified on <doc>");
	_source = SXMLUtil::getAttributeAsStdWString(doc, "source");
	if (_source == L"")
		throw std::runtime_error("ElfDocument::ElfDocument(std::string): No source attribute specified on <doc>");
	_contents = SXMLUtil::getAttributeAsStdWString(doc, "contents");
	if (_contents == L"")
		throw std::runtime_error("ElfDocument::ElfDocument(std::string): No contents attribute specified on <doc>");

	// Check how we're handling offsets (currently everyone is correctly inclusive)
	bool exclusive_end_offsets = false;

	// Read all of the <individual>s in this document
	//   We read these first so that they're available for lookup when reading <relation>s
	DOMNodeList* individual_nodes = SXMLUtil::getNodesByTagName(doc, "individual");
	ElfIndividualSet individuals;
	for (XMLSize_t r = 0; r < individual_nodes->getLength(); r++) {
		DOMElement* individual_element = dynamic_cast<DOMElement*>(individual_nodes->item(r));
		try {
			ElfIndividual_ptr individual = boost::make_shared<ElfIndividual>(individual_element, _id, _source, exclusive_end_offsets);
			if (individual.get() != NULL)
				// Store the read individual
				individuals.insert(individual);
			else
				throw std::runtime_error("Returned ElfIndividual was NULL");
		} catch (std::exception& e) {
			SessionLogger::info("LEARNIT") << "ElfDocument::ElfDocument(std::string): bad <individual>, skipping: " << e.what() << std::endl;
		}
	}

	// Read all of the <relation>s in this document
	DOMNodeList* relation_nodes = SXMLUtil::getNodesByTagName(doc, "relation");
	for (XMLSize_t r = 0; r < relation_nodes->getLength(); r++) {
		DOMElement* relation_element = dynamic_cast<DOMElement*>(relation_nodes->item(r));
		try {
			ElfRelation_ptr relation = boost::make_shared<ElfRelation>(relation_element, individuals, exclusive_end_offsets);
			if (relation.get() != NULL)
				_relations.insert(relation);
			else
				throw std::runtime_error("Returned ElfRelation was NULL");
		} catch (std::exception& e) {
			SessionLogger::info("LEARNIT") << "ElfDocument::ElfDocument(std::string): bad <relation>, skipping: " << e.what() << std::endl;
		}
	}
}

/**
 * Copy constructor.
 *
 * @param other ElfDocument to deep copy from.
 *
 * @author nward@bbn.com
 * @date 2010.08.31
 **/
ElfDocument::ElfDocument(const ElfDocument_ptr other) {
	// Copy the other document's metadata
	_id = other->_id;
	_version = other->_version;
	_source = other->_source;
	_contents = other->_contents;

	// Deep copy the other document's relations
	BOOST_FOREACH(ElfRelation_ptr relation, other->_relations) {
		_relations.insert(boost::make_shared<ElfRelation>(relation));
	}

	// Deep copy the other document's individuals
	BOOST_FOREACH(ElfIndividual_ptr individual, other->_individuals) {
		if (individual.get() != NULL)
			_individuals.insert(boost::make_shared<ElfIndividual>(individual));
	}
}

/**
 * Merges multiple documents into one, applying coreference.
 *
 * @param doc_theory The Serif document theory.
 * @param others The other ElfDocuments to merge.
 *
 * @author nward@bbn.com
 * @date 2010.08.31
 **/
ElfDocument::ElfDocument(const DocTheory* doc_theory, const std::vector<ElfDocument_ptr> & others) {
	// Store the document ID
	_id = std::wstring(doc_theory->getDocument()->getName().to_string());

	// Initialize the ELF metadata
	_version = L"2.2";
	_source = L"BBN PredFinder Merge";

	// Make sure the documents we're merging are all the same type of ELF
	_contents = L"";
	BOOST_FOREACH(ElfDocument_ptr other, others) {
		if (_contents == L"")
			_contents = other->_contents;
		else if (_contents != other->_contents)
			throw std::runtime_error("ElfDocument::ElfDocument(DistillationDoc_ptr, ElfDocument_ptr): "
			"cannot merge documents of mixed ELF varieties");
	}

	// Merge all of the documents into one
	BOOST_FOREACH(ElfDocument_ptr other, others) {
		// Store a copy of each relation
		BOOST_FOREACH(ElfRelation_ptr other_relation, other->_relations) {
			// Copy the relation
			ElfRelation_ptr relation = boost::make_shared<ElfRelation>(doc_theory, other_relation);
			if (relation.get() == NULL)
				continue;

			// If the relation didn't specify a source, copy in the more general one from its containing document
			if (relation->get_source() == L"")
				if (other->_source != L"BBN")
					relation->set_source(other->_source);

			// Store this relation (no individual mapping yet)
			_relations.insert(relation);
		}

		// Store or merge a copy of each individual
		BOOST_FOREACH(ElfIndividual_ptr other_individual, other->_individuals) {
			// Check if we're merging or not
			ElfIndividual_ptr individual = boost::make_shared<ElfIndividual>(doc_theory, other_individual);
			if (individual.get() == NULL)
				continue;

			// Store this unmerged individual
			_individuals.insert(individual);
		}
	}
}

/**
 * Takes a list of relations to remove from this
 * document. All comparisons are made using pointer
 * equality; it's assumed that these shared pointer
 * references were output from get_relations() that
 * has been filtered in some way.
 *
 * @param relations The list of relations to remove.
 *
 * @author nward@bbn.com
 * @date 2010.10.18
 **/
void ElfDocument::remove_relations(const std::set<ElfRelation_ptr> & relations_to_remove) {
	BOOST_FOREACH(ElfRelation_ptr relation_to_remove, relations_to_remove) {
		std::set<ElfRelation_ptr>::iterator relation_i = _relations.find(relation_to_remove);
		if (relation_i != _relations.end()) {
			_relations.erase(relation_i);
		}
	}
}

/**
 * Takes a list of individuals to remove from this
 * document. All comparisons are made using both string
 * matching of <name>/<desc> and <type>s and exact and
 * complete ID set matching. Removes any relations
 * that contain <arg>s that contain those individuals.
 *
 * @param relations The list of individuals to remove.
 *
 * @author nward@bbn.com
 * @date 2011.09.21
 **/
void ElfDocument::remove_individuals(const ElfIndividualSet & individuals_to_remove) {
	// Look for matching unreferenced individuals
	ElfIndividualSet matched_doc_individuals_to_remove;
	BOOST_FOREACH(ElfIndividual_ptr individual, _individuals) {
		BOOST_FOREACH(ElfIndividual_ptr individual_to_remove, individuals_to_remove) {
			// Check if this document individual has identical IDs and provenance
			if (individual->compare(*individual_to_remove) == 0 && individual->has_equal_ids(individual_to_remove)) {
				// Mark this document individual for removal
				matched_doc_individuals_to_remove.insert(individual);
				break;
			}
		}
	}

	// Actually remove document individuals that we found, so we don't invalidate iterators when searching by value above
	BOOST_FOREACH(ElfIndividual_ptr matched_doc_individual_to_remove, matched_doc_individuals_to_remove) {
		ElfIndividualSet::iterator individual_i = _individuals.find(matched_doc_individual_to_remove);
		if (individual_i != _individuals.end()) {
			_individuals.erase(individual_i);
		}
	}

	// Look for any relations containing matching referenced individuals
	std::set<ElfRelation_ptr> relations_to_remove;
	BOOST_FOREACH(ElfRelation_ptr relation, _relations) {
		bool matched = false;
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
			ElfIndividual_ptr individual = arg->get_individual();
			if (individual.get() == NULL)
				continue;
			BOOST_FOREACH(ElfIndividual_ptr individual_to_remove, individuals_to_remove) {
				// Check if this relation arg individual has identical IDs and provenance
				if (individual->compare(*individual_to_remove) == 0 && individual->has_equal_ids(individual_to_remove)) {
					// Mark this relation for removal
					relations_to_remove.insert(relation);
					matched = true;
					break;
				}
			}
			if (matched)
				break;
		}
	}

	// Actually remove relations that we found by pointer
	remove_relations(relations_to_remove);
}

/**
 * Public accessor to get all individuals, or individuals
 * having a particular type. Returns copies of the underlying
 * ElfIndividuals, so changing any of the return values doesn't
 * change the contents of the document.
 *
 * @param search_type The optional ontology type to find
 * individuals by; if not specified, all individuals are returned.
 * @return A dictionary of individuals by ID.
 *
 * @author nward@bbn.com
 * @date 2010.08.18
 **/
ElfIndividualSet ElfDocument::get_individuals_by_type(const std::wstring & search_type) const {
	// Selected individuals
	ElfIndividualSet individuals;

	// Collect all of the individuals in the document (both in relations and standalone)
	// that have the specified type.
	BOOST_FOREACH(ElfRelation_ptr relation, _relations) {
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
			ElfIndividual_ptr individual = arg->get_individual();
			if (individual.get() != NULL && !individual->has_value() && (search_type == L"" || individual->has_type(search_type))) {
				// Add copy of a matching individual
				individuals.insert(boost::make_shared<ElfIndividual>(individual));
			}
		}
	}
	BOOST_FOREACH(ElfIndividual_ptr individual, _individuals) {
		// Check if we're searching for a particular type
		if (search_type == L"" || individual->has_type(search_type))
			// Add copy of a matching individual
			individuals.insert(boost::make_shared<ElfIndividual>(individual));
	}

	// Done
	return individuals;
}

/**
 * Public accessor to get an individual with the specified
 * generated URI, intended for use by Blitz coreference. Even
 * if multiple unmerged individuals have the same URI, they'll
 * have the same content, since the generated URI comes from a hash.
 *
 * @param generated_uri The URI to find an individual instance for
 * @return A copy of an unmerged individual with the specified URI.
 *
 * @author nward@bbn.com
 * @date 2011.09.22
 **/
ElfIndividual_ptr ElfDocument::get_individual_by_generated_uri(const std::wstring & generated_uri) const {
	// Find an individual in the document or relations with the specified URI
	BOOST_FOREACH(ElfIndividual_ptr individual, _individuals) {
		if (!individual->has_value() && individual->get_generated_uri() == generated_uri) {
			return individual;
		}
	}
	BOOST_FOREACH(ElfRelation_ptr relation, _relations) {
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
			ElfIndividual_ptr individual = arg->get_individual();
			if (individual.get() != NULL && !individual->has_value() && individual->get_generated_uri() == generated_uri) {
				return individual;
			}
		}
	}

	// No match
	return ElfIndividual_ptr();
}

/**
 * Public accessor to get all merged individuals, or individuals
 * having a particular type. Returns merges of the underlying
 * ElfIndividuals, so changing any of the return values doesn't
 * change the contents of the document.
 *
 * @param search_type The optional ontology type to find
 * individuals by; if not specified, all individuals are returned.
 * @return A dictionary of individuals by ID.
 *
 * @author nward@bbn.com
 * @date 2011.08.05
 **/
ElfIndividualSet ElfDocument::get_merged_individuals_by_type(const std::wstring & search_type) const {
	// Selected individuals
	ElfIndividualSet merged_individuals = get_merged_individuals();

	// If no search type, just return the full merge
	if (search_type == L"")
		return merged_individuals;

	// Collect the merged individuals that have at least one instance of the specified type
	ElfIndividualSet individuals;
	BOOST_FOREACH(ElfIndividual_ptr individual, merged_individuals) {
		if (individual->has_type(search_type))
			individuals.insert(individual);
	}

	// Done
	return individuals;
}

/**
 * Public accessor to get all individuals, or individuals
 * having a particular type. Returns copies of the underlying
 * ElfIndividuals, so changing any of the return values doesn't
 * change the contents of the document. Individuals are organized
 * by their containing sentence.
 *
 * @param search_type The optional ontology type to find
 * individuals by; if not specified, all individuals are returned.
 * @return A vector of sorted sets; each vector bucket corresponds
 * to the sentence of the same index, containing just those individuals
 * whose type assertion offsets are within the bounds of that sentence.
 *
 * @author nward@bbn.com
 * @date 2011.01.06
 **/
std::vector<ElfIndividualSet> ElfDocument::get_individuals_by_type_and_sentence(const DocTheory* doc_theory, 
																				const std::wstring & search_type) const 
{
	// Selected individuals
	std::vector<ElfIndividualSet> individuals_by_sentence;
	individuals_by_sentence.resize(doc_theory->getNSentences());

	// Loop over all of the individuals that have the specified type and sort them by sentence
	ElfIndividualSet individuals = get_merged_individuals_by_type(search_type);
	BOOST_FOREACH(ElfIndividual_ptr individual, individuals) {
		// Find the sentence containing the individual's type provenance
		int sent_no, start_token, end_token;
		EDTOffset start_offset, end_offset;
		individual->get_spanning_offsets(start_offset, end_offset);
		bool token_align = MainUtilities::getSerifStartEndTokenFromCharacterOffsets(doc_theory, start_offset, end_offset, sent_no, 
			start_token, end_token);

		// Only sentence-level cluster individuals we can align
		if (token_align)
			// Don't need to make a copy, since the returns of get_individuals_by_type are already copies
			individuals_by_sentence[sent_no].insert(individual);
	}

	// Done
	return individuals_by_sentence;
}

/**
 * Instead of generating all possible merged individuals,
 * generate just the one created from all of the individuals
 * in the document whose best URI is the one specified. Intended
 * for use during inference, when it might be worthwhile to filter
 * based on what a given individual will be merged into.
 *
 * @param uri The best URI to match against; probably returned from
 * ElfIndividual::get_best_uri() with no docid argument (that is, a
 * document-unique but not necessarily globally-unique URI).
 * @return The individual generated from the merging constructor.
 *
 * @author nward@bbn.com
 * @date 2011.06.09
 **/
ElfIndividual_ptr ElfDocument::get_merged_individual_by_uri(const std::wstring & uri) const {
	// Collect all of the individuals in the document (both in relations and standalone)
	// whose best URI matches the search URI.
	ElfIndividualSet individuals_to_merge;
	BOOST_FOREACH(ElfRelation_ptr relation, _relations) {
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
			ElfIndividual_ptr individual = arg->get_individual();
			if (individual.get() != NULL && !individual->has_value() && individual->get_best_uri() == uri) {
				individuals_to_merge.insert(individual);
			}
		}
	}
	BOOST_FOREACH(ElfIndividual_ptr individual, _individuals) {
		if (individual.get() != NULL && !individual->has_value() && individual->get_best_uri() == uri) {
			individuals_to_merge.insert(individual);
		}
	}

	// Return the merged individual constructed from matching individuals
	return boost::make_shared<ElfIndividual>(individuals_to_merge);
}

/**
 * Generate all possible merged individuals. Intended only
 * for use during document writing. Ignores ElfIndividuals.
 *
 * @return The individuals generated from the merging constructor,
 * one per best URI in the set of all best URIs present for individuals
 * in the document. These individuals may have multiple <type> assertions.
 *
 * @author nward@bbn.com
 * @date 2011.06.10
 **/
ElfIndividualSet ElfDocument::get_merged_individuals(void) const {
	// Collect all of the individuals in the document (both in relations and standalone)
	// by best URI.
	std::map<std::wstring, ElfIndividualSet> individuals_to_merge_by_uri;
	std::pair<std::map<std::wstring, ElfIndividualSet>::iterator, bool> individual_insert;
	BOOST_FOREACH(ElfRelation_ptr relation, _relations) {
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
			ElfIndividual_ptr individual = arg->get_individual();
			if (individual.get() != NULL && !individual->has_value()) {
				// Store this individual in the set of individuals that share its best URI
				individual_insert = individuals_to_merge_by_uri.insert(std::pair<std::wstring, ElfIndividualSet>(individual->get_best_uri(), ElfIndividualSet()));
				individual_insert.first->second.insert(individual);
			}
		}
	}
	BOOST_FOREACH(ElfIndividual_ptr individual, _individuals) {
		if (individual.get() != NULL && !individual->has_value()) {
			// Store this individual in the set of individuals that share its best URI
			individual_insert = individuals_to_merge_by_uri.insert(std::pair<std::wstring, ElfIndividualSet>(individual->get_best_uri(), ElfIndividualSet()));
			individual_insert.first->second.insert(individual);
		}
	}

	// Merge each set of individuals that share a best URI into a single merged individual with that best URI
	//   BOOST_FOREACH didn't like iterating over these templated types, so just do an old for iterator
	ElfIndividualSet merged_individuals;
	for (std::map<std::wstring, ElfIndividualSet>::iterator i = individuals_to_merge_by_uri.begin(); i != individuals_to_merge_by_uri.end(); i++) {
		// Only merge if there's actually more than one
		if (i->second.size() > 1) {
			merged_individuals.insert(boost::make_shared<ElfIndividual>(i->second));
		} else {
			merged_individuals.insert(*(i->second.begin()));
		}
	}
	return merged_individuals;
}

/**
 * Public accessor to get all relations, or relations containing
 * an arg referring to a particular individual. Returns copies
 * of the underlying ElfRelations, so changing any of the return
 * values doesn't change the contents of the document.
 *
 * @param search_individual The individual to find
 * relations by; if NULL, all relations are returned.
 * @return A dictionary of relations by name.
 *
 * @author nward@bbn.com
 * @date 2010.08.18
 **/
ElfRelationMap ElfDocument::get_relations_by_individual(const ElfIndividual_ptr search_individual) const {
	// Construct a search arg from the specified individual, if necessary
	ElfRelationArg_ptr search_arg;
	if (search_individual.get() != NULL) {
		// Create a search arg containing this individual
		search_arg = boost::make_shared<ElfRelationArg>(L"", search_individual);
	}
	else{
		ElfRelationMap empty_map;
		return empty_map;
	}
	// Return the results of the arg search
	return get_relations_by_arg(search_arg);
}

/**
 * Public accessor to get all relations, or relations containing
 * a match for a particular arg. Returns copies
 * of the underlying ElfRelations, so changing any of the return
 * values doesn't change the contents of the document.
 *
 * @param search_arg The arg to find relations by;
 * if NULL, all relations are returned.
 * @param seen_relations A list of relations we've already
 * seen in the recursion, to avoid arg reference loops; initially
 * empty. Shares pointers with the result list, but is discarded.
 * @return A dictionary of relations by name.
 *
 * @author nward@bbn.com
 * @date 2010.08.18
 **/
ElfRelationMap ElfDocument::get_relations_by_arg(const ElfRelationArg_ptr search_arg) const {
	// Copies of any matching relations we found
	ElfRelationMap matching_relations;

	// Loop over all of the relations in this document
	BOOST_FOREACH(ElfRelation_ptr relation, _relations) {
		// Get a matching arg from this relation, if there is one
		ElfRelationArg_ptr matching_arg = relation->get_matching_arg(search_arg);
		if (matching_arg.get() != NULL) {
			// Add this found relation to the return list
			ElfRelation_ptr relation_copy = boost::make_shared<ElfRelation>(relation);
			std::pair<ElfRelationMap::iterator, bool> relation_match_insert = matching_relations.insert(ElfRelationMap::value_type(relation_copy->get_name(), std::vector<ElfRelation_ptr>()));
			relation_match_insert.first->second.push_back(relation_copy);
		}
	}

	// Done
	return matching_relations;
}

bool ElfDocument::individual_in_relation(const ElfIndividual_ptr search_individual,
		const std::wstring& relation_type)
{
	if (search_individual) {
		ElfRelationArg_ptr searchArg = boost::make_shared<ElfRelationArg>(L"",
				search_individual);
		BOOST_FOREACH(ElfRelation_ptr relation, _relations) {
			if (relation->get_name() == relation_type) {
				if (relation->get_matching_arg(searchArg)) {
					return true;
				}
			}
		}
	}

	return false;
}

/**
 * For a specified type, collects all of the individuals
 * that have that type, collects all of the relations
 * relevant to those individuals, and then clusters
 * individuals together.
 *
 * @param domain_prefix The domain we're in, used to determine
 * clustering strategy.
 * @param coreferent_types A list of individuals that should be
 * candidates for clustering.
 *
 * @author nward@bbn.com
 * @date 2010.08.19
 **/
void ElfDocument::do_document_level_individual_coreference(const std::wstring& domain_prefix, 
														   const std::vector<std::wstring> & coreferent_types) {
	// Initialize the individual coreference engine
	ElfIndividualCoreference_ptr coref_engine = boost::make_shared<ElfIndividualCoreference>(domain_prefix);

	// Get all of the individuals that have the specified types (or all individuals if none)
	ElfIndividualSet individuals_to_coref;
	if (coreferent_types.size() > 0) {
		BOOST_FOREACH(std::wstring coreferent_type, coreferent_types) {
			if (coreferent_type != L"") {
				ElfIndividualSet typed_individuals_to_coref = get_merged_individuals_by_type(coreferent_type);
				BOOST_FOREACH(ElfIndividual_ptr typed_individual, typed_individuals_to_coref) {
					individuals_to_coref.insert(typed_individual);
				}
			}
		}
	}
	//SessionLogger::info("LEARNIT") << "individuals: " << individuals_to_coref.size() << std::endl;

	// For each individual, collect all of its relevant relations
	BOOST_FOREACH(ElfIndividual_ptr individual, individuals_to_coref) {
		// Get the relations for this individual (recursive)
		ElfRelationMap relations_for_individual = get_relations_by_individual(individual);
		//SessionLogger::info("LEARNIT") << individual_pair.first << " relation predicates: " << relations_for_individual.size() << std::endl;

		// Create a new cluster member and add it
		coref_engine->add_cluster_member(individual, relations_for_individual);
	}

	// Do the meat of the coreference
	coref_engine->combine_clusters();

	// Get the map of old individual generated URIs to new cluster generated URIs (reflecting cluster membership)
	ElfIndividualUriMap clustered_individual_uri_map = coref_engine->get_individual_to_cluster_uri_map(_id);
	//SessionLogger::info("LEARNIT") << "Clustered " << clustered_individual_uri_map.size() << " Individuals" << std::endl;
	//BOOST_FOREACH(ElfIndividualUriMap::value_type individual_pair, clustered_individual_uri_map) {
	//	SessionLogger::info("LEARNIT") << individual_pair.first << " -> " << individual_pair.second << std::endl;
	//}

	// Update all of the changed individual URIs
	replace_individual_uris(clustered_individual_uri_map);
}


/**
 * For a specified type, collects all of the individuals
 * that have that type, collects all of the relations
 * relevant to those individuals, and then clusters
 * individuals together, but only within a sentence.
 *
 * @param doc_info The Serif document contents and metadata.
 * @param domain_prefix The domain we're in, used to determine
 * clustering strategy.
 * @param coreferent_types A list of individuals that should be
 * candidates for clustering.
 *
 * @author nward@bbn.com
 * @date 2011.01.06
 **/
void ElfDocument::do_sentence_level_individual_coreference(const DocTheory* doc_theory, 
														   const std::wstring& domain_prefix, 
														   const std::vector<std::wstring> & coreferent_types) {
	// Get all of the individuals that have the specified types in each sentence (or all individuals if none)
	std::vector<ElfIndividualSet> individuals_to_coref_by_sentence;
	individuals_to_coref_by_sentence.resize(doc_theory->getNSentences());
	if (coreferent_types.size() > 0) {
		BOOST_FOREACH(std::wstring coreferent_type, coreferent_types) {
			if (coreferent_type != L"") {
				std::vector<ElfIndividualSet> typed_individuals_to_coref_by_sentence = get_individuals_by_type_and_sentence(doc_theory, coreferent_type);
				for (int i = 0; i < doc_theory->getNSentences(); i++) {
					BOOST_FOREACH(ElfIndividual_ptr typed_individual, typed_individuals_to_coref_by_sentence[i]) {
						individuals_to_coref_by_sentence[i].insert(typed_individual);
					}
				}
			}
		}
	}

	// Do coreference sentence by sentence
	ElfIndividualUriMap clustered_individual_uri_map;
	BOOST_FOREACH(ElfIndividualSet individuals_to_coref, individuals_to_coref_by_sentence) {
		// Initialize the individual coreference engine in sentence mode
		ElfIndividualCoreference_ptr coref_engine = boost::make_shared<ElfIndividualCoreference>(domain_prefix, true);
		
		// For each individual, collect all of its relevant relations
		//SessionLogger::info("LEARNIT") << "individuals: " << individuals_to_coref.size() << std::endl;
		BOOST_FOREACH(ElfIndividual_ptr individual, individuals_to_coref) {
			// Get the relations for this individual (recursive)
			ElfRelationMap relations_for_individual = get_relations_by_individual(individual);
			//SessionLogger::info("LEARNIT") << individual->get_best_uri() << " relation predicates: " << relations_for_individual.size() << std::endl;

			// Create a new cluster member and add it
			coref_engine->add_cluster_member(individual, relations_for_individual);
		}

		// Do the meat of the coreference
		coref_engine->combine_clusters();

		// Get the map of old individual IDs to new individuals (reflecting cluster membership)
		// and add them to the document map
		ElfIndividualUriMap sentence_clustered_individual_uri_map = coref_engine->get_individual_to_cluster_uri_map(_id);
		BOOST_FOREACH(ElfIndividualUriMap::value_type individual_uri_pair, sentence_clustered_individual_uri_map) {
			// Make sure we haven't assigned a mapping for this individual already
			if (clustered_individual_uri_map.find(individual_uri_pair.first) == clustered_individual_uri_map.end()) {
				clustered_individual_uri_map.insert(individual_uri_pair);
			} else {
				SessionLogger::info("LEARNIT") << "individual " << individual_uri_pair.first << " already mapped, ignoring" << std::endl;
			}
		}
	}

	//SessionLogger::info("LEARNIT") << "Clustered " << clustered_individual_uri_map.size() << " Individuals" << std::endl;
	//BOOST_FOREACH(ElfIndividualUriMap::value_type individual_pair, clustered_individual_uri_map) {
	//	SessionLogger::info("LEARNIT") << individual_pair.first << " -> " << individual_pair.second << std::endl;
	//}

	// Update all of the changed individual URIs
	replace_individual_uris(clustered_individual_uri_map);
}

/**
 * Updates all individuals in this document by replacing
 * matching generated individual URIs with the specified
 * mapped value for that URI. Operates in-place.
 *
 * @param individual_uri_map Pairs of URI strings, from
 * original URI to replacement URI.
 *
 * @author nward@bbn.com
 * @date 2011.06.22
 **/
void ElfDocument::replace_individual_uris(const ElfIndividualUriMap & individual_uri_map) {
	// Print replacements
	//std::cout << "Replacing " << individual_uri_map.size() << " individual URIs" << std::endl;
	//BOOST_FOREACH(ElfIndividualUriMap::value_type uri_pair, individual_uri_map) {
	//	std::cout << uri_pair.first << " -> " << uri_pair.second << std::endl;
	//}

	// Check all of the individuals in the document (both in relations and standalone)
	BOOST_FOREACH(ElfRelation_ptr relation, _relations) {
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
			ElfIndividual_ptr individual = arg->get_individual();
			if (individual.get() != NULL && !individual->has_value()) {
				ElfIndividualUriMap::const_iterator uri_replacement = individual_uri_map.find(individual->get_best_uri(_id));
				if (uri_replacement != individual_uri_map.end()) {
					// If there was a replacement specified for this URI, replace it
					individual->set_generated_uri(uri_replacement->second);
				}
			}
		}
	}
	BOOST_FOREACH(ElfIndividual_ptr individual, _individuals) {
		if (individual.get() != NULL && !individual->has_value()) {
			ElfIndividualUriMap::const_iterator uri_replacement = individual_uri_map.find(individual->get_best_uri(_id));
			if (uri_replacement != individual_uri_map.end()) {
				// If there was a replacement specified for this URI, replace it
				individual->set_generated_uri(uri_replacement->second);
			}
		}
	}
}

/**
 * Updates all individuals in this document by adding the
 * coref URI in the specified map to an individual that has
 * the matching generated URI. Based on replace_individual_uris.
 *
 * @param individual_uri_map Pairs of URI strings, from
 * original generated URI to new coreference URI.
 *
 * @author nward@bbn.com
 * @date 2011.09.22
 **/
void ElfDocument::add_individual_coref_uris(const ElfIndividualUriMap & individual_uri_map) {
	// Print replacements
	//std::cout << "Replacing " << individual_uri_map.size() << " individual URIs" << std::endl;
	//BOOST_FOREACH(ElfIndividualUriMap::value_type uri_pair, individual_uri_map) {
	//	std::cout << uri_pair.first << " -> " << uri_pair.second << std::endl;
	//}

	// Check all of the individuals in the document (both in relations and standalone)
	BOOST_FOREACH(ElfRelation_ptr relation, _relations) {
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
			ElfIndividual_ptr individual = arg->get_individual();
			if (individual.get() != NULL && !individual->has_value()) {
				ElfIndividualUriMap::const_iterator coreferent_uri = individual_uri_map.find(individual->get_generated_uri());
				if (coreferent_uri != individual_uri_map.end()) {
					// If there was a coreferent URI specified for this URI, use it
					individual->set_coref_uri(coreferent_uri->second);
				}
			}
		}
	}
	BOOST_FOREACH(ElfIndividual_ptr individual, _individuals) {
		if (individual.get() != NULL && !individual->has_value()) {
			ElfIndividualUriMap::const_iterator coreferent_uri = individual_uri_map.find(individual->get_generated_uri());
			if (coreferent_uri != individual_uri_map.end()) {
				// If there was a coreferent URI specified for this URI, use it
				individual->set_coref_uri(coreferent_uri->second);
			}
		}
	}
}

/**
 * Convenience method that wraps the functionality
 * of std::map for storing ElfIndividuals in _individuals.
 *
 * @param individual The individual to be inserted; a deep
 * copy is made to prevent unexpected pointer sharing, and
 * all memory is handled by boost::shared_ptr.
 *
 * @author nward@bbn.com
 * @date 2010.06.23
 **/
void ElfDocument::insert_individual(ElfIndividual_ptr individual) {
	// Make a deep copy of this individual, so we don't modify types in the wrong context
	_individuals.insert(boost::make_shared<ElfIndividual>(individual));
}

/**
 * Splits any ElfRelations in this document that have
 * multiple arguments with the same role. The effect is
 * multiplicative, in that the full combinatorics are
 * expanded. The original relation is removed.
 *
 * Intended for use during S-ELF macro conversion.
 * Operates in-place. For the most part, due to the way
 * patterns are written, this will mostly affect "binary"
 * predicates that have multiple different object args.
 *
 * @author nward@bbn.com
 * @date 2010.10.29
 **/
void ElfDocument::split_duplicate_role_relations() {
	// Multiplex any relation args that have the same role
	std::set<ElfRelation_ptr> relations_to_remove;
	std::set<ElfRelation_ptr> relations_to_add;
	BOOST_FOREACH(ElfRelation_ptr relation, _relations) {
		// Track what splitting/deduplication was done
		bool duplicate_args_present = false;
		std::vector<std::vector<ElfRelationArg_ptr> > split_arg_subsets;

		// Collect all of the args for this relation by role
		ElfRelationArgMap args = relation->get_arg_map();
		BOOST_FOREACH(ElfRelationArgMap::value_type args_by_role, args) {
			// Is this arg unique for its role, or a temporal arg?
			if (args_by_role.second.size() == 1 || boost::starts_with(args_by_role.first, L"t:")) { 
				// Is this the first arg we've tried splitting?
				if (split_arg_subsets.size() == 0) {
					// Start a new arg subset with just this arg
					std::vector<ElfRelationArg_ptr> split_arg_subset;
					split_arg_subset.push_back(args_by_role.second[0]);
					split_arg_subsets.push_back(split_arg_subset);
				} else if (split_arg_subsets.size() == 1) {
					// Add this arg to the only existing arg subset
					//   This is the most common case, since most args are unique by role
					split_arg_subsets[0].push_back(args_by_role.second[0]);
				} else {
					// Add this unique-by-role arg to every existing arg subset
					for (size_t i = 0; i < split_arg_subsets.size(); i++) {
						split_arg_subsets[i].push_back(args_by_role.second[0]);
					}
				}
			} else {
				// At least one role in this relation had real or false duplicates, so we need to update it
				duplicate_args_present = true;

				// Remove strictly duplicate args with the same role
				//   We can't use compare() because it is looser on individual values for sorting
				std::vector<ElfRelationArg_ptr> unique_args_with_same_role;
				BOOST_FOREACH(ElfRelationArg_ptr arg, args_by_role.second) {
					// Check if we've already found an arg with the same provenance and individual
					bool found = false;
					BOOST_FOREACH(ElfRelationArg_ptr unique_arg, unique_args_with_same_role) {
						if (arg->get_start() == unique_arg->get_start() &&
							arg->get_end() == unique_arg->get_end() &&
							arg->get_text() == unique_arg->get_text()) {
							if (arg->get_individual().get() != NULL && unique_arg->get_individual().get() != NULL) {
								if (arg->get_individual()->has_value() && unique_arg->get_individual()->has_value()) {
									if (arg->get_individual()->get_value() == unique_arg->get_individual()->get_value()) {
										found = true;
										break;
									}
								} else {
									if (arg->get_individual()->get_best_uri() == unique_arg->get_individual()->get_best_uri()) {
										found = true;
										break;
									}
								}
							}
						}
					}

					// Only collect the arg if we haven't seen it before
					if (!found)
						unique_args_with_same_role.push_back(arg);
				}

				// Is this the first arg we've tried splitting?
				if (split_arg_subsets.size() == 0) {
					// Start a subset for each instance of this arg
					BOOST_FOREACH(ElfRelationArg_ptr arg, unique_args_with_same_role) {
						std::vector<ElfRelationArg_ptr> split_arg_subset;
						split_arg_subset.push_back(arg);
						split_arg_subsets.push_back(split_arg_subset);
					}
				} else {
					// Generate the cross product of the current arg subsets and all
					// the args that have this role
					std::vector<std::vector<ElfRelationArg_ptr> > multiplied_arg_subsets;
					BOOST_FOREACH(ElfRelationArg_ptr arg, unique_args_with_same_role) {
						std::vector<ElfRelationArg_ptr> split_arg_subset;
						BOOST_FOREACH(split_arg_subset, split_arg_subsets) {
							// Copy the current arg subset and add the current arg
							std::vector<ElfRelationArg_ptr> multiplied_arg_subset(split_arg_subset);
							multiplied_arg_subset.push_back(arg);
							multiplied_arg_subsets.push_back(multiplied_arg_subset);
						}
					}

					// Swap
					split_arg_subsets = multiplied_arg_subsets;
				}
			}
		}

		// Only modify the document if this relation was split
		if (duplicate_args_present) {
			// Mark the original relation that we split for removal
			//   Don't want to invalidate our iterators
			relations_to_remove.insert(relation);

			// For each of the split arg sets, generate a new copy of
			// the relation with that subset of the args
			std::vector<ElfRelationArg_ptr> split_arg_subset;
			BOOST_FOREACH(split_arg_subset, split_arg_subsets) {
				// Create a new split relation for this arg subset,
				// copying metadata from the original relation, and
				// add it to the set of relations we're going to add
				//   Don't want to invalidate our iterators
				ElfRelation_ptr split_relation = boost::make_shared<ElfRelation>(relation->get_name(), split_arg_subset, relation->get_text(), relation->get_start(), relation->get_end(), relation->get_confidence(), relation->get_score_group());
				split_relation->add_source(relation->get_source());
				split_relation->add_source(L"eru:elfdocument-split_duplicate_role_relations");
				relations_to_add.insert(split_relation);
			}
		}
	}

	// Loop through the relations we split and remove them from the document
	remove_relations(relations_to_remove);

	// Insert all of the split relations we generated
	_relations.insert(relations_to_add.begin(), relations_to_add.end());
}

/**
 * Converts all of the individuals in this document to relations,
 * following the convention that <name> becomes ic:hasName, <desc>
 * becomes rdfs:label, and <type> becomes rdf:type. The relations
 * so generated are added to the document directly, the originating
 * individuals are removed, and where applicable id attributes on
 * args are replaced with the equivalent URI value.
 *
 * Intended for use during R-ELF macro conversion. Operates in-place.
 *
 * @author nward@bbn.com
 * @date 2010.10.26
 **/
void ElfDocument::convert_individuals_to_relations(const std::wstring& domain_prefix) {
	// Loop over all of the individuals in this document, converting to relations
	ElfIndividualSet individuals = get_individuals_by_type();
	BOOST_FOREACH(ElfIndividual_ptr individual, individuals) {
		// Convert this individual to its equivalent relations
		std::set<ElfRelation_ptr> individual_relations = ElfRelationFactory::from_elf_individual(domain_prefix, individual);
		_relations.insert(individual_relations.begin(), individual_relations.end());
	}

	// Nuke any individuals we just converted
	_individuals.clear();
}

/**
 * Dumps all of the args in this document, grouped by
 * individual, for debugging.
 *
 * @param output_dir The path to a directory where this dump will
 * be written.
 * @param types The types of individual to restrict to.
 *
 * @author nward@bbn.com
 * @date 2011.01.06
 **/
void ElfDocument::dump_individuals_by_type(const std::wstring & output_dir, const std::vector<std::wstring> & types) const {
	// Make sure the output directory exists
	boost::filesystem::wpath output_dir_path = boost::filesystem::system_complete(boost::filesystem::wpath(output_dir));
	boost::filesystem::create_directories(output_dir_path);

	// Generate a path to the output file
	std::wstring dump_file = output_dir + L"/" + _id + L".ind.txt";
	UTF8OutputStream dump_stream(dump_file.c_str());

	// Get the set of merged individuals (avoids repeats during dump)
	ElfIndividualSet individuals = get_merged_individuals();

	// Dump the relations/args grouped by type and individual
	BOOST_FOREACH(std::wstring type, types) {
		dump_stream << type << "\n";
		BOOST_FOREACH(ElfIndividual_ptr individual, individuals) {
			if (!individual->has_type(type))
				continue;
			dump_stream << "  " << individual->get_best_uri() << "\n";
			ElfRelationMap relations = get_relations_by_individual(individual);
			BOOST_FOREACH(ElfRelationMap::value_type relation_group, relations) {
				BOOST_FOREACH(ElfRelation_ptr relation, relation_group.second) {
					BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
						// Ignore args containing this individual
						if (arg->get_individual().get() == NULL || arg->get_individual()->get_best_uri() != individual->get_best_uri()) {
							dump_stream << "    " << relation->get_name() << " " << arg->get_role() << " \"";
							if (arg->get_individual().get() != NULL) {
								if (arg->get_individual()->has_value())
									dump_stream << arg->get_individual()->get_value();
								else
									dump_stream << arg->get_individual()->get_best_uri();
							}
							dump_stream << "\"\n";
						}
					}
				}
			}
		}
	}
}

/**
 * Convenience method that replaces a URI prefix with the
 * prefix from this document's _source; generally intended
 * for use with generated individual IDs based on a particular
 * domain ontology type.
 *
 * @param uri The URI whose prefix is being replaced.
 * @return The prefix-replaced URI, if _source was defined.
 *
 * @author nward@bbn.com
 * @date 2010.10.19
 **/
std::wstring ElfDocument::replace_uri_prefix(const std::wstring & uri) {
	if (_source == L"") {
		return uri;
	} else {
		std::wstring source_prefix = _source.substr(0, _source.find_first_of(' '));
        boost::to_lower(source_prefix);
		//std::transform(source_prefix.begin(), source_prefix.end(), source_prefix.begin(), std::tolower<wchar_t>);
		std::wstring uri_ret(uri);
		return uri_ret.replace(0, uri_ret.find_first_of(':'), source_prefix);
	}
}

/**
 * Converts this document to an XML <doc> element using the
 * Xerces-C++ library.
 *
 * XMLPlatformUtils::Initialize() must be called before this method.
 *
 * @author nward@bbn.com
 * @date 2010.05.14
 **/
DOMDocument* ElfDocument::to_xml(void) const {
	// Build the output element tree for this document
    SerifXML::xstring x_ls = SerifXML::transcodeToXString("LS");
    SerifXML::xstring x_doc = SerifXML::transcodeToXString("doc");
    SerifXML::xstring x_mr_elf = SerifXML::transcodeToXString("http://www.bbn.com/MR/ELF");
	DOMImplementation* dom = DOMImplementationRegistry::getDOMImplementation(x_ls.c_str());
	DOMDocument* xml = dom->createDocument(x_mr_elf.c_str(), x_doc.c_str(), NULL);
	DOMElement* doc = xml->getDocumentElement();
    SXMLUtil::setAttributeFromStdWString(doc, "id", _id);
    SXMLUtil::setAttributeFromStdWString(doc, "elf-version", _version);
    SXMLUtil::setAttributeFromStdWString(doc, "source", _source);
    SXMLUtil::setAttributeFromStdWString(doc, "contents", _contents);

	bool sort_elf_elements = ParamReader::getOptionalTrueFalseParamWithDefaultVal("sort_elf_elements", /* defaultVal=*/ false);
	if (sort_elf_elements) {
		// Write out relations
		ElfRelationSortedSet sorted_relations(_relations.begin(), _relations.end());

		BOOST_FOREACH(ElfRelation_ptr relation, sorted_relations) {
			doc->appendChild(relation->to_xml(xml, _id));
		}
		if (_contents != L"R-ELF") {
			// Merge and write out individuals (across all relations)
			ElfIndividualSet individuals = get_merged_individuals();
			ElfIndividualSortedSet sorted_individuals(individuals.begin(), individuals.end());
			BOOST_FOREACH(ElfIndividual_ptr individual, sorted_individuals) {
				doc->appendChild(individual->to_xml(xml, _id));
			}
		}
	} else {
		// Write out relations
		BOOST_FOREACH(ElfRelation_ptr relation, _relations) {
			doc->appendChild(relation->to_xml(xml, _id));
		}
		if (_contents != L"R-ELF") {
			// Merge and write out individuals (across all relations)
			ElfIndividualSet individuals = get_merged_individuals();
			BOOST_FOREACH(ElfIndividual_ptr individual, individuals) {
				doc->appendChild(individual->to_xml(xml, _id));
			}
		}
	}

	// Done
	return xml;
}

/**
 * Writes this document as an ELF XML file to the specified directory.
 * If an XML file for this document (by unique ID) already exists in
 * the directory, it will be overwritten.
 *
 * XMLPlatformUtils::Initialize() must be called before this method.
 *
 * @param output_dir The path to a directory where this document will
 * be written.
 *
 * @author nward@bbn.com
 * @date 2010.05.14
 **/
void ElfDocument::to_file(const std::wstring & output_dir) const {
	// Make sure the output directory exists
	boost::filesystem::wpath output_dir_path = boost::filesystem::system_complete(boost::filesystem::wpath(output_dir));
	boost::filesystem::create_directories(output_dir_path);

	// Generate a full path to the output file
	std::wstring doc_output_file = output_dir + L"/" + _id;
	if (_contents == L"S-ELF") {
		doc_output_file += L".self.xml";
	} else if (_contents == L"R-ELF") {
		doc_output_file += L".relf.xml";
	} else {
		doc_output_file += L".elf.xml";
	}
    
	// Write out the document
	DOMDocument* output_doc = to_xml();
    XMLUtil::saveXercesDOMToFilename(output_doc, doc_output_file.c_str());
	output_doc->release();
}

/*void ElfDocument::addPatternMatches(const MatchInfo::PatternMatches& patternMatches) {
	BOOST_FOREACH(MatchInfo::PatternMatch match, patternMatches) {
		// Create a new relation for this match
		try {
			_relations.insert(boost::make_shared<ElfRelation>(pattern, 
								docData->getDocTheory(), sent_theory, match));
		} catch (UnexpectedInputException& error) {
			SessionLogger::info("LEARNIT") << pattern->getName() << " failed to create relation" << std::endl;
			if ((error.getSource() && strlen(error.getSource()) > 0) || 
				(error.getMessage() && strlen(error.getMessage()) > 0)) {
					SessionLogger::info("LEARNIT") << "    " << error.getSource() << ": " << error.getMessage() << std::endl;
			}
		}
	}
}
*/
