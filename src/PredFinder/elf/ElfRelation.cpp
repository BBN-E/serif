/**
 * Parallel implementation of ElfRelation object based on Python
 * implementation in ELF.py. Uses Xerces-C++ for XML writing.
 *
 * @file ElfRelation.cpp
 * @author nward@bbn.com
 * @date 2010.06.16
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Parse.h"
#include "Generic/common/ParamReader.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/PatternSet.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/features/TopLevelPFeature.h"
#include "LearnIt/MainUtilities.h"
#include "PredFinder/common/SXMLUtil.h"
#include "PredFinder/inference/EITbdAdapter.h"
#include "LearnIt/Target.h"
#include "ElfRelation.h"
#include "ElfRelationArgFactory.h"
#include "ElfIndividualFactory.h"
#include "ElfDescriptor.h"
#pragma warning(push, 0)
#include "boost/regex.hpp"
#pragma warning(pop)
#include "boost/foreach.hpp"
#include "boost/make_shared.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/algorithm/string/replace.hpp"
#include <stdexcept>

using boost::dynamic_pointer_cast;
const double ElfRelation::DEFAULT_CONFIDENCE = 0.5;

XERCES_CPP_NAMESPACE_USE

/**
 * Construct a relation from a particular LearnIt pattern match.
 * If this was an NFL pattern that didn't directly produce an
 * arg with a game role, generate one.
 *
 * @param target The Target that fired to produce this match.
 * @param doc_theory The Serif DocTheory containing this match,
 * used for determining offsets.
 * @param sent_theory The Serif SentenceTheory that caused this
 * pattern match to fire.
 * @param match The PatternMatch containing matching slots.
 *
 * @author nward@bbn.com
 * @date 2010.05.14
 **/

ElfRelation::ElfRelation(const DocTheory* doc_theory, const SentenceTheory* sent_theory,
						 const Target_ptr target, 
						 const MatchInfo::PatternMatch& match, double confidence,
						 bool learnit2) 
{
	// Determine the overall start and end offsets of the entire sentence
	//   We use the entire sentence span, as opposed to a more constrained
	//   pattern maximum span.
	TokenSequence* tokens = sent_theory->getTokenSequence();
	EDTOffset sent_start = tokens->getToken(0)->getStartEDTOffset();
	EDTOffset sent_end = tokens->getToken(tokens->getNTokens() - 1)->getEndEDTOffset();

	// Get the relation properties from the pattern and match
	_name = target->getELFOntologyType();
	_start = sent_start;
	_end = sent_end;
	_confidence = confidence;
	_score_group = 1; // all learnit patterns are assumed high-precision
	if (learnit2) {
		_source = L"bbn:learnit2-" + target->getName() + L"-" + match.source;
	} else {
		_source = L"bbn:learnit-" + target->getName() + L"-" + match.source;
	}

	// Get the raw document text of the pattern match
	LocatedString* text = MainUtilities::substringFromEdtOffsets(doc_theory->getDocument()->getOriginalText(), _start, _end);
	_text = std::wstring(text->toString());
	delete text;

	// Check if we're looking at an NFL LearnIt pattern
	bool nfl_pattern = false;
	if (_name.length() >= 7 && _name.substr(0, 7) == L"eru:NFL")
		nfl_pattern = true;

	// Loop over the match slots and create arguments for each
	bool found_game = false;
	std::wstring docid = std::wstring(doc_theory->getDocument()->getName().to_string());
	for (size_t slot_index = 0; slot_index < match.slots.size(); slot_index++) {
		// Construct and add a new argument from this slot's constraints and match
		try {
			ElfRelationArg_ptr arg = boost::make_shared<ElfRelationArg>(doc_theory, target->getSlotConstraints(slot_index), 
				match.slots[slot_index]);

			// Check if this is an NFL learnit pattern which needs special game conversion
			if (nfl_pattern && arg->get_individual()->has_value()) {
				if (arg->get_role() == L"eru:NFLGame" || arg->get_role() == L"eru:gameName") {
					// This pattern match asserts a game name string, which we want to convert to an individual immediately
					found_game = true;

					// Generate a game individual and override the value-based argument
					std::wstring game_uri = ElfIndividual::generate_uri_from_offsets(docid, arg->get_start(), arg->get_end());
					ElfIndividual_ptr game_individual = boost::make_shared<ElfIndividual>(game_uri, arg->get_individual()->get_name_or_desc(), arg->get_individual()->get_type());
					arg = boost::make_shared<ElfRelationArg>(L"eru:NFLGame", game_individual);
				}
			}

			// Store this argument slot match in this relation
			insert_argument(arg);
		} catch (std::exception& e) {
			SessionLogger::info("LEARNIT") << "Slot " << slot_index << ": " << e.what() << std::endl;
		}
	}

	// If this is an NFL pattern, but no game arg was found, hallucinate one
	if (nfl_pattern && !found_game) {
		// Generate a game individual
		std::wstring game_uri = ElfIndividual::generate_uri_from_offsets(docid, _start, _end);
		ElfDescriptor_ptr game_desc = boost::make_shared<ElfDescriptor>(_text, -1.0, _text, _start, _end);
		ElfType_ptr game_type = boost::make_shared<ElfType>(L"nfl:NFLGame", _text, _start, _end);
		ElfIndividual_ptr game_individual = boost::make_shared<ElfIndividual>(game_uri, game_desc, game_type);

		// Generate a game individual argument and add it
		ElfRelationArg_ptr game_arg = boost::make_shared<ElfRelationArg>(L"eru:NFLGame", game_individual);
		insert_argument(game_arg);
	}

	// Make sure we had valid match slots
	if (_arg_map.size() < 2)
		throw UnexpectedInputException("ElfRelation::ElfRelation(Pattern_ptr, DocTheory*, SentenceTheory*, PatternMatch)", "Pattern match did not generate enough relation args");
}

