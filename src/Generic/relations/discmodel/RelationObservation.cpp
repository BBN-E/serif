// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/DTRelationSet.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/relations/xx_RelationUtilities.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "Generic/relations/discmodel/DTRelationAltModel.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Argument.h"
#include "Generic/relations/discmodel/PropTreeLinks.h"
#include "Generic/wordnet/xx_WordNet.h"
#include "Generic/common/SymbolArray.h"
#include "Generic/common/version.h"


const Symbol RelationObservation::_className(L"relation");
wstring RelationObservation::TOO_LONG = L":TOO_LONG";
wstring RelationObservation::ADJACENT = L":ADJACENT";
wstring RelationObservation::CONFUSED = L":CONFUSED";

DTRelationAltModel RelationObservation::_altModels;
DTRelSentenceInfo RelationObservation::_1bestSentenceInfo(1);

RelationObservation::RelationObservation(DTFeatureTypeSet *features) : 
	_wcMent1(WordClusterClass::nullCluster()), 
	_wcMent2(WordClusterClass::nullCluster()), 
	_wcVerb(WordClusterClass::nullCluster()), 
	_wcVerbPred(WordClusterClass::nullCluster()),
	m1Name(0), m2Name(0),
	DTObservation(_className), 
	_npchunkFeatures(false)
{	
	for (int i = 0; i < 10; i++) {
		_propLinks[i] = _new RelationPropLink();
	}
	_validation_type = L"NONE";

	_n_predictions = 0;
	if (ParamReader::getRequiredTrueFalseParam("relation_finder_use_alt_models"))
		_altModels.initialize();

	examineFeatures(features);

	/*
	std::string buffer = ParamReader::getParam("relation_pattern_model");
	if (!buffer.empty()) {
	_patternModel = _new PatternMatcherModel(buffer.c_str());
	} else _patternModel = 0;

	_instance = _new PotentialRelationInstance();*/

}

RelationObservation::RelationObservation(const wchar_t* validation_str, 
										 DTFeatureTypeSet *features) : 
	_wcMent1(WordClusterClass::nullCluster()), 
	_wcMent2(WordClusterClass::nullCluster()), 
	_wcVerb(WordClusterClass::nullCluster()), 
	_wcVerbPred(WordClusterClass::nullCluster()),
	m1Name(0), m2Name(0),
	DTObservation(_className),
	_npchunkFeatures(false)
{
	for (int i = 0; i < 10; i++) {
		_propLinks[i] = _new RelationPropLink();
	}
	_validation_type = Symbol(validation_str);

	_n_predictions = 0;
	if (ParamReader::getRequiredTrueFalseParam("relation_finder_use_alt_models"))
		_altModels.initialize();

	examineFeatures(features);

	/*char patterns[500];
	std::string buffer = ParamReader::getParam("relation_pattern_model");
	if (!buffer.empty()) {
	_patternModel = _new PatternMatcherModel(buffer.c_str());
	} else _patternModel = 0;

	_instance = _new PotentialRelationInstance();*/

}
RelationObservation::RelationObservation(const char* validation_str, 
										 DTFeatureTypeSet *features) : 
	_wcMent1(WordClusterClass::nullCluster()), 
	_wcMent2(WordClusterClass::nullCluster()), 
	_wcVerb(WordClusterClass::nullCluster()), 
	_wcVerbPred(WordClusterClass::nullCluster()),
	m1Name(0), m2Name(0),
	DTObservation(_className),
	_npchunkFeatures(false)
{
	wchar_t validation_wstr[500];
	mbstowcs(validation_wstr, validation_str, 500);
	for (int i = 0; i < 10; i++) {
		_propLinks[i] = _new RelationPropLink();
	}
	_validation_type = Symbol(validation_wstr);

	_n_predictions = 0;
	if (ParamReader::getRequiredTrueFalseParam("relation_finder_use_alt_models"))
		_altModels.initialize();

	examineFeatures(features);

	/*
	std::string buffer = ParamReader::getParam("relation_pattern_model");
	if (!buffer.empty()) {
	_patternModel = _new PatternMatcherModel(buffer.c_str());
	} else _patternModel = 0;

	_instance = _new PotentialRelationInstance();*/

}

RelationObservation::~RelationObservation() {
	for (int i = 0; i < 10; i++) {
		delete _propLinks[i];
	}
	delete m1Name; m1Name = 0;
	delete m2Name; m2Name = 0;
}


DTObservation *RelationObservation::makeCopy() {
	RelationObservation *copy = _new RelationObservation(_validation_type.to_string());

	copy->resetForNewSentence(_sentenceInfo);
	copy->populate(_m1_id, _m2_id);
	return copy;
}


