// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/names/IdFDecoder.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/names/NameClassTags.h"
#include "Generic/names/IdFModel.h"
#include "Generic/names/IdFSentenceTokens.h"
#include "Generic/common/limits.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/names/IdFListSet.h"
#include <fcntl.h>
#include <boost/scoped_ptr.hpp>

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


/** 
 * Initializes IdFDecoder with a NameClassTags object and with 
 * a model_file_prefix specifying the location of the model.
 * If given a listfile, initializes an IdFListSet as well.
 *
 * Pruning is turned off by default (FORWARD_PRUNING_THRESHOLD = 100
 * and FORWARD_BEAM_WIDTH = nameClassTags->getNumTags(). To turn it on,
 * use the functions setForwardPruningThreshold() and setForwardBeamWidth().
 * 5 is a good value for the former; 50% or less of getNumTags() is a good
 * value for the latter.
 * 
 */
IdFDecoder::IdFDecoder(const char *model_file_prefix, const NameClassTags *nameClassTags, 
		   const char *list_file_name, int word_features_mode)
{
	IdFListSet *listSet = list_file_name == 0 ? 0 : _new IdFListSet(list_file_name);
	new(this) IdFDecoder(model_file_prefix, nameClassTags, 
						IdFWordFeatures::build(word_features_mode, listSet));
}

/** 
 * Use this constructor to directly give the decoder a WordFeatures object.
 * See other comments above. listSet argument is optional, but the IdFListSet 
 * object (if any) inside wordFeatures should obviously be the same as listSet, 
 * but that's not technically enforced.
 */


IdFDecoder::IdFDecoder(const char *model_file_prefix, 
					   const NameClassTags *nameClassTags,
					   const IdFWordFeatures *wordFeatures) : 
	    BACKWARD_BEAM_WIDTH(10), FORWARD_PRUNING_THRESHOLD(100.0),
		BACKWARD_PRUNING_THRESHOLD(5.0), _nameClassTags(nameClassTags), 
		FORWARD_BEAM_WIDTH(nameClassTags->getNumTags())
{
	_model = _new IdFModel(_nameClassTags, model_file_prefix, wordFeatures);

	//Initialize a matrix to store the trellis scores for debugging purposes
    // _breakdownMatrix = new probBreakdown* [_nameClassTags->getNumRegularTags()];
	// for (int i = 0; i < _nameClassTags->getNumRegularTags(); i++) {
	//	 _breakdownMatrix[i] = new probBreakdown[MAX_SENTENCE_TOKENS];
	// }

	_alphaProbs = _new double* [_nameClassTags->getNumRegularTags()];
	for (int i = 0; i < _nameClassTags->getNumRegularTags(); i++) {
		_alphaProbs[i] = _new double[MAX_SENTENCE_TOKENS];
	}
	_backPointers = _new int* [_nameClassTags->getNumRegularTags()];
	for (int j = 0; j < _nameClassTags->getNumRegularTags(); j++) {
		_backPointers[j] = _new int[MAX_SENTENCE_TOKENS];
	}

	_currentTheories = _new IdFSentenceTheory* [BACKWARD_BEAM_WIDTH];
	for (int k = 0; k < BACKWARD_BEAM_WIDTH; k++) {
		_currentTheories[k] = 0;
	}
	_numCurrentTheories = 0;
	_previousTheories = _new IdFSentenceTheory* [BACKWARD_BEAM_WIDTH];
	for (int l = 0; l < BACKWARD_BEAM_WIDTH; l++) {
		_previousTheories[l] = 0;
	}	
	_numPreviousTheories = 0;
	// this is a slight cheat: this array should really be the size of
	// max_theories, but then we couldn't statically allocate it.
	// we can't produce more theories than BACKWARD_BEAM_WIDTH, however,
	// so this is sufficient (and, currently, it's enforced in decodeSentenceNBest).
	_translatedNBestTheories = _new IdFSentenceTheory* [BACKWARD_BEAM_WIDTH];
	for (int m = 0; m < BACKWARD_BEAM_WIDTH; m++) {
		_translatedNBestTheories[m] = 0;
	}	

	if (_model->useLists()) {
		_tokenTranslationArray = _new int[MAX_SENTENCE_TOKENS];
	}
	else {
		_tokenTranslationArray=NULL;
	}

}

