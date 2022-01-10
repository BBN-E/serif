// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/EventFinder.h"
#include "Generic/events/patterns/DeprecatedPatternEventFinder.h"
#include "Generic/events/PatternEventFinder.h"
#include "Generic/events/stat/StatEventFinder.h"
#include "Generic/events/EventUtilities.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/Parse.h"
#include "Generic/events/patterns/SurfaceLevelSentence.h"
#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

UTF8OutputStream EventFinder::_debugStream;
bool EventFinder::DEBUG = false;

const int initialTableSize = 100;
static const Symbol _symbol_source = Symbol(L"SOURCE");
static const Symbol _symbol_target = Symbol(L"TARGET");


EventFinder::EventFinder(Symbol scope)
{
	//std::cout << "Making a new EventFinder in scope " << scope.to_debug_string() << " \n";
	_allow_mention_set_changes = false;
	
	_deprecatedPatternEventFinder = 0;
	_patternEventFinder = 0;
	_statEventFinder = 0;

	std::string buffer = ParamReader::getParam("event_debug");
	DEBUG = (!buffer.empty());
	if (DEBUG) {
		std::string event_debug_fn = buffer.c_str();
		event_debug_fn += scope.to_debug_string();
		_debugStream.open(event_debug_fn.c_str());
		std::cout << "*event_debug on " << event_debug_fn << " ***\n";
	}

	std::string parameter = ParamReader::getParam("event_model_type");
	if (parameter == "PATTERNS") {
		_patternEventFinder = _new PatternEventFinder();
	} else if (parameter == "STAT") {
		_statEventFinder = _new StatEventFinder();
	} else if (parameter == "NONE") {
		// nothing
	} else if (parameter == "BOTH") {
		_patternEventFinder = _new PatternEventFinder();
		_statEventFinder = _new StatEventFinder();
	} else {
		std::string error = "Parameter 'event_model_type' must be set to 'PATTERNS', 'STAT', 'BOTH' or 'NONE'";
		throw UnexpectedInputException("EventFinder::EventFinder()", error.c_str());
	}
	std::string ext_events_file = ParamReader::getParam("external_events_file");
	_add_external_events = (!ext_events_file.empty());
	if (_add_external_events) {
		//_external_events = _new ExternalEventsMap();
		loadExternalEvents(ext_events_file);
	}
	
	_use_preexisting_event_values = ParamReader::isParamTrue("use_preexisting_event_only_values");

	// only allow use_preexisting_event_only_values to be true if we are finding some events somehow
	if (!_deprecatedPatternEventFinder && !_patternEventFinder && !_statEventFinder)
		_use_preexisting_event_values = false;

	if (_use_preexisting_event_values) {
		std::string event_values = ParamReader::getRequiredParam("legal_event_value_attachments");
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(event_values.c_str()));
		UTF8InputStream& in(*in_scoped_ptr);
		Sexp *sexp = _new Sexp(in);
		in.close();
		_n_event_value_types = sexp->getNumChildren();
		if (_n_event_value_types == 0) {
			throw UnexpectedInputException("EventFinder::EventFinder()", 
				"legal_event_value_attachments sexp must have at least one child");
		}
		_evType = _new Symbol[_n_event_value_types];
		_evValidRole = _new Symbol[_n_event_value_types];
		_evValidEventTypes = _new Symbol *[_n_event_value_types];
		for (int val = 0; val < _n_event_value_types; val++) {
			Sexp *valSexp = sexp->getNthChild(val);
			if (valSexp->getNumChildren() != 3 || !valSexp->getFirstChild()->isAtom() ||
				!valSexp->getSecondChild()->isAtom() || !valSexp->getThirdChild()->isList()) 
			{
				char c[500];
				sprintf(c, "%s not a valid legal_event_value_attachments sexp", valSexp->to_debug_string().c_str());
				throw UnexpectedInputException("EventFinder::EventFinder()", c);
			}	    
			_evType[val] = valSexp->getFirstChild()->getValue();
			_evValidRole[val] = valSexp->getSecondChild()->getValue();
			Sexp *listSexp = valSexp->getThirdChild();
			int ntypes = listSexp->getNumChildren();
			_evValidEventTypes[val] = _new Symbol[ntypes+1];
			for (int etype = 0; etype < ntypes; etype++) {
				_evValidEventTypes[val][etype] = listSexp->getNthChild(etype)->getValue();
			}
			_evValidEventTypes[val][ntypes] = Symbol();
		}
		delete sexp;
	}


	_event_filters = ParamReader::getStringVectorParam("event_filters");
	BOOST_FOREACH(std::string eventFilter, _event_filters) {
		if (!(eventFilter == "none" || eventFilter == "dtra-communicate" || eventFilter == "dtra-knowsdomain")) {
			std::stringstream err;
			err << "Parameter event_filters must be set to 'none', 'dtra-communicate' or 'dtra-knowsdomain'";
			throw UnexpectedInputException("EventFinder::EventFinder()", err.str().c_str());
		}
	}
}

