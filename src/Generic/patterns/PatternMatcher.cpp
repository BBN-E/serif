// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/patterns/ExtractionPattern.h"
#include "Generic/patterns/RegexPattern.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/PatternSet.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/GenericPFeature.h"
#include "Generic/patterns/features/CoveredPropNodePFeature.h"
#include "Generic/patterns/QueryDate.h"
#include "Generic/patterns/SimpleSlot.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/TimexUtils.h"
#include "Generic/common/ParamReader.h"
#include "Generic/PropTree/PropNode.h"
#include "Generic/PropTree/PropEdgeMatch.h"
#include "Generic/PropTree/PropForestFactory.h"
#include "Generic/PropTree/expanders/DistillationExpander.h"
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/foreach.hpp>
#include <algorithm>


using boost::make_shared;
namespace { // Private expander for matching methods.
	
	DistillationDocExpander _expander;
}


PatternMatcher_ptr PatternMatcher::makePatternMatcher(const DocTheory *docTheory, PatternSet_ptr patternSet, 
													  const QueryDate *activityDate,
								   PatternMatcher::SnippetCombinationMethod snippet_combination_method,
								   Symbol::HashMap<const AbstractSlot*> slots, const NameDictionary *nameDictionary,
								bool reset_mention_to_entity_cache) {
	PatternMatcher_ptr pm = boost::make_shared<PatternMatcher>(docTheory, patternSet, activityDate, snippet_combination_method, slots, nameDictionary);
	pm->initializePatternMatcher(reset_mention_to_entity_cache);
	return pm;
}

PatternMatcher_ptr PatternMatcher::makePatternMatcher(const DocTheory* docTheory, Pattern_ptr pattern,  
													  bool reset_mention_to_entity_cache)
{
	PatternSet_ptr patternSet = make_shared<PatternSet>(pattern);
	return makePatternMatcher(docTheory, patternSet); 
}

PatternMatcher_ptr PatternMatcher::makePatternMatcher(const AlignedDocSet_ptr docSet, PatternSet_ptr patternSet,  
													  const QueryDate *activityDate,
								   PatternMatcher::SnippetCombinationMethod snippet_combination_method,
								   Symbol::HashMap<const AbstractSlot*> slots, const NameDictionary *nameDictionary,
									bool reset_mention_to_entity_cache) {
	PatternMatcher_ptr pm = boost::make_shared<PatternMatcher>(docSet, patternSet, activityDate, snippet_combination_method, slots, nameDictionary);
	pm->initializePatternMatcher(reset_mention_to_entity_cache);
	return pm;
}

PatternMatcher_ptr PatternMatcher::makePatternMatcher(const AlignedDocSet_ptr docSet, Pattern_ptr pattern,  
													  bool reset_mention_to_entity_cache)
{
	PatternSet_ptr patternSet = make_shared<PatternSet>(pattern);
	return makePatternMatcher(docSet, patternSet); 
}

PatternMatcher::PatternMatcher(const DocTheory *docTheory, PatternSet_ptr patternSet, const QueryDate *activityDate,
							   PatternMatcher::SnippetCombinationMethod snippet_combination_method,
							   Symbol::HashMap<const AbstractSlot*> slots, const NameDictionary *nameDictionary)
: _docSet(boost::make_shared<AlignedDocSet>()), _patternSet(patternSet), 
  _activityDate(activityDate), 
  _snippet_combination_method(snippet_combination_method),
  _nameDictionary(nameDictionary), _slots(slots)
{
	_docSet->loadDocTheory(LanguageVariant::getLanguageVariant(), docTheory);
	_activeLanguageVariant = LanguageVariant::getLanguageVariant();
	if (!ParamReader::isParamTrue("pattern_matcher_ignore_dates")) 
		_documentDate = docTheory->getDocument()->getDocumentDate();
}

PatternMatcher::PatternMatcher(const AlignedDocSet_ptr docSet, PatternSet_ptr patternSet, const QueryDate *activityDate,
							   PatternMatcher::SnippetCombinationMethod snippet_combination_method,
							   Symbol::HashMap<const AbstractSlot*> slots, const NameDictionary *nameDictionary)
: _docSet(docSet), _patternSet(patternSet), _activityDate(activityDate), 
  _snippet_combination_method(snippet_combination_method),
  _nameDictionary(nameDictionary), _slots(slots)
{ 
	_activeLanguageVariant = docSet->getDefaultLanguageVariant();
	if (!ParamReader::isParamTrue("pattern_matcher_ignore_dates")) 
		_documentDate = getDocTheory()->getDocument()->getDocumentDate();
}

void PatternMatcher::initializePatternMatcher(bool reset_mention_to_entity_cache) {
	//TODO: BLL Update slot scoring to be bilingual as well
	// We can move it into the loop below as long as we move the accessing of 
	//_slotSentenceMatchScores and _slotSentenceMatchFeatures to be per DocTheory
	
	//Patterns access a stored table for Mention to Entity lookup.  If there is a possibility
	//that the EntitySet has changed (e.g. Patterns are used during sentence level relation finding, pre-coref and again after coref), 
	//then the table needs to be explicitly cleared (it will be recreated with the first call to EntitySet::lookUpEntityForMention().
	//If the creator of the PatternMatcher can ensure that the entityset hasn't changed and there are lots of PatternMatchers being created
	//(e.g. LearnIt), maintaining the same cache improves speed.
	if(reset_mention_to_entity_cache){
		BOOST_FOREACH(LanguageVariant_ptr langVar, _docSet->getLanguageVariants()) {
			setActiveLanguageVariant(langVar);
			EntitySet* entitySet = _docSet->getDocTheory(_activeLanguageVariant)->getEntitySet();
			if(entitySet != 0){
				entitySet->clearEntityByMentionTable();
			}
		}
	}
	typedef Symbol::HashMap<const AbstractSlot*>::const_iterator SlotIter;
	for (SlotIter it=_slots.begin(); it!=_slots.end(); ++it) {
		const AbstractSlot* slot = (*it).second;
		if (!ParamReader::isParamTrue("accept_bolt_queries") && (!ParamReader::isParamTrue("topicality_only_for_selected_slot_types") || slot->requiresProptrees())) {
			fillSlotScores(slot);
		}
	}
	if (SessionLogger::dbg_or_msg_enabled("PatternMatcher")) {
		printSlotScores();
	}

	BOOST_FOREACH(LanguageVariant_ptr langVar, _docSet->getLanguageVariants()) {
		setActiveLanguageVariant(langVar);
		_docInfos[langVar] = boost::make_shared<PatternMatchDocumentInfo>();

		// If we are running FactFinder, we are going to handle the entity labeling external
		//   to this initalization, so let's skip this (and the associated warnings that will be written)
		if (!ParamReader::isParamTrue("skip_automatic_entity_labeling"))
			labelEntities();
		runDocumentPatterns();
	}
	setActiveLanguageVariant(_docSet->getDefaultLanguageVariant());
}

const LocatedString *PatternMatcher::getString() const { 
	return getDocTheory()->getDocument()->getOriginalText(); 
}


