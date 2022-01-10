// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P_IDF_AL_SENTENCE_H
#define P_IDF_AL_SENTENCE_H

#include "Generic/common/LocatedString.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/names/discmodel/PIdFSentence.h"
#include "Generic/discTagger/DTTagSet.h"

class UTF8InputStream;
class UTF8OutputStream;
class TokenSequence;


class PIdFActiveLearningSentence {
private:
	
	bool _in_training_set;
	Symbol _sentId;	//represented as docid-sentid
	Symbol _docId;
	int _sentNum;
	LocatedString* _sentenceString;
	TokenSequence* _tokens;
	PIdFSentence* _sentence;
	void AppendSentenceStart(std::wstring& str);
	void AppendSentenceEnd(std::wstring& str);
	void AppendSentenceBody(std::wstring& str, DTTagSet* tagSet, bool includeannotation = true);


public:
	PIdFActiveLearningSentence(): _in_training_set(false), _sentenceString(0), _tokens(0), _sentence(0){};
	void deleteMembers();
	void populate(Symbol id, LocatedString* sentenceString, 
		TokenSequence* tokens, PIdFSentence* sentence );
	void populate(const PIdFActiveLearningSentence& other );

	void WriteSentence(UTF8OutputStream& uos, DTTagSet* tagSet);
	void WriteTokenSentence(UTF8OutputStream& uos, Symbol id, TokenSequence* tokens);
	bool _isFirstSent;
	bool _isLastSent;
	PIdFSentence* GetPIdFSentence(){return _sentence;};
	Symbol GetId(){return _sentId;};
	Symbol GetDocId(){return _docId;};
	int GetSentenceNumber(){return _sentNum;};
	void AppendSentence(std::wstring& str, DTTagSet* tagSet, bool includeannotation = true);
	void AppendSentenceWithContext(std::wstring& str, DTTagSet* tagSet, 
		PIdFActiveLearningSentence** context, int ncontext, bool includeannotation = true);





};
#endif

