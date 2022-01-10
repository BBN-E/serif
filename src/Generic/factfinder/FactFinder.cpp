// Copyright (c) 2009 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/reader/MTDocumentReader.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/FactSet.h"
#include "Generic/theories/Value.h"
#include "wordnet/xx_WordNet.h"
#include "Generic/factfinder/FactFinder.h"
#include "Generic/factfinder/FactPatternManager.h"
#include "Generic/factfinder/FactGoldStandardStorage.h"
#include "Generic/factfinder/SectorFactFinder.h"
#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/features/TokenSpanPFeature.h"
#include "Generic/patterns/features/TopLevelPFeature.h"
#include "Generic/patterns/features/MentionPFeature.h"
#include "Generic/patterns/features/ValueMentionPFeature.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/InputUtil.h"
#include "Generic/common/UTF8InputStream.h"

#include "Generic/theories/ActorEntity.h"
#include "Generic/theories/ActorEntitySet.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#include <boost/regex.hpp>
#pragma warning(pop)
#include <boost/filesystem.hpp>
#include <boost/scoped_ptr.hpp>

const std::wstring FactFinder::FF_FACT_TYPE = L"ff_fact_type";
const std::wstring FactFinder::FF_AGENT1 = L"AGENT1";
const std::wstring FactFinder::FF_ROLE = L"ff_role";
const std::wstring FactFinder::FF_DATE = L"ff_date";

const Symbol FactFinder::FF_START_DATE = Symbol(L"start");
const Symbol FactFinder::FF_END_DATE = Symbol(L"end");
const Symbol FactFinder::FF_HOLD_DATE = Symbol(L"hold");
const Symbol FactFinder::FF_NON_HOLD_DATE = Symbol(L"non_hold");
const Symbol FactFinder::FF_ACTIVITY_DATE = Symbol(L"activity");

const Symbol FactFinder::FF_EMPLOYMENT = Symbol(L"Employment");
const Symbol FactFinder::FF_ACTION = Symbol(L"Action");
const Symbol FactFinder::FF_QUOTATION_ABOUT = Symbol(L"QuotationAbout");
const Symbol FactFinder::FF_QUOTATION = Symbol(L"Quotation");
const Symbol FactFinder::FF_VISITED_PLACE = Symbol(L"VisitedPlace");

FactFinder::FactFinder() :
	DEBUG(false), _is_verbose(false), _sectorFactFinder(0)
{	
	// FactFinder is NOT turned on by default
	_run_fact_finder = ParamReader::isParamTrue("run_fact_finder");
	if (!_run_fact_finder)
		return;

	// Handle case where original MT source language isn't a language Serif implements natively
	try {
		LanguageAttribute original_language = LanguageAttribute::getFromString(ParamReader::getRequiredWParam("original_language").c_str());
		_is_english_source = original_language == Language::ENGLISH;
	} catch (UnexpectedInputException) {
		_is_english_source = false;
	}

	_pattern_manager = _new FactPatternManager(ParamReader::getRequiredParam("fact_pattern_list"));

	//Open the optional debug stream for verbose pattern matching
	std::string debug_file = ParamReader::getParam("fact_finder_debug");
	if (debug_file != "") {
		DEBUG = true;
		_debugStream.open(debug_file.c_str());
	}
	//Store the optional list of canonical names to restrict to
	std::string entity_restriction_file = ParamReader::getParam("entity_restriction_list");
	if (entity_restriction_file != "") {
		//Loop over the restriction file, storing one entity canonical name per line
		boost::scoped_ptr<UTF8InputStream> entity_restriction_stream(UTF8InputStream::build(entity_restriction_file.c_str()));
		std::wstring docid;
		int entity_id;
		while (!entity_restriction_stream->eof()) {
			//Read the docid/entityid pair
			(*entity_restriction_stream) >> docid;
			(*entity_restriction_stream) >> entity_id;

			//Ignore junk lines
			if (docid == L"" || entity_id < 0)
				continue;

			//Store entity ID with its parent document
			_restricted_entities[docid].insert(entity_id);
		}
		entity_restriction_stream->close();
		SessionLogger::info("BRANDY") << "Loaded " << _restricted_entities.size() << " entity restrictions from " << entity_restriction_file << std::endl;
	}
	
	_find_custom_facts = ParamReader::getRequiredTrueFalseParam("find_custom_facts");
	_augment_employment_facts = ParamReader::getRequiredTrueFalseParam("augment_employment_facts");
	_print_factfinder_coldstart_info = ParamReader::isParamTrue("print_factfinder_coldstart_info");
	_print_factfinder_match_info = ParamReader::isParamTrue("print_factfinder_match_info");
	_use_actor_id_for_entity_linker = ParamReader::isParamTrue("use_actor_id_for_entity_linker");
	_min_actor_match_conf =  ParamReader::getOptionalFloatParamWithDefaultValue("min_actor_match_conf", 0.55);

	// ##
        _minActorPatternConf = ParamReader::getOptionalFloatParamWithDefaultValue("min_actor_mention_match_actor_pattern_conf", 1.1);
        _minEditDistance = ParamReader::getOptionalFloatParamWithDefaultValue("min_actor_mention_match_edit_distance", 1.1);
        _minPatternMatchScorePlusAssociationScore = ParamReader::getOptionalFloatParamWithDefaultValue("min_pattern_match_score_plus_association_score", 1000);
        _min_actor_entity_match_conf_accept_at_mention_level = ParamReader::getOptionalFloatParamWithDefaultValue("min_actor_entity_match_conf_accept_at_mention_level", 1.1);
	// ##
 
	addToSetFromString(_validTitleAdjectives, L"academic acting adjunct administrative advertising aeronautical "
		L"alumni appellate armed artistic assistant associate athletic "
		L"attorney auxiliary central certified chief chief-executive civil civilian classroom "
		L"clinical co-chief co-creative co-executive co-general col. college-football collegiate "
		L"commanding commercial common congressional constitutional contributing coordinating corporate "
		L"cosmonaut court creative cricket crown cub defensive deputy detective diplomatic "
		L"director- district divisional divorce doctoral domestic domestic-policy economic electoral "
		L"electrical elementary emeritus environmental equestrian executive external faculty "
		L"federal finance financial first foreign foreign-policy forensic "
		L"founding four-star full fund-raising funeral general global goodwill graduate handball head "
		L"health hedge-fund high honorary human infantry infirmary inspector institutional interim "
		L"interior internal international investigative joint joint-managing junior juvenile lead "
		L"leading legal legislative leveraged-finance lieutenant limited lower-division lt. "
		L"magistrate main maj. major major-league managing marketing mechanical "
		L"medical metallurgical migrant military minor-league municipal mutual mutual-fund national "
		L"naval non-executive nonexecutive northern nuclear offensive official one-star operating "
		L"outside overseas parliamentary parole pension-fund personal personnel physical poker "
		L"police political presidential primary prime princess principal private "
		L"pro pro- probationary professor programming prosecutor provincial public publishing "
		L"publishing-group regional regulatory resource retail riverboat ruling scientific scouting "
		L"second secretaries- secretary secretary- secretary-general security seminary senior "
		L"soccer social-work space-flight special spiritual staff state strategic sub- "
		L"superior supervising supreme surgical tactical talk-show technical textbook theatrical "
		L"third top veteran vice vice- vice-chancellor vice-foreign vice-president vocational "
		L"ward wedding wholesale wide working");

	_futureEmploymentStatusWords = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("future_employment_status_words"), false, true);
	_currentEmploymentStatusWords = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("current_employment_status_words"), false, true);
	_pastEmploymentStatusWords = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("past_employment_status_words"), false, true);

	_validGPETitles = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("valid_gpe_titles"), true, true);
	_validGPETitleStarts = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("valid_gpe_title_starts"), true, true);
	_validGPETitleEnds = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("valid_gpe_title_ends"), true, true);
	_validTitleModifiers = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("valid_title_modifiers"), true, true);

	// This should perhaps be read from a file, but for now it's just here
	_reciprocalRoleLabelTransformations[Symbol(L"Spouse")] = Symbol(L"Actor");
	_reciprocalRoleLabelTransformations[Symbol(L"Family")] = Symbol(L"Actor");
	_reciprocalRoleLabelTransformations[Symbol(L"Contact")] = Symbol(L"Actor");

	if (ParamReader::getParam("sector_fact_pattern_list").length() != 0)
		_sectorFactFinder = _new SectorFactFinder();

}
	
FactFinder::~FactFinder() {
	delete _sectorFactFinder;
}

bool FactFinder::isRestrictedEntity(std::wstring docid, int entity_id) {
	//Check if we're restricting
	if (_restricted_entities.empty())
		//No restrictions, so keep this entity
		return true;

	//Check if this document is in the restriction list
	DocidEntityTable::iterator restrictedDoc = _restricted_entities.find(docid);
	if (restrictedDoc == _restricted_entities.end())
		return false;

	//Check if this entity is in the restriction list
	return (restrictedDoc->second.find(entity_id) != restrictedDoc->second.end());
}


int FactFinder::getPassageID(DocTheory *docTheory, Fact_ptr fact) {

	Document *document = docTheory->getDocument();
	EDTOffset start_offset = docTheory->getSentenceTheory(fact->getStartSentence())->getTokenSequence()->getToken(0)->getStartEDTOffset();	
	std::vector<WSegment>& segments = docTheory->getDocument()->getSegments();

	// NOTE we are using the "raw" unchecked version because seqments and sentences are not 1:1

	if (segments.size() == 0) {
		return fact->getStartSentence();
	} else {
		// For segment files, regions and segments are the same. So, we can find the region_id and use it
		//  to get the segment. This is horribly brittle and should be fixed in a deeper way later.
		size_t docNumRegions = document->getNRegions();
		if (segments.size() != docNumRegions){
			throw UnrecoverableException("FactEntry::getPassageID", "got number of regions different from number of segments\n");
		}
		int found_region_id = -1;
		for (size_t region_id = 0; region_id < docNumRegions; region_id++) {
			const Region *region = document->getRegions()[region_id];
			// Start and end offsets should be inclusive, I believe
			if (start_offset >= region->getStartEDTOffset() &&
				start_offset <= region->getEndEDTOffset())
			{
				found_region_id = static_cast<int>(region_id);
				break;
			}
		}
		if (found_region_id == -1) {
			SessionLogger::err("FF") << "No region found for sentence; using id=0 for passage";
			return 0;
		} else if (segments.size() != docNumRegions) {
			SessionLogger::err("FF") << "Got number of regions different from number of segments; using region ID instead of passage ID";
			return found_region_id;
		} else {
			WSegment correctSegment = segments.at(found_region_id);
			if (correctSegment.segment_attributes().find(L"passage-id") != correctSegment.segment_attributes().end())
				return boost::lexical_cast<int>(correctSegment.segment_attributes()[L"passage-id"]);
			else {
				SessionLogger::err("FF") << "No passage ids; using region ID instead";			
				return found_region_id;
			}
		}
	}

	return 0;
}


// Create a set of strings from a longer string; delimiter defaults to L" "
void FactFinder::addToSetFromString(std::set<std::wstring> &set, std::wstring joined_tokens, 
										   const std::wstring & possible_tok_delims) {
	std::vector<std::wstring> vec;
    boost::split(vec, joined_tokens, boost::is_any_of(possible_tok_delims));
	set.insert(vec.begin(), vec.end());
}