IdFDecoder::~IdFDecoder() {
	for (int i = 0; i < _nameClassTags->getNumRegularTags(); i++) {
		delete []_alphaProbs[i];
		delete []_backPointers[i];
	}
	delete _model;
	delete []_alphaProbs;
	delete []_backPointers;
	delete []_currentTheories;
	delete [] _previousTheories;
	delete [] _translatedNBestTheories;
	if (_tokenTranslationArray!=NULL) {delete []_tokenTranslationArray;} //if lists, delete this array
}

/* sets FORWARD_PRUNING_THRESHOLD (see comment on walkForwardThroughSentence()) */
void IdFDecoder::setForwardPruningThreshold(float threshold) {
	if (threshold > 0)
		FORWARD_PRUNING_THRESHOLD = threshold;
	else SessionLogger::info("SERIF") << "tried to set IdF pruning threshold to value <= 0: " << threshold << "\n";
}

/* sets FORWARD_BEAM_WIDTH (see comment on walkForwardThroughSentence()) */
void IdFDecoder::setForwardBeamWidth(int beamwidth) {
	if (beamwidth > 0)
		FORWARD_BEAM_WIDTH = beamwidth;
	else SessionLogger::info("SERIF") << "tried to set IdF beam width to a value <= 0: " << beamwidth << "\n";	
}

/**
 * Decodes from file, prints to file; NOT USED IN SERIF (used in stand-alone decoder). 
 */
void IdFDecoder::decode(const char *input_file, const char *output_file) {
	bool DEBUG = false;
	UTF8OutputStream debugStream;
	if (DEBUG) debugStream.open("debug.idf");
	UTF8OutputStream outStream;
	outStream.open(output_file);	

	boost::scoped_ptr<UTF8InputStream> decodeStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& decodeStream(*decodeStream_scoped_ptr);
	decodeStream.open(input_file);

	UTF8OutputStream sgmlStream;
	sgmlStream.open("bracketed.out");

	IdFSentenceTokens* sentenceTokens = new IdFSentenceTokens();	
	IdFSentenceTheory* sentenceTheory = new IdFSentenceTheory();

	// makes my life easier, since I currently only run this
	// stand-alone decoder for MUC decoding. Remove if it annoys you.
	outStream << L"<DOC>\n<DOCID>0</DOCID>\n";
	
	while (sentenceTokens->readDecodeSentence(decodeStream)) {
		decodeSentence(sentenceTokens, sentenceTheory);
		if (DEBUG)  {
			// if you want to debug with lists, you'll need to do it somewhere where
			//   you have access to the translated sentenceTokens array that was actually
			//   decoded. for now, I didn't bother.
			if (!_model->useLists())
				printTrellis(sentenceTokens, debugStream);
			debugStream << _nameClassTags->to_string(sentenceTokens, sentenceTheory);
			debugStream << L"\n";
			debugStream << _nameClassTags->to_enamex_sgml_string(sentenceTokens, sentenceTheory);
			debugStream << L"\n\n";
		}
		outStream << _nameClassTags->to_enamex_sgml_string(sentenceTokens, sentenceTheory);
		outStream << L"\n\n";

		sgmlStream << _nameClassTags->to_string(sentenceTokens, sentenceTheory);
		sgmlStream << L"\n";
	}
	
	// see above
	outStream << L"</DOC>\n";

	decodeStream.close();
	outStream.close();
	sgmlStream.close();
	if (DEBUG) debugStream.close();

}

/**
 * Decodes from file, prints n-best to file; NOT USED IN SERIF (used in stand-alone decoder). 
 */
