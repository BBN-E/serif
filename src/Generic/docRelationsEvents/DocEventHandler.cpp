// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/docRelationsEvents/DocEventHandler.h"
#include "Generic/docRelationsEvents/StatEventLinker.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/Sexp.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/EventSet.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/EventMention.h"
#include "Generic/events/EventUtilities.h"
#include "Generic/common/version.h"
#include "Generic/common/Symbol.h"
#include <math.h>
#include <boost/scoped_ptr.hpp>

//#if defined(ENGLISH_LANGUAGE) || defined(CHINESE_LANGUAGE) 
//#define TRANSFER_ARGS_BETWEEN_EVENTS
//#endif

static const Symbol _symbol_document = Symbol(L"document");
Symbol *DocEventHandler::_topics = 0;
SymbolHash **DocEventHandler::_topicWordSets = 0;
int *DocEventHandler::_topicWordCounts = 0;
int DocEventHandler::_n_topics = 0;
bool DocEventHandler::_topics_use_only_nouns = false;
bool DocEventHandler::_topics_initialized = false;

DocEventHandler::DocEventHandler(bool use_correct_events) 
	: _use_correct_events(use_correct_events), 
	  _eventLinker(0),_statEventLinker(0), _linkScores(0),
	  _correctAnswers(0)
{
	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");
	_allow_events_to_be_mentions = ParamReader::isParamTrue("allow_events_to_be_mentions");
	_allowMultipleEventsOfDifferentTypeWithSameAnchor = ParamReader::isParamTrue("allow_multiple_events_of_different_type_with_same_anchor");
	_allowMultipleEventsOfSameTypeWithSameAnchor = ParamReader::isParamTrue("allow_multiple_events_of_same_type_with_same_anchor");

	if (!use_correct_events) {
		SessionLogger::info("SERIF") << "Initializing document-level Event Finder...\n";
		_eventMentionFinder = _new EventFinder(_symbol_document);
		_eventMentionFinder->disallowMentionSetChanges();
	} else _eventMentionFinder = 0;

	SessionLogger::info("SERIF") << "Initializing document-level Event Linker...\n";
	std::string param = ParamReader::getParam("event_linking_style");
	if (!use_correct_events) {
		_event_linking_style = DOCUMENT;
		if (param == "SENTENCE")
			_event_linking_style = SENTENCE;
		else if (param == "DOCUMENT")
			_event_linking_style = DOCUMENT;
		else if (param == "NONE")
			_event_linking_style = NONE;
		else if (!param.empty())
			throw UnexpectedInputException("DocEventHandler::DocEventHandler()", 
					"Parameter 'event_linking_style' must be set to 'SENTENCE', 'DOCUMENT' or 'NONE'");
	} else {
		_event_linking_style = SENTENCE;
		if (!param.empty() && param != "SENTENCE") {
			SessionLogger::warn("coercing_parameter") << "Coercing parameter "
				<< "event_linking_style to be SENTENCE because we are producing "
				<< "correct events.";
		}
	}

	if (_event_linking_style == SENTENCE) {
		_eventLinker = EventLinker::build();
	} else if (_event_linking_style == DOCUMENT) {
		_statEventLinker = _new StatEventLinker(StatEventLinker::DECODE);
		_link_threshold = (float) ParamReader::getRequiredFloatParam("event_link_threshold");
	}	

	ignoreSym = Symbol(L"IGNORE");

	if (!_topics_initialized) 
		initializeDocumentTopics();

	_documentTopic = Symbol();

	std::string buffer = ParamReader::getParam("doc_event_debug");	
	DEBUG = false;
	if (!buffer.empty()) {
		DEBUG = true;
		_debugStream.open(buffer.c_str());
	}
	_use_preexisting_event_values = ParamReader::isParamTrue("use_preexisting_event_only_values");

}