EventFinder::~EventFinder() {
	delete _deprecatedPatternEventFinder;
	delete _patternEventFinder;
	delete _statEventFinder;
	if (_use_preexisting_event_values) {
		delete[] _evType;
		delete[] _evValidRole;
		for (int i = 0; i < _n_event_value_types; i++) {
            delete[] _evValidEventTypes[i];
		}
		delete[] _evValidEventTypes;
	}
	if (_add_external_events){
		// need to delete a std::map of <string, vector<struct> >
		//std::cout << "called to delete the external events map .. figure out how ...\n";
	}
}

void EventFinder::resetForNewSentence(DocTheory *docTheory, int sentence_num)
{
	_doc_theory = docTheory;
	if (_deprecatedPatternEventFinder) {
		_deprecatedPatternEventFinder->resetForNewSentence(docTheory, sentence_num);
	} 
	
	if (_patternEventFinder) {
		_patternEventFinder->resetForNewSentence(docTheory, sentence_num);
	} 
	
	if (_statEventFinder) {
		_statEventFinder->resetForNewSentence(docTheory, sentence_num);
	}

	if (sentence_num == 0) {
		if (DEBUG) {
			_debugStream << L"*** NEW DOCUMENT:" << _doc_theory->getDocument()->getName().to_string() << L"***\n";
		}
	}
}

const DTTagSet* EventFinder::getEventTypeTagSet() {
	if (_statEventFinder)
		return _statEventFinder->getEventTypeTagSet();
	else 
		return 0;
}

const DTTagSet* EventFinder::getArgumentTypeTagSet() {
	if (_statEventFinder)
		return _statEventFinder->getArgumentTypeTagSet();
	else 
		return 0;
}

void EventFinder::allowMentionSetChanges() { 
	if (_deprecatedPatternEventFinder)
		_deprecatedPatternEventFinder->allowMentionSetChanges();
	_allow_mention_set_changes = true;
}
void EventFinder::disallowMentionSetChanges() { 
	if (_deprecatedPatternEventFinder)
		_deprecatedPatternEventFinder->disallowMentionSetChanges();	
	_allow_mention_set_changes = false;
}


EventMentionSet *EventFinder::getEventTheory(const TokenSequence *tokens,
								   ValueMentionSet *valueMentionSet,
			                       Parse *parse,
			                       MentionSet *mentionSet,
			                       PropositionSet *propSet)