std::wstring FactFinder::getMultiSentenceString(const DocTheory *docTheory, int start_sentence, int end_sentence, int start_token, int end_token) {
	
	std::wstring text = L"";
	for (int s = start_sentence; s <= end_sentence; s++) {
		TokenSequence *tseq = docTheory->getSentenceTheory(s)->getTokenSequence();
		int stok = 0;
		int etok = tseq->getNTokens() - 1;
		if (s == start_sentence)
			stok = start_token;
		if (s == end_sentence)
			etok = end_token;

		for (int tok = stok; tok <= etok; tok++) {
			text += tseq->getToken(tok)->getSymbol().to_string();
			if (tok != etok)
				text += L" ";
		}
		if (s != end_sentence)
			text += L" ";
	}

	return text;
}

void FactFinder::findFacts(std::wstring filename, std::vector<DocTheory *>& documentTheories) {
	if (!_run_fact_finder)
		return;

	// output stream for writing knowledge bases (in cold start KBP format)
	UTF8OutputStream kbStream;
	std::wstring kb_filename = filename + L".kb";
	if (_print_factfinder_coldstart_info)
		kbStream.open(kb_filename.c_str());
	EntityLinker entityLinker(_use_actor_id_for_entity_linker);

	// output stream for collecting pairs
	UTF8OutputStream matchStream;
	std::wstring matchFilename = filename + L".match";
	if (_print_factfinder_match_info)
		matchStream.open(matchFilename.c_str());

	BOOST_FOREACH(DocTheory *docTheory, documentTheories) {
		processDocument(docTheory, kbStream, matchStream, entityLinker);
		if (_sectorFactFinder)
			_sectorFactFinder->findFacts(docTheory);
	}

	if (_print_factfinder_coldstart_info)
		kbStream.close();

	if (_print_factfinder_match_info)
		matchStream.close();

	return;
}

void FactFinder::findFacts(DocTheory *docTheory) {
	if (!_run_fact_finder)
		return;
	
	std::vector<DocTheory *> temp;
	temp.push_back(docTheory);

	if (_print_factfinder_coldstart_info) {
		std::wstring ff_dir = ParamReader::getRequiredWParam("fact_finder_dir");
		if (!boost::filesystem::exists(ff_dir))
			boost::filesystem::create_directories(ff_dir);
		std::wstring filename = getFactsFileName(docTheory);
		std::wstringstream filepath;
		filepath << ff_dir << LSERIF_PATH_SEP << filename;
		findFacts(filepath.str(), temp);
	} else {
		findFacts(L"", temp);
	}

}

std::wstring FactFinder::getFactsFileName(const DocTheory *docTheory) {
	std::wstring documentName = docTheory->getDocument()->getName().to_string();

	// Check for paths in document name, the text after the last slash. We're assuming it's unique for the output directory
	size_t pos = documentName.find_last_of(L'/');
	if (pos+1 == documentName.length())
		throw UnexpectedInputException("FactFinder::getFactsFileName", "Document name ends in slash, can't use as output file name");
	if (pos != std::wstring::npos)  
		documentName = documentName.substr(pos + 1);

	pos = documentName.find_last_of(L'\\');
	if (pos+1 == documentName.length())
		throw UnexpectedInputException("FactFinder::getFactsFileName", "Document name ends in slash, can't use as output file name");
	if (pos != std::wstring::npos)  
		documentName = documentName.substr(pos + 1);

	return documentName + L".facts.xml";
}

void FactFinder::processDocument(DocTheory *docTheory, UTF8OutputStream& kbStream, UTF8OutputStream& matchStream, EntityLinker& entityLinker) {
	FactSet *factSet = _new FactSet();
	docTheory->takeFactSet(factSet);

	//Skip empty documents
	if (docTheory->getNSentences() == 0) {
		SessionLogger::warn("FactFinder") << "FactFinder processDocumenty skipping document with no sentences";
		return;
	}

	// write all entity info with mentions
	if (_print_factfinder_coldstart_info) 
	{
		writeAllMentionsForColdStart(docTheory, kbStream, entityLinker);
		writeNestedNamesFactsForColdStart(docTheory, kbStream);
	}

	// Make sure this is empty
	_documentFacts.clear();

	//Loop over all of the pattern sets and apply the ones of right type to the current document
	PatternSet_ptr currentPatternSet;
	for (size_t setno = 0; setno < _pattern_manager->getNFactPatternSets(); setno++) {
		currentPatternSet = _pattern_manager->getFactPatternSet(setno);
		Symbol patternSetEntityTypeSymbol = _pattern_manager->getEntityTypeSymbol(setno);
		Symbol extraFlagSymbol = _pattern_manager->getExtraFlagSymbol(setno);

		PatternMatcher_ptr pm = PatternMatcher::makePatternMatcher(docTheory, currentPatternSet, 0, PatternMatcher::COMBINE_SNIPPETS_BY_COVERAGE);
		
		if (extraFlagSymbol == Symbol(L"EN_SOURCE_ONLY") && !_is_english_source)
			continue;
		if (extraFlagSymbol == Symbol(L"MT_ONLY") && _is_english_source)
			continue;

		// processPatternSet with this doc using only the right kind of entities
		processPatternSet(pm, patternSetEntityTypeSymbol, extraFlagSymbol);
	}

	if (_find_custom_facts) {
		PatternSet_ptr emptyPatternSet = boost::make_shared<PatternSet>();			
		for (int entity_id = 0; entity_id < docTheory->getEntitySet()->getNEntities(); entity_id++) {
			PatternMatcher_ptr pm = PatternMatcher::makePatternMatcher(docTheory, emptyPatternSet, 0, PatternMatcher::COMBINE_SNIPPETS_BY_COVERAGE);
			pm->clearEntityLabels();
			pm->setEntityLabelConfidence(FF_AGENT1, entity_id, 1.0);
			pm->labelPatternDrivenEntities();
			findCustomFacts(pm, entity_id);
		}		
	}

	for (std::map<int, std::vector<Fact_ptr> >::iterator iter = _documentFacts.begin(); iter != _documentFacts.end(); iter++) {
		pruneAndStoreFacts(docTheory, kbStream, matchStream, (*iter).first);
	}

	// Leave it as we found it
	_documentFacts.clear();

}

void FactFinder::processPatternSet(PatternMatcher_ptr pm, Symbol patSetEntityTypeSymbol, Symbol extraFlagSymbol){ 
	// pattern set is embedded in Doc Info
	// loops entities, doing sentences and storing facts inside entity process

	const Symbol ALLOW_DESC_SYM(L"ALLOW_DESC");

	bool names_only = true;
	if (extraFlagSymbol == ALLOW_DESC_SYM)
		names_only = false;

	const DocTheory* docTheory = pm->getDocTheory();
	const EntitySet *entitySet = docTheory->getEntitySet();
	int docEntitiesCount = entitySet->getNEntities();
	for (int entity_id = 0; entity_id < docEntitiesCount; entity_id++) {
		Entity* entity = entitySet->getEntity(entity_id);
		if (entity == NULL)
			continue;
		EntityType entityType = entity->getType();
		// Only process entities of the type that matches the pattern set type
		if (patSetEntityTypeSymbol != entityType.getName()){
			continue;
		}
		// Only process entities with names
		if (!names_only || entity->hasNameMention(entitySet)) {
			//Check if we're restricting entities
			if (isRestrictedEntity(pm->getDocID(), entity_id)) {
				//Apply this pattern set to this entity for all sentences in the doc
				processEntity(pm, entity_id);
			}
		}
	}
}

void FactFinder::processEntity(PatternMatcher_ptr pm, int entity_id) {

	//Limit the maximum number of features that can fire per pattern per sentence
	size_t max_sets = 50;

	// set the labels for this entity
	pm->clearEntityLabels();
	pm->setEntityLabelConfidence(FF_AGENT1, entity_id, 1.0);
	pm->labelPatternDrivenEntities();

	const DocTheory* docTheory = pm->getDocTheory();
	//Loop over all of the sentences in this document
	for (int sentno = 0; sentno < docTheory->getNSentences(); sentno++) {
		if (_is_verbose) std::cerr << "  sent " << (sentno + 1) << std::endl;

		SentenceTheory* sTheory = docTheory->getSentenceTheory(sentno);
		std::vector<PatternFeatureSet_ptr> featureSets = 
			pm->getSentenceSnippets(sTheory,  DEBUG?&_debugStream:0, false);
	
		for (size_t i = 0; i < featureSets.size() && i < max_sets; ++i) {
			if (featureSets[i] != NULL){				
				addFactsFromFeatureSet(pm, entity_id, featureSets[i]);
			}
		}
	}

	// Not clear if this is used, but do it anyway: find all document-level features
	std::vector<PatternFeatureSet_ptr> docFeatureSets = pm->getDocumentSnippets();
	for (size_t i = 0; i < docFeatureSets.size(); ++i) {
		if (docFeatureSets[i] != NULL)
			addFactsFromFeatureSet(pm, entity_id, docFeatureSets[i]);
	}

	// clear all the labels for this pattern set just in case someone finds old setting
	pm->clearEntityLabels();
}