void RelationObservation::resetForNewSentence(DTRelSentenceInfo *sentenceInfo)
{
	_sentenceInfo = sentenceInfo;
	_m1_id = -1;
	_m2_id = -1;
	_m1 = 0;
	_m2 = 0;

	// moved to populate since m1Name and m2Name get changed once for every mention
	// pair, not once for every sentence. -AZ 8/21/08
	//delete m1Name;
	//delete m2Name;
	//m1Name = 0;
	//m2Name = 0;

	_sentenceInfo->parses[0]->getRoot()->getTerminalSymbols(_tokens, MAX_SENTENCE_TOKENS);
	_sentenceInfo->parses[0]->getRoot()->getPOSSymbols(_pos, MAX_SENTENCE_TOKENS);
	_sentenceInfo->propSets[0]->fillDefinitionsArray();
}

void RelationObservation::resetForNewSentence(EntitySet *eset, 
											  const Parse *parse,
											  const MentionSet *mentionSet, 
											  const ValueMentionSet *valueMentionSet, 
											  PropositionSet *propSet, 
											  const Parse *secondaryParse,
											  const NPChunkTheory *npChunk,
											  const PropTreeLinks* ptLinks) 
{
	_1bestSentenceInfo.populate(eset, parse, secondaryParse, mentionSet, valueMentionSet, propSet, npChunk, Symbol(L":NULL"), 0, ptLinks);
	resetForNewSentence(&_1bestSentenceInfo);
}

void RelationObservation::resetForNewSentence(const Parse *parse,
											  const MentionSet *mentionSet, const ValueMentionSet *valueMentionSet, 
											  PropositionSet *propSet, 
											  const Parse *secondaryParse,
											  const NPChunkTheory *npChunk) 
{
	_1bestSentenceInfo.populate(0, parse, secondaryParse, mentionSet, valueMentionSet, propSet, npChunk, Symbol(L":NULL"));
	resetForNewSentence(&_1bestSentenceInfo);
}

void RelationObservation::resetForNewSentence(EntitySet *eset, 
											  const Parse *parse,
											  const MentionSet *mentionSet, 
											  const ValueMentionSet *valueMentionSet, 
											  PropositionSet *propSet, 
											  Symbol documentTopic,
											  const Parse *secondaryParse,
											  const NPChunkTheory *npChunk,
											  const PropTreeLinks* ptLinks) 
{
	_1bestSentenceInfo.populate(eset, parse, secondaryParse, mentionSet, valueMentionSet, propSet, npChunk, documentTopic, 0, ptLinks);
	resetForNewSentence(&_1bestSentenceInfo);
}

void RelationObservation::resetForNewSentence(const Parse *parse,
											  const MentionSet *mentionSet, const ValueMentionSet *valueMentionSet, 
											  PropositionSet *propSet, 
											  Symbol documentTopic,
											  const Parse *secondaryParse,
											  const NPChunkTheory *npChunk) 
{
	_1bestSentenceInfo.populate(0, parse, secondaryParse, mentionSet, valueMentionSet, propSet, npChunk, documentTopic);
	resetForNewSentence(&_1bestSentenceInfo);
}

void RelationObservation::populate(int mention1ID, int mention2ID) {
	_m1_id = mention1ID;
	_m2_id = mention2ID;
	_m1 = _sentenceInfo->mentionSets[0]->getMention(_m1_id);
	_m2 = _sentenceInfo->mentionSets[0]->getMention(_m2_id);

	int m1_head_index = _m1->getNode()->getHeadPreterm()->getEndToken();
	int m2_head_index = _m2->getNode()->getHeadPreterm()->getEndToken();
	// standardize ordering by where they are in the sentence
	if (m1_head_index > m2_head_index) {
		int temp = _m1_id;
		_m1_id = _m2_id;
		_m2_id = temp;
		const Mention *tm = _m1;
		_m1 = _m2;
		_m2 = tm;
	}

	for (int i = 0; i < _sentenceInfo->nTheories; i++) {
		findPropLink(i);
	}
	
	delete m1Name;
	delete m2Name;
	m1Name = 0;
	m2Name = 0;
	
	findMentionNames();
	findWordsBetween();
	_n_predictions = 0;
	if (_altModels.getNAltDecoders() > 0) {
		_n_predictions = _altModels.addDecoderPredictionsToObservation(this);
	}
	_altPatternPrediction = Symbol();
	/*_instance->setStandardInstance(this);
	if (_patternModel != 0) {
	if (!_propLinks[0]->isEmpty() && !_propLinks[0]->isNegative()) {
	_altPatternPrediction = _patternModel->findBestRelationType(_instance, false, false);
	}
	}*/


	//added for relation models based on NP chunk output
	if (_npchunkFeatures) {
		_stemmedHeadofm1 = WordNet::getInstance()->stem_noun(_m1->getNode()->getHeadWord());
		_stemmedHeadofm2 = WordNet::getInstance()->stem_noun(_m2->getNode()->getHeadWord());

		_hasPossessiveRel = false;
		findPossessiveRel();
		_hasPossessiveAfterMent2 = false;
		setHasPossessiveAfterMent2();
		_hasPPRel = false;
		_verbinPPRel = Symbol(L"NULL");
		findPPRel();
		_verbinProp = Symbol(L"NULL");
		findverbProp();
	}


}

