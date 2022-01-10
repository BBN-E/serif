// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/NgramScoreTable.h"
#include "English/relations/en_PatternMatcherModel.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/Sexp.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "English/relations/en_OldRelationFinder.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/ParamReader.h"
#include "Generic/discTagger/DTTagSet.h"

#include <math.h>
#include <stdio.h>
#include <boost/scoped_ptr.hpp>

#if defined(_WIN32)
#define snprintf _snprintf
#endif

static Symbol FAC_symbol(L"FAC");
static Symbol PER_symbol(L"PER");
static Symbol ORG_symbol(L"ORG");
static Symbol LOC_symbol(L"LOC");
static Symbol GPE_symbol(L"GPE");

PatternMatcherModel::PatternMatcherModel(const char *file_prefix)
	: wildcardSym(Symbol(L"*")), nullSym(Symbol(L"NULL"))
{
	std::string tags_file = ParamReader::getRequiredParam("relation_pattern_model_tags");
    _relationTags = _new DTTagSet(tags_file.c_str(), false, false);

	std::string filename = ParamReader::getParam("person_like_org_words");
	if (!filename.empty())
		_personLikeOrgWords = _new SymbolHash(filename.c_str());
	else _personLikeOrgWords = _new SymbolHash(5);

	filename = ParamReader::getParam("facility_like_org_words");
	if (!filename.empty())
		_facilityLikeOrgWords = _new SymbolHash(filename.c_str());
	else _facilityLikeOrgWords = _new SymbolHash(5);

	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& stream(*stream_scoped_ptr);
	Symbol reltype;
	Symbol FULL_PATTERN(L"FULL_PATTERN");

	stream.open(file_prefix);
	_patterns = _new NgramScoreTable(6, 10000);
	_fullPatterns = _new NgramScoreTable(9, 1000);

	// ( predicates TYPE TYPE ROLE ROLE [nested-role] type )
	// ---- where nested-role is optional
	// OR
	// (FULL_PATTERN predicates TYPE TYPE ROLE ROLE nested-role left-hw right-hw nested-hw type)
	// (predicates TYPE TYPE ROLE ROLE nested-role left-hw right-hw nested-hw type)
	//
	// any of which can be lists except the relation type at the end
	//
	int num_entries = 0;
	while (!stream.eof()) {

		Sexp *pattern = _new Sexp(stream);
		if (pattern->isVoid())
			break;
        num_entries++;

		int nkids = pattern->getNumChildren();
		int pattern_type;
		int start_index = 0;
		if (nkids == 6)
			pattern_type = BASIC;
		else if (nkids == 7)
			pattern_type = BASIC_NESTED;
		else if (nkids == 10)
			pattern_type = FULL;
		else if (nkids == 11) {
			pattern_type = FULL;
			start_index = 1;
		} else {
			char c[1000];
			snprintf(c, 1000, "ERROR: invalid entry: %s",
					          pattern->to_debug_string().c_str());
			throw UnexpectedInputException("PatternMatcherModel::PatternMatcherModel()", c);
		}

		for (int j = 0; j < 9; j++) {
			if (j == 5 && pattern_type == BASIC) {
				_slots[j][0] = nullSym;
				_slots[j][1] = Symbol();
				continue;
			}
			if (j >= 6 && pattern_type != FULL) {
				_slots[j][0] = nullSym;
				_slots[j][1] = Symbol();
				continue;
			}

			Sexp *child = pattern->getNthChild(start_index + j);
			if (child->isAtom()) {
				_slots[j][0] = child->getValue();
				_slots[j][1] = Symbol();
			} else {
				int n_grandkids = child->getNumChildren();
				for (int k = 0; k < n_grandkids; k++) {
					Sexp *grandchild = child->getNthChild(k);
					if (!grandchild->isAtom()) {
						char c[1000];
						snprintf(c, 1000, "ERROR: invalid entry: %s",
										  pattern->to_debug_string().c_str());
						throw UnexpectedInputException("PatternMatcherModel::PatternMatcherModel()",
							c);
					}
					if (k < PM_MODEL_MAX_SLOT_FILLS)
						_slots[j][k] = grandchild->getValue();
					else {
						char c[100];
						sprintf( c, "ERROR: too many words in a set at entry: %d", num_entries );
						throw UnexpectedInputException("PatternMatcherModel::PatternMatcherModel()",
							c);
					}
				}
				_slots[j][n_grandkids] = Symbol();
			}
		}

		Sexp *relSexp = pattern->getNthChild(nkids-1);
		if (!relSexp->isAtom()) {
			char c[1000];
			sprintf( c, "ERROR: relation type can't be list? (rule %d)", num_entries);
			throw UnexpectedInputException("PatternMatcherModel::PatternMatcherModel()", c);
		}
		int type = _relationTags->getTagIndex(relSexp->getValue());
		if (type == -1) {
			char c[1000];
			sprintf( c, "ERROR: unknown relation type (rule %d): %s",
				num_entries, relSexp->getValue().to_debug_string());
			throw UnexpectedInputException("PatternMatcherModel::PatternMatcherModel()", c);
		}
		// NB: can't use 0 if we use NGramScoreTable,
		//     so let's add 10000 to everything and shift it up
		type += 10000;

		for (int i0 = 0; !_slots[0][i0].is_null(); i0++) {
		for (int i1 = 0; !_slots[1][i1].is_null(); i1++) {
		for (int i2 = 0; !_slots[2][i2].is_null(); i2++) {
		for (int i3 = 0; !_slots[3][i3].is_null(); i3++) {
		for (int i4 = 0; !_slots[4][i4].is_null(); i4++) {
		for (int i5 = 0; !_slots[5][i5].is_null(); i5++) {
		for (int i6 = 0; !_slots[6][i6].is_null(); i6++) {
		for (int i7 = 0; !_slots[7][i7].is_null(); i7++) {
		for (int i8 = 0; !_slots[8][i8].is_null(); i8++) {
			setPattern(i0, i1, i2, i3, i4,i5, i6, i7, i8, num_entries, pattern_type==FULL, type);
		}}}}}}}}}

		delete pattern;

    }
	stream.close();
}

