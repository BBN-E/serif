// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/DepNode.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/state/XMLIdMap.h"
#include <boost/foreach.hpp>




Parse::Parse(const TokenSequence *tokenSequence) : 
	_score(0),
	_is_default_parse(false),
	_tokenSequence(tokenSequence)
{
	_root = _new SynNode(0, 0, Symbol(L"X"), 1);
	SynNode *child = _new SynNode(1, _root, Symbol(L"-empty-"), 0);
	child->setTokenSpan(0, 0);
	_root->setChild(0, child);
}
 
Parse::Parse(const TokenSequence *tokenSequence, SynNode *root, float score) : 
		_root(root), _score(score), _is_default_parse(false),
		_tokenSequence(tokenSequence)
{ 
	if (root == NULL)
		throw InternalInconsistencyException("Parse::Parse()", 
											 "Parse root should never be set to NULL.");
}

Parse::Parse(const Parse &other, const TokenSequence *tokSequence)
: _tokenSequence(tokSequence), _score(other._score), _is_default_parse(other._is_default_parse) {
	_root = _new SynNode(*other._root, NULL);
}

void Parse::setTokenSequence(const TokenSequence* tokenSequence) {
	if (_tokenSequence != 0)
		throw InternalInconsistencyException("Parse::setTokenSequence",
			"Token sequence is already set!");
	_tokenSequence = tokenSequence;
}

void Parse::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Parse (score = " << _score << "):"
		<< newline << "  ";
	_root->dump(out, indent + 2);

	delete[] newline;
}


void Parse::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);

	_root->updateObjectIDTable();
}

void Parse::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"Parse", this);

	stateSaver->saveReal(_score);
	if (stateSaver->getVersion() >= std::make_pair(1,6))
		stateSaver->savePointer(_tokenSequence);
	_root->saveState(stateSaver);

	stateSaver->endList();
}

Parse::Parse(StateLoader *stateLoader, bool is_dependency_parse)
: _tokenSequence(0)
{
	int id = stateLoader->beginList(L"Parse");
	stateLoader->getObjectPointerTable().addPointer(id, this);
	stateLoader->registerParse(this);

	_score = stateLoader->loadReal();
	if (stateLoader->getVersion() >= std::make_pair(1,6)) 
		_tokenSequence = static_cast<TokenSequence*>(stateLoader->loadPointer());

	if (is_dependency_parse)
		_root = _new DepNode(stateLoader);
	else
		_root = _new SynNode(stateLoader);

	_is_default_parse = determineIfDefaultParse(_root);

	stateLoader->endList();
}

bool Parse::determineIfDefaultParse(const SynNode *node)
{
	if (node->getTag() != ParserTags::FRAGMENTS) return false;
	for (int i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = node->getChild(i);
		if (child->getTag() != ParserTags::FRAGMENTS) return false;
		if (child->getNChildren() > 1 || 
			(child->getNChildren() == 1 && !child->getChild(0)->isTerminal())) 
		{
			return false;
		}
	}
	return true;
}

void Parse::resolvePointers(StateLoader * stateLoader) {
	_tokenSequence = static_cast<TokenSequence*>(stateLoader->getObjectPointerTable().getPointer(_tokenSequence));
	_root->resolvePointers(stateLoader);
}

const SynNode * Parse::getSynNode( int id, const SynNode * node ) const {
	
	if( node == 0 ) node = this->getRoot();
	if( node->getID() == id ) return node;
	
	const SynNode * c_node = 0;
	for( int i = 0; i < node->getNChildren(); i++ )
		if( ( c_node = getSynNode( id, node->getChild(i) ) ) )
			return c_node;
	
	return 0;
}

const wchar_t* Parse::XMLIdentifierPrefix() const {
	return L"parse";
}

