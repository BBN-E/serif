// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DESCRIPTOR_OBSERVATION_H
#define DESCRIPTOR_OBSERVATION_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/discTagger/DTAltModelSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/names/IdFWordFeatures.h"


class SynNode;
class MentionSet;

/** A DescriptorObservation represents all the information about a descriptor
  * that the P1 extracts features from.
  */
#define MAX_WN_OFFSETS 20

class DescriptorObservation : public DTObservation {
public:
	DescriptorObservation(bool use_wordnet);
	~DescriptorObservation();
		
	virtual DTObservation *makeCopy();

	/// Recycle the instance by entering new information into it.
	void populate(const MentionSet *mentionSet, const SynNode* node, 
				  const PropositionSet *propSet = 0);

	const SynNode *getNode() { return _node; }
	const MentionSet *getMentionSet() { return _mentionSet; }
	WordClusterClass getHeadwordWC() { return _headwordWC; }	
	int getNOffsets() { return _n_offsets; }
	int getNthOffset(int n) { return _wordnetOffsets[n]; }
	bool isPersonHyponym() { return _is_person_hyponym; }
	int getNValidProps(){return static_cast<int>(_valid_props.size());}
	Proposition* getValidProp(int i) { return _propSet->getProposition(_valid_props[i]); }
	Symbol getSentWord(int i) { return _sentence_words[i]; }
	int getNSentWords() { return _n_words; }
	Symbol getIdFWordFeature() { return _idfWordFeature; }

	Symbol getNthAltDecoderName(int i);
	Symbol getNthAltDecoderPrediction(int i);
	int getNAltDecoderPredictions();
	void setAltDecoderPrediction(int i, Symbol prediction);
	bool isPrevWordPrep() { return _prev_word_prep; }

private:

	void addPropositionSet(const PropositionSet *propositionSet);
	bool checkPrevWordPrep (const SynNode *node);

	bool _use_wordnet;
	const SynNode *_node;
	const MentionSet *_mentionSet;
	WordClusterClass _headwordWC;
	const PropositionSet *_propSet;
	Symbol _idfWordFeature;
	IdFWordFeatures* _wordFeatures;
	std::vector<int> _valid_props;
	//since these are pulled from the parse tree, they may not exactly match the sentence tokens 
	//(eg. in English they will all be lc???)
	Symbol _sentence_words[MAX_SENTENCE_TOKENS];
	int _n_words;

	// only used in English, but I can't figure out how to make this class
	// lang-specific due to _className ??
	int _wordnetOffsets[MAX_WN_OFFSETS];
	int _n_offsets;
	bool _is_person_hyponym;

	static DTAltModelSet _altModels;
	Symbol _altDecoderPredictions[MAX_ALT_DT_DECODERS];
	int _n_alt_predictions;
	bool _prev_word_prep;

	static const Symbol _className;	
};

#endif
