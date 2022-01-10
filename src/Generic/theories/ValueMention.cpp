// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/Value.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/common/InternalInconsistencyException.h"

#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"


ValueMention::ValueMention() 
	: _uid(0), _sent_no(-1), _type(), _start_tok(0), _end_tok(0), _docValue(NULL) {}

ValueMention::ValueMention(int sent_no, ValueMentionUID uid, int start, int end, Symbol type)
	: _uid(uid), _sent_no(sent_no), _type(ValueType(type)), _start_tok(start), _end_tok(end), _docValue(NULL) {
	if (_sent_no < 0 || _start_tok < 0 || _end_tok < 0) {	
		throw InternalInconsistencyException(
			"ValueMention::ValueMention()",
			"ValueMention's sent_no, start_tok, and end_tok must all be non-negative!");
	}
}

ValueMention::ValueMention(ValueMention &source)
	: _uid(source._uid), _sent_no(source._sent_no), _type(source._type), _start_tok(source._start_tok), _end_tok(source._end_tok), _docValue(NULL) {
	if (_sent_no < 0 || _start_tok < 0 || _end_tok < 0) {	
		throw InternalInconsistencyException(
			"ValueMention::ValueMention()",
			"ValueMention's sent_no, start_tok, and end_tok must all be non-negative!");
	}
}

ValueMentionUID ValueMention::makeUID(int sentence, int index, bool doclevel) {
	if (doclevel || index < MAX_SENTENCE_VALUES) {
		return ValueMentionUID(sentence, index);
	}
	else {
		std::stringstream errMsg;
		errMsg << "Sentence " << sentence << " has too many 'value' objects (dates, numbers, etc.); "
			   << "the limit is " << MAX_SENTENCE_VALUES << ".";
		throw InternalInconsistencyException("ValueMention::makeUID()",errMsg.str().c_str());
	}
}

int ValueMention::getSentenceNumber() const {
	return _sent_no;
}

int ValueMention::getIndex() const {
        return getIndexFromUID(_uid);
}

int ValueMention::getIndexFromUID(ValueMentionUID uid) {
        return uid.index();
}

std::wstring ValueMention::toCasedTextString(const TokenSequence* tokens) const {
	std::wstringstream wss;
	for (int i = _start_tok; i <= _end_tok; i++) {
		wss << tokens->getToken(i)->getSymbol().to_string();
		if (i != _end_tok)
			wss << L" ";
	}	
	return wss.str();
}

std::string ValueMention::toDebugString(const TokenSequence *tokens) const {
	std::string result = _type.getNameSymbol().to_debug_string();
	result += ": ";
	for (int i = _start_tok; i <= _end_tok; i++) {
		result += tokens->getToken(i)->getSymbol().to_debug_string();
		if (i != _end_tok)
			result += " ";
	}
	return result;
}
std::wstring ValueMention::toString(const TokenSequence *tokens) const {
	std::wstring result = _type.getNameSymbol().to_string();
	result += L": ";
	for (int i = _start_tok; i <= _end_tok; i++) {
		result += tokens->getToken(i)->getSymbol().to_string();
		if (i != _end_tok)
			result += L" ";
	}
	return result;
}


void ValueMention::dump(std::ostream &out, int indent) const {
	out << "Value Mention " <<  getUID().toInt()
		<< " (" << _type.getNameSymbol().to_debug_string() << "): "
		<< "tokens " << _start_tok << ":" << _end_tok;

	// Only print the value if it exists (should be set by doc-values)
	if (_docValue && (!_docValue->getTimexVal().is_null())) {
		out << " [" << _docValue->getTimexVal() << "]";
	}
}

void ValueMention::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);
}

void ValueMention::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"ValueMention", this);

	stateSaver->saveInteger(_uid.toInt());
	stateSaver->saveInteger(_sent_no);
	stateSaver->saveInteger(_start_tok);
	stateSaver->saveInteger(_end_tok);
	stateSaver->saveSymbol(_type.getNameSymbol());

	stateSaver->endList();
}