// original function
/*
void RelationObservation::findWordsBetween() {

_wordsBetweenMentions = CONFUSED;
_posBetweenMentions = CONFUSED;
_stemmedWBMentions = CONFUSED;

// this is bad form, and should be fixed later
if (SerifVersion::isEnglish()) {
// when we are dealing with two premodifiers, we should deal with this differently
if (_sentenceInfo->entitySets[0] &&
_m1->getNode()->getParent() == _m2->getNode()->getParent() &&
_m1->getParent() == 0 && _m2->getParent() == 0)
{
bool title_case = false;
if (_m1->getNode()->getParent()->hasMention()) {
const Mention *parentMent 
= _sentenceInfo->mentionSets[0]->getMentionByNode(_m1->getNode()->getParent());
if (parentMent->getEntityType().isRecognized()) {
Entity *parentEnt = _sentenceInfo->entitySets[0]->getEntityByMention(parentMent->getUID(),parentMent->getEntityType());
Entity *m1Ent = _sentenceInfo->entitySets[0]->getEntityByMention(_m1->getUID(),_m1->getEntityType());
Entity *m2Ent = _sentenceInfo->entitySets[0]->getEntityByMention(_m2->getUID(),_m2->getEntityType());
if (parentEnt != 0 && (parentEnt == m1Ent || parentEnt == m2Ent))
title_case = true;
}
}
if (!title_case) {
_wordsBetweenMentions = Symbol(L"TWO_PREMODS");
_posBetweenMentions = Symbol(L"TWO_PREMODS");
_stemmedWBMentions = Symbol(L"TWO_PREMODS");
return;
}
}
}

int end1 = _m1->getNode()->getEndToken();
if (_m1->getNode()->getEndToken() < _m2->getNode()->getStartToken()) {
makeWBSymbol(_m1->getNode()->getEndToken() + 1, _m2->getNode()->getStartToken());
return;
} 

const SynNode* node1 = _m1->getNode()->getHeadPreterm();
const SynNode* node2 = _m2->getNode()->getHeadPreterm();
if (node1 != _m1->getNode())
node1 = _m1->getNode()->getHeadPreterm()->getParent();
if (node2 != _m2->getNode())
node2 = _m2->getNode()->getHeadPreterm()->getParent();

if (node1->getEndToken() < node2->getStartToken()) {
makeWBSymbol(node1->getEndToken() + 1, node2->getStartToken());
return;
} 

const SynNode *temp = node2;
node2 = _m2->getNode()->getHeadPreterm();
if (node1->getEndToken() < node2->getStartToken()) {
makeWBSymbol(node1->getEndToken() + 1, node2->getStartToken());
return;
} 
node2 = temp;

node1 = _m1->getNode()->getHeadPreterm();
if (node1->getEndToken() < node2->getStartToken()) {
makeWBSymbol(node1->getEndToken() + 1, node2->getStartToken());
return;
} 

node2 = _m2->getNode()->getHeadPreterm();
if (node1->getEndToken() < node2->getStartToken()) {
makeWBSymbol(node1->getEndToken() + 1, node2->getStartToken());
return;
} 

//std::cerr << _m1->getNode()->toDebugTextString() << " ~ " << _m2->getNode()->toDebugTextString() << "\n";
//std::cerr << node1->toDebugTextString() << " ~ " << node2->toDebugTextString() << "\n";
//std::cerr << _m1->getMentionType() << " " << _m2->getMentionType() << "\n\n";

}
*/

