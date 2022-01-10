// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "theories/Entity.h"
#include "theories/NameTheory.h"
#include "theories/DocTheory.h"
#include "theories/TokenSequence.h"
#include "names/IdFSentenceTokens.h"
#include "names/IdFDecoder.h"
#include "common/Symbol.h"
#include "common/SymbolHash.h"
#include "common/SessionLogger.h"
#include "English/common/en_WordConstants.h"
#include "names/NameClassTags.h"
#include "common/UnexpectedInputException.h"
#include "common/ParamReader.h"
#include "theories/EntityType.h"
#include "names/IdFSentence.h"
#include "English/common/en_NationalityRecognizer.h"
#include "English/names/en_IdFNameRecognizer.h"

#include <boost/foreach.hpp>

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


#define MAX_IDF_THEORIES 30

using namespace std;


EnglishIdFNameRecognizer::EnglishIdFNameRecognizer() : _num_names(0) {

	std::string debug_buffer = ParamReader::getRequiredParam("name_debug");
	DEBUG = (debug_buffer != "");
	if (DEBUG) _debugStream.open(debug_buffer.c_str());

	std::string model_prefix = ParamReader::getRequiredParam("new_idf_model");
	_nameClassTags = _new NameClassTags();

	string list_file = ParamReader::getParam("idf_list_file");
	if (!list_file.empty())
		_defaultDecoder = _new IdFDecoder(model_prefix.c_str(), _nameClassTags, list_file.c_str());
	else _defaultDecoder = _new IdFDecoder(model_prefix.c_str(), _nameClassTags);

	_decoder = _defaultDecoder;

	std::string lc_model_prefix = ParamReader::getParam("lowercase_idf_model");	
	if (!lc_model_prefix.empty()) {
		if (!list_file.empty())
			_lowerCaseDecoder = _new IdFDecoder(lc_model_prefix.c_str(), _nameClassTags, list_file.c_str());
		else _lowerCaseDecoder = _new IdFDecoder(lc_model_prefix.c_str(), _nameClassTags);
	} else _lowerCaseDecoder = 0;

	std::string uc_model_prefix = ParamReader::getParam("uppercase_idf_model");	
	if (!uc_model_prefix.empty()) {
		if (!list_file.empty())
			_upperCaseDecoder = _new IdFDecoder(uc_model_prefix.c_str(), _nameClassTags, list_file.c_str());
		else _upperCaseDecoder = _new IdFDecoder(uc_model_prefix.c_str(), _nameClassTags);
	} else _upperCaseDecoder = 0;


	//if (!_decoder->checkNameClassTagConsistency())
	//	throw UnexpectedInputException("EnglishIdFNameRecognizer::EnglishIdFNameRecognizer()",
	//								   "model tags inconsistent with name class tags");
	_sentenceTokens = _new IdFSentenceTokens();
	_IdFTheories = _new IdFSentenceTheory* [MAX_IDF_THEORIES];

	SPLIT_ORGS = ParamReader::isParamTrue("split_gpes_from_orgs");

	Symbol ORG = EntityType::getORGType().getName();
	wchar_t str[101];
	swprintf(str, 100, L"%ls-ST", EntityType::getORGType().getName().to_string());
	_ORG_ST_index = _nameClassTags->getIndexForTag(Symbol(str));
	swprintf(str, 100, L"%ls-ST", EntityType::getGPEType().getName().to_string());
	_GPE_ST_index = _nameClassTags->getIndexForTag(Symbol(str));

	std::string nations_file = ParamReader::getParam("simple_nations_file");
	if (!nations_file.empty()) {
		_nationsTable = _new SymbolHash(nations_file.c_str());
	} else _nationsTable = 0;

	std::string splits_file = ParamReader::getParam("splittable_gpe_org_names");
	if (!splits_file.empty()) {
		_splittableOrgsTable = _new IdFListSet(splits_file.c_str());
	} else _splittableOrgsTable = 0;

}


void EnglishIdFNameRecognizer::resetForNewSentence() {
}

