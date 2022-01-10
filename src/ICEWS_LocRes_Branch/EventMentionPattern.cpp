// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/Sexp.h"
#include "ICEWS/EventMentionPattern.h"
#include "ICEWS/EventMentionSet.h"
#include "ICEWS/ActorMentionPattern.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/PatternMatcher.h"
#include <boost/algorithm/string.hpp>

namespace {
	Symbol participantSym(L"participant");
	Symbol blockParticipantSym(L"block-participant");
	Symbol eventCodeSym(L"event-code");
	Symbol blockEventCodeSym(L"block-code");
	Symbol sameActorSym(L"same-actor");
	Symbol blockSameActorSym(L"block-same-actor");
	Symbol sameAgentSym(L"same-agent");
	Symbol blockSameAgentSym(L"block-same-agent");
	Symbol sentenceMatchesSym(L"sentence-matches");
	Symbol blockSentenceMatchesSym(L"block-sentence-matches");
	Symbol documentMatchesSym(L"document-matches");
	Symbol blockDocumentMatchesSym(L"block-document-matches");
	Symbol anySym(L"ANY");
}

namespace ICEWS {

ICEWSEventMentionPattern::ICEWSEventMentionPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets)
{
	initializeFromSexp(sexp, entityLabels, wordSets);
}

bool ICEWSEventMentionPattern::initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol atom = childSexp->getValue();
	if (Pattern::initializeFromAtom(childSexp, entityLabels, wordSets)) {
		return true;
	} else {
		logFailureToInitializeFromAtom(childSexp);
		return false;
	}
	return true;
}

bool ICEWSEventMentionPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (Pattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (constraintType == participantSym) {
		if ((childSexp->getNumChildren() != 3) || (!childSexp->getNthChild(1)->isAtom()))
			throwError(childSexp, "expected to see \"(icews-event (participant <role> (icews-actor ...)))\" but got");
		Symbol role = childSexp->getNthChild(1)->getValue();
		Pattern_ptr pat = parseSexp(childSexp->getNthChild(2), entityLabels, wordSets);
		_participants[role] = pat;
	} else if (constraintType == blockParticipantSym) {
		if ((childSexp->getNumChildren() != 3) || (!childSexp->getNthChild(1)->isAtom()))
			throwError(childSexp, "expected to see \"(icews-event (block-participant <role> (icews-actor ...)))\" but got");
		Symbol role = childSexp->getNthChild(1)->getValue();
		Pattern_ptr pat = parseSexp(childSexp->getNthChild(2), entityLabels, wordSets);
		_blockParticipants[role] = pat;
	} else if (constraintType == sentenceMatchesSym) {
		if (childSexp->getNumChildren() != 2)
			throwError(childSexp, "sentence-matches should have a single child");
		_sentenceMatches.push_back(parseSexp(childSexp->getNthChild(1), entityLabels, wordSets));
	} else if (constraintType == blockSentenceMatchesSym) {
		if (childSexp->getNumChildren() != 2)
			throwError(childSexp, "block-sentence-matches should have a single child");
		_blockSentenceMatches.push_back(parseSexp(childSexp->getNthChild(1), entityLabels, wordSets));
	} else if (constraintType == documentMatchesSym) {
		if (childSexp->getNumChildren() != 2)
			throwError(childSexp, "document-matches should have a single child");
		_documentMatches.push_back(parseSexp(childSexp->getNthChild(1), entityLabels, wordSets));
	} else if (constraintType == blockDocumentMatchesSym) {
		if (childSexp->getNumChildren() != 2)
			throwError(childSexp, "block-document-matches should have a single child");
		_blockDocumentMatches.push_back(parseSexp(childSexp->getNthChild(1), entityLabels, wordSets));
	} else if (constraintType == eventCodeSym) {
		fillListOfWords(childSexp, wordSets, _eventCode); // allow for wildcards (18* etc)?
	} else if (constraintType == blockEventCodeSym) {
		fillListOfWords(childSexp, wordSets, _blockEventCode);
	} else if (constraintType == sameActorSym) {
		_sameActorSets.push_back(std::set<Symbol>());
		fillListOfSymbols(childSexp, _sameActorSets.back());
	} else if (constraintType == blockSameActorSym) {
		_blockSameActorSets.push_back(std::set<Symbol>());
		fillListOfSymbols(childSexp, _blockSameActorSets.back());
	} else if (constraintType == sameAgentSym) {
		_sameAgentSets.push_back(std::set<Symbol>());
		fillListOfSymbols(childSexp, _sameAgentSets.back());
	} else if (constraintType == blockSameAgentSym) {
		_blockSameAgentSets.push_back(std::set<Symbol>());
		fillListOfSymbols(childSexp, _blockSameAgentSets.back());
	} else {
		logFailureToInitializeFromChildSexp(childSexp);		
		return false;
	}
	return true;
}

