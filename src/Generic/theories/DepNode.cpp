// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/theories/DepNode.h"

DepNode::DepNode(int ID, DepNode *parent, const Symbol &tag, int n_children, const Symbol &word, const Symbol &partOfSpeech, int token_number) 
	: SynNode(ID, parent, tag, n_children), _word(word), _partOfSpeech(partOfSpeech), _token_number(token_number) {
	_head_index = -1;  // Set to zero by the SynNode constructor, but meaningless for a DepNode, so we set it to -1
}

const Symbol &DepNode::getHeadWord() const {
	return _word;
}

Symbol DepNode::getPartOfSpeech() const {
	return _partOfSpeech;
}

int DepNode::getTokenNumber() const {
	return _token_number;
}

Symbol DepNode::getWord() const {
	return _word;
}

void DepNode::setID(int ID) {
	_ID = ID;
}

// State saving and loading
#define BEGIN_DEPNODE (reinterpret_cast<const wchar_t*>(StateLoader::IntegerCompressionStart + StateLoader::SynNodeOffset))
void DepNode::saveState(StateSaver *stateSaver) const {
	if (ParamReader::isParamTrue("use_state_file_integer_compression")) {
		stateSaver->beginList(BEGIN_DEPNODE, this);
	} else {
		stateSaver->beginList(L"DepNode", this);
	}

	stateSaver->saveInteger(_ID);
	stateSaver->saveSymbol(_tag);
	stateSaver->saveSymbol(_word);
	stateSaver->saveSymbol(_partOfSpeech);
	stateSaver->saveInteger(_token_number);

	stateSaver->saveInteger(_start_token);
	stateSaver->saveInteger(_end_token);

	stateSaver->saveInteger(_mention_index);

	stateSaver->savePointer(_parent);
	stateSaver->saveInteger(_n_children);
	stateSaver->beginList();
	for (int i = 0; i < _n_children; i++)
		_children[i]->saveState(stateSaver);
	stateSaver->endList();

	stateSaver->endList();
}

DepNode::DepNode(StateLoader *stateLoader) {
	//Use the integer replacement for "SynNode" if the state file was compressed
	int id = stateLoader->beginList(stateLoader->useCompressedState() ? BEGIN_DEPNODE : L"DepNode");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	_ID = stateLoader->loadInteger();
	_tag = stateLoader->loadSymbol();
	_word = stateLoader->loadSymbol();
	_partOfSpeech = stateLoader->loadSymbol();
	_token_number = stateLoader->loadInteger();
	_start_token = stateLoader->loadInteger();
	_end_token = stateLoader->loadInteger();

	_mention_index = stateLoader->loadInteger();

	_parent = (SynNode *) stateLoader->loadPointer();
	_n_children = stateLoader->loadInteger();
	_head_index = -1;
	_children = _new SynNode*[_n_children];
	stateLoader->beginList();
	for (int i = 0; i < _n_children; i++)
		_children[i] = _new DepNode(stateLoader);
	stateLoader->endList();

	stateLoader->endList();
}

// XML saving and loading
void DepNode::saveXML(SerifXML::XMLTheoryElement depNodeElem, const Theory *context) const {
	// BEGIN SynNode::saveXML
	using namespace SerifXML;
	const TokenSequence *tokSeq = dynamic_cast<const TokenSequence*>(context);
	if (context == 0) {
		throw InternalInconsistencyException("SynNode::saveXML", "Expected context to be a TokenSequence");
	}
	depNodeElem.setAttribute(X_tag, _tag);
	const Token *startTok = tokSeq->getToken(_start_token);
	depNodeElem.saveTheoryPointer(X_start_token, startTok);
	const Token *endTok = tokSeq->getToken(_end_token);
	depNodeElem.saveTheoryPointer(X_end_token, endTok);
	// Child nodes
	for (int i = 0; i < _n_children; ++i) {
		XMLTheoryElement childElem = depNodeElem.saveChildTheory(X_DepNode, _children[i], tokSeq);
		//if (i == _head_index)
		//	childElem.setAttribute(X_is_head, X_TRUE);
	}
	// * Don't record mention_index -- it's redundant.
	// * Don't recored _ID -- it's just a depth-first identifier within this parse tree, and
	//   can be reconstructed at load time.
	// END SynNode::saveXML

	// DepNode specific member variables
	const Token *token = tokSeq->getToken(_token_number);
	depNodeElem.saveTheoryPointer(X_token_id, token);
	depNodeElem.setAttribute(X_pos, _partOfSpeech);
	depNodeElem.setAttribute(X_text, _word);
}

// Note: node_id_counter is an in-out parameter, used to assign a unique identifier
// to each SynNode, starting with zero at the root of the tree, and increasing by
// one for each node in a depth first traversal.  It is used to pick a value for _ID.
DepNode::DepNode(SerifXML::XMLTheoryElement depNodeElem, DepNode* parent, int &node_id_counter) {
//: _parent(parent), _head_index(-1), _mention_index(-1) {
	_parent = parent;
	_head_index = -1;
	_mention_index = -1;

	// BEGIN SynNode::SynNode
	using namespace SerifXML;
	depNodeElem.loadId(this);
	_tag = depNodeElem.getAttribute<Symbol>(X_tag);
	_ID = node_id_counter++;
	
	XMLTheoryElementList childElems = depNodeElem.getChildElementsByTagName(X_DepNode);
	_n_children = static_cast<int>(childElems.size());
	_children = _new SynNode*[_n_children];
	for (int i=0; i<_n_children; ++i) {
		_children[i] = _new DepNode(childElems[i], this, node_id_counter);
		//if (childElems[i].getAttribute<bool>(X_is_head, false))
		//	_head_index = i;
	}
	//if (_head_index == -1) {
	//	depNodeElem.reportLoadWarning("No head element found");
	//	// Todo: apply default head-finding rules??
	//}

	XMLSerializedDocTheory *xmldoc = depNodeElem.getXMLSerializedDocTheory();
	_start_token = static_cast<int>(xmldoc->lookupTokenIndex(depNodeElem.loadTheoryPointer<Token>(X_start_token)));
	_end_token = static_cast<int>(xmldoc->lookupTokenIndex(depNodeElem.loadTheoryPointer<Token>(X_end_token)));
	// END SynNode::SynNode

	// DepNode specific member variables
	_token_number = static_cast<int>(xmldoc->lookupTokenIndex(depNodeElem.loadTheoryPointer<Token>(X_token_id)));
	_partOfSpeech = depNodeElem.getAttribute<Symbol>(X_pos);
	_word = depNodeElem.getAttribute<Symbol>(X_text);
}
