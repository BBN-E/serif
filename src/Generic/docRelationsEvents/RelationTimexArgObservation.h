// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_TIMEX_ARG_OBSERVATION_H
#define RELATION_TIMEX_ARG_OBSERVATION_H

#include "common/limits.h"
#include "common/Symbol.h"
#include "discTagger/DTObservation.h"
#include "theories/Mention.h"
#include "theories/RelMention.h"
#include "theories/ValueMention.h"
#include "relations/discmodel/RelationPropLink.h"

#include <string>

class TokenSequence;
class Parse;
class MentionSet;
class PropositionSet;
class SynNode;
class Proposition;


class RelationTimexArgObservation : public DTObservation {
public:
	RelationTimexArgObservation();

	~RelationTimexArgObservation();

	virtual DTObservation *makeCopy();

	/// Recycle the instance by entering new information into it.
	void resetForNewSentence(const TokenSequence *tokens, Parse *parse, 
		MentionSet *mentionSet,	ValueMentionSet *valueMentionSet, PropositionSet *propSet) 
	{
		_tokens = tokens;
		_parse = parse;
		_mentionSet = mentionSet;
		_valueMentionSet = valueMentionSet;
		_propSet = propSet;
		_propSet->fillDefinitionsArray();
	}

	void setRelation(RelMention *relMention);
	void setCandidateArgument(const ValueMention *value);

	// "his trial" --> trial
	Symbol getStemmedPredicate() { return _propLink->getTopStemmedPred(); }

	// "his trial" --> <poss>
	Symbol getCandidateRoleInPredicateProp() { return _roleInRelationProp; }

	// "he won the election" --> <sub>
	Symbol getCandidateRoleInCP() { return _candidateRoleInCP; }

	// "he won the election" --> <obj>
	Symbol getRelationRoleInCP() { return _relationRoleInCP; }

	// "he won the election" --> win
	Symbol getStemmedCPPredicate() { return _stemmedCPPredicate; }

	// "the people were moved" --> "PER were moved"
	// "he said by telephone from Belgium" --> "telephone from GPE"
	std::wstring& getConnectingString() { return _connectingString; }
	std::wstring& getStemmedConnectingString() { return _stemmedConnectingString; }
	std::wstring& getPOSConnectingString() { return _posConnectingString; }
	std::wstring& getAbbrevConnectingString() { return _abbrevConnectingString; }

	std::wstring& getConnectingCandParsePath() { return _connectingCandParsePath; }
	std::wstring& getConnectingPredicateParsePath() { return _connectingPredicateParsePath; }

	std::wstring& getMention1ConnectingString() { return _ment1ConnectingString; }
	std::wstring& getMention1StemmedConnectingString() { return _ment1StemmedConnectingString; }
	std::wstring& getMention1POSConnectingString() { return _ment1POSConnectingString; }
	std::wstring& getMention1AbbrevConnectingString() { return _ment1AbbrevConnectingString; }

	std::wstring& getMention2ConnectingString() { return _ment2ConnectingString; }
	std::wstring& getMention2StemmedConnectingString() { return _ment2StemmedConnectingString; }
	std::wstring& getMention2POSConnectingString() { return _ment2POSConnectingString; }
	std::wstring& getMention2AbbrevConnectingString() { return _ment2AbbrevConnectingString; }

	std::wstring& getTimexString() { return _timexString; }
	Symbol getGoverningPrep() { return _governingPrep; }
	Symbol getSentenceLocation() { return _sentenceLocation; }
	int getDistance() { return _distance; }
	Symbol getCandidateType() { return _candidateValue->getType(); }
	Symbol getRelationType() { return _relMention->getType(); }
	RelationPropLink *getPropLink() { return _propLink; }
	
private:
	static const Symbol _className;

	// features of the relation
	RelMention *_relMention;
	RelationPropLink *_propLink;

	// the candidate
	const Mention *_candidateArgument;
	const ValueMention *_candidateValue;

	// features of the event's relationship to the candidate
	Symbol _roleInRelationProp;
	Symbol _relationRoleInCP;
	Symbol _candidateRoleInCP;
	Symbol _stemmedCPPredicate;	
	
	std::wstring _connectingString;
	std::wstring _stemmedConnectingString;
	std::wstring _posConnectingString;
	std::wstring _abbrevConnectingString;

	std::wstring _connectingCandParsePath;
	std::wstring _connectingPredicateParsePath;
	
	std::wstring _ment1ConnectingString;
	std::wstring _ment1StemmedConnectingString;
	std::wstring _ment1POSConnectingString;
	std::wstring _ment1AbbrevConnectingString;
	
	std::wstring _ment2ConnectingString;
	std::wstring _ment2StemmedConnectingString;
	std::wstring _ment2POSConnectingString;
	std::wstring _ment2AbbrevConnectingString;
	
	std::wstring _timexString;
	Symbol _sentenceLocation;
	Symbol _governingPrep;

	int _distance;

	void setDistance();
	void setConnectingString();
	void setMentionConnectingStrings();
	void setConnectingParsePath();
	void setTimexString();
	void setGoverningPrep();
	void setSentenceLocation();
	void setConnectingProp();
	void setCandidateRoleInRelationProp();
	const Mention *findMentionForValue(const ValueMention *value);
	const SynNode *findSynNodeForValue(const SynNode *node, const ValueMention *value);
	void findPropLink();

	const TokenSequence *_tokens;
	Parse *_parse;
	MentionSet *_mentionSet;
	ValueMentionSet *_valueMentionSet;
	PropositionSet *_propSet;

	std::wstring makeConnectingStringSymbol(int start, int end, bool candidate_in_front);
	std::wstring makeAbbrevConnectingStringSymbol(int start, int end, bool candidate_in_front);
	std::wstring makePOSConnectingStringSymbol(int start, int end, bool candidate_in_front);
	std::wstring makeStemmedConnectingStringSymbol(int start, int end, bool candidate_in_front);

	void makeTimexString(int start, int end);
	
};

#endif
