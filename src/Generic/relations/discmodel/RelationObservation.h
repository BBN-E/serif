// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_OBSERVATION_H
#define RELATION_OBSERVATION_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/theories/Mention.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/relations/discmodel/P1RelationTrainer.h"
#include "Generic/relations/discmodel/DTRelSentenceInfo.h"
#include "Generic/relations/discmodel/DTRelationAltModel.h"
#include "Generic/relations/xx_RelationUtilities.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "Generic/common/ParamReader.h"

#define REL_MAX_WN_OFFSETS 20

class Parse;
class MentionSet;
class PropositionSet;
class DTRelationSet;
class TreeNodeChain;
class PropTreeLink;
class SymbolArray;
class DTFeatureTypeSet;

/** A RelationObservation represents all the information about a relation (or potential 
  * relation) that the P1 extracts features from.
  */

class RelationObservation : public DTObservation {
private: 
	Symbol _validation_type;
	const Mention *_m1;
	const Mention *_m2;
	Symbol _altDecoderPredictions[MAX_ALT_DT_DECODERS];
	int _n_predictions;
	Symbol _altPatternPrediction;	

	// I'm going to leave this in, commented out, even though it doesn't seem to help and 
	// would need to be changed from being English-specific to be checked-in
	//PatternMatcherModel *_patternModel;
	//PotentialRelationInstance *_instance;

	static DTRelationAltModel _altModels;

public:
	RelationObservation(DTFeatureTypeSet *features = 0);
	RelationObservation(const wchar_t* validation_str, DTFeatureTypeSet *features = 0);
	RelationObservation(const char* validation_str, DTFeatureTypeSet *features = 0);
	~RelationObservation();

	virtual DTObservation *makeCopy();
public:
	Symbol getValidationType(){return _validation_type;}
	void setValidationType(Symbol type){_validation_type = type;}

	void setPatternPrediction(Symbol type) { _altPatternPrediction = type; }
	Symbol getPatternPrediction() { return _altPatternPrediction; }


	Symbol getNthAltDecoderName(int i);
	Symbol getNthAltDecoderPrediction(int i);
	int getNAltDecoderPredictions();
	void setAltDecoderPrediction(int i, Symbol prediction);

	static wstring TOO_LONG;
	static wstring ADJACENT;
	static wstring CONFUSED;
	

	/// Recycle the instance by entering new information into it.
	void resetForNewSentence(DTRelSentenceInfo *sentenceInfo);
	void resetForNewSentence(EntitySet *eset, 
				  const Parse *parse, 
				  const MentionSet *mentionSet, 
				  const ValueMentionSet *valueMentionSet, 
				  PropositionSet *propSet,
				  const Parse *secondaryParse = 0,
				  const NPChunkTheory *npChunk = 0,
				  const PropTreeLinks* ptLinks = 0);
	void resetForNewSentence(const Parse *parse, 
				  const MentionSet *mentionSet, 
				  const ValueMentionSet *valueMentionSet,
				  PropositionSet *propSet,
				  const Parse *secondaryParse = 0,
				  const NPChunkTheory *npChunk = 0);
	void resetForNewSentence(EntitySet *eset, 
				  const Parse *parse, 
				  const MentionSet *mentionSet, 
				  const ValueMentionSet *valueMentionSet, 
				  PropositionSet *propSet,
				  Symbol documentTopic,
				  const Parse *secondaryParse = 0,
				  const NPChunkTheory *npChunk = 0,
				  const PropTreeLinks* ptLinks = 0);
	void resetForNewSentence(const Parse *parse, 
				  const MentionSet *mentionSet, 
				  const ValueMentionSet *valueMentionSet,
				  PropositionSet *propSet,
				  Symbol documentTopic,
				  const Parse *secondaryParse = 0,
				  const NPChunkTheory *npChunk = 0);


	void populate(int mention1ID, int mention2ID);

	const Parse *getParse() { return _sentenceInfo->parses[0]; }
	const Parse *getSecondaryParse() { return _sentenceInfo->secondaryParses[0]; }

	const MentionSet *getMentionSet() { return _sentenceInfo->mentionSets[0]; }
	const Mention *getMention1() { return _m1; }
	const Mention *getMention2() { return _m2; }
	const PropositionSet *getPropSet() { return _sentenceInfo->propSets[0]; }

	RelationPropLink *getPropLink() { return _propLinks[0]; }
	const TreeNodeChain* getPropTreeNodeChain();
	RelationPropLink *getNthPropLink(int n) { return _propLinks[n]; }
	wstring& getWordsBetween() { return _wordsBetweenMentions; }
	wstring& getPOSBetween() { return _posBetweenMentions; }
	wstring& getStemmedWB() { return _stemmedWBMentions; }
	bool isTooLong() { return (_wordsBetweenMentions == TOO_LONG); };
	Symbol getPOS(int i){ return _pos[i];};
	Symbol getToken(int i){ return _tokens[i];};
	Symbol getDocumentTopic() { return _sentenceInfo->documentTopic; }
	