// to improve relation model's performance when using NP chunk
// added on 20071126
// diff from original: not use TWO_PREMODS feature
void RelationObservation::findWordsBetween() {

	_wordsBetweenMentions = CONFUSED;
	_posBetweenMentions = CONFUSED;
	_stemmedWBMentions = CONFUSED;

	// this is bad form, and should be fixed later
	if (SerifVersion::isEnglish()) {
//#ifdef ENGLISH_LANGUAGE
		// when we are dealing with two premodifiers, we should deal with this differently
		if (_sentenceInfo->entitySets[0] &&
			_m1->getNode()->getParent() == _m2->getNode()->getParent() &&
			_m1->getParent() == 0 && _m2->getParent() == 0)
		{
			bool title_case = false;
			if (_m1->getNode()->getParent()->hasMention()) {
				const Mention *parentMent 
					= _sentenceInfo->mentionSets[0]->getMentionByNode(_m1->getNode()->getParent());
				if (parentMent->getEntityType().isRecognized()) {
					Entity *parentEnt = _sentenceInfo->entitySets[0]->getEntityByMention(parentMent->getUID(),parentMent->getEntityType());
					Entity *m1Ent = _sentenceInfo->entitySets[0]->getEntityByMention(_m1->getUID(),_m1->getEntityType());
					Entity *m2Ent = _sentenceInfo->entitySets[0]->getEntityByMention(_m2->getUID(),_m2->getEntityType());
					if (parentEnt != 0 && (parentEnt == m1Ent || parentEnt == m2Ent))
						title_case = true;
				}
			}

			if (!_npchunkFeatures) {
				if (!title_case) {
					_wordsBetweenMentions = L"TWO_PREMODS";
					_posBetweenMentions = L"TWO_PREMODS";
					_stemmedWBMentions = L"TWO_PREMODS";
					return;
				}
			}
		}
	}
//#endif

	if (_npchunkFeatures) {
		int end1 = _m1->getNode()->getEndToken();
		int start2=_m2->getNode()->getStartToken();
		if (end1+1 == start2 && _m2->getNode()->getFirstTerminal()->getHeadWord() == Symbol(L"'s")){

			// option 1
			const SynNode* node1 = _m1->getNode()->getHeadPreterm();
			const SynNode* node2 = _m2->getNode()->getHeadPreterm();
			makeWBSymbol(node1->getEndToken() + 1, node2->getStartToken());


			// option 2
			/*
			_wordsBetweenMentions = Symbol(L"'s");
			_posBetweenMentions = Symbol(L"POS");
			_stemmedWBMentions = Symbol(L"'s");
			*/
			return;

		}
	}


	if (_m1->getNode()->getEndToken() < _m2->getNode()->getStartToken()) {
		makeWBSymbol(_m1->getNode()->getEndToken() + 1, _m2->getNode()->getStartToken());
		return;
	} 

	const SynNode* node1 = _m1->getNode()->getHeadPreterm();
	const SynNode* node2 = _m2->getNode()->getHeadPreterm();
	if (node1 != _m1->getNode())
		node1 = _m1->getNode()->getHeadPreterm()->getParent();
	if (node2 != _m2->getNode())
		node2 = _m2->getNode()->getHeadPreterm()->getParent();

	if (node1->getEndToken() < node2->getStartToken()) {
		makeWBSymbol(node1->getEndToken() + 1, node2->getStartToken());
		return;
	} 

	const SynNode *temp = node2;
	node2 = _m2->getNode()->getHeadPreterm();
	if (node1->getEndToken() < node2->getStartToken()) {
		makeWBSymbol(node1->getEndToken() + 1, node2->getStartToken());
		return;
	} 
	node2 = temp;

	node1 = _m1->getNode()->getHeadPreterm();
	if (node1->getEndToken() < node2->getStartToken()) {
		makeWBSymbol(node1->getEndToken() + 1, node2->getStartToken());
		return;
	} 

	node2 = _m2->getNode()->getHeadPreterm();
	if (node1->getEndToken() < node2->getStartToken()) {
		makeWBSymbol(node1->getEndToken() + 1, node2->getStartToken());
		return;
	} 

	//std::cerr << _m1->getNode()->toDebugTextString() << " ~ " << _m2->getNode()->toDebugTextString() << "\n";
	//std::cerr << node1->toDebugTextString() << " ~ " << node2->toDebugTextString() << "\n";
	//std::cerr << _m1->getMentionType() << " " << _m2->getMentionType() << "\n\n";

}

