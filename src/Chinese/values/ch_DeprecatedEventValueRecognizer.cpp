// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.
#include "Generic/common/leak_detection.h"

#include "Chinese/values/ch_DeprecatedEventValueRecognizer.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/EventSet.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/Event.h"
#include "Generic/events/patterns/EventPatternMatcher.h"
#include "Generic/events/patterns/EventPatternSet.h"
#include "Generic/events/patterns/EventSearchNode.h"
#include <boost/scoped_ptr.hpp>

ChineseDeprecatedEventValueRecognizer::ChineseDeprecatedEventValueRecognizer() {

	std::string rules = ParamReader::getParam("event_value_patterns");
	if (!rules.empty()) {
		EventPatternSet *set = _new EventPatternSet(rules.c_str());
		_matcher = _new EventPatternMatcher(set);	
	} else _matcher = 0;
	
	TOPLEVEL_PATTERNS = Symbol(L"TOPLEVEL_PATTERNS");
	CERTAIN_SENTENCES = Symbol(L"CERTAIN_SENTENCES");

	if (ParamReader::isParamTrue("add_event_values_as_event_args"))
		_add_values_as_args = true;
	else
		_add_values_as_args = false;

	std::string title_file = ParamReader::getParam("job_title_list");
	if (!title_file.empty()) {
		_jobTitleList = _new SymbolHash(title_file.c_str());
	} else _jobTitleList = 0;

	std::string crime_file = ParamReader::getParam("crime_list");
	if (!crime_file.empty()) {
		boost::scoped_ptr<UTF8InputStream> crimeFile_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& crimeFile(*crimeFile_scoped_ptr);
		crimeFile.open(crime_file.c_str());
		_crimeList = _new ChineseDeprecatedEventValueRecognizer::SymbolSet(1024);
		loadList(_crimeList, crimeFile);
		crimeFile.close();
	} else _crimeList = 0;

	std::string sentence_file = ParamReader::getParam("crime_sentence_list");
	if (!sentence_file.empty()) {
		boost::scoped_ptr<UTF8InputStream> sentenceFile_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& sentenceFile(*sentenceFile_scoped_ptr);
		sentenceFile.open(sentence_file.c_str());
		_sentenceList = _new ChineseDeprecatedEventValueRecognizer::SymbolSet(1024);
		loadList(_sentenceList, sentenceFile);
		sentenceFile.close();
	} else _sentenceList = 0;

}

ChineseDeprecatedEventValueRecognizer::~ChineseDeprecatedEventValueRecognizer() {
	delete _jobTitleList;
	delete _crimeList;
	delete _sentenceList;
}