void PatternMatcher::labelEntities() {
	//SessionLogger::dbg("PatternMatcher") << "PatternMatcher::labelEntities()\n";
	size_t n_entity_label_patterns = _patternSet->getNEntityLabelPatterns();
	for (size_t i=0; i<n_entity_label_patterns; ++i) {
		EntityLabelPattern_ptr labelPattern = _patternSet->getNthEntityLabelPattern(i);
		if (labelPattern->hasPattern() || _slots.size() == 0) {
			labelPattern->labelEntities(shared_from_this(), getDocumentInfo()->entityLabelFeatureMap);
		} else {
			const AbstractSlot* abstractSlot = getSlot(labelPattern->getLabel());
			if (abstractSlot == 0)
				continue;
			const SimpleSlot* simpleSlot = dynamic_cast<const SimpleSlot*>(abstractSlot);
			if (simpleSlot) {
				labelEntities(labelPattern->getLabel(), simpleSlot, 0.8f, false, false);
			} else if (labelPattern->getLabel() == Symbol(L"LOC_RESTRICTION") || labelPattern->getLabel() == Symbol(L"SUB_LOC_RESTRICTION")) {
				; // For now we are not labeling these.  TODO: Figure out how to handle them.
			} else {
				std::wstringstream wss;
				wss << "Pattern-less entity label patterns must correspond to slot labels.  You provided a label of " << labelPattern->getLabel();
				throw UnexpectedInputException("PatternMatcher::labelEntities", wss);
			}
		}
	}	
	if (SessionLogger::dbg_or_msg_enabled("PatternMatcher")) {
		printEntityLabels();
	}
}

void PatternMatcher::labelEntities(const Symbol& label, const SimpleSlot* slot, float min_topicality, bool match_names_loosely, bool ultimate_backoff) {
	const EntitySet *entitySet = getDocTheory()->getEntitySet();
	bool slot_has_name = slot->hasName();
	//SessionLogger::dbg("PatternMatcher") << "Our slot has a name? " << slot_has_name << "\n";

	// Construct some regular expressions
	boost::wregex * regex_str_100 = 0;
	boost::wregex * regex_str_80 = 0;
	boost::wregex * regex_str_40 = 0;
	boost::wregex * regex_str_10 = 0;
	boost::wregex * regex_str_00 = 0;
	if (slot_has_name) {
		std::wstring str_100 = slot->getRegexNameString(1.0f);
		std::wstring str_80  = slot->getRegexNameString(0.8f);
		std::wstring str_40  = slot->getRegexNameString(0.4f);
		std::wstring str_10  = slot->getRegexNameString(0.1f);
		std::wstring str_00  = slot->getRegexNameString(0.0f);
		//SessionLogger::dbg("PatternMatcher") << "100: " << str_100 << "\n";
		//SessionLogger::dbg("PatternMatcher") << " 80: " << str_80 << "\n";
		//SessionLogger::dbg("PatternMatcher") << " 40: " << str_40 << "\n";
		//SessionLogger::dbg("PatternMatcher") << " 10: " << str_10 << "\n";
		//SessionLogger::dbg("PatternMatcher") << " 00: " << str_00 << "\n";
		regex_str_100 = _new boost::wregex(str_100);
		regex_str_80  = _new boost::wregex(str_80);
		regex_str_40  = _new boost::wregex(str_40);
		regex_str_10  = _new boost::wregex(str_10);
		regex_str_00  = _new boost::wregex(str_00);
	} else {
		regex_str_00 = _new boost::wregex(slot->getBackoffRegexText());
	}

	// Loop over our entities
	for (int entid = 0; entid < entitySet->getNEntities(); entid++) {
		Entity *entity = entitySet->getEntity(entid);

		// Look for a topicality match.  Note we only consider the first mention for some reason.
		if (!slot_has_name && entity->getNMentions() != 0) {
			const Mention *ment = getDocTheory()->getEntitySet()->getMention(entity->getMention(0));
			float topic_score = getFullScore(slot, ment->getSentenceNumber(), ment);
			if (topic_score > min_topicality) {
				if (SessionLogger::info_or_msg_enabled("SimpleSlot")) {
					SessionLogger::info("SimpleSlot") << "Selecting entity for slot " << label << " via topicality: " << topic_score << "\n";	
					for (int mentno = 0; mentno < entity->getNMentions(); mentno++) {
						const Mention *ment = entitySet->getMention(entity->getMention(mentno));
						SessionLogger::info("SimpleSlot") << ment->getNode()->toTextString() << "\n";
					}
				}
				setEntityLabelConfidence(label, entid, 1.0);
				continue;
			}
		}

		// Loop over mentions and look for a regex match
		float best_confidence = 0.0f;
		for (int mentno = 0; mentno < entity->getNMentions(); mentno++) {
			const Mention *ment = entitySet->getMention(entity->getMention(mentno));
			std::wstring mentString;
			if (slot_has_name) {
				// We only match on the actual name: Chinese premier Jiang Zemin --> "Jiang Zemin" 
				if (ment->getMentionType() == Mention::NAME) {
					mentString = ment->getNode()->getHeadPreterm()->getParent()->toTextString();
				} else { 
					continue;
				}
			} else {				
				// But if there is no name, we grab the whole string
				mentString = ment->getNode()->toTextString();
			}
			//if(_fullQuery->getLanguage() == DistillUtilities::CHINESE){
			//	//do normalization to remove white space ect. (this was done to regex_str as well)
			//	wchar_t norm_string[1024];
			//	DistillUtilities::CHNormalizeStringForDB(mentString.c_str(), norm_string,1023);
			//	mentString = norm_string;
			//}
			std::wstring modifiedMentString = UnicodeUtil::normalizeTextString(mentString);	
			boost::match_results<std::wstring::const_iterator> matchResult;
			std::wstring fullyModifiedMentString = UnicodeUtil::normalizeNameString(modifiedMentString);				
			//SessionLogger::dbg("PatternMatcher") << "mentString: " << mentString;
			//SessionLogger::dbg("PatternMatcher") << " modified: " << modifiedMentString;
			//SessionLogger::dbg("PatternMatcher") << " fully modified: " << fullyModifiedMentString << "\n";
			if (regex_str_100 &&
				(boost::regex_search(mentString, matchResult, *regex_str_100) ||
					boost::regex_search(modifiedMentString, matchResult, *regex_str_100) ||
					boost::regex_search(fullyModifiedMentString, matchResult, *regex_str_100)))
				{
					best_confidence = std::max(1.0F, best_confidence);
					break; //can't do any better!
				} else if (regex_str_80 &&
					(boost::regex_search(mentString, matchResult, *regex_str_80) ||
					boost::regex_search(modifiedMentString, matchResult, *regex_str_80) ||
					boost::regex_search(fullyModifiedMentString, matchResult, *regex_str_80)))
				{
					//SessionLogger::dbg("PatternMatcher") << "80 match:" << mentString << "\n";
					best_confidence = std::max(0.8F, best_confidence);
				} else if (regex_str_40 &&
					(boost::regex_search(mentString, matchResult, *regex_str_40) ||
					boost::regex_search(modifiedMentString, matchResult, *regex_str_40) ||
					boost::regex_search(fullyModifiedMentString, matchResult, *regex_str_40)))
				{
					//SessionLogger::dbg("PatternMatcher") << "40 match:" << mentString << "\n";
					best_confidence = std::max(0.4F, best_confidence);
				} else if (regex_str_10 &&
					(boost::regex_search(mentString, matchResult, *regex_str_10) ||
					boost::regex_search(modifiedMentString, matchResult, *regex_str_10) ||
					boost::regex_search(fullyModifiedMentString, matchResult, *regex_str_10)))
				{
					//SessionLogger::dbg("PatternMatcher") << "10 match:" << mentString << "\n";
					best_confidence = std::max(0.1F, best_confidence);
				} else if (regex_str_00 &&
					(boost::regex_search(mentString, matchResult, *regex_str_00) ||
					boost::regex_search(modifiedMentString, matchResult, *regex_str_00) ||
					boost::regex_search(fullyModifiedMentString, matchResult, *regex_str_00)))
				{
					//SessionLogger::dbg("PatternMatcher") << "00 match:" << mentString << "\n";
					if (regex_str_100 == 0) {
						best_confidence = 1.0f; // not its fault it didn't get a 100!
						break; // can't do any better!
					} else best_confidence = std::max(0.05F, best_confidence);
				} 
			}
		if (best_confidence > 0) {
			setEntityLabelConfidence(label, entid, best_confidence);
		}
	}
	delete regex_str_100;
	delete regex_str_80;
	delete regex_str_40;
	delete regex_str_10;
	delete regex_str_00;
}