void Parse::saveXML(SerifXML::XMLTheoryElement parseElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("Parse::saveXML", "Expected context to be NULL");
	parseElem.setAttribute(X_score, _score);
	parseElem.saveTheoryPointer(X_token_sequence_id, _tokenSequence);

	if (parseElem.getOptions().use_treebank_tree) {
		// Assign identifiers to all the nodes.  These aren't explicitly encoded in the
		// treebank string; instead, they have the form: <parseid>.<nodeid>, where 
		// <parseid> is the id of this Parse, and <nodeid> is an integer identifier
		// based on a depth-first traversal of the tree.
		std::wstringstream idPrefix;
		idPrefix << parseElem.getAttribute<std::wstring>(X_id) << L".";
		int node_id_counter = 0;
		XMLIdMap* idMap = parseElem.getXMLSerializedDocTheory()->getIdMap();
		assignTreebankNodeIds(_root, idMap, idPrefix.str(), node_id_counter);

		// Save the tree itself as a treebank string.
		XMLTheoryElement treebankElem = parseElem.addChild(X_TreebankString);
		treebankElem.addText(transcodeToXString(toTreebankString(true).c_str()).c_str());
		treebankElem.setAttribute(X_node_id_method, X_DFS);
	} else {
		parseElem.saveChildTheory(X_SynNode, _root, _tokenSequence);
	}
}

// Helper that registers the identifier for each node in the tree.
// node_id_counter is an in/out parameter used to assign unique identifiers
// following a depth-first traversal.
void Parse::assignTreebankNodeIds(const SynNode *node, SerifXML::XMLIdMap *idMap, 
								  const std::wstring &prefix, int &node_id_counter) const 
{
	using namespace SerifXML;
	std::wstringstream id;
	id << prefix << node_id_counter++;
	idMap->registerId(transcodeToXString(id.str().c_str()).c_str(), node);
	for (int i=0; i<node->getNChildren(); ++i) {
		assignTreebankNodeIds(node->getChild(i), idMap, prefix, node_id_counter);
	}
}

std::wstring Parse::toTreebankString(bool mark_heads) const{
	int tok_pos = 0;
	std::wstringstream out;
	try {
		writeTreebankString(_root, out, tok_pos, mark_heads, true);
	} catch (InternalInconsistencyException &e) {
		std::ostringstream context;
		context << "While serializing";
		dump(context, 4);
		context << "\n->  ";
		e.prependToMessage(context.str().c_str());
		throw;
	}
	return out.str();
}

void Parse::writeTreebankString(const SynNode *node, std::wstringstream &out, int &tok_pos, bool mark_heads, bool is_head) const {
	if (node->isTerminal() && (node->getEndToken() >= node->getStartToken())) {
		if ((node->getStartToken() != tok_pos) || (node->getEndToken() != tok_pos)) {
			std::ostringstream err;
			err << "Terminal parse node spans multiple or unexpected tokens.  "
				<< "Expected tok_pos=" << tok_pos << "; node start tok="
				<< node->getStartToken() << "; node end tok=" << node->getEndToken()
				<< "; node tag=" << node->getTag();
			throw InternalInconsistencyException("Parse::writeTreebankString", err.str().c_str());
		}
		out << node->getTag();
		++tok_pos;
	} else {
		out << "(" << node->getTag();
		if (is_head) out << "^"; // head marker
		int head_index = node->getHeadIndex();
		for (int i=0; i<node->getNChildren(); ++i) {
			out << L" ";
			writeTreebankString(node->getChild(i), out, tok_pos, mark_heads, (i==head_index));
		}
		out << ")";
	}
}

SynNode* Parse::fromTreebankString(const std::wstring &src) {
	wchar_seperator sep(L" ", L"()");
	wchar_tokenizer tokens(src, sep);
	SynNode* root = 0;

	int node_id_counter = 0; // Assigned depth-first.
	int tok_pos = 0;
	bool isHead = false;
	wchar_tokenizer::const_iterator tok_iter = tokens.begin();
	wchar_tokenizer::const_iterator tok_end = tokens.end();
	if (tokens.begin() == tokens.end())
		throw UnexpectedInputException("Parse::parseTreebankString", "Empty treebank string");
	if (*tok_iter != L"(")
		throw UnexpectedInputException("Parse::parseTreebankString", "Expected '('");
	++tok_iter;
	SynNode* node = fromTreebankString(tok_iter, tok_end, node_id_counter, tok_pos, isHead);
	if (tok_iter != tok_end)
		throw UnexpectedInputException("Parse::parseTreebankString", "Extra text after final ')'");
	return node;
}

