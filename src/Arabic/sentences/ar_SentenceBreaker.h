#ifndef AR_SENTENCE_BREAKER_H
#define AR_SENTENCE_BREAKER_H

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include <vector>

#include "Generic/common/LocatedString.h"
#include "Generic/theories/Sentence.h"
#include "Generic/common/limits.h"
#include "Generic/common/hash_set.h"
#include "Generic/sentences/DefaultSentenceBreaker.h"

//for NameRecognition
#include "Arabic/tokens/ar_Tokenizer.h"
#include "Arabic/names/ar_NameRecognizer.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/NameTheory.h"

#define MAX_SUB_SENTENCES_WRAPPER_SIZE 100

class PartOfSpeech;
class ArabicSentenceBreaker : public DefaultSentenceBreaker {
public:
	~ArabicSentenceBreaker();
	ArabicSentenceBreaker();

	/**
	 *  Sets the current document pointer to point to the 
	 *	document passed in and resets the current sentence
	 *	number to 0.
	 **/
	void resetForNewDocument(const Document *doc);
	
    /** 
	 *  This does the work. It passes in an array of pointers to LocatedStrings
	 *  corresponding to the regions of the document text and the number of regions.
	 *  It puts an array of pointers to Sentences where specified by results arg, and 
	 *  returns its size. It returns 0 if something goes wrong. The client is 
	 *  responsible for deleting the array and the Sentences. 
	 **/
	int getSentencesRaw(Sentence **results, int max_sentences, const Region* const* regions, int num_regions);

private:
	int getSentencesRawNew(Sentence **results, int max_sentences, const Region* const* regions, int num_regions);
	int getSentencesRawOld(Sentence **results, int max_sentences, const Region* const* regions, int num_regions);
	void doTokenizationAndMorphologicalAnalysis(Sentence *sentence, int sent_num);

	/**
	 *  Reads the next complete sentence from input, beginning at 
	 *  offset and creates a new Sentence object in the location
	 *  specified by the sentence arg. Returns the number of characters
	 *	read from input.
	 **/
	int getNextSentence(Sentence* &sentence, int offset, LocatedString *input);
	bool isElipsePer(LocatedString const *input, int pos, int& newpos);
	bool isAbrevPer(LocatedString const  *input, int pos, int& newpos);
	bool isNumSeparatorPer(LocatedString const  *input, int pos);
	const static wchar_t EOS_MARK = L'.'; // period  can be used in ellipses "..." and ".." 
	const static wchar_t EOQ_MARK = 0x061F; //Arabic question mark
	const static wchar_t NEW_LINE = 0x000A;
	const static wchar_t CARRIAGE_RETURN = 0x000D;

	// Tries to match and return a dateline from the beginning of input.
	int getDatelineSentences(Sentence **results, int max_sentences, const LocatedString *input);
	bool _breakLongSentences;

	//Do preliminary NameRecognition so that we don't split long sentences inside
	//names, also add a hash table of words that can not end sentences (eg. prepositions).
	struct HashKey {
		size_t operator()(const Symbol& s) const {
			return s.hash_code();
		}
	};
    struct EqualKey {
        bool operator()(const Symbol& s1, const Symbol& s2) const {
            return s1 == s2;
        }
    };
	/// List of nationality names to help identify person descriptors
	static hash_set<Symbol, HashKey, EqualKey> _nonEOSWords;


	class TokenSequencesWrapper {
	public:
		TokenSequencesWrapper();
		~TokenSequencesWrapper();
		void addSequence(TokenSequence * sequence);
		const Token* getToken(int pos);
		int getNTokens() { return _n_tokens; }
		void cleanup();
		int getTokenFromOffset(const LocatedString* source, int offset);
	private:
		std::vector<TokenSequence*> _sequences;
		std::vector<int> _n_tokens_in_sequence;
		int _n_sequences;
		int _n_tokens;
	};

	class POSSequencesWrapper {
	public:
		POSSequencesWrapper();
		~POSSequencesWrapper();
		void addSequence(PartOfSpeechSequence *sequence);
		const PartOfSpeech* getPOS(int pos);
		int getNTokens() { return _n_tokens; }
		void cleanup();
	private:
		std::vector<PartOfSpeechSequence*> _sequences;
		std::vector<int> _n_tokens_in_sequence;
		int _n_sequences;
		int _n_tokens;
	};



	class NameTheoriesWrapper {
	public:
		NameTheoriesWrapper();
		~NameTheoriesWrapper();
		void addSequence(NameTheory *sequence, int n_tokens);
		const NameTheory * getSequence(int pos);
		int getNSequences(){ return _n_sequences; };
		int getNTokens() { return _n_tokens; }
		void cleanup();
	private:
		std::vector<NameTheory*> _sequences;
		std::vector<int> _n_tokens_in_sequence;	
		int _n_sequences;
		int _n_tokens;
		;
	};

	/** Token theory buffer */
	class Tokenizer *_tokenizer;
	class TokenSequence **_tokenSequenceBuf;
	TokenSequencesWrapper _tokenSequenceWrapper;

	/** Name theory buffer */
	ArabicNameRecognizer *_nameRecognizer;
	class NameTheory **_nameTheoryBuf;
	NameTheoriesWrapper _nameTheoriesWrapper;

	/** Morphology */
	class MorphologicalAnalyzer *_morphAnalysis;
	class MorphSelector *_morphSelector;

	/** PartOfSpeech */
	class PartOfSpeechRecognizer *_posRecognizer;
	class PartOfSpeechSequence **_partOfSpeechSequenceBuf;
	POSSequencesWrapper _partOfSpeechWrapper;

	/** Parameters for breaking long sentences */
	bool _breakLongSentencesUsingHeuristicRule;
	bool _breakLongSentencesUsingStatisticalModel;

	static Symbol ARABIC_AND_SYMBOL;
	static Symbol ARABIC_BUT_SYMBOL;
	static Symbol ARABIC_XINHUA_SYMBOL;
	bool  isMidName(int currTok, NameTheoriesWrapper &nameWrapper);

	static int getTokenFromOffset(const LocatedString* source, const TokenSequence* toks, int offset, int startTok);
	static int getTokenFromOffset(const LocatedString* source, const TokenSequence* toks, int offset);
	bool isMidName(int currTok, const NameTheory* nt);
	bool  isBeforeName(int currTok, const NameTheory* nt);

};

#endif
