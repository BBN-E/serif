// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_MODALITY_OBSERVATION_H
#define EVENT_MODALITY_OBSERVATION_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/WordConstants.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Proposition.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/common/SymbolHash.h"


#define ETO_MAX_WN_OFFSETS 20
#define ETO_MAX_OTHER_ARGS 3
#define MAX_INDICATORS 5
#define WINDOW_SIZE_VERB 5
#define WINDOW_SIZE_NOUN 5


class EventModalityObservation : public DTObservation {
public:
	/** Create and return a new EventModalityObservation. */
	static EventModalityObservation *build() { return _factory()->build(); }
	/** Hook for registering new EventModalityObservation factories */
	struct Factory { virtual EventModalityObservation *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }
	virtual ~EventModalityObservation() {}

	virtual DTObservation *makeCopy();

	void populate(EventModalityObservation *other);	
	void populate(int token_index, const TokenSequence *tokens, const Parse *parse,
		MentionSet *mentionSet, const PropositionSet *propSet, bool use_wordnet);

	Symbol getPOS();
	Symbol getStemmedWord();
	Symbol getWord();
	Symbol getLCWord();
	bool isLastWord();
	bool isFirstWord();
	Symbol getNextWord();
	Symbol getNextPOS();
	Symbol getSecondNextWord();
	Symbol getPrevWord();
	Symbol getPrevPOS();
	Symbol getSecondPrevWord();
	int getNthOffset(int n);
	int getReversedNthOffset(int n);
	int getNOffsets();
	WordClusterClass getWordCluster();
	Symbol getObjectOfTrigger();
	Symbol getIndirectObjectOfTrigger();
	Symbol getSubjectOfTrigger();
	Symbol getOtherArgToTrigger(int n);
	Symbol getDocumentTopic();
	void setDocumentTopic(Symbol topic);
	bool isNominalPremod();
	bool isCopula();

	static const int _MAX_OTHER_ARGS;
	static SymbolHash * _nonAssertedIndicators;


	// newly added
	static SymbolHash * _nonAssertedIndicatorsNoun;
	static SymbolHash * _nonAssertedIndicatorsVerb;
	static SymbolHash * _nonAssertedIndicatorsAdj;
	static SymbolHash * _nonAssertedIndicatorsAdv;
	static SymbolHash * _nonAssertedIndicatorsMD;
	static SymbolHash * _nonAssertedIndicatorsNearby;
	static SymbolHash * _nonAssertedIndicatorsOther;
	static SymbolHash * _verbsCausingEvents;

	bool isLedbyAllegedAdverb();
	bool isLedbyModalWord();
	bool isFollowedbyIFWord();
	bool isLedbyIFWord();
	bool hasNonAssertedParent();
	bool isNonAssertedbyRules();
	bool isModifierOfNoun();
	bool isNounModifierOfNoun();
	int getNIndicatorsModifyingNoun(){return _numIndicatorModifyingNoun;}
	int getNIndicatorsAboveS(){return _numIndicatorAboveS;}
	int getNIndicatorsAboveVP(){return _numIndicatorAboveVP;}
	int getNIndicatorsMDAboveVP(){return _numIndicatorMDAboveVP;}
	int getNIndicatorsNearby(){return _numIndicatorNearby;}
	bool hasIndicatorsAboveS () {return _hasIndicatorAboveS;}
	bool hasIndicatorsAboveVP () {return _hasIndicatorAboveVP;}
	bool hasIndicatorsMDAboveVP () {return _hasIndicatorMDAboveVP;}
	bool hasIndicatorsModifyingNoun (){return _hasIndicatorModifyingNoun;}
	bool hasIndicatorsNearby (){return _hasIndicatorNearby;}
	Symbol * getIndicatorsAboveS () {return _indicatorAboveS; }
	Symbol * getIndicatorsAboveVP () {return _indicatorAboveVP; }
	Symbol * getIndicatorsMDAboveVP () {return _indicatorMDAboveVP; }
	Symbol * getIndicatorsModifyingNoun() {return _indicatorModifyingNoun; }
	Symbol * getIndicatorsNearby() {return _indicatorNearby; }
	bool hasRichFeatures() {return _hasRichFeatures; }
	bool isPremodOfMention() {return _isPremodOfMention;}
	bool isPremodOfNP(){return _isPremodOfNP;}