// When this is called, we've already consumed the open '(' token for a constituent.
// node_id_counter and tok_pos are in/out parameters.
// is_head is an out parameter.
SynNode* Parse::fromTreebankString(wchar_tokenizer::const_iterator &tok_iter, wchar_tokenizer::const_iterator &end,
									int &node_id_counter, int &tok_pos, bool &is_head) {
	int node_id = node_id_counter++;

	if ((tok_iter == end) || (*tok_iter == L"(") || (*tok_iter == L")"))
		throw UnexpectedInputException("Parse::parseTreebankString", "Expected node tag");
	std::wstring tag_str(*tok_iter);
	++tok_iter;

	// split ^ off the end of tag if it's there, and set is_head appropriately
	if (*(tag_str.rbegin()) == L'^') {
		tag_str.erase(tag_str.end()-1);
		is_head = true;
	}
	
	Symbol tag(tag_str);
	int head_index = -1;
	int start_tok_pos = tok_pos;

	std::vector<SynNode*> children;
	while (tok_iter != end) {
		if (*tok_iter == L"(") {
			// Nested constituent:
			++tok_iter;
			bool child_is_head = false;
			SynNode *child = fromTreebankString(tok_iter, end, node_id_counter, tok_pos, child_is_head);
			children.push_back(child);
			if (child_is_head)
				head_index = static_cast<int>(children.size())-1;
		} else if (*tok_iter == L")") {
			// End of this constituent:
			++tok_iter;
			if ((head_index == -1) && (!children.empty()))
				throw UnexpectedInputException("Parse::parseTreebankString", 
					"No head child found"); // [xx] do automatic head finding?
			SynNode *node = _new SynNode(node_id, 0, tag, static_cast<int>(children.size()));
			node->setTokenSpan(start_tok_pos, tok_pos-1);
			if (!children.empty()) {
				node->setChildren(static_cast<int>(children.size()), head_index, &(*children.begin()));
				for (size_t i=0; i<children.size(); ++i)
					children[i]->setParent(node);
			}
			return node;
		} else {
			// Nested token:
			SynNode *child = _new SynNode(node_id_counter++, 0, Symbol(*tok_iter), 0);
			child->setTokenSpan(tok_pos, tok_pos);
			children.push_back(child);
			head_index = static_cast<int>(children.size())-1;
			++tok_iter;
			++tok_pos;
		}
	}
	throw UnexpectedInputException("Parse::parseTreebankString", "Unexpected end of string");
}


Parse::Parse(SerifXML::XMLTheoryElement parseElem)
{
	using namespace SerifXML;
	parseElem.loadId(this);
	_score = parseElem.getAttribute<float>(X_score, 0);
	_tokenSequence = parseElem.loadTheoryPointer<TokenSequence>(X_token_sequence_id);
	if (_tokenSequence == 0)
		parseElem.reportLoadError("Expected a token_sequence_id");

	if (XMLTheoryElement treebankElem = parseElem.getOptionalUniqueChildElementByTagName(X_TreebankString)) {
		_root = fromTreebankString(treebankElem.getText<std::wstring>());
		// Assign identifiers to the indiviaul parse nodes.  These identifiers are automatically
		// constructed based on a depth first traversal of the tree.  We skip id generation if
		// the parse element has no identifier.
		if (parseElem.hasAttribute(X_id)) {
			if (treebankElem.getAttribute<std::wstring>(X_node_id_method) == std::wstring(L"DFS")) {
				std::wstringstream idPrefix;
				idPrefix << parseElem.getAttribute<std::wstring>(X_id) << L".";
				int node_id_counter = 0;
				XMLIdMap* idMap = parseElem.getXMLSerializedDocTheory()->getIdMap();
				assignTreebankNodeIds(_root, idMap, idPrefix.str(), node_id_counter);
			} else {
				treebankElem.reportLoadError("Unsupported value for node_id_method attribute");
			}
		}
	} else {
		int node_id_counter = 0;
		_root = parseElem.loadChildTheory<SynNode>(X_SynNode, static_cast<SynNode*>(0), node_id_counter);
	}
	_is_default_parse = determineIfDefaultParse(_root);
}
