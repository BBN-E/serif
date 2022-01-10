// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/NPChunkTheory.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"

NPChunkTheory::NPChunkTheory(const NPChunkTheory &other, const TokenSequence *tokSequence)
: _tokenSequence(tokSequence), n_npchunks(other.n_npchunks), score(other.score)
{
	_parse = _new Parse(*(other._parse), _tokenSequence);
	_parse->gainReference();

	for (int i = 0; i < n_npchunks; i++)
		for (int j = 0; j < 2; j++)
			npchunks[i][j] = other.npchunks[i][j];
}

void NPChunkTheory::setTokenSequence(const TokenSequence* tokenSequence) {
	if (_tokenSequence != 0)
		throw InternalInconsistencyException("NPChunkTheory::setTokenSequence",
			"Token sequence is already set!");
	_tokenSequence = tokenSequence;
}


void NPChunkTheory::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "NPChunk Theory (score: " << score << "):";
	if (n_npchunks == 0) {
		out << " (no npchunks)";
	}
	else {
		for (int i = 0; i < n_npchunks; i++)
			out << newline << "- " << npchunks[i][0]<<" "<<npchunks[i][1];
		out << newline;
	}

	out << "NPChunk Parse " << newline;
	out << "  ";
	if(_parse != NULL)
		_parse->getRoot()->dump(out, indent + 2);

	delete[] newline;
}



void NPChunkTheory::resolvePointers(StateLoader * stateLoader){
	_tokenSequence = static_cast<TokenSequence*>(stateLoader->getObjectPointerTable().getPointer(_tokenSequence));
	if (_parse != NULL)
		_parse->resolvePointers(stateLoader);
}

void NPChunkTheory::saveState(StateSaver *stateSaver) const{
	stateSaver->beginList(L"NPChunkTheory", this);

	stateSaver->saveReal(score);
	if (stateSaver->getVersion() >= std::make_pair(1,6))
		stateSaver->savePointer(_tokenSequence);
	stateSaver->saveInteger(n_npchunks);
	stateSaver->beginList(L"ChunkSpans");

	for (int i = 0; i < n_npchunks; i++){
		stateSaver->beginList(L"Span");
		stateSaver->saveInteger(npchunks[i][0]);
		stateSaver->saveInteger(npchunks[i][1]);
		stateSaver->endList();
	}
	stateSaver->endList();
	stateSaver->beginList(L"NPChunkParse", _parse);
	if (_parse != NULL)
		_parse->saveState(stateSaver);
	stateSaver->endList();
	stateSaver->endList();
}
void NPChunkTheory::updateObjectIDTable()const {
	ObjectIDTable::addObject(this);
	if (_parse != NULL)
		_parse->updateObjectIDTable();
}
NPChunkTheory::NPChunkTheory(StateLoader *stateLoader)
: _tokenSequence(0)
{
	int id = stateLoader->beginList(L"NPChunkTheory");
	stateLoader->getObjectPointerTable().addPointer(id, this);
	score = stateLoader->loadReal();
	if (stateLoader->getVersion() >= std::make_pair(1,6)) 
		_tokenSequence = static_cast<TokenSequence*>(stateLoader->loadPointer());
	n_npchunks = stateLoader->loadInteger();
	stateLoader->beginList(L"ChunkSpans");
	for (int i = 0; i < n_npchunks; i++){
		stateLoader->beginList(L"Span");

		npchunks[i][0] = stateLoader->loadInteger();
		npchunks[i][1] = stateLoader->loadInteger();
		stateLoader->endList();
	}
	stateLoader->endList();
	stateLoader->beginList(L"NPChunkParse");
	_parse = _new Parse(stateLoader);
	_parse->gainReference();
	stateLoader->endList();
	stateLoader->endList();

}

bool NPChunkTheory::inChunk(int tokNum, int chunkNum){
	return ((tokNum >= npchunks[chunkNum][0] ) && (tokNum <=npchunks[chunkNum][1]));
}
int NPChunkTheory::getChunk(int tokNum){
	for(int i=0; i<n_npchunks;i++){
		if(inChunk(tokNum, i)) return i;
	}
	return -1;
}

const wchar_t* NPChunkTheory::XMLIdentifierPrefix() const {
	return L"chunktheory";
}

void NPChunkTheory::saveXML(SerifXML::XMLTheoryElement npchunktheoryElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("NPChunkTheory::saveXML", "Expected context to be NULL");
	npchunktheoryElem.setAttribute(X_score, score);
	npchunktheoryElem.saveTheoryPointer(X_token_sequence_id, _tokenSequence);
	for (int i = 0; i < n_npchunks; ++i) {
		XMLTheoryElement npchunkElem = npchunktheoryElem.addChild(X_NPChunk);
		// Pointers to start/end tokens:
		const Token *startTok = _tokenSequence->getToken(npchunks[i][0]);
		npchunkElem.saveTheoryPointer(X_start_token, startTok);
		const Token *endTok = _tokenSequence->getToken(npchunks[i][1]);
		npchunkElem.saveTheoryPointer(X_end_token, endTok);
	}
	if (_parse)
		npchunktheoryElem.saveChildTheory(X_Parse, _parse);
}

NPChunkTheory::NPChunkTheory(SerifXML::XMLTheoryElement npChunkTheoryElem)
: n_npchunks(0), _parse(0)
{
	using namespace SerifXML;
	npChunkTheoryElem.loadId(this);
	score = npChunkTheoryElem.getAttribute<float>(X_score, 0);
	_tokenSequence = npChunkTheoryElem.loadTheoryPointer<TokenSequence>(X_token_sequence_id);
	if (_tokenSequence == 0)
		npChunkTheoryElem.reportLoadError("Expected a token_sequence_id");
	XMLTheoryElementList childElems = npChunkTheoryElem.getChildElementsByTagName(X_NPChunk, false);
	n_npchunks = static_cast<int>(childElems.size());
	XMLSerializedDocTheory *xmldoc = npChunkTheoryElem.getXMLSerializedDocTheory();
	for (int i=0; i<n_npchunks; ++i) {
		npchunks[i][0] = static_cast<int>(xmldoc->lookupTokenIndex(childElems[i].loadTheoryPointer<Token>(X_start_token)));
		npchunks[i][1] = static_cast<int>(xmldoc->lookupTokenIndex(childElems[i].loadTheoryPointer<Token>(X_end_token)));
	}
	_parse = npChunkTheoryElem.loadChildTheory<Parse>(X_Parse);
	_parse->gainReference();
}