void FactFinder::addFactsFromFeatureSet(PatternMatcher_ptr pm, int entity_id, PatternFeatureSet_ptr featureSet) {

	Symbol factType;
	Symbol patternID;
	std::map<Symbol, std::vector<FactArgument_ptr> > factArguments;
	std::vector<ReturnPatternFeature_ptr> returnFeatures;

	for (size_t i = 0; i < featureSet->getNFeatures(); i++) {

		PatternFeature_ptr feature = featureSet->getFeature(i);		

		if (TopLevelPFeature_ptr topLevelFeature = boost::dynamic_pointer_cast<TopLevelPFeature>(feature)) {
			patternID = topLevelFeature->getPatternLabel();	
		}

		if (ReturnPatternFeature_ptr returnFeature = boost::dynamic_pointer_cast<ReturnPatternFeature>(feature)){
			// Sometimes propositions fire doubly for some reason, generating multiple SnippetReturnFeatures for the same object
			// We want to make sure we don't print things twice, so we check to make sure we haven't already seen a SnippetReturnFeature
			// that points to the same underlying object
			bool is_new_feature = true;
			for (size_t j = 0; j < returnFeatures.size(); j++) {
				ReturnPatternFeature_ptr currentReturnFeature = returnFeatures[j];
				if (returnFeature->returnValueIsEqual(currentReturnFeature)) {
					is_new_feature = false;
					break;
				}
			}

			if (!is_new_feature)
				continue;

			returnFeatures.push_back(returnFeature);

			if (returnFeature->hasReturnValue(FF_FACT_TYPE)) {
				Symbol newFactType = Symbol(returnFeature->getReturnValue(FF_FACT_TYPE));
				if (!factType.is_null() && factType != newFactType) {
					SessionLogger::warn("FactFinder") << "More than one fact type for fact:" << factType << ", " << newFactType << std::endl;
				}
				factType = newFactType;
			}

			if (returnFeature->hasReturnValue(FF_ROLE) || returnFeature->hasReturnValue(FF_DATE)) {

				Symbol ffRoleList;
				if (returnFeature->hasReturnValue(FF_ROLE))
					ffRoleList = returnFeature->getReturnValue(FF_ROLE);
				else ffRoleList = returnFeature->getReturnValue(FF_DATE);

				// split multiple roles seperated by "+"
				std::wstring strFfRoleSym(ffRoleList.to_string());
				std::vector<std::wstring> vecFfRoles;
				split(vecFfRoles, strFfRoleSym, boost::is_any_of(L"+"));
				for(std::vector<std::wstring>::iterator role_iter = vecFfRoles.begin(); role_iter != vecFfRoles.end(); ++role_iter)
				{
					std::wstring strFfRole = *role_iter;
					Symbol ffRoleSym(strFfRole.c_str());

					int sent_no = returnFeature->getSentenceNumber();
					int start_token = featureSet->getStartToken();
					int end_token = featureSet->getEndToken();

					if (RelMentionReturnPFeature_ptr rmFeature = boost::dynamic_pointer_cast<RelMentionReturnPFeature>(returnFeature)) {
						factArguments[ffRoleSym].push_back(boost::make_shared<TextSpanFactArgument>(ffRoleSym, sent_no, sent_no, start_token, end_token));
					} else if (EventMentionReturnPFeature_ptr vmFeature = boost::dynamic_pointer_cast<EventMentionReturnPFeature>(returnFeature)) {
						factArguments[ffRoleSym].push_back(boost::make_shared<TextSpanFactArgument>(ffRoleSym, sent_no, sent_no, start_token, end_token));
					} else if (PropositionReturnPFeature_ptr propFeature = boost::dynamic_pointer_cast<PropositionReturnPFeature>(returnFeature)) {
						factArguments[ffRoleSym].push_back(boost::make_shared<TextSpanFactArgument>(ffRoleSym, sent_no, sent_no, start_token, end_token));
					} else if (TokenSpanReturnPFeature_ptr tokSpanFeature = boost::dynamic_pointer_cast<TokenSpanReturnPFeature>(returnFeature)) {
						start_token = tokSpanFeature->getStartToken();
						end_token = tokSpanFeature->getEndToken();
						factArguments[ffRoleSym].push_back(boost::make_shared<TextSpanFactArgument>(ffRoleSym, sent_no, sent_no, start_token, end_token));
					} else if (MentionReturnPFeature_ptr menFeature = boost::dynamic_pointer_cast<MentionReturnPFeature>(returnFeature)) {
						factArguments[ffRoleSym].push_back(boost::make_shared<MentionFactArgument>(ffRoleSym, menFeature->getMention()->getUID()));
					} else if (ValueMentionReturnPFeature_ptr valFeature = boost::dynamic_pointer_cast<ValueMentionReturnPFeature>(returnFeature)) {
						factArguments[ffRoleSym].push_back(boost::make_shared<ValueMentionFactArgument>(ffRoleSym, valFeature->getValueMention(), false));
					} else if (DocumentDateReturnPFeature_ptr dateFeature = boost::dynamic_pointer_cast<DocumentDateReturnPFeature>(returnFeature)) {
						factArguments[ffRoleSym].push_back(boost::make_shared<ValueMentionFactArgument>(ffRoleSym, (const ValueMention *)0, true));
					} else {
						SessionLogger::warn("FactFinder") << "FactFinder pattern returns unsupported feature " << returnFeature->getReturnTypeName() << std::endl;
					}
				}
			}
		}
	}

	if (factType.is_null()) {
		SessionLogger::warn("FactFinder") << "No fact type for FactEntry\n";;
		return;
	} 

	if (featureSet->getScore() <= 0) {
		SessionLogger::warn("FactFinder") << "Score less than zero for FactEntry\n";
		return;
	}

	std::wstring factText = getMultiSentenceString(pm->getDocTheory(), featureSet->getStartSentence(), featureSet->getEndSentence(), featureSet->getStartToken(), featureSet->getEndToken());

	if (isLikelyBadFact(factText)) {
		return;
	}

	// If Joe knows Bob and Sheila, we need to split this into Joe/Bob and Joe/Sheila
	std::set<Symbol> regularRoles;
	std::set<Symbol> dateRoles;
	for (std::map<Symbol, std::vector<FactArgument_ptr> >::const_iterator iter = factArguments.begin(); iter != factArguments.end(); iter++) {
		Symbol role = (*iter).first;
		if (isActivityDate(role))
			dateRoles.insert(role);
		else regularRoles.insert(role);
	}

	if (regularRoles.size() < 2) {
		SessionLogger::warn("FactFinder") << "Fewer than two non-activity-date arguments for FactEntry\n";
		return;
	}

	std::vector<Symbol> regularRolesVec;
	BOOST_FOREACH(Symbol sym, regularRoles) {
		regularRolesVec.push_back(sym);
	}

	std::vector< std::vector<FactArgument_ptr> > nullList;
	std::vector< std::vector<FactArgument_ptr> > argLists = recursiveAddOneArgPerRole(nullList, regularRolesVec, factArguments, 0);

	for (size_t i = 0; i < argLists.size(); i++) {

		Fact_ptr fact = boost::make_shared<Fact>(factType, patternID, featureSet->getScore(), featureSet->getBestScoreGroup(), featureSet->getStartSentence(), 
			featureSet->getEndSentence(), featureSet->getStartToken(), featureSet->getEndToken());

		BOOST_FOREACH(FactArgument_ptr arg, argLists.at(i)) {
			fact->addFactArgument(arg);
		}

		std::vector<FactArgument_ptr> dateArgs;
		for (std::set<Symbol>::iterator iter = dateRoles.begin(); iter != dateRoles.end(); iter++) {
			BOOST_FOREACH(FactArgument_ptr arg, factArguments[*iter]) {
				dateArgs.push_back(arg);
			}
		}

		if (_augment_employment_facts && factType == FF_EMPLOYMENT) {
			augmentEmploymentFact(pm, fact, dateArgs.size() > 0);

			const Mention *employer = 0;
			std::wstring title = L"";
			BOOST_FOREACH (FactArgument_ptr arg, fact->getArguments()) {
				if (MentionFactArgument_ptr mentArg = boost::dynamic_pointer_cast<MentionFactArgument>(arg)) {
					if (arg->getRole() == Symbol(L"Employer"))
						employer = mentArg->getMentionFromMentionSet(pm->getDocTheory());
				} 
				if (TextSpanFactArgument_ptr textArg = boost::dynamic_pointer_cast<TextSpanFactArgument>(arg)) {
					if (arg->getRole() == Symbol(L"Title")) {
						title = textArg->getStringFromDocTheory(pm->getDocTheory());
						boost::to_lower(title);
					}
				}
			}
			if (employer && employer->getEntityType().matchesGPE() && title.size() > 0) {
				bool is_valid = false;
				BOOST_FOREACH(std::wstring mod, _validTitleModifiers) {
					if (title.find(mod) == 0 && title != mod)
						title = title.substr(mod.size() + 1, title.size());
				}
				if (_validGPETitles.find(title) != _validGPETitles.end())
					is_valid = true;
				else {
					BOOST_FOREACH(std::wstring str, _validGPETitleStarts) {
						if (title.find(str) == 0)
							is_valid = true;
					}
					
					BOOST_FOREACH(std::wstring str, _validGPETitleEnds) {
						if (boost::algorithm::ends_with(title, str))
							is_valid = true;
					}
				}

				// SKIP FACT!
				if (!is_valid)
					continue;
			}
		}

		if (!hasArgumentRole(fact, FF_NON_HOLD_DATE)) {
			BOOST_FOREACH(FactArgument_ptr arg, dateArgs) {
				fact->addFactArgument(arg);
			}
		}

		_documentFacts[entity_id].push_back(fact);
	}

	return;
}

bool FactFinder::hasArgumentRole(Fact_ptr fact, Symbol role) {
	std::vector<FactArgument_ptr> arguments = fact->getArguments();
	for (size_t i = 0; i < arguments.size(); i++) {
		FactArgument_ptr arg = arguments[i];
		if (arg->getRole() == role)
			return true;
	}
	return false;
}

bool FactFinder::isActivityDate(Symbol sym) {
	return (sym == FF_START_DATE || sym == FF_END_DATE || sym == FF_HOLD_DATE || sym == FF_NON_HOLD_DATE || sym == FF_ACTIVITY_DATE);
}

std::vector< std::vector<FactArgument_ptr> > FactFinder::recursiveAddOneArgPerRole(std::vector< std::vector<FactArgument_ptr> > existingLists, 
																				   std::vector<Symbol> roles, std::map<Symbol, std::vector<FactArgument_ptr> >& argMap, int start_role) 
{
	if (start_role == (int)roles.size())
		return existingLists;

	std::vector< std::vector<FactArgument_ptr> > extendedLists;
	for (size_t i = 0; i < argMap[roles.at(start_role)].size(); i++) {
		FactArgument_ptr arg = argMap[roles.at(start_role)].at(i);
		if (existingLists.size() == 0) {
			std::vector<FactArgument_ptr> newList;
			newList.push_back(arg);
			extendedLists.push_back(newList);
		} else {
			for (size_t j = 0; j < existingLists.size(); j++) {
				std::vector<FactArgument_ptr> newList;
				newList.push_back(arg);
				BOOST_FOREACH(FactArgument_ptr existingArg, existingLists.at(j)) {
					newList.push_back(existingArg);
				}
				extendedLists.push_back(newList);
			}
		}
	}

	return recursiveAddOneArgPerRole(extendedLists, roles, argMap, start_role + 1);
}

bool FactFinder::isLikelyBadFact(std::wstring fact_text) {
	
	size_t textLength = fact_text.length();
	
	// too long will be dropped later anyway
	if (textLength > 1024) 
		return true;

	//texts that end in ellipsis most likely came from a truncated sentence.
	if (fact_text.rfind(L"...") == (textLength-3))
		return true;

	//texts that include " recent comment" (sometimes pluralized) are a conflated mess
	if (fact_text.find(L"recent comment") != std::wstring::npos)
		return true;

	return false;
}

bool FactFinder::entityShouldBeProcessed(PatternMatcher_ptr pm, int entity_id) {

	Entity* thisEntity = pm->getDocTheory()->getEntitySet()->getEntity(entity_id);
	if (thisEntity == 0)
		return false;

	if (!thisEntity->hasNameMention(pm->getDocTheory()->getEntitySet()))
		return false;

	if (!thisEntity->getType().matchesPER() && !thisEntity->getType().matchesORG() && !thisEntity->getType().matchesGPE() && thisEntity->getType().getName() != Symbol(L"PSN"))
		return false;
		
	if (!isRestrictedEntity(pm->getDocID(), entity_id))
		return false;

	return true;
}


void FactFinder::findCustomFacts(PatternMatcher_ptr pm, int entity_id) {

	if (!entityShouldBeProcessed(pm, entity_id))
		return;

	const DocTheory* docTheory = pm->getDocTheory();
	for (int sentno = 0; sentno < docTheory->getNSentences(); sentno++) {
		addDescriptionFacts(pm, entity_id, docTheory->getSentenceTheory(sentno));
	}

}

