// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_VALUE_RECOGNIZER_H
#define ch_VALUE_RECOGNIZER_H

#include "Generic/values/ValueRecognizer.h"

class ValueMentionSet;
class DocTheory;
class TokenSequence;
class PIdFModel;
class PIdFSentence;
class PIdFCharModel;
class PIdFCharSentence;
class DTTagSet;

class ChineseValueRecognizer : public ValueRecognizer {
private:
	friend class ChineseValueRecognizerFactory;

public:
	~ChineseValueRecognizer();
	void resetForNewSentence() {};
	void resetForNewDocument(class DocTheory *docTheory);

	/** 
	  * This does the work. It populates an array of pointers to ValueMentionSets
	  * specified by <code>results</code> with up to <code>max_theories</code> 
	  * ValueMentionSet pointers, and returns the number of theories actually 
	  * created, or 0 if something goes wrong. The client is responsible for 
	  * deleting the ValueMentionSets.
	  *
	  * @param results the array of pointers to populate
	  * @param max_theories the maximum number of theories to return
	  * @param tokenSequence the TokenSequence representing source text 
	  * @return the number of theories produced or 0 for failure
	  */
	int getValueTheories(ValueMentionSet **results, int max_theories,
						 TokenSequence *tokenSequence);


private:
	ChineseValueRecognizer();

	SpanList getPIdFValuesSingleModel(TokenSequence *tokenSequence, float& penalty);
	SpanList getPIdFValuesDualModel(TokenSequence *tokenSequence, float& penalty);

	SpanList collectValueSpans(const PIdFCharSentence &sentence, const DTTagSet *tagSet, float& penalty);
	
	bool firstValueHasRightsToToken(const PIdFCharSentence &sentence,
								   int first_token_char_index, 
								   int second_token_char_index); 
	

	bool DO_VALUES;
	int _sent_no;
	const TokenSequence *_tokenSequence;
	DocTheory *_docTheory;

	void correctSentence(PIdFCharSentence &sentence);
	void forceTag(PIdFCharSentence &sentence, 
				  int forced_ST_tag, int forced_CO_tag, int start, int end);
	bool isNoneTag(PIdFCharSentence &sentence, int i);
	bool isURLTag(PIdFCharSentence &sentence, int i);
	bool isPhoneTag(PIdFCharSentence &sentence, int i);
	bool isEmailTag(PIdFCharSentence &sentence, int i);
	bool isPercentTag(PIdFCharSentence &sentence, int i);
	bool isMoneyTag(PIdFCharSentence &sentence, int i);
	bool isTimexTag(PIdFCharSentence &sentence, int i);

	PIdFModel *_valueDecoder;
	PIdFCharModel *_valueCharDecoder;
	DTTagSet *_tagSet;

	PIdFModel *_timexDecoder;
	PIdFCharModel *_timexCharDecoder;
	PIdFModel *_otherValueDecoder;
	PIdFCharModel *_otherValueCharDecoder;
	DTTagSet *_timexTagSet;
	DTTagSet *_otherValueTagSet;

	bool _run_on_tokens;
	bool _use_dual_model;
	bool _use_pidf;
	bool _use_rules;
};

class ChineseValueRecognizerFactory: public ValueRecognizer::Factory {
	virtual ValueRecognizer *build() { return _new ChineseValueRecognizer(); } 
};
 
#endif
