// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
#include "Generic/common/leak_detection.h"

#include "Generic/theories/LexicalToken.h"

#include "string.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/morphAnalysis/SessionLexicon.h"

void LexicalToken::saveState(StateSaver *stateSaver) const {
	if(Token::_saveLexicalTokensAsDefaultTokens) {
		Token::saveState(stateSaver);
	} else {
		stateSaver->beginList(L"Token", this);
		stateSaver->saveSymbol(_symbol);
		stateSaver->saveInteger(_start.edtOffset.value());
		stateSaver->saveInteger(_end.edtOffset.value());
		stateSaver->saveInteger(_original_token_index);
		stateSaver->saveInteger(static_cast<int>(_lex_entries.size()));
		for(size_t i =0; i< _lex_entries.size(); i++){
			if(_lex_entries[i] == 0){
				stateSaver->saveUnsigned(0);
			}
			else{
				stateSaver->saveUnsigned((unsigned)_lex_entries[i]->getID());
			}
		}
		stateSaver->endList();
	}
}

LexicalToken::LexicalToken(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"Token");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	_symbol = stateLoader->loadSymbol();
	_start.edtOffset = EDTOffset(stateLoader->loadInteger());
	_end.edtOffset = EDTOffset(stateLoader->loadInteger());
	_original_token_index = stateLoader->loadInteger();
	int n_lex_entries = stateLoader->loadInteger();

	for(int i=0; i < n_lex_entries; i++){
		size_t id = stateLoader->loadUnsigned();
		_lex_entries.push_back(SessionLexicon::getInstance().getLexicalEntryByID(id));
	}

	stateLoader->endList();
}

void LexicalToken::saveXML(SerifXML::XMLTheoryElement tokenElem, const Theory *context) const {
	using namespace SerifXML;
	Token::saveXML(tokenElem, context);
	// Record the lexical token info.
	tokenElem.setAttribute(X_original_token_index, _original_token_index);
	for(size_t i=0; i<_lex_entries.size(); ++i)
		tokenElem.getXMLSerializedDocTheory()->registerLexicalEntry(_lex_entries[i]);
	std::vector<const Theory*> lex_entries(_lex_entries.begin(), _lex_entries.end());
	tokenElem.saveTheoryPointerList(X_lexical_entries, lex_entries);
}

LexicalToken::LexicalToken(SerifXML::XMLTheoryElement tokenElem, int sent_num, int token_num)
: Token(tokenElem, sent_num, token_num)
{
	using namespace SerifXML;
	// Load lexical token info.
	_original_token_index = tokenElem.getAttribute<int>(X_original_token_index);
	_lex_entries = tokenElem.loadNonConstTheoryPointerList<LexicalEntry>(X_lexical_entries);
}

