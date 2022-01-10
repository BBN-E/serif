// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Chinese/edt/ch_NameLinkFunctions.h"
#include "Chinese/edt/AliasTable.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/AbbrevTable.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include <boost/scoped_ptr.hpp>


SymbolHash ChineseNameLinkFunctions::_stopWords(100);
bool ChineseNameLinkFunctions::_is_initialized = false;

bool ChineseNameLinkFunctions::populateAcronyms(const Mention *mention, EntityType linkType) {
	AliasTable *aliasTable = AliasTable::getInstance();

	const size_t MAX_KEY_LENGTH = 250;
	const int MAX_KEY_WORDS = 32;
	const int MAX_ALIASES = 16;
	wchar_t search_key[MAX_KEY_LENGTH];
	Symbol words[MAX_KEY_WORDS], aliases[MAX_ALIASES];
	int i, nAliases = 0;
	size_t key_len = 0;

	int nWords = mention->getHead()->getTerminalSymbols(words, MAX_KEY_WORDS);
	search_key[0] = L'\0';
	for (i = 0; i < nWords && key_len < MAX_KEY_LENGTH; i++) {
		wcscat(search_key, words[i].to_string());
		key_len += wcslen(words[i].to_string());
	}
	if (i < nWords) {
		SessionLogger::warn("name_link") << "ChineseNameLinkFunctions::populateAcronyms(): "
								<< "terminal symbols exceed MAX_KEY_LENGTH\n";
	}

	nAliases = aliasTable->lookup(Symbol(search_key), aliases, MAX_ALIASES);

	if (nAliases > 0) {
		for (i = 0; i < nAliases; i++) {
			AbbrevTable::add(&aliases[i], 1, words, nWords);
		}
	}

	return (nAliases > 0);

}

void ChineseNameLinkFunctions::destroyDataStructures() {
	AliasTable::destroyInstance();
}

void ChineseNameLinkFunctions::recomputeCounts(CountsTable &inTable,
										CountsTable &outTable,
										int &outTotalCount)
{
	// Default behavior: just recopy counts from inTable.
	// No need to resolveSymbols in AbbrevTable to recompute
	// because we don't want to back resolve (link a name
	// to a *previously* seen potential short form).
	outTotalCount = 0;
	outTable.cleanup();
	for(CountsTable::iterator iterator = inTable.begin(); iterator!=inTable.end(); ++iterator) {
		Symbol thisWord = iterator.value().first;
		int thisCount = iterator.value().second;
		outTable.add(thisWord, thisCount);
		outTotalCount += thisCount;
	}
}

int ChineseNameLinkFunctions::getLexicalItems(Symbol words[], int nWords,
									   Symbol results[],  int max_results)
{
	int nResults = 0;
	wchar_t sym[2];
	sym[1] = L'\0';

	if (!_is_initialized) {
		
		std::string file = ParamReader::getRequiredParam("stopword_file");
		loadStopWords(file.c_str());
		_is_initialized = true;
	}

	for (int i = 0; i < nWords; i++) {
		const wchar_t* word = words[i].to_string();
		for (int j = 0; word[j] != '\0'; j++) {
			if (nResults == max_results) {
				SessionLogger::warn("name_link") << "ChineseNameLinkFunctions::getLexicalItems(): "
					<< "number of lexical items exceeds max_results: truncating.\n";
				break;
			}
			sym[0] = word[j];
			Symbol s = Symbol(sym);
			if (_stopWords.lookup(s))
				continue;
			results[nResults++] = s;
		}
	}

	return nResults;
}

void ChineseNameLinkFunctions::loadStopWords(const char *file) {
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(file));
	UTF8InputStream& stream(*stream_scoped_ptr);
	int size = -1;
	stream >> size;
	if (size < 0)
		throw UnexpectedInputException("NameLinkerTrainer::loadStopWords()",
									   "couldn't read size of stopWord array");

	UTF8Token token;
	int i;
	for (i = 0; i < size; i++) {
		stream >> token;
		_stopWords.add(token.symValue());
	}
}