void ChineseDeprecatedEventValueRecognizer::createEventValues(DocTheory* docTheory) {
	_n_value_mentions = 0;

	EventSet *eventSet = docTheory->getEventSet();
	for (int i = 0; i < eventSet->getNEvents(); i++) {
		Event *event = eventSet->getEvent(i);
		Event::LinkedEventMention *vment = event->getEventMentions();
		while (vment != 0) {
			int sentno = vment->eventMention->getSentenceNumber();			
			const PropositionSet *propSet = docTheory->getSentenceTheory(sentno)->getPropositionSet();
			
			bool found_position = false;
			bool found_sentence = false;
			bool found_crime = false;
			if (vment->eventMention->getAnchorProp() != 0 && _matcher != 0) {

				_matcher->resetForNewSentence(propSet,
					docTheory->getSentenceTheory(sentno)->getMentionSet(),
					docTheory->getSentenceTheory(sentno)->getValueMentionSet());

				EventSearchNode *result = 
					_matcher->matchAllPossibleNodes(vment->eventMention->getAnchorProp(), 
												    TOPLEVEL_PATTERNS);
				if (result != 0) {
					EventSearchNode *iter = result;
					EventSearchNode *iterNext;
					while (iter != 0) {
						iterNext = iter->getNext();
						iter->setNext(0);
						if (getBaseType(vment->eventMention->getEventType()) == Symbol(L"Justice") &&
							vment->eventMention->getFirstValueForSlot(Symbol(L"Crime")) == 0) 
						{
							const SynNode *node = iter->findNodeForOntologyLabel(Symbol(L"Crime"));
							if (node) {
								addValueMention(docTheory, vment->eventMention, node, 
									Symbol(L"Crime"), Symbol(L"Crime"), 1.0);
								found_crime = true;
							}
						}
						if (vment->eventMention->getEventType() == Symbol(L"Justice.Sentence") &&
							vment->eventMention->getFirstValueForSlot(Symbol(L"Sentence")) == 0) 
						{
							const SynNode *node = iter->findNodeForOntologyLabel(Symbol(L"Sentence"));
							if (node) {
								addValueMention(docTheory, vment->eventMention, node, 
									Symbol(L"Sentence"), Symbol(L"Sentence"), 1.0);
								found_sentence = true;
							}
						}						
						if (getBaseType(vment->eventMention->getEventType()) == Symbol(L"Personnel") &&
							vment->eventMention->getFirstValueForSlot(Symbol(L"Position")) == 0) 
						{
							const SynNode *node = iter->findNodeForOntologyLabel(Symbol(L"Position"));
							if (node) {
								addValueMention(docTheory, vment->eventMention, node, 
									Symbol(L"Job-Title"), Symbol(L"Position"), 1.0);
								found_position = true;
							}
						}
						delete iter;
						iter = iterNext;
					}
				}
			}

			if (!found_position) 
				addPosition(docTheory, vment->eventMention);

			if (!found_sentence && _matcher != 0 &&
				vment->eventMention->getEventType() == Symbol(L"Justice.Sentence") &&
				vment->eventMention->getFirstValueForSlot(Symbol(L"Sentence")) == 0)
			{
				for (int p = 0; p < propSet->getNPropositions(); p++) {
					const Proposition *prop = propSet->getProposition(p);
					EventSearchNode *result = _matcher->matchAllPossibleNodes(prop, CERTAIN_SENTENCES);
					if (result != 0) {
						EventSearchNode *iter = result;
						EventSearchNode *iterNext;
						while (iter != 0 && !found_sentence) {
							iterNext = iter->getNext();
							iter->setNext(0);
							const SynNode *node = iter->findNodeForOntologyLabel(Symbol(L"Sentence"));
							if (node) {
								addValueMention(docTheory, vment->eventMention, node, 
									Symbol(L"Sentence"), Symbol(L"Sentence"), 1.0);
								found_sentence = true;
							}		
							delete iter;
							iter = iterNext;
						}
						delete iter;
					}
				}
			}

			if (!found_sentence &&
				vment->eventMention->getEventType() == Symbol(L"Justice.Sentence") &&
				vment->eventMention->getFirstValueForSlot(Symbol(L"Sentence")) == 0)
			{
				addSentence(docTheory, vment->eventMention);
			}

			if (!found_crime &&
				getBaseType(vment->eventMention->getEventType()) == Symbol(L"Justice") &&
				vment->eventMention->getFirstValueForSlot(Symbol(L"Crime")) == 0)
			{
				addCrime(docTheory, vment->eventMention);
			}

			vment = vment->next;
		}
	}

	// Document-level value mention set gets NULL as its TokenSequence.
	ValueMentionSet *valueMentionSet = _new ValueMentionSet(NULL, _n_value_mentions);
	for (int j = 0; j < _n_value_mentions; j++) {
		valueMentionSet->takeValueMention(j, _valueMentions[j]);
	}
	docTheory->takeDocumentValueMentionSet(valueMentionSet);
}

void ChineseDeprecatedEventValueRecognizer::addValueMention(DocTheory *docTheory, EventMention *vment, 
										   const Mention *ment, Symbol valueType, 
										   Symbol argType, float score)
{
	addValueMention(docTheory, vment, ment->getNode(), valueType, argType, score);
}

void ChineseDeprecatedEventValueRecognizer::addValueMentionForDesc(DocTheory *docTheory, EventMention *vment, 
												  const Mention *ment, Symbol valueType, 
												  Symbol argType, float score) 
{
	const MentionSet *mentionSet = docTheory->getSentenceTheory(vment->getSentenceNumber())->getMentionSet();

	const SynNode *head = ment->getNode()->getHead();
	if (head->hasMention() && 
		mentionSet->getMentionByNode(head)->getMentionType() == Mention::NONE)
	{
		const SynNode *child = head->getHead();
		while (child->hasMention() &&
				mentionSet->getMentionByNode(head)->getMentionType() == Mention::NONE)
		{
			head = child;
			child = head->getHead();
		}
		addValueMention(docTheory, vment, head, valueType, argType, score);
	}
	else {
		addValueMention(docTheory, vment, ment->getNode(), valueType, argType, score);
	}
}