void FactFinder::addDescriptionFacts(PatternMatcher_ptr pm, int entity_id, SentenceTheory *sTheory) {

	const DocTheory* docTheory = pm->getDocTheory();
	int sent_no = sTheory->getTokenSequence()->getSentenceNumber();
	MentionSet *mentionSet = sTheory->getMentionSet();
	RelMentionSet *relMentionSet = sTheory->getRelMentionSet();
	PropositionSet *propSet = sTheory->getPropositionSet();	
	
	for (int m = 0; m < mentionSet->getNMentions(); m++) {
		const Mention *ment = mentionSet->getMention(m);
		if (ment->getMentionType() != Mention::DESC)
			continue;
		Entity* ent = docTheory->getEntityByMention(ment);
		if (ent == 0 || ent->getID() != entity_id)
			continue;

		const SynNode *node = ment->getNode();
		const Symbol headword = node->getHeadWord();

		// kill known bad descriptions
		if (headword == Symbol(L"mr") || headword == Symbol(L"mr.") ||
			headword == Symbol(L"mrs") || headword == Symbol(L"mrs.") ||
			headword == Symbol(L"ms") || headword == Symbol(L"ms.") ||
			headword == Symbol(L"dr") || headword == Symbol(L"dr.") ||
			headword == Symbol(L"sen") || headword == Symbol(L"sen.") ||
			headword == Symbol(L"rep") || headword == Symbol(L"rep.") ||
			headword == Symbol(L"anybody") || headword == Symbol(L"anyone") ||
			headword == Symbol(L"everybody") || headword == Symbol(L"everyone") ||
			headword == Symbol(L"anything") || headword == Symbol(L"everything") ||
			headword == Symbol(L"itself") || headword == Symbol(L"whoever") ||
			headword == Symbol(L"who") || headword == Symbol(L"government"))
			continue;

		// kill plurals		
		if (node->getHeadPreterm()->getTag() == Symbol(L"NNS") || 
			node->getHeadPreterm()->getTag() == Symbol(L"NNPS"))
			continue;

		std::wstring hstr = headword.to_string();
		bool wordnet_kill = false;
		// Kill all words not in WordNet as these are unlikely to be correct
		if (!WordNet::getInstance()->isInWordnet(headword)) {
			wordnet_kill = true;
		} else if (ment->getEntityType().matchesPER() && !WordNet::getInstance()->isPerson(headword)) {
			// For persons, kill descriptions that are not hyponyms of "person"
			// Considered using all senses here for hyponyms, but it makes performance worse
			// Major good cases missed here (e.g. 'founder') are captured as special cases
			wordnet_kill = true;
		}

		if (wordnet_kill) {
			// Special cases
			bool special_case = false;
			if (// misspellings
				hstr == L"minster" || hstr == L"councilor" || 

				// not in wordnet
				hstr == L"midfielder" || hstr == L"billionaire" || hstr == L"frontrunner" || hstr == L"firefighter" || 

				// person is not the first sense
				hstr == L"agent" || hstr == L"ally" || hstr == L"anchor" || hstr == L"authority" || hstr == L"ace" || 
				hstr == L"chair" || hstr == L"creator" || hstr == L"driver" || hstr == L"enemy" || hstr == L"fan" || 
				hstr == L"founder" || hstr == L"head" || hstr == L"holder" || hstr == L"justice" || hstr == L"principal" || 
				hstr == L"publisher" || hstr == L"queen" || hstr == L"star" || hstr == L"counterpart" ||

				// ends with something interesting
				boost::ends_with(hstr, L"-elect") || boost::ends_with(hstr, L"-old") || boost::ends_with(hstr, L"-general") ||
				boost::ends_with(hstr, L"-chief") || boost::ends_with(hstr, L"maker") || boost::ends_with(hstr, L"-designate") ||
				boost::ends_with(hstr, L"ist") || boost::ends_with(hstr, L"woman") ||

				// starts with something interesting
				boost::starts_with(hstr, L"vice-") || boost::starts_with(hstr, L"co-") || boost::starts_with(hstr, L"ex-") || 
				boost::starts_with(hstr, L"sub-") || boost::starts_with(hstr, L"then-"))				
			{
				special_case = true;
			} 
			// We considered allowing through things where at least one part was in WordNet, but we've covered most of the good
			//   stuff above already, and the rest is about 50% chaff
			if (!special_case) {
				continue;
			}
		}	

		bool other_slot_descriptor = false;
		// Don't return things like "Sarkozy's wife"; 
		// These will be output in the other slots if they are good enough,
		//   so they should not be included as descriptions
		for (int r = 0; r < relMentionSet->getNRelMentions(); r++) {
			const RelMention *rm = relMentionSet->getRelMention(r);
			if ((rm->getLeftMention() == ment || rm->getRightMention() == ment) &&
				(rm->getType() == Symbol(L"PER-SOC.Family") ||
				rm->getType() == Symbol(L"PER-SOC.Lasting-Personal") ||
				rm->getType() == Symbol(L"PER-SOC.Business") ||
				rm->getType() == Symbol(L"ORG-AFF.Employment")))
			{
				other_slot_descriptor = true;
				break;
			}
		}
		if (other_slot_descriptor)
			continue;

		// Look for negated copulas
		const SynNode *negation = 0;
		for(int p_no = 0; p_no < propSet->getNPropositions(); p_no++){
			const Proposition *copula= propSet->getProposition(p_no);
			if (copula->getPredType() == Proposition::COPULA_PRED && copula->getNegation() != 0 &&
				copula->getArg(0)->getType() == Argument::MENTION_ARG && copula->getArg(1)->getType() == Argument::MENTION_ARG)
			{
				const Mention *lhs = copula->getArg(0)->getMention(mentionSet);
				const Mention *rhs = copula->getArg(1)->getMention(mentionSet);
				if (!lhs || !rhs)
					continue;
				if (lhs->getUID() == ment->getUID() || rhs->getUID() == ment->getUID()) {
					negation = copula->getNegation();
					break;
				}
			}
		}

		// for now no good way to represent these as a token span since they could be discontinuous
		if (negation)
			continue;

		Proposition *prop = propSet->getDefinition(ment->getIndex());

		if (prop != 0) {
			bool kill_descriptor = false;

			// Look for things like "the XXX descriptor" or "the descriptor of XXX"
			// If XXX is a person, this is risky in MT if this is not also a relation
			// I [Liz] don't honestly remember why I implemented this, but I must have had a reason :(
			for (int a = 1; a < prop->getNArgs(); a++) {
				if (prop->getArg(a)->getType() != Argument::MENTION_ARG)
					continue;
				const Mention *argMent = prop->getArg(a)->getMention(mentionSet);

				// if the argument is the same entity as the descriptor, that's OK
				Entity* argEnt = docTheory->getEntityByMention(argMent);
				if (argEnt != 0 && argEnt->getID() == entity_id)
					continue;

				if (argMent->getEntityType().matchesPER() && 
					(prop->getArg(a)->getRoleSym() == Argument::UNKNOWN_ROLE || 
					prop->getArg(a)->getRoleSym() == Symbol(L"of")))
				{
					// then this is a potentially bad mention, at least in MT
					// (is there a way to check for MT?)
					// unless there is a SERIF relation between the person and
					// our agent, let's kill this description

					// NOTE: this does kill some good things

					// special exception for "shi 'ite" and "shi 'i", which get missed as relations
					if (argMent->getNode()->getHeadWord() == Symbol(L"'ite") ||
						argMent->getNode()->getHeadWord() == Symbol(L"'i"))
						continue;

					kill_descriptor = true;
					for (int r = 0; r < relMentionSet->getNRelMentions(); r++) {
						const RelMention *rm = relMentionSet->getRelMention(r);
						if ((rm->getLeftMention() == argMent && rm->getRightMention() == ment) ||
							(rm->getRightMention() == argMent && rm->getLeftMention() == ment))
						{
							kill_descriptor = false;
							break;
						}
					}
					break;
				}
			}
			if (kill_descriptor) {
				continue;
			}
		}

		int start_token = node->getStartToken();
		int end_token = node->getEndToken();

		// look to expand token coverage for premods
		//   e.g. U.S. vice president Joe Biden --> "U.S. vice president", rather than just "president"
		const SynNode *addedNode = 0;
		if (start_token == end_token) {
			if (node->getParent() != 0 && node->getParent()->hasMention()) {
				const Mention *parentMent = mentionSet->getMentionByNode(node->getParent());
				Entity* parentEnt = docTheory->getEntityByMention(parentMent);
				if (parentMent->getMentionType() == Mention::NAME && 
					parentEnt != 0 && parentEnt->getID() == entity_id &&
					parentMent->getChild() != 0 &&
					parentMent->getChild()->getMentionType() == Mention::NONE)
				{
					const Mention *nameChild = parentMent->getChild();
					if (nameChild->getNode()->getStartToken() > parentMent->getNode()->getStartToken()) {
						start_token = parentMent->getNode()->getStartToken();
						end_token = nameChild->getNode()->getStartToken() - 1;
					}
				}
			}
		} else {
			// if the head node is not last node of the mention
			if (node->getHeadIndex() + 1 < node->getNChildren()) {
				const SynNode* firstPostMod = node->getChild(node->getHeadIndex() + 1);
				// if the first postmod is a comma, cut off all the postmods
				if (firstPostMod->getTag() == Symbol(L",")) {
					end_token = node->getHead()->getEndToken();
				}
			}
			// if the next node in the sentence starts with "to", add it on in some cases, e.g. "the first miner + to be rescued"
			if (node->getParent() != 0) {
				int our_index = -1;
				for (int i = 0; i < node->getParent()->getNChildren(); i++) {
					if (node->getParent()->getChild(i) == node) {
						our_index = i;
						break;
					}
				}
				if (our_index + 1 < node->getParent()->getNChildren()) {
					const SynNode *nextNode = node->getParent()->getChild(our_index + 1);
					int next_node_start = nextNode->getStartToken();
					Symbol nextNodeHeadword = nextNode->getCoveringNodeFromTokenSpan(next_node_start,next_node_start)->getHeadWord();
					std::wstring node_text = node->toTextString();
					if (nextNodeHeadword == Symbol(L"to") &&
						(node_text.find(L"first") != std::wstring::npos || 
						node_text.find(L"last") != std::wstring::npos || 
						node_text.find(L"second") != std::wstring::npos || 
						node_text.find(L"third") != std::wstring::npos))
					{
						end_token = nextNode->getEndToken();
						addedNode = nextNode;
					} 
				}
			}
		}

		
		// Kill appositives that don't start with a/an/the... they are usually bad
		std::wstring startToken = sTheory->getTokenSequence()->getToken(start_token)->getSymbol().to_string();
		if (ment->getParent() != 0 && ment->getParent()->getMentionType() == Mention::APPO) {
			if (startToken != L"the" && startToken != L"The" && 
				startToken != L"an" && startToken != L"An" && 
				startToken != L"a" && startToken != L"A")
				continue;
		}	


		Symbol lastToken = sTheory->getTokenSequence()->getToken(end_token)->getSymbol();
		while (end_token > start_token && 
			(lastToken == Symbol(L";") || lastToken == Symbol(L",") || 
			 lastToken == Symbol(L":") || lastToken == Symbol(L"-") ||
			 lastToken == Symbol(L"-LRB-") || lastToken == Symbol(L"+") ||
			 lastToken == Symbol(L"/") || lastToken == Symbol(L"\\")))
		{
			end_token--;
			lastToken = sTheory->getTokenSequence()->getToken(end_token)->getSymbol();
		}
		if (end_token < start_token)
			continue;

		// Start and end token are finalized now; so create fact
		Fact_ptr fact = boost::make_shared<Fact>(Symbol(L"Description"), Symbol(L"hardcoded"), 1.0, 1, sent_no, sent_no, 0, sTheory->getTokenSequence()->getNTokens() - 1);

		// Add full extent of description
		fact->addFactArgument(boost::make_shared<TextSpanFactArgument>(Symbol(L"Extent"), sent_no, sent_no, start_token, end_token));

		// Add headword
		int headword_token = ment->getNode()->getHeadPreterm()->getStartToken();		
		fact->addFactArgument(boost::make_shared<TextSpanFactArgument>(Symbol(L"Headword"), sent_no, sent_no, headword_token, headword_token));

		// Add the mention from which this is derived
		fact->addFactArgument(boost::make_shared<MentionFactArgument>(Symbol(L"Actor"), ment->getUID()));

		// If we extended the node in any way, add it in as a non-mention argument modifier
		if (addedNode != 0) {
			fact->addFactArgument(boost::make_shared<TextSpanFactArgument>(Symbol(L"NonMentionArg"), sent_no, sent_no, addedNode->getStartToken(), addedNode->getEndToken()));
		}

		if (prop != 0) {

			// Store each argument to this description's definitional proposition
			for (int a = 1; a < prop->getNArgs(); a++) {
				Argument *arg = prop->getArg(a);
				if (arg->getType() == Argument::MENTION_ARG) {
					const Mention *argMent = arg->getMention(mentionSet);
					if (argMent->getEntityType() != EntityType::getOtherType()) {
						fact->addFactArgument(boost::make_shared<MentionFactArgument>(Symbol(L"MentionArg"), argMent->getUID()));
					} else if (argMent->getEntityType() == EntityType::getOtherType()) {
						fact->addFactArgument(boost::make_shared<TextSpanFactArgument>(Symbol(L"NonMentionArg"), sent_no, sent_no, 
							argMent->getNode()->getHeadPreterm()->getStartToken(), argMent->getNode()->getHeadPreterm()->getEndToken()));
					}
				}
			}	

			// Now grab all modifiers to this description, e.g. "the /benign/ tyrant"
			for (int mp = 0; mp < propSet->getNPropositions(); mp++) {
				const Proposition *modifierProp = propSet->getProposition(mp);
				if (modifierProp->getPredType() == Proposition::MODIFIER_PRED &&
					modifierProp->getNArgs() > 0 &&
					modifierProp->getArg(0)->getType() == Argument::MENTION_ARG &&
					modifierProp->getArg(0)->getMentionIndex() == ment->getIndex())
				{
					const SynNode *modifierNode = modifierProp->getPredHead();
					fact->addFactArgument(boost::make_shared<TextSpanFactArgument>(Symbol(L"NonMentionArg"), sent_no, sent_no, 
							modifierNode->getStartToken(), modifierNode->getEndToken()));
				}
			}

			// Now get many of the modifying clauses, e.g. "who..."
			// These will almost never match directly, but they'll block "the man who said yes" from matching "the man who said no"
			for (int child = 0; child < node->getNChildren(); child++) {
				const SynNode *childNode = node->getChild(child);
				if (childNode->getTag() == Symbol(L"SBAR") && childNode->getEndToken() <= end_token) {
					fact->addFactArgument(boost::make_shared<TextSpanFactArgument>(Symbol(L"NonMentionArg"), sent_no, sent_no, 
							node->getChild(child)->getStartToken(), node->getChild(child)->getEndToken()));
				}
			}


		} else {
			if (start_token == end_token || start_token + 1 == end_token) {
				// ok
			} else {
				// too weird and risky
				continue;
			}
		}

		_documentFacts[entity_id].push_back(fact);
	}
	
}

