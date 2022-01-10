// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/common/ch_WordConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/names/discmodel/PIdFModel.h"
#include "Chinese/names/ch_IdFNameRecognizer.h"
#include "Chinese/names/discmodel/ch_PIdFNameRecognizer.h"
#include "Chinese/names/ch_NameRecognizer.h"

#include "Generic/common/InputUtil.h"
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>


using namespace std;


ChineseNameRecognizer::ChineseNameRecognizer() : 
	_idfNameRecognizer(0), _pidfNameRecognizer(0), _docTheory(0)
{
	std::string name_finder_mode = ParamReader::getRequiredParam("name_finder");
	if (name_finder_mode == "idf" || name_finder_mode == "IDF") {
		_name_finder = IDF_NAME_FINDER;
	}
	else if (name_finder_mode == "pidf" || name_finder_mode == "PIDF") {
		_name_finder = PIDF_NAME_FINDER;
	}
	else {
		throw UnexpectedInputException(
			"ChineseNameRecognizer::ChineseNameRecognizer()",
			"name-finder parameter not specified. (Should be 'idf' or 'pidf'.)");
	}

	// This should only be turned on if sentence selection is the only thing you are doing!
	if (ParamReader::isParamTrue("print_sentence_selection_info"))		
		PRINT_SENTENCE_SELECTION_INFO = true;
	else PRINT_SENTENCE_SELECTION_INFO = false;

	if (_name_finder == IDF_NAME_FINDER) {
		_idfNameRecognizer = _new ChineseIdFNameRecognizer();
	}
	else {
		_pidfNameRecognizer = _new PIdFNameRecognizer();
	}

	// load dictionary (force entity types)
	if (ParamReader::hasParam("names_with_forced_entity_types")) {
		typedef std::vector<std::wstring> wstring_vector_t;
		BOOST_FOREACH(wstring_vector_t vec, InputUtil::readColumnFileIntoSet(ParamReader::getRequiredParam("names_with_forced_entity_types"), false, L"\t")) {
			if (vec.size() == 0)
				continue;
			std::wstring left = vec.at(0);
			if (left.size() == 0)
				continue;
			if (left.at(0) == L'#')
				continue;
			if (vec.size() != 2) {
				std::wstringstream msg;
				msg << L"Forced entity types rows must have exactly two elements per row (tab-separated): ";
				for (size_t i = 0; i < vec.size(); i++) {
					msg << vec.at(i) << "\t";
				}
				throw UnexpectedInputException("ChineseNameRecognizer::ChineseNameRecognizer()", msg);
			}
			std::wstring right = vec.at(1);
			boost::to_upper(right);
			if (_forcedTypeNames.find(right) != _forcedTypeNames.end()) {
				std::wstringstream msg;
				msg << L"Same entry appears multiple times in forced entity types list: " << left << L"\t" << right;
				throw UnexpectedInputException("EnglishNameRecognizer::EnglishNameRecognizer()", msg);
			}
			_forcedTypeNames[right] = EntityType(Symbol(left));	// This will throw an error if the entity type is invalid		
		}
	}
}


void ChineseNameRecognizer::resetForNewSentence(const Sentence *sentence) {
	if (_name_finder == IDF_NAME_FINDER) {
		_idfNameRecognizer->resetForNewSentence(sentence);
	}
	else {
		_pidfNameRecognizer->resetForNewSentence(sentence);
	}
}

void ChineseNameRecognizer::cleanUpAfterDocument() {
	if (_name_finder == IDF_NAME_FINDER) {
		_idfNameRecognizer->cleanUpAfterDocument();
	}
	else {
		_pidfNameRecognizer->cleanUpAfterDocument();
	}
}

void ChineseNameRecognizer::resetForNewDocument(DocTheory *docTheory) {
	_docTheory = docTheory;
	if (_name_finder == IDF_NAME_FINDER) {
		_idfNameRecognizer->resetForNewDocument(docTheory);
	} else if (_name_finder == PIDF_NAME_FINDER) {
		_pidfNameRecognizer->resetForNewDocument(docTheory);
	}
}