void PatternMatcherModel::setPattern(int i0, int i1, int i2, int i3, int i4,
									 int i5, int i6, int i7, int i8, int index,
									 bool fullPattern,
									 int type)
{
	Symbol ngram[9];
	ngram[0] = _slots[0][i0];
	ngram[1] = _slots[1][i1];
	ngram[2] = _slots[2][i2];
	ngram[3] = _slots[3][i3];
	ngram[4] = _slots[4][i4];
	ngram[5] = _slots[5][i5];
	ngram[6] = _slots[6][i6];
	ngram[7] = _slots[7][i7];
	ngram[8] = _slots[8][i8];

	int old_type;
	if (fullPattern)
		old_type = static_cast<int>(_fullPatterns->lookup(ngram));
	else old_type = static_cast<int>(_patterns->lookup(ngram));

	if (old_type == 0) {
		if (fullPattern) _fullPatterns->add(ngram, static_cast<float>(type));
		else _patterns->add(ngram, static_cast<float>(type));
	} else if (type == old_type) {
		SessionLogger::warn("repeated_pattern") << "PatternMatcherModel::PatternMatcherModel(): "
			<< " repeated pattern: "
			<< ngram[0].to_debug_string() << " " << ngram[1].to_debug_string() << " "
			<< ngram[2].to_debug_string() << " " << ngram[3].to_debug_string() << " "
			<< ngram[4].to_debug_string() << " " << ngram[5].to_debug_string() << " "
			<< ngram[6].to_debug_string() << " " << ngram[7].to_debug_string() << " "
			<< ngram[8].to_debug_string() << " "
			<< _relationTags->getTagSymbol(type-10000).to_debug_string();
	} else {
		char c[1000];
		sprintf( c, "ERROR: repeated pattern (line %d): %s %s %s %s %s %s %s %s %s-- %s or %s",
			index,
			ngram[0].to_debug_string(), ngram[1].to_debug_string(),
			ngram[2].to_debug_string(), ngram[3].to_debug_string(),
			ngram[4].to_debug_string(), ngram[5].to_debug_string(),
			ngram[6].to_debug_string(), ngram[7].to_debug_string(),
			ngram[8].to_debug_string(),
			_relationTags->getTagSymbol(old_type-10000).to_debug_string(),
			_relationTags->getTagSymbol(type-10000).to_debug_string());
		throw UnexpectedInputException("PatternMatcherModel::PatternMatcherModel()", c);
	}
}


PatternMatcherModel::~PatternMatcherModel() {
	delete _patterns;
	delete _fullPatterns;
}

