// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DT_PRONOUN_LINKER_H
#define DT_PRONOUN_LINKER_H

#include "Generic/edt/PronounLinker.h"
//#include "Generic/theories/EntitySet.h"
#include "Generic/edt/LexEntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/LinkGuess.h"
#include "Generic/edt/HobbsDistance.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityType.h"
#include "Generic/discTagger/DTFeature.h"

#include <vector>

class DTCorefObservation;
class DTTagSet;
class DTFeatureTypeSet;
class P1Decoder;
class MaxEntModel;

class DTPronounLinker {
public:
	DTPronounLinker ();
	~DTPronounLinker();

	virtual void addPreviousParse(const Parse *parse);
	virtual void resetPreviousParses();

	virtual int linkMention (LexEntitySet * currSolution, 
							 MentionUID currMentionUID, 
							 EntityType linkType, 
							 LexEntitySet *results[], 
							 int max_results);

	virtual int getLinkGuesses(EntitySet *currSolution, 
							   Mention *currMention, 
							   LinkGuess results[], 
							   int max_results);

	void resetForNewDocument(DocTheory *docTheory);
	void resetForNewSentence();
private:

	std::vector<const Parse *> _previousParses;

	DocTheory *_docTheory;

	double _p1_overgen_threshold;
	double _maxent_link_threshold;
	double _rank_overgen_threshold;

	bool _model_outside_and_within_sentence_links_separately;

	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;
	DTCorefObservation *_observation;
	P1Decoder *_p1Decoder;
	P1Decoder *_sentenceP1Decoder;
	P1Decoder *_outsideP1Decoder;
	MaxEntModel *_maxEntDecoder;
	DTFeature::FeatureWeightMap *_weights;
	DTFeature::FeatureWeightMap *_sentenceWeights;
	DTFeature::FeatureWeightMap *_outsideWeights;
	double *_tagScores;

	DTFeatureTypeSet **_featureTypesArr;
	DTFeatureTypeSet *_noneFeatureTypes;

	int MODEL_TYPE;
	enum {P1, MAX_ENT,P1_RANKING};

	bool _isLinkedBadly(EntitySet *currSolution, Mention* pronMention, Entity* linkedEntity);

	int getHobbsDistance(EntitySet *entitySet, Entity *entity,
						HobbsDistance::SearchResult *hobbsCandidates, 
						int nHobbsCandidates);

	static DebugStream &_debugStream;

	bool _isSpeakerMention(Mention *ment);	
	bool _isSpeakerEntity(Entity *ent, EntitySet *ents);

	// if true we do not create new pronoun entities here
	bool _discard_new_pronoun_entities;

	// use non-ACE mentions as candidates for no-links
	bool _use_non_ace_entities_as_no_links;
	int _max_non_ace_candidates;

	bool _use_correct_answers;
};

#endif