void DocEventHandler::initializeDocumentTopics() {
	if (_topics_initialized)
		return;

	
	std::string buffer = ParamReader::getParam("event_topic_set");
	if (!buffer.empty()) {
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(buffer.c_str()));
		UTF8InputStream& in(*in_scoped_ptr);
		Sexp *sexp = _new Sexp(in);
		_n_topics = sexp->getNumChildren();
        _topics = _new Symbol[_n_topics];
		_topicWordSets = _new SymbolHash *[_n_topics];
		_topicWordCounts = _new int[_n_topics];
		for (int i = 0; i < _n_topics; i++) {
			_topicWordSets[i] = _new SymbolHash(50);
			Sexp *child = sexp->getNthChild(i);
			int nwords = child->getNumChildren();
			_topics[i] = child->getFirstChild()->getValue();
			for (int j = 1; j < nwords; j++) {
				_topicWordSets[i]->add(child->getNthChild(j)->getValue());
			}
		}
		delete sexp;
		in.close();
	}

	if (ParamReader::isParamTrue("event_topics_use_only_nouns"))
		_topics_use_only_nouns = true;

	_topics_initialized = true;
}

DocEventHandler::~DocEventHandler() {
	delete _eventLinker;
	delete _statEventLinker;
	delete _eventMentionFinder;
	delete[] _topics;
	delete[] _topicWordCounts;
	for (int i = 0; i < _n_topics; i++) {
		delete _topicWordSets[i];
	}
	delete[] _topicWordSets;
}

const DTTagSet* DocEventHandler::getEventTypeTagSet() {
	if (_eventMentionFinder)
		return _eventMentionFinder->getEventTypeTagSet();
	else
		return 0;
}

const DTTagSet* DocEventHandler::getArgumentTypeTagSet() {
	if (_eventMentionFinder)
		return _eventMentionFinder->getArgumentTypeTagSet();
	else
		return 0;
}

void DocEventHandler::createEventSet(DocTheory* docTheory) {
	if (DEBUG) {
		_debugStream << docTheory->getDocument()->getName() << L"\n";
	}

	if (_event_linking_style == NONE) {
		docTheory->takeEventSet(_new EventSet());
		return;
	}

	_documentTopic = assignTopic(docTheory);
	if (_eventMentionFinder)
		_eventMentionFinder->setDocumentTopic(_documentTopic);

	findEventMentions(docTheory);
	
	if (_event_linking_style == SENTENCE) {
		doSentenceLevelEventLinking(docTheory);
		return;
	}

	// filter and pile all the event mentions into one array
	int n_doc_mentions = 0;

	_allVMentions.clear(); //need to clear out vector for new mentions.

	for (int event_sentno = 0; event_sentno < docTheory->getNSentences(); event_sentno++) {
		EventMentionSet *eventMentionSet = docTheory->getSentenceTheory(event_sentno)->getEventMentionSet();
		MentionSet *mentionSet = docTheory->getSentenceTheory(event_sentno)->getMentionSet();
		int n_filtered = filterEventMentionSet(docTheory, mentionSet, eventMentionSet);

		EventMentionSet *filteredEventMentionSet = _new EventMentionSet(eventMentionSet->getParse());
		for (int v = 0; v < eventMentionSet->getNEventMentions(); v++) {			
			if (eventMentionSet->getEventMention(v)->getEventType() != ignoreSym){
				EventMention *newMention = _new EventMention(*eventMentionSet->getEventMention(v));
				filteredEventMentionSet->takeEventMention(newMention);
                
				_allVMentions.push_back(newMention);
				n_doc_mentions++; //NOTE: could also resize() _allVMentions above and access like array
			}
		}	
		docTheory->getSentenceTheory(event_sentno)->adoptSubtheory(SentenceTheory::EVENT_SUBTHEORY, filteredEventMentionSet);
	}

	doDocumentLevelEventLinking(docTheory, n_doc_mentions);
}