/**
 * Construct a relation from a SnippetFeatureSet found using
 * manual Distillation patterns.
 *
 * @param doc_theory The DocTheory containing the matched
 * SnippetFeatureSet.
 * @param pattern The PatternSet that was used to find
 * the feature_set; only used for metadata.
 * @param feature_set The SnippetFeatureSet that contains
 * SnippetFeatures that we can convert to an ElfRelation.
 *
 * @author nward@bbn.com
 * @date 2010.06.18
 **/
ElfRelation::ElfRelation(const DocTheory* doc_theory, const PatternSet_ptr pattern, const PatternFeatureSet_ptr feature_set) {
	// Check for bad relation
	if (doc_theory == NULL)
		throw std::runtime_error("ElfRelation::ElfRelation(DocTheory*, PatternSet_ptr, PatternFeatureSet_ptr): Document theory null");

	// The pattern set name should always be the output relation name, though it
	//   might get reset if these are TBD patterns.
	_name = std::wstring(pattern->getPatternSetName().to_string());
	_text = L"";
	
	// Find all return features for this relation
	std::vector<ReturnPatternFeature_ptr> returnFeatures;
	for (size_t i = 0; i < feature_set->getNFeatures(); i++) {
		PatternFeature_ptr feature = feature_set->getFeature(i);
		if (ReturnPatternFeature_ptr rf = dynamic_pointer_cast<ReturnPatternFeature>(feature)) {
			returnFeatures.push_back(rf);
		}
	}

	// Identify all TBD arguments. TBD arguments are those which have been set
	//   with (tbd some_value) in the pattern file. Their true values will
	//   be determined by other features of the event, e.g. whether the agent
	//   is a group or not. The features are separated into a single 
	//   tbdEventFeature and then vectors of agent and patient features.
	ReturnPatternFeature_ptr tbdEventFeature;
	std::vector<ReturnPatternFeature_ptr> tbdAgentFeatures;
	std::vector<ReturnPatternFeature_ptr> tbdPatientFeatures;
	BOOST_FOREACH (ReturnPatternFeature_ptr feature, returnFeatures) {
		std::wstring tbd = feature->getReturnValue(L"tbd");
		if (tbd.empty())
			continue;
		
		if (tbd.compare(L"bad_date") == 0) {
			continue;
		} else if (tbd.compare(L"agent") == 0) {
			tbdAgentFeatures.push_back(feature);
		} else if (tbd.compare(L"patient") == 0) {
			tbdPatientFeatures.push_back(feature);
		} else if (!EITbdAdapter::getEventRole(tbd).empty()) {// i.e., tbd is in {"generic_violence","killing","injury","bombing","attack"}
			if (tbdEventFeature == 0)
				tbdEventFeature = feature;
			else {
				ostringstream ostr;
				if (feature->getPatternReturn()) {
					feature->getPatternReturn()->dump(ostr, /*indent=*/0);
				}
				std::string main_str;
				main_str = "Feature set has multiple 'tbd' event features; not wise to create an event here.\n";
				main_str += ostr.str();
				// This might happen with "X was blamed for Y and Z", where Y and Z are separate events
				throw UnexpectedInputException("ElfRelation::ElfRelation"
					"(DocTheory*, PatternSet_ptr, PatternFeatureSet_ptr)", 
					main_str.c_str());
			}
		} else {
			SessionLogger::warn("LEARNIT") << "Unknown \"tbd\" feature in pattern file: -->" << tbd << "<--\n";
		}
	}

	// Fill in type/role for all arguments that were "TBD"; this will also reset "_name"
	fix_tbd_arguments(doc_theory, tbdEventFeature, tbdAgentFeatures, tbdPatientFeatures);

	// Now create and insert an argument for each SnippetReturnFeature
	BOOST_FOREACH (ReturnPatternFeature_ptr feature, returnFeatures) {
		std::vector<ElfRelationArg_ptr> args = ElfRelationArgFactory::from_return_feature(doc_theory, feature);
		BOOST_FOREACH(ElfRelationArg_ptr arg, args) {
			insert_argument(arg);
		}
	}

	// Make sure we had a valid feature set
	if (_arg_map.size() < 2)
		throw UnexpectedInputException("ElfRelation::ElfRelation(DocTheory*, PatternSet_ptr, PatternFeatureSet_ptr)", 
		"Feature set did not generate enough relation args");

	// Generate a source from the pattern set name (namespaces become underscored prefixes).
	// Append the subpattern id. Throw an exception if none is found.
	std::wstring source = _name;
	boost::algorithm::replace_all(source, L":", L"_");
	std::wstring subpattern_id = L"";
	for (size_t f = 0; f < feature_set->getNFeatures(); f++) {
		PatternFeature_ptr feature = feature_set->getFeature(f);
		Pattern_ptr pattern = feature->getPattern();
		if (pattern.get()) {
			Symbol subpattern_id_sym = pattern->getID();
			if (subpattern_id_sym != Symbol()) {
				subpattern_id = std::wstring(subpattern_id_sym.to_string());
				break;
			}
		}
	}
	_source = L"bbn:manual-" + source;
	if (subpattern_id != L"") {
		_source += L"-" + subpattern_id;
	} else {
		throw std::runtime_error("ElfRelation::ElfRelation(DocTheory*, PatternSet_ptr, PatternFeatureSet_ptr): No id defined for subpattern");
	}

	// We retrieve the feature set's score estimate. If it has
	// not been set (i.e., if it is equal to Pattern::UNSPECIFIED_SCORE, 
	// which is -1.0), set it to our default value. Since _confidence is a double 
	// rather than an int, we play it safe rather than simply test 
	// whether _confidence == Pattern::UNSPECIFIED_SCORE.
	_confidence = feature_set->getScore();
	_score_group = feature_set->getBestScoreGroup();
	
	if (abs(_confidence - Pattern::UNSPECIFIED_SCORE) < .01)
		_confidence = DEFAULT_CONFIDENCE;

	// Get the start offset of the snippet
	SentenceTheory* start_sent_theory = doc_theory->getSentenceTheory(feature_set->getStartSentence());
	if (start_sent_theory == NULL)
		throw std::runtime_error("ElfRelation::ElfRelation(DocTheory*, "
		"PatternSet_ptr, PatternFeatureSet_ptr): Start sentence theory null");
	TokenSequence* start_token_sequence = start_sent_theory->getTokenSequence();
	if (start_token_sequence == NULL)
		throw std::runtime_error("ElfRelation::ElfRelation(DocTheory*, "
		"PatternSet_ptr, PatternFeatureSet_ptr): Start token sequence null");
	_start = start_token_sequence->getToken(feature_set->getStartToken())->getStartEDTOffset();

	// Get the end offset of the snippet
	SentenceTheory* end_sent_theory = doc_theory->getSentenceTheory(feature_set->getEndSentence());
	if (end_sent_theory == NULL)
		throw std::runtime_error("ElfRelation::ElfRelation(DocTheory*, "
		"PatternSet_ptr, PatternFeatureSet_ptr): End sentence theory null");
	TokenSequence* end_token_sequence = end_sent_theory->getTokenSequence();
	if (end_token_sequence == NULL)
		throw std::runtime_error("ElfRelation::ElfRelation(DocTheory*, "
		"PatternSet_ptr, PatternFeatureSet_ptr): End token sequence null");
	_end = end_token_sequence->getToken(feature_set->getEndToken())->getEndEDTOffset();

	// Get the text of the snippet
	LocatedString* text = MainUtilities::substringFromEdtOffsets(
		doc_theory->getDocument()->getOriginalText(), _start, _end);
	_text = std::wstring(text->toString());
	delete text;
}