{
	// don't bother finding events if we didn't parse
	if (parse->isDefaultParse())
		return _new EventMentionSet(parse);

	if (!_deprecatedPatternEventFinder && !_patternEventFinder && !_statEventFinder)
		return _new EventMentionSet(parse);

	if (_allow_mention_set_changes) {
		// we're just in it for the mention set changes
		if (_deprecatedPatternEventFinder) {
			EventMentionSet *patternSet = _deprecatedPatternEventFinder->getEventTheory(parse, mentionSet, valueMentionSet, propSet);
			delete patternSet;
		}
		return _new EventMentionSet(parse);
	} 

	if (DEBUG) {
		SurfaceLevelSentence surfaceSentence(parse->getRoot(), mentionSet);

		_debugStream << L"*** NEW SENTENCE ***\n";
		_debugStream << surfaceSentence.toString() << L"\n";
		for (int i = 0; i < propSet->getNPropositions(); i++) {
			propSet->getProposition(i)->dump(_debugStream, 0);
			_debugStream << L"\n";
		}
		_debugStream << parse->getRoot()->toTextString() << L"\n";
		for (int j = 0; j < mentionSet->getNMentions(); j++) {
			Mention *ment = mentionSet->getMention(j);
			if (ment->getEntityType().isRecognized()) {
				_debugStream << ment->getNode()->toTextString() << ": "
							 << ment->getEntityType().getName().to_string()
							 << "\n";
			}
		}
		_debugStream << parse->getRoot()->toPrettyParse(0) << L"\n";

	}

	EventMentionSet *finalSet = _new EventMentionSet(parse);
	if (_deprecatedPatternEventFinder) {
		EventMentionSet *patternSet = _deprecatedPatternEventFinder->getEventTheory(parse, mentionSet, valueMentionSet, propSet);
		finalSet->takeEventMentions(patternSet);
		delete patternSet;
	} 
	if (_patternEventFinder) {		
		// Info is stored in docTheory, passed in by resetForNewSentence(). 
		// If that's not there we're screwed anyway, since we need it for pattern matching.
		EventMentionSet *patternSet = _patternEventFinder->getEventTheory(parse);
		finalSet->takeEventMentions(patternSet);
		delete patternSet;
	} 
	if (_statEventFinder) {
		EventMentionSet *statSet = _statEventFinder->getEventTheory(tokens, valueMentionSet, parse, mentionSet, propSet);
		finalSet->takeEventMentions(statSet);
		delete statSet;
	} 
	
	if (_use_preexisting_event_values) {	
		addFakedMentions(finalSet, tokens, valueMentionSet, parse, propSet);
	}
	if (_add_external_events) {	
		EventMentionSet *plusSet = addExternalEvents(finalSet, tokens, parse, mentionSet);
		finalSet->takeEventMentions(plusSet);
		delete plusSet;
	}
	return finalSet;
}

void EventFinder::printEventMentionSet(EventMentionSet *vmSet, const TokenSequence *tokens, Parse *parse) {
	std::vector<std::wstring> outBufLines;

        for(int emIndex=0; emIndex<vmSet->getNEventMentions(); emIndex++) {
                EventMention *em = vmSet->getEventMention(emIndex);
                outBufLines.push_back(L"<event_mention eventType=\"" + std::wstring(em->getEventType().to_string()) + L"\">");

                int anchorStartOffset = tokens->getToken( em->getAnchorNode()->getStartToken() )->getStartEDTOffset().value();
                int anchorEndOffset = tokens->getToken( em->getAnchorNode()->getEndToken() )->getEndEDTOffset().value();
                outBufLines.push_back(L"<anchor offset=\"" + boost::lexical_cast<std::wstring>(anchorStartOffset) + 
			L"," + boost::lexical_cast<std::wstring>(anchorEndOffset) + 
			L"\">" + std::wstring(em->getAnchorNode()->getHeadWord().to_string()) + L"</anchor>");

                for(int emArgIndex=0; emArgIndex<em->getNArgs(); emArgIndex++) {
                        const Mention* m = em->getNthArgMention(emArgIndex);

                        int headStartOffset = tokens->getToken( m->getHead()->getStartToken() )->getStartEDTOffset().value();
                        int headEndOffset = tokens->getToken( m->getHead()->getEndToken() )->getEndEDTOffset().value();
                        int nodeStartOffset = tokens->getToken( m->getNode()->getStartToken() )->getStartEDTOffset().value();
                        int nodeEndOffset = tokens->getToken( m->getNode()->getEndToken() )->getEndEDTOffset().value();

                        outBufLines.push_back(L"<mention_arg role=\"" + std::wstring(em->getNthArgRole(emArgIndex).to_string()) + 
				L":" + boost::lexical_cast<std::wstring>(em->getNthArgScore(emArgIndex)) + 
				L"\" head=\"" + std::wstring(m->getHead()->getHeadWord().to_string()) + 
				L"\" headOffset=\"" + boost::lexical_cast<std::wstring>(headStartOffset) + 
				L"," + boost::lexical_cast<std::wstring>(headEndOffset) + 
				L"\" extentOffset=\"" + boost::lexical_cast<std::wstring>(nodeStartOffset) + 
				L"," + boost::lexical_cast<std::wstring>(nodeEndOffset) + 
				L"\">" + m->getNode()->toTextString() + L"</mention_arg>");
                }

                for(int emArgIndex=0; emArgIndex<em->getNValueArgs(); emArgIndex++) {
                        const ValueMention* m = em->getNthArgValueMention(emArgIndex);

                        int headStartOffset = tokens->getToken( m->getStartToken() )->getStartEDTOffset().value();
                        int headEndOffset = tokens->getToken( m->getEndToken() )->getEndEDTOffset().value();
                        int nodeStartOffset = tokens->getToken( m->getStartToken() )->getStartEDTOffset().value();
                        int nodeEndOffset = tokens->getToken( m->getEndToken() )->getEndEDTOffset().value();

                        outBufLines.push_back(L"<value_arg role=\"" + std::wstring(em->getNthArgValueRole(emArgIndex).to_string()) + 
				L":" + boost::lexical_cast<std::wstring>(em->getNthArgValueScore(emArgIndex)) + 
				L"\" head=\"" + std::wstring(parse->getRoot()->getNthTerminal(m->getEndToken())->getHeadWord().to_string()) + 
				L"\" headOffset=\"" + boost::lexical_cast<std::wstring>(headStartOffset) + 
				L"," + boost::lexical_cast<std::wstring>(headEndOffset) + 
				L"\" extentOffset=\"" + boost::lexical_cast<std::wstring>(nodeStartOffset) + 
				L"," + boost::lexical_cast<std::wstring>(nodeEndOffset) + 
				L"\">" + m->toCasedTextString(tokens) + L"</value_arg>");
                }

                outBufLines.push_back(L"</event_mention>");
        }

	for(unsigned i=0; i<outBufLines.size(); i++) {
		std::cout << UnicodeUtil::toUTF8StdString(outBufLines[i]) << std::endl;
	}
}