Symbol DocEventHandler::assignTopic(const DocTheory* docTheory) {
	if (!_topics_initialized) 
		initializeDocumentTopics();

	if (_n_topics == 0)
		return Symbol();

	for (int i = 0; i < _n_topics; i++) {
		_topicWordCounts[i] = 0;
	}
	int total_tokens = 0;
	for (int sent = 0; sent < docTheory->getNSentences(); sent++) {
		Symbol pos[MAX_SENTENCE_TOKENS];
		TokenSequence *toks = docTheory->getSentenceTheory(sent)->getTokenSequence();
		const SynNode *root = docTheory->getSentenceTheory(sent)->getPrimaryParse()->getRoot();
		root->getPOSSymbols(pos, MAX_SENTENCE_TOKENS);

		for (int t = 0; t < toks->getNTokens(); t++) {
			if (_topics_use_only_nouns && !LanguageSpecificFunctions::isNPtypePOStag(pos[t]))
				continue;
			Symbol sym = toks->getToken(t)->getSymbol();
			total_tokens++;
			for (int topic = 0; topic < _n_topics; topic++) {
				if (_topicWordSets[topic]->lookup(sym)) {
					_topicWordCounts[topic]++;
				}
			}
		}
	}

	int best_topic = -1;
	int most_words = 0;
	for (int topic = 0; topic < _n_topics; topic++) {
		if (_topicWordCounts[topic] > most_words) {
			most_words = _topicWordCounts[topic];
			best_topic = topic;
		}
	}

	//std::cerr << docTheory->getDocument()->getName() << " -- ";
	if (best_topic != -1 && total_tokens != 0) {
		float percentage = (float) _topicWordCounts[best_topic] / total_tokens; 
		if (percentage > .01) {
			//std::cerr << "Topic (" << _topics[best_topic] << ") with " << _topicWordCounts[best_topic] << " words, out of " << total_tokens << " (" << (float) 100.0 * _topicWordCounts[best_topic] / total_tokens << "%)\n";
			return _topics[best_topic];
		}
	}

	return Symbol();
	
}

Symbol DocEventHandler::assignTopic(TokenSequence **tokenSequences, Parse **parses,
									   int start_index, int max) 
{
	if (!_topics_initialized) 
		initializeDocumentTopics();

	if (_n_topics == 0)
		return Symbol();

	for (int i = 0; i < _n_topics; i++) {
		_topicWordCounts[i] = 0;
	}
	int total_tokens = 0;
	int index = start_index;
	while (index < max) {
		Symbol pos[MAX_SENTENCE_TOKENS];
		TokenSequence *toks = tokenSequences[index];
		const SynNode *root = parses[index]->getRoot();
		root->getPOSSymbols(pos, MAX_SENTENCE_TOKENS);
		if (toks->getSentenceNumber() == 0 && index != start_index)
			break;		
		
		for (int t = 0; t < toks->getNTokens(); t++) {
			if (_topics_use_only_nouns && !LanguageSpecificFunctions::isNPtypePOStag(pos[t]))
				continue;
			Symbol sym = toks->getToken(t)->getSymbol();
			total_tokens++;
			for (int topic = 0; topic < _n_topics; topic++) {
				if (_topicWordSets[topic]->lookup(sym)) {
					_topicWordCounts[topic]++;
				}
			}
		}
		index++;
	}

	int best_topic = -1;
	int most_words = 0;
	for (int topic = 0; topic < _n_topics; topic++) {
		if (_topicWordCounts[topic] > most_words) {
			most_words = _topicWordCounts[topic];
			best_topic = topic;
		}
	}

	if (best_topic != -1 && total_tokens != 0) {
		float percentage = (float) _topicWordCounts[best_topic] / total_tokens; 
		if (percentage > .01) {
			//std::cerr << "Topic (" << _topics[best_topic] << ") with " << _topicWordCounts[best_topic] << " words, out of " << total_tokens << " (" << (float) 100.0 * _topicWordCounts[best_topic] / total_tokens << "%)\n";
			return _topics[best_topic];
		}
	}

	return Symbol();
	
}


void DocEventHandler::doDocumentLevelEventLinking(DocTheory* docTheory, int n_vmentions) 
{
	// get link scores
  	_statEventLinker->getLinkScores(docTheory->getEntitySet(),
		_allVMentions, n_vmentions, _linkScores);
	int i;
	int event_index = 0;
	for (i = 0; i < n_vmentions; i++) {
		if (_allVMentions[i] == 0)
			continue;
        _events.push_back((Event*) 0);
		_events[event_index] = _new Event(event_index);
		_events[event_index]->addEventMention(_allVMentions[i]);
		for (int j = i + 1; j < n_vmentions; j++) {
			if (_allVMentions[j] == 0)
				continue;		
			if (_linkScores[i][j] > _link_threshold) {
				_events[event_index]->addEventMention(_allVMentions[j]);
				_allVMentions[j] = 0;
			}
		}
		event_index++;
	}	

	// transfer events into an actual EventSet
	EventSet *eventSet = _new EventSet();
	for (i = 0; i < event_index; i++) {
		eventSet->takeEvent(_events[i]);
	}
	assignEventAttributes(eventSet);
	docTheory->takeEventSet(eventSet);

}