	Symbol getMentionType() {return _mentionType;}
	Symbol getEntityType() {return _entityType;} 

	// used in two layer model
	void assignClassifier(int i){_classifierNo =i;}
	int getClassifierNo (){return _classifierNo;}

	static void finalize();

protected:
	EventModalityObservation() : 
		DTObservation(_className),
		_wordCluster(WordClusterClass::nullCluster()) {}

	virtual bool isLikelyNonAsserted (Proposition *prop) const;
	virtual std::vector<bool> identifyNonAssertedProps(const PropositionSet *propSet, 
													   const MentionSet *mentionSet) const;

	virtual bool isLedbyAllegedAdverb(Proposition * prop) const { return false; }
	virtual bool isLedbyModalWord(Proposition * prop) const { return false; }
	virtual bool isFollowedbyIFWord(Proposition * prop) const { return false; }
	virtual bool isLedbyIFWord(Proposition * prop) const { return false; }
	virtual bool parentIsLikelyNonAsserted(Proposition * prop, const PropositionSet *propSet, const MentionSet *mentionSet) const { return false; }
    virtual void findIndicators(int token_index, const TokenSequence *tokens,
								const Parse *parse, MentionSet *mentionSet) {}

private:
	static void initializeStaticVariables();
	static const Symbol _className;

protected:	
	Symbol _word;
	Symbol _nextWord;
	Symbol _secondNextWord;
	Symbol _prevWord;
	Symbol _secondPrevWord;
	Symbol _lcWord;
	Symbol _stemmedWord;
	Symbol _pos;
	Symbol _prevPOS;
	Symbol _nextPOS;
	Symbol _subjectOfTrigger;
	Symbol _objectOfTrigger;
	Symbol _indirectObjectOfTrigger;
	Symbol _otherArgsToTrigger[ETO_MAX_OTHER_ARGS];
	WordClusterClass _wordCluster;
	int _wordnetOffsets[ETO_MAX_WN_OFFSETS];
	int _n_offsets;
	Symbol _documentTopic;
	bool _is_nominal_premod;
	bool _is_copula;

	bool _ledbyAllegedAdverb;
	bool _ledbyModalWord;
	bool _followedbyIFWord;
	bool _ledbyIFWord;
	bool _parentIsLikelyNonAsserted;
	bool _isNonAssertedbyRules;
	bool _modifyingNoun;
	bool _nounModifyingNoun;

	bool _hasIndicatorAboveS;
	Symbol _indicatorAboveS[MAX_INDICATORS];
	bool _hasIndicatorAboveVP;
	Symbol _indicatorAboveVP[MAX_INDICATORS];
	bool _hasIndicatorModifyingNoun;
	Symbol _indicatorModifyingNoun[MAX_INDICATORS];
	bool _hasIndicatorNearby;
	Symbol _indicatorNearby[MAX_INDICATORS];
	bool _hasIndicatorMDAboveVP;
	Symbol _indicatorMDAboveVP[MAX_INDICATORS];

	int _numIndicatorModifyingNoun;	
	int _numIndicatorAboveS;
	int _numIndicatorAboveVP;
	int _numIndicatorNearby;
	int _numIndicatorMDAboveVP;

	bool _hasRichFeatures;
	int _classifierNo;
	bool _isPremodOfMention;
	bool _isPremodOfNP;
	Symbol _mentionType;
	Symbol _entityType; 

private:
	static boost::shared_ptr<Factory> &_factory();
	class GenericEventModalityObservationFactory: public Factory {
		EventModalityObservation *build() 
		{ return _new EventModalityObservation(); }
	};
};

#endif