void EnglishIdFNameRecognizer::cleanUpAfterDocument() {
	// heap-allocated word lengths need cleanup.
	// everything else can persist, with just the counter reset
	int i;
	for (i=0; i < _num_names; i++)
		delete [] _nameWords[i].words;
	_num_names = 0;
}

void EnglishIdFNameRecognizer::resetForNewDocument(DocTheory *docTheory) {
	_decoder = _defaultDecoder;
	if (docTheory != 0) {
		int doc_case = docTheory->getDocumentCase();
		if (doc_case == DocTheory::LOWER && _lowerCaseDecoder != 0) {
			SessionLogger::dbg("low_idf_dec_0") << "Using lowercase IdF decoder\n";
			_decoder = _lowerCaseDecoder;
		} else if (doc_case == DocTheory::UPPER && _upperCaseDecoder != 0) {
			SessionLogger::dbg("upp_idf_dec_0") << "Using uppercase IdF decoder\n";
			_decoder = _upperCaseDecoder;
		}
	}
}

int EnglishIdFNameRecognizer::getNameTheories(NameTheory **results, int max_theories,
									   TokenSequence *tokenSequence)
{
	if (max_theories > MAX_IDF_THEORIES)
		max_theories = MAX_IDF_THEORIES;

	int n_tokens = tokenSequence->getNTokens();

	for (int i = 0; i < n_tokens; i++) {
		_sentenceTokens->setWord(i, tokenSequence->getToken(i)->getSymbol());
	}
	_sentenceTokens->setLength(n_tokens);

	int num_theories = _decoder->decodeSentenceNBest(_sentenceTokens,
				_IdFTheories, max_theories);

	if (num_theories == 0) {
		// should never happen, but...
		_IdFTheories[0] = new IdFSentenceTheory(_sentenceTokens->getLength(),
			_nameClassTags->getNoneStartTagIndex());
		num_theories = 1;
	}

	if (DEBUG)  {
//		_decoder->printTrellis(_sentenceTokens, _debugStream);
		_debugStream << _nameClassTags->to_string(_sentenceTokens, _IdFTheories[0]);
		_debugStream << L"\n";
		_debugStream << _nameClassTags->to_enamex_sgml_string(_sentenceTokens, _IdFTheories[0]);
		_debugStream << L"\n\n";
	}

	for (int theorynum = 0; theorynum < num_theories; theorynum++) {
		if (DEBUG)
			_debugStream << _nameClassTags->to_sgml_string(_sentenceTokens,
				_IdFTheories[theorynum]) << L"\n";

		// find names that should be split
		splitOrgGPENames(_IdFTheories[theorynum]);

		if (DEBUG)
			_debugStream << L"SPLIT: " << _nameClassTags->to_sgml_string(_sentenceTokens,
				_IdFTheories[theorynum]) << L"\n";

		int name_count = _nameClassTags->getNumNamesInTheory(_IdFTheories[theorynum]);

		bool in_name = false;
		int name_start = 0;

		// find nationality names using our list
		std::vector<int> nat_name_tokens;
		for (int i = 0; i < n_tokens; i++) {
			int tag = _IdFTheories[theorynum]->getTag(i);
			if (_nameClassTags->isStart(tag)) {
				if (in_name) {
/*					// namefinder found a single-token name, and it's a nat,
					// so call it a nat
					if (name_start == i - 1 &&
						EnglishNationalityRecognizer::isNationalityWord(
							tokenSequence->getToken(i-1)->getSymbol()))
					{
						_IdFTheories[theorynum]->setTag(i - 1,
							_nameClassTags->getIndexForTag(Symbol(L"GPE-ST")));
					}
*/
					in_name = false;
				}
				if (_nameClassTags->isMeaningfulNameTag(tag)) {
					name_start = i;
					in_name = true;
				}
			}
			if (!in_name) {
				// namefinder says nothing about this token, and it's a nat,
				// so add it to nat list
				if (EnglishNationalityRecognizer::isNationalityWord(
						tokenSequence->getToken(i)->getSymbol()))
				{
					nat_name_tokens.push_back(i);
				}
			}
		}

		NameTheory* pTheory = _new NameTheory(tokenSequence);
		pTheory->setScore(static_cast<float>(_IdFTheories[theorynum]->getBestPossibleScore()));

		// add the splittable ORG names we found earlier
		for (int f = 0; f < _num_splittable_orgs; f++) {
			pTheory->takeNameSpan(
				_new NameSpan(_splittableOrgStarts[f], _splittableOrgEnds[f], EntityType::getOtherType()));

			if (DEBUG) {
				_debugStream << L"Found splittable name: ";
				_debugStream <<
					tokenSequence->getToken(_splittableOrgStarts[f])->getSymbol().to_string();
				_debugStream << L"\n";
			}
		}

		// add the nat names we found earlier
		BOOST_FOREACH(int nat_name_token, nat_name_tokens) {
			NameSpan *span = _new NameSpan(nat_name_token, nat_name_token, EntityType::getNationalityType());
			fixName(span, _sentenceTokens);
			pTheory->takeNameSpan(span);

			if (DEBUG) {
				_debugStream << L"Found untagged nationality word: ";
				_debugStream << tokenSequence->getToken(nat_name_token)->getSymbol().to_string();
				_debugStream << L" tagging as " << EntityType::getNationalityType().getName().to_string();
				_debugStream << L"\n";
			}
		}
		
		in_name = false;
		int name_index = pTheory->getNNameSpans();
		for (int k = 0; k < n_tokens; k++) {
			int tag = _IdFTheories[theorynum]->getTag(k);
			if (_nameClassTags->isStart(tag)) {
				if (in_name) {
					pTheory->getNameSpan(name_index)->end = k - 1;
					fixName(pTheory->getNameSpan(name_index), _sentenceTokens);

					if (DEBUG) {
						_debugStream << pTheory->getNameSpan(name_index)->type.getName().to_string() << L" ";
						_debugStream << pTheory->getNameSpan(name_index)->start << L" ";
						_debugStream << pTheory->getNameSpan(name_index)->end << L"\n";
					}
					name_index++;
					in_name = false;
				}
				if (_nameClassTags->isMeaningfulNameTag(tag)) {
					// The name span's end (which we set to -1 here) will get filled in
					// during a subsequent pass through this loop (where in_name=true).
					pTheory->takeNameSpan(
						_new NameSpan(k, -1, _nameClassTags->getReducedTagSymbol(_IdFTheories[theorynum]->getTag(k))));
					in_name = true;
				}
			}
		}
		if (in_name) {
			pTheory->getNameSpan(name_index)->end = n_tokens - 1;
			fixName(pTheory->getNameSpan(name_index), _sentenceTokens);
			if (DEBUG) {
				_debugStream << pTheory->getNameSpan(name_index)->type.getName().to_string() << L" ";
				_debugStream << pTheory->getNameSpan(name_index)->start << L" ";
				_debugStream << pTheory->getNameSpan(name_index)->end << L"\n";
			}
			name_index++;
			in_name = false;
		}

		results[theorynum] = pTheory;
	}

	if (DEBUG) _debugStream << "\n";
	for (int theory_num = 0; theory_num < num_theories; theory_num++) {
		delete _IdFTheories[theory_num];
	}

	return num_theories;
}