EventMentionSet* EventFinder::filterEventTheory(DocTheory* docTheory, EventMentionSet* eventMentionSet) {
	if (_event_filters.size() > 0) {
		// Loop over all events
		EventMentionSet* filteredSet = _new EventMentionSet(eventMentionSet->getParse());
		for (int e = 0; e < eventMentionSet->getNEventMentions(); e++) {
			EventMention* em = eventMentionSet->getEventMention(e);
			BOOST_FOREACH(std::string eventFilter, _event_filters) {
				// Only filter if this event still exists after previous filters
				if (em != NULL) {
					// Check which filter was specified
					if (eventFilter == "dtra-communicate") {
						// This filter only affects Communicate events; all others are kept
						if (em->getEventType() == Symbol(L"Communicate")) {
							// Make sure that Agents aren't Addressees
							const Mention* agent = NULL;
							const Mention* addressee = NULL;
							for (int a = 0; a < em->getNArgs(); a++) {
								Symbol role = em->getNthArgRole(a);
								if (role == Symbol(L"Agent"))
									agent = em->getNthArgMention(a);
								else if (role == Symbol(L"Addressee"))
									addressee = em->getNthArgMention(a);
							}
							// Make sure this event has both args; partial Communicate events are kept
							if (agent != NULL && addressee != NULL) {
								// Make sure these aren't linked; keep it if the agent and addressee are distinct
								if (agent->getUID() == addressee->getUID()) {
									// Nuke this event
									delete em;
									em = NULL;
								}
							}
						}
					} else if (eventFilter == "dtra-knowsdomain") {
						// This filter only affects Communicate events; all others are kept
						if (em->getEventType() == Symbol(L"KnowsDomain")) {
							// Get all the argument Mentions by role
							std::vector< std::pair<int, const Mention*> > agentMentions;
							std::vector< std::pair<int, const Mention*> > domainMentions;
							for (int a = 0; a < em->getNArgs(); a++) {
								Symbol role = em->getNthArgRole(a);
								if (role == Symbol(L"Arg1"))
									agentMentions.push_back(std::make_pair(a, em->getNthArgMention(a)));
								else if (role == Symbol(L"Arg2"))
									domainMentions.push_back(std::make_pair(a, em->getNthArgMention(a)));
							}

							// Make the cross-product of event mentions if there are multiple Agent or Domain arguments
							if (agentMentions.size() > 1 || domainMentions.size() > 1) {
								// Loop over all the Agent/Domain combinations
								for (size_t a = 0; a < agentMentions.size(); a++) {
									for (size_t d = 0; d < domainMentions.size(); d++) {
										// Copy the other event attributes
										EventMention* filtered = _new EventMention(*em);
										filtered->resetArguments();

										// Add this specific expanded Agent/Domain pair
										int agentIndex = agentMentions.at(a).first;
										const Mention* agent = agentMentions.at(a).second;
										int domainIndex = domainMentions.at(d).first;
										const Mention* domain = domainMentions.at(d).second;
										filtered->addArgument(Symbol(L"Arg1"), agent, em->getNthArgScore(agentIndex));
										filtered->addArgument(Symbol(L"Arg2"), domain, em->getNthArgScore(domainIndex));
										eventMentionSet->takeEventMention(filtered);
									}
								}

								// Nuke the original event
								delete em;
								em = NULL;
							}
						}
					}
				}
			}

			// If the event survived all of the filters, keep it
			if (em != NULL)
				filteredSet->takeEventMention(em);
		}

		// Replace the event set with the filtered one
		eventMentionSet->clear();  // This clears the vector of EventMention pointers without actually deleting them
		delete eventMentionSet;  // Now when we delete the event mention set, we don't delete any event mentions
		return filteredSet;
	} else {
		// Pass through
		return eventMentionSet;
	}
}