Symbol PatternMatcherModel::findBestRelationType(PotentialRelationInstance *instance,
											  bool tryLeftMetonymy,
											  bool tryRightMetonymy) {

	Symbol ngram[9];

	Symbol leftEntity = instance->getLeftEntityType();
	Symbol rightEntity = instance->getRightEntityType();
	if (OldRelationFinder::_relationFinderType == OldRelationFinder::EELD) {
		EntityType leftEntityType(instance->getLeftEntityType());
		EntityType rightEntityType(instance->getRightEntityType());

		if (leftEntityType.matchesFAC())
			leftEntity = FAC_symbol;
		else if (leftEntityType.matchesGPE())
			leftEntity = GPE_symbol;
		else if (leftEntityType.matchesLOC())
			leftEntity = LOC_symbol;
		else if (leftEntityType.matchesPER())
			leftEntity = PER_symbol;
		else if (leftEntityType.matchesORG())
			leftEntity = ORG_symbol;

		if (rightEntityType.matchesFAC())
			rightEntity = FAC_symbol;
		else if (rightEntityType.matchesGPE())
			rightEntity = GPE_symbol;
		else if (rightEntityType.matchesLOC())
			rightEntity = LOC_symbol;
		else if (rightEntityType.matchesPER())
			rightEntity = PER_symbol;
		else if (rightEntityType.matchesORG())
			rightEntity = ORG_symbol;
	}

	ngram[0] = instance->getStemmedPredicate();
	ngram[1] = leftEntity;
	ngram[2] = rightEntity;
	ngram[3] = instance->getLeftRole();
	ngram[4] = instance->getRightRole();
	ngram[5] = instance->getNestedRole();
	ngram[6] = instance->getLeftHeadword();
	ngram[7] = instance->getRightHeadword();
	ngram[8] = instance->getNestedWord();

	if (OldRelationFinder::DEBUG) {
		OldRelationFinder::_debugStream << L"INSTANCE: ";
		for (int i = 0; i < 9; i++)
			OldRelationFinder::_debugStream << ngram[i].to_string() << L" ";
		OldRelationFinder::_debugStream << L"\n";
	}

	Symbol extra_ngram[3];
	extra_ngram[0] = instance->getLeftHeadword();
	extra_ngram[1] = instance->getRightHeadword();
	extra_ngram[2] = instance->getNestedWord();

	Symbol symResult = testForFullPattern(ngram, extra_ngram);
	if (symResult != _relationTags->getNoneTag())
		return symResult;
	int result = static_cast<int>(_patterns->lookup(ngram));
	if (result > 0)
		return returnType(ngram, result - 10000);

	// first try replacing left role
	ngram[3] = wildcardSym;
	symResult = testForFullPattern(ngram, extra_ngram);
	if (symResult != _relationTags->getNoneTag())
		return symResult;
	result = static_cast<int>(_patterns->lookup(ngram));
	if (result > 0)
		return returnType(ngram, result - 10000);
	ngram[3] = instance->getLeftRole();

	// now right role
	ngram[4] = wildcardSym;
	symResult = testForFullPattern(ngram, extra_ngram);
	if (symResult != _relationTags->getNoneTag())
		return symResult;
	result =  static_cast<int>(_patterns->lookup(ngram));
	if (result > 0)
		return returnType(ngram, result - 10000);
	ngram[4] = instance->getRightRole();

	// now predicate
	ngram[0] = wildcardSym;
	symResult = testForFullPattern(ngram, extra_ngram);
	if (symResult != _relationTags->getNoneTag())
		return symResult;
	result = static_cast<int>(_patterns->lookup(ngram));
	if (result > 0)
		return returnType(ngram, result - 10000);
	ngram[0] = instance->getStemmedPredicate();

	Symbol metonymy_result = _relationTags->getNoneTag();
	if (tryLeftMetonymy && EntityType::isValidEntityType(instance->getLeftEntityType())) {
		EntityType leftEntityType(instance->getLeftEntityType());
		if (leftEntityType.matchesORG()) {
			if (_personLikeOrgWords->lookup(instance->getLeftHeadword())) {
				instance->setLeftEntityType(EntityType::getPERType().getName());
				metonymy_result = findBestRelationType(instance, false, tryRightMetonymy);
			} else if (_facilityLikeOrgWords->lookup(instance->getLeftHeadword())) {
				instance->setLeftEntityType(EntityType::getFACType().getName());
				metonymy_result = findBestRelationType(instance, false, tryRightMetonymy);
			} else if (OldRelationFinder::_relationFinderType == OldRelationFinder::EELD) {
				instance->setLeftEntityType(EntityType::getFACType().getName());
				metonymy_result = findBestRelationType(instance, false, tryRightMetonymy);
			}
			if (metonymy_result != _relationTags->getNoneTag())
				return metonymy_result;
			instance->setLeftEntityType(EntityType::getORGType().getName());
		} else if (OldRelationFinder::_relationFinderType == OldRelationFinder::EELD &&
				   leftEntityType.matchesFAC())
		{
			instance->setLeftEntityType(EntityType::getORGType().getName());
			metonymy_result = findBestRelationType(instance, false, tryRightMetonymy);
			if (metonymy_result != _relationTags->getNoneTag())
				return metonymy_result;
			instance->setLeftEntityType(EntityType::getFACType().getName());
		}
	}

	if (tryRightMetonymy && EntityType::isValidEntityType(instance->getRightEntityType())) {
		EntityType rightEntityType(instance->getRightEntityType());
		if (rightEntityType.matchesORG()) {
			if (_personLikeOrgWords->lookup(instance->getRightHeadword())) {
				instance->setRightEntityType(EntityType::getPERType().getName());
				metonymy_result = findBestRelationType(instance, tryLeftMetonymy, false);
			} else if (_facilityLikeOrgWords->lookup(instance->getRightHeadword())) {
				instance->setRightEntityType(EntityType::getFACType().getName());
				metonymy_result = findBestRelationType(instance, tryLeftMetonymy, false);
			} else if (OldRelationFinder::_relationFinderType == OldRelationFinder::EELD) {
				instance->setRightEntityType(EntityType::getFACType().getName());
				metonymy_result = findBestRelationType(instance, tryLeftMetonymy, false);
			}
			if (metonymy_result != _relationTags->getNoneTag())
				return metonymy_result;
			instance->setRightEntityType(EntityType::getORGType().getName());
		} else if (OldRelationFinder::_relationFinderType == OldRelationFinder::EELD &&
			       rightEntityType.matchesFAC())
		{
			instance->setRightEntityType(EntityType::getORGType().getName());
			metonymy_result = findBestRelationType(instance, tryLeftMetonymy, false);
			if (metonymy_result != _relationTags->getNoneTag())
				return metonymy_result;
			instance->setLeftEntityType(EntityType::getFACType().getName());
		}
	}

	return _relationTags->getNoneTag();
}

