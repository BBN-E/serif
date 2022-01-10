// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_AA_OBSERVATION_H
#define EVENT_AA_OBSERVATION_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/events/EventUtilities.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/EventMention.h"
#include "Generic/wordClustering/WordClusterClass.h"

#include "Generic/distributionalKnowledge/DistributionalKnowledgeClass.h"
#include "Generic/common/ParamReader.h"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
class TokenSequence;
class Parse;
class MentionSet;
class PropositionSet;
class SynNode;
class Proposition;

class EntitySet;
class EventAAObservation;
typedef boost::shared_ptr<EventAAObservation> EventAAObservation_ptr; 

class EventAAObservation : public DTObservation {
private:
	typedef std::pair<Symbol, Symbol> SymbolPair;

public:
	friend EventAAObservation_ptr boost::make_shared<EventAAObservation>();
	
	void initializeEventAAObservation(const TokenSequence *tokens, const ValueMentionSet *valueMentionSet, const Parse *parse, 
		const MentionSet *mentionSet, const PropositionSet *propSet, 
		const EventMention *vMention, const Mention* mention);
	
	void initializeEventAAObservation(const TokenSequence *tokens, const ValueMentionSet *valueMentionSet, const Parse *parse, 
		const MentionSet *mentionSet, const PropositionSet *propSet, 
		const EventMention *vMention, const ValueMention* valueMention);



	EventAAObservation(const TokenSequence *tokens, const ValueMentionSet *valueMentionSet, const Parse *parse, 
		const MentionSet *mentionSet, const PropositionSet *propSet, 
		const EventMention *vMention, const Mention* mention)
		: DTObservation(_className), 
		_triggerWC(WordClusterClass::nullCluster()),
		_aaDK(DistributionalKnowledgeClass::nullDistributionalKnowledge()) {}


	EventAAObservation(const TokenSequence *tokens, const ValueMentionSet *valueMentionSet, const Parse *parse, 
		const MentionSet *mentionSet, const PropositionSet *propSet, 
		const EventMention *vMention, const ValueMention* valueMention)
		: DTObservation(_className), 
		_triggerWC(WordClusterClass::nullCluster()),
		_aaDK(DistributionalKnowledgeClass::nullDistributionalKnowledge()) {}

	EventAAObservation() : DTObservation(_className), 
		_triggerWC(WordClusterClass::nullCluster()),
		_aaDK(DistributionalKnowledgeClass::nullDistributionalKnowledge()) {}

	~EventAAObservation() {}

	virtual DTObservation *makeCopy();
	
	//CandidateType is reset to try classifing a potential argument with a different entity type (e.g. an Organization as a Person)
	void setCandidateType(Symbol t) { _candidateType = t; }

	//Methods for accessing core sentence, event, and argument information
	const Mention* getCandidateArgumentMention() const {return _candidateArgument;};
	const ValueMention* getCandidateValueMention()const {return _candidateValue;};
	const TokenSequence* getTokenSequence() const {return _tokens;};
	Symbol getCandidateType() const { return _candidateType; }
	Symbol getEventType() { return _vMention->getEventType(); }
	Symbol getReducedEventType() { return EventUtilities::getReduced2005EventType(_vMention->getEventType()); }
	bool isValue() const { return (_candidateValue != 0); }
	bool isRegularMention() const { return (_candidateValue == 0); }
	bool isName() const { return (!isValue() && _candidateArgument->getMentionType() == Mention::NAME); }


	//Methods for accessing derived information used in features
	// "his trial" --> trial
	Symbol getStemmedTrigger() { return _stemmedTrigger; }

	// "his trial" --> <poss>
	Symbol getCandidateRoleInTriggerProp() { return _roleInTriggerProp; }

	// "he won the election" --> <sub>
	Symbol getCandidateRoleInCP() { return _candidateRoleInCP; }

	// "he won the election" --> <obj>
	Symbol getEventRoleInCP() { return _eventRoleInCP; }

	// "he won the election" --> win
	Symbol getStemmedCPPredicate() { return _stemmedCPPredicate; }

