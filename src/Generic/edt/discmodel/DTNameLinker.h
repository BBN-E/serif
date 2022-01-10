// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DT_NAME_COREF_LINKER_H
#define DT_NAME_COREF_LINKER_H

#include "Generic/edt/MentionLinker.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/EntityGuess.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityType.h"
#include "Generic/common/DebugStream.h"
#include "Generic/discTagger/DTFeature.h"

class DTCorefObservation;
class DTTagSet;
class DTFeatureTypeSet;
class P1Decoder;
class MaxEntModel;
class DocumentMentionInformationMapper;
class SimpleRuleNameLinker;

#include "Generic/CASerif/correctanswers/CorrectDocument.h"

class DTNameLinker: public MentionLinker {
public:
	DTNameLinker(DocumentMentionInformationMapper *infoMap);
	~DTNameLinker();

	virtual void resetForNewSentence();
	virtual void resetForNewDocument(Symbol docName);
	virtual void resetForNewDocument(DocTheory *docTheory) { MentionLinker::resetForNewDocument(docTheory); }
//	virtual void cleanUpAfterDocument();
	virtual int linkMention (LexEntitySet *currSolution, MentionUID currMentionUID, 
							 EntityType linkType, LexEntitySet *results[], int max_results);

	static const int MAX_PREDICATES;

	// Correct Answers Serif methods
	CorrectDocument *currentCorrectDocument;
	int guessCorrectAnswerEntity(EntitySet *currSolution, Mention *currMention, 
								 EntityType linkType, EntityGuess *results[], int max_results); 
	void correctAnswersLinkMention(EntitySet *currSolution, MentionUID currMentionUID, EntityType linkType) {}
	void printCorrectAnswer(int id) {}

	double getOvergen(){ return _p1_overgen_threshold;}; 
	void setOvergen(double threshold) { _p1_overgen_threshold = threshold;}; 
	double getMaxentThreshold(){ return _maxent_link_threshold;}; 
	void setMaxentThreshold(double threshold) { _maxent_link_threshold = threshold;}; 


private:
	int guessEntity(LexEntitySet *currSolution, Mention *currMention, 
					EntityType linkType, EntityGuess results[], int max_results);

	bool _filter_by_entity_type;
	bool _filter_by_entity_subtype;
//	bool subtypeMatch(LexEntitySet *currSolution, Entity *entity, Mention *mention);
	bool _limitToNames;

	double _p1_overgen_threshold;
	//double _p1_undergen_threshold;
	double _maxent_link_threshold;
	double _rank_overgen_threshold;

	static DebugStream _debugStream;
	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;
	DTCorefObservation *_observation;
	P1Decoder *_p1Decoder;
	MaxEntModel *_maxEntDecoder;
	DTFeature::FeatureWeightMap *_p1Weights;
	DTFeature::FeatureWeightMap *_maxentWeights;

	DTFeatureTypeSet **_featureTypesArr;
	DTFeatureTypeSet *_noneFeatureTypes;

	double *_tagScores;
	bool _use_rules;
	SimpleRuleNameLinker *_simpleRuleNameLinker;
	bool _distillation_mode;

	int MODEL_TYPE;
	enum {P1, MAX_ENT, BOTH, P1_RANKING};

//	DocumentMentionInformationMapper *_infoMap;

};

#endif