Symbol PatternMatcherModel::testForFullPattern(Symbol *ngram, Symbol *extraNgram) {
	int result = static_cast<int>(_fullPatterns->lookup(ngram));
	if (result > 0)
		return returnType(ngram, result-10000, true);
	for (int i = 6; i < 9; i++) {
		ngram[i] = wildcardSym;
		result = static_cast<int>(_fullPatterns->lookup(ngram));
        if (result > 0)
			return returnType(ngram, result-10000, true);
		ngram[i] = extraNgram[i-6];
	}
	ngram[6] = wildcardSym;
	ngram[7] = wildcardSym;
	ngram[8] = wildcardSym;
	for (int j = 6; j < 9; j++) {
		ngram[j] = extraNgram[j-6];
		result = static_cast<int>(_fullPatterns->lookup(ngram));
        if (result > 0)
			return returnType(ngram, result-10000, true);
		ngram[j] = wildcardSym;
	}
	ngram[6] = extraNgram[0];
	ngram[7] = extraNgram[1];
	ngram[8] = extraNgram[2];

	return _relationTags->getNoneTag();
}

Symbol PatternMatcherModel::returnType(Symbol *ngram, int result, bool full) {
	if (OldRelationFinder::DEBUG) {
		if (full)
			OldRelationFinder::_debugStream << L"FULL PATTERN FIRED: ";
		else OldRelationFinder::_debugStream << L"REGULAR PATTERN FIRED: ";
		for (int i = 0; i < 9; i++) {
			if (i < 6 || full)
				OldRelationFinder::_debugStream << ngram[i].to_string() << L" ";
		}
		OldRelationFinder::_debugStream << L"\n";
	}
	return _relationTags->getTagSymbol(result);
}
