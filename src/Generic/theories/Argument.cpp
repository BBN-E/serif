// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/limits.h"
#include "common/InternalInconsistencyException.h"
#include "theories/Argument.h"
#include "theories/Mention.h"
#include "theories/MentionSet.h"
#include "theories/SynNode.h"
#include "theories/Parse.h"
#include "theories/Proposition.h"

#include "state/StateSaver.h"
#include "state/StateLoader.h"
#include "state/ObjectIDTable.h"
#include "state/ObjectPointerTable.h"

#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

#include "Generic/common/ParamReader.h"

using namespace std;

const Symbol Argument::REF_ROLE		= Symbol(L"<ref>");
const Symbol Argument::SUB_ROLE		= Symbol(L"<sub>");
const Symbol Argument::OBJ_ROLE		= Symbol(L"<obj>");
const Symbol Argument::IOBJ_ROLE	= Symbol(L"<iobj>");
const Symbol Argument::POSS_ROLE	= Symbol(L"<poss>");
const Symbol Argument::TEMP_ROLE	= Symbol(L"<temp>");
const Symbol Argument::LOC_ROLE		= Symbol(L"<loc>");
const Symbol Argument::MEMBER_ROLE	= Symbol(L"<member>");
const Symbol Argument::UNKNOWN_ROLE	= Symbol(L"<unknown>");

void Argument::populateWithMention(Symbol roleSym,
								   int mention_index)
{
	_type = MENTION_ARG;
	_roleSym = roleSym;

	_arg.mention_index = mention_index;
}

void Argument::populateWithProposition(Symbol roleSym,
									   const Proposition *prop)
{
	_type = PROPOSITION_ARG;
	_roleSym = roleSym;

	_arg.prop = prop;
}

void Argument::populateWithText(Symbol roleSym,
							    const SynNode *node)
{
	_type = TEXT_ARG;
	_roleSym = roleSym;

	_arg.node = node;
}


int Argument::getMentionIndex() const {
	if (_type == MENTION_ARG) {
		return _arg.mention_index;
	}
	else {
		throw InternalInconsistencyException("Argument::getMentionID()",
			"Request for mention content of non-mention argument");
	}
}

const Mention *Argument::getMention(const MentionSet *mentionSet) const {
	if (_type == MENTION_ARG) {
		return mentionSet->getMention(_arg.mention_index);
	}
	else {
		throw InternalInconsistencyException("Argument::getMention()",
			"Request for mention content of non-mention argument");
	}
}

const Proposition *Argument::getProposition() const {
	if (_type == PROPOSITION_ARG) {
		return _arg.prop;
	}
	else {
		throw InternalInconsistencyException("Argument::getProposition()",
			"Request for Proposition content of non-proposition argument");
	}
}

const SynNode *Argument::getNode() const {
	if (_type == TEXT_ARG) {
		return _arg.node;
	}
	else {
		throw InternalInconsistencyException("Argument::getNode()",
			"Request for SynNode content of non-text argument");
	}
}


void Argument::dump(std::ostream &out, int indent) const {
	if (!_roleSym.is_null())
		out << _roleSym.to_debug_string() << ":";

	if (_type == MENTION_ARG) {
		out << "e" << _arg.mention_index;
	}
	else if (_type == PROPOSITION_ARG) {
		out << "p" << _arg.prop->getID() % MAX_SENTENCE_PROPS;
	}
	else if (_type == TEXT_ARG) {
		Symbol symArray[MAX_SENTENCE_TOKENS];
		int n_tokens = _arg.node->getTerminalSymbols(symArray, 
													 MAX_SENTENCE_TOKENS);
		out << "\"";
		if (n_tokens > 0)
			out << symArray[0].to_debug_string();
		for (int i = 1; i < n_tokens; i++)
			out << " " << symArray[i].to_debug_string();
		out << "\"";
	}
	else {
		throw InternalInconsistencyException("Argument::dump()",
			"_type is not MENTION_ARG, PROPOSITION_ARG, or TEXT_ARG");
	}
}

void Argument::dump(UTF8OutputStream &out, int indent) const {
	if (!_roleSym.is_null())
		out << _roleSym.to_string() << L":";

	if (_type == MENTION_ARG) {
		out << L"e" << _arg.mention_index;
	}
	else if (_type == PROPOSITION_ARG) {
		out << L"p" << _arg.prop->getID() % MAX_SENTENCE_PROPS;
	}
	else if (_type == TEXT_ARG) {
		Symbol symArray[MAX_SENTENCE_TOKENS];
		int n_tokens = _arg.node->getTerminalSymbols(symArray, 
													 MAX_SENTENCE_TOKENS);
		out << L"\"";
		if (n_tokens > 0)
			out << symArray[0].to_string();
		for (int i = 1; i < n_tokens; i++)
			out << L" " << symArray[i].to_string();
		out << L"\"";
	}
	else {
		throw InternalInconsistencyException("Argument::dump()",
			"_type is not MENTION_ARG, PROPOSITION_ARG, or TEXT_ARG");
	}
}