void ChineseDeprecatedEventValueRecognizer::addValueMention(DocTheory *docTheory, EventMention *vment, 
										   const SynNode *node, Symbol valueType, Symbol argType,
										   float score) 
{
	addValueMention(docTheory, vment, node->getStartToken(), node->getEndToken(), valueType, argType, score);
}

void ChineseDeprecatedEventValueRecognizer::addValueMention(DocTheory *docTheory, EventMention *vment, 
										   int start_token, int end_token, Symbol valueType, 
										   Symbol argType, float score) 
{
	ValueMention *val = 0;
	for (int i = 0; i < _n_value_mentions; i++) {
		if ((_valueMentions[i]->getSentenceNumber() == vment->getSentenceNumber()) &&
			(_valueMentions[i]->getFullType().getNameSymbol() == valueType || _valueMentions[i]->getFullType().getNicknameSymbol() == valueType) &&
			(_valueMentions[i]->getStartToken() == start_token) &&
			(_valueMentions[i]->getEndToken() == end_token))
		{
			val = _valueMentions[i];
			break;
		}
	}

	if (val == 0 && _n_value_mentions < MAX_DOCUMENT_VALUES) {
		try {
			ValueMentionUID uid = ValueMention::makeUID(docTheory->getNSentences(), _n_value_mentions);
			val = _new ValueMention(vment->getSentenceNumber(), uid, start_token, end_token, valueType);
			_valueMentions[_n_value_mentions++] = val;
		} catch (InternalInconsistencyException &e) {
			SessionLogger::err("value_mention_uid") << e;
		}
	}

	if (val != 0 && _add_values_as_args)
		vment->addValueArgument(argType, val, score);
}


const Mention *ChineseDeprecatedEventValueRecognizer::getNthMentionFromProp(SentenceTheory *sTheory, 
														   const Proposition *prop, int n) 
{
	Argument *arg = prop->getArg(n);
	if (arg->getType() == Argument::MENTION_ARG) {
		return arg->getMention(sTheory->getMentionSet());
	} else return 0;
}

const SynNode *ChineseDeprecatedEventValueRecognizer::getNthPredHeadFromProp(const Proposition *prop, int n) {
	Argument *arg = prop->getArg(n);
	if (arg->getType() == Argument::PROPOSITION_ARG) {
		const SynNode *pred = arg->getProposition()->getPredHead();
		while (true) {
			if (pred->getParent() != 0 && pred->getParent()->getHead() == pred) 
				pred = pred->getParent();
			else break;
		}
		return pred;
	} else return 0;
}

void ChineseDeprecatedEventValueRecognizer::addPosition(DocTheory* docTheory, EventMention *vment) {
	if (getBaseType(vment->getEventType()) != Symbol(L"Personnel"))
		return;

	SentenceTheory *thisSentence = docTheory->getSentenceTheory(vment->getSentenceNumber());	

	for (int i = 0; i < vment->getNArgs(); i++) {
		if (vment->getNthArgRole(i) == Symbol(L"Person")) {
			const Mention *personMent = vment->getNthArgMention(i);
			if (personMent->mentionType == Mention::DESC &&
				isJobTitle(personMent->getNode()->getHeadWord())) 
			{
				addValueMentionForDesc(docTheory, vment, personMent, 
							Symbol(L"Job-Title"), Symbol(L"Position"), .5);
				return;
			}
			Entity *entity = docTheory->getEntitySet()->getEntityByMention(personMent->getUID());
			if (entity == 0)
				continue;
			for (int j = 0; j < entity->getNMentions(); j++) {
				if (Mention::getSentenceNumberFromUID(entity->getMention(j))
					== vment->getSentenceNumber()) 
				{
					int index = Mention::getIndexFromUID(entity->getMention(j));
					const Mention *ment = thisSentence->getMentionSet()->getMention(index);
					if (ment->mentionType == Mention::DESC && 
						isJobTitle(ment->getNode()->getHeadWord()))
					{
						addValueMentionForDesc(docTheory, vment, ment, Symbol(L"Job-Title"), 
									Symbol(L"Position"), .5);
						return;
					}
				}
			}
		}
	}
}

