// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SENTENCE_BREAKER_H
#define SENTENCE_BREAKER_H

#include <boost/shared_ptr.hpp>

#include "Generic/theories/Sentence.h"
#include "Generic/common/LocatedString.h"
#include "English/sentences/WordSet.h"

class StatSentenceBreaker;
class NullSentenceBreaker;
class Region;
//class Zone;
class Document;
class Metadata;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED SentenceBreaker {
public:
	/** Create and return a new SentenceBreaker. */
	static SentenceBreaker *build();
	/** Hook for registering new SentenceBreaker factories. */
	struct Factory { virtual SentenceBreaker *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory);

	virtual ~SentenceBreaker();

	/** Set the "maximum sentence length."  Sentence breakers should
	 * try not to generate sentences longer than this maximum (though
	 * no guarantees are made).  In particular, if a sentence breaker
	 * finds a sentence longer than this maximum, then it should try
	 * to break it into smaller chunks.  This is typically given a
	 * language-dependent value. */
	static void setMaxSentenceChars(int max_sentence_chars);
	static int getMaxSentenceChars() { return _max_sentence_chars; }

	/// Reset the sentence breaker to run on the next new document.
	virtual void resetForNewDocument(const Document *doc) = 0;

	/** Split a given list of regions into sentences.  
	  * 
	  * The input is specified by the array `regions`, which must
	  * contain `num_region` Regions.  The text of these regions are
	  * split into sentences, which are written to the array
	  * `results`.  No more than `max_sentences` sentences will be
	  * created.  The return value is the number of sentences created.
	  *
	  * This method returns 0 on error.
	  *
	  * This method begins by delegates to the method
	  * getSentencesRaw(), which must be defined by subclasses.  If
	  * getSentencesRaw() returns any sentences consisting only of
	  * whitespace, then they will be removed from the result before
	  * it is returned.
	  *
	  * Subclasses should *not* override this method -- override the
	  * protected method getSentencesRaw() instead.  Note that this
	  * method is *not* virtual. */
	int getSentences(Sentence **results, int max_sentences, const Region* const* regions, int num_regions);
	//int getSentences(Sentence **results, int max_sentences, const Zone* const* zones, int num_zones);

	/**
	  * This is what SentenceBreaker subclasses actually define.
	  * getSentences() calls this. It shouldn't ever return sentences
	  * with nothing but whitespace (or nothing at all), but if it
	  * does, getSentences() removes such sentences.
	  */
	virtual int getSentencesRaw(Sentence **results, int max_sentences, const Region* const* regions, int num_regions) = 0;
	//virtual int getSentencesRaw(Sentence **results, int max_sentences, const Zone* const* zones, int num_zones) = 0;

protected:
	SentenceBreaker();

private:
	static boost::shared_ptr<Factory> &_factory();
	static int _max_sentence_chars;

	bool _do_language_id_check;
	Symbol::HashSet *_commonInLanguageWords;
	Symbol::HashSet *_commonNameTokens;
	float percentageInLangDoc(Sentence **result, int n_results);
	bool isInLangSentence(Sentence* sent, float percentage_in_lang);
};

#endif