void EnglishIdFNameRecognizer::fixName(NameSpan* span, IdFSentenceTokens* sent)
{
	// first create the symbol of the entire name
	Symbol nameSym = getNameSymbol(span->start, span->end, sent);
	int span_length = (span->end+1 - span->start);
	EntityType type;
	int i;
	bool foundMatch = false;
	bool exactMatch = false;
	// exact match phase
	for (i=0; i < _num_names; i++) {
		if (_names[i] == nameSym) {
			type = _types[i];
			foundMatch = true;
			exactMatch = true;
			break;
		}
	}
	// inexact match phase
	if (!foundMatch) {
		for (i=0; i < _num_names; i++) {
			// name must be longer to be inexact
			if (_nameWords[i].num_words <= span_length)
				continue;

			// GPE/LOC/NATIONALITY matching to non-GPE/LOC/NATIONALITY can
			// only match to PER, and can only match as a terminating substring
			if ((span->type.matchesGPE() || span->type.matchesLOC() || span->type.isNationality()) &&
				(!_types[i].matchesGPE() && !_types[i].matchesLOC() && !_types[i].isNationality()))
			{
				if (!_types[i].matchesPER())
					continue;
				int start_idx = _nameWords[i].num_words-span_length;
				Symbol subName = getNameSymbol(start_idx, _nameWords[i].num_words-1, _nameWords[i]);
				if (subName == nameSym) {
					type = _types[i];
					foundMatch = true;
					break;
				}
			}
			// for all others, we must check all substring-by-word combinations
			else {
				// consider each span of the proper length
				int j;
				for (j = 0; j <= _nameWords[i].num_words-span_length; j++) {
					Symbol subName = getNameSymbol(j, j+(span_length-1), _nameWords[i]);
					if (subName == nameSym) {
						type = _types[i];
						foundMatch = true;
						break;
					}
				}
				if (foundMatch)
					break;
			}
		}
	}

	// matched the name - in some way
	if (foundMatch) {
		// fix name so it's consistent
		if (type != span->type) {

			if (DEBUG) {
				_debugStream << L"Changing " << nameSym.to_string() << L" from ";
				_debugStream << span->type.getName().to_string() << L" to ";
				_debugStream << type.getName().to_string() << L"\n";
			}
			span->type = type;
		}
	}
	// didn't match perfectly, so we have a new name
	if (!exactMatch) {
		// can't save any more names, so warn, but continue
		if (_num_names >= MAX_DOC_NAMES) {
			SessionLogger::warn("document_name_limit") << "Document exceeds name limit of "
				   << MAX_DOC_NAMES << "\n";
		}
		else {
			_names[_num_names] = nameSym;
			_types[_num_names] = span->type;
			_nameWords[_num_names].num_words = span_length;
			_nameWords[_num_names].words = _new Symbol[span_length];
			int index = 0;
			for (i=span->start; i <= span->end; i++)
				_nameWords[_num_names].words[index++] = sent->getWord(i);
			_num_names++;
		}
	}
	return;
}

