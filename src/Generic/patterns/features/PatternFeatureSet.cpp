// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include <boost/foreach.hpp>

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/InternalInconsistencyException.h"
#include "common/SessionLogger.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/features/CoveredPropNodePFeature.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/features/DocumentPFeature.h"
#include "Generic/patterns/features/EventMentionPFeature.h"
#include "Generic/patterns/features/ExactMatchPFeature.h"
#include "Generic/patterns/features/GenericPFeature.h"
#include "Generic/patterns/features/MentionPFeature.h"
#include "Generic/patterns/features/MutualAcquaintPFeature.h"
#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/patterns/features/PropPFeature.h"
#include "Generic/patterns/features/PseudoPatternFeature.h"
#include "Generic/patterns/features/QuotePFeature.h"
#include "Generic/patterns/features/RelatedWordPFeature.h"
#include "Generic/patterns/features/RelMentionPFeature.h"
#include "Generic/patterns/features/TokenSpanPFeature.h"
#include "Generic/patterns/features/TopicPFeature.h"
#include "Generic/patterns/features/TopLevelPFeature.h"
#include "Generic/patterns/features/ValueMentionPFeature.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/state/XMLElement.h"

namespace { // Private Symbols.
	Symbol ANSWER(L"ANSWER");
}

PatternFeatureSet::PatternFeatureSet():
_start_sentence(-1), _end_sentence(-1), _start_token(-1), _end_token(-1), _score(Pattern::UNSPECIFIED_SCORE)
{}

PatternFeature_ptr PatternFeatureSet::getFeature(size_t n) const {
	if (n < _features.size())
		return _features[n]; 
	else
		throw InternalInconsistencyException::arrayIndexException(
			"PatternFeatureSet::getFeature()", _features.size(), n);
}

bool PatternFeatureSet::hasFeature(PatternFeature_ptr feature) {
	BOOST_FOREACH (PatternFeature_ptr f,_features) {
		if (f->equals(feature))
			return true;
	}
	return false;
}

void PatternFeatureSet::addFeature(PatternFeature_ptr feature) {
	if (!hasFeature(feature))
		_features.push_back(feature);
}

void PatternFeatureSet::addFeatures(boost::shared_ptr<PatternFeatureSet> source) {
	if (!source->_features.empty()) {
		//_features.reserve(_features.size()+source->_features.size());
		//_features.insert(_features.end(), source->_features.begin(), source->_features.end());
		for (size_t i=0;i<source->getNFeatures();i++) {
			addFeature(source->getFeature(i));
		}
	}
}

void PatternFeatureSet::removeFeature(size_t n) {
	if (n < _features.size()) {
		_features.erase(_features.begin()+n);
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"PatternFeatureSet::removeFeature()", _features.size(), n);
	}
}

void PatternFeatureSet::replaceFeature(size_t n, PatternFeature_ptr feature) {
	if (n < _features.size()) {
		if (!hasFeature(feature))
			_features[n] = feature;
		else
			_features.erase(_features.begin()+n);
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"PatternFeatureSet::replaceFeature()", _features.size(), n);
	}
}

void PatternFeatureSet::clear() {
	_features.clear();
	_start_sentence = _end_sentence = _start_token = _end_token = -1;
	_score = Pattern::UNSPECIFIED_SCORE;
}

bool PatternFeatureSet::equals(PatternFeatureSet_ptr other) {
	if (getNFeatures() == other->getNFeatures()) {
		for (size_t i=0;i<other->getNFeatures();i++) {
			if (!hasFeature(other->getFeature(i)))
				return false;
		}
		return true;
	} else {
		return false;
	}
}

void PatternFeatureSet::gatherAnswerFeatures(std::vector<MentionReturnPFeature_ptr>& returnFeatures) {
	for (size_t i = 0; i < _features.size(); i++) {
		if (MentionReturnPFeature_ptr returnFeature = boost::dynamic_pointer_cast<MentionReturnPFeature>(_features[i])) {
			if (returnFeature->getReturnLabel() == ANSWER)
				returnFeatures.push_back(returnFeature);
		}
	}
}