void ValueMention::loadState(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"ValueMention");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	if (stateLoader->getVersion() >= std::make_pair(1,10)) {
		_uid = ValueMentionUID(stateLoader->loadInteger());
	} else {
		int n = stateLoader->loadInteger();
		int sent_no = static_cast<int>(n/100);    // MAX_SENTENCE_VALUES used to be 100
		int index = static_cast<int>(n%100);      // MAX_SENTENCE_VALUES used to be 100
		_uid = ValueMentionUID(sent_no, index);
	}

	_sent_no = stateLoader->loadInteger();
	_start_tok = stateLoader->loadInteger();
	_end_tok = stateLoader->loadInteger();
	_type = ValueType(stateLoader->loadSymbol());
	
	stateLoader->endList();
}

void ValueMention::resolvePointers(StateLoader * stateLoader) {}

const wchar_t* ValueMention::XMLIdentifierPrefix() const {
	return L"vm";
}

void ValueMention::saveXML(SerifXML::XMLTheoryElement valuementionElem, const Theory *context) const {
	using namespace SerifXML;
	const TokenSequence *tokSeq = dynamic_cast<const TokenSequence*>(context);
	if (context == 0)
		throw InternalInconsistencyException("ValueMention::saveXML", "Expected context to be a TokenSequence");
	valuementionElem.setAttribute(X_value_type, _type.getNameSymbol());
	// Pointers to start/end tokens:
	const Token *startTok = tokSeq->getToken(_start_tok);
	valuementionElem.saveTheoryPointer(X_start_token, startTok);
	const Token *endTok = tokSeq->getToken(_end_tok);
	valuementionElem.saveTheoryPointer(X_end_token, endTok);
	// Start/end offsets:
	const OffsetGroup &startOffset = startTok->getStartOffsetGroup();
	const OffsetGroup &endOffset = endTok->getEndOffsetGroup();
	valuementionElem.saveOffsets(startOffset, endOffset);
	// Contents (optional):
	if (valuementionElem.getOptions().include_spans_as_elements) 
		valuementionElem.addText(valuementionElem.getOriginalTextSubstring(startOffset, endOffset));
	if (valuementionElem.getOptions().include_spans_as_comments)
		valuementionElem.addComment(valuementionElem.getOriginalTextSubstring(startOffset, endOffset));
	// Record sentence number only if it doesn't match the UID (this should
	// only happen for ValueMentions in the Document ValueMentionSet)
	if (_sent_no != getUID().sentno())
		valuementionElem.setAttribute(X_sent_no, _sent_no);
	// Don't record the pointer to the docValue -- we can reconstruct it later.

}

ValueMention::ValueMention(SerifXML::XMLTheoryElement vmElem, int sent_no, int valmention_no)
: _sent_no(sent_no), _uid(sent_no, valmention_no), _docValue(NULL)
{
	using namespace SerifXML;
	vmElem.loadId(this);
	if (vmElem.hasAttribute(X_value_type))
		_type = ValueType(vmElem.getAttribute<Symbol>(X_value_type, Symbol()));
	else {
		_type = ValueType();
		vmElem.reportLoadWarning("No type specified for ValueMention");
	}
	XMLSerializedDocTheory *xmldoc = vmElem.getXMLSerializedDocTheory();
	_start_tok = static_cast<int>(xmldoc->lookupTokenIndex(vmElem.loadTheoryPointer<Token>(X_start_token)));
	_end_tok = static_cast<int>(xmldoc->lookupTokenIndex(vmElem.loadTheoryPointer<Token>(X_end_token)));
	if (vmElem.hasAttribute(X_sent_no))
		_sent_no = vmElem.getAttribute<int>(X_sent_no);
	// The pointer to docValue (if non-NULL) gets set by the Value(SerifXML::XMLTheoryElement) constructor
}