int DocEventHandler::filterEventMentionSet(DocTheory* docTheory, MentionSet *mentionSet,
										   EventMentionSet *eventMentionSet) {

	int v1, v2;
	// first filter out events that we think are low probability based on their crucial arguments
	for (v1 = 0; v1 < eventMentionSet->getNEventMentions(); v1++) {
		EventMention *v1Mention = eventMentionSet->getEventMention(v1);

		// something cannot be both an event and a mention (e.g. "the /wounded/ were taken home")
		if (!_allow_events_to_be_mentions) {
			const SynNode *node = v1Mention->getAnchorNode();
			const Mention *ment = mentionSet->getMentionByNode(node);
			while (ment == 0) {
				// have to look up head chain to potentially find mentioned headed by this node
				if (node->getParent() != 0 && node->getParent()->getHead() == node) {
					node = node->getParent();
					ment = mentionSet->getMentionByNode(node);
				} else break;
			}
			if (ment != 0 && ment->getEntityType().isRecognized()) {
				killEventMention(v1Mention, "trigger cannot also be the head of an entity");
				continue;
			}
		}

		if (_use_preexisting_event_values) {
			bool has_event_only_value = false;
			for (int valid = 0; valid < v1Mention->getNValueArgs(); valid++) {
				if (v1Mention->getNthArgValueMention(valid)->getFullType().isForEventsOnly()) {
					has_event_only_value = true;
					break;
				}
			}
			if (has_event_only_value)
				continue;
		}

		Symbol eventType = v1Mention->getEventType();

		// This set of rules fires above the "even if it's a great trigger" cutoff,
		//  since these are special annotation rules that mean the trigger-finding
		//  can be WAY off (i.e. "meetings in Atlanta" might not be a Meet event)
		if (eventType == Symbol(L"Contact.Phone-Write")) {
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Entity")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Entity)");
				continue;
			}
		} else if (eventType == Symbol(L"Contact.Meet")) {
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Entity")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Entity)");
				continue;
			}
		} else if (eventType == Symbol(L"Movement.Transport")) {
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Destination")) == 0 &&
				v1Mention->getFirstMentionForSlot(Symbol(L"Origin")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Destination or Origin)");
				continue;
			}
		} else if (eventType == Symbol(L"Transaction.Transfer-Ownership")) {
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Artifact")) == 0 &&
				v1Mention->getFirstValueForSlot(Symbol(L"Price")) == 0)
			{
				bool found_money = false;
				TokenSequence *tokens = docTheory->getSentenceTheory(v1Mention->getSentenceNumber())->getTokenSequence();
				for (int tok = 0; tok < tokens->getNTokens(); tok++) {
					if (EventUtilities::isMoneyWord(tokens->getToken(tok)->getSymbol()))
						found_money = true;
				}
				if (!found_money) {
					killEventMention(v1Mention, "missing crucial argument (Artifact or Price)");
					continue;
				}
			}
		} else if (eventType == Symbol(L"Transaction.Transfer-Money")) {
			if (v1Mention->getFirstValueForSlot(Symbol(L"Money")) == 0)
			{
				bool found_money = false;
				TokenSequence *tokens = docTheory->getSentenceTheory(v1Mention->getSentenceNumber())->getTokenSequence();
				for (int tok = 0; tok < tokens->getNTokens(); tok++) {
					if (EventUtilities::isMoneyWord(tokens->getToken(tok)->getSymbol()))
						found_money = true;
				}
				if (!found_money) {
					killEventMention(v1Mention, "missing crucial argument (Money)");
					continue;
				}
			}
		}

		bool automatically_allow_high_scoring_events = true;
        
		if (_use_correct_answers && SerifVersion::isEnglish()) {
			automatically_allow_high_scoring_events = false;
		}

		if (automatically_allow_high_scoring_events && v1Mention->getScore() > log(.5))
			continue;

		if (_documentTopic == Symbol(L"FINANCE") ||
			_documentTopic == Symbol(L"SPORTS")) 
		{
			if (eventType == Symbol(L"Conflict.Attack")) {
				killEventMention(v1Mention, "Killed due to document topic");
				continue;
			}		
		}

		
		// this was an experiment that didn't seem to help
		/*float max_arg_score = 0;
		for (int arg = 0; arg < v1Mention->getNArgs(); arg++) {
			if (v1Mention->getNthArgScore(arg) > max_arg_score)
				max_arg_score = v1Mention->getNthArgScore(arg);
		}
		if (max_arg_score > .9)
			continue;*/

		// no current rules for Acquit, Appeal, Release-Pardon, or, strangely, Attack :)
		if (eventType == Symbol(L"Personnel.Start-Position") ||
			eventType == Symbol(L"Personnel.End-Position")) 
		{
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Person")) == 0) {
				killEventMention(v1Mention, "missing crucial argument (Person)");
				continue;
			}
		} else if (eventType == Symbol(L"Personnel.Nominate")) {
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Person")) == 0) {
				killEventMention(v1Mention, "missing crucial argument (Person)");
				continue;
			}			
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Agent")) == 0) {
				killEventMention(v1Mention, "missing crucial argument (Agent)");
				continue;
			}
		} else if (eventType == Symbol(L"Personnel.Elect")) {
			if (v1Mention->getScore() < log(.7) && 
				v1Mention->getFirstMentionForSlot(Symbol(L"Person")) == 0) 
			{
				killEventMention(v1Mention, "missing crucial argument (Person)");
				continue;
			}
		} else if (eventType == Symbol(L"Life.Marry")) {
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Person")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Person)");
				continue;
			}
		} else if (eventType == Symbol(L"Life.Divorce")) {
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Person")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Person)");
				continue;
			}
		} else if (eventType == Symbol(L"Life.Injure")) {
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Victim")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Victim)");
				continue;
			}
		} else if (eventType == Symbol(L"Life.Die")) {
			/// murders in Russia?
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Victim")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Victim)");
				continue;
			}
		} else if (eventType == Symbol(L"Life.Be-Born")) {
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Person")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Person)");
				continue;
			}
		} else if (eventType == Symbol(L"Justice.Trial-Hearing")) {
			if (v1Mention->getScore() < log(.7) && 
				v1Mention->getFirstMentionForSlot(Symbol(L"Defendant")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Defendant)");
				continue;
			}
		} else if (eventType == Symbol(L"Justice.Sue")) {
			if (v1Mention->getScore() < log(.7) && 
				v1Mention->getFirstMentionForSlot(Symbol(L"Plaintiff")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Plaintiff)");
				continue;
			}
		} else if (eventType == Symbol(L"Justice.Sentence")) {
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Defendant")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Defendant)");
				continue;
			}
		} else if (eventType == Symbol(L"Justice.Fine")) {
			if (v1Mention->getFirstValueForSlot(Symbol(L"Money")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Money)");
				continue;
			}
		} else if (eventType == Symbol(L"Justice.Execute")) {
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Person")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Person)");
				continue;
			}
		} else if (eventType == Symbol(L"Justice.Convict")) {
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Defendant")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Defendant)");
				continue;
			}
		} else if (eventType == Symbol(L"Justice.Charge-Indict")) {
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Defendant")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Defendant)");
				continue;
			}
		} else if (eventType == Symbol(L"Justice.Arrest-Jail")) {
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Person")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Person)");
				continue;
			}
		} else if (eventType == Symbol(L"Conflict.Demonstrate")) {
			if (v1Mention->getScore() < log(.7) && 
				v1Mention->getFirstMentionForSlot(Symbol(L"Entity")) == 0 &&
				v1Mention->getFirstMentionForSlot(Symbol(L"Place")) == 0)
			{
				killEventMention(v1Mention, "missing crucial argument (Entity or Place)");
				continue;
			}
		} else if (eventType == Symbol(L"Business.Start-Org") ||
			eventType == Symbol(L"Business.End-Org") ||
			eventType == Symbol(L"Business.Declare-Bankruptcy")) 
		{
			if (v1Mention->getFirstMentionForSlot(Symbol(L"Org")) == 0 )
			{
				killEventMention(v1Mention, "missing crucial argument (Org)");
				continue;
			}
		} 
	}
	
	for (v1 = 0; v1 < eventMentionSet->getNEventMentions(); v1++) {
		EventMention *v1Mention = eventMentionSet->getEventMention(v1);
		if (v1Mention->getEventType() == ignoreSym) continue;
		
		for (v2 = v1 + 1; v2 < eventMentionSet->getNEventMentions(); v2++) {
			EventMention *v2Mention = eventMentionSet->getEventMention(v2);
			if (v2Mention->getEventType() == ignoreSym) continue;

			if (v1Mention->getAnchorNode() == v2Mention->getAnchorNode() &&
				v1Mention->getEventType() == v2Mention->getEventType()) 
			{
				// 1) Same trigger, same type (e.g. from two different models)

				if (!_allowMultipleEventsOfSameTypeWithSameAnchor) {
					if (v2Mention->getScore() >= v1Mention->getScore()) {
						mergeEventMentions(v2Mention, v1Mention);
						break;
					} else mergeEventMentions(v1Mention, v2Mention);
				}

			} else if (v1Mention->getAnchorNode() == v2Mention->getAnchorNode()) {

				// 2) Same trigger, different type 

				// for now, choose the one whose type was more likely -- though
				//  presumably we can do better

				// tried using a score that backed off to a prior
				//  (so that it would prefer more common types)
				// but that actually backfired, for instance picking Life.Die over
				// Justice.Execute for words like "execution"

				//Only actually kill mentions if set to do so in paramFiles
				if (!_allowMultipleEventsOfDifferentTypeWithSameAnchor){
					if (v2Mention->getScore() >= v1Mention->getScore()) {
						killEventMention(v1Mention, "two types for same trigger");
						break;
					} else killEventMention(v2Mention, "two types for same trigger");
				}

			} else if (v1Mention->getEventType() == v2Mention->getEventType()) {

				// 3) Same sentence, same type (e.g. from two different models)

				if (!_allowMultipleEventsOfSameTypeWithSameAnchor) {
					// adjacent
					if ((v1Mention->getAnchorNode()->getStartToken() ==
						 v2Mention->getAnchorNode()->getStartToken() - 1) ||
						(v2Mention->getAnchorNode()->getStartToken() ==
						 v1Mention->getAnchorNode()->getStartToken() - 1))
					{
						if (v2Mention->getScore() >= v1Mention->getScore()) {
							mergeEventMentions(v2Mention, v1Mention);
							break;
						} else mergeEventMentions(v1Mention, v2Mention);

					} else transferArgsBetweenConnectedEvents(docTheory, v1Mention, v2Mention);
				}
			} else transferArgsBetweenConnectedEvents(docTheory, v1Mention, v2Mention);
		}
	}

	int n_vmentions = 0;
	for (int v = 0; v < eventMentionSet->getNEventMentions(); v++) {
		if (eventMentionSet->getEventMention(v)->getEventType() != ignoreSym)
			n_vmentions++;
	}

	return n_vmentions;

}