Pattern_ptr ICEWSEventMentionPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {
	for (Symbol::HashMap<Pattern_ptr>::iterator it=_participants.begin(); it!=_participants.end(); ++it)
		replaceShortcut<ICEWSActorMentionMatchingPattern>((*it).second, refPatterns);
	for (Symbol::HashMap<Pattern_ptr>::iterator it=_blockParticipants.begin(); it!=_blockParticipants.end(); ++it)
		replaceShortcut<ICEWSActorMentionMatchingPattern>((*it).second, refPatterns);
	BOOST_FOREACH(Pattern_ptr& pat, _sentenceMatches)
		replaceShortcut<SentenceMatchingPattern>(pat, refPatterns);
	BOOST_FOREACH(Pattern_ptr& pat, _blockSentenceMatches)
		replaceShortcut<SentenceMatchingPattern>(pat, refPatterns);
	BOOST_FOREACH(Pattern_ptr& pat, _documentMatches)
		replaceShortcut<DocumentMatchingPattern>(pat, refPatterns);
	BOOST_FOREACH(Pattern_ptr& pat, _blockDocumentMatches)
		replaceShortcut<DocumentMatchingPattern>(pat, refPatterns);
	return shared_from_this();
}

void ICEWSEventMentionPattern::getReturns(PatternReturnVecSeq & output) const {
	// Note: we do not recurse into blockedParticipants or blockedSentenceMatches.
	Pattern::getReturns(output);
	for (Symbol::HashMap<Pattern_ptr>::const_iterator it=_participants.begin(); it!=_participants.end(); ++it)
		(*it).second->getReturns(output);
	BOOST_FOREACH(Pattern_ptr pat, _blockSentenceMatches)
		pat->getReturns(output);
}

void ICEWSEventMentionPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";	
	out << "EventMentionPattern:";
	if (!getID().is_null()) out << getID();
	out << std::endl;
	for (int i = 0; i < indent; i++) out << " ";	
	out << "  (dump method not implemented yet)";
}

bool ICEWSEventMentionPattern::actorsAreTheSame(ICEWSEventMention_ptr em, const std::set<Symbol> &roles) {
	ActorId::HashSet actorIds(1);
	BOOST_FOREACH(Symbol role, roles) {
		ActorMention_ptr actor = em->getParticipant(role);
		if (ProperNounActorMention_ptr pm = boost::dynamic_pointer_cast<ProperNounActorMention>(actor)) {
			actorIds.insert(pm->getActorId());
		} else if (CompositeActorMention_ptr cm = boost::dynamic_pointer_cast<CompositeActorMention>(actor)) {
			actorIds.insert(cm->getPairedActorId());
		} else {
			actorIds.insert(ActorId());
		}
	}
	return actorIds.size()==1;
}

bool ICEWSEventMentionPattern::agentsAreTheSame(ICEWSEventMention_ptr em, const std::set<Symbol> &roles) {
	AgentId::HashSet agentIds(1);
	BOOST_FOREACH(Symbol role, roles) {
		ActorMention_ptr actor = em->getParticipant(role);
		if (CompositeActorMention_ptr cm = boost::dynamic_pointer_cast<CompositeActorMention>(actor)) {
			agentIds.insert(cm->getPairedAgentId());
		} else {
			agentIds.insert(AgentId());
		}
	}
	return agentIds.size()==1;
}

