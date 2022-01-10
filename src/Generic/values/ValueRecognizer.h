// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef VALUE_RECOGNIZER_H
#define VALUE_RECOGNIZER_H

#include "Generic/values/ValueRuleRepository.h"
#include "Generic/theories/ValueType.h"
#include "Generic/common/Symbol.h"
#include <boost/shared_ptr.hpp>

class ValueMentionSet;
class TokenSequence;
class PIdFSentence;
class DTTagSet;
class DocTheory;
class Sexp;

class ValueRecognizer {
public:
	/** Create and return a new ValueRecognizer. */
	static ValueRecognizer *build() { return _factory()->build(); }
	/** Hook for registering new ValueRecognizer factories. */
	struct Factory { virtual ValueRecognizer *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual void resetForNewSentence() = 0;
	virtual void resetForNewDocument(DocTheory *docTheory = 0) = 0;

	// This does the work. It populates an array of pointers to ValueMentionSets
	// specified by results arg with up to max_theories ValueMentionSet pointers,
	// and returns the number of theories actually created, or 0 if
	// something goes wrong. The client is responsible for deleting the
	// ValueMentionSets.
	virtual int getValueTheories(ValueMentionSet **results, int max_theories,
		TokenSequence *tokenSequence) = 0;

	class ValueSpan {
	public:
		int start;
		int end;
		Symbol tag;

		ValueSpan() : start(-1), end(-1), tag(Symbol()) {}
		ValueSpan(int s, int e, Symbol t) : start(s), end(e), tag(t) {}

		/* Comparison operator needed for the sort() operation */
		bool operator < (const ValueSpan &vs) const { return (start < vs.start);} 
	};
	typedef std::vector<ValueSpan> SpanList;

	virtual ~ValueRecognizer() {}

protected:

	ValueMentionSet *createValueMentionSet(TokenSequence *tokenSequence, SpanList valueSpans);
	SpanList collectValueSpans(const PIdFSentence &sentence, const DTTagSet *tagSet);
	SpanList filterOverlappingAndEmptySpans(const SpanList &spans);

	static Symbol NONE_ST;
	static Symbol POSTDATE_SYM;

	ValueRuleRepository _valueRuleRepository;
	SpanList identifyRuleRepositoryPhoneNumbers(TokenSequence *tokenSequence);
	SpanList identifyRuleRepositoryValues(TokenSequence *tokenSequence);

private:
	static boost::shared_ptr<Factory> &_factory();

};


//#ifdef ENGLISH_LANGUAGE
//	#include "English/values/en_ValueRecognizer.h"
//#elif defined (CHINESE_LANGUAGE)
//	#include "Chinese/values/ch_ValueRecognizer.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/values/ar_ValueRecognizer.h"
//#else
//	#include "Generic/values/xx_ValueRecognizer.h"
//#endif


#endif
