// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_PATTERN_MATCHER_MODEL_H
#define es_PATTERN_MATCHER_MODEL_H

#include <cstddef>
#include "Generic/common/Symbol.h"
template <size_t N>
class NgramScoreTableGen;
class PotentialRelationInstance;
class SymbolHash;
class DTTagSet;
#include "Generic/theories/RelationConstants.h"
#include "Generic/common/UTF8InputStream.h"

#define PM_MODEL_MAX_SLOT_FILLS 500

class PatternMatcherModel {
private:
	NgramScoreTable *_patterns;
	NgramScoreTable *_fullPatterns;
	Symbol _slots[9][PM_MODEL_MAX_SLOT_FILLS];
	void setPattern(int i0, int i1, int i2, int i3, int i4, int i5, 
		int i6, int i7, int i8, int index, bool fullPattern,
		int type);
	Symbol testForFullPattern(Symbol *ngram, Symbol *extraNgram);

	DTTagSet *_relationTags;

	enum { BASIC, BASIC_NESTED, FULL };

	SymbolHash * _personLikeOrgWords;
	SymbolHash * _facilityLikeOrgWords;
	Symbol returnType(Symbol *ngram, int result, bool full = false);

public:
	PatternMatcherModel(const char *file_prefix);
	~PatternMatcherModel();
	Symbol findBestRelationType(PotentialRelationInstance *instance, 
											  bool tryLeftMetonymy = true,
											  bool tryRightMetonymy = true);
	const Symbol wildcardSym;
	const Symbol nullSym;
};

#endif