Symbol RelationObservation::isMentionOrValue(int start, int end, int& skipto) {
	for (int j = 0; j < _sentenceInfo->mentionSets[0]->getNMentions(); j++) {
		const Mention *ment = _sentenceInfo->mentionSets[0]->getMention(j);
		if (ment->getNode()->getStartToken() == start &&
			ment->getNode()->getEndToken() < end &&
			ment->getEntityType().isRecognized()) 
		{
			skipto = ment->getNode()->getEndToken();
			return ment->getEntityType().getName();
		}
	}
	// For some reason, this actually hurts us. I think this is weird, but I'm turning it off.
	/*for (int k = 0; k < _sentenceInfo->valueMentionSets[0]->getNValueMentions(); k++) {
	const ValueMention *vment = _sentenceInfo->valueMentionSets[0]->getValueMention(k);
	if (vment->getStartToken() == start && vment->getEndToken() < end) {
	skipto = vment->getEndToken();
	Symbol sym = vment->getFullType().getNameSymbol();
	// hack
	if (sym == Symbol(L"TIMEX2.TIME"))
	sym = Symbol(L"TIMEX2");
	return sym;
	}
	}*/
	return Symbol();
}

Symbol RelationObservation::isReducedGroup(int start, int end, int& skipto) {
	// Everything intelligent I try to do doesn't help. 
	/*const SynNode *root = _sentenceInfo->parses[0]->getRoot();
	const SynNode *term = root->getNthTerminal(start);
	while (term->getParent() != 0 && term->getEndToken() < end) {
	if (LanguageSpecificFunctions::isCoreNPLabel(term->getTag())) {
	skipto = term->getEndToken();
	return term->getTag();
	}
	term = term->getParent();
	}
	int nonverb = start;
	for ( ; nonverb < end; nonverb++) {
	if (!LanguageSpecificFunctions::isVerbPOSLabel(_pos[nonverb]))
	break;
	}
	if (nonverb > start) {
	skipto = nonverb - 1;
	return Symbol(L"VBGROUP");
	}*/
	return Symbol();
}



void RelationObservation::makeWBSymbol(int start, int end) {
	if (end - start > 10) {
		_wordsBetweenMentions = TOO_LONG;
		_posBetweenMentions = TOO_LONG;
		_stemmedWBMentions = TOO_LONG;
		return;
	}
	if (start == end) {
		_wordsBetweenMentions = ADJACENT;
		_posBetweenMentions = ADJACENT;
		_stemmedWBMentions = ADJACENT;
		return;
	}
	std::wstring str = L"";
	std::wstring posStr = L"";	
	std::wstring stemStr = L"";
	int skipto = -1;
	for (int i = start; i < end; i++) {
		Symbol mentOrValue = isMentionOrValue(i, end, skipto);
		if (!mentOrValue.is_null()) {
			str += mentOrValue.to_string();
			posStr += mentOrValue.to_string();
			stemStr += mentOrValue.to_string();
			i = skipto;
		} else {
			Symbol groupingSym = isReducedGroup(i, end, skipto);
			if (!groupingSym.is_null()) {
				i = skipto;
				posStr += groupingSym.to_string();
			} else {				
				posStr += _pos[i].to_string();
			}
			str += _tokens[i].to_string();
			Symbol word = RelationUtilities::get()->stemWord(_tokens[i].to_string(), _pos[i]);
			stemStr += word.to_string();
		}
		if (i != end - 1) {
			str += L"_";
			posStr += L"_";
			stemStr += L"_";
		}
	}
	_wordsBetweenMentions = str.c_str();
	_posBetweenMentions = posStr.c_str();
	_stemmedWBMentions = stemStr.c_str();

}


const TreeNodeChain* RelationObservation::getPropTreeNodeChain() {
	return ( (_m1 && _m2) ? _sentenceInfo->propTreeLinks[0]->getLink(_m1_id, _m2_id) : 0 );
}


