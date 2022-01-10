// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/PartOfSpeech.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/theories/TokenSequence.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"
#include "boost/foreach.hpp"

/*
PartOfSpeechSequence::PartOfSpeechSequence(int sent_no, int n_tokens)
: _sent_no(sent_no), _score(0),
  _n_tokens(n_tokens), _posSequence(_new PartOfSpeech*[n_tokens])
{
	_sent_no = sent_no;
	_n_tokens = n_tokens;
	_score = 0; 
	for (int i = 0; i < _n_tokens; i++)
		_posSequence[i] = _new PartOfSpeech();
}
*/

PartOfSpeechSequence::PartOfSpeechSequence(const TokenSequence *tokenSequence)
: _tokenSequence(tokenSequence), _sent_no(tokenSequence->getSentenceNumber()),
  _n_tokens(tokenSequence->getNTokens()), _score(0),
  _posSequence(_new PartOfSpeech*[tokenSequence->getNTokens()])
{
	for (int i = 0; i < _n_tokens; i++)
		_posSequence[i] = _new PartOfSpeech();
}

PartOfSpeechSequence::PartOfSpeechSequence(const PartOfSpeechSequence &other, const TokenSequence *tokenSequence)
: _tokenSequence(tokenSequence), _sent_no(tokenSequence->getSentenceNumber()),
_n_tokens(tokenSequence->getNTokens()), _score(other._score) {
	if (_n_tokens != other._n_tokens)
		throw InternalInconsistencyException("PartOfSpeechSequence::PartOfSpeechSequence", "Merged token sequence not the same length as copied POS sequence");
	if (_n_tokens > 0) {
		_posSequence = _new PartOfSpeech*[_n_tokens];
		for (int i = 0; i < _n_tokens; i++)
			_posSequence[i] = _new PartOfSpeech(*(other._posSequence[i]));
	} else
		_posSequence = NULL;
}

PartOfSpeechSequence::PartOfSpeechSequence(const TokenSequence *tokenSequence, int sent_no, int n_tokens)
: _tokenSequence(tokenSequence), _sent_no(sent_no),
  _n_tokens(n_tokens), _score(0),
  _posSequence(_new PartOfSpeech*[n_tokens])
{
	for (int i = 0; i < _n_tokens; i++)
		_posSequence[i] = _new PartOfSpeech();
}

PartOfSpeechSequence::~PartOfSpeechSequence() {
	for (int i = 0; i < _n_tokens; i++)
		delete _posSequence[i];
	delete[] _posSequence;
}

void PartOfSpeechSequence::setTokenSequence(const TokenSequence* tokenSequence) {
	if (_tokenSequence != 0 && tokenSequence != _tokenSequence)
		throw InternalInconsistencyException("PartOfSpeechSequence::setTokenSequence",
			"Token sequence is already set!");
	_tokenSequence = tokenSequence;
}


bool PartOfSpeechSequence::isEmpty() const {
	for (int i = 0; i < _n_tokens; i++) {
		if (_posSequence[i]->getNPOS() != 0)
			return false;
	}
	return true;
}

PartOfSpeech* PartOfSpeechSequence::getPOS(int i) const {
	if (i >= _n_tokens) 
		throw InternalInconsistencyException::arrayIndexException(
			"PartOfSpeechSequence::getPOS()", _n_tokens, i);
	return _posSequence[i];
}

int PartOfSpeechSequence::addPOS(Symbol pos, float prob, int token_num) {
	if (token_num >= _n_tokens) 
		throw InternalInconsistencyException::arrayIndexException(
			"PartOfSpeechSequence::addPOS()", _n_tokens, token_num);
	return _posSequence[token_num]->addPOS(pos, prob);
}

void PartOfSpeechSequence::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Part of Speech Sequence (sentence " << _sent_no << "; "
							<< _n_tokens << " tokens):";

	if (_n_tokens > 0) {
		out << newline << "  ";
		for (int i = 0; i < _n_tokens; i++) 
			out << *_posSequence[i] << " ";
	}

	delete[] newline;
}

void PartOfSpeechSequence::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);
	for (int i = 0; i < _n_tokens; i++)
		_posSequence[i]->updateObjectIDTable();
}

