// Copyright 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "theories/TokenSequence.h"

#include "common/InternalInconsistencyException.h"
#include "common/OutputUtil.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

TokenSequence::TokenSequence(int sent_no, int n_tokens, 
                                           Token** tokens, float score)
	: _sent_no(sent_no), _n_tokens(n_tokens), 
	  _tokens(NULL), _score(score), _stringStore(L"")
{
	// Take ownership of the given tokens.  We only bother to allocate
	// _tokens if n_tokens>0.  Otherwise, we leave it as NULL.
	if (n_tokens) {
		_tokens = _new Token*[n_tokens];
		for (int i=0; i<n_tokens; ++i)
			_tokens[i] = tokens[i];
	}
}

TokenSequence::TokenSequence(const TokenSequence &other, int sent_offset) {
	_sent_no = other._sent_no + sent_offset;
	_n_tokens = other._n_tokens;
	_score = other._score;
	_stringStore = L"";
	if (_n_tokens > 0) {
		_tokens = _new Token*[_n_tokens];
		for (int i = 0; i < _n_tokens; i++)
			_tokens[i] = _new Token(*(other._tokens[i]));
	} else
		_tokens = NULL;
}

TokenSequence::~TokenSequence() {
	for (int i = 0; i < _n_tokens; ++i)
		delete _tokens[i];
	delete[] _tokens;
	_tokens = NULL;
}

const Token *TokenSequence::getToken(int i) const {
	if ((0 <= i) && (i < _n_tokens))
		return _tokens[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"TokenSequence::getToken()", _n_tokens, i);
}

void TokenSequence::retokenize(int n_tokens, Token* tokens[]) {
	// Delete the old tokens.
	for (int i = 0; i < _n_tokens; i++)
		delete _tokens[i];
	
	// Allocate spaced for the new tokens.
	if (n_tokens > _n_tokens) {
		delete _tokens;
		_tokens = new Token*[n_tokens];
	}

	// Take ownership of the new tokens.
	_n_tokens = n_tokens;
	for (int j = 0; j < n_tokens; j++)
		_tokens[j] = tokens[j];
}



std::wstring TokenSequence::toString() const {
	if (_stringStore != L"") {
		return _stringStore;
	}
	std::wstring str = L"( ";
	for (int i = 0; i < _n_tokens; i++) {
		str += _tokens[i]->getSymbol().to_string();
		str += L" ";
	}
	str += L")";
	_stringStore = str;
	return str;
}

std::string TokenSequence::toDebugString() const {	
	return toDebugString(0, _n_tokens - 1);
}

std::string TokenSequence::toDebugString(int start_token_inclusive, int end_token_inclusive) const {	
	std::stringstream stream;
	for (int i = start_token_inclusive; i <= end_token_inclusive; i++) {
		stream << _tokens[i]->getSymbol().to_debug_string();
		if (i != end_token_inclusive) {
			stream << ' ';
		}
	}
	return stream.str();
}

std::wstring TokenSequence::toString(int start_token_inclusive, int end_token_inclusive) const {	
	std::wstringstream stream;
	for (int i = start_token_inclusive; i <= end_token_inclusive; i++) {
		stream << _tokens[i]->getSymbol().to_string();
		if (i != end_token_inclusive) {
			stream << L' ';
		}
	}
	return stream.str();
}

std::wstring TokenSequence::toStringWithOffsets() const {
	std::wstringstream s;
	s << L"(sentence " << _sent_no << L"; " << _n_tokens;
	s << " tokens: ";
	for (int i = 0; i < _n_tokens; i++) {
		s << L"[" << _tokens[i]->getStartEDTOffset()
			<< L":" << _tokens[i]->getEndEDTOffset()
			<< L"]" << _tokens[i]->getSymbol().to_string() << L" ";
	}
	s << L")";
	return s.str();
}

std::wstring TokenSequence::toStringNoSpaces() const {
	std::wstringstream s;
	for (int i = 0; i < _n_tokens; i++) {
		s << _tokens[i]->getSymbol().to_string();
	}
	return s.str();
}


void TokenSequence::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Token Sequence (sentence " << _sent_no << "; "
							<< _n_tokens << " tokens):";

	if (_n_tokens > 0) {
		out << newline << "  ";
		for (int i = 0; i < _n_tokens; i++) {
			out << *_tokens[i] << " ";
		}
	}

	delete[] newline;
}

const wchar_t* TokenSequence::XMLIdentifierPrefix() const {
	return L"tokseq";
}

void TokenSequence::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);

	for (int i = 0; i < _n_tokens; i++)
		_tokens[i]->updateObjectIDTable();
}

void TokenSequence::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"TokenSequence", this);

	stateSaver->saveReal(_score);
	stateSaver->saveInteger(_sent_no);
	stateSaver->saveInteger(_n_tokens);
	stateSaver->beginList(L"TokenSequence::_tokens");
	for (int i = 0; i < _n_tokens; i++)
		_tokens[i]->saveState(stateSaver);
	stateSaver->endList();

	stateSaver->endList();
}

TokenSequence::TokenSequence(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"TokenSequence");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	_score = stateLoader->loadReal();
	_sent_no = stateLoader->loadInteger();
	_n_tokens = stateLoader->loadInteger();
	_tokens = _new Token*[_n_tokens];
	stateLoader->beginList(L"TokenSequence::_tokens");
	for (int i = 0; i < _n_tokens; i++)
		_tokens[i] = _new Token(stateLoader);
	stateLoader->endList();

	stateLoader->endList();
}

void TokenSequence::resolvePointers(StateLoader * stateLoader) {
	for (int i = 0; i < _n_tokens; i++)
		_tokens[i]->resolvePointers(stateLoader);
}

void TokenSequence::saveXML(SerifXML::XMLTheoryElement tokensequenceElem, const Theory *context) const {
	// We do not serialize getSentenceNumber(), since it's redundant.
	// We do not serialize getNTokens(), since it's redundant.
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("TokenSequence::saveXML", "Expected context to be NULL");
	tokensequenceElem.setAttribute(X_score, getScore());
	for (int i = 0; i < _n_tokens; i++) 
		tokensequenceElem.saveChildTheory(X_Token, getToken(i));
}

TokenSequence::TokenSequence(SerifXML::XMLTheoryElement tokenSequenceElem, int sent_no) 
	: _sent_no(0), _n_tokens(0), _tokens(NULL), _score(0)
{
	using namespace SerifXML;
	tokenSequenceElem.loadId(this);
	_sent_no = sent_no;
	_score = tokenSequenceElem.getAttribute<float>(X_score, 0);

	XMLTheoryElementList tokElems = tokenSequenceElem.getChildElementsByTagName(X_Token);
	_n_tokens = static_cast<int>(tokElems.size());
	_tokens = _new Token*[_n_tokens];
	for (int i=0; i<_n_tokens; ++i)
		_tokens[i] = _new Token(tokElems[i], sent_no, i);
}

namespace {
	TokenSequence::TokenSequenceType& _defaultTokenSequenceType() {
		static TokenSequence::TokenSequenceType _t = TokenSequence::TOKEN_SEQUENCE;
		return _t;
	}
}

void TokenSequence::setTokenSequenceTypeForStateLoading(TokenSequenceType type) {
	_defaultTokenSequenceType() = type;
}
TokenSequence::TokenSequenceType TokenSequence::getTokenSequenceTypeForStateLoading() {
	return _defaultTokenSequenceType();
}