Symbol PatternFeatureSet::getTopLevelPatternLabel() {	
	for (size_t f = 0; f < getNFeatures(); f++) {
		if (TopLevelPFeature_ptr tsf = boost::dynamic_pointer_cast<TopLevelPFeature>(getFeature(f)))
			return tsf->getPatternLabel();
	}
	return Symbol();
}

bool PatternFeatureSet::hasSameAnswerFeatures(PatternFeatureSet_ptr otherFS) {
	std::vector<MentionReturnPFeature_ptr> myAnswerFeatures;
	gatherAnswerFeatures(myAnswerFeatures);
	std::vector<MentionReturnPFeature_ptr> otherAnswerFeatures;
	otherFS->gatherAnswerFeatures(otherAnswerFeatures);
	//SessionLogger::dbg("BRANDY") << "I have " << myAnswerFeatures.size() << " answer features and other has " << otherAnswerFeatures.size() << " answer features.";
	if (myAnswerFeatures.size() != otherAnswerFeatures.size()) {
		//SessionLogger::dbg("BRANDY") << "DIFFERENT SIZES!";
		return false;
	}

	// Our vectors can contain duplicates.  We consider things identical if the sets of mention uids are equivalent.
	std::set<MentionUID> my_mention_uids;
	BOOST_FOREACH(MentionReturnPFeature_ptr my_srf, myAnswerFeatures) {
		my_mention_uids.insert(my_srf->getMention()->getUID());
	}
	std::set<MentionUID> other_mention_uids;
	BOOST_FOREACH(MentionReturnPFeature_ptr other_srf, otherAnswerFeatures) {
		other_mention_uids.insert(other_srf->getMention()->getUID());
	}
	return my_mention_uids == other_mention_uids;
}

const SynNode* PatternFeatureSet::getBestCoveringNode(LanguageAttribute language, SentenceTheory *st, int start, int end) {
	const SynNode* coveringNode = st->getPrimaryParse()->getRoot()->getCoveringNodeFromTokenSpan(start, end);

	// I don't like verb phrases not covered by S/SBAR... let's make sure that doesn't happen, 
	// at least in English, for now...
	Symbol tag = coveringNode->getHeadPreterm()->getTag();
	if (language == Language::ENGLISH && SynNode::isVerbTag(tag, language)) {
		while (coveringNode->getParent() != 0 && !SynNode::isSentenceLikeTag(coveringNode->getTag(), language))
			coveringNode = coveringNode->getParent();
	}
	return coveringNode;
}

void PatternFeatureSet::setCoverage(const PatternMatcher_ptr patternMatcher) {
	setCoverage(patternMatcher->getDocumentSet());
}

void PatternFeatureSet::setCoverage(const DocTheory * docTheory) {
	AlignedDocSet_ptr _docSet(boost::make_shared<AlignedDocSet>());
	_docSet->loadDocTheory(LanguageVariant::getLanguageVariant(), docTheory);
	setCoverage(_docSet);
}

