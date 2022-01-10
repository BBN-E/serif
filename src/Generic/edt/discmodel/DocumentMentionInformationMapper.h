// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOCUMENTMENTIONINFORMATIONMAPPER_H
#define DOCUMENTMENTIONINFORMATIONMAPPER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolArray.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/AbbrevTable.h"

class TokenSequence;

typedef serif::hash_map<MentionUID, SymbolArray*, MentionUID::HashOp, MentionUID::EqualOp> MentionSymArrayMap;
typedef serif::hash_map<MentionUID, Symbol, MentionUID::HashOp, MentionUID::EqualOp> MentionToSymbolMap;

/* This class is used to map Mention information so that it is not processed multiple
   times during feature extraction */
class DocumentMentionInformationMapper {
public:
	DocumentMentionInformationMapper();
	void cleanUpAfterDocument();
	void addMentionInformation(const Mention *ment) { addMentionInformation(ment,NULL); }
	void addMentionInformation(const Mention *ment, TokenSequence *tokenSequence);
	const MentionSymArrayMap* getHWMentionMapper() const { return &_mentionToHW; }
	const MentionSymArrayMap* getAbbrevMentionMapper() const { return &_mentionToAbbrev; }
	const MentionSymArrayMap* getPremodMentionMapper() const { return &_mentionToPremod; }
	const MentionSymArrayMap* getPostmodMentionMapper() const { return &_mentionToPostmod; }
	const MentionSymArrayMap* getWordsWithinBracketsMapper() const { return &_mentionToWordsWithinBrackets; }
	const MentionSymArrayMap* getMentionWordsMentionMapper() const { return &_mentionToWords; }
	const MentionToSymbolMap* getMentionGenderMapper() const { return &_mentionToGender; }
	const MentionToSymbolMap* getMentionNumberMapper() const { return &_mentionToNumber; }

private:
	MentionSymArrayMap _mentionToAbbrev;
	MentionSymArrayMap _mentionToHW;
	MentionSymArrayMap _mentionToWords;
	MentionSymArrayMap _mentionToPremod;
	MentionSymArrayMap _mentionToPostmod;
	MentionSymArrayMap _mentionToWordsWithinBrackets;
	MentionToSymbolMap _mentionToGender;
	MentionToSymbolMap _mentionToNumber;

	int cleanWords(Symbol words[], int nwords, Symbol results[], int maxNResults
		, Symbol wordsInBrackets[], int *nWordsInBrackets);
};

#endif