void PatternMatcher::setEntityLabelConfidence(const Symbol& label, int entity_id, float confidence) {
	// confidence is currently NOT used here
	PatternFeatureSet_ptr sfs = boost::make_shared<PatternFeatureSet>();
	sfs->addFeature(boost::make_shared<GenericPFeature>(Pattern_ptr(), -1, getActiveLanguageVariant()));
	getDocumentInfo()->entityLabelFeatureMap[label][entity_id] = sfs;
	//SessionLogger::dbg("PatternMatcher") << "Added a pattern feature set for entity label " << label << " and entity id " << entity_id << "\n";
}

void PatternMatcher::clearEntityLabels(){
	for (PatternMatchDocInfoMap::iterator it = _docInfos.begin(); it != _docInfos.end(); ++it) {
		(*it).second->entityLabelFeatureMap.clear();
	}
}

void PatternMatcher::printEntityLabels() {
	SessionLogger::dbg("PatternMatcher") << "Entity labels: ";
	if (getPatternSet()->getNEntityLabelPatterns() == 0) {
		SessionLogger::dbg("PatternMatcher") << "(Document has no entity label patterns to label.)\n";
		return;
	}
	for (size_t i = 0; i < getPatternSet()->getNEntityLabelPatterns(); ++i) {
		EntityLabelPattern_ptr labelPattern = getPatternSet()->getNthEntityLabelPattern(i);
		SessionLogger::dbg("PatternMatcher") << labelPattern->getLabel() << " ";
	}
	SessionLogger::dbg("PatternMatcher") << "\n";
	const EntitySet *entitySet = getDocTheory()->getEntitySet();
	for (int i = 0; i < entitySet->getNEntities(); i++) {
		bool found_match = false;
		std::wstringstream wss;
		for (size_t j = 0; j < getPatternSet()->getNEntityLabelPatterns(); ++j) {
			EntityLabelPattern::EntityLabelFeatureMap& map = getDocumentInfo()->entityLabelFeatureMap[getPatternSet()->getNthEntityLabelPattern(j)->getLabel()];
			PatternFeatureSet_ptr featureSet = map[i];
			if (featureSet != 0) {
				wss << "matched ";
				found_match = true;
			} else wss << "unmatched ";
		}
		if (found_match) {
			SessionLogger::dbg("PatternMatcher") << "Entity " << i << ": " << wss.str() << "\n";
		}
	}

}

void PatternMatcher::printSlotScores() {
	typedef Symbol::HashMap<const AbstractSlot*>::const_iterator SlotIter;
	std::stringstream ss;
	ss << "SENT_NO NODE EDGE FULL\n";
	int n_sentences = getDocTheory()->getNSentences();
	for (int cur_sent = 0; cur_sent != n_sentences; cur_sent++) {
		ss << cur_sent << " ";
		for (SlotIter it=_slots.begin(); it!=_slots.end(); ++it) {
			const AbstractSlot* slot = (*it).second;			
			ss << _slotSentenceMatchScores[slot][AbstractSlot::NODE][cur_sent] << " ";
			ss << _slotSentenceMatchScores[slot][AbstractSlot::EDGE][cur_sent] << " ";
			ss << _slotSentenceMatchScores[slot][AbstractSlot::FULL][cur_sent] << " ";
		}
		ss << "\n";
	}
	SessionLogger::dbg("PatternMatcher") << ss.str();
}

PatternFeatureSet_ptr PatternMatcher::getEntityLabelMatch(const Symbol& label, int entity_id) const {
	// First look up the label.
	EntityLabelPattern::EntityLabeling::const_iterator it1 = getDocumentInfo()->entityLabelFeatureMap.find(label);
	if (it1 == getDocumentInfo()->entityLabelFeatureMap.end()) return PatternFeatureSet_ptr(); // label not found at all.
	const EntityLabelPattern::EntityLabelFeatureMap &labels = (*it1).second;
	// Then look up the entity (by id)
	EntityLabelPattern::EntityLabelFeatureMap::const_iterator it2 = labels.find(entity_id);
	if (it2==labels.end()) return PatternFeatureSet_ptr(); // this entity does not have the label.
	return (*it2).second;
}

void PatternMatcher::runDocumentPatterns(UTF8OutputStream *debug) {
	size_t n_doc_patterns = _patternSet->getNDocPatterns();
	int n_sentences = getDocTheory()->getNSentences();

	for (size_t i = 0; i < n_doc_patterns; i++) {
		Pattern_ptr pattern = _patternSet->getNthDocPattern(i);
		Symbol id = pattern->getID();
		if (SentenceMatchingPattern_ptr sentPattern = boost::dynamic_pointer_cast<SentenceMatchingPattern>(pattern)) {
			for (int i = 0; i < n_sentences; i++) {
				SentenceTheory *sTheory = getDocTheory()->getSentenceTheory(i);
				if (sentPattern->matchesSentence(shared_from_this(), sTheory, debug)) {
					getDocumentInfo()->matchedDocPatterns.insert(pattern->getID());
					break;
				}
			}
		} else if (DocumentMatchingPattern_ptr docPattern = boost::dynamic_pointer_cast<DocumentMatchingPattern>(pattern)) {
			if (docPattern->matchesDocument(shared_from_this(), debug))
				getDocumentInfo()->matchedDocPatterns.insert(pattern->getID());
		} else {
			throw UnexpectedInputException("PatternMatcher::runDocumentPatterns",
				"Document-level patterns must be SentenceMatchingPatterns or DocumentMatchingPatterns");
		}
		if (ParamReader::isParamTrue("verbose")) {
			SessionLogger::info("doc_patt_1") << "doclevel pattern " << i << " = " << matchesDocPattern(pattern->getID()) << "\n";
		}
	}
	runDocumentPseudoPatterns();
}