std::wstring Argument::toString(int indent) {
	std::wstring str = L"";
	if (!_roleSym.is_null()) {
		str += _roleSym.to_string();
		str += L":";
	}

	if (_type == MENTION_ARG) {
		str += L"e";
		wchar_t wc[100];
		swprintf(wc, 100, L"%d", _arg.mention_index);
		str += wc;
	}
	else if (_type == PROPOSITION_ARG) {
		str += L"p"; 
		wchar_t wc[100];
		swprintf(wc, 100, L"%d", _arg.prop->getID() % MAX_SENTENCE_PROPS);
		str += wc;
	}
	else if (_type == TEXT_ARG) {
		Symbol symArray[MAX_SENTENCE_TOKENS];
		int n_tokens = _arg.node->getTerminalSymbols(symArray, 
													 MAX_SENTENCE_TOKENS);
		str += L"\"";
		if (n_tokens > 0)
			str += symArray[0].to_string();
		for (int i = 1; i < n_tokens; i++) {
			str += L" "; 
			str += symArray[i].to_string();
		}
		str += L"\"";
	}
	else {
		throw InternalInconsistencyException("Argument::dump()",
			"_type is not MENTION_ARG, PROPOSITION_ARG, or TEXT_ARG");
	}
	return str;
}

std::string Argument::toDebugString(int indent) const {
	std::string str = "";
	if (!_roleSym.is_null()) {
		str += _roleSym.to_debug_string();
		str += ":";
	}

	if (_type == MENTION_ARG) {
		str += "e";
		char c[100];
		sprintf(c, "%d", _arg.mention_index);
		str += c;
	}
	else if (_type == PROPOSITION_ARG) {
		str += "p"; 
		char c[100];
		sprintf(c, "%d", _arg.prop->getID() % MAX_SENTENCE_PROPS);
		str += c;
	}
	else if (_type == TEXT_ARG) {
		Symbol symArray[MAX_SENTENCE_TOKENS];
		int n_tokens = _arg.node->getTerminalSymbols(symArray, 
													 MAX_SENTENCE_TOKENS);
		str += "\"";
		if (n_tokens > 0)
			str += symArray[0].to_debug_string();
		for (int i = 1; i < n_tokens; i++) {
			str += " "; 
			str += symArray[i].to_debug_string();
		}
		str += "\"";
	}
	else {
		throw InternalInconsistencyException("Argument::dump()",
			"_type is not MENTION_ARG, PROPOSITION_ARG, or TEXT_ARG");
	}
	return str;
}

void Argument::updateObjectIDTable() const {
	if (_type == PROPOSITION_ARG)
        ObjectIDTable::addObject(_arg.prop);
	else if (_type == TEXT_ARG)
		ObjectIDTable::addObject(_arg.node);

	ObjectIDTable::addObject(this);
}

#define BEGIN_ARGUMENT (reinterpret_cast<const wchar_t*>(StateLoader::IntegerCompressionStart + StateLoader::ArgumentOffset))

void Argument::saveState(StateSaver *stateSaver) const {
	if (ParamReader::isParamTrue("use_state_file_integer_compression")) {
		stateSaver->beginList(BEGIN_ARGUMENT, this);
	} else {
		stateSaver->beginList(L"Argument", this);
	}

	stateSaver->saveInteger(_type);
	stateSaver->saveSymbol(_roleSym);

	switch (_type) {
	case MENTION_ARG:
		stateSaver->saveInteger(_arg.mention_index);
		break;
	case PROPOSITION_ARG:
        stateSaver->savePointer(_arg.prop);
		break;
	case TEXT_ARG:
		stateSaver->savePointer(_arg.node);
	}

	stateSaver->endList();
}

void Argument::loadState(StateLoader *stateLoader) {
	//Use the integer replacement for "Argument" if the state file was compressed
	int id = stateLoader->beginList(stateLoader->useCompressedState() ? BEGIN_ARGUMENT : L"Argument");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	//if (stateLoader->_stateByteBuffer)
	//	std::cerr << "    arg type: " << stateLoader->_stateByteBuffer->peekInt() << std::endl;
    _type = static_cast<Type>(stateLoader->loadInteger());
	_roleSym = stateLoader->loadSymbol();

	switch (_type) {
	case MENTION_ARG:
		//if (stateLoader->_stateByteBuffer)
		//	std::cerr << "    arg mention_index: " << stateLoader->_stateByteBuffer->peekInt() << std::endl;
		_arg.mention_index = stateLoader->loadInteger();
		break;
	case PROPOSITION_ARG:
		//if (stateLoader->_stateByteBuffer)
		//	std::cerr << "    arg prop ptr: " << std::hex << stateLoader->_stateByteBuffer->peekInt() << std::dec << std::endl;
		_arg.prop = 
			static_cast<const Proposition*>(stateLoader->loadPointer());
		break;
	case TEXT_ARG:
		//if (stateLoader->_stateByteBuffer)
		//	std::cerr << "    arg node ptr: " << std::hex << stateLoader->_stateByteBuffer->peekInt() << std::dec << std::endl;
		_arg.node = 
			static_cast<const SynNode*>(stateLoader->loadPointer());
	}

	stateLoader->endList();
}