void IdFDecoder::decodeNBest(const char *input_file, const char *output_file) {
	bool DEBUG = false;
	UTF8OutputStream outStream;
	outStream.open(output_file);	

	boost::scoped_ptr<UTF8InputStream> decodeStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& decodeStream(*decodeStream_scoped_ptr);
	decodeStream.open(input_file);

	IdFSentenceTokens* sentenceTokens = _new IdFSentenceTokens();	
	IdFSentenceTheory **sentenceTheories = _new IdFSentenceTheory*[BACKWARD_BEAM_WIDTH];

	int count = 0;
	while (sentenceTokens->readDecodeSentence(decodeStream)) {
		int num_theories = decodeSentenceNBest(sentenceTokens, 
			sentenceTheories, BACKWARD_BEAM_WIDTH);
		outStream << L"***BEGIN SENTENCE " << count << "***\n\n";
		double best_score = -10000;
		int best_theory = -1;
		for (int i = 0; i < num_theories; i++) {
			if (sentenceTheories[i]->getBetaScore() > best_score) {
				best_score = sentenceTheories[i]->getBetaScore();
				best_theory = i;
			}
		}
		outStream << L"BEST: ";
		outStream << _nameClassTags->to_enamex_sgml_string(sentenceTokens, 
			sentenceTheories[best_theory]);
		outStream << L"\n\n";
		for (int j = 0; j < num_theories; j++) {
			if (j == best_theory) continue;
			outStream << _nameClassTags->to_enamex_sgml_string(sentenceTokens, sentenceTheories[j]);
			outStream << L"\n\n";
		}
		outStream << L"***END SENTENCE " << count << "***\n\n";
		count++;
	}
	
	decodeStream.close();
	outStream.close();

}

/**
 * FOR DEBUGGING -- prints trellis of alpha probs and back pointers to stream
 */
void IdFDecoder::printTrellis(IdFSentenceTokens* sentenceTokens, UTF8OutputStream &stream) {

	wchar_t buffer[100];
	stream << L"                    ";
	for (int sent_index = 0; sent_index < sentenceTokens->getLength(); sent_index++) {
		swprintf(buffer, 100, L"\t(%52ls) ", sentenceTokens->getWord(sent_index).to_string());
		stream << buffer;
	}
	stream << L"\n";
	stream << L"                    ";
	for (int s_index = 0; s_index < sentenceTokens->getLength(); s_index++) {
		stream <<"\t";
		Symbol this_word = sentenceTokens->getWord(s_index);
		if (!_model->wordIsInVocab(this_word))
			swprintf(buffer, 100, L"%30ls ", this_word.to_string());
		else 
			swprintf(buffer, 100, L"%30ls ", L"***");
		stream << buffer;

	}
	stream << L"\n";
	for (int tag = 0; tag < _nameClassTags->getNumRegularTags(); tag++) {
		swprintf(buffer, 100, L"%10.20ls ", _nameClassTags->getTagSymbol(tag).to_string());
		stream << buffer;
		for (int sent_index = 0; sent_index < sentenceTokens->getLength(); sent_index++) {
			stream <<"\t";
			swprintf(buffer, 100, L"%0.5g = w%0.5g + t%0.5g (%2d) ", 
				_alphaProbs[tag][sent_index], 

				//Fill the debugging matrix
				/*
				_breakdownMatrix[tag][sent_index].wordProb,
				_breakdownMatrix[tag][sent_index].tagProb,
				*/
				_backPointers[tag][sent_index]);
			stream << buffer;

		}

		stream << L"\n";
	}
	stream << L"\n";
}


/**
 * Given a set of sentence tokens, _transforms them as necessary_, and
 * then calls decodeSentenceAsGiven -- results end up in sentenceTheory
 * one way or another
 */
int IdFDecoder::decodeSentence(IdFSentenceTokens* sentenceTokens, 
		IdFSentenceTheory* sentenceTheory)
{
	int retval;
	if (_model->useLists()) {
		IdFSentenceTokens translatedTokens;
		IdFSentenceTheory translatedTheory;
		translateTokens(sentenceTokens, &translatedTokens);
		retval = decodeSentenceAsGiven(&translatedTokens, &translatedTheory);
		unTranslateTheory(&translatedTheory, sentenceTheory, sentenceTokens->getLength());
	} else retval = decodeSentenceAsGiven(sentenceTokens, sentenceTheory);
	
	return retval;
}


/**
 * Given a set of sentence tokens, decodes over them and places best result 
 * in the given IdFSentenceTheory 
 */