void PartOfSpeechSequence::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"POSSequence", this);
	stateSaver->saveInteger(_sent_no);
	if (stateSaver->getVersion() >= std::make_pair(1,6))
		stateSaver->savePointer(_tokenSequence);
	stateSaver->saveInteger(_n_tokens);
	stateSaver->saveReal(_score);
	stateSaver->beginList(L"POSSequence::_pos");
	for (int i = 0; i < _n_tokens; i++)
		_posSequence[i]->saveState(stateSaver);
	stateSaver->endList();
	stateSaver->endList();
}

// For loading state:
PartOfSpeechSequence::PartOfSpeechSequence(StateLoader *stateLoader)
: _tokenSequence(0)
{
	int id = stateLoader->beginList(L"POSSequence");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	_sent_no = 	stateLoader->loadInteger();
	if (stateLoader->getVersion() >= std::make_pair(1,6)) 
		_tokenSequence = static_cast<TokenSequence*>(stateLoader->loadPointer());
	_n_tokens = stateLoader->loadInteger();
	_score = stateLoader->loadReal();
	stateLoader->beginList(L"POSSequence::_pos");
	_posSequence = _new PartOfSpeech*[_n_tokens];
	for(int i = 0; i< _n_tokens; i++){
		_posSequence[i] = _new PartOfSpeech(stateLoader);

	}
	stateLoader->endList();
	stateLoader->endList();
}

void PartOfSpeechSequence::resolvePointers(StateLoader * stateLoader) {
	if (stateLoader->getVersion() >= std::make_pair(1,6)) 
		_tokenSequence = static_cast<TokenSequence*>(stateLoader->getObjectPointerTable().getPointer(_tokenSequence));
	for (int i = 0; i < _n_tokens; i++)
		_posSequence[i]->resolvePointers(stateLoader);
}

const wchar_t* PartOfSpeechSequence::XMLIdentifierPrefix() const {
	return L"posseq";
}

void PartOfSpeechSequence::saveXML(SerifXML::XMLTheoryElement posSeqElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("PartOfSpeechSequence::saveXML", "Expected context to be NULL");
	posSeqElem.setAttribute(X_score, getScore());
	posSeqElem.saveTheoryPointer(X_token_sequence_id, _tokenSequence);
	if (_n_tokens != _tokenSequence->getNTokens())
		throw InternalInconsistencyException("PartOfSpeechSequence::saveXML", "Number of tokens does not match corresponding TokenSequence");
	for (int i = 0; i < _n_tokens; i++) {
		if (posSeqElem.getOptions().skip_dummy_pos_tags &&
			_posSequence[i]->getNPOS() == 1 && 
			_posSequence[i]->getLabel(0) == Symbol(L"DummyPOS"))
		{
			continue;
		}
		posSeqElem.saveChildTheory(X_POS, _posSequence[i], _tokenSequence->getToken(i));
	}
}

PartOfSpeechSequence::PartOfSpeechSequence(SerifXML::XMLTheoryElement posSeqElem)
: _posSequence(0), _n_tokens(0)
{
	using namespace SerifXML;
	posSeqElem.loadId(this);
	_score = posSeqElem.getAttribute<float>(X_score, 0);
	_tokenSequence = posSeqElem.loadTheoryPointer<TokenSequence>(X_token_sequence_id);
	if (_tokenSequence == 0)
		posSeqElem.reportLoadError("Expected a token_sequence_id");
	_n_tokens = _tokenSequence->getNTokens();
	_sent_no = _tokenSequence->getSentenceNumber();

	// Initialize the posSequence to be empty.
	_posSequence = _new PartOfSpeech*[_n_tokens];
	for (int i=0; i<_n_tokens; ++i)
		_posSequence[i] = 0;
	// Fill in any pos tags that were specified.
	XMLTheoryElementList posElems = posSeqElem.getChildElementsByTagName(X_POS);
	XMLSerializedDocTheory *xmldoc = posSeqElem.getXMLSerializedDocTheory();
	BOOST_FOREACH(XMLTheoryElement posElem, posElems) {
		int i = static_cast<int>(xmldoc->lookupTokenIndex(posElem.loadTheoryPointer<Token>(X_token_id)));
		_posSequence[i] = _new PartOfSpeech(posElem);
	}
	// Anything else gets the 'dummy pos' tag.
	Symbol DummyPOS(L"DummyPOS");
	for (int i=0; i<_n_tokens; ++i) {
		if (_posSequence[i] == 0) {
			_posSequence[i] = _new PartOfSpeech();
			_posSequence[i]->addPOS(DummyPOS, 1);
		}
	}
}