	// "the people were moved" --> "PER were moved"
	// "he said by telephone from Belgium" --> "telephone from GPE"
	Symbol getConnectingString() { return _connectingString; }
	Symbol getStemmedConnectingString() { return _stemmedConnectingString; }
	Symbol getPOSConnectingString() { return _posConnectingString; }
	Symbol getAbbrevConnectingString() { return _abbrevConnectingString; }
	Symbol getConnectingCandParsePath() { return _connectingCandParsePath; }
	Symbol getConnectingTriggerParsePath() { return _connectingTriggerParsePath; }
	int getDistance() { return _distance; }
	int getNCandidatesOfSameType() { return _n_candidates_of_same_type; }
	WordClusterClass getTriggerWC() { return _triggerWC; }

	bool hasArgumentWithThisRole(Symbol role);

	Symbol getCandidateHeadword() { return _candidateHeadword; }

	bool isDirectProp() const;
	bool isSharedProp() const;
	bool isUnconnectedProp() const;

	// distributional knowledge
	DistributionalKnowledgeClass getAADistributionalKnowledge() { return _aaDK; }
	std::set<Symbol> contentWords() { return _contentWords; }				

	int calculateNumOfPropFeatures();
	void setAADistributionalKnowledge();

	// local context
	int numberOfTokensBetweenAnchorArg();							
	std::set<Symbol> getBowBetween() { return _bowBetween; }				
	std::set< std::pair<Symbol,Symbol> > getBigramsBetween() { return _bigramsBetween; }	
	std::set<Symbol> getMentionEntityTypeBetween() { return _mentionEntityTypeBetween; }	
	Symbol getHasSameEntityTypeBetween() { return _hasSameEntityTypeBetween; }		

	Symbol getFirstWordBeforeM1NP() { return _firstWordBeforeM1NP; }			

	std::wstring getEntityTypesInNPSpanOfArg() { return _entityTypesInNPSpanOfArg; }	

	Symbol getAnchorArgRelativePositionNP() { return _anchorArgRelativePositionNP; }	
	Symbol getPrepBeforeArg() { return _prepBeforeArg; }					


private:
	static const Symbol _className;
	
	// we keep in its own place so we can manipulate it
	Symbol _candidateType;

	// features of the event
	const EventMention *_vMention;
	Symbol _stemmedTrigger;
	WordClusterClass _triggerWC;

	// the candidate
	const Mention *_candidateArgument;
	const ValueMention *_candidateValue;
	Symbol _candidateHeadword;

	// features of the event's relationship to the candidate
	Symbol _roleInTriggerProp;
	void setCandidateRoleInTriggerProp();
	Symbol _eventRoleInCP;
	Symbol _candidateRoleInCP;
	Symbol _stemmedCPPredicate;	
	Symbol _connectingString;
	Symbol _stemmedConnectingString;
	Symbol _posConnectingString;
	Symbol _abbrevConnectingString;
	Symbol _connectingCandParsePath;
	Symbol _connectingTriggerParsePath;

	int _distance;
	void setDistance();
	void setConnectingString();
	void setConnectingProp();
	void setConnectingParsePath();
	const Mention *findMentionForValue(const ValueMention *value);

	int _n_candidates_of_same_type;
	void setNCandidatesOfSameType();

	const TokenSequence *_tokens;
	const Parse *_parse;
	const MentionSet *_mentionSet;
	const ValueMentionSet *_valueMentionSet;
	const PropositionSet *_propSet;


	//methods for setting sentence/event information during intialization
	void resetForNewSentence(const TokenSequence *tokens, const Parse *parse, 
		const MentionSet *mentionSet,	const ValueMentionSet *valueMentionSet, const PropositionSet *propSet) 
	{
		_tokens = tokens;
		_parse = parse;
		_mentionSet = mentionSet;
		_valueMentionSet = valueMentionSet;
		_propSet = propSet;
		

		// distributional knowledge
	        if( ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_distributional_knowledge", false) ) {
			_predAssocPropFeas.clear();
			_argAssocPropFeas.clear();
        		_posTokens.clear();
        		_wordTokens.clear();
        		_contentWords.clear();

			setTokens();	// set: _posTokens , _wordTokens , _contentWords
		}
	}

