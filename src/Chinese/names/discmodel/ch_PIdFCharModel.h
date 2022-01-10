// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_PIDF_CHAR_MODEL_H
#define CH_PIDF_CHAR_MODEL_H

#include "Generic/common/Symbol.h"
#include "Generic/discTagger/DTFeature.h"
#include "Chinese/names/discmodel/ch_PIdFCharSentence.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/names/discmodel/PIdFSecondaryDecoders.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/common/limits.h"

#include "Generic/discTagger/DTMaxMatchingListFeatureType.h"
#include "Generic/discTagger/DTObservation.h"

//use STL implementations, maybe I should implement these myself.....
#include <iostream>
#include <algorithm>
#include <functional>
#include <vector>

class DTFeatureTypeSet;
class DTTagSet;
class TokenObservation;
class IdFWordFeatures;
class PDecoder;
class NameSpan;
class BlockFeatureTable;

/** Maximum number of characters in a sentence.  If a sentence has
 * more characters than this limit, then no names will be found
 * past the max_sentence_char-th character. */
#define MAX_SENTENCE_CHARS_FOR_CH_PIDF_CHAR_MODEL 300

class PIdFCharModel {
public:

	/** The default constructor gets all the information it needs from
	  * ParamReader. */
	PIdFCharModel();

	/** -WARNING- This constructor may only be used in decode mode
	  *	This constructor uses the input files specified in 
	  *	the parameter list
	  */
	PIdFCharModel(const char* tag_set_file, const char* features_file, 
				  const char* model_file, bool learn_transitions = false);


	~PIdFCharModel();

	DTTagSet *getTagSet() { return _tagSet; }

	/** This method decodes on a PIdFCharSentence by storing its answers
	  * in the PIdFCharSentence instance. This is the decode method to use
	  * when you do your own I/O */
	void decode(PIdFCharSentence &sentence);
	void decode(PIdFCharSentence &sentence, double &margin);

	// for use as a Serif stage (as opp. to standalone):
	int getNameTheories(NameTheory **results, int max_theories,
						TokenSequence *tokenSequence);

	void resetForNewDocument(DocTheory *docTheory = 0);

	/* Fill in observation with values of word 
	*/
	void populateObservation(TokenObservation *observation,
		IdFWordFeatures *wordFeatures, Symbol word, bool first_word);
	
	/*
	*	Fill in all of the observations for a sentence
	*/
	void populateSentence(std::vector<DTObservation *> & observation, PIdFCharSentence* sent, int sent_length);

	/* Make a name theory (as used in Serif) from a PIdFSentence
	*/
	NameTheory *makeNameTheory(PIdFCharSentence &sentence);

	// for list-based feature match
	void updateDictionaryMatchingCache(const DTMaxMatchingListFeatureType* dtMaxMatchingListFeature, 
		std::vector<DTObservation *> &observations);

private:
	PIdFSecondaryDecoders *_secondaryDecoders;
	
	std::string _model_file;

	//DTModel classes
	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;
	IdFWordFeatures *_wordFeatures;

	bool _interleave_tags;	//this changes the order tags are processed 
	bool _learn_transitions_from_training; //learn possible features from training file, then produce model-transition file
	

	//Used when calling _decoder->decode(), since decode works on a series of observations, 
	//not a PIdFCharSentence
	std::vector<DTObservation *> _observations;
	static Token _blankToken;
	static Symbol _blankLCSymbol;
	static Symbol _blankWordFeatures;
	static WordClusterClass _blankWordClass;
	static Symbol _NONE_ST;
	static Symbol _NONE_CO;

	PDecoder *_decoder;
	BlockFeatureTable *_weights;

	NameSpan* _spanBuffer[MAX_SENTENCE_CHARS_FOR_CH_PIDF_CHAR_MODEL];
	bool firstNameHasRightsToToken(PIdFCharSentence &sentence,
								   int first_token_char_index, 
								   int second_token_char_index); 

	static const int OVERLAPPING_SPAN = -1;

	bool _print_sentence_selection_info;
};





#endif