/**
 * Take the return features that represent a TBD relation from the pattern file.
 * Transform their roles and types into the appropriate values for this event.
 * Also, set _name appropriately.
 *
 * @param doc_theory The DocTheory containing the matched
 * SnipppetFeatureSet.
 * @param tbdEventFeature The SnippetReturnFeature* corresponding to the
 * event itself, typically pointing to a Proposition or Event. 
 * @param tbdAgentFeatures The SnippetReturnFeature* corresponding
 * to agents of the event.
 * @param tbdPatientFeatures The SnippetReturnFeature* corresponding
 * to patients of the event.
 *
 * @author eboschee@bbn.com
 * @date 2010.12.12
 **/
void ElfRelation::fix_tbd_arguments(const DocTheory *doc_theory, ReturnPatternFeature_ptr tbdEventFeature, 
									std::vector<ReturnPatternFeature_ptr>& tbdAgentFeatures,
									std::vector<ReturnPatternFeature_ptr>& tbdPatientFeatures)
{
	if (tbdEventFeature == 0)
		return;

	std::wstring event_type = tbdEventFeature->getReturnValue(L"tbd");
	
	// Set event role/type
	std::wstring role = EITbdAdapter::getEventRole(event_type);
	if (!role.empty())
		tbdEventFeature->setReturnValue(L"role", role);
	std::wstring type = EITbdAdapter::getEventType(event_type, !tbdAgentFeatures.empty());
	if (!type.empty())
		tbdEventFeature->setReturnValue(L"type", type);

	// Set agent role/type
	BOOST_FOREACH(ReturnPatternFeature_ptr feature, tbdAgentFeatures) {
		MentionReturnPFeature_ptr mf = dynamic_pointer_cast<MentionReturnPFeature>(feature);
		if (!mf) {
			continue; // maybe we should throw instead?
		}
		const Mention *ment = mf->getMention();
		EntitySubtype est = ment->guessEntitySubtype(doc_theory);
		std::wstring role = EITbdAdapter::getAgentRole(event_type);
		if (!role.empty())
			mf->setReturnValue(L"role", role);
		std::wstring type = EITbdAdapter::getAgentType(ment->getEntityType(), est);
		if (!type.empty())
			mf->setReturnValue(L"type", type);
	}

	// Set patient role/type
	BOOST_FOREACH(ReturnPatternFeature_ptr feature, tbdPatientFeatures) {
		MentionReturnPFeature_ptr mf = dynamic_pointer_cast<MentionReturnPFeature>(feature);
		if (!mf) {
			continue; // maybe we should throw instead?
		}
		const Mention *ment = mf->getMention();
		EntitySubtype est = ment->guessEntitySubtype(doc_theory);
		std::wstring role = EITbdAdapter::getPatientRole(event_type, ment->getEntityType(), est);		
		if (!role.empty())
			mf->setReturnValue(L"role", role);
		std::wstring type = EITbdAdapter::getPatientType(event_type, ment->getEntityType(), est);
		if (!type.empty())
			mf->setReturnValue(L"type", type);
	}

	_name = EITbdAdapter::getEventName(event_type, !tbdAgentFeatures.empty());
}