void FactFinder::augmentEmploymentFact(PatternMatcher_ptr pm, Fact_ptr fact, bool found_date_through_pattern) {
	const Mention *employer = 0;
	const Mention *employee = 0;

	BOOST_FOREACH (FactArgument_ptr arg, fact->getArguments()) {
		if (MentionFactArgument_ptr mentArg = boost::dynamic_pointer_cast<MentionFactArgument>(arg)) {
			if (arg->getRole() == Symbol(L"Employer"))
				employer = mentArg->getMentionFromMentionSet(pm->getDocTheory());
			else if (arg->getRole() == Symbol(L"Employee"))
				employee = mentArg->getMentionFromMentionSet(pm->getDocTheory());
		} 
	}

	if (employer == 0 || employee == 0)
		return;

	const Entity *employerEntity = pm->getDocTheory()->getEntityByMention(employer);
	if (employerEntity == 0)
		return;

	const Entity *employeeEntity = pm->getDocTheory()->getEntityByMention(employee);
	if (employeeEntity == 0)
		return;
		
	int sent_no = employer->getSentenceNumber();
	float score = 0.0f;
	int title_start = -1;
	int title_end = -1;
	extractTitle(pm, employee, employer, employeeEntity->getID(), title_start, title_end, score);
	if (title_start != -1)
		fact->addFactArgument(boost::make_shared<TextSpanFactArgument>(Symbol(L"Title"), sent_no, sent_no, title_start, title_end));

	int status_start = -1;
	int status_end = -1;
	std::wstring statusWord;
	findEmploymentStatus(pm, employee, employer, title_start, title_end, status_start, status_end, statusWord);

	const ValueMention* nullValueMention = 0; // boost::make_shared requires us to do it this way
	if (status_start != -1) {
		fact->addFactArgument(boost::make_shared<TextSpanFactArgument>(Symbol(L"Status"), sent_no, sent_no, status_start, status_end));

		if (_futureEmploymentStatusWords.find(statusWord) != _futureEmploymentStatusWords.end() || 
			_pastEmploymentStatusWords.find(statusWord) != _pastEmploymentStatusWords.end())
		{
			fact->addFactArgument(boost::make_shared<ValueMentionFactArgument>(FF_NON_HOLD_DATE, nullValueMention, true));
		}
		if (_currentEmploymentStatusWords.find(statusWord) != _currentEmploymentStatusWords.end())
			fact->addFactArgument(boost::make_shared<ValueMentionFactArgument>(FF_HOLD_DATE, nullValueMention, true));
		
	} else {
		if (!found_date_through_pattern && title_start != -1 && canInferHoldDate(employer, employee))
			fact->addFactArgument(boost::make_shared<ValueMentionFactArgument>(FF_HOLD_DATE, nullValueMention, true));
	}
	
}

bool FactFinder::isEmploymentStatusWord(std::wstring word) {
	return (_futureEmploymentStatusWords.find(word) != _futureEmploymentStatusWords.end() || 
			_pastEmploymentStatusWords.find(word) != _pastEmploymentStatusWords.end() ||
			_currentEmploymentStatusWords.find(word) != _currentEmploymentStatusWords.end());			
}

void FactFinder::findEmploymentStatus(PatternMatcher_ptr pm, const Mention *employee, const Mention *employer,
									  int title_start, int title_end, int& start, int& end, std::wstring& statusWord) 
{	
	start = -1;
	end = -1;
	int sent_no = employee->getSentenceNumber();
	SentenceTheory *sTheory = pm->getDocTheory()->getSentenceTheory(sent_no);
	TokenSequence *ts = sTheory->getTokenSequence();

	int pre_title = title_start - 1;
	if (pre_title >= 0 && isEmploymentStatusWord(ts->getToken(pre_title)->getSymbol().to_string())) {
		start = pre_title;
		end = pre_title;
		statusWord = ts->getToken(pre_title)->getSymbol().to_string();
		return;
	}

	int pre_employer = employer->getNode()->getStartToken() - 1;
	if (pre_employer >= 0 && isEmploymentStatusWord(ts->getToken(pre_employer)->getSymbol().to_string())) {
		start = pre_employer;
		end = pre_employer;
		statusWord = ts->getToken(pre_employer)->getSymbol().to_string();
		return;
	}
	return;
}


void FactFinder::extractTitle(PatternMatcher_ptr pm, const Mention *employee, const Mention *employer,
							  int employee_entity_id, 
							  int& start_token, int& end_token, float& score) 
{
	int sent_no = employee->getSentenceNumber();
	SentenceTheory *sTheory = pm->getDocTheory()->getSentenceTheory(sent_no);
	const SynNode *root = sTheory->getPrimaryParse()->getRoot();

	if (employee->getMentionType() == Mention::NAME) {
		// no hope for finding the occupation here, e.g. "Barack Obama of the United States"
		if (employer->getMentionType() == Mention::NAME)
			score = 0.9f;
		else score = 0.5f;
	} else if (employee->getMentionType() == Mention::PRON) {
		// no hope for finding the occupation here
		if (employer->getMentionType() == Mention::NAME)
			score = 0.5f;
		else score = 0.1f;
	} 
	
	if (employee->getMentionType() != Mention::DESC) {
		return;
	}
	
	// OK, now we're cooking
	const SynNode* employeeNode = employee->getNode();
	int employee_start = employee->getNode()->getStartToken();
	int employee_end = employee->getNode()->getEndToken();
	int employee_head_start = employee->getHead()->getStartToken();
	int employee_head_end = employee->getHead()->getEndToken();
	int employer_start = employer->getNode()->getStartToken();
	int employer_end = employer->getNode()->getEndToken();

	score = 0.5;

	// Increase score if this is part of an appositive
	if (employee->getParent() != 0 && employee->getParent()->getMentionType() == Mention::APPO) {
		// good news! it's part of an appositive!
		if (employer->getMentionType() == Mention::NAME &&
			employee->getParent()->getNode()->getStartToken() <= employer_start &&
			employer_end <= employee->getParent()->getNode()->getStartToken())
		{
			score = 1.0f;
		} else score = 0.5f;
	} 

	// Find title and increase score if this is part of title construction, e.g. "U.S. vice president Joe Biden"
	const SynNode *employeeParentNode = employeeNode->getParent();
	if (employeeParentNode != 0 && employeeParentNode->hasMention() &&
		employeeParentNode->getStartToken() <= employer_start &&
		employer_end <= employeeParentNode->getEndToken()) 
	{		
		const Mention *parentMent = sTheory->getMentionSet()->getMentionByNode(employeeNode->getParent());
		Entity* parentEnt = pm->getDocTheory()->getEntityByMention(parentMent);
		if (parentMent->getMentionType() == Mention::NAME && parentEnt != NULL && 
			parentEnt->getID() == employee_entity_id)
		{
			// EMPLOYER EMPLOYEE_TITLE EMPLOYEE_NAME, e.g. "U.S. vice president Joe Biden"
			if (employer->getMentionType() == Mention::NAME)
				score = 1.0f;
			else score = 0.6f;
			if (employer_end < employee_start && parentMent->getHead()->getStartToken() > employee_end) {
				start_token = employer->getNode()->getEndToken() + 1;
				end_token = parentMent->getHead()->getStartToken() - 1;
			}
		} 
	}

	// If we haven't found a title yet, test to see if employer is enclosed in 
	//   employee mention, e.g. "Microsoft's vice president"
	if (start_token == -1) {
		if (employee_start <= employer_start && employer_end <= employee_end) {
			if (employer_end < employee_head_start) {
				// If the employer mention is before the head, take the text after the employer
				//   and up through the head. "BBN's president"
				start_token = employer_end + 1;
				end_token = employee_head_end;
			} else {
				// If not, try to stretch backwards inside core NPA. "vice president of BBN"
				start_token = employee_head_start;
				end_token = employee_head_end;
				while (start_token >= employee_start && start_token > 0) {
					const SynNode *precedingNode = root->getCoveringNodeFromTokenSpan(start_token-1,start_token-1)->getParent();
					std::wstring prev_word = precedingNode->getHeadWord().to_string();
					if (isEmploymentStatusWord(prev_word))
						break;
					if (prev_word != L"" && (prev_word.at(0) == L'-' || prev_word.find_first_of(L"0123456789") != std::wstring::npos))
						break;
					if (precedingNode->getTag() == Symbol(L"NN") || precedingNode->getTag() == Symbol(L"NNS") ||
						_validTitleAdjectives.find(prev_word) != _validTitleAdjectives.end())
						start_token--;
					else break;
				}
			}		
		} else {		
			score = 0.1f;
			start_token = employee_head_start;
			end_token = employee_head_end;
		}
	}

	if (start_token > 0 && start_token < end_token) {
		Symbol startTokenSym = sTheory->getTokenSequence()->getToken(start_token)->getSymbol();
		if (startTokenSym == Symbol(L"'s") || startTokenSym == Symbol(L"'") || startTokenSym == Symbol(L"''"))					
			start_token++;
		if (start_token < end_token) {
			startTokenSym = sTheory->getTokenSequence()->getToken(start_token)->getSymbol();
			if (isEmploymentStatusWord(startTokenSym.to_string()))
				start_token++;
		}
	}

	return;
}