PatternFeatureSet_ptr ICEWSEventMentionPattern::matchesICEWSEventMention(PatternMatcher_ptr patternMatcher, ICEWSEventMention_ptr em) {
	// Constraint: event code
	Symbol eventCode = em->getEventType()->getEventCode();
	if ((!_eventCode.empty()) && (_eventCode.find(eventCode)==_eventCode.end()))
		return PatternFeatureSet_ptr();
	if ((!_blockEventCode.empty()) && (_blockEventCode.find(eventCode)!=_blockEventCode.end()))
		return PatternFeatureSet_ptr();

	// Constraint: same actors
	typedef std::set<Symbol> SymbolSet;
	BOOST_FOREACH(SymbolSet &roles, _sameActorSets) {
		if (!actorsAreTheSame(em, roles)) 
			return PatternFeatureSet_ptr();
	}
	BOOST_FOREACH(SymbolSet &roles, _blockSameActorSets) {
		if (actorsAreTheSame(em, roles)) 
			return PatternFeatureSet_ptr();
	}

	// Constraint: same agents
	typedef std::set<Symbol> SymbolSet;
	BOOST_FOREACH(SymbolSet &roles, _sameAgentSets) {
		if (!agentsAreTheSame(em, roles)) 
			return PatternFeatureSet_ptr();
	}
	BOOST_FOREACH(SymbolSet &roles, _blockSameAgentSets) {
		if (agentsAreTheSame(em, roles)) 
			return PatternFeatureSet_ptr();
	}

	// Constraint: block participants
	for (Symbol::HashMap<Pattern_ptr>::const_iterator it=_blockParticipants.begin(); it!=_blockParticipants.end(); ++it) {
		Symbol role = (*it).first;
		boost::shared_ptr<ICEWSActorMentionMatchingPattern> actorPattern = (*it).second->castTo<ICEWSActorMentionMatchingPattern>();
		typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
		BOOST_FOREACH(ParticipantPair participantPair, em->getParticipantMap()) {
			if ((role == anySym) || (role == participantPair.first)) {
				if (actorPattern->matchesICEWSActorMention(patternMatcher, participantPair.second))
					return PatternFeatureSet_ptr();
			}
		}
	}

	// Determine the sentence that this event occurs in.
	std::set<SentenceTheory*> sentTheories;
	typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
	BOOST_FOREACH(const ParticipantPair &participantPair, em->getParticipantMap())
		sentTheories.insert(const_cast<SentenceTheory*>(participantPair.second->getSentence()));

	// Constraint: block sentence matches.
	BOOST_FOREACH(Pattern_ptr& pat, _blockSentenceMatches) {
		for (std::set<SentenceTheory*>::const_iterator it=sentTheories.begin(); it!=sentTheories.end(); ++it) {
			SentenceTheory *sentTheory = *it;
			if (pat->castTo<SentenceMatchingPattern>()->matchesSentence(patternMatcher, sentTheory))
				return PatternFeatureSet_ptr();
		}
	}

	// Constraint: block document matches.
	BOOST_FOREACH(Pattern_ptr& pat, _blockDocumentMatches) {
		if (pat->castTo<DocumentMatchingPattern>()->matchesDocument(patternMatcher))
			return PatternFeatureSet_ptr();
	}

	// Build our final result.
	PatternFeatureSet_ptr result = boost::make_shared<PatternFeatureSet>();

	// Constraint: participants.
	for (Symbol::HashMap<Pattern_ptr>::const_iterator it=_participants.begin(); it!=_participants.end(); ++it) {
		Symbol role = (*it).first;
		boost::shared_ptr<ICEWSActorMentionMatchingPattern> actorPattern = (*it).second->castTo<ICEWSActorMentionMatchingPattern>();
		bool ok = false;
		typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
		BOOST_FOREACH(ParticipantPair participantPair, em->getParticipantMap()) {
			if ((role == anySym) || (role == participantPair.first)) {
				PatternFeatureSet_ptr actorSFS = actorPattern->matchesICEWSActorMention(patternMatcher, participantPair.second);
				if (actorSFS) {
					result->addFeatures(actorSFS);
					ok = true;
					break;
				}
			}
		}
		if (!ok)
			return PatternFeatureSet_ptr();
	}

	// Do this before bound variable checking, below
	// Add a feature for the return value (if we have one)
	if (getReturn())
		result->addFeature(boost::make_shared<ICEWSEventMentionReturnPFeature>(shared_from_this(), em));
	// Add a feature for the pattern itself
	result->addFeature(boost::make_shared<ICEWSEventMentionPFeature>(shared_from_this(), em));
	
	std::map<std::wstring, Symbol> empty;
	std::map<std::wstring, Symbol> bound_variables;
	bool check = getAndCheckBoundVariables(result, patternMatcher->getDocTheory(), empty, bound_variables);
	if (!check)
		return PatternFeatureSet_ptr();

	// Constraint: sentence matches.
	// Note that we only test bound-variable consistency against the parent (not against sibling sentence/document matches)
	BOOST_FOREACH(Pattern_ptr pat, _sentenceMatches) {
		for (std::set<SentenceTheory*>::const_iterator it=sentTheories.begin(); it!=sentTheories.end(); ++it) {
			SentenceTheory *sentTheory = *it;
			std::vector<PatternFeatureSet_ptr> sentMatches = pat->castTo<SentenceMatchingPattern>()->multiMatchesSentence(patternMatcher, sentTheory);
			bool found_match = false;
			for (std::vector<PatternFeatureSet_ptr>::const_iterator iter = sentMatches.begin(); iter != sentMatches.end(); iter++) {
				std::map<std::wstring, Symbol> local_variables;
				if (getAndCheckBoundVariables(*iter, patternMatcher->getDocTheory(), bound_variables, local_variables)) {
					result->addFeatures(*iter);
					found_match = true;
					break;
				}
			}
			if (!found_match)
				return PatternFeatureSet_ptr();
		}
	}

	// Constraint: document matches.
	// Note that we only test bound-variable consistency against the parent (not against sibling sentence/document matches)
	BOOST_FOREACH(Pattern_ptr pat, _documentMatches) {
		std::vector<PatternFeatureSet_ptr> docMatches = pat->castTo<DocumentMatchingPattern>()->multiMatchesDocument(patternMatcher);
		bool found_match = false;
		for (std::vector<PatternFeatureSet_ptr>::const_iterator iter = docMatches.begin(); iter != docMatches.end(); iter++) {
			std::map<std::wstring, Symbol> local_variables;
			if (getAndCheckBoundVariables(*iter, patternMatcher->getDocTheory(), bound_variables, local_variables)) {
				result->addFeatures(*iter);
				found_match = true;
				break;
			}
		}
		if (!found_match)
			return PatternFeatureSet_ptr();
	}


	// Add a feature for the ID (if we have one)
	addID(result);
	// Initialize our score.
	result->setScore(this->getScore());
	return result;
}