int IdFDecoder::decodeSentenceAsGiven(IdFSentenceTokens* sentenceTokens, 
							          IdFSentenceTheory *sentenceTheory) 
{
	sentenceTheory->setLength(sentenceTokens->getLength());
    int back_pointer = walkForwardThroughSentence(sentenceTokens);

	// no valid path found
	if (back_pointer == -1) {
		for (int i = 0; i < sentenceTokens->getLength(); i++) {
			sentenceTheory->setTag(i, _nameClassTags->getNoneStartTagIndex());
		}
		sentenceTheory->setBestPossibleScore(-10000);
		return 0;
	}

	for (int sent_index = sentenceTokens->getLength() - 1; sent_index >= 0; sent_index--) {
		sentenceTheory->setTag(sent_index, back_pointer);
		back_pointer = _backPointers[back_pointer][sent_index];
	}
	sentenceTheory->setBestPossibleScore(_best_alpha_prob);		
	return 1;
}

/**
 * Given a set of sentence tokens, _transforms them as necessary_, and
 * then calls decodeSentenceNBestAsGiven -- results end up in theories
 * array one way or another
 */
int IdFDecoder::decodeSentenceNBest(IdFSentenceTokens* sentenceTokens, 
									IdFSentenceTheory **theories, 
									int max_theories)
{
	int retval;
	if (_model->useLists()) {
		IdFSentenceTokens translatedTokens;
		translateTokens(sentenceTokens, &translatedTokens);

		// so that _translatedNBestTheories can have a maximum size...
		//   currently this shouldn't matter, but I don't want to have to track
		//   down a memory corruption bug if someone changes something
		if (max_theories > BACKWARD_BEAM_WIDTH)
			max_theories = BACKWARD_BEAM_WIDTH;

		retval = decodeSentenceNBestAsGiven(&translatedTokens, _translatedNBestTheories, max_theories);
		for (int i = 0; i < retval; i++) {
			// NB: theories[] will own this pointer
			theories[i] = new IdFSentenceTheory();
			unTranslateTheory(_translatedNBestTheories[i], theories[i], sentenceTokens->getLength());
			delete _translatedNBestTheories[i];
		}
	} else retval = decodeSentenceNBestAsGiven(sentenceTokens, theories, max_theories);
	
	return retval;
}

/**
 * Given a set of sentence tokens, decodes over them and fills in set of theories 
 */
int IdFDecoder::decodeSentenceNBestAsGiven(IdFSentenceTokens* sentenceTokens, 
										   IdFSentenceTheory **theories, 
									       int max_theories)
{
	int back_pointer = walkForwardThroughSentence(sentenceTokens);

	// no valid path found
	if (back_pointer == -1) {
		theories[0] = new IdFSentenceTheory(sentenceTokens->getLength(),
			_nameClassTags->getNoneStartTagIndex());
		theories[0]->setBestPossibleScore(-10000);
		return 1;
	}


	if (max_theories == 1) {
		theories[0] = new IdFSentenceTheory();
		theories[0]->setLength(sentenceTokens->getLength());
		for (int sent_index = sentenceTokens->getLength() - 1; sent_index >= 0; sent_index--) {
			theories[0]->setTag(sent_index, back_pointer);
			back_pointer = _backPointers[back_pointer][sent_index];
		}
		return 1;
	}

	walkBackwardThroughSentence(sentenceTokens);

	if (max_theories > _numCurrentTheories) {
		int num_theories = _numCurrentTheories;
		for (int i = 0; i < _numCurrentTheories; i++) {
			// NB: theories[] will now own the pointer to this IdFSentenceTheory 
			theories[i] = _currentTheories[i];
			theories[i]->setLength(sentenceTokens->getLength());
			_currentTheories[i] = 0;
		}
		_numCurrentTheories = 0;
		return num_theories;
	} else {
		for (int j = 0; j < max_theories; j++) {
			double best_score = -10000;
			int best_index = 0;
			for (int i = 0; i < _numCurrentTheories; i++) {
				if (_currentTheories[i] == 0)
					continue;
				if (_currentTheories[i]->getBestPossibleScore() > best_score) {
					best_index = i;
					best_score = _currentTheories[i]->getBestPossibleScore();
				}
			}
			// NB: theories[] will now own the pointer to this IdFSentenceTheory 
			theories[j] = _currentTheories[best_index];
			theories[j]->setLength(sentenceTokens->getLength());
			_currentTheories[best_index] = 0;
		}
		
		// clean up _currentTheories
		for (int i = 0; i < _numCurrentTheories; i++) {
			if (_currentTheories[i] != 0)
				delete _currentTheories[i];
			_currentTheories[i] = 0;
		}
		_numCurrentTheories = 0;
		return max_theories;
	}


}