void EventFinder::addFakedMentions(EventMentionSet *targetSet, const TokenSequence *tokens, ValueMentionSet *valueMentionSet, 
								   Parse *parse, PropositionSet *propSet) 
{
	int n_faked_mentions = 0;
	for (int valid = 0; valid < valueMentionSet->getNValueMentions(); valid++) {
		ValueMention *vm = valueMentionSet->getValueMention(valid);
		if (!vm->getFullType().isForEventsOnly())
			continue;

		// check to see if it's already attached to a mention
		if (isAlreadyAttachedToMention(vm, targetSet))
			continue;

		// OK, it's not already attached, so let's see if we know
		//  about where it can be legally attached
		int this_valtype = -1;
		for (int valtype = 0; valtype < _n_event_value_types; valtype++) {
			if (vm->getFullType().getNameSymbol() == _evType[valtype]) {
				this_valtype = valtype;
				break;
			}
		}
		if (this_valtype == -1)
			continue;

		if (attachToPreexistingMention(vm, targetSet, this_valtype))
			continue;

		// OK, no luck. Fortunately, it actually doesn't matter what word we attach 
		// this value to, since we are not scored on triggers. 
		// So let's pick the first word in the sentence.
		EventMention *em = _new EventMention(tokens->getSentenceNumber(), n_faked_mentions++);
		em->setEventType(_evValidEventTypes[this_valtype][0]);
		em->setAnchor(parse->getRoot()->getFirstTerminal(), propSet);
		em->addValueArgument(_evValidRole[this_valtype], vm, 0.5);		
		for (int temp = vm->getStartToken(); temp <= vm->getEndToken(); temp++) {
			SessionLogger::info("SERIF") << tokens->getToken(temp)->getSymbol().to_debug_string() << " ";
		}
		SessionLogger::info("SERIF") << "-- ";
		SessionLogger::info("SERIF") << "faked and attached to " << em->toDebugString() << "\n";
	}
}

bool EventFinder::isAlreadyAttachedToMention(ValueMention *val, EventMentionSet *eventMentionSet) {
	if (eventMentionSet == 0)
		return false;
	for (int event_id = 0; event_id < eventMentionSet->getNEventMentions(); event_id++) {
		EventMention *em = eventMentionSet->getEventMention(event_id);
		for (int evalid = 0; evalid < em->getNValueArgs(); evalid++) {
			if (val == em->getNthArgValueMention(evalid)) {
				SessionLogger::info("SERIF") << "already attached to " << em->toDebugString() << "\n";
				return true;
			}
		}
	}
	return false;
}