// Private symbols
namespace {
	Symbol _inActivityDateRangeSym = Symbol(L"IN_AD_RANGE");
	Symbol _outOfActivityDateRangeSym = Symbol(L"OUT_OF_AD_RANGE");
	Symbol _activityDateNotInCorpusSym = Symbol(L"AD_NOT_IN_CORPUS");
	Symbol _activityDateUndefinedSym = Symbol(L"AD_UNDEFINED");
}
void PatternMatcher::runDocumentPseudoPatterns() {
	if (_activityDate) {
		if (docDateWithinActivityRange())
			getDocumentInfo()->matchedDocPatterns.insert(_inActivityDateRangeSym);
		else {
			if (_activityDate->isActivityDateRestrictionInCorpusRange())
				getDocumentInfo()->matchedDocPatterns.insert(_outOfActivityDateRangeSym);
			else 
				getDocumentInfo()->matchedDocPatterns.insert(_activityDateNotInCorpusSym);
		}
	} else
		getDocumentInfo()->matchedDocPatterns.insert(_activityDateUndefinedSym);
}


bool PatternMatcher::matchesDocPattern(const Symbol& docPatternId) const {
	return (getDocumentInfo()->matchedDocPatterns.find(docPatternId) != getDocumentInfo()->matchedDocPatterns.end());
}

bool PatternMatcher::docDateWithinActivityRange(int extra_months) const {
	if (_activityDate == 0)
		return false;

	if (getDocTheory()->getNSentences() == 0)
		return false;

	std::wstring documentDate = TimexUtils::extractDateFromDocId(getDocTheory()->getDocument()->getName().to_string());
	const wchar_t *startDate = _activityDate->getNormalizedDateStart();
	wchar_t endDate[20]; //hard-coded array (OK because dates should always be 8 characters)
	_activityDate->getNormalizedDateEndPlusMonths(endDate, extra_months);
	
	return (wcscmp(startDate, documentDate.c_str()) <= 0 &&
			wcscmp(documentDate.c_str(), endDate) <= 0);
}

std::vector<LanguageVariant_ptr> PatternMatcher::getAlignedLanguageVariants (const LanguageVariant_ptr& restriction) const {
	return _docSet->getCompatibleLanguageVariants(_activeLanguageVariant, restriction);
}

std::vector<PatternFeatureSet_ptr> PatternMatcher::getDocumentSnippets() {
	std::vector<PatternFeatureSet_ptr> allMatches;

	for (size_t i=0; i<_patternSet->getNTopLevelPatterns(); ++i) {
		Pattern_ptr pattern = _patternSet->getNthTopLevelPattern(i);
		if (DocumentMatchingPattern_ptr docMatchPattern = boost::dynamic_pointer_cast<DocumentMatchingPattern>(pattern)) {
			std::vector<PatternFeatureSet_ptr> patMatches = docMatchPattern->multiMatchesDocument(shared_from_this());
			allMatches.insert(allMatches.end(), patMatches.begin(), patMatches.end());
		}
	}

	// now that we know we're keeping all these, go to the trouble of finding snippet spans
	BOOST_FOREACH(PatternFeatureSet_ptr sfs, allMatches) {
		sfs->setCoverage(shared_from_this());
	}

	if (!ParamReader::isParamTrue("dont_combine_snippets")) combineSnippets(allMatches);

	return allMatches;
}

std::vector<PatternFeatureSet_ptr> PatternMatcher::getSentenceSnippets(SentenceTheory* sTheory, UTF8OutputStream *out,
																	   bool force_multimatches /*=false*/)
{	
	std::vector<PatternFeatureSet_ptr> allMatches;

	int sent_no = sTheory->getTokenSequence()->getSentenceNumber();
	for (size_t i=0; i<_patternSet->getNTopLevelPatterns(); ++i) {
		//if (sent_no != 20) { continue; }
		//if (i != 2) { continue; }
		Pattern_ptr pattern = _patternSet->getNthTopLevelPattern(i);
		if (SentenceMatchingPattern_ptr sentMatchPattern = boost::dynamic_pointer_cast<SentenceMatchingPattern>(pattern)) {
			// Just use matchesSentence for now so we can compare with the system - AHZ 6/17/2011 
			//std::vector<PatternFeatureSet_ptr> patMatches = sentMatchPattern->multiMatchesSentence(shared_from_this(), sTheory, out);
			
			std::vector<PatternFeatureSet_ptr> patMatches;

			// Extraction patterns use multiMatchesSentence
			if (boost::dynamic_pointer_cast<ExtractionPattern>(pattern) || force_multimatches) {
				patMatches = sentMatchPattern->multiMatchesSentence(shared_from_this(), sTheory, out);
			} else {
				PatternFeatureSet_ptr result = sentMatchPattern->matchesSentence(shared_from_this(), sTheory, out);
				if (result) {
					patMatches.push_back(result);
				}		
			}

			//Eliminate duplicates (TODO: make PatternFeatureSetCollection object to do this as we add feature sets)
			for (int x=0;x<(int)patMatches.size();x++) {
				for (int y=x+1;y<(int)patMatches.size();y++) {
					if (patMatches[x]->equals(patMatches[y])) {
						patMatches.erase(patMatches.begin()+y);
						y--;
					}
				}
			}

			SessionLogger::dbg("patt_to_sent_0") << "Pattern " << i << " (" << pattern->getDebugID() << ") applied to sentence " 
				<< sTheory->getTokenSequence()->getSentenceNumber() << ".\n";
			if (patMatches.size() > 0) {
				if (SessionLogger::dbg_or_msg_enabled("patt_to_sent_1")) {
					std::stringstream ss;
					ss << "Pattern " << i << " (" << pattern->getDebugID() << ") found " << patMatches.size() << " matches with scores (score groups): ";
					for (size_t x = 0; x < patMatches.size(); x++) {
						ss << patMatches[x]->getScore() << " (";
						for (size_t f = 0; f < patMatches[x]->getNFeatures(); f++) {
							if (patMatches[x]->getFeature(f)->getPattern() == 0) {
								ss << "N/A ";
							} else {
								ss << patMatches[x]->getFeature(f)->getPattern()->getScoreGroup() << " ";
							}
						}
						ss << ")";
					}
					SessionLogger::dbg("patt_to_sent_1") << ss.str();
				}
				allMatches.insert(allMatches.end(), patMatches.begin(), patMatches.end());
			}
		}
	}

	// now that we know we're keeping all these, go to the trouble of finding snippet spans
	BOOST_FOREACH(PatternFeatureSet_ptr sfs, allMatches) {
		sfs->setCoverage(shared_from_this());
	}

	if (!ParamReader::isParamTrue("dont_combine_snippets"))
		combineSnippets(allMatches, _patternSet->keepAllFeaturesWhenCombiningFeatureSets());	

	return allMatches;
}