bool FactFinder::canInferHoldDate(const Mention *employer, const Mention *employee) {
	const Mention *employeeParent = employee->getParent();
	if (employeeParent != 0 && employeeParent->getMentionType() == Mention::APPO &&
		employer->getMentionType() == Mention::NAME &&
		employer->getNode()->getStartToken() >= employee->getNode()->getStartToken() &&
		employer->getNode()->getEndToken() <= employee->getNode()->getEndToken())
	{
		return true;
	}
	return false;
}	




void FactFinder::pruneAndStoreFacts(DocTheory *docTheory,  UTF8OutputStream& kbStream, UTF8OutputStream& matchStream, int entity_id) {

	std::set<int> sentencesWithSpecificActions;
	std::set<int> sentencesWithQuotations;

	BOOST_FOREACH(Fact_ptr fact, _documentFacts[entity_id]) {
		int sent_no = fact->getStartSentence();
		if (fact->getFactType() == FF_VISITED_PLACE || fact->getFactType() == FF_QUOTATION || fact->getFactType() == FF_QUOTATION_ABOUT)
			sentencesWithSpecificActions.insert(sent_no);
		if (fact->getFactType() == FF_QUOTATION)
			sentencesWithQuotations.insert(sent_no);
	}
	
	BOOST_FOREACH(Fact_ptr fact, _documentFacts[entity_id]) {
		int sent_no = fact->getStartSentence();

		// For reciprocal facts, we can't use the same role for each player, since otherwise they get split up
		//   when we're handling potential lists (e.g. "Joe and Bob know Sheila")
		// But the ontology would really prefer them to be reciprocal.
		std::map<Symbol, Symbol>::const_iterator iter;
		for (iter = _reciprocalRoleLabelTransformations.begin(); iter != _reciprocalRoleLabelTransformations.end(); ++iter) {
			std::vector<FactArgument_ptr> args = fact->getArguments();
			for (std::vector<FactArgument_ptr>::const_iterator fa_iter = args.begin(); fa_iter != args.end(); ++fa_iter) {
				if ((*fa_iter)->getRole() == (*iter).first)
					(*fa_iter)->setRole((*iter).second);
			}
		}

		if (fact->getFactType() == FF_ACTION && sentencesWithSpecificActions.find(sent_no) != sentencesWithSpecificActions.end()) {
			// This sentence has a specific action fact already, so skip the generic one
			continue;
		}

		if (fact->getFactType() == FF_QUOTATION_ABOUT && sentencesWithQuotations.find(sent_no) != sentencesWithQuotations.end()) {
			// for each sentence skip a quotation_about if there is a direct quote
			continue;
		}
		
		// add to SERIF docTheory
		docTheory->getFactSet()->addFact(fact);

		// print to KB stream
		if (_print_factfinder_coldstart_info)
			printFactForColdStart(docTheory, fact, kbStream, matchStream);
	}
}


// ##
void FactFinder::updateMentionActorIdMap(int entityId, Symbol actorId, MentionUID mentionUID) {
	std::multimap<int,std::multimap<Symbol, MentionUID> >::iterator it = entityId2actorId2mentionId.find(entityId);
	if(it==entityId2actorId2mentionId.end()) { // not found
		std::multimap<Symbol, MentionUID> actorId2mentionId;
		actorId2mentionId.insert(std::pair<Symbol, MentionUID>(actorId, mentionUID));

		entityId2actorId2mentionId.insert(std::pair<int,std::multimap<Symbol, MentionUID> >(entityId, actorId2mentionId));
	}
	else {
		(*it).second.insert(std::pair<Symbol, MentionUID>(actorId, mentionUID));
	}

	mentionId2actorId.insert(std::pair<MentionUID, Symbol>(mentionUID, actorId));
}
// ## 

void FactFinder::writeAllMentionsForColdStart(const DocTheory *docTheory, UTF8OutputStream& kbStream, EntityLinker& entityLinker) {

	_entityLinkerCache.clear();

	// ##
	entityId2actorId2mentionId.clear();
	mentionId2actorId.clear();
	// ##

	if(docTheory==NULL) return;

	Symbol docID = docTheory->getDocument()->getName();

	EntitySet* entitySet = docTheory->getEntitySet();

	if(entitySet==NULL) return;

	// ## cache all actorID -> conf
	std::map<int, double> actorId2actorEntityConf;
	std::vector<ActorEntity_ptr> vecActorEntityPtr = docTheory->getActorEntitySet()->getAll();
	if(!vecActorEntityPtr.empty()) {
		for(unsigned int i=0; i<vecActorEntityPtr.size(); i++) {
			ActorEntity_ptr actorEntity = vecActorEntityPtr[i];

			if(actorEntity->getConfidence()>_min_actor_entity_match_conf_accept_at_mention_level 
					&& !actorEntity->isUnmatchedActor()) {

				double actor_match_conf = actorEntity->getConfidence();
				int actorId = actorEntity->getActorId().getId();
				actorId2actorEntityConf[actorId] = actor_match_conf;
			}
		}
	}
	// ##

	for(int i=0; i<entitySet->getNEntities(); i++) {
		Entity* entity = entitySet->getEntity(i);

		if(entity==NULL) continue;

		if(entity->getType().getName().is_null()) continue; 

		std::wstring entityType = entity->getType().getName().to_string();
		std::pair<std::wstring, const Mention*> pair = entity->getBestNameWithSourceMention(docTheory);
		std::wstring bestEntName = pair.first;
		const Mention* bestMention = pair.second;

		bestEntName = OutputUtil::escapeXML(bestEntName);
		if(bestEntName == L"NO_NAME" || bestMention == 0) {
			continue;
		}

		if(!bestMention->getUID().isValid()) continue;
			
		TokenSequence* tokenSequence = docTheory->getSentenceTheory(bestMention->getSentenceNumber())->getTokenSequence();
//		const SynNode* mentNode = bestMention->getNode();
//		CharOffset offset_start = tokenSequence->getToken(mentNode->getStartToken())->getStartCharOffset();
//		CharOffset offset_end = tokenSequence->getToken(mentNode->getEndToken())->getEndCharOffset();
		CharOffset offset_start = tokenSequence->getToken(bestMention->getAtomicHead()->getStartToken())->getStartCharOffset();
		CharOffset offset_end = tokenSequence->getToken(bestMention->getAtomicHead()->getEndToken())->getEndCharOffset();

//		Symbol entity_id_query = entityLinker.getEntityID(entityType, bestEntName, bestMention, docTheory);
                Symbol entity_id_query = entityLinker.getEntityID(entityType, bestEntName, entity, docTheory, _min_actor_match_conf);

		_entityLinkerCache[i] = entity_id_query;

		std::wstring entityType4coldStart = entityType.substr(0,3);
		boost::to_lower(entityType4coldStart);

//		if(entityType4coldStart!=L"per" && entityType4coldStart != L"org")
//			continue;

		kbStream << entity_id_query << "\t" << "type" << "\t" << entityType4coldStart << "\n";
		kbStream << entity_id_query << "\t" << "canonical_mention" << "\t" << "\"" << bestEntName << "\"" 
			<< "\t" << docID << "\t" << offset_start << "\t" << offset_end << "\n";

		for(int j=0; j<entity->getNMentions(); j++) {
			// Mention* ment = entity->getMention(j);
			const Mention* ment = docTheory->getMention(entity->getMention(j));

                        if(ment==NULL) continue;

			// ##
			Symbol mention_actor_id = entityLinker.getMentionActorID(entityType, ment, docTheory, _minActorPatternConf, _minEditDistance, _minPatternMatchScorePlusAssociationScore, actorId2actorEntityConf);
			if(mention_actor_id!=NULL && mention_actor_id!=entity_id_query)
				updateMentionActorIdMap(i, mention_actor_id, ment->getUID());
			// ##

			tokenSequence = docTheory->getSentenceTheory(ment->getSentenceNumber())->getTokenSequence();	
//			mentNode = ment->getNode();

//			std::wstring literalText = tokenSequence->toString(mentNode->getStartToken(), mentNode->getEndToken());
			std::wstring literalText = tokenSequence->toString(ment->getAtomicHead()->getStartToken(), ment->getAtomicHead()->getEndToken());

			std::wstring resolvedText = OutputUtil::escapeXML(literalText);
//			CharOffset offset_start = tokenSequence->getToken(mentNode->getStartToken())->getStartCharOffset();
//			CharOffset offset_end = tokenSequence->getToken(mentNode->getEndToken())->getEndCharOffset();
			CharOffset offset_start = tokenSequence->getToken(ment->getAtomicHead()->getStartToken())->getStartCharOffset();
			CharOffset offset_end = tokenSequence->getToken(ment->getAtomicHead()->getEndToken())->getEndCharOffset();

			if(!offset_start.is_defined() || !offset_end.is_defined()) {
				//std::cout << "OFFSETS ARE WONKY!" << "\n";
				continue;
			}

			if(entity_id_query.is_null()) {
				//std::cout << "ENTITY ID QUERY IS NULL!" << "\n";
				continue;
			}

			// ##
			Symbol actorIdToWrite;
			if(mention_actor_id==NULL)
				actorIdToWrite=entity_id_query;
			else
				actorIdToWrite=mention_actor_id;
			// ##

			// if this is a nested name
			if(ment->getMentionType() == Mention::NEST) 
			{ 
				kbStream << actorIdToWrite << "\t" << "mention" << "\t" << "\"" << resolvedText << "\""
				<< "\t" << docID << "\t" << offset_start << "\t" << offset_end
				<< "\t" << "1.0"
				<< "\t" << "1.0" << "\t" << "NEST"
				<< "\n";
			}
			else
			{
				kbStream << actorIdToWrite << "\t" << "mention" << "\t" << "\"" << resolvedText << "\""
				<< "\t" << docID << "\t" << offset_start << "\t" << offset_end
				<< "\t" << boost::lexical_cast<std::wstring>(((Mention*)ment)->getConfidence())
				<< "\t" << boost::lexical_cast<std::wstring>(ment->getLinkConfidence()) << "\t" << entity->getMentionConfidence(ment->getUID()).toString()
				<< "\n";
			}
		}

		// ## write first mention of each new splited entity as canonical mention
		std::multimap<int,std::multimap<Symbol, MentionUID> >::iterator it = entityId2actorId2mentionId.find(i);
		if(it==entityId2actorId2mentionId.end())
			continue;
		for(std::multimap<Symbol, MentionUID>::iterator iter=(*it).second.begin(); iter!=(*it).second.end(); iter=(*it).second.upper_bound(iter->first)) {
			Symbol actor_id_current = (*iter).first;
			const Mention* firstMention = docTheory->getMention((*iter).second);
			if(firstMention==NULL) continue;

                        tokenSequence = docTheory->getSentenceTheory(firstMention->getSentenceNumber())->getTokenSequence();

			CharOffset offset_start = tokenSequence->getToken(firstMention->getAtomicHead()->getStartToken())->getStartCharOffset();
			CharOffset offset_end = tokenSequence->getToken(firstMention->getAtomicHead()->getEndToken())->getEndCharOffset();
			kbStream << actor_id_current << "\t" << "type" << "\t" << entityType4coldStart << "\n";
			kbStream << actor_id_current << "\t" << "canonical_mention" << "\t" << "\"" << firstMention->toCasedTextString() << "\""
				<< "\t" << docID << "\t" << offset_start << "\t" << offset_end << "\n"; 
		}
		// ##
	}
}