void RelationObservation::findPropLink(int index)
{
	const PropositionSet *propSet = _sentenceInfo->propSets[index];
	const MentionSet *mentionSet = _sentenceInfo->mentionSets[index];
	int mappedArg1 = -1;
	int mappedArg2 = -1;
	if (index == 0) {
		mappedArg1 = _m1->getIndex();
		mappedArg2 = _m2->getIndex();
	} else {
		for (int i = 0; i < mentionSet->getNMentions(); i++) {
			if (mentionSet->getMention(i)->getMentionType() != Mention::NONE &&
				mentionSet->getMention(i)->getMentionType() != Mention::APPO &&
				mentionSet->getMention(i)->getMentionType() != Mention::LIST)
			{
				int end_token = mentionSet->getMention(i)->getNode()->getHeadPreterm()->getEndToken();
				if (end_token == _m1->getNode()->getHeadPreterm()->getEndToken())
					mappedArg1 = i;
				else if (end_token == _m2->getNode()->getHeadPreterm()->getEndToken())
					mappedArg2 = i;
			}
		}
	}

	if (mappedArg1 == -1 ||
		mappedArg2 == -1)
	{
		_propLinks[index]->reset();
		return;
	}

	// props that link them both
	for (int i = 0; i < propSet->getNPropositions(); i++) {
		Proposition *p = propSet->getProposition(i);
		Argument *a1 = 0;
		Argument *a2 = 0;
		for (int j = 0; j < p->getNArgs(); j++) {
			if (p->getArg(j)->getType() == Argument::MENTION_ARG) {
				if (p->getArg(j)->getMentionIndex() == mappedArg1)
					a1 = p->getArg(j);
				else if (p->getArg(j)->getMentionIndex() == mappedArg2)
					a2 = p->getArg(j);
				else {
					Proposition *set = propSet->getDefinition(p->getArg(j)->getMentionIndex());
					if (set != 0 && set->getPredType() == Proposition::SET_PRED) {
						for (int k = 1; k < set->getNArgs(); k++) {
							Argument *setArg = set->getArg(k);
							if (setArg->getType() == Argument::MENTION_ARG) {
								// we need these breaks here so we don't take both mentions
								// from the same set
								if (setArg->getMentionIndex() == mappedArg1) {
									a1 = p->getArg(j);
									break;
								} else if (setArg->getMentionIndex() == mappedArg2) {
									a2 = p->getArg(j);
									break;
								}
							}
						}
					}
				}
			}
		}
		// if we find both in one proposition, yay!
		if (a1 != 0 && a2 != 0) {
			_propLinks[index]->populate(p, a1, a2, mentionSet);
			if (RelationUtilities::get()->isPrepStack(this)) {
				Proposition *def = propSet->getDefinition(a1->getMentionIndex());
				if (def != 0)
					_propLinks[index]->populate(def, def->getArg(0), a2, mentionSet);
			}
			return;
		}
	}

	// nesting
	/*for (int i = 0; i < propSet->getNPropositions(); i++) {
	Proposition *p = propSet->getProposition(i);
	Argument *a1 = 0;
	Argument *intermed = 0;
	Argument *a2 = 0;
	Proposition *intermedProp = 0;
	int nest_direction = 0;
	for (int j = 0; j < p->getNArgs(); j++) {
	if (p->getArg(j)->getType() == Argument::MENTION_ARG) {
	if (p->getArg(j)->getMentionIndex() == mappedArg1)
	a1 = p->getArg(j);
	else if (p->getArg(j)->getMentionIndex() == mappedArg2)
	a2 = p->getArg(j);
	else {
	Proposition *def = propSet->getDefinition(p->getArg(j)->getMentionIndex());
	if (def == 0)
	continue;
	for (int k = 1; k < def->getNArgs(); k++) {
	Argument *defArg = def->getArg(k);
	if (defArg->getType() == Argument::MENTION_ARG) {
	// we need these breaks here so we don't take both mentions
	// from the same set
	if (def->getPredType() == Proposition::SET_PRED) {
	if (defArg->getMentionIndex() == mappedArg1) {
	a1 = p->getArg(j);
	break;
	} else if (defArg->getMentionIndex() == mappedArg2) {
	a2 = p->getArg(j);
	break;
	}
	} else if (defArg->getMentionIndex() == mappedArg1) {
	intermedProp = def;
	a1 = def->getArg(k);
	intermed = p->getArg(j);
	nest_direction = RelationPropLink::LEFT;
	break;
	} else if (defArg->getMentionIndex() == mappedArg2) {
	intermedProp = def;
	a2 = def->getArg(k);
	intermed = p->getArg(j);
	nest_direction = RelationPropLink::RIGHT;
	break;
	}
	}
	} 
	}
	}
	}
	// if we find both in one (nested) proposition, yay!
	if (a1 != 0 && a2 != 0 && intermed != 0) {
	_propLinks[index]->populate(p, intermedProp, a1, intermed, a2, 
	mentionSet, nest_direction);
	return;
	}
	}*/

	if ( ParamReader::isParamTrue("simulate_props_from_proptrees") && 
		_sentenceInfo->propTreeLinks && _sentenceInfo->propTreeLinks[0] ) {
			const TreeNodeChain* tnChain=_sentenceInfo->propTreeLinks[0]->getLink(mappedArg1,mappedArg2);
			if ( tnChain ) {
				_propLinks[index]->populate(tnChain);
				return;
			}
	}

	_propLinks[index]->reset();

}
void RelationObservation::setAltDecoderPrediction(int i, Symbol prediction){
	_altDecoderPredictions[i] = prediction;
}
Symbol RelationObservation::getNthAltDecoderName(int i){
	return _altModels.getDecoderName(i);
}
Symbol RelationObservation::getNthAltDecoderPrediction(int i){
	return _altDecoderPredictions[i];
}
int RelationObservation::getNAltDecoderPredictions(){
	return _n_predictions;
}