/**
 * Walks forward through sentence, filling in _alphaProbs and _backPointers
 *
 * Prunes any score that is more than FORWARD_PRUNING_THRESHOLD lower than the
 * best score in its column. Only allows the FORWARD_BEAM_WIDTH best entries from
 * each column to advance. (there is a "column" for each word in the sentence)
 *
 * @return the best back pointer (from the end of the sentence)
 */
int IdFDecoder::walkForwardThroughSentence(IdFSentenceTokens* sentenceTokens) {
	double prev_score, word_prob, tag_prob, score;
	int this_tag, prev_tag;
	int num_valid_tags_found_for_word;
	double worst_score;
	int worst_score_index;

	Symbol this_word = sentenceTokens->getWord(0);
	Symbol prev_word = _nameClassTags->getSentenceStartTag();

	if (!_model->wordIsInVocab(this_word)) {
		this_word = _model->getWordFeatures(this_word, true,
						_model->normalizedWordIsInVocab(this_word));
	}

	// zero out chart
	_best_alpha_prob = _model->LOG_OF_ZERO;
	for (int sent_index = 1; sent_index < sentenceTokens->getLength(); sent_index++) {
		for (this_tag = 0; this_tag < _nameClassTags->getNumRegularTags(); this_tag++) {

			//Zero out the debugging matrix as well
			/*
			_breakdownMatrix[this_tag][0].wordProb = _model->LOG_OF_ZERO;
			_breakdownMatrix[this_tag][0].tagProb = _model->LOG_OF_ZERO;
			*/

			_alphaProbs[this_tag][sent_index] = _model->LOG_OF_ZERO;
		}
	}

	// first word in sentence
	for (this_tag = 0; this_tag < _nameClassTags->getNumRegularTags(); this_tag++) {
		if (_nameClassTags->isContinue(this_tag)) {
			
			//Fill the debugging matrix as well
			/*
			_breakdownMatrix[this_tag][0].wordProb = _model->LOG_OF_ZERO;
			_breakdownMatrix[this_tag][0].tagProb = _model->LOG_OF_ZERO;
			*/
			_alphaProbs[this_tag][0] = _model->LOG_OF_ZERO;
			_backPointers[this_tag][0] = 0;
			continue;
		}
		prev_tag = _nameClassTags->getSentenceStartIndex();
		word_prob = _model->getWordProbability(this_word, this_tag, prev_tag, prev_word);
		tag_prob = _model->getTagProbability(this_tag, prev_tag, prev_word);

		//Keep track of the individual logprobs for debugging
		/*
		_breakdownMatrix[this_tag][0].wordProb = word_prob;
		_breakdownMatrix[this_tag][0].tagProb = tag_prob;
        */
		_alphaProbs[this_tag][0] = word_prob + tag_prob;
		_backPointers[this_tag][0] = 0;
		if (_alphaProbs[this_tag][0] > _best_alpha_prob)
			_best_alpha_prob = _alphaProbs[this_tag][0];
	}

	// rest of words
	for (int sentence_index = 1; sentence_index < sentenceTokens->getLength(); sentence_index++) {
		prev_word = this_word;
		this_word = sentenceTokens->getWord(sentence_index);
		if (!_model->wordIsInVocab(this_word))
			this_word = _model->getWordFeatures(this_word, false,
							_model->normalizedWordIsInVocab(this_word));
		for (prev_tag = 0; prev_tag < _nameClassTags->getNumRegularTags(); prev_tag++) {
			prev_score = _alphaProbs[prev_tag][sentence_index - 1];
			if (prev_score <= _model->LOG_OF_ZERO ||
				prev_score <= _best_alpha_prob - FORWARD_PRUNING_THRESHOLD)
				continue;

			// START tags
			// notice that now we reference an array of start tags to avoid making tag
			// location assignments in code other than NameClassTags

			// these are all tags that can come anywhere
			int tag_idx;
			for (tag_idx = 0; tag_idx < _nameClassTags->getNumStartTags(); tag_idx ++) {
				this_tag = _nameClassTags->trueStartTagArray[tag_idx];
				word_prob = _model->getWordProbability(this_word, 
							this_tag, prev_tag, prev_word);
				tag_prob = _model->getTagProbability(this_tag, prev_tag, prev_word);
				score = word_prob + tag_prob + prev_score;
				if (score > _alphaProbs[this_tag][sentence_index]) {
	
					//logprob breakdown for debugging
					/*
					_breakdownMatrix[this_tag][sentence_index].wordProb = word_prob;
					_breakdownMatrix[this_tag][sentence_index].tagProb = tag_prob;
					*/



					_alphaProbs[this_tag][sentence_index] = score;
					_backPointers[this_tag][sentence_index] = prev_tag;
				}
			}
			// CONTINUE tag(s)
			// if prev is top level, add the top level continue
			// if prev is nested, add both nested and top level continues
			if (!sentenceTokens->isConstrained(sentence_index)) {
				// this is the pop-up case, for nesteds. Previous tag was nested, and we're 
				// staying
				// in the top but popping up
				if (_nameClassTags->isNested(prev_tag)) {
					this_tag = _nameClassTags->getTopContinueForm(prev_tag);


					word_prob = _model->getWordProbability(this_word, this_tag, prev_tag, prev_word);
					tag_prob = _model->getTagProbability(this_tag, prev_tag, prev_word);
					score = word_prob + tag_prob + prev_score;
					if (score > _alphaProbs[this_tag][sentence_index]) {

						//logprob breakdown for debugging
						/*
						_breakdownMatrix[this_tag][sentence_index].wordProb = word_prob;
						_breakdownMatrix[this_tag][sentence_index].tagProb = tag_prob;
						*/

						_alphaProbs[this_tag][sentence_index] = score;
						_backPointers[this_tag][sentence_index] = prev_tag;
					}
				}
				// this is the stay-where-you-are case. We were top and stay top, or we were
				// nest and stay int the same nest
				this_tag = _nameClassTags->getContinueForm(prev_tag);
				word_prob = _model->getWordProbability(this_word, this_tag, prev_tag, prev_word);
				tag_prob = _model->getTagProbability(this_tag, prev_tag, prev_word);
				score = word_prob + tag_prob + prev_score;
				if (score > _alphaProbs[this_tag][sentence_index]) {

					//logprob breakdown for debugging
					/*
					_breakdownMatrix[this_tag][sentence_index].wordProb = word_prob;
					_breakdownMatrix[this_tag][sentence_index].tagProb = tag_prob;
					*/

					_alphaProbs[this_tag][sentence_index] = score;
					_backPointers[this_tag][sentence_index] = prev_tag;
				}
				// now the go-down case. We were top and go to a (starting) nest case, or we
				// were nested and go to a new (starting) nest case
				// only NONE is not allowed
				// if we're not allowed to nest, obviously nothing is added here
				if (_nameClassTags->getNumNestableTags() > 0 &&
					prev_tag < _nameClassTags->getNoneStartTagIndex() && 
					prev_tag < _nameClassTags->getNoneContinueTagIndex()) {
					int* tagArray = _nameClassTags->getStartInMiddleTagArray(prev_tag);
					for (tag_idx = 0; tag_idx < _nameClassTags->getNumNestableTags(); tag_idx++) {
						this_tag = tagArray[tag_idx];
						if (this_tag < 0)
							continue;
						word_prob = _model->getWordProbability(this_word, this_tag, prev_tag, prev_word);
						tag_prob = _model->getTagProbability(this_tag, prev_tag, prev_word);
						score = word_prob + tag_prob + prev_score;
						if (score > _alphaProbs[this_tag][sentence_index]) {

							//logprob breakdown for debugging
							/*
							_breakdownMatrix[this_tag][sentence_index].wordProb = word_prob;
							_breakdownMatrix[this_tag][sentence_index].tagProb = tag_prob;
							*/

							_alphaProbs[this_tag][sentence_index] = score;
							_backPointers[this_tag][sentence_index] = prev_tag;
						}
					}
				}
			}
		}


		_best_alpha_prob = _model->LOG_OF_ZERO;
		num_valid_tags_found_for_word = 0;
		for (this_tag = 0; this_tag < _nameClassTags->getNumRegularTags(); this_tag++) {
			if (_alphaProbs[this_tag][sentence_index] > _best_alpha_prob)
				_best_alpha_prob = _alphaProbs[this_tag][sentence_index];
			if (_alphaProbs[this_tag][sentence_index] > _model->LOG_OF_ZERO)
				num_valid_tags_found_for_word++;
		}
		while (num_valid_tags_found_for_word > FORWARD_BEAM_WIDTH) {
			worst_score = 0;
			worst_score_index = -1;
			for (this_tag = 0; this_tag < _nameClassTags->getNumRegularTags(); this_tag++) {
				if (_alphaProbs[this_tag][sentence_index] > _model->LOG_OF_ZERO &&
					_alphaProbs[this_tag][sentence_index] < worst_score) {
					worst_score = _alphaProbs[this_tag][sentence_index];
					worst_score_index = this_tag;
				}
			}
			_alphaProbs[worst_score_index][sentence_index] = _model->LOG_OF_ZERO;
			num_valid_tags_found_for_word--;
		}

	}
				
	// end "word"
	_best_alpha_prob = _model->LOG_OF_ZERO;
	int best_back_pointer = -1;
	prev_word = this_word;
	for (int p_tag = 0; p_tag < _nameClassTags->getNumRegularTags(); p_tag++) {
		prev_score = _alphaProbs[p_tag][sentenceTokens->getLength() - 1];
		if (prev_score <= _model->LOG_OF_ZERO)
			continue;
		tag_prob = _model->getTagProbability(_nameClassTags->getSentenceEndIndex(), 
			p_tag, prev_word);
		double score = tag_prob + prev_score;
		if (score > _best_alpha_prob) {
			_best_alpha_prob = score;
			best_back_pointer = p_tag;
		}
	}

	return best_back_pointer;
}