void DocEventHandler::transferArgsBetweenConnectedEvents(DocTheory *docTheory, EventMention *vm1, EventMention *vm2) {

	//#ifdef TRANSFER_ARGS_BETWEEN_EVENTS
	if (SerifVersion::isEnglish() || SerifVersion::isChinese()) {
		PropositionSet *propSet = docTheory->getSentenceTheory(vm1->getSentenceNumber())->getPropositionSet();
		MentionSet *mentionSet = docTheory->getSentenceTheory(vm1->getSentenceNumber())->getMentionSet();
		Symbol ngram[4];
		for (int p = 0; p < propSet->getNPropositions(); p++) {
			const Proposition *prop = propSet->getProposition(p);
			Symbol role1 = Symbol();
			Symbol role2 = Symbol();
			if (prop == vm1->getAnchorProp())
				role1 = Symbol(L"trigger");
			if (prop == vm2->getAnchorProp())
				role2 = Symbol(L"trigger");
			for (int a = 0; a < prop->getNArgs(); a++) {
				Argument *arg = prop->getArg(a);
				if (arg->getType() == Argument::PROPOSITION_ARG) {
					if (arg->getProposition() == vm1->getAnchorProp())
						role1 = arg->getRoleSym();
					if (arg->getProposition() == vm2->getAnchorProp())
						role2 = arg->getRoleSym();
				} else if (arg->getType() == Argument::MENTION_ARG) {
					const SynNode *node = arg->getMention(mentionSet)->getNode();
					if (node->getHeadPreterm() == vm1->getAnchorNode()->getHeadPreterm())
						role1 = arg->getRoleSym();
					if (node->getHeadPreterm() == vm2->getAnchorNode()->getHeadPreterm())
						role2 = arg->getRoleSym();
				}
			}
			if (!role1.is_null() && !role2.is_null()) {
				if (role1 == Symbol(L"when")) {
					transferArgs(vm2, vm1, Symbol(L"Time-Within"));
				} 
				if (role2 == Symbol(L"when")) {
					transferArgs(vm1, vm2, Symbol(L"Time-Within"));
				} 
				if (role1 == Symbol(L"where")) {
					transferArgs(vm2, vm1, Symbol(L"Place"));
				} 
				if (role2 == Symbol(L"where")) {
					transferArgs(vm1, vm2, Symbol(L"Place"));
				} 

				if (vm1->getEventType() == Symbol(L"Life.Die"))
				{
					if (vm2->getEventType() == Symbol(L"Conflict.Attack")) {
						transferArgs(vm1, vm2, Symbol(L"Place"));
						transferArgs(vm1, vm2, Symbol(L"Time-Within"));					
						transferArgs(vm2, vm1, Symbol(L"Place"));
						transferArgs(vm2, vm1, Symbol(L"Time-Within"));
					} else if (vm2->getEventType() == Symbol(L"Life.Injure") &&
						role1 == Symbol(L"<member>") &&
						role2 == Symbol(L"<member>")) 
					{
						transferArgs(vm1, vm2, Symbol(L"Place"));
						transferArgs(vm1, vm2, Symbol(L"Time-Within"));					
						transferArgs(vm2, vm1, Symbol(L"Place"));
						transferArgs(vm2, vm1, Symbol(L"Time-Within"));
					} 

				} else if (vm1->getEventType() == Symbol(L"Life.Injure")) {
					if (vm2->getEventType() == Symbol(L"Conflict.Attack")) {
						transferArgs(vm1, vm2, Symbol(L"Place"));
						transferArgs(vm1, vm2, Symbol(L"Time-Within"));					
						transferArgs(vm2, vm1, Symbol(L"Place"));
						transferArgs(vm2, vm1, Symbol(L"Time-Within"));
					} else if (vm2->getEventType() == Symbol(L"Life.Die") &&
						role1 == Symbol(L"<member>") &&
						role2 == Symbol(L"<member>")) 
					{
						transferArgs(vm1, vm2, Symbol(L"Place"));
						transferArgs(vm1, vm2, Symbol(L"Time-Within"));					
						transferArgs(vm2, vm1, Symbol(L"Place"));
						transferArgs(vm2, vm1, Symbol(L"Time-Within"));
					} 

				} else if (vm1->getEventType() == Symbol(L"Conflict.Attack")) {
					if (vm2->getEventType() == Symbol(L"Life.Die") ||
						vm2->getEventType() == Symbol(L"Life.Injure"))
					{
						transferArgs(vm1, vm2, Symbol(L"Place"));
						transferArgs(vm1, vm2, Symbol(L"Time-Within"));					
						transferArgs(vm2, vm1, Symbol(L"Place"));
						transferArgs(vm2, vm1, Symbol(L"Time-Within"));
					}
				} 
			}
		}
	}
	//#endif
}