// functions added for relation models based on NP chunk
void RelationObservation::findPossessiveRel(){
	int end1 = _m1->getNode()->getEndToken();
	int start2 = _m2->getNode()->getStartToken();
	if (end1+1 == start2 && _m2->getNode()->getFirstTerminal()->getHeadWord() == Symbol(L"'s")){
		_hasPossessiveRel = true;

		// old implementation, not use stem words
		//_n_offsets_ment2 = SymbolUtilities::fillWordNetOffsets(_m2->getNode()->getHeadWord(), _wordnetOffsetsofMent2, REL_MAX_WN_OFFSETS);
		//_wcMent2 = WordClusterClass(_m2->getNode()->getHeadWord(), true);

		// use stem words
		_n_offsets_ment2 = SymbolUtilities::fillWordNetOffsets(_stemmedHeadofm2, _wordnetOffsetsofMent2, REL_MAX_WN_OFFSETS);
		_wcMent2 = WordClusterClass(_stemmedHeadofm2, true);
	}
	return;
}


void RelationObservation::setHasPossessiveAfterMent2(){
	int nextIndex = _m2->getNode()->getEndToken()+1;
	if (nextIndex < MAX_SENTENCE_TOKENS && _tokens[nextIndex] == Symbol(L"'s")){
		_hasPossessiveAfterMent2 = true;
	}
	return;
}

void RelationObservation::findPPRel(){
	// simplify the judgment by only caring about 
	// two mentions which are at the same level of the NP chunk parse tree (common case)
	const SynNode *node1=_m1->getNode();
	const SynNode *node2=_m2->getNode();
	if (node1->getParent() == node2->getParent()){
		int start2 = _m2->getNode()->getStartToken(); 
		int onePosBefstart2 = start2  - 1;
		if (onePosBefstart2 >= 0  && _pos[onePosBefstart2] == Symbol(L"IN")){
			const SynNode *parent=node1->getParent();
			int i = 0;
			while (parent->getChild(i) != node1){
				i++;
			}
			i++;
			while (parent->getChild(i) != node2){
				const SynNode *child = parent->getChild(i);
				Symbol childPOS = child->getTag();

				if ( childPOS == Symbol(L"NP")){
					_verbinPPRel = Symbol(L"NULL");
					return;
				}

				// fix a bug on 2008/01/25 (bug: use "VP" for node search in NP Chunk parses )
				//char childPOStxt[50];
				std::string childPOStxt;
				childPOStxt = childPOS.to_debug_string();
				if ( childPOStxt[0] == 'V'){
					_verbinPPRel = child->getHeadWord();
				}
				i++;
			}

			_prepinPPRel = _tokens[onePosBefstart2];
			_hasPPRel = true;

			// the code written here are not efficient
			// but is necessary in order to use the mixtype-pp-relation-wordnet

			// old implementation -- not use stem words 
			// _n_offsets_ment1 = SymbolUtilities::fillWordNetOffsets(_m1->getNode()->getHeadWord(), _wordnetOffsetsofMent1, REL_MAX_WN_OFFSETS);
			// _wcMent1 = WordClusterClass(_m1->getNode()->getHeadWord(), true);

			// use stem words
			_n_offsets_ment1 = SymbolUtilities::fillWordNetOffsets(_stemmedHeadofm1, _wordnetOffsetsofMent1, REL_MAX_WN_OFFSETS);
			_wcMent1 = WordClusterClass(_stemmedHeadofm1, true);

			if (_verbinPPRel != Symbol(L"NULL")){
				_stemmedVerbinPPRel = WordNet::getInstance()->stem_verb(_verbinPPRel);
				// old implementation -- not use stem words 
				//_n_offsets_verb =  SymbolUtilities::fillWordNetOffsets(_verbinPPRel, _wordnetOffsetsofVerb, REL_MAX_WN_OFFSETS);
				//_wcVerb = WordClusterClass(_verbinPPRel, true);

				// use stem words
				_n_offsets_verb =  SymbolUtilities::fillWordNetOffsets(_stemmedVerbinPPRel, _wordnetOffsetsofVerb, REL_MAX_WN_OFFSETS);
				_wcVerb = WordClusterClass(_stemmedVerbinPPRel, true);
			}

		}
	}
	return;
}