void PatternFeatureSet::setCoverage(const AlignedDocSet_ptr& docSet) {
	// can this happen? I hope not
	if (getNFeatures() == 0)
		return;

	const DocTheory* docTheory;
	for (size_t i = 0; i < getNFeatures(); i++) {
		docTheory = docSet->getDocTheory(_features[i]->getLanguageVariant());
		_features[i]->setCoverage(docTheory);
	}

	_start_sentence = -1;
	_start_token = -1;
	_end_sentence = -1;
	_end_token = -1;
	int any_start_sentence = -1;
	for (size_t i = 0; i < getNFeatures(); i++) {
		if (_features[i]->getStartToken() == -1)
			continue;
		if (_features[i]->getSentenceNumber() > any_start_sentence)
			any_start_sentence = _features[i]->getSentenceNumber();;
		if (_start_token == -1) {
			_start_sentence = _features[i]->getSentenceNumber();
			_end_sentence = _features[i]->getSentenceNumber();
			_start_token = _features[i]->getStartToken();
			_end_token = _features[i]->getEndToken();
			continue;
		}
		if (_features[i]->getSentenceNumber() < _start_sentence || 
			(_features[i]->getSentenceNumber() == _start_sentence &&
			 _features[i]->getStartToken() < _start_token))
		{
			_start_sentence = _features[i]->getSentenceNumber();
			_start_token = _features[i]->getStartToken();
		}
		if (_features[i]->getSentenceNumber() > _end_sentence || 
			(_features[i]->getSentenceNumber() == _end_sentence &&
			 _features[i]->getEndToken() > _end_token))
		{
			_end_sentence = _features[i]->getSentenceNumber();
			_end_token = _features[i]->getEndToken();
		}
	}

	// we didn't find anything... take from some the latest sentence we found... does this ever happen successfully?
	if (_start_sentence == -1) {
		if (any_start_sentence == -1) {
			return;
		} else {
			_start_sentence = any_start_sentence;
			TokenSequence *tokens = docTheory->getSentenceTheory(_start_sentence)->getTokenSequence();
			if (tokens->getNTokens() != 0) {
				_start_sentence = any_start_sentence;
				_end_sentence = _start_sentence;
				_start_token = 0;
				_end_token = tokens->getNTokens() - 1;
			}
			return;
		}
	}

	SentenceTheory *startST = docTheory->getSentenceTheory(_start_sentence);
	
	// In case we are running patterns before we have a parse
	if (startST->getPrimaryParse() == 0) 
		return;
	
	// do we want covering nodes? I think we do.
	// in fact, I think for VPs we want to go up to the nearest S or SBAR
	if (_start_token != 0) {
		int etok = _end_token;
		if (_start_sentence != _end_sentence)
			etok = startST->getTokenSequence()->getNTokens() - 1;
		const SynNode *coveringNode = getBestCoveringNode(docTheory->getDocument()->getLanguage(), startST, _start_token, etok);
        _start_token = coveringNode->getStartToken();
	}
	
	SentenceTheory *endST = docTheory->getSentenceTheory(_end_sentence);
	int stok = _start_token;
	if (_start_sentence != _end_sentence)
		stok = 0;
	const SynNode *coveringNode = getBestCoveringNode(docTheory->getDocument()->getLanguage(), endST, stok, _end_token);
	_end_token = coveringNode->getEndToken();

	// Figure out our text
	// Do our start sentence (TokenSequence offsets are inclusive)
	std::wstringstream wss;
	int end_token;
	if (_start_sentence == _end_sentence) {
		end_token = _end_token;
	} else {
		end_token = docTheory->getSentenceTheory(_start_sentence)->getTokenSequence()->getNTokens() - 1;
	}
	wss << docTheory->getSentenceTheory(_start_sentence)->getTokenSequence()->toString(_start_token, end_token);
	if (_start_sentence != _end_sentence) {
		int sent_index = _start_sentence + 1;

		// Do any intervening sentences
		while (sent_index != _end_sentence) {
			end_token = docTheory->getSentenceTheory(sent_index)->getTokenSequence()->getNTokens() - 1;
			wss << docTheory->getSentenceTheory(sent_index)->getTokenSequence()->toString(0, end_token);
			sent_index += 1;
		}

		// Do the end sentence
		wss << docTheory->getSentenceTheory(_end_sentence)->getTokenSequence()->toString(0, _end_token);
	}
	_text = wss.str();
}

void PatternFeatureSet::setCoverage(int start_sentence, int end_sentence, int start_token, int end_token) {
	_start_sentence = start_sentence;
	_end_sentence = end_sentence;
	_start_token = start_token;
	_end_token = end_token;
}

int PatternFeatureSet::getBestScoreGroup() const {
	int best_score_group = Pattern::UNSPECIFIED_SCORE_GROUP;
	for (size_t f = 0; f < getNFeatures(); f++) {		
		if (_features[f]->getPattern() == 0)
			continue;
		int pattern_score_group = _features[f]->getPattern()->getScoreGroup();
		if (best_score_group == -1) 
			best_score_group = pattern_score_group;
		else if (pattern_score_group != -1)
			best_score_group = std::min(best_score_group, pattern_score_group);
	}
	return best_score_group;
}

