// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.
#include "Generic/common/leak_detection.h"
#include "Generic/theories/LexicalTokenSequence.h"
#include "Generic/theories/LexicalToken.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"


LexicalTokenSequence::LexicalTokenSequence() 
	: TokenSequence(),
	  _n_orig_tokens(1), _originaltokens(_new LexicalToken*[1])
{
	_n_tokens = 1;
	_tokens = _new Token*[1];
	_tokens[0] = _new LexicalToken(Symbol(L"."));
	_originaltokens[0] = _new LexicalToken(Symbol(L"."));
}

LexicalTokenSequence::LexicalTokenSequence(int sent_no, int n_tokens, Token *tokens[])
	: TokenSequence(sent_no, n_tokens, tokens),
	  _n_orig_tokens(n_tokens), _originaltokens(NULL)
{
	// Make a deep copy of the tokens in _originaltokens.  We only
	// bother to allocate _orig_tokens if n_tokens>0.  Otherwise, we
	// leave it as NULL.
	if (n_tokens) {
		_originaltokens = _new LexicalToken*[n_tokens];
		for (int i=0; i<n_tokens; ++i)
			if (LexicalToken *tok = dynamic_cast<LexicalToken*>(tokens[i])) {
				_originaltokens[i] = _new LexicalToken(*tok);
			} else {
				throw InternalInconsistencyException("LexicalTokenSequence::LexicalTokenSequence",
					"tokens contained a Token that was not a LexicalToken");
			}
	}
}

void LexicalTokenSequence::retokenize(int n_tokens, Token* tokens[]) {
	for (int i=0; i<n_tokens; i++) {
		if (!dynamic_cast<LexicalToken*>(tokens[i])) {
			throw InternalInconsistencyException("LexicalTokenSequence::LexicalTokenSequence",
				"tokens contained a Token that was not a LexicalToken");
		}
	}
	TokenSequence::retokenize(n_tokens, tokens);
}


LexicalTokenSequence::~LexicalTokenSequence() {
	for (int j = 0; j < _n_orig_tokens; j++){
		delete _originaltokens[j];
	}
	delete[] _originaltokens;
}


const LexicalToken *LexicalTokenSequence::getOriginalToken(int i) const {
	if ((0 <= i) && (i < _n_orig_tokens))
		return _originaltokens[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"LexicalTokenSequence::getOriginalToken()", _n_orig_tokens, i);
}

void LexicalTokenSequence::updateObjectIDTable() const {
	TokenSequence::updateObjectIDTable();
	for (int j = 0; j < _n_orig_tokens; j++)
		_originaltokens[j]->updateObjectIDTable();
}

void LexicalTokenSequence::saveState(StateSaver *stateSaver) const {
	if(Token::_saveLexicalTokensAsDefaultTokens) {
		TokenSequence::saveState(stateSaver);
	} else {
		stateSaver->beginList(L"TokenSequence", this);

		stateSaver->saveReal(_score);
		stateSaver->saveInteger(_sent_no);
		stateSaver->saveInteger(_n_tokens);
		stateSaver->saveInteger(_n_orig_tokens);
		stateSaver->beginList(L"TokenSequence::_tokens");
		for (int i = 0; i < _n_tokens; i++)
			_tokens[i]->saveState(stateSaver);
		stateSaver->endList();
		stateSaver->beginList(L"TokenSequence::_originalTokens");
		for(int j =0; j< _n_orig_tokens; j++){
			_originaltokens[j]->saveState(stateSaver);
		}
		stateSaver->endList();
		stateSaver->endList();
	}		
}

LexicalTokenSequence::LexicalTokenSequence(StateLoader *stateLoader)
	: TokenSequence() {
	int id = stateLoader->beginList(L"TokenSequence");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	_score = stateLoader->loadReal();
	_sent_no = stateLoader->loadInteger();
	_n_tokens = stateLoader->loadInteger();
	_n_orig_tokens = stateLoader->loadInteger();
	stateLoader->beginList(L"TokenSequence::_tokens");
	_tokens = _new Token*[_n_tokens];
	for (int i = 0; i < _n_tokens; i++)
		_tokens[i] = _new LexicalToken(stateLoader);
	stateLoader->endList();
	stateLoader->beginList(L"TokenSequence::_originalTokens");
	_originaltokens = _new LexicalToken*[_n_orig_tokens];
	for(int j =0; j< _n_orig_tokens; j++){
		_originaltokens[j] = _new LexicalToken(stateLoader);
	}
	stateLoader->endList();


	stateLoader->endList();
}

void LexicalTokenSequence::resolvePointers(StateLoader *stateLoader) {
	TokenSequence::resolvePointers(stateLoader);
	for (int i = 0; i < _n_orig_tokens; i++)
		_originaltokens[i]->resolvePointers(stateLoader);
}

void LexicalTokenSequence::saveXML(SerifXML::XMLTheoryElement tokensequenceElem, const Theory *context) const {
	// WARNING: we do not currently serialized the original token sequence
	TokenSequence::saveXML(tokensequenceElem, context);
}

LexicalTokenSequence::LexicalTokenSequence(SerifXML::XMLTheoryElement tokenSequenceElem, int sent_no) 
	: TokenSequence(), _n_orig_tokens(0), _originaltokens(NULL)
{
	using namespace SerifXML;
	tokenSequenceElem.loadId(this);
	_sent_no = sent_no;
	_score = tokenSequenceElem.getAttribute<float>(X_score, 0);

	XMLTheoryElementList tokElems = tokenSequenceElem.getChildElementsByTagName(X_Token);
	_n_tokens = static_cast<int>(tokElems.size());
	_tokens = _new Token*[_n_tokens];
	for (int i=0; i<_n_tokens; ++i)
		_tokens[i] = _new LexicalToken(tokElems[i], sent_no, i);

	// Fake the original tokens -- no one uses it anyway.  (If necessary, we 
	// could probably reconstruct it based on the tokens and their pointers
	// to original tokens indices.  But for now, we don't bother.)
	_n_orig_tokens = _n_tokens;
	_originaltokens = _new LexicalToken*[_n_tokens];
	for (int i=0; i<_n_tokens; ++i) {
		_originaltokens[i] = _new LexicalToken(*(dynamic_cast<LexicalToken*>(_tokens[i])));
	}
}