void Argument::resolvePointers(StateLoader * stateLoader) {
	if (_type == PROPOSITION_ARG)
		_arg.prop = static_cast<const Proposition*>(
			stateLoader->getObjectPointerTable().getPointer(_arg.prop));
	else if (_type == TEXT_ARG)
		_arg.node = static_cast<const SynNode*>(
			stateLoader->getObjectPointerTable().getPointer(_arg.node));
}

//mrf convenince accessors for getting the start and end offsets of an argument
int Argument::getStartToken(const MentionSet *mentionSet) const{
	if( _type == MENTION_ARG){
		Mention* ment = mentionSet->getMention(_arg.mention_index);
		return ment->getNode()->getStartToken();
	}
	else if(_type == PROPOSITION_ARG){
		int last_tok = mentionSet->getParse()->getRoot()->getEndToken() + 1;
		int startTok = last_tok;
		int endTok = -1;
		_arg.prop->getStartEndTokenProposition(mentionSet, startTok, endTok);
		if(startTok == last_tok){
			SessionLogger::dbg("SERIF")<<"Argument::getStartToken(): Defaulting to 0 start token for Prop-type Argument: "
				<<_arg.prop->toDebugString()<<std::endl;
			return 0;
		}
		return startTok;
	}
	else if(_type == TEXT_ARG){
		return _arg.node->getHighestHead()->getStartToken();
	}
	else{
		throw InternalInconsistencyException("Argument::getStartToken()",
			"getStartToken(), type does not match MENTION_ARG, PROPOSITON_ARG, or TEXT_ARG");
	}
}
int Argument::getEndToken(const MentionSet *mentionSet) const {
	if( _type == MENTION_ARG){
		Mention* ment = mentionSet->getMention(_arg.mention_index);
		return ment->getNode()->getEndToken();
	}
	else if(_type == PROPOSITION_ARG){
		int endTok = -1;
		int startTok = mentionSet->getParse()->getRoot()->getEndToken()+1;
		_arg.prop->getStartEndTokenProposition(mentionSet, startTok, endTok);
		if(endTok == -1){
			SessionLogger::dbg("SERIF")<<"Argument::getEndToken(): Defaulting to last token in sentence for Prop-type Argument: "
				<<_arg.prop->toDebugString()<<std::endl;
				return mentionSet->getParse()->getRoot()->getEndToken();
		}
		return endTok;
	}
	else if(_type == TEXT_ARG){
		return _arg.node->getHighestHead()->getEndToken();
	}
	else{
		throw InternalInconsistencyException("Argument::getEndToken()",
			"getStartToken(), type does not match MENTION_ARG, PROPOSITON_ARG, or TEXT_ARG");
	}
}

const wchar_t* Argument::XMLIdentifierPrefix() const {
	return L"arg";
}

void Argument::saveXML(SerifXML::XMLTheoryElement argumentElem, const Theory *context) const {
	using namespace SerifXML;
	const MentionSet *mentionSet = dynamic_cast<const MentionSet*>(context);
	if (context == 0)
		throw InternalInconsistencyException("Argument::saveXML()", "Expected context to be a MentionSet");
	switch(_type) {
		case MENTION_ARG:
			argumentElem.saveTheoryPointer(X_mention_id, getMention(mentionSet));
		break;
		case PROPOSITION_ARG:
			argumentElem.saveTheoryPointer(X_proposition_id, getProposition());
		break;
		case TEXT_ARG:
			argumentElem.saveTheoryPointer(X_syn_node_id, getNode());
		break;
		default:
			throw InternalInconsistencyException("Argument::saveXML()", "Unknown arg type");
	}
	if (!_roleSym.is_null())
		argumentElem.setAttribute(X_role, _roleSym);
	
}

void Argument::loadXML(SerifXML::XMLTheoryElement argElem)
{
	using namespace SerifXML;
	_roleSym = argElem.getAttribute<Symbol>(X_role, Symbol());
	if (const Mention* mention = argElem.loadOptionalTheoryPointer<Mention>(X_mention_id)) {
		_type = MENTION_ARG;
		_arg.mention_index = mention->getUID().index();
	}
	else if (const SynNode* synNode = argElem.loadOptionalTheoryPointer<SynNode>(X_syn_node_id)) {
		_type = TEXT_ARG;
		_arg.node = synNode;
	} else if (argElem.hasAttribute(X_proposition_id)) {
		_type = PROPOSITION_ARG;
		_arg.prop = 0; // Filled in by resolvePointers(XMLTheoryElement)
	} else {
		argElem.reportLoadError("Expected a pointer to a mention, syn_node, or proposition");
	}
}

void Argument::resolvePointers(SerifXML::XMLTheoryElement argElem) {
	using namespace SerifXML;
	if (_type == PROPOSITION_ARG) 
		_arg.prop = argElem.loadTheoryPointer<Proposition>(X_proposition_id);
}