void DocEventHandler::transferArgs(EventMention *fromVM, EventMention *toVM,
														 Symbol role) 
{
	const Mention *ment = fromVM->getFirstMentionForSlot(role);
	if (ment != 0 && toVM->getFirstMentionForSlot(role) == 0) {
		toVM->addArgument(role, ment, 0);
	}
	const ValueMention *valueMent = fromVM->getFirstValueForSlot(role);
	if (valueMent != 0 && toVM->getFirstValueForSlot(role) == 0) {
		toVM->addValueArgument(role, valueMent, 0);
	}
}

void DocEventHandler::mergeEventMentions(EventMention *keepMent, EventMention *elimMent) {
	int i, j;
	if (DEBUG) {
		_debugStream << L"Merging " << elimMent->toString() << L"  into  \n";
		_debugStream << keepMent->toString() << L"\n";
	}
	elimMent->setEventType(ignoreSym);
	for (i = 0; i < elimMent->getNArgs(); i++) {
		const Mention *argMent = elimMent->getNthArgMention(i);
		Symbol argRole = elimMent->getNthArgRole(i);
		float argScore = elimMent->getNthArgScore(i);
		bool found = false;
		for (int j = 0; j < keepMent->getNArgs(); j++) {
			if (keepMent->getNthArgRole(j) == argRole &&
				keepMent->getNthArgMention(j) == argMent) 
			{
				found = true;
				break;
			}
		}
		if (!found) {
			// assuming we should keep the real scores here
			keepMent->addArgument(argRole, argMent, argScore);
		}
	}	
	for (i = 0; i < elimMent->getNValueArgs(); i++) {
		const ValueMention *argMent = elimMent->getNthArgValueMention(i);
		Symbol argRole = elimMent->getNthArgValueRole(i);
		float argScore = elimMent->getNthArgValueScore(i);
		bool found = false;
		for (j = 0; j < keepMent->getNValueArgs(); j++) {
			if (keepMent->getNthArgValueRole(j) == argRole &&
				keepMent->getNthArgValueMention(j) == argMent) 
			{
				found = true;
				break;
			}
		}
		if (!found) {
			// believe we should keep the real scores here
			keepMent->addValueArgument(argRole, argMent, argScore);
		}
	}	
}

