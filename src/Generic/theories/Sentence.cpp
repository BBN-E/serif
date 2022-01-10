// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/Sentence.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/Zone.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/StringTransliterator.h"
#include <wchar.h>
#include "Generic/state/XMLStrings.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/reader/DefaultDocumentReader.h"

// memory stuff
const size_t Sentence::_block_size = SENTENCE_BLOCK_SIZE;
Sentence* Sentence::_freeList = 0;

Sentence::Sentence(const Document *doc, const Region *region, int sent_no, const LocatedString *content)
	: _doc(doc), _region(region), _sent_no(sent_no), _isAnnotated(true)
{
	if(content != 0){
		_n_chars = content->length();
		_content = _new LocatedString(*content);
	}
	else{
		_n_chars = 0;
		_content = 0;
	}
}

Sentence::~Sentence() {
	delete _content;
}

void Sentence::dump(std::ostream &out, int indent) {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "(sentence " << _sent_no << "):";
	if (_content != 0) {
		out << newline;
		_content->dump(out, indent + 2);
	}
	delete[] newline;
}

void Sentence::updateObjectIDTable() const {
	throw InternalInconsistencyException("Sentence::updateObjectIDTable()",
										"Using unimplemented method.");
}

void Sentence::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("Sentence::saveState()",
										"Using unimplemented method.");

}

void Sentence::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("Sentence::resolvePointers()",
										"Using unimplemented method.");

}

// I think this only makes sense for English, but it won't do any harm being
// at the language-independent level.
int Sentence::getSentenceCase() const {
	bool is_all_upper = true;
	bool is_all_lower = true;
	
	// this will happen for faked DocTheorys
	if (_content == 0)
		return MIXED;

	for (int k = 0; k < _content->length(); k++) {
		if (iswupper(_content->charAt(k))) {
			is_all_lower = false;
			if (!is_all_upper)
				return MIXED;
		} else if (iswlower(_content->charAt(k))) {
			is_all_upper = false;
			if (!is_all_lower)
				return MIXED;
		} 
	}

	if (is_all_lower)
		return LOWER;
	else if (is_all_upper)
		return UPPER;
	else return MIXED;
}

void Sentence::setAnnotationFlag(bool flag){
	_isAnnotated = flag;
}
// memory stuff
#if 0
void* Sentence::operator new(size_t)
{
    Sentence* p = _freeList;
    if (p) {
        _freeList = p->next;
    } else {
        Sentence* newBlock = static_cast<Sentence*>(::operator new(
            _block_size * sizeof(Sentence)));
        for (size_t i = 1; i < (_block_size - 1); i++)
            newBlock[i].next = &newBlock[i + 1];
        newBlock[_block_size - 1].next = 0;
        p = newBlock;
        _freeList = &newBlock[1];
    }
    return p;
}
void Sentence::operator delete(void* object)
{
    Sentence* p = static_cast<Sentence*>(object);
    p->next = _freeList;
    _freeList = p;
}
#endif

// Ended up not using this state-saving stuff, but I didn't want to delete
// it because we may end up saving sentences some day after all. -- SRS
#if 0
void Sentence::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);

	_content->updateObjectIDTable();
}

void Sentence::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"Sentence", this);

	stateSaver->saveInteger(_sent_no);
	stateSaver->saveInteger(_n_chars);
	_content->saveState(stateSaver);

	stateSaver->endList();
}

Sentence::Sentence(StateLoader *stateLoader) {
	id = stateLoader->beginList(L"Sentence");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	_sent_no = stateLoader->loadInteger();
	_n_chars = stateLoader->loadInteger();
	_content = _new LocatedString(stateLoader);

	stateLoader->endList();
}

void Sentence::resolvePointers() {
	_content->resolvePointers();
}

#endif

const wchar_t* Sentence::XMLIdentifierPrefix() const {
	return L"sent";
}

void Sentence::saveXML(SerifXML::XMLTheoryElement sentenceElem, const Theory *context) const {
	// We do not serialize getSentNumber(), since it's redundant.
	// We do not serialize getNChars(), since it's redundant.
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("Sentence::saveXML", "Expected context to be NULL");
	sentenceElem.setAttribute(X_is_annotated, isAnnotated());
	getString()->saveXML(sentenceElem);
	const Document* current_doc=getDocument();
	//if(current_doc->getSourceType()==DefaultDocumentReader::DF_SYM){
	//	EDTOffset regionStart=getRegion()->getStartEDTOffset();
	//	EDTOffset regionEnd=getRegion()->getEndEDTOffset();
	//	Zone* zone=current_doc->findZone(regionStart,regionEnd);
	//	sentenceElem.saveTheoryPointer(X_zone_id, zone);
	//}
	//else{
	sentenceElem.saveTheoryPointer(X_region_id, getRegion());
	//}
	if (getNChars() != getString()->length())
		throw InternalInconsistencyException("Sentence::saveXML", "inconsistent value for nChars");
}

/** Requires: regions must be identified. */
Sentence::Sentence(SerifXML::XMLTheoryElement sentenceElem, const Document *doc, int sent_no) {
	using namespace SerifXML;
	sentenceElem.loadId(this);
	_doc = doc;
	_sent_no = sent_no;
	_content = _new LocatedString(sentenceElem);
	_n_chars = getString()->length(); // based on _content.
	_isAnnotated = sentenceElem.getAttribute<bool>(X_is_annotated, true);
	_region = sentenceElem.loadTheoryPointer<Region>(X_region_id);
}
