// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"

#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/StateSaver.h"

#include "Generic/theories/DependencyParseTheory.h"
#include "Generic/theories/DepNode.h"

DependencyParseTheory::DependencyParseTheory(const DependencyParseTheory &other, const TokenSequence *tokSequence)
: _tokenSequence(tokSequence), score(other.score)
{
	_parse = _new Parse(*(other._parse), _tokenSequence);
	_parse->gainReference();
}

void DependencyParseTheory::setTokenSequence(const TokenSequence* tokenSequence) {
	if (_tokenSequence != 0)
		throw InternalInconsistencyException("DependencyParseTheory::setTokenSequence",
			"Token sequence is already set!");
	_tokenSequence = tokenSequence;
}

void DependencyParseTheory::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "DependencyParse Theory (score: " << score << "):";
	
	out << "DependencyParse Parse " << newline;
	out << "  ";
	if(_parse != NULL)
		_parse->getRoot()->dump(out, indent + 2);

	delete[] newline;
}

void DependencyParseTheory::resolvePointers(StateLoader * stateLoader){
	_tokenSequence = static_cast<TokenSequence*>(stateLoader->getObjectPointerTable().getPointer(_tokenSequence));
	if (_parse != NULL)
		_parse->resolvePointers(stateLoader);
}

void DependencyParseTheory::saveState(StateSaver *stateSaver) const{
	stateSaver->beginList(L"DependencyParseTheory", this);

	stateSaver->saveReal(score);
	if (stateSaver->getVersion() >= std::make_pair(1,6))
		stateSaver->savePointer(_tokenSequence);
	
	stateSaver->beginList(L"DependencyParse", _parse);
	if (_parse != NULL)
		_parse->saveState(stateSaver);
	stateSaver->endList();
	stateSaver->endList();
}

void DependencyParseTheory::updateObjectIDTable()const {
	ObjectIDTable::addObject(this);
	if (_parse != NULL)
		_parse->updateObjectIDTable();
}

DependencyParseTheory::DependencyParseTheory(StateLoader *stateLoader)
: _tokenSequence(0)
{
	int id = stateLoader->beginList(L"DependencyParseTheory");
	stateLoader->getObjectPointerTable().addPointer(id, this);
	score = stateLoader->loadReal();
	if (stateLoader->getVersion() >= std::make_pair(1,6)) 
		_tokenSequence = static_cast<TokenSequence*>(stateLoader->loadPointer());
	
	stateLoader->beginList(L"DependencyParse");
	_parse = _new Parse(stateLoader, true);
	_parse->gainReference();
	stateLoader->endList();
	stateLoader->endList();

}

const wchar_t* DependencyParseTheory::XMLIdentifierPrefix() const {
	return L"dependencyparsetheory";
}

void DependencyParseTheory::saveXML(SerifXML::XMLTheoryElement dependencyParseTheoryElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("DependencyParseTheory::saveXML", "Expected context to be NULL");
	dependencyParseTheoryElem.setAttribute(X_score, score);
	dependencyParseTheoryElem.saveTheoryPointer(X_token_sequence_id, _tokenSequence);
	
	if (_parse) {
		dependencyParseTheoryElem.saveChildTheory(X_DepNode, _parse->getRoot(), _tokenSequence);
	}
}

DependencyParseTheory::DependencyParseTheory(SerifXML::XMLTheoryElement dependencyParseTheoryElem) 
: _parse(0) {
	using namespace SerifXML;
	dependencyParseTheoryElem.loadId(this);
	score = dependencyParseTheoryElem.getAttribute<float>(X_score, 0);
	_tokenSequence = dependencyParseTheoryElem.loadTheoryPointer<TokenSequence>(X_token_sequence_id);
	if (_tokenSequence == 0) {
		dependencyParseTheoryElem.reportLoadError("Expected a token_sequence_id");
	}
	
	XMLSerializedDocTheory *xmldoc = dependencyParseTheoryElem.getXMLSerializedDocTheory();

	// Load our root and its children
	int node_id_counter = 0;
	DepNode* depNodeRoot = dependencyParseTheoryElem.loadChildTheory<DepNode>(X_DepNode, static_cast<DepNode*>(0), node_id_counter);

	// Create a parse from our root
	_parse = _new Parse(_tokenSequence, (SynNode*) depNodeRoot, score);
}