// concatenate symbols with spaces, and make them all uppercase

Symbol EnglishIdFNameRecognizer::getNameSymbol(int start, int end, IdFSentenceTokens* sent)
{
	wchar_t nameStr[MAX_SENTENCE_LENGTH];
	int i;
	int offset = 0;
	for (i = start; i <= end; i++) {
		Symbol word = sent->getWord(i);
		offset += swprintf(nameStr+offset, MAX_SENTENCE_LENGTH - offset,
						   L"%ls", word.to_string());
		if (i < end)
			offset += swprintf(nameStr+offset, MAX_SENTENCE_LENGTH - offset,
							   L" ");
	}
	// transform to uppercase
	for (i = 0; i < offset; i++)
		nameStr[i] = towupper(nameStr[i]);
	return Symbol(nameStr);
}

Symbol EnglishIdFNameRecognizer::getNameSymbol(int start, int end, NameSet set)
{
	wchar_t nameStr[MAX_SENTENCE_LENGTH];
	int i;
	int offset = 0;
	for (i = start; i <= end; i++) {
		Symbol word = set.words[i];
		offset += swprintf(nameStr+offset, MAX_SENTENCE_LENGTH - offset,
						   L"%ls", word.to_string());
		if (i < end)
			offset += swprintf(nameStr+offset, MAX_SENTENCE_LENGTH - offset,
							   L" ");
	}
	// transform to uppercase
	for (i = 0; i < offset; i++)
		nameStr[i] = towupper(nameStr[i]);
	return Symbol(nameStr);
}