void PatternMatcher::combineSnippets(std::vector<PatternFeatureSet_ptr> &featureSets, bool keep_all_features) {	
	if (_snippet_combination_method == DO_NOT_COMBINE_SNIPPETS) 
		return;

	size_t n_sets = featureSets.size();
	SessionLogger::dbg("BRANDY") << "PatternMatcher::combineSnippets about to merge " << n_sets << " different feature sets\n";

	// Make an N^2 pass through the feature sets, comparing each feature set
	// with every subsequent feature set.  If we identify two that should be
	// combined, then we merge them into one of the vector indices, and set
	// the other vector index to NULL.  Once we are done, we make a final pass
	// to eliminate all the NULL values.
	for (size_t snip1 = 0; snip1 < n_sets; snip1++) {		
		if (!featureSets[snip1]) {
			SessionLogger::dbg("BRANDY") << snip1 << "(skipping - snip1 has been merged and set to 0)\n";
			continue; // snip1 has been merged; move on to the next feature set.
		}
		for (size_t snip2 = snip1+1; snip2 < n_sets; snip2++) {
			if (featureSets[snip2] == 0) {
				SessionLogger::dbg("BRANDY") << snip1 << " and " << snip2 << " (skipping - snip2 has been merged and set to 0)\n";
				continue; // snip2 has been merged; move on to the next feature set.
			}

			// If the sentence coverage doesn't match, then they should never be merged.
			if (featureSets[snip1]->getStartSentence() != featureSets[snip2]->getStartSentence() ||
				featureSets[snip1]->getEndSentence() != featureSets[snip2]->getEndSentence()) {
				SessionLogger::dbg("BRANDY") << snip1 << " and " << snip2 << " (skipping - different sentence coverage)\n";
				continue;
			}

			bool combine = false;

			// for unstructured queries, just look at coverage
			if (_snippet_combination_method == COMBINE_SNIPPETS_BY_COVERAGE) {
				int snip1_start = featureSets[snip1]->getStartToken();
				int snip1_end = featureSets[snip1]->getEndToken();
				int snip2_start = featureSets[snip2]->getStartToken();
				int snip2_end = featureSets[snip2]->getEndToken();

				// length will only make sense for single-sentence snippets, beware
				int length1 = snip1_end - snip1_start;
				int length2 = snip2_end - snip2_start;
				if (snip1_end == snip2_end && snip1_start == snip2_start) {
					SessionLogger::dbg("BRANDY") << snip1 << " and " << snip2 << " have identical token spans\n";
					combine = true;
				} else if (featureSets[snip1]->getStartSentence() == featureSets[snip1]->getEndSentence() &&
					snip1_start <= snip2_start && snip1_end >= snip2_end) {
						// snip1 contains snip2
						if ((length1 - length2) < 0.2 * length1) {
							SessionLogger::dbg("BRANDY") << snip1 << " and " << snip2 << " snip1 contains snip2\n";
							combine = true;
						}
				} else if (featureSets[snip1]->getStartSentence() == featureSets[snip1]->getEndSentence() &&
					snip1_start >= snip2_start && snip1_end <= snip2_end) {
						// snip2 contains snip1
						if ((length2 - length1) < 0.2 * length2) {
							SessionLogger::dbg("BRANDY") << snip1 << " and " << snip2 << " snip2 contains snip1\n";
							combine = true;
						}
				} 
			} else {
				// For structured queries, check that answser features match.	
				SessionLogger::dbg("BRANDY") << snip1 << " vs " << snip2;
				if (featureSets[snip1]->hasSameAnswerFeatures(featureSets[snip2])) {
					SessionLogger::dbg("BRANDY") << snip1 << " and " << snip2 << " were deemed identical by hasSameAnswerFeatures\n";
					combine = true;
				}
			}
			if (combine) {
				// The higher-scoring snippet takes the other snippet's features only
				// if keep_all_features is true.
				float snip1_score = featureSets[snip1]->getScore();
				float snip2_score = featureSets[snip2]->getScore();
				if (snip1_score >= snip2_score) {
					if (keep_all_features) 
						featureSets[snip1]->addFeatures(featureSets[snip2]);
					SessionLogger::dbg("BRANDY") << "Keeping " << snip1 << ", deleting " << snip2;
					featureSets[snip2] = PatternFeatureSet_ptr(); // NULL
				} else {
					if (keep_all_features)
						featureSets[snip2]->addFeatures(featureSets[snip1]);
					SessionLogger::dbg("BRANDY") << "Keeping " << snip2 << ", deleting " << snip1;
					featureSets[snip1] =  PatternFeatureSet_ptr(); // NULL
					break; // snip1 is NULL now; move on to the next snip1.
				}
			}
		}
	}

	// Erase any NULL feature set pointers from the vector.
	featureSets.erase(std::remove(featureSets.begin(), featureSets.end(), PatternFeatureSet_ptr()), 
	                  featureSets.end());
	//SessionLogger::dbg("BRANDY") << "PatternMatcher::combineSnippets went from " << n_sets << " to " << featureSets.size() << " feature sets.\n";
}

/***************
To initialize this I need:
  - A list of slots
  - A name dictionary.

We need...
  - A map from Slot labels to Slots, where a Slot provides:
    - 3 prop matchers (node/edge/full)
  - getBackoffRegexSlotText(), which can be implemented entirely in terms
    of a Slot.  Perhaps it should even be a method on a slot (not on a query)??
	It needs the slot's text and its sentence theory.
  - slotNamesNearSentence.. For this we need:
    - a name dictionary
	- the slot (so we can check what names it has in its sentence theory)

  _slotNameAware: set externally by PatternMatcher::setNameAware().  This controls
  whether we require that a name be within a window to consider prop-tree matches.
  It is set on a per-slot basis.  Used by AnswerFinder::analyzeSlots for four
  specific slots.

  _slotNames: initialized by PatternMatcher::findSlotNames().  Uses
  Slot::getSlotSentenceTheory().  Basically, looks at the sentence theory for
  each slot, goes through its mention set, and collects all the names as strings
  (well, actually as Symbols).

  _slotNamesInSentences: a mapping from <sentno, slotno> to sets of names (as
  Symbols).  Uses _fullQuery->getNameDictionary()??  Basically this is a list of
  all possibly relevant names in the sentence.  I think there are several steps
  we could skip here. :)  It looks in the first sentence in the PropForest.  Here
  are the types used for the name dictionary:
        //synonyms and its distance
        typedef std::map<std::wstring,double> NameSynonyms;
        //all synonyms for a name
        typedef std::map<std::wstring,NameSynonyms> NameDictionary;
  In particular, we check dict[slot_name][name_from_sentence], and count that as
  a match if it's nonzero (we ignore the distance).

  The name dictionary comes from FullQuery::_allEquivNames.  In English, this comes
  from loadEquivNames().  Each query constructs its own, though that seems a little
  wasteful.  In other languages, it's empty.


****************/

#include "Generic/PropTree/PropNodeMatch.h"
#include "Generic/PropTree/PropEdgeMatch.h"
#include "Generic/PropTree/PropFullMatch.h"
#include "Generic/PropTree/DocPropForest.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"