	void setEvent(const EventMention *vMention);
	void setCandidateArgument(const Mention *mention);
	void setCandidateArgument(const ValueMention *value);


	void makeConnectingStringSymbol(int start, int end, bool candidate_in_front);


	// ==== START : DISTRIBUTIONAL KNOWLEDGE ==== 

	// ==== variables ====
	DistributionalKnowledgeClass _aaDK;
	std::set<SymbolPair> _predAssocPropFeas;
	std::set<SymbolPair> _argAssocPropFeas;	
	std::vector<Symbol> _posTokens;		
        std::vector<Symbol> _wordTokens;	
	std::vector<Symbol> _wordTokensLower;	
	std::vector<Symbol> _lemmaTokens;	
        std::set<Symbol> _contentWords;		
	std::set<Symbol> _nounPostags;		
	Proposition *_connectingProp;		// 10/17/2013 Yee Seng Chan. Useful to get info on connecting prop between anchor and arg
	std::vector<Symbol> _npChunks;		

	// local context
	std::set<Symbol> _bowBetween;				// bag of (lemma) unigrams between anchor and arg
	std::set< std::pair<Symbol,Symbol> > _bigramsBetween;	// bag of (lemma) bigrams between anchor and arg
	std::set<Symbol> _mentionEntityTypeBetween;
	Symbol _hasSameEntityTypeBetween;
	Symbol _firstWordBeforeM1NP;
	std::wstring _entityTypesInNPSpanOfArg;
	Symbol _anchorArgRelativePositionNP;
	Symbol _prepBeforeArg;

	// ==== functions ====
	// functions that set class variables
	void setTokens();						// invokes setNPChunks(.)
	void setNPChunks();
	void setAssociatedProps();
	void setAnchorArgRelativePositionNP();
	void setEntityTypesInNPSpanOfArg();
	void setAAPrepositions();	// sets : _anchorArgRelativePosition _prepBeforeArg _prepAfterArg _prepBeforeAnchor _prepAfterAnchor _anchorArgSharePrep

	// common helper functions
	Symbol getAnchorHwLemma();
	Symbol getAnchorPostag();
	Symbol getArgumentHwLemma();
	Symbol getCandidateArgumentHwLemma();
	Symbol getCandidateValueHwLemma();
	Symbol getArgumentPostag();
	std::pair<int,int> getAnchorArgBoundaryTokenOffset();		// if anchor_arg or arg_anchor, get token offsets for their boundaries in between
	std::wstring getNounPhraseTokensOfArg();
	std::pair<int, int> getNounPhraseBoundaryOfArg();
	std::pair<int, int> getNounPhraseBoundary(const int& start, const int& end);
	std::pair<int,int> getAnchorStartEndTokenOffset();
	std::pair<int,int> getArgStartEndTokenOffset();

	// sets local context words around anchor and arg
	void setBOWBetweenAnchorArg();
	void setBigramsBetweenAnchorArg();
	void setLocalContextWordsNP();					// invokes setLocalContextWordsNP(m1Offset, m2Offset)
	void setLocalContextWordsNP(const std::pair<int,int>& m1Offset, const std::pair<int,int>& m2Offset);

	// grab entity mentions between anchor and arg
	std::set<const Mention*> getMentionsBetweenAnchorArg();		// invokes mentionIsBetweenAnchorAndArg(.)
	void setMentionInfoBetweenAnchorArg(std::set<const Mention*> mentionsBetweenAnchorArg);
	void setHasSameEntityTypeBetween();
	bool mentionIsBetweenAnchorAndArg(const Mention* m);		// invokes mentionIsBetweenSpanInclusive(.)
	bool mentionIsBetweenSpanInclusive(const Mention* m, const int& startIndex, const int& endIndex);

	std::vector<std::wstring> displayDistributionalKnowledgeInfo();
	std::wstring prepositionInfoToString();
	std::wstring localContextWordsNPToString();
	std::wstring bowBetweenToString();
	std::wstring bigramsBetweenToString();
	// ==== END : DISTRIBUTIONAL KNOWLEDGE ====

	Symbol _oldRoleInTriggerProp;
	Symbol _oldEventRoleInCP;
	Symbol _oldCandidateRoleInCP;
};

#endif