std::vector<PatternFeatureSet_ptr> ICEWSEventMentionPattern::multiMatchesDocument(PatternMatcher_ptr patternMatcher, UTF8OutputStream *debug) {
	std::vector<PatternFeatureSet_ptr> allMatches;
	ICEWSEventMentionSet *eventMentions = patternMatcher->getDocTheory()->getSubtheory<ICEWSEventMentionSet>();
	if (eventMentions) {
		BOOST_FOREACH(ICEWSEventMention_ptr em, (*eventMentions)) {
			PatternFeatureSet_ptr match = matchesICEWSEventMention(patternMatcher, em);
			if (match)
				allMatches.push_back(match);
		}
	}
	return allMatches;
}

PatternFeatureSet_ptr ICEWSEventMentionPattern::matchesDocument(PatternMatcher_ptr patternMatcher, UTF8OutputStream *debug) {
	ICEWSEventMentionSet *eventMentions = patternMatcher->getDocTheory()->getSubtheory<ICEWSEventMentionSet>();
	if (eventMentions) {
		BOOST_FOREACH(ICEWSEventMention_ptr em, (*eventMentions)) {
			PatternFeatureSet_ptr match = matchesICEWSEventMention(patternMatcher, em);
			if (match) return match;
		}
	}
	return PatternFeatureSet_ptr();
}