void PatternMatcher::fillSentencesNearSlotNames(DocPropForest::ptr docPropForest, const AbstractSlot* slot, int context_size) {
	_sentencesNearSlotNames[slot].clear();
	//SessionLogger::dbg("PatternMatcher") << L"findSlotNamesInSentences has names (";

	// Find all the names that occur in this slot.
	std::set<Symbol> slotNames;
	const SentenceTheory* slotST = slot->getSlotSentenceTheory();
	if (slotST) {
		const MentionSet* menSet = slotST->getMentionSet();
		if (menSet) {
			for ( int k=0; k < menSet->getNMentions(); k++ ) {
				const Mention* men=menSet->getMention(k);
				if (men->getMentionType() == Mention::NAME) {
					Symbol name(UnicodeUtil::normalizeTextString(men->getAtomicHead()->toTextString()));
					slotNames.insert(name);
					//SessionLogger::dbg("PatternMatcher") << name << ", ";
				}
			}
		}
	}
	//SessionLogger::dbg("PatternMatcher") << ")\n";

	int numSentences = getDocTheory()->getNSentences();	
	int n_sentences = getDocTheory()->getNSentences();
	std::map<int, std::set<Symbol> > namesInSentence; // sentno -> names
	for (int sentno = 0; sentno < numSentences; sentno++) {
		PropNode::PropNodes_ptr sentence_root_nodes = (*docPropForest)[sentno];
		PropNode::PropNodes sentence_nodes;
		PropNode::enumAllNodes(*sentence_root_nodes, sentence_nodes);	
		std::wstring sentText = UnicodeUtil::normalizeTextString(getDocTheory()->getSentenceTheory(sentno)->getTokenSequence()->toString());
		BOOST_FOREACH(PropNode_ptr node, sentence_nodes) {
			PropNode::WeightedPredicates preds = node->getPredicates();
			BOOST_FOREACH(PropNode::WeightedPredicate pred, preds) {
				BOOST_FOREACH(Symbol slotName, slotNames) {		
					bool found = false;
					if (pred.first.pred() == slotName) {
						found = true;
						namesInSentence[sentno].insert(slotName);
					} else if (_nameDictionary) {
						std::map<std::wstring,NameSynonyms>::const_iterator it;
						if ((it=_nameDictionary->find(slotName.to_string())) != _nameDictionary->end()) {
							NameSynonyms synonyms = it->second;
							if (synonyms.find(pred.first.pred().to_string()) != synonyms.end()) {
								found = true;
								namesInSentence[sentno].insert(slotName);
							}
						}
					}
					if (!found) {
						// check for exact match despite name boundaries
						if (sentText.find(slotName.to_string()) != std::wstring::npos) {
							namesInSentence[sentno].insert(slotName);
						}
					}
				}
			}
		}
		//SessionLogger::dbg("PatternMatcher") << sentno << ": ";
		//BOOST_FOREACH(Symbol s, namesInSentence[sentno]) {
		//	SessionLogger::dbg("PatternMatcher") << s << ", ";
		//}
		//SessionLogger::dbg("PatternMatcher") << "\n";
	}
			
	// Finally, determine which sentences have all names in the 
	// desired context window.
	for (int sentno = 0; sentno < numSentences; ++sentno) {
		bool found_all_names = true;
		for (std::set<Symbol>::const_iterator symIter = slotNames.begin(); symIter != slotNames.end(); ++symIter) {
			Symbol slot_name = (*symIter);	
			int start = std::max(sentno-context_size, 0);
			int end = std::min(sentno+context_size+1, numSentences);
			bool found_name = false;
			for (int k=start; k<end; ++k) {
				//SessionLogger::dbg("PatternMatcher") << "sentno " << sentno << " looking at " << k << "\n";
				const std::set<Symbol> &namesInThisSentence = namesInSentence[k];
				if (namesInThisSentence.find(slot_name) != namesInThisSentence.end()) {
					found_name = true;
					//SessionLogger::dbg("PatternMatcher") << "found_name = true\n";
					break;
				}
			}
			if (!found_name) {
				found_all_names = false;
				break;
			}
		}
		if (found_all_names) {
			_sentencesNearSlotNames[slot].insert(sentno);
		}
	}
}

/** Initialize the private member variables _sentenceSlotMatchScore and 
  * _sentenceBestMatchScore for the given slot.  
  *
  * @param sentencesNearSlotNames The set of sentence indices such that
  *        all of the names appropriate for the slot are contained within
  *        a 5-sentence window around that index.
  */
void PatternMatcher::fillSlotScores(const AbstractSlot* slot) {
	// Get the PropTree forest for the document.
	DocPropForest::ptr docPropForest = getDocTheory()->getPropForest();

	// Find which sentences are near names from the slot
	fillSentencesNearSlotNames(docPropForest, slot);

	Symbol slotLabel = Symbol(L"NONE");
	if (const SimpleSlot* simpleSlot = dynamic_cast<const SimpleSlot*>(slot)) {
		slotLabel = simpleSlot->getLabel();
	}

	// Get a regex that can be used to check if we have an exact match for a slot.
	boost::wregex exact_match_regex(slot->getBackoffRegexText());

	if (SessionLogger::dbg_or_msg_enabled("PatternMatcher")) {
		std::wstringstream wss;
		wss << L"\nNode matcher:\n";
		slot->getMatcher(AbstractSlot::NODE)->print(wss);
		wss << L"\nEdge matcher:\n";
		slot->getMatcher(AbstractSlot::EDGE)->print(wss);
		wss << L"\nFull matcher:\n";
		slot->getMatcher(AbstractSlot::FULL)->print(wss);
		SessionLogger::dbg("PatternMatcher") << wss.str() << L"\n";
		//std::wcerr << wss.str() << L"\n";
	}

	// Clear our slot sentence matches
	BOOST_FOREACH(AbstractSlot::MatchType matchType, AbstractSlot::ALL_MATCH_TYPES) {
		_slotSentenceMatchScores[slot][matchType].clear();
		_slotSentenceMatchFeatures[slot][matchType].clear();
	}

	// Match each sentence against the slot, and record the scores.
	int n_sentences = getDocTheory()->getNSentences();
	for (int cur_sent = 0; cur_sent != n_sentences; cur_sent++) {
		std::wstring sentString = getDocTheory()->getSentenceTheory(cur_sent)->getPrimaryParse()->getRoot()->toTextString();
		bool is_exact_match = false;
		if (boost::regex_search(sentString, exact_match_regex)) {
			// We still want to run the prop-matching below, so we can get the node coverage as best as possible
			is_exact_match = true;		
		} 		
		if (!is_exact_match && (_slotsWithDisabledNameFilter.find(slot) == _slotsWithDisabledNameFilter.end()) &&
		            (_sentencesNearSlotNames[slot].find(cur_sent) == _sentencesNearSlotNames[slot].end()) ) 
		{
			// The slot's names were not all found near this sentence: set
			// all scores to zero.
			BOOST_FOREACH(AbstractSlot::MatchType matchType, AbstractSlot::ALL_MATCH_TYPES) {
				_slotSentenceMatchScores[slot][matchType].push_back(0.0f);
				_slotSentenceMatchFeatures[slot][matchType].push_back(PatternFeatureSet_ptr());
			}
			SessionLogger::dbg("PatternMatcher") << "NO_NAMES\n";
		} else {
			// All of the slot's names were found near this sentence: use
			// each of the slot's PropMatchers to compare the slot's PropTree 
			// with the sentence's PropTree.
			PropNode::PropNodes_ptr sentence_root_nodes = (*docPropForest)[cur_sent];
			PropNode::PropNodes sentence_nodes;
			PropNode::enumAllNodes(*sentence_root_nodes, sentence_nodes);
			boost::shared_ptr<PropNodeMatch> nodeMatcher = boost::dynamic_pointer_cast<PropNodeMatch>(slot->getMatcher(AbstractSlot::NODE));
			PatternFeatureSet_ptr nodePFS = boost::make_shared<PatternFeatureSet>();
			if (SessionLogger::dbg_or_msg_enabled("PatternMatcher")){
				BOOST_FOREACH(PropNode::PropNode_ptr node, sentence_nodes) {
					std::wostringstream stream;
					node->compactPrint(stream, true, false, false, 0);
					SessionLogger::dbg("PatternMatcher") << cur_sent << " " << stream.str() << "\n";
				}
			}
			BOOST_FOREACH(AbstractSlot::MatchType matchType, AbstractSlot::ALL_MATCH_TYPES) {				
				float score = slot->getMatcher(matchType)->compareToTarget(sentence_nodes, false);
				
				
				if (matchType == AbstractSlot::NODE) {
					if (score > 0.0f) {
						boost::shared_ptr<PropMatch> propMatch = slot->getMatcher(matchType);
						for (size_t i = 0; i < propMatch->getMatchCovered().size(); i++) {						
							if (propMatch->getMatchCovered().at(i)) {
								Symbol predicate = nodeMatcher->getNthPredicateSymbol(i);
								nodePFS->addFeature(boost::make_shared<CoveredPropNodePFeature>(matchType, propMatch->getMatchCovering().at(i), 
									propMatch->getMatchCovered().at(i), slotLabel, predicate, int(i), int(propMatch->getMatchCovered().size()),
									getActiveLanguageVariant()));
							}
						}
					} else if (is_exact_match) {
						boost::shared_ptr<PropMatch> propMatch = slot->getMatcher(matchType);
						CoveredPropNodePFeature::CPN_struct cpn_struct;
						cpn_struct.pnode_sent_no = cur_sent;
						cpn_struct.pnode_start_token = 0;
						cpn_struct.pnode_end_token = getDocTheory()->getSentenceTheory(cur_sent)->getTokenSequence()->getNTokens() - 1;
						cpn_struct.query_predicate = Symbol(L"exact_match");
						cpn_struct.document_predicate = Symbol(L"exact_match");
						cpn_struct.query_slot = slotLabel;
						cpn_struct.matcher_total = int(propMatch->getMatchCovered().size());
						for (size_t i = 0; i < propMatch->getMatchCovered().size(); i++) {	
							cpn_struct.matcher_index = static_cast<int>(i);
							nodePFS->addFeature(boost::make_shared<CoveredPropNodePFeature>(matchType, 1.05f, cpn_struct, getActiveLanguageVariant()));
						}
					}
				}
				if (is_exact_match)
					score = 1.05f;

				_slotSentenceMatchScores[slot][matchType].push_back(score);
			}

			// Use node coverage for all matchers, since other matchers don't store it right
			BOOST_FOREACH(AbstractSlot::MatchType matchType, AbstractSlot::ALL_MATCH_TYPES) {				
				_slotSentenceMatchFeatures[slot][matchType].push_back(nodePFS);
			}
			//SessionLogger::dbg("PatternMatcher") << "SCORE\n";
		}
	}

	// Determine the maximum match score for each match type.
	BOOST_FOREACH(AbstractSlot::MatchType matchType, AbstractSlot::ALL_MATCH_TYPES) {
		const std::vector<float> &scores = _slotSentenceMatchScores[slot][matchType];
		_slotBestMatchScores[slot][matchType] = *(std::max_element(scores.begin(), scores.end()));
	}
}