void PatternFeatureSet::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	elem.setAttribute(X_start_sentence, _start_sentence);
	elem.setAttribute(X_end_sentence, _end_sentence);
	elem.setAttribute(X_start_token, _start_token);
	elem.setAttribute(X_end_token, _end_token);
	elem.setAttribute(X_score, _score);
	elem.setAttribute(X_score_group, getBestScoreGroup());
	elem.setAttribute(X_text, _text);

	BOOST_FOREACH(PatternFeature_ptr pf, _features) {
		XMLElement child;
		if (boost::dynamic_pointer_cast<CoveredPropNodePFeature>(pf))
			child = elem.addChild(X_CoveredPropNodePFeature);
		else if (boost::dynamic_pointer_cast<DateSpecReturnPFeature>(pf))
			child = elem.addChild(X_DateSpecReturnPFeature);
		else if (boost::dynamic_pointer_cast<DocumentDateReturnPFeature>(pf))
			child = elem.addChild(X_DocumentDateReturnPFeature);
		else if (boost::dynamic_pointer_cast<DocumentPFeature>(pf))
			child = elem.addChild(X_DocumentPFeature);
		else if (boost::dynamic_pointer_cast<EventMentionPFeature>(pf))
			child = elem.addChild(X_EventMentionPFeature);
		else if (boost::dynamic_pointer_cast<EventMentionReturnPFeature>(pf))
			child = elem.addChild(X_EventMentionReturnPFeature);
		else if (boost::dynamic_pointer_cast<ExactMatchPFeature>(pf))
			child = elem.addChild(X_ExactMatchPFeature);
		else if (boost::dynamic_pointer_cast<GenericPFeature>(pf))
			child = elem.addChild(X_GenericPFeature);
		else if (boost::dynamic_pointer_cast<GenericReturnPFeature>(pf))
			child = elem.addChild(X_GenericReturnPFeature);
		else if (boost::dynamic_pointer_cast<MentionPFeature>(pf))
			child = elem.addChild(X_MentionPFeature);
		else if (boost::dynamic_pointer_cast<MentionReturnPFeature>(pf))
			child = elem.addChild(X_MentionReturnPFeature);
		else if (boost::dynamic_pointer_cast<MutualAcquaintPFeature>(pf))
			child = elem.addChild(X_MutualAcquaintPFeature);
		else if (boost::dynamic_pointer_cast<PropositionReturnPFeature>(pf))
			child = elem.addChild(X_PropositionReturnPFeature);
		else if (boost::dynamic_pointer_cast<PropPFeature>(pf))
			child = elem.addChild(X_PropPFeature);
		else if (boost::dynamic_pointer_cast<QuotePFeature>(pf))
			child = elem.addChild(X_QuotePFeature);
		else if (boost::dynamic_pointer_cast<RelatedWordPFeature>(pf))
			child = elem.addChild(X_RelatedWordPFeature);
		else if (boost::dynamic_pointer_cast<RelMentionPFeature>(pf))
			child = elem.addChild(X_RelMentionPFeature);
		else if (boost::dynamic_pointer_cast<RelMentionReturnPFeature>(pf))
			child = elem.addChild(X_RelMentionReturnPFeature);
		else if (boost::dynamic_pointer_cast<TokenSpanPFeature>(pf))
			child = elem.addChild(X_TokenSpanPFeature);
		else if (boost::dynamic_pointer_cast<TokenSpanReturnPFeature>(pf))
			child = elem.addChild(X_TokenSpanReturnPFeature);
		else if (boost::dynamic_pointer_cast<TopicPFeature>(pf))
			child = elem.addChild(X_TopicPFeature);
		else if (boost::dynamic_pointer_cast<TopicReturnPFeature>(pf))
			child = elem.addChild(X_TopicReturnPFeature);
		else if (boost::dynamic_pointer_cast<TopLevelPFeature>(pf))
			child = elem.addChild(X_TopLevelPFeature);
		else if (boost::dynamic_pointer_cast<ValueMentionPFeature>(pf))
			child = elem.addChild(X_ValueMentionPFeature);
		else if (boost::dynamic_pointer_cast<ValueMentionReturnPFeature>(pf))
			child = elem.addChild(X_ValueMentionReturnPFeature);
		else 
			throw UnexpectedInputException("PatternFeatureSet::saveXML", "Unknown feature type");

		pf->saveXML(child, idMap);
	}
}