/**
 * Walks backward through sentence, doing a beam search. Final results end up in 
 * _currentTheories. Pruning governed by BACKWARD_PRUNING_THRESHOLD and 
 * BACKWARD_BEAM_WIDTH.
 */
void IdFDecoder::walkBackwardThroughSentence(IdFSentenceTokens* sentenceTokens) {

	double alpha_score, word_prob, tag_prob;
	int tag, tag_plus_one;
	Symbol word, word_plus_one;

	_previousTheories[0] = new IdFSentenceTheory(sentenceTokens->getLength(),
		_nameClassTags->getNoneStartTagIndex());
	_numPreviousTheories = 1;

	word = sentenceTokens->getWord(sentenceTokens->getLength() - 1);
	if (!_model->wordIsInVocab(word)) {
		word = _model->getWordFeatures(word, false, _model->normalizedWordIsInVocab(word));
	}

	// generate end tag
	for (tag = 0; tag < _nameClassTags->getNumRegularTags(); tag++) {
		alpha_score = _alphaProbs[tag][sentenceTokens->getLength() - 1];
		if (alpha_score <= _model->LOG_OF_ZERO)
			continue;
		tag_prob = _model->getTagProbability(_nameClassTags->getSentenceEndIndex(), 
			tag, word);
		addCurrentTheory(_previousTheories[0], sentenceTokens->getLength() - 1, 
			tag, alpha_score, tag_prob);
	}
	transferCurrentTheoriesToPrevious();

	// we're going backwards
	word_plus_one = word;

	// generate each word and its tag
	for (int sent_index = sentenceTokens->getLength() - 2; sent_index >= 0; sent_index--) {
		word_plus_one = word;
		word = sentenceTokens->getWord(sent_index);
		if (!_model->wordIsInVocab(word)) {
			word = _model->getWordFeatures(word, sent_index == 0, 
				_model->normalizedWordIsInVocab(word));
		}		
		for (int t = 0; t < _numPreviousTheories; t++) {
			tag_plus_one = _previousTheories[t]->getTag(sent_index + 1);
			for (tag = 0; tag < _nameClassTags->getNumRegularTags(); tag++) {
				alpha_score = _alphaProbs[tag][sent_index];
				if (alpha_score <= _model->LOG_OF_ZERO)
					continue;
				word_prob = _model->getWordProbability(word_plus_one,
					tag_plus_one, tag, word);
				tag_prob = _model->getTagProbability(tag_plus_one, tag, word);
				addCurrentTheory(_previousTheories[t], 
					sent_index, tag, alpha_score, word_prob + tag_prob);
			}
		}
		transferCurrentTheoriesToPrevious();
	}

	for (int i = 0; i < _numPreviousTheories; i++) {
		IdFSentenceTheory *theory = new IdFSentenceTheory(_previousTheories[i]);
		word_prob = _model->getWordProbability(word,
					_previousTheories[i]->getTag(0),
					_nameClassTags->getSentenceStartIndex(),
					_nameClassTags->getSentenceStartTag());
		tag_prob = _model->getTagProbability(_previousTheories[i]->getTag(0),
					_nameClassTags->getSentenceStartIndex(),
					_nameClassTags->getSentenceStartTag());
		theory->incrementBetaScore(word_prob + tag_prob);
		theory->setBestPossibleScore(theory->getBetaScore());
		_currentTheories[_numCurrentTheories] = theory;
		_numCurrentTheories++;
		delete _previousTheories[i];
		_previousTheories[i] = 0;
	}
	_numPreviousTheories = 0;
}

