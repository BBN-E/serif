// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "theories/Token.h"

#include "string.h"

#include "state/StateSaver.h"
#include "state/StateLoader.h"
#include "state/ObjectIDTable.h"
#include "state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"

#include "Generic/common/ParamReader.h"

bool Token::_saveLexicalTokensAsDefaultTokens = false;

void Token::dump(std::ostream &out, int indent) const{
	out << "[" << _start.edtOffset << ":" << _end.edtOffset << "]" 
		<< _symbol.to_debug_string();
}


void Token::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);
}

#define BEGIN_TOKEN (reinterpret_cast<const wchar_t*>(StateLoader::IntegerCompressionStart + StateLoader::TokenOffset))

void Token::saveState(StateSaver *stateSaver) const {
	if (ParamReader::isParamTrue("use_state_file_integer_compression")) {
		stateSaver->beginList(BEGIN_TOKEN, this);
	} else {
		stateSaver->beginList(L"Token", this);
	}

	stateSaver->saveSymbol(_symbol);
	stateSaver->saveInteger(_start.edtOffset.value());
	stateSaver->saveInteger(_end.edtOffset.value());	
	if (ParamReader::getOptionalTrueFalseParamWithDefaultVal("include_all_offsets_in_state_file", true)) {
		stateSaver->saveInteger(_start.charOffset.value());
		stateSaver->saveInteger(_end.charOffset.value());	
		stateSaver->saveInteger(_start.byteOffset.value());
		stateSaver->saveInteger(_end.byteOffset.value());	
	}

	stateSaver->endList();
}

Token::Token(StateLoader *stateLoader) {
	//Use the integer replacement for "Token" if the state file was compressed
	int id = stateLoader->beginList(stateLoader->useCompressedState() ? BEGIN_TOKEN : L"Token");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	_symbol = stateLoader->loadSymbol();
	_start.edtOffset = EDTOffset(stateLoader->loadInteger());
	_end.edtOffset = EDTOffset(stateLoader->loadInteger());
	if (ParamReader::getOptionalTrueFalseParamWithDefaultVal("include_all_offsets_in_state_file", true)) {
		_start.charOffset = CharOffset(stateLoader->loadInteger());
		_end.charOffset = CharOffset(stateLoader->loadInteger());
		_start.byteOffset = ByteOffset(stateLoader->loadInteger());
		_end.byteOffset = ByteOffset(stateLoader->loadInteger());
	}	

	stateLoader->endList();
}

void Token::resolvePointers(StateLoader * stateLoader) {
}

const wchar_t* Token::XMLIdentifierPrefix() const {
	return L"tok";
}

void Token::saveXML(SerifXML::XMLTheoryElement tokenElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("Token::saveXML", "Expected context to be NULL");
	tokenElem.saveOffsets(getStartOffsetGroup(), getEndOffsetGroup());
	// We include the token symbol string in the output if either: 
	//   * include_token_symbols is true
	//   * The token's symbol contents do not match the corresponding string
	//     in the original text.  This can occur because of various types of
	//     normalization (e.g., "[" becomes "-LSB-").
	Symbol origstr(tokenElem.getOriginalTextSubstring(_start, _end).c_str());
	if ((!tokenElem.getXMLSerializedDocTheory()->getOptions().use_implicit_tokens) || (origstr != _symbol))
		tokenElem.addText(_symbol);
}

Token::Token(SerifXML::XMLTheoryElement tokenElem, int sent_num, int token_num)
{
	using namespace SerifXML;
	tokenElem.loadId(this);
	tokenElem.getXMLSerializedDocTheory()->registerTokenIndex(this, sent_num, token_num);
	tokenElem.loadOffsets(_start, _end); // Read the token's offsets.

	// If the XML element contains text, use that as the Token symbol.
	// Otherwise, read the Token's symbol from the original text.
	std::wstring text = tokenElem.getText<std::wstring>();
	if (!text.empty()) 
		_symbol = Symbol(text);
	else
		_symbol = Symbol(tokenElem.getOriginalTextSubstring(_start, _end).c_str());
}