	bool isValidTag(Symbol tag) { 
		//mrf 9-15
		//this function wasn't being called, b/c it wasn't virtual in DTObservation, 
		//I've made it virtual in DTObservation, and am using it through the language specific
		//isValidRelationEntityTypeCombo.  I'm commenting out the previous behavior default (but
		//never actually called) behavior.  
		/*
		Symbol base = RelationConstants::getBaseTypeSymbol(tag);
		if (base == Symbol(L"PER-SOC") &&
			(!_m1->getEntityType().matchesPER() ||
			 !_m2->getEntityType().matchesPER()))
		{
			return false;
		}
		*/
		if(RelationUtilities::get()->isValidRelationEntityTypeCombo(_validation_type, _m1, _m2, tag)){
			return true;
		}
		return false;

	}


	//added for NP chunk based relation models
	bool hasPossessiveRel() { return _hasPossessiveRel; };
	bool hasPossessiveAfterMent2() {return _hasPossessiveAfterMent2; };
	int getNOffsetsOfMent1() const { return _n_offsets_ment1; };
	int getNthOffsetOfMent1(int n) const { return _wordnetOffsetsofMent1[n]; };
	int getNOffsetsOfMent2() const { return _n_offsets_ment2; };
	int getNthOffsetOfMent2(int n) const { return _wordnetOffsetsofMent2[n]; };
	int getNOffsetsOfVerb() const { return _n_offsets_verb; };
	int getNthOffsetOfVerb(int n) const { return _wordnetOffsetsofVerb[n]; };


	WordClusterClass getWCMent2() const { return _wcMent2; };
	bool hasPPRel() {return _hasPPRel;};
	Symbol getPrepinPPRel () {return _prepinPPRel; };
	Symbol getVerbinPPRel () {return _verbinPPRel; };
	Symbol getStemmedVerbinPPRel(){return _stemmedVerbinPPRel;};
	WordClusterClass getWCMent1() const { return _wcMent1; };
	WordClusterClass getWCVerb() const { return _wcVerb; };

	WordClusterClass getWCVerbPred() const { return _wcVerbPred; };
	Symbol getVerbinProp () {return _verbinProp; };
	Symbol getStemmedVerbinProp(){return _stemmedVerbinProp;};
	int getNOffsetsOfVerbPred() const { return _n_offsets_verbPred; };
	int getNthOffsetOfVerbPred(int n) const { return _wordnetOffsetsofVerbPred[n]; };

	Symbol getStemmedHeadofMent1(){return _stemmedHeadofm1;};
	Symbol getStemmedHeadofMent2(){return _stemmedHeadofm2;};

	SymbolArray *getMention1Name() { return m1Name; }
	SymbolArray *getMention2Name() { return m2Name; }

private:
	DTRelSentenceInfo *_sentenceInfo;
	static DTRelSentenceInfo _1bestSentenceInfo;
	Symbol _tokens[MAX_SENTENCE_TOKENS];
	Symbol _pos[MAX_SENTENCE_TOKENS];
	int _m1_id;
	int _m2_id;

	SymbolArray *m1Name;
	SymbolArray *m2Name;
	void findMentionNames();
	int getMentionNameSymbols(const Mention *ment, Symbol *array_, int max_length);

	Symbol isMentionOrValue(int start, int end, int& skipto);
	Symbol isReducedGroup(int start, int end, int& skipto);

	RelationPropLink *_propLinks[10];

	static const Symbol _className;
	void findPropLink(int index);
	
	wstring _wordsBetweenMentions;
	wstring _posBetweenMentions;
	wstring _stemmedWBMentions;
	void findWordsBetween();
	void makeWBSymbol(int start, int end);

	//added for NP chunk based relation models

	Symbol _stemmedHeadofm1;
	Symbol _stemmedHeadofm2;
	Symbol _stemmedVerbinPPRel;
	Symbol _stemmedVerbinProp;

	void findPossessiveRel();
	bool _hasPossessiveRel;

	void setHasPossessiveAfterMent2();
	bool _hasPossessiveAfterMent2;

	int _wordnetOffsetsofMent1[REL_MAX_WN_OFFSETS];
	int _n_offsets_ment1;
	WordClusterClass _wcMent1;

	int _wordnetOffsetsofMent2[REL_MAX_WN_OFFSETS];
	int _n_offsets_ment2;
	WordClusterClass _wcMent2;

	void findPPRel();
	bool _hasPPRel;
	Symbol _prepinPPRel;
	Symbol _verbinPPRel;
	int _wordnetOffsetsofVerb[REL_MAX_WN_OFFSETS];
	int _n_offsets_verb;
	WordClusterClass _wcVerb;

	void findverbProp();
	Symbol _verbinProp;
	int _wordnetOffsetsofVerbPred[REL_MAX_WN_OFFSETS];
	int _n_offsets_verbPred;
	WordClusterClass _wcVerbPred;

	void examineFeatures(DTFeatureTypeSet *features);
	bool _npchunkFeatures;

};

#endif