bool ICEWSEventMentionPattern::getAndCheckBoundVariables(const PatternFeatureSet_ptr match, const DocTheory *docTheory, 
												 std::map<std::wstring, Symbol>& existing_variables, std::map<std::wstring, Symbol>& new_variables) 
{

	for (size_t f = 0; f < match->getNFeatures(); f++) {
		if (ICEWSEventMentionPFeature_ptr empf = boost::dynamic_pointer_cast<ICEWSEventMentionPFeature>(match->getFeature(f))) {
			// If the event we are matching is one we will eventually discard anyway, then don't allow this at all
			if (empf->isEventToBeDiscarded())
				return false;
		}
		if (ReturnPatternFeature_ptr rpf = boost::dynamic_pointer_cast<ReturnPatternFeature>(match->getFeature(f))) {
			std::map<std::wstring, std::wstring>::const_iterator it;
			for (it=rpf->begin(); it!=rpf->end(); ++it) {
				const std::wstring &valtype = (*it).first;
				const std::wstring &var = (*it).second;
				Symbol value;
				if (ICEWSEventMentionReturnPFeature_ptr empf = boost::dynamic_pointer_cast<ICEWSEventMentionReturnPFeature>(rpf)) {
					if (boost::iequals(valtype, L"EVENT-CODE"))
						value = empf->getEventMention()->getEventType()->getEventCode();
					else if (boost::iequals(valtype, L"SENTNO"))
						value = Symbol(boost::lexical_cast<std::wstring>(empf->getEventMention()->getIcewsSentNo(docTheory)).c_str());
					else if (boost::iequals(valtype, L"BLOCK") || boost::iequals(valtype, L"EVENT-TENSE")  || boost::iequals(valtype, L"NEW-EVENT-CODE"))
						continue;
					else {
						SessionLogger::err("ICEWS") << "Unexpected return value in ICEWSEventMentionPattern::getAndCheckBoundVariables(): " << valtype;
						continue;
					}
				} else if (ActorMentionReturnPFeature_ptr ampf = boost::dynamic_pointer_cast<ActorMentionReturnPFeature>(rpf)) {
					ProperNounActorMention_ptr pnActor= boost::dynamic_pointer_cast<ProperNounActorMention>(ampf->getActorMention());
					CompositeActorMention_ptr cActor = boost::dynamic_pointer_cast<CompositeActorMention>(ampf->getActorMention());
					if (boost::iequals(valtype, L"PAIRED-ACTOR")) {
						if (cActor)
							value = cActor->getPairedActorCode();
						else
							return false; // No paired actor.
					} else if (boost::iequals(valtype, L"PAIRED-AGENT")) {
						if (cActor)
							value = cActor->getPairedAgentCode();
						else
							return false; // No paired agent.
					} else if (boost::iequals(valtype, L"ACTOR")) {
						if (pnActor)
							value = pnActor->getActorCode();
						else if (cActor) {
							std::wostringstream strm;
							strm << (cActor->getPairedAgentCode().is_null()?L"UNKNOWN":cActor->getPairedAgentCode().to_string())
								<< " FOR " << (cActor->getPairedActorCode().is_null()?L"UNKNOWN":cActor->getPairedActorCode().to_string());
							value = Symbol(strm.str());
						} else 
							return false; // No actor
					} else  {
						SessionLogger::err("ICEWS") << "Unexpected return value in ICEWSEventMentionPattern::getBoundVariables(): " << valtype;
						continue;
					}			
				}
				
				// check against pre-existing variables
				if (existing_variables.find(var) != existing_variables.end()) {
					if (existing_variables[var] != value)
						return false;
				}
					
				// now add to new set (but also check consistency)
				if (new_variables.find(var) == new_variables.end())
					new_variables[var] = value;
				else if (new_variables[var] != value)
					return false; // value did not match.
			}
		}
	}
	return true;
}

} // end of namespace ICEWS