/**
 * Copy constructor.
 *
 * @param other ElfRelation to deep copy from.
 *
 * @author nward@bbn.com
 * @date 2010.08.18
 **/
ElfRelation::ElfRelation(const ElfRelation_ptr other) {
	// Copy the other relation's name, confidence, source, text, and offsets
	_name = other->_name;
	_confidence = other->_confidence;
	_score_group = other->_score_group;
	_source = other->_source;
	_text = other->_text;
	_start = other->_start;
	_end = other->_end;

	// Deep copy the other relation's args
	BOOST_FOREACH(ElfRelationArg_ptr arg, other->get_args()) {
		insert_argument(arg);
	}
}

/**
 * Copy constructor that also coerces individuals associated
 * with args of the relation being copied into the closest
 * Serif matches (that is, it adds any Serif IDs to a copy of
 * the individual where possible).
 *
 * @param doc_theory The DocTheory containing the relation,
 * which will be used to align with Serif mentions.
 * @param other ElfRelation to deep copy from.
 *
 * @author nward@bbn.com
 * @date 2011.06.09
 **/
ElfRelation::ElfRelation(const DocTheory* doc_theory, const ElfRelation_ptr other) {
	// Copy the other relation's name, confidence, source, text, and offsets
	_name = other->_name;
	_confidence = other->_confidence;
	_score_group = other->_score_group;
	_source = other->_source;
	_text = other->_text;
	_start = other->_start;
	_end = other->_end;

	// Deep copy the other relation's args (some additional copies of the individual are done
	BOOST_FOREACH(ElfRelationArg_ptr arg, other->get_args()) {
		// Get a reference to the inserted arg
		ElfRelationArg_ptr inserted_arg = insert_argument(arg);

		// Check if we need to update this individual
		ElfIndividual_ptr individual = inserted_arg->get_individual();
		if (individual.get() != NULL && !individual->has_value()) {
			// Copy the individual and add Serif IDs
			ElfIndividual_ptr updated_individual = boost::make_shared<ElfIndividual>(doc_theory, individual);
			inserted_arg->set_individual(updated_individual);
		}
	}
}

/**
 * Reads this ElfRelation from an XML <relation> element.
 *
 * XMLPlatformUtils::Initialize() must be called before this method.
 *
 * @param relation The DOMElement containing a <relation>.
 * @param individuals The read <individual>s from this <relation>'s
 * containing document that are passed through to <arg>s for id lookup.
 *
 * @author nward@bbn.com
 * @date 2010.08.31
 **/