void RelationObservation::findverbProp(){
	// simplify the judgment by only caring about 
	// two mentions which are at the same level of the NP chunk parse tree (common case) and 
	// there is a verb between these two NP's (need to filter out certain cases, e.g., 
	// NP1 to verb NP2; NP1 's NPx verb NP2 etc.)

	const SynNode *node1=_m1->getNode();
	const SynNode *node2=_m2->getNode();
	int state = 0;
	if (node1->getParent() == node2->getParent()){

		const SynNode *parent=node1->getParent();
		int i = 0;

		while (parent->getChild(i) != node1){
			i++;
		}
		i++;

		while (parent->getChild(i) != node2){
			const SynNode *child = parent->getChild(i);
			Symbol childPOS = child->getTag();

			if (state == 0){
				if ( childPOS == Symbol(L"NP") || childPOS == Symbol(L"TO")){
					return;
				}

				std::string childPOStxt;
				childPOStxt = childPOS.to_debug_string();
				if ( childPOStxt[0] == 'V'){
					_verbinProp = child->getHeadWord();
					state = 1;
				}
			}else if (state == 1){
				// refine verb proposition identification 
				if ( childPOS == Symbol(L"NP") || childPOS == Symbol(L"IN")){
					_verbinProp = Symbol(L"NULL");
					return;
				}

				std::string childPOStxt;
				childPOStxt = childPOS.to_debug_string();
				if ( childPOStxt[0] == 'V'){
					_verbinProp = child->getHeadWord();
				}
			}

			if (_verbinProp != Symbol(L"NULL")){
				_stemmedVerbinProp = WordNet::getInstance()->stem_verb(_verbinProp);

				// old implementation, not use stem words
				//_n_offsets_verbPred =  SymbolUtilities::fillWordNetOffsets(_verbinProp, _wordnetOffsetsofVerbPred, REL_MAX_WN_OFFSETS);
				//_wcVerbPred = WordClusterClass(_verbinProp, true);				

				//use stem words
				_n_offsets_verbPred =  SymbolUtilities::fillWordNetOffsets(_stemmedVerbinProp, _wordnetOffsetsofVerbPred, REL_MAX_WN_OFFSETS);
				_wcVerbPred = WordClusterClass(_stemmedVerbinProp, true);				
			}

			i++;

		}
	}
	return;
}

void RelationObservation::findMentionNames() {
	int n_symbols;
	Symbol name_symbols[16];

	delete m1Name;
	n_symbols = getMentionNameSymbols(_m1, name_symbols, 16);
	m1Name = _new SymbolArray(name_symbols, n_symbols);

	delete m2Name;
	n_symbols = getMentionNameSymbols(_m2, name_symbols, 16);
	m2Name = _new SymbolArray(name_symbols, n_symbols);

}

int RelationObservation::getMentionNameSymbols(const Mention *ment, Symbol *array_, int max_length) {

	if (ment->getMentionType() == Mention::NAME) {
		return ment->getNode()->getTerminalSymbols(array_, max_length);
	}
	// Ideally, we'd like to find the canonical name for the entity, but referencing
	// mentions in previous sentences presents a problem for the trainer, so we'll just
	// stick with the current mention for now.
	/*else {
	const EntitySet *eset = _sentenceInfo->entitySets[0];
	Entity *ent = eset->getEntityByMention(ment->getUID(), ment->getEntityType());

	int n_longest_name = 0;
	Mention *long_name = 0;
	for (int i = 0; i < ent->getNMentions(); i++) {
	Mention *m = eset->getMention(ent->getMention(i));
	int diff = m->getNode()->getEndToken() - m->getNode()->getStartToken() + 1;
	if (m->getMentionType() == Mention::NAME && diff > n_longest_name) {
	n_longest_name = diff;
	long_name = m;
	}
	}

	if (long_name != 0) {
	return long_name->getNode()->getTerminalSymbols(array_, max_length);
	}*/
	else {
		return ment->getNode()->getTerminalSymbols(array_, max_length);
	}
}

void RelationObservation::examineFeatures(DTFeatureTypeSet *features)
{
	if (features == 0)
		return;

	for (int i = 0; i < features->getNFeaturesTypes(); i++) {
		const DTFeatureType *featureType = features->getFeatureType(i);
		Symbol name = featureType->getName();

		if (name == Symbol(L"poss-relation") ||
			name == Symbol(L"poss-after-ment2") ||
			name == Symbol(L"poss-wordnet") ||
			name == Symbol(L"mixtype-pp-relation-head") ||
			name == Symbol(L"mixtype-simple-pp-relation") ||
			name == Symbol(L"verb-prop") ||
			name == Symbol(L"verb-prop-wordnet") ||
			name == Symbol(L"verb-prop-wc")) 
		{
			_npchunkFeatures = true;
			return;
		}
	}
	return;
}