void IdFDecoder::transferCurrentTheoriesToPrevious() {
	for (int i = 0; i < _numPreviousTheories; i++) {
		delete _previousTheories[i];
		 _previousTheories[i] = 0;
	}
	for (int j = 0; j < _numCurrentTheories; j++) {
		_previousTheories[j] = _currentTheories[j];
		_currentTheories[j] = 0;
	}
	_numPreviousTheories = _numCurrentTheories;
	_numCurrentTheories = 0;
}

void IdFDecoder::addCurrentTheory(IdFSentenceTheory *prevTheory, int sentenceTokens_index, 
								  int tag, double alpha_score, double beta_score) 
{
	double total_score = alpha_score + beta_score + prevTheory->getBetaScore();

	if (total_score < _best_alpha_prob - BACKWARD_PRUNING_THRESHOLD)
		return;

	if (_numCurrentTheories < BACKWARD_BEAM_WIDTH) {
		IdFSentenceTheory *theory = new IdFSentenceTheory(prevTheory);
		theory->incrementBetaScore(beta_score);
		theory->setTag(sentenceTokens_index, tag);
		theory->setBestPossibleScore(total_score);
		_currentTheories[_numCurrentTheories] = theory;
		_numCurrentTheories++;
	} else {
		double lowest_score = 0;
		int lowest_index = 0;
		for (int i = 0; i < _numCurrentTheories; i++) {
			if (_currentTheories[i]->getBestPossibleScore() < lowest_score) {
				lowest_score = _currentTheories[i]->getBestPossibleScore();
				lowest_index = i;
			}
		}
		IdFSentenceTheory *theory = new IdFSentenceTheory(prevTheory);
		theory->incrementBetaScore(beta_score);
		theory->setTag(sentenceTokens_index, tag);
		theory->setBestPossibleScore(total_score);
		delete _currentTheories[lowest_index];
		_currentTheories[lowest_index] = theory;
	}
}