bool EventFinder::attachToPreexistingMention(ValueMention *val, EventMentionSet *eventMentionSet,
											 int valtype) {
	if (eventMentionSet == 0)
		return false;

	for (int event_id = 0; event_id < eventMentionSet->getNEventMentions(); event_id++) {
		EventMention *em = eventMentionSet->getEventMention(event_id);
		int iter = 0;
		while (!_evValidEventTypes[valtype][iter].is_null()) 
		{
			if (em->getEventType() == _evValidEventTypes[valtype][iter]) {
				em->addValueArgument(_evValidRole[valtype], val, 0.5);
				SessionLogger::info("SERIF") << "can be attached to " << em->toDebugString() << "\n";
				return true;
			}
			iter++;
		}
	}
	return false;
}


void EventFinder::setDocumentTopic(Symbol topic) { 
		if (_statEventFinder) 
			_statEventFinder->setDocumentTopic(topic); 
	}

void EventFinder::loadExternalEvents(std::string filepath) {
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(filepath.c_str());
	if (in.fail()) {
		std::string error = "Error opening external events file: " + filepath;
		throw UnrecoverableException("EventFinder::LoadExtternalEvents", error.c_str());
	}
	if (DEBUG) {
		_debugStream << L"*** opened external events file" << filepath << L"***\n";
		}
	std::wstring buf;
	std::wstring entry;
	ExternalEvents_ptr events_struct;
	int ext_ev_count = 0;
	while (!in.eof()) {
		in.getLine(buf);
		// doc_id prim_off0 prim_off1 seco_off0 seco_off1 prop_oof0 prop_off1 event
		//if (DEBUG) {
		//	_debugStream << L"external_event" << buf.c_str() << L"\n";
		//}
		size_t begin = 0;
		size_t iter = buf.find(L' ', 0);
		if (iter != std::string::npos) {
			entry = buf.substr(begin, iter - begin);
			ExternalEvent *ext_event = _new ExternalEvent;
			ext_ev_count++;
			int tab = 1;
			//std::transform(entry.begin(), entry.end(), entry.begin(), towlower);
			Symbol doc_id = Symbol(entry.c_str());
			iter += tab;
			begin = iter;
			iter = buf.find(L' ', iter);
			if (iter != std::string::npos) {
				entry = buf.substr(begin, iter - begin).c_str();
				ext_event->primary_st = boost::lexical_cast<int>(entry);
				iter += tab;
				begin = iter;
				iter = buf.find(L' ', iter);
			}
			if (iter != std::string::npos) {
				entry = buf.substr(begin, iter - begin).c_str();
				ext_event->primary_end = boost::lexical_cast<int>(entry);
				iter += tab;
				begin = iter;
				iter = buf.find(L' ', iter);
			}
			if (iter != std::string::npos) {
				entry = buf.substr(begin, iter - begin).c_str();
				ext_event->secondary_st = boost::lexical_cast<int>(entry);
				iter += tab;
				begin = iter;
				iter = buf.find(L' ', iter);
			}
			if (iter != std::string::npos) {
				entry = buf.substr(begin, iter - begin).c_str();
				ext_event->secondary_end = boost::lexical_cast<int>(entry);
				iter += tab;
				begin = iter;
				iter = buf.find(L' ', iter);
			}
			if (iter != std::string::npos) {
				entry = buf.substr(begin, iter - begin).c_str();
				ext_event->props_st = boost::lexical_cast<int>(entry);
				iter += tab;
				begin = iter;
				iter = buf.find(L' ', iter);
			}
			if (iter != std::string::npos) {
				entry = buf.substr(begin, iter - begin).c_str();
				ext_event->props_end = boost::lexical_cast<int>(entry);
				iter += tab;
				begin = iter;
			}
			entry = buf.substr(begin).c_str();
			// make the event name from this
			ext_event->event_type = Symbol(entry);
			//if (DEBUG) {
			//	_debugStream << L"  entered as" << ext_event->primary_st << L" " << ext_event->primary_end << L" : " << ext_event->secondary_st 
			//      << L" " << ext_event->secondary_end << L" " << ext_event->event_type.to_string() << L"\n";
			//}
	
			events_struct = _external_events[doc_id];
			if (events_struct == NULL) {
				events_struct = _new ExternalEvents();
			}
			events_struct->vect.push_back(ext_event);
			_external_events[doc_id] = events_struct;
		}
	}
	if (DEBUG) {
		_debugStream << L"*** closing external events file" << filepath << L".  Loaded "<< ext_ev_count << L" events***\n ";
	}
	in.close();
}
EventMentionSet * EventFinder::addExternalEvents(EventMentionSet *oldEventMentionSet, const TokenSequence *tokens, Parse *parse, MentionSet *mentionSet) {
	// if my doc_id in the hash _externalEvents
	int sent_num =  tokens->getSentenceNumber();
	Symbol doc_id = _doc_theory->getDocument()->getName();  
	std::wstring doc_id_key = doc_id.to_string();
	// some of the bolt files may have split nums appended -- non-bolt file have dots in their base name
	size_t dot_in_id = doc_id_key.rfind(L".");
	size_t bolt_in_id = doc_id_key.rfind(L"bolt");
		if (dot_in_id != std::wstring::npos && bolt_in_id != std::wstring::npos){
		doc_id_key = doc_id_key.substr(0,dot_in_id);
	}
	Symbol base_doc_id = Symbol(doc_id_key);
	ExternalEvents_ptr events_struct =_external_events[base_doc_id];
	EventMentionSet* plusSet = _new EventMentionSet(parse);
	if (events_struct == NULL ) {
		if (DEBUG) {
		_debugStream << L"add_external_events found none for base "<< doc_id_key << ", doc_id " << doc_id.to_string() << L"\n ";
		}
		return plusSet;
	}
	if (DEBUG) {
		_debugStream << L"add_external_events found base " << doc_id_key << ", doc_id " << doc_id.to_string() << L"\n ";
	}

	int n_external_mentions = 0;

		
	// loop through external_events vector for this doc
	for (std::vector<ExternalEvent_ptr>::const_iterator it = events_struct->vect.begin(); it != events_struct->vect.end(); it++){
		// if we find the Mentions, add this event to the plusSet
		ExternalEvent *const ext_event = *it;
		const CharOffset source_st_ch = CharOffset(ext_event->primary_st);
		const CharOffset source_end_ch = CharOffset(ext_event->primary_end);
		const CharOffset target_st_ch = CharOffset(ext_event->secondary_st);
		const CharOffset target_end_ch = CharOffset(ext_event->secondary_end);
		const CharOffset props_st_ch = CharOffset(ext_event->props_st);
		const CharOffset props_end_ch = CharOffset(ext_event->props_end);
		const Mention * source_ment;
		const Mention * target_ment;
		bool source = false;
		const SynNode * source_syn_node;
		const SynNode * target_syn_node;
		bool target = false;
		if (DEBUG) {
			_debugStream << L"external event source charoffs from  " << source_st_ch << L" to " << source_end_ch << L"\n ";
			_debugStream << L"external event target charoffs from  " << target_st_ch << L" to " << target_end_ch << L"\n ";
		}
		int n_mens = mentionSet->getNMentions();
		for (int i = 0; i < n_mens; i++){
			const Mention* ment = mentionSet->getMention(i);
			const SynNode * sn = ment->getNode();
			const SynNode * st_node = sn->getFirstTerminal();
			const SynNode * e_node = sn->getLastTerminal();
			int st_token_i = st_node->getStartToken();
			const CharOffset start_char = tokens->getToken(st_token_i)->getStartCharOffset();
			int e_token_i = e_node->getEndToken();
			const CharOffset end_char = tokens->getToken(e_token_i)->getEndCharOffset();

			if (start_char == source_st_ch && end_char == source_end_ch) {
				source = true;
				source_ment = ment;
				source_syn_node = sn;
			}else if (start_char == target_st_ch && end_char == target_end_ch){
				target = true;
				target_ment = ment;
				target_syn_node = sn;
			}
			if (source && target) {
				break;
			}
		} // end sentence mentions loop
		if (source && target){
			bool found_anchor = false;
			const SynNode *fallback_anchor_node = 0;
			int fallback_index = -1;
			if (DEBUG) {
				_debugStream << L"add external_events found a mentions match in sentence " << sent_num << L"\n ";
				//std::cout << "external Event found in sentence " << tokens->getSentenceNumber() << "\n";
			}
			EventMention *em = _new EventMention(sent_num, n_external_mentions++);
			Symbol ev_sym =  ext_event->event_type;
			em->setEventType(ev_sym);
			std::wstring ev_sym_str = ev_sym.to_string();

			em->addArgument(Argument::SUB_ROLE, source_ment, 1.0f);
			em->addArgument(Argument::OBJ_ROLE, target_ment, 1.0f);
			
			if (boost::starts_with(ev_sym_str, L"pos")){
				em->setPolarity(Polarity::POSITIVE);
			} else if (boost::starts_with(ev_sym_str, L"neg")){
				em->setPolarity(Polarity::NEGATIVE);
			}
			// find a prop inside the range that is NOT the node of either mention
			const PropositionSet *propSet = _doc_theory->getSentenceTheory(sent_num)->getPropositionSet();
			const SynNode *anchor_node = 0;
			
			if (propSet == 0) {
				std::cerr << "EventFinder:AddExternalEvents found no proposition set for sentence " << sent_num << " so dropping event type " 
					<< ev_sym.to_debug_string() << "\n";
				if (DEBUG) {
					_debugStream << L"cannot find any prop set for sentence " << sent_num << L"  so bailing out on external events\n";
				}
				break;
			}
			for (int i = 0; i < propSet->getNPropositions(); i++) {
				const Proposition *prop = propSet->getProposition(i); 
				if (prop->getPredHead() != 0){
					anchor_node = prop->getPredHead()->getHeadPreterm();
					if (anchor_node != source_syn_node && anchor_node != target_syn_node){
						if (fallback_anchor_node == 0){
							fallback_anchor_node = anchor_node;
							fallback_index = i;
						}
						const CharOffset prop_start_char = tokens->getToken(anchor_node->getFirstTerminal()->getStartToken())->getStartCharOffset();
						const CharOffset prop_end_char = tokens->getToken(anchor_node->getLastTerminal()->getEndToken())->getEndCharOffset();
						if (prop_start_char >= props_st_ch && prop_end_char <= props_end_ch){
							if (DEBUG) {
								_debugStream << L"found an anchor prop between " << prop_start_char << L" and  " << prop_end_char << L"\n";
							}
							found_anchor = true;
							break;
						}
					}
				}
			}// end loop over propositions
			if (found_anchor) {
				em->setAnchor(anchor_node, propSet);
			}else if (fallback_anchor_node != 0){
				em->setAnchor(fallback_anchor_node, propSet);	
				_debugStream << L"add external event used fallback anchor prop "<< fallback_index << L" among "<< propSet->getNPropositions() 
					<< L" props of sentence " << sent_num  << L"\n";
			}else {
				std::cerr << "AddExternalEvent found no anchor prop among "<< propSet->getNPropositions() << " props of sentence " << sent_num 
					<< ", so dropped external event\n";
				if (DEBUG) {
					_debugStream << L"add external event found no anchor prop among "<< propSet->getNPropositions() << L" props of sentence " 
						<< sent_num <<L", so dropped external event for sentence " << sent_num << L"\n";
				}
				break;
			}
			if (DEBUG) {
				_debugStream << L"add external event mention type " << ext_event->event_type.to_string() << L"\n ";
				_debugStream << L"add external event mention source " << source_ment->getUID()<< L" " <<source_ment->getNode()->toTextString() 
					<< L" ("<<  source_ment->getEntityType().getName().to_string() <<L")\n"; 
				_debugStream << L"add external event mention target " << target_ment->getUID()<< L" " <<target_ment->getNode()->toTextString() 
					<< L" ("<<  target_ment->getEntityType().getName().to_string() <<L")\n"; 
				// needs a UTF*Stream?
				//_debugStream << L"add external event mention " << em << L"\n ";
			}
			plusSet->takeEventMention(em);
		}// end matching an external event

	} // end loop through doc external events
	if (DEBUG) {
		_debugStream << L"end of add external_events pushing " << n_external_mentions << L" event mentions\n ";
		//printEventMentionSet(plusSet, tokens, parse);
	}
	return plusSet;
}