static Symbol bankSym(L"bank");
static Symbol universitySym(L"university");
static Symbol republicSym(L"republic");
static Symbol russianSym(L"Russian");
static Symbol federationSym(L"Federation");
void EnglishIdFNameRecognizer::splitOrgGPENames(IdFSentenceTheory *theory)
{
	_num_splittable_orgs = 0;

	if (!SPLIT_ORGS)
		return;

	for (int k = 0; k < _sentenceTokens->getLength(); k++) {
		int tag = theory->getTag(k);
		if (_nameClassTags->isStart(tag) && _nameClassTags->isMeaningfulNameTag(tag)) {
			Symbol sym = _nameClassTags->getReducedTagSymbol(tag);
			if (!EntityType(sym).matchesORG())
				continue;

			// start-end [inclusive]
			int start = k;
			int end = 0;
			for (end = k+1; end < _sentenceTokens->getLength(); end++) {
				if (_nameClassTags->isStart(theory->getTag(end)))
					break;
			}
			end--;

			if (start == end)
				continue;

			if (end - 1 > start &&
				_sentenceTokens->getWord(end - 1) == EnglishWordConstants::OF &&
				_nationsTable != 0 &&
				_nationsTable->lookup(lowercaseSymbol(_sentenceTokens->getWord(end))))
			{
				Symbol lowerSym = lowercaseSymbol(_sentenceTokens->getWord(end - 2));
				if (lowerSym != bankSym &&
					lowerSym != universitySym &&
					lowerSym != republicSym)
				{
					theory->setTag(end - 1, _nameClassTags->getNoneStartTagIndex());
					theory->setTag(end, _GPE_ST_index);
				}
			} else if (end - 3 > start &&
					   _sentenceTokens->getWord(end - 3) == EnglishWordConstants::OF &&
					   _sentenceTokens->getWord(end - 2) == EnglishWordConstants::THE &&
					   _sentenceTokens->getWord(end - 1) == russianSym &&
					   _sentenceTokens->getWord(end - 0) == federationSym)
			{
				Symbol lowerSym = lowercaseSymbol(_sentenceTokens->getWord(end - 4));
				if (lowerSym != bankSym &&
					lowerSym != universitySym &&
					lowerSym != republicSym)
				{
					theory->setTag(end - 3, _nameClassTags->getNoneStartTagIndex());
					theory->setTag(end - 2, _nameClassTags->getNoneStartTagIndex() + 1);
					theory->setTag(end - 1, _GPE_ST_index);
					theory->setTag(end, _GPE_ST_index + 1);
				}
			} else if (EnglishNationalityRecognizer::isNationalityWord(_sentenceTokens->getWord(start)) &&
				isSplittableOrgName(start + 1, end))
			{
				theory->setTag(start, _GPE_ST_index);
				theory->setTag(start + 1, _ORG_ST_index);
				_splittableOrgStarts[_num_splittable_orgs] = start;
				_splittableOrgEnds[_num_splittable_orgs] = end;
				_num_splittable_orgs++;
			} else if (start + 1 < end &&
				_sentenceTokens->getWord(start) == EnglishWordConstants::THE &&
				EnglishNationalityRecognizer::isNationalityWord(_sentenceTokens->getWord(start + 1)) &&
				isSplittableOrgName(start + 2, end))
			{
				theory->setTag(start, _nameClassTags->getNoneStartTagIndex());
				theory->setTag(start + 1, _GPE_ST_index);
				theory->setTag(start + 2, _ORG_ST_index);
				_splittableOrgStarts[_num_splittable_orgs] = start;
				_splittableOrgEnds[_num_splittable_orgs] = end;
				_num_splittable_orgs++;
			}
			k = end;
		}
	}
}

static Symbol ministrySym(L"ministry");
static Symbol officeSym(L"office");
static Symbol partySym(L"party");
bool EnglishIdFNameRecognizer::isSplittableOrgName(int start, int end) {
	Symbol firstword = lowercaseSymbol(_sentenceTokens->getWord(start));
	Symbol lastword = lowercaseSymbol(_sentenceTokens->getWord(end));

	if (firstword == ministrySym)
		return true;

	if (lastword == ministrySym ||
		lastword == officeSym ||
		lastword == partySym)
		return true;

	if (_splittableOrgsTable == 0)
		return false;

	int name_found = _splittableOrgsTable->isListMember(_sentenceTokens, start);
	if (name_found - 1 == end - start)
		return true;

	return false;
}

Symbol EnglishIdFNameRecognizer::lowercaseSymbol(Symbol sym) {
	wstring str = sym.to_string();
	wstring::size_type length = str.length();
    for (size_t i = 0; i < length; ++i) {
        str[i] = towlower(str[i]);
	}
	return Symbol(str.c_str());
}
