// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DT_COREF_LINKER_H
#define DT_COREF_LINKER_H

#include "Generic/edt/MentionLinker.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/EntityGuess.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityType.h"
#include "Generic/common/DebugStream.h"
#include "Generic/discTagger/DTFeature.h"

class DocTheory;
class DTCorefObservation;
class DTTagSet;
class DTFeatureTypeSet;
class P1Decoder;
class MaxEntModel;

#include "Generic/CASerif/correctanswers/CorrectDocument.h"

class DTCorefLinker: public MentionLinker {
public: 
	//DTCorefLinker (bool strict = false); // JJO 09 Aug 2011 - word inclusion
	DTCorefLinker();
	~DTCorefLinker();

	virtual void resetForNewSentence();
	virtual void resetForNewDocument(Symbol docName);
	void resetForNewDocument(DocTheory *docTheory) { _docTheory = docTheory; }
	virtual int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, 
							 EntityType linkType, LexEntitySet *results[], int max_results);

	static const int MAX_PREDICATES;

	// Correct Answers Serif Methods:
	CorrectDocument *currentCorrectDocument;
	int guessCorrectAnswerEntity(EntitySet *currSolution, Mention *currMention, 
								 EntityType linkType, EntityGuess *results[], int max_results); 
	void correctAnswersLinkMention(EntitySet *currSolution, MentionUID currMentionUID, EntityType linkType) {}
	void printCorrectAnswer(int id) {}

	double getOvergen(){ return _p1_overgen_threshold;}; 
	void setOvergen(double threshold) { _p1_overgen_threshold = threshold;}; 
	double getMaxentThreshold(){ return _maxent_link_threshold;}; 
	void setMaxentThreshold(double threshold) { _maxent_link_threshold = threshold;}; 

/*
// +++ JJO 09 Aug 2011 +++
// Word inclusion 
private:
	bool _strict; 
// --- JJO 09 Aug 2011 ---
*/

private:

	int guessEntity(LexEntitySet * currSolution, Mention * currMention, 
					EntityType linkType, EntityGuess results[], int max_results);

	bool _filter_by_entity_type;
	bool _filter_by_entity_subtype;
	bool _filter_by_name_gpe;
	bool _block_headword_clashes;
	bool subtypeMatch(LexEntitySet *currSolution, Entity *entity, Mention *mention);

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
	DocTheory *_docTheory;

	double *_tagScores;

	DTFeatureTypeSet **_featureTypesArr;
	DTFeatureTypeSet *_noneFeatureTypes;

	int MODEL_TYPE;
	enum {P1, MAX_ENT, BOTH, P1_RANKING};


	// use non-ACE mentions as candidates for no-links
	bool _use_non_ace_entities_as_no_links;
//	void addNonAceEntities(GrowableArray<Entity *> &candidates, EntitySet *entitySet , Mention * ment, int maxEntites);
	int _max_non_ace_entities;

public:
	// use non-ACE mentions as candidates for no-links
	void useNonAceEntitiesAsNoLinks();

};

#endif