std::wstring PatternMatcher::getDocID() const {
	return getDocTheory()->getDocument()->getName().to_string();
}

bool PatternMatcher::isBlockedPropositionStatus(PropositionStatusAttribute prop_status) const {
	return getPatternSet()->isBlockedPropositionStatus(prop_status);
}

void PatternMatcher::regenerateProptreeScores(const AbstractSlot* slot) {
	if (!ParamReader::isParamTrue("topicality_only_for_selected_slot_types") || slot->requiresProptrees()) {
		fillSlotScores(slot);
	}

	// sadly, these can depend on the proptree scores, so we have to redo them
	for (PatternMatchDocInfoMap::iterator it = _docInfos.begin(); it != _docInfos.end(); ++it) {
		setActiveLanguageVariant((*it).first);
		getDocumentInfo()->matchedDocPatterns.clear();
		runDocumentPatterns();
	}
}

/* \brief for an element of an array return a smoothed optimistic version of its value.
	Optimistic smoothing is done by taking maximum of weighted neighbors within a window;
	currently following window-lengths are supported: 1 (no smoothing), 3, 5 and 7
	\param array array to get a smoothed value from
	\param index index of the element in the array
	\param filter_size window length
	\return smoothed value
*/
float PatternMatcher::getFilteredScore(const std::vector<float>& array, int index, int filter_size) {
	int size = (int)array.size();
	
	if (index >= size) {
		throw InternalInconsistencyException::arrayIndexException(
		"PatternMatcher::getFilteredScore()", size, index);
	} else if ( index < 0 ) {
		throw InternalInconsistencyException::arrayIndexException(
		"PatternMatcher::getFilteredScore()", 0, index); 
	}

	float COEFFS_1[]={1};
	float COEFFS_3[]={1, (float)0.5};
	float COEFFS_5[]={1, (float)0.8, (float)0.5};
	float COEFFS_7[]={1, (float)0.9, (float)0.75, (float)0.5};
	float COEFFS_9[]={1, (float)0.95, (float)0.82, (float)0.70, (float)0.5};
	float COEFFS_11[]={1, (float)0.98, (float)0.90, (float)0.82, (float)0.7, (float)0.5};
	float* smoothArray;
	switch ( filter_size ) {
	case 1: smoothArray = COEFFS_1; break;
	case 3: smoothArray = COEFFS_3; break;
	case 5: smoothArray = COEFFS_5; break;
	case 7: smoothArray = COEFFS_7; break;
	case 9: smoothArray = COEFFS_9; break;
	case 11: smoothArray = COEFFS_11; break;
	default: 
		SessionLogger::info("SERIF") << "smoothing filter of size " << filter_size << " is not implemented; reverse to unfiltered version\n";
		smoothArray = COEFFS_1;
		filter_size = 1;
	}
	float result=0.0;

	for ( int i=std::max(0,index-(filter_size-1)/2); i < std::min(size,index+(filter_size+1)/2); i++ ) {
		result = (float)std::max(result,	smoothArray[std::abs(index-i)]*array[i]);
	}
	return result;
}

float PatternMatcher::getFullScore(const AbstractSlot* slot, int sentno, int filter_size) {
	return getFilteredScore(_slotSentenceMatchScores[slot][AbstractSlot::FULL], sentno, filter_size);
}

float PatternMatcher::getEdgeScore(const AbstractSlot* slot, int sentno, int filter_size) {
	return getFilteredScore(_slotSentenceMatchScores[slot][AbstractSlot::EDGE], sentno, filter_size);
}

float PatternMatcher::getNodeScore(const AbstractSlot* slot, int sentno, int filter_size) {
	return getFilteredScore(_slotSentenceMatchScores[slot][AbstractSlot::NODE], sentno, filter_size);
}

PatternFeatureSet_ptr PatternMatcher::getFullFeatures(const AbstractSlot* slot, int sentno) {
	return _slotSentenceMatchFeatures[slot][AbstractSlot::FULL][sentno];
}

PatternFeatureSet_ptr PatternMatcher::getEdgeFeatures(const AbstractSlot* slot, int sentno) {
	return _slotSentenceMatchFeatures[slot][AbstractSlot::EDGE][sentno];
}