int ChineseNameRecognizer::getNameTheories(NameTheory **results, int max_theories, 
									TokenSequence *tokenSequence)
{
	_sent_no = tokenSequence->getSentenceNumber();

	// make the contents of SPEAKER and POSTER regions automatically into person names
	if (_docTheory->isSpeakerSentence(_sent_no)) {

		// although not if there is a comma involved (e.g. "BOB JONES, REPORTER FOR CNN")
		bool found_comma = false;
		for (int i = 0; i < tokenSequence->getNTokens(); i++) {
			if (tokenSequence->getToken(i)->getSymbol() == ChineseWordConstants::CH_COMMA ||
				tokenSequence->getToken(i)->getSymbol() == ChineseWordConstants::ASCII_COMMA ||
				tokenSequence->getToken(i)->getSymbol() == ChineseWordConstants::FULL_COMMA) 
			{
				found_comma = true;
				break;
			}
		}

		if (!found_comma) {

			results[0] = _new NameTheory(tokenSequence);
			results[0]->takeNameSpan(_new NameSpan(0, tokenSequence->getNTokens() - 1,
				EntityType::getPERType()));

			// faked
			if (PRINT_SENTENCE_SELECTION_INFO)
				results[0]->setScore(100000);

			return 1;

		}
	}

	int n_results;
	if (_name_finder == IDF_NAME_FINDER) {
		n_results = _idfNameRecognizer->getNameTheories(results, max_theories,
												   tokenSequence);
	}
	else { // PIDF_NAME_FINDER
		n_results = _pidfNameRecognizer->getNameTheories(results, max_theories,
													tokenSequence);
	}

	for (int i = 0; i < n_results; i++) 
		postProcessNameTheory(results[i], tokenSequence);

	// force name types by dictionary
	forceNameTypes(results, n_results);

	return n_results;
}

// concatenate symbols with spaces
std::wstring ChineseNameRecognizer::getNameString(int start, int end, TokenSequence* tokenSequence)
{
	wchar_t nameStr[MAX_SENTENCE_LENGTH];
	int i;
	int offset = 0;
	for (i = start; i <= end; i++) {
		Symbol word = tokenSequence->getToken(i)->getSymbol();
		offset += swprintf(nameStr+offset, MAX_SENTENCE_LENGTH - offset,
						   L"%ls", word.to_string());
		if (i < end)
			offset += swprintf(nameStr+offset, MAX_SENTENCE_LENGTH - offset,
							   L" ");
	}

	return wstring(nameStr);
}

void ChineseNameRecognizer::forceNameTypes(NameTheory **results, int n_results)
{
	for (int i=0; i<n_results; i++) 
	{
		for(int j=0; j<results[i]->getNNameSpans(); j++) {
			std::wstring strName = getNameString(results[i]->getNameSpan(j)->start, results[i]->getNameSpan(j)->end, (TokenSequence*)results[i]->getTokenSequence());
//			std::wcout << strName << std::endl;

			_force_type_map_t::const_iterator iter = _forcedTypeNames.find(strName);
			if (iter != _forcedTypeNames.end()) {
				EntityType et = (*iter).second;

				SessionLogger::info("fix_name_by_force") << L"Changing " << std::wstring(strName.c_str()) << L" by force from "
					<< results[i]->getNameSpan(j)->type.getName().to_string()	<< L" to " << et.getName().to_string() << L"\n";

				results[i]->getNameSpan(j)->type = et;
			}
		}
	}
}

int ChineseNameRecognizer::getNameTheoriesForSentenceBreaking(NameTheory **results, int max_theories, 
									TokenSequence *tokenSequence)
{
	if (_name_finder == IDF_NAME_FINDER) {
		return _idfNameRecognizer->getNameTheories(results, max_theories,
												   tokenSequence);
	}
	else { // PIDF_NAME_FINDER
		return _pidfNameRecognizer->getNameTheories(results, max_theories,
													tokenSequence);
	}
}
