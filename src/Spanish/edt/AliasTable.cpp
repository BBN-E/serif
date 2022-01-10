// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Chinese/edt/AliasTable.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"

AliasTable* AliasTable::_instance = 0;
DebugStream AliasTable::_debugOut;

AliasTable* AliasTable::getInstance() {
	if (_instance == 0)
		_instance = _new AliasTable();
	return _instance;
}

void AliasTable::destroyInstance() {
	if (_instance != 0) {
		SymbolSymArrayMap::iterator iter;
		SymbolSymArrayMap *table = _instance->_table;
		for (iter = table->begin();
			iter != table->end();
			++iter)
		{
			// don't delete (*iter).first because Symbols are not deleted
			delete (*iter).second;
		}
		delete table;
		_instance = 0;
	}
}

AliasTable::AliasTable() {
	_debugOut.init(Symbol(L"alias_table_debug_file"));

	//initialize hash map
	_table = _new SymbolSymArrayMap(300);

	std::string table_file = ParamReader::getParam("alias_table");
	if (!table_file.empty()) {
		initialize(table_file.c_str());
	}
}

void AliasTable::initialize(const char *file_name) {
	AliasFileReader reader(file_name);
	const int MAX_ALIASES = 16;
	Symbol newKey;
	SymArray *newValue;
	Symbol aliasArray[MAX_ALIASES];
	int nArray;

	while(reader.hasMoreTokens()) {
		reader.getLeftParen();

		newKey = reader.getKeySymbol();
		_debugOut << "found KEY: " << newKey.to_debug_string() << "\n";

		nArray = reader.getAliasSymbolArray(aliasArray, MAX_ALIASES);
		newValue = _new SymArray(aliasArray, nArray);
		for (int i = 0; i < nArray; i++)
			_debugOut << "found ALIAS: " << aliasArray[i].to_debug_string() << "\n";

		add(newKey, newValue);

		reader.getRightParen();
	}

}

bool AliasTable::add(Symbol key, SymArray *value) {
	if((*_table)[key] == NULL) {
		(*_table)[key] = value;
		return true;
	}
	else {
		(*_table)[key] = value;
		_debugOut << "found multiple values for KEY: " << key.to_string() << "\n";
		return false;
	}
}

int AliasTable::lookup(Symbol key, Symbol results[], int max_results) {
	SymArray **result = (*_table).get(key);
	int nResults = 0;
	if (result != NULL) {
		_debugOut << "Successful lookup: ";
		SymArray *value = *result;
		if (value->length > max_results) {
			SessionLogger &logger = *SessionLogger::logger;
			logger.reportWarning() << "AliasTable::lookup(): Number of aliases returned exceeds max_results.";
		}
		for(int i = 0; i < value->length && nResults < max_results; i++) {
			results[nResults++] = value->array[i];
			_debugOut << " " << value->array[i].to_string();
		}

	}
	return nResults;
}


AliasTable::AliasFileReader::AliasFileReader() : SexpReader() { }
AliasTable::AliasFileReader::AliasFileReader(const char *file_name) : SexpReader(file_name) { }

Symbol AliasTable::AliasFileReader::getKeySymbol() throw(UnexpectedInputException) {
	UTF8Token token = getToken(WORD);
	return token.symValue();
}

int AliasTable::AliasFileReader::getAliasSymbolArray(Symbol results[], int max_results)
throw(UnexpectedInputException)
{
	UTF8Token token = getToken(LPAREN | WORD);
	int nResults = 0;
	if(token.symValue() == SymbolConstants::leftParen) {
		while((token = getToken(RPAREN | WORD)).symValue() != SymbolConstants::rightParen &&
			   (nResults < max_results))
		{
			results[nResults++] = token.symValue();
		}
		if (token.symValue() != SymbolConstants::rightParen) {
			SessionLogger &logger = *SessionLogger::logger;
			logger.reportWarning() << "AliasTable::getAliasSymbolArray(): Number of aliases read exceeds max_results.";
		}
	}
	else results[nResults++] = token.symValue();
	return nResults;
}