void ChineseDeprecatedEventValueRecognizer::addCrime(DocTheory* docTheory, EventMention *vment) {
	if (getBaseType(vment->getEventType()) != Symbol(L"Justice"))
		return;

	if (_crimeList == 0)
		return;

	const Sentence *thisSentence = docTheory->getSentence(vment->getSentenceNumber());
	SentenceTheory *thisSentenceTheory = docTheory->getSentenceTheory(vment->getSentenceNumber());	
	TokenSequence *tokens = thisSentenceTheory->getTokenSequence();

	const LocatedString *sentStr = thisSentence->getString();

	SymbolSet::iterator iter = _crimeList->begin();
	while (iter != _crimeList->end()) {
		Symbol crime = (*iter);
		int index = sentStr->indexOf(crime.to_string());
		if (index != -1) {
			size_t len = wcslen(crime.to_string());
			EDTOffset start_offset = sentStr->firstStartOffsetStartingAt<EDTOffset>(index);
			EDTOffset end_offset = sentStr->lastEndOffsetEndingAt<EDTOffset>(index + (int)len - 1);
			int start_tok = -1;
			int end_tok = -1;
			for (int i = 0; i < tokens->getNTokens(); i++) {
				if (tokens->getToken(i)->getStartEDTOffset() == start_offset) 
					start_tok = i;
				if (tokens->getToken(i)->getEndEDTOffset() == end_offset)
					end_tok = i;
			}
			if (start_tok != -1 && end_tok != -1) {
				addValueMention(docTheory, vment, start_tok, end_tok, Symbol(L"Crime"), Symbol(L"Crime"), 0.5);
			}
		}
		++iter;
	}

	return;
}


void ChineseDeprecatedEventValueRecognizer::addSentence(DocTheory* docTheory, EventMention *vment) {
	if (vment->getEventType() != Symbol(L"Justice.Sentence"))
		return;

	if (_sentenceList == 0)
		return;

	const Sentence *thisSentence = docTheory->getSentence(vment->getSentenceNumber());
	SentenceTheory *thisSentenceTheory = docTheory->getSentenceTheory(vment->getSentenceNumber());	
	TokenSequence *tokens = thisSentenceTheory->getTokenSequence();

	const LocatedString *sentStr = thisSentence->getString();

	SymbolSet::iterator iter = _sentenceList->begin();
	while (iter != _sentenceList->end()) {
		Symbol sentence = (*iter);
		int index = sentStr->indexOf(sentence.to_string());
		if (index != -1) {
			size_t len = wcslen(sentence.to_string());
			EDTOffset start_offset = sentStr->firstStartOffsetStartingAt<EDTOffset>(index);
			EDTOffset end_offset = sentStr->lastEndOffsetEndingAt<EDTOffset>(index + (int)len - 1);
			int start_tok = -1;
			int end_tok = -1;
			for (int i = 0; i < tokens->getNTokens(); i++) {
				if (tokens->getToken(i)->getStartEDTOffset() == start_offset) 
					start_tok = i;
				if (tokens->getToken(i)->getEndEDTOffset() == end_offset)
					end_tok = i;
			}
			if (start_tok != -1 && end_tok != -1) {
				addValueMention(docTheory, vment, start_tok, end_tok, Symbol(L"Sentence"), Symbol(L"Sentence"), 0.5);
			}
		}
		++iter;
	}

	return;
}


bool ChineseDeprecatedEventValueRecognizer::isJobTitle(Symbol word) {
	if (_jobTitleList != 0 && _jobTitleList->lookup(word)) 
		return true;
	return false;
}

bool ChineseDeprecatedEventValueRecognizer::isCrime(Symbol word) {
	if (_crimeList != 0 && _crimeList->find(word) != _crimeList->end()) 
		return true;
	return false;
}

bool ChineseDeprecatedEventValueRecognizer::isSentence(Symbol word) {
	if (_sentenceList != 0 && _sentenceList->find(word) != _sentenceList->end()) 
		return true;
	return false;
}

Symbol ChineseDeprecatedEventValueRecognizer::getBaseType(Symbol fullType) {
	std::wstring str = fullType.to_string();
	size_t index = str.find(L".");
	return Symbol(str.substr(0, index).c_str());
}

void ChineseDeprecatedEventValueRecognizer::loadList(SymbolSet *list, UTF8InputStream &file) {
	wchar_t buffer[256];
	while (!file.eof()) {
		file.getLine(buffer, 256);
		if (wcscmp(buffer, L"") != 0)
			list->insert(Symbol(buffer));
	}
}