ElfRelation::ElfRelation(const DOMElement* relation, const ElfIndividualSet & individuals, bool exclusive_end_offsets) {
	// Read the required relation attributes
	_name = SXMLUtil::getAttributeAsStdWString(relation, "name");
	if (_name == L"")
		throw std::runtime_error("ElfRelation::ElfRelation(DOMElement*, ElfIndividualSet): No name attribute specified on <relation>");

	// Read the optional relation attributes
	_source = SXMLUtil::getAttributeAsStdWString(relation, "source");
	std::wstring start = SXMLUtil::getAttributeAsStdWString(relation, "start");
	if (start != L"") {
		_start = EDTOffset(boost::lexical_cast<int>(start));
	} else {
		_start = EDTOffset();
	}
	std::wstring end = SXMLUtil::getAttributeAsStdWString(relation, "end");
	if (end != L"") {
		_end = EDTOffset(boost::lexical_cast<int>(end));
	} else {
		_end = EDTOffset();
	}
	std::wstring confidence = SXMLUtil::getAttributeAsStdWString(relation, "p");
	if (confidence != L"") {
		_confidence = boost::lexical_cast<double>(confidence);
	} else {
		// No confidence specified
		_confidence = Pattern::UNSPECIFIED_SCORE;
	}	
	std::wstring score_group = SXMLUtil::getAttributeAsStdWString(relation, "score_group");
	if (score_group != L"") {
		_score_group = boost::lexical_cast<int>(score_group);
	} else {
		// No score group specified
		_score_group = Pattern::UNSPECIFIED_SCORE_GROUP;
	}

	// Adjust the offsets if necessary
	if (exclusive_end_offsets && _end.is_defined())
		--_end;

	// Get the string contents of the <text> element if any
	DOMNodeList* text_nodes = SXMLUtil::getNodesByTagName(relation, "text");
	if (text_nodes->getLength() == 0) {
		_text = L"";
	} else if (text_nodes->getLength() == 1) {
		_text = SerifXML::transcodeToStdWString(text_nodes->item(0)->getTextContent());
	} else {
		throw std::runtime_error("ElfRelation::ElfRelation(DOMElement*, ElfIndividualSet): More than one <text> child specified in <relation>");
	}

	// Check the validity of the offsets and text
	if ((!_start.is_defined() && _end.is_defined()) || (_start.is_defined() && !_end.is_defined())) {
		throw std::runtime_error("ElfRelation::ElfRelation(DOMElement*, ElfIndividualSet): Invalid offset attributes on <relation>, specify both or neither");
	} else if ((_start.is_defined() && _end.is_defined() && _text == L"") || (!_start.is_defined() && !_end.is_defined() && _text != L"")) {
		throw std::runtime_error("ElfRelation::ElfRelation(DOMElement*, ElfIndividualSet): <relation> must specify both offset attributes and <text> child or neither");
	}

	// Read all of the <arg>s in this <relation>
	DOMNodeList* arg_nodes = SXMLUtil::getNodesByTagName(relation, "arg");
	for (XMLSize_t a = 0; a < arg_nodes->getLength(); a++) {
		DOMElement* arg_element = dynamic_cast<DOMElement*>(arg_nodes->item(a));
		ElfRelationArg_ptr arg = boost::make_shared<ElfRelationArg>(arg_element, individuals, exclusive_end_offsets);
		if (arg.get() != NULL)
			insert_argument(arg);
		else
			throw std::runtime_error("ElfRelation::ElfRelation(DOMElement*, ElfIndividualSet): bad <arg> element in <relation>");
	}
}

/**
 * Creates an empty relation with the specified name
 * and store the specified arguments (not a copy).
 *
 * @param name The predicate for this new ElfRelation.
 * @param arguments The ElfRelationArgs to reference
 * in this ElfRelation (which gets a reference to the
 * underlying shared pointers).
 *
 * @author nward@bbn.com
 * @date 2010.10.18
 **/
ElfRelation::ElfRelation(const std::wstring & name, 
						 const std::vector<ElfRelationArg_ptr> & arguments, 
						 const std::wstring & text, EDTOffset start, EDTOffset end, 
						 double confidence, int score_group) 
{
	// Copy the member variables and insert all of the arguments
	_name = name;
	_text = text;
	_start = start;
	_end = end;
	BOOST_FOREACH(ElfRelationArg_ptr argument, arguments) {
		insert_argument(argument);
	}

	// Initialize the confidence
	_confidence = confidence; //-1.0;
	_score_group = score_group;
}

/**
 * Flattens _arg_map into a vector containing all of the 
 * args in the original relation. They will be sorted by role,
 * though the role does not appear in the output vector.
 *
 * @return std::vector containing 0 or more ElfRelationArgs,
 * the concatenated leaves of _arg_map.
 *
 * @author nward@bbn.com
 * @date 2010.10.13
 **/
std::vector<ElfRelationArg_ptr> ElfRelation::get_args(void) const {
	std::vector<ElfRelationArg_ptr> all_args;
	BOOST_FOREACH(ElfRelationArgMap::value_type role_arg_mapping, _arg_map) {
		all_args.insert(all_args.end(), role_arg_mapping.second.begin(), role_arg_mapping.second.end());
	}
	return all_args;
}

/**
 * Gets the first ElfRelationArg for a given role, if any.
 * The returned argument is a direct reference, not a copy.
 *
 * @return The first matching ElfRelationArg, or null.
 *
 * @author nward@bbn.com
 * @date 2010.10.13
 **/
ElfRelationArg_ptr ElfRelation::get_arg(const std::wstring & role) const {
	ElfRelationArg_ptr arg;
	ElfRelationArgMap::const_iterator args_i = _arg_map.find(role);
	if (args_i != _arg_map.end()) {
		if (args_i->second.size() > 0) {
			if (args_i->second.size() > 1) {
				SessionLogger::err("LEARNIT") << L"WARNING: (ElfRelation::get_arg) getting only first arg for role " << role << L". Multiple args with that role exist. Results will likely be inconsistent.\n";
				SessionLogger::err("LEARNIT") << this->toDebugString(0) << "\n";
			}
			return args_i->second[0];
		}
	}
	return arg;
}
/**
 * Returns the vector of arguments with a given role
 *
 * @return The vector of arguments with a given role; length of vector is usually 1
 *
 **/