PatternFeatureSet::PatternFeatureSet(SerifXML::XMLElement elem, const SerifXML::XMLIdMap *idMap) {
	using namespace SerifXML;

	_start_sentence = elem.getAttribute<int>(X_start_sentence);
	_end_sentence = elem.getAttribute<int>(X_end_sentence);
	_start_token = elem.getAttribute<int>(X_start_token);
	_end_token = elem.getAttribute<int>(X_end_token);
	_score = elem.getAttribute<float>(X_score);

	std::vector<XMLElement> children = elem.getChildElements();
	BOOST_FOREACH(XMLElement child, children) {
		PatternFeature_ptr pf;

		if (child.hasTag(X_DocumentPFeature))
			pf = boost::make_shared<DocumentPFeature>(child, idMap);
		else if (child.hasTag(X_EventMentionPFeature)) 
			pf = boost::make_shared<EventMentionPFeature>(child, idMap);
		else if (child.hasTag(X_ExactMatchPFeature)) 
			pf = boost::make_shared<ExactMatchPFeature>(child, idMap);
		else if (child.hasTag(X_GenericPFeature)) 
			pf = boost::make_shared<GenericPFeature>(child, idMap);
		else if (child.hasTag(X_MentionPFeature)) 
			pf = boost::make_shared<MentionPFeature>(child, idMap);
		else if (child.hasTag(X_MutualAcquaintPFeature)) 
			pf = boost::make_shared<MutualAcquaintPFeature>(child, idMap);
		else if (child.hasTag(X_PropPFeature)) 
			pf = boost::make_shared<PropPFeature>(child, idMap);
		else if (child.hasTag(X_QuotePFeature)) 
			pf = boost::make_shared<QuotePFeature>(child, idMap);
		else if (child.hasTag(X_RelatedWordPFeature)) 
			pf = boost::make_shared<RelatedWordPFeature>(child, idMap);
		else if (child.hasTag(X_RelMentionPFeature))
			pf = boost::make_shared<RelMentionPFeature>(child, idMap);
		else if (child.hasTag(X_TokenSpanPFeature)) 
			pf = boost::make_shared<TokenSpanPFeature>(child, idMap);
		else if (child.hasTag(X_TopicPFeature)) 
			pf = boost::make_shared<TopicPFeature>(child, idMap);
		else if (child.hasTag(X_TopLevelPFeature)) 
			pf = boost::make_shared<TopLevelPFeature>(child, idMap);
		else if (child.hasTag(X_ValueMentionPFeature)) 
			pf = boost::make_shared<ValueMentionPFeature>(child, idMap);

		else if (child.hasTag(X_MentionReturnPFeature))
			pf = boost::make_shared<MentionReturnPFeature>(child, idMap);
		else if (child.hasTag(X_EventMentionReturnPFeature)) 
			pf = boost::make_shared<EventMentionReturnPFeature>(child, idMap);
		else if (child.hasTag(X_RelMentionReturnPFeature)) 
			pf = boost::make_shared<RelMentionReturnPFeature>(child, idMap);
		else if (child.hasTag(X_PropositionReturnPFeature)) 
			pf = boost::make_shared<PropositionReturnPFeature>(child, idMap);
		else if (child.hasTag(X_ValueMentionReturnPFeature)) 
			pf = boost::make_shared<ValueMentionReturnPFeature>(child, idMap);
		else if (child.hasTag(X_DateSpecReturnPFeature)) 
			pf = boost::make_shared<DateSpecReturnPFeature>(child, idMap);
		else if (child.hasTag(X_GenericReturnPFeature)) 
			pf = boost::make_shared<GenericReturnPFeature>(child, idMap);
		else if (child.hasTag(X_TopicReturnPFeature)) 
			pf = boost::make_shared<TopicReturnPFeature>(child, idMap);
		else if (child.hasTag(X_TokenSpanReturnPFeature)) 
			pf = boost::make_shared<TokenSpanReturnPFeature>(child, idMap);
		else 
			UnexpectedInputException("PatternFeatureSet::PatternFeatureSet", "unrecognized child tag of PatternFeatureSet");

		addFeature(pf);
	}
}