void FactFinder::printFactForColdStart(const DocTheory *docTheory, Fact_ptr fact, UTF8OutputStream& kbStream, UTF8OutputStream& matchStream) 
{
	std::vector<FactArgument_ptr> arguments;
	BOOST_FOREACH(FactArgument_ptr arg, fact->getArguments()) {
		if (ValueMentionFactArgument_ptr vmentArg = boost::dynamic_pointer_cast<ValueMentionFactArgument>(arg)) {
			if (vmentArg->isDocDate())
				continue;
		}

		if (MentionFactArgument_ptr mentArg = boost::dynamic_pointer_cast<MentionFactArgument>(arg)) {
			const Mention *ment = mentArg->getMentionFromMentionSet(docTheory);
			if(ment==NULL) continue;
			const Entity *ent = docTheory->getEntitySet()->getEntityByMention(ment->getUID());
			if (ent == NULL || _entityLinkerCache.find(ent->getID()) == _entityLinkerCache.end())
				continue;
		}

		if (!isActivityDate(arg->getRole()))
			arguments.push_back(arg);
	}

	// Should always be of size 2 for ColdStart facts
	// This will usually skip Description and augmented Employment facts
	// It will also skip facts without two arguments that are either mentions-of-named-entities or text spans
	if (arguments.size() != 2)
		return;

	CharOffset fact_start_offset = docTheory->getSentenceTheory(fact->getStartSentence())->getTokenSequence()->getToken(fact->getStartToken())->getStartCharOffset();
	CharOffset fact_end_offset = docTheory->getSentenceTheory(fact->getEndSentence())->getTokenSequence()->getToken(fact->getEndToken())->getEndCharOffset();

	kbStream << docTheory->getDocument()->getName() << "\t";
	kbStream << fact->getFactType() << "\t";

	// force to write string text for value slots
	bool force_write_mention_text = false;
//	if(fact->getFactType() == Symbol(L"PerTitle"))
//		force_write_mention_text = true;
	std::wstring str_fact_type(fact->getFactType().to_string());
	if(str_fact_type.find(L"PerTitle") == 0 || str_fact_type.find(L"per_date_of_death")==0 || str_fact_type.find(L"DeathDate")==0 ||
		str_fact_type.find(L"per_origin") == 0 || str_fact_type.find(L"org_date_founded") == 0 || str_fact_type.find(L"FoundingDate") == 0 ||
		str_fact_type.find(L"per_alternate_names") == 0 || str_fact_type.find(L"org_alternate_names") == 0 || str_fact_type.find(L"per_date_of_birth") == 0 ||
		str_fact_type.find(L"org_date_dissolved") == 0 ||  str_fact_type.find(L"org_website") == 0 ||  str_fact_type.find(L"per_charges") == 0 ||
		 str_fact_type.find(L"per_title") == 0 || str_fact_type.find(L"BirthDate") == 0)
	        force_write_mention_text = true;



	std::wstring strTextArg1 = L"";
	std::wstring strTextArg2 = L"";
	std::wstring strTmpForArgSwap = L"";

	std::wstring strTextArg1canonical = L"";
	std::wstring strTextArg2canonical = L"";

	std::wstring strTextFact = getMultiSentenceString(docTheory, fact->getStartSentence(), fact->getEndSentence(), fact->getStartToken(), fact->getEndToken());
	strTextFact = normalizeString(strTextFact);

	// Make the ordering of the arguments deterministic
	Mention::Type arg1mentionType;
	Mention::Type arg2mentionType;

	std::wstring arg1 = printColdStartArgument(docTheory, arguments.at(0), force_write_mention_text, strTextArg1, strTextArg1canonical, arg1mentionType);
	std::wstring arg2 = printColdStartArgument(docTheory, arguments.at(1), force_write_mention_text, strTextArg2, strTextArg2canonical, arg2mentionType);
	if (arg1.compare(arg2) < 0) {
		kbStream << arg1 << "\t" << arg2 << "\t";
		matchStream << fact->getFactType() << "\t" 
			<< strTextArg1canonical << "\t" << strTextArg1 << "\t" << arg1mentionType << "\t"
			<< strTextArg2canonical << "\t" << strTextArg2 << "\t" << arg2mentionType << "\n";
	}
	else {
		kbStream << arg2 << "\t" << arg1 << "\t";
		matchStream << fact->getFactType() << "\t"
			<< strTextArg2canonical << "\t" << strTextArg2 << "\t" << arg2mentionType << "\t"
			<< strTextArg1canonical << "\t" << strTextArg1 << "\t" << arg1mentionType << "\n";

		// swap argument text
		strTmpForArgSwap = strTextArg1;
		strTextArg1 = strTextArg2;
		strTextArg2 = strTmpForArgSwap;
	}

	kbStream << fact_start_offset << "\t";
	kbStream << fact_end_offset << "\t";
	kbStream << fact->getScore() << "\t";
	
	strTextArg1 = normalizeString(strTextArg1);
	strTextArg2 = normalizeString(strTextArg2);

	kbStream << strTextArg1 << "\t";
	kbStream << strTextArg2 << "\t";
	kbStream << strTextFact << "\n";
}