std::vector<ElfRelationArg_ptr> ElfRelation::get_args_with_role(const std::wstring & role) const {
	ElfRelationArg_ptr arg;
	//SessionLogger::info("LEARNIT")<<"\tLook For: "<<role<<std::endl;
	ElfRelationArgMap::const_iterator args_i = _arg_map.find(role);
	if (args_i != _arg_map.end()){
		//SessionLogger::info("LEARNIT")<<"\t\t returning: "<<args_i->second.size()<<std::endl;
		return args_i->second;
	}
	else{
		//SessionLogger::info("LEARNIT")<<"\t\t returning empty: "<<std::endl;
		//BOOST_FOREACH(ElfRelationArgMap::value_type vt, _args){
		//	SessionLogger::info("LEARNIT")<<"\t\t OtherArg: "<<vt.first<<std::endl;
		//}
		std::vector<ElfRelationArg_ptr> empty_vector;
		return empty_vector;
	}
}

/**
 * Collects all of the non-value individuals associated with arguments
 * of this relation, if any.
 *
 * @return std::vector containing 0 or more ElfIndividuals associated
 * with the ElfRelationArgs of this ElfRelation.
 *
 * @author nward@bbn.com
 * @date 2010.06.23
 **/
std::vector<ElfIndividual_ptr> ElfRelation::get_referenced_individuals(void) const {
	// Get entities from each argument contained in this relation
	std::vector<ElfIndividual_ptr> individuals;
	BOOST_FOREACH(ElfRelationArg_ptr arg, get_args()) {
		ElfIndividual_ptr individual = arg->get_individual();
		if (individual && individual.get() != NULL && !individual->has_value()) {
			individuals.push_back(individual);
		}
	}
	return individuals;
}

/**
 * Public accessor to get an arg that matches a specified arg
 * as much as possible. Returns a copy of the underlying
 * ElfRelationArg, so changing any of the return
 * values doesn't change the contents of the document.
 *
 * We assume there's only one matching argument per relation,
 * although we could imagine underspecifying a search and getting
 * multiple matches.
 *
 * @param search_arg The arg to match against.
 * @return A matching arg, or NULL shared pointer if none.
 *
 * @author nward@bbn.com
 * @date 2010.08.18
 **/
ElfRelationArg_ptr ElfRelation::get_matching_arg(const ElfRelationArg_ptr search_arg) const {
	// The match we return, defaulting to empty
	ElfRelationArg_ptr matching_arg;

	// Loop through this relation's arguments looking for a match, ignoring role names
	BOOST_FOREACH(ElfRelationArg_ptr arg, get_args()) {
		if (search_arg->offsetless_equals(arg, false)) {
			matching_arg = boost::make_shared<ElfRelationArg>(arg);
			break;
		}
	}

	// Done
	return matching_arg;
}

/**
 * Equality operation for relations. Only compares predicate
 * name and child arguments for equality; ignores offsets,
 * text, confidence, and source.
 *
 * @param pointer to other ElfRelation to compare with.
 *
 * @author nward@bbn.com
 * @date 2010.08.19
 **/
bool ElfRelation::offsetless_equals(const ElfRelation_ptr other) {
	// Compare ignoring offsets
	if (_name != other->_name)
		return false;

	// Check all args by role
	BOOST_FOREACH(ElfRelationArgMap::value_type role_to_arg_mapping, _arg_map) {
		// Get the args for "other" that have this role, inequal if none
		ElfRelationArgMap::iterator other_args_i = other->_arg_map.find(role_to_arg_mapping.first);
		if (other_args_i == other->_arg_map.end())
			return false;

		// Check all args (usually no more than 1) in this relation that have this role
		BOOST_FOREACH(ElfRelationArg_ptr arg, role_to_arg_mapping.second) {
			// Check for arg matches
			bool arg_match = false;
			BOOST_FOREACH(ElfRelationArg_ptr other_arg, other_args_i->second) {
				if (arg->offsetless_equals(other_arg)) {
					arg_match = true;
					break;
				}
			}
			if (!arg_match)
				return false;
		}
	}

	// Checks passed, equal
	return true;
}

/**
 * Private convenience method that wraps the functionality
 * of std::map for storing an ElfRelationArg in _arg_map.
 * Designed to store a single arg in a one-item vector, using
 * its role name as a key; does not append the arg to an existing vector.
 *
 * @param argument The argument to be inserted; a deep
 * copy is made to prevent unexpected pointer sharing, and
 * all memory is handled by boost::shared_ptr.
 * @return The shared pointer for the copied argument, in case it needs
 * to be modified after insertion.
 *
 * @author nward@bbn.com
 * @date 2010.10.13
 **/
ElfRelationArg_ptr ElfRelation::insert_argument(const ElfRelationArg_ptr arg) {
	// Make a deep copy of this argument, so we don't modify members in the wrong context
	ElfRelationArg_ptr arg_copy = boost::make_shared<ElfRelationArg>(arg);
	
	// Append this argument by role
	std::pair<ElfRelationArgMap::iterator, bool> arg_insert = 
		_arg_map.insert(ElfRelationArgMap::value_type(arg_copy->get_role(), std::vector<ElfRelationArg_ptr>()));
	arg_insert.first->second.push_back(arg_copy);

	// Return a reference to the inserted arg copy, in case we need it
	return arg_copy;
}