void DocEventHandler::killEventMention(EventMention *vm, const char *reason) {
	if (DEBUG) {
		const SynNode *root = vm->getAnchorNode();
		while (root->getParent() != 0)
			root = root->getParent();
		_debugStream << L"Killed (" << reason << L"): " << root->toTextString() << L"\n";
		_debugStream << vm->toString() << L"\n";
		_debugStream << L"Event mention score: " << exp(vm->getScore()) << L"\n";
	}
	vm->setEventType(ignoreSym);
}

void DocEventHandler::doSentenceLevelEventLinking(DocTheory* docTheory) {
	EntitySet *entitySet = docTheory->getEntitySet();

	EventSet *eventSet = _new EventSet();
	for (int event_sentno = 0; event_sentno < docTheory->getNSentences(); event_sentno++) {
		const EventMentionSet *eventMentionSet 
			= docTheory->getSentenceTheory(event_sentno)->getEventMentionSet();
        const PropositionSet *propSet
			= docTheory->getSentenceTheory(event_sentno)->getPropositionSet();

		_eventLinker->linkEvents(eventMentionSet, eventSet, entitySet, propSet,
			_correctAnswers,
			event_sentno, docTheory->getDocument()->getName());
	}

	for (int i = 0; i < eventSet->getNEvents(); i++) {
		Event *e = eventSet->getEvent(i);
		e->consolidateEERelations(entitySet);
	}
	assignEventAttributes(eventSet);
	docTheory->takeEventSet(eventSet);
}