void FactFinder::writeNestedNamesFactsForColdStart(const DocTheory *docTheory, UTF8OutputStream& kbStream) {
	/*
	NB: For every nested name (which currently only consists of GPE's found in ORG's)
	add a fact to the ColdStart KB which says that the ORG is located in the GPE

	Output should be tab separated with the following columns:
	1) docid				(e.g. 1360778221-fcfe9f4be10252fdfcd8dba7341f13e4)
	2) fact_type			(e.g. Employer)
	3) role1				(always AGENT1)
	4) entityId_query		(e.g. Actor_658847 --> looked up in entity linker cache)
	5) type				(e.g. PER)
	6) subtype				(e.g. Individual)
	7) start				(e.g. 176)
	8) end					(e.g. 178)
	9) mention_confidence  (e.g. 0.878000021)
	10) linkConfidence		(e.g. 0.755999982)
	11) brandyConfidence	(e.g. TitleDesc) [TWO TABS]
	12) role2				(e.g. Employer)
	13) entityId_filler		(e.g. :ORG_signature_brands --> looked up in entity linker cache)
	14) type				(e.g. ORG)
	15) subtype				(e.g. UNDET)
	16) start				(e.g. 159)
	17) end					(e.g. 174)
	18) 1mention_confidence  (e.g. 0.0879999995)
	19) linkConfidence		(e.g. 1)
	20) brandyConfidence	(e.g. AnyName) [TWO TABS]
	21) start_sentence		(e.g. 159)
	22) end_sentence		(e.g. 212)
	23) confidence_fact		(e.g. 0.9)
	24) role1 text			(e.g. "Schneider")
	25) role2 text			(e.g. "Signature Brands")
	26) string_sentence		(e.g. Signature Brands	CEO	Signature Brands CEO Schneider retires after 20 years .)
	*/
	if(docTheory==NULL) return;
	Symbol docID = docTheory->getDocument()->getName();
	Symbol gpe_fact_type = Symbol(L"GPENestedInORG");
	Symbol org_fact_type = Symbol(L"ORGNestedInORG");
	Symbol role1 = Symbol(L"AGENT1");
	Symbol fact_type;
	Symbol role2;

	for (int sent_no = 0; sent_no < docTheory->getNSentences(); sent_no++) 
	{
		if (DEBUG) 
			std::cout << "SENTENCE NUMBER: " << boost::lexical_cast<std::wstring>(sent_no) << "\n";
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(sent_no);
		if (!sentTheory) {
			if (DEBUG) 
				std::cout << "SENTENCE THEORY NOT FOUND!" << "\n";
			continue;
		}
		MentionSet *mentionSet = sentTheory->getMentionSet();
		TokenSequence *tokenSequence = sentTheory->getTokenSequence();
		if (!mentionSet) {
			if (DEBUG) 
				std::cout << "MENTION SET NOT FOUND!" << "\n";
			continue;
		}
		if (!tokenSequence) {
			if (DEBUG)
				std::cout << "TOKEN SEQUENCE NOT FOUND!" << "\n";
			continue;
		}
		for (int m = 0; m < mentionSet->getNMentions(); m++) 
		{
			if (DEBUG)
				std::cout << "MENTION NUMBER: " << boost::lexical_cast<std::wstring>(m) << "\n";

			Mention *nestedMent = mentionSet->getMention(m);
			if (nestedMent->getMentionType() != Mention::NEST)
				continue;

			int nestedTokenStart = nestedMent->getAtomicHead()->getStartToken();
			int nestedTokenEnd = nestedMent->getAtomicHead()->getEndToken();

			Mention *fullMent = NULL;
			for (int om = 0; om < mentionSet->getNMentions(); om++) 
			{
				Mention *tmpMent = mentionSet->getMention(om);
				if (tmpMent->getMentionType() != Mention::NAME)
					continue;
				if (tmpMent->getAtomicHead()->getStartToken() <= nestedTokenStart 
					&&
					tmpMent->getAtomicHead()->getEndToken() >= nestedTokenEnd)
					fullMent = tmpMent;
			}
			if (!fullMent) {
				if (DEBUG) 
					std::cout << "parent not found for nested name: " << nestedMent->getAtomicHead()->toTextString() << "\n";
				continue;
			}

			// We have a nested name! 

			CharOffset sentStartChar = tokenSequence->getToken(0)->getStartCharOffset();
			CharOffset sentEndChar = tokenSequence->getToken(tokenSequence->getNTokens()-1)->getEndCharOffset();

			// 1) get info for the parent mention
			EntityType fullMentionType = fullMent->getEntityType();
			EntitySubtype fullMentionSubtype = fullMent->getEntitySubtype();
			int fullTokenStart = fullMent->getAtomicHead()->getStartToken();
			CharOffset fullCharStart = tokenSequence->getToken(fullTokenStart)->getStartCharOffset();
			int fullTokenEnd = fullMent->getAtomicHead()->getEndToken();
			CharOffset fullCharEnd = tokenSequence->getToken(fullTokenEnd)->getEndCharOffset();
			const Entity *fullEnt = docTheory->getEntitySet()->getEntityByMention(fullMent->getUID());	
			if (!fullEnt) {
				if (DEBUG) 
					std::cout << "can't find full entity for mention" << "\n";
				continue;
			}
			if(_entityLinkerCache.find(fullEnt->getID()) == _entityLinkerCache.end()) {
				if (DEBUG) 
					std::cout << "fullEntId not in entity linker cache" << "\n";
				continue;
			}
			Symbol fullEntId = _entityLinkerCache[fullEnt->getID()];
			// 2) get info for the nested mention
			EntityType nestedMentionType = nestedMent->getEntityType();
			if (nestedMentionType == EntityType::getGPEType())
				fact_type = role2 = gpe_fact_type;
			else if (nestedMentionType == EntityType::getORGType())
				fact_type = role2 = org_fact_type;
			else
				continue;
			EntitySubtype nestedMentionSubtype = nestedMent->getEntitySubtype();
			CharOffset nestedCharStart = tokenSequence->getToken(nestedTokenStart)->getStartCharOffset();
			CharOffset nestedCharEnd = tokenSequence->getToken(nestedTokenEnd)->getEndCharOffset();
			const Entity *nestedEnt = docTheory->getEntitySet()->getEntityByMention(nestedMent->getUID());	
			if (!nestedEnt) {
				if (DEBUG) 
					std::cout << "can't find nested entity for mention" << "\n";
				continue;
			}
			if(_entityLinkerCache.find(nestedEnt->getID()) == _entityLinkerCache.end()) {
				if (DEBUG) 
					std::cout << "fullEntId not in entity linker cache" << "\n";
				continue;
			}
			Symbol nestedEntId = _entityLinkerCache[nestedEnt->getID()];
			double conf;
			if (nestedEnt->getNMentions() > 1) 
				conf = 0.95;
			else 
				conf = 0.9;

			if(fullEntId.is_null()) 
			{
				if (DEBUG)
					std::cout << "fullEntId is null" << "\n";
				continue;
			}
			if(nestedEntId.is_null()) 
			{
				if (DEBUG)
					std::cout << "nestedEntId is null" << "\n";
				continue;
			}

			// 3) write out relation to the kb
			kbStream << docID << "\t";  //1
			kbStream << fact_type << "\t"; //2
			kbStream << role1 << "\t"; //3
			kbStream << fullEntId << "\t"; //4
			kbStream << fullMentionType.getName() << "\t"; //5
			kbStream << fullMentionSubtype.getName() << "\t"; //6
			kbStream << boost::lexical_cast<std::wstring>(fullCharStart) << "\t"; //7
			kbStream << boost::lexical_cast<std::wstring>(fullCharEnd) << "\t"; //8
			kbStream << boost::lexical_cast<std::wstring>(fullMent->getConfidence()) << "\t"; //9
			kbStream << boost::lexical_cast<std::wstring>(fullMent->getLinkConfidence()) << "\t" ; //10
			kbStream << fullEnt->getMentionConfidence(fullMent->getUID()).toString() << "\t\t"; //11 [TWO TABS]
			kbStream << role2 << "\t"; //12
			kbStream << nestedEntId << "\t"; //13
			kbStream << nestedMentionType.getName() << "\t"; //14
			kbStream << nestedMentionSubtype.getName() << "\t"; //15
			kbStream << boost::lexical_cast<std::wstring>(nestedCharStart) << "\t"; //16
			kbStream << boost::lexical_cast<std::wstring>(nestedCharEnd) << "\t"; //17
			kbStream << boost::lexical_cast<std::wstring>(nestedMent->getConfidence()) << "\t"; //18
			kbStream << boost::lexical_cast<std::wstring>(nestedMent->getLinkConfidence()) << "\t" ; //19
			kbStream << nestedEnt->getMentionConfidence(nestedMent->getUID()).toString() << "\t\t"; //20 [TWO TABS]
			kbStream << boost::lexical_cast<std::wstring>(sentStartChar) << "\t"; //21
			kbStream << boost::lexical_cast<std::wstring>(sentEndChar) << "\t"; //22
			kbStream << boost::lexical_cast<std::wstring>(conf) << "\t"; //23
			kbStream << normalizeString(fullMent->getAtomicHead()->toTextString()) << "\t"; //24
			kbStream << normalizeString(nestedMent->getAtomicHead()->toTextString()) << "\t"; //25
			kbStream << normalizeString(getMultiSentenceString(docTheory, sent_no, sent_no, 0, tokenSequence->getNTokens()-1)); //26
			kbStream << "\n";
		}
	}
}

std::wstring FactFinder::normalizeString(std::wstring str) {
	boost::replace_all(str,L"\n",L" ");
	boost::replace_all(str,L"\r",L" ");
	boost::replace_all(str,L"\t",L" ");

	return str;
}

std::wstring FactFinder::printColdStartArgument(const DocTheory *docTheory, FactArgument_ptr arg, bool force_write_mention_text, std::wstring& strTextArg, std::wstring& strTextArgCanonical, Mention::Type& argMentionType) {

	// role1 thing1 thing1_type thing1_subtype start_offset1 end_offset1
	argMentionType = Mention::NONE;

	std::wstringstream outStream;

	outStream << arg->getRole() << L"\t";

	bool isAgent1 = arg->getRole() == Symbol(L"AGENT1");

	// argument sentno & offsets
	int argSentStart = 0, argSentEnd = 0;
	int argTokenStart = 0, argTokenEnd = 0;

	if (MentionFactArgument_ptr mentArg = boost::dynamic_pointer_cast<MentionFactArgument>(arg)) {
		const Mention *ment = mentArg->getMentionFromMentionSet(docTheory);

		argMentionType = ment->getMentionType();

		argTokenStart = ment->getAtomicHead()->getStartToken();
		argTokenEnd = ment->getAtomicHead()->getEndToken();

		const Entity *ent = docTheory->getEntitySet()->getEntityByMention(ment->getUID());

		// get best string
		std::pair<std::wstring, const Mention*> pair = ent->getBestNameWithSourceMention(docTheory);
		strTextArgCanonical = pair.first;
		strTextArgCanonical = OutputUtil::escapeXML(strTextArgCanonical);
		//

		int sent_no = Mention::getSentenceNumberFromUID(ment->getUID());

		argSentStart = sent_no;
		argSentEnd = sent_no;

		const TokenSequence *ts = docTheory->getSentenceTheory(sent_no)->getTokenSequence();

		if(!isAgent1 && force_write_mention_text) {
			outStream << L"\"" << getMultiSentenceString(docTheory, argSentStart, argSentEnd, argTokenStart, argTokenEnd) << L"\"\t";
		}
		else {
			// ##
			std::map<MentionUID, Symbol>::iterator it = mentionId2actorId.find(ment->getUID());
			if(it!=mentionId2actorId.end())
				outStream << (*it).second << L"\t";
			else
				// We already tested above to make sure this exists
				outStream << _entityLinkerCache[ent->getID()] << L"\t";
			// ##
		}

		EntitySubtype subtype;
		if (ent) {
			subtype = ent->guessEntitySubtype(docTheory);
		} else {
			subtype = ment->getEntitySubtype();
		}

		outStream << ment->getEntityType().getName() << L"\t";
		outStream << subtype.getName() << L"\t";

		outStream << ts->getToken(argTokenStart)->getStartCharOffset() << L"\t";
		outStream << ts->getToken(argTokenEnd)->getEndCharOffset() << L"\t";

		outStream << boost::lexical_cast<std::wstring>(((Mention*)ment)->getConfidence()) << L"\t";
		outStream << boost::lexical_cast<std::wstring>(ment->getLinkConfidence()) << L"\t";
		outStream << ent->getMentionConfidence(ment->getUID()).toString() << L"\t";
	
	} else {
		if (ValueMentionFactArgument_ptr vmentArg = boost::dynamic_pointer_cast<ValueMentionFactArgument>(arg)) {
			const ValueMention* valMent = vmentArg->getValueMention();

			argTokenStart = valMent->getStartToken();
			argTokenEnd = valMent->getEndToken();

			int sent_no = valMent->getSentenceNumber();
			
			argSentStart = sent_no;
			argSentEnd = sent_no;

			bool gotTimexStr=false;
			if(valMent->isTimexValue()) {
				const Value* val = valMent->getDocValue();
				if(val!=NULL) {
					const wchar_t* normalizedDate = val->getTimexVal().to_string();
					if(normalizedDate!=NULL) {
						std::wstring str_normalized_date(normalizedDate);
						outStream << L"\"" << str_normalized_date << L"\"\t";
						gotTimexStr=true;
					}
				}
			}

			if(!gotTimexStr)
				outStream << L"\"" << getMultiSentenceString(docTheory, sent_no, sent_no, argTokenStart, argTokenEnd) << L"\"\t";

			// These could print the value type if desired
			outStream << L"NULL" << L"\t";
			outStream << L"NULL" << L"\t";

			// We have already tested to make sure this isn't a document date, above, so valMent should be nonzero
			const TokenSequence *ts = docTheory->getSentenceTheory(sent_no)->getTokenSequence();
			outStream << ts->getToken(argTokenStart)->getStartCharOffset() << L"\t";
			outStream << ts->getToken(argTokenEnd)->getEndCharOffset() << L"\t";

			outStream << L"NULL" << L"\t";
			outStream << L"NULL" << L"\t";
			outStream << L"NULL" << L"\t";
		
		} else if (TextSpanFactArgument_ptr tspanArg = boost::dynamic_pointer_cast<TextSpanFactArgument>(arg)) {
			argTokenStart = tspanArg->getStartToken();
			argTokenEnd = tspanArg->getEndToken();
			
			argSentStart = tspanArg->getStartSentence();
			argSentEnd = tspanArg->getEndSentence();

			outStream << L"\"" << getMultiSentenceString(docTheory, argSentStart, argSentEnd, argTokenStart, argTokenEnd) << L"\"\t";
			outStream << L"NULL" << L"\t";
			outStream << L"NULL" << L"\t";

			outStream << docTheory->getSentenceTheory(tspanArg->getStartSentence())->getTokenSequence()->getToken(argTokenStart)->getStartCharOffset() << L"\t";
			outStream << docTheory->getSentenceTheory(tspanArg->getEndSentence())->getTokenSequence()->getToken(argTokenEnd)->getEndCharOffset() << L"\t";	

                        outStream << L"NULL" << L"\t";
                        outStream << L"NULL" << L"\t";
                        outStream << L"NULL" << L"\t";
		}

		strTextArgCanonical = getMultiSentenceString(docTheory, argSentStart, argSentEnd, argTokenStart, argTokenEnd);
	}

	strTextArg = getMultiSentenceString(docTheory, argSentStart, argSentEnd, argTokenStart, argTokenEnd);

	boost::replace_all(strTextArgCanonical,L"\t",L" ");

	return outStream.str();
}