/**
 * Private convenience method that wraps the functionality
 * of std::map for removing an ElfRelationArg from _args.
 *
 * @param arg The argument to be removed. This must be
 * the actual argument that is part of the relation, not a copy.
 *
 * @author eboschee@bbn.com
 * @date 2010.12.19
 **/
void ElfRelation::remove_argument(const ElfRelationArg_ptr arg) {
	ElfRelationArgMap::iterator iter = _arg_map.find(arg->get_role());

	// just in case
	if (iter == _arg_map.end())
		return; 

	std::vector<ElfRelationArg_ptr>& arg_vector = (*iter).second;
	std::vector<ElfRelationArg_ptr>::iterator vector_iter;
	for (vector_iter = arg_vector.begin(); vector_iter != arg_vector.end(); vector_iter++) {
		if ((*vector_iter) == arg) {
			arg_vector.erase(vector_iter);
			break;
		}
	}
	_arg_map.erase(iter);
}

/**
 * Converts this relation to an XML <relation> element using the
 * Xerces-C++ library.
 *
 * XMLPlatformUtils::Initialize() must be called before this method.
 *
 * @param doc An already-instantiated Xerces DOMDocument that
 * provides namespace context for the created element (since
 * Xerces doesn't support easy anonymous element import).
 * @return The constructed <relation> DOMElement
 *
 * @author nward@bbn.com
 * @date 2010.05.14
 **/
DOMElement* ElfRelation::to_xml(DOMDocument* doc, const std::wstring & docid) const {
	// Until we get rules firing in a predictable order, the source and confidence attributes may differ from run to run.
	// To make it easier to compare diffs, we add params that allow us to blank out the contents of these
	// attributes when set to false.
	static bool show_source_attrib = ParamReader::getOptionalTrueFalseParamWithDefaultVal(
		"show_source_attrib", /* defaultVal=*/ true);
	static bool show_confidence_attrib = ParamReader::getOptionalTrueFalseParamWithDefaultVal(
		"show_confidence_attrib", /* defaultVal=*/ true);

	// Create a new relation element
	DOMElement* relation = SXMLUtil::createElement(doc, "relation");

	// Copy the relation's properties in as attributes of the <relation>
	SXMLUtil::setAttributeFromStdWString(relation, "name", _name);
	if (_start.is_defined() && _end.is_defined()) {
        SXMLUtil::setAttributeFromEDTOffset(relation, "start", _start);
        SXMLUtil::setAttributeFromEDTOffset(relation, "end", _end);
	}

	// Format the relation's confidence, if any, as an xsd:double with 3 digits of precision
	if (show_confidence_attrib && _confidence > 0.0) {
		std::wostringstream confidence;
		confidence << std::fixed << std::setprecision(3) << _confidence;
		SXMLUtil::setAttributeFromStdWString(relation, "p", confidence.str());
	}

	// Print score group only if we are printing scores
	if (show_confidence_attrib && _score_group > 0) {
		std::wostringstream score_group;
		score_group << _score_group;
		SXMLUtil::setAttributeFromStdWString(relation, "score_group", score_group.str());
	}

	// Copy the relation's source pattern in as an attribute.
	if (show_source_attrib && !_source.empty()) {
		SXMLUtil::setAttributeFromStdWString(relation, "source", _source);
	}

	// Optionally print the text evidence for the relation
	if (ParamReader::isParamTrue("elf_include_text_excerpts") && _text != L"") {
		// Extract the text for the entire match
		DOMElement* text = SXMLUtil::createElement(doc, "text");
        SerifXML::xstring x_text = SerifXML::transcodeToXString(_text.c_str());
		DOMText* text_cdata = doc->createTextNode(x_text.c_str());
		text->appendChild(text_cdata);
		relation->appendChild(text);
	}

	// Note that this parameter is also read by ElfDocument.
	bool sort_elf_elements = ParamReader::getOptionalTrueFalseParamWithDefaultVal("sort_elf_elements", /* defaultVal=*/ false);
	if (sort_elf_elements) {
		std::vector<ElfRelationArg_ptr> args = get_args();
		ElfRelationArgSortedSet sorted_args(args.begin(), args.end());
		BOOST_FOREACH(ElfRelationArg_ptr arg, sorted_args) {
			// Suppress type attribute on rdf:subject args of rdf:/rdfs: relations
			relation->appendChild(arg->to_xml(doc, docid, (arg->get_role() == L"rdf:subject" && (boost::starts_with(_name, L"rdf") || _name == L"ic:hasName"))));
		}
	} else {
		BOOST_FOREACH(ElfRelationArg_ptr arg, get_args()) {
			// Suppress type attribute on rdf:subject args of rdf:/rdfs: relations
			relation->appendChild(arg->to_xml(doc, docid, (arg->get_role() == L"rdf:subject" && (boost::starts_with(_name, L"rdf") || _name == L"ic:hasName"))));
		}
	}

	// Done
	return relation;
}

/**
 * Hash implementation for ElfRelation shared_ptrs
 * that will be used by boost::hash to generate unique
 * IDs but also by various keyed containers. Parallel to
 * the __hash__ implementation in ELF.py.
 *
 * @param relation The pointer referencing the relation
 * to be hashed.
 * @return An integer hash of the hashes of each argument
 * XORed with the hashes of the predicate and offsets.
 **/