void DocEventHandler::findEventMentions(DocTheory* docTheory) {

	for (int sent = 0; sent < docTheory->getNSentences(); sent++) {
#ifdef SERIF_SHOW_PROGRESS
		std::cout << "Finding event mentions in sentence " << sent
			 << "/" << docTheory->getNSentences()
			 << "...";
#endif
		SentenceTheory *sTheory = docTheory->getSentenceTheory(sent);
		EventMentionSet *vmSet = 0;
		if (_use_correct_events) {
			if (_use_correct_answers) {
				vmSet = _correctAnswers->correctEventTheory(sTheory->getTokenSequence(),
					sTheory->getValueMentionSet(),
					sTheory->getPrimaryParse(),	sTheory->getMentionSet(),
					sTheory->getPropositionSet(), docTheory->getDocument()->getName());
			} else {
				throw UnexpectedInputException("DocEventHandler::findEventMentions",
						"Attempting to use correct events, but use_correct_answers is false");
			}
		} else {
			_eventMentionFinder->resetForNewSentence(docTheory, sent);
			vmSet= _eventMentionFinder->getEventTheory(sTheory->getTokenSequence(),
				sTheory->getValueMentionSet(),
				sTheory->getPrimaryParse(), sTheory->getMentionSet(),
				sTheory->getPropositionSet());
			vmSet = _eventMentionFinder->filterEventTheory(docTheory, vmSet);
		}
		sTheory->adoptSubtheory(SentenceTheory::EVENT_SUBTHEORY, vmSet);
#ifdef SERIF_SHOW_PROGRESS
		std::cout << "\r                                                                        \r";
#endif
	}


}

void DocEventHandler::assignEventAttributes(EventSet *eventSet) {
	for (int i = 0; i < eventSet->getNEvents(); i++) {
		Event *e = eventSet->getEvent(i);
		e->setGenericityFromMentions();
		e->setTenseFromMentions();
		e->setPolarityFromMentions();
		e->setModalityFromMentions();
	}
}

Symbol DocEventHandler::getBaseType(Symbol fullType) {
	std::wstring str = fullType.to_string();
	size_t index = str.find(L".");
	return Symbol(str.substr(0, index).c_str());
}