/** for use with lists */
void IdFDecoder::translateTokens(IdFSentenceTokens *original, IdFSentenceTokens *modified)
{
	int new_token_count = 0;
	for (int i = 0; i < original->getLength(); ) {
		int length = _model->isListMember(original, i);
		if (length != 0) {
			for (int j = i; j < i + length; j++) {
				_tokenTranslationArray[j] = new_token_count;
			}
			modified->setWord(new_token_count, _model->barSymbols(original, i, length));
			i += length;
		} else {
			modified->setWord(new_token_count, original->getWord(i));
			_tokenTranslationArray[i] = new_token_count;
			i++;
		}
		new_token_count++;
	}

	modified->setLength(new_token_count);

}

/** for use with lists */
void IdFDecoder::unTranslateTheory(IdFSentenceTheory *modified, IdFSentenceTheory *original,
								   int original_length)
{
	for (int i = 0; i < original_length; i++) {
		int tag = modified->getTag(_tokenTranslationArray[i]);
		if (i == 0 || _tokenTranslationArray[i] != _tokenTranslationArray[i-1])
			original->setTag(i, tag);
		else if (_nameClassTags->isStart(tag))
			original->setTag(i, _nameClassTags->getContinueForm(tag));
		else original->setTag(i, tag);
	}
	original->setBestPossibleScore(modified->getBestPossibleScore());
	original->setLength(original_length);
}