PatternFeatureSet_ptr PatternMatcher::getNodeFeatures(const AbstractSlot* slot, int sentno) {
	return _slotSentenceMatchFeatures[slot][AbstractSlot::NODE][sentno];
}

float PatternMatcher::getFullScore(const AbstractSlot* slot, int sentno, const Proposition* prop) {
	return getMatchScore(slot, sentno, prop, AbstractSlot::FULL);
}

float PatternMatcher::getEdgeScore(const AbstractSlot* slot, int sentno, const Proposition* prop) {
	return getMatchScore(slot, sentno, prop, AbstractSlot::EDGE);
}

float PatternMatcher::getFullScore(const AbstractSlot* slot, int sentno, const Mention *mention) {
	return getMatchScore(slot, sentno, mention, AbstractSlot::FULL);
}

float PatternMatcher::getEdgeScore(const AbstractSlot* slot, int sentno, const Mention *mention) {
	return getMatchScore(slot, sentno, mention, AbstractSlot::EDGE);
}

float PatternMatcher::getMatchScore(const AbstractSlot* slot, int sentno, const Proposition* prop, AbstractSlot::MatchType matchType) {
	// If the sentence didn't match, then give up now.
	if (_slotSentenceMatchScores[slot][matchType][sentno] == 0.0f ) { return 0.0f; }
	
	// Construct our prop forest from our proposition
	PropForestFactory propForestFactory = PropForestFactory(getDocTheory(), prop, sentno);
	PropNode::PropNodes proposition_nodes = propForestFactory.getAllNodes();
	_expander.expand(proposition_nodes);
	return slot->getMatcher(matchType)->compareToTarget(proposition_nodes, false);
}

float PatternMatcher::getMatchScore(const AbstractSlot* slot, int sentno, const Mention *mention, AbstractSlot::MatchType matchType) {
	// If the sentence didn't match, then give up now.
	if (_slotSentenceMatchScores[slot][matchType][sentno] == 0.0f ) { return 0.0f; }
		
	// Construct our prop forest from our mention
	PropForestFactory propForestFactory = PropForestFactory(getDocTheory(), mention);
	PropNode::PropNodes mention_nodes = propForestFactory.getAllNodes();
	_expander.expand(mention_nodes);
	return slot->getMatcher(matchType)->compareToTarget(mention_nodes, false);
}

const AbstractSlot* PatternMatcher::getSlot(Symbol name) {
	if (_slots[name]) {
		return _slots[name];
	} else {
		std::wstring label = name.to_string();
		if (label.find(L"1") != label.npos && label.find(L"2") == label.npos) {
			label = L"SLOT1";
		} else if (label.find(L"2") != label.npos && label.find(L"1") == label.npos) {
			label = L"SLOT2";
		}
		// NOTE: This could be 0!
		return _slots[label];
	}
}

// node 1.1724 -0.1368
// edge 1.1951 -0.0321
// full 0.9482  0.1668
float PatternMatcher::getDecisionTreeScore(const AbstractSlot* slot, int sentNumber) {

	// Figure out our full match score on the current sentence
	float full_score = getFullScore(slot, sentNumber);

	// Figure out our edge match score on the current sentence
	float edge_score = getEdgeScore(slot, sentNumber);

	// Figure out our node score on the current sentence
	float node_score = getNodeScore(slot, sentNumber);

	// Get our document level scores
	bool doc_great_match = (_slotBestMatchScores[slot][AbstractSlot::FULL] > .55f || 
	                        _slotBestMatchScores[slot][AbstractSlot::NODE] > .80f);
	bool doc_good_match = _slotBestMatchScores[slot][AbstractSlot::NODE] > .60f;

	float best_nearby_full_score = 0.0f;
	float best_nearby_node_score = 0.0f;
	for (int i = sentNumber - 2; i <= sentNumber + 2; i++) {
		if (i < 0 || i >= getDocTheory()->getNSentences())
			continue;
		best_nearby_full_score = std::max(best_nearby_full_score, getFullScore(slot, sentNumber));
		best_nearby_node_score = std::max(best_nearby_node_score, getNodeScore(slot, sentNumber));
	}
	bool nearby_excellent_match = best_nearby_full_score > 0.55f || best_nearby_node_score > .80f;

	if (full_score > .93f) { return .95f; }
	if (full_score > .90f) { return .90f; }
	if (node_score > .90f) { return .89f; }
	if (full_score > .80f) { return .88f; }
	if (node_score > .80f) { return .87f; }
	if (full_score > .55f) { return .80f; }
	if (node_score > .70f) { return .75f; }	
	
	// Edges will only fire when a node-edge-node from the query is there. This is usually a pretty decent
    // combination, especially if a bunch of the other words are around. The benefit of this combo is that
    // it also excludes the single-word queries from matching with a synonym, which would get an OK subtree score.
	if (edge_score > .00f && node_score > .50f) { return 0.69f; }
	if (full_score > .30f && node_score > .60f) { return 0.65f; }
	if (node_score > .60f && doc_great_match) { return 0.64f; }
	if (full_score > .30f && doc_great_match) { return 0.64f; }
	if (edge_score > .20f) { return 0.60f; }
	if (node_score > .60f) { return 0.60f; }	
	if (node_score > .49f && nearby_excellent_match) { return 0.55f; }	
	if (node_score > .49f && doc_great_match) { return 0.54f; }
	if (node_score > .39f && nearby_excellent_match) { return 0.53f; }	
	if (node_score > .39f && doc_great_match) { return 0.52f; }
	if (node_score > .29f && nearby_excellent_match) { return 0.51f; }	
	if (node_score > .29f && doc_great_match) { return 0.50f; }
	if (node_score > .39f && doc_good_match) { return 0.45f; }
	
	if (node_score > .39f) { return 0.35f; }
	if (full_score > .20f) { return 0.30f; }
	
	if (edge_score > .00f && nearby_excellent_match) { return 0.29f; }
	if (edge_score > .00f && doc_great_match) { return 0.28f; }
	if (node_score > .29f && nearby_excellent_match) { return 0.27f; }	
	if (node_score > .29f && doc_great_match) { return 0.26f; }
	if (node_score > .29f) { return 0.20f; }
	if (node_score > .19f) { return 0.18f; }
	if (node_score > .10f) { return 0.14f; }

	return 0.0f;
}

/*Entity* PatternMatcher::getEntityByMention(MentionUID uid) {
	if (!getDocumentInfo()->mentionEntityMap) {
		if (!getDocTheory()->getEntitySet()) return 0;
		getDocumentInfo()->mentionEntityMap = getDocTheory()->getEntitySet()->getMentionEntityMap();
	}
	return (*getDocumentInfo()->mentionEntityMap)[uid];
}*/

boost::optional<boost::gregorian::date> PatternMatcher::getDocumentDate() {
	return _documentDate;
}

// call this function to label pattern-driven entities (as opposed to, e.g., AGENT1)
void PatternMatcher::labelPatternDrivenEntities() {
	for (size_t i = 0; i < getPatternSet()->getNEntityLabelPatterns(); ++i) {		
		EntityLabelPattern_ptr labelPattern = getPatternSet()->getNthEntityLabelPattern(i);
		if (labelPattern->hasPattern()) {
			labelPattern->labelEntities(shared_from_this(), getDocumentInfo()->entityLabelFeatureMap);
		} 
	}
}