size_t hash_value(ElfRelation_ptr const& relation) {
	// Hash in the predicate name and start and end offsets
	boost::hash<std::wstring> string_hasher;
	boost::hash<int> int_hasher;
	size_t relation_hash = string_hasher(relation->get_name());
	relation_hash ^= int_hasher(relation->get_start().value());
	relation_hash ^= int_hasher(relation->get_end().value());

	// Hash in each of the predicate's arguments
	boost::hash<ElfRelationArg_ptr> arg_hasher;
	std::vector<ElfRelationArg_ptr> args = relation->get_args();
	BOOST_FOREACH(ElfRelationArg_ptr arg, args) {
		relation_hash ^= arg_hasher(arg);
	}

	// Done
	return relation_hash;
}

/**
 * Dump the fields for the relation, then dump all args that are
 * contained by the relation.
 * @param out The std::ostream to which output is to be written.
 * @param indent The number of levels of indentation (one level = one space; default = 0)
 * at which to dump the output for the relation. 
 **/
void ElfRelation::dump(std::ostream &out, int indent /* = 0 */) const {
	std::wstring spaces(indent, L' ');
	std::wstring spaces_plus_2(indent + 2, L' ');
	out << spaces << "name: " << _name << endl;
	// TODO: Control the information printed out with a parameter (debug_level) rather than comment it out.
	//out << spaces << "start: " << _start << endl;
	//out << spaces << "end: " << _end << endl;
	//out << spaces << "text: " << _text << endl;
	//out << spaces << "confidence: " << _confidence << endl;
	//out << spaces << "score group: " << _score_group << endl;
	boost::wregex spaces_re(L"\\s+");
	std::wstring cleaned_text = boost::regex_replace(_text, spaces_re, L" ");
	out << spaces << "text: " << cleaned_text << endl;	
	out << spaces << "source: " << _source << endl;
	for (ElfRelationArgMap::const_iterator pos = _arg_map.begin(); pos != _arg_map.end(); ++pos) {
		BOOST_FOREACH(ElfRelationArg_ptr ptr, pos->second) {
			ptr->dump(out, indent + 4);
		}
	}
}

/**
 * Similar to the ElfRelation.full_cmp implementation in ELF.py.
 *
 * @param other The relation to be compared against.
 * @return -1 if *this < other, 0 if *this == other, 1 if *this > other
 **/
int ElfRelation::compare(const ElfRelation & other) const {
	int name_diff = _name.compare(other._name);
	if (name_diff == 0) {
        int text_diff = _text.compare(other._text);
		if (text_diff == 0) {
			int start_diff = (_start < other._start ? -1 : (_start > other._start ? 1 : 0));
			if (start_diff == 0) {
                int end_diff = (_end < other._end ? -1 : (_end > other._end ? 1 : 0));
				if (end_diff == 0) {
					int roles_diff = lexicographicallyCompareArgMapsByKey(other);
					if (roles_diff == 0) {
						ElfRelationArgMap::const_iterator iter = _arg_map.begin();
						ElfRelationArgMap::const_iterator other_iter = other._arg_map.begin();
						int arg_diff(0);
						while (iter != _arg_map.end()) {
							// We know the _arg_map members are of equal length (otherwise 
							// lexicographicallyCompareArgMapsByKey would have returned a nonzero value).
							// Ignore all elements of the vectors other than the first of each.
							if (iter->second.empty()) {
								if (!other_iter->second.empty()) {
									return -1;
								}
							} else if (other_iter->second.empty()) {
								return 1;
							} else {
								arg_diff = (iter->second.at(0))->compare(*(other_iter->second.at(0)));
								if (arg_diff != 0) {
									return arg_diff;
								} else {
									++iter;
									++other_iter;
								}
							}
						}
						return 0;
					} else {
						return roles_diff;
					}
				} else {
                    return end_diff;
				}
			} else {
                return start_diff;
			}
		} else {
            return text_diff;
		}
	} else {
        return name_diff;
	}
}


int ElfRelation::lexicographicallyCompareArgMapsByKey(const ElfRelation & other) const {
	// The algorithm std::lexicographical_compare() almost gives us what we want,
	// but (despite its name) it returns a bool rather than an int, so we'd have to call 
	// it twice to get a {-1, 0, 1} return value.
	ElfRelationArgMap::const_iterator iter0 = _arg_map.begin();
	ElfRelationArgMap::const_iterator iter1 = other._arg_map.begin();
	while (true) {
		if (iter0 == _arg_map.end()) {
			if (iter1 == other._arg_map.end()) {
				return 0; // the sets have the same content and the same length
			} else {
				return -1; // first set is shorter
			} 
		} else if (iter1 == other._arg_map.end()) {
			return 1; // second set is shorter
		} else if (iter0->first < iter1->first) {
			return -1;
		} else if (iter1->first < iter0->first) {
			return 1;
		} else {
			++iter0;
			++iter1;
		}
	}
}

std::string ElfRelation::toDebugString(int indent) const {
	std::ostringstream ostr;
	dump(ostr, indent);
	return ostr.str();
}
