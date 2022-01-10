// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLIdMap.h"
#include "Generic/state/XMLStrings.h"

#include "Generic/theories/DocTheory.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/XMLUtil.h"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <boost/foreach.hpp>

using namespace xercesc;

namespace SerifXML {

XMLSerializedDocTheory::~XMLSerializedDocTheory() {
	if (_xercesDOMDocument)
		_xercesDOMDocument->release();
	_xercesDOMDocument = 0;
}

xercesc::DOMDocument* XMLSerializedDocTheory::adoptXercesXMLDoc() {
	DOMDocument* return_value = _xercesDOMDocument;
	_xercesDOMDocument = 0;
	_originalText = 0;
	return return_value;
}


//==================== Serialization Interface ========================

XMLSerializedDocTheory::XMLSerializedDocTheory(const DocTheory* docTheory)
: _xercesDOMDocument(0), _originalText(0)
{ readDocTheory(docTheory); }

XMLSerializedDocTheory::XMLSerializedDocTheory(const DocTheory* docTheory, 
											   const XMLTheoryElement::SerializationOptions::OptionMap &optionMap)
: _xercesDOMDocument(0), _originalText(0), _options(optionMap)
{ readDocTheory(docTheory); }

void XMLSerializedDocTheory::readDocTheory(const DocTheory* docTheory) {
	DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(X_Core);

	_xercesDOMDocument = impl->createDocument(0, X_SerifXML, 0);
	_originalText = docTheory->getDocument()->getOriginalText();

	XMLTheoryElement rootElem(this, _xercesDOMDocument->getDocumentElement());
	rootElem.setAttribute(X_version, SERIF_XML_VERSION);
	rootElem.saveChildTheory(X_Document, docTheory);

	// We're done serializing, so relinquish our _originalText pointer.
	_originalText = 0;
}

void XMLSerializedDocTheory::save(const char* filename) const {
	XMLUtil::saveXercesDOMToFilename(_xercesDOMDocument, filename);
}

void XMLSerializedDocTheory::save(const wchar_t* filename) const {
	save(OutputUtil::convertToUTF8BitString(filename).c_str());
}

void XMLSerializedDocTheory::save(std::ostream &out) const {
	XMLUtil::saveXercesDOMToStream(_xercesDOMDocument, out);
}

void XMLSerializedDocTheory::setOptions(const XMLTheoryElement::SerializationOptions::OptionMap &optionMap) {
	_options = XMLTheoryElement::SerializationOptions(optionMap);
}

void XMLSerializedDocTheory::copyOptionsFrom(const XMLSerializedDocTheory& source) {
	_options = source._options;
}


//=================== Deserialization Interface =======================

XMLSerializedDocTheory::XMLSerializedDocTheory(const wchar_t* filename)
: _xercesDOMDocument(0), _originalText(0)
{
	load(OutputUtil::convertToUTF8BitString(filename).c_str());
}

XMLSerializedDocTheory::XMLSerializedDocTheory(const char* filename)
: _xercesDOMDocument(0), _originalText(0)
{
	load(filename);
}

void XMLSerializedDocTheory::load(const char* filename) {
	if (_xercesDOMDocument != 0)
		throw InternalInconsistencyException("XMLSerializedDocTheory::load",
			"load should not be called twice with the same XMLSerializedDocTheory object");
	_originalText = 0;
	_xercesDOMDocument = XMLUtil::loadXercesDOMFromFilename(filename);
	// Should we check the SerifXML version of the document?
}

XMLSerializedDocTheory::XMLSerializedDocTheory(xercesc::DOMDocument *domDocument)
: _xercesDOMDocument(domDocument), _originalText(0)
{
	// Record the SerifXML version.
	// [XX] Should this check for an existing version number?  And if we find
	// one, should we try to provide backwards compatibility?
	XMLTheoryElement rootElem(this, _xercesDOMDocument->getDocumentElement());
	rootElem.setAttribute(X_version, SERIF_XML_VERSION);
}

std::pair<Document*, DocTheory*> XMLSerializedDocTheory::generateDocTheory() {
	DocTheory *docTheory = _new DocTheory(getDocumentElement());
	return std::make_pair(docTheory->getDocument(), docTheory);
}

XMLTheoryElement XMLSerializedDocTheory::getDocumentElement() {
	XMLTheoryElement rootElem(this, _xercesDOMDocument->getDocumentElement());
    if (rootElem.hasTag(X_Document))
        return rootElem;
    else if (rootElem.hasTag(X_SerifXML)) {
        std::vector<XMLTheoryElement> doclist = rootElem.getChildElementsByTagName(X_Document);
        if (doclist.size() == 1)
            return doclist[0];
        else if (doclist.size() == 0)
            throw UnexpectedInputException("XMLSerializedDocTheory::load",
                           "No 'Document' element found in xml file!");
        else
            throw UnexpectedInputException("XMLSerializedDocTheory::load",
                        "Multiple 'Document' element found in xml file!");
    } else {
        std::ostringstream err;
        err << "Bad SerifXML root tag: <" 
            << transcodeToStdString(rootElem.getTag())
            << ">; expected <SerifXML> or <Document>";
        throw UnexpectedInputException("XMLSerializedDocTheory::load", 
                                       err.str().c_str());

    }

}

//=============== Methods Used While (De)Serializing ==================

void XMLSerializedDocTheory::setOriginalText(const LocatedString* originalText) {
	if (originalText == 0)
		throw InternalInconsistencyException("XMLSerializedDocTheory::setOriginalText",
			"Attempt to set the original text to NULL");
	if (_originalText != 0) 
		throw InternalInconsistencyException("XMLSerializedDocTheory::setOriginalText",
			"Attempt to set the original text on an XMLSerializedDocTheory that already has an original text");
	_originalText = originalText;

	// Sanity check: make sure that character offsets and positions line up:
	if (originalText->length() == 0) { return; } // Sanity check makes no sense if we have the empty string.
	CharOffset offset = originalText->start<CharOffset>();
	for (int pos=0; pos<originalText->length(); ++pos) {
		if ((originalText->start<CharOffset>(pos) != offset) ||
			(originalText->end<CharOffset>(pos) != offset))
			throw InternalInconsistencyException("XMLSerializedDocTheory::setOriginalText",
				"Char offsets not contiguous -- has this LocatedString been modified?");
		++offset;
	}
}

void XMLSerializedDocTheory::registerMentionId(const Mention* mention) {
	MentionUID id = mention->getUID();
	if (_mentionMap.find(id) != _mentionMap.end())
		throw InternalInconsistencyException("XMLSerializedDocTheory::registerMentionId",
			"Mention has already been registered.");
	_mentionMap[id] = mention;
}

const Mention* XMLSerializedDocTheory::lookupMentionById(MentionUID id) {
	typedef std::map<MentionUID, const Mention*>::iterator MentionMapIter;
	MentionMapIter iter = _mentionMap.find(id);
	if (iter == _mentionMap.end())
		throw InternalInconsistencyException("XMLSerializedDocTheory::lookupMentionById",
			"Mention has not been registered.");
	else
		return iter->second;
}

void XMLSerializedDocTheory::registerTokenIndex(const ::Token* tok, size_t sentno, size_t tokno) {
	if (_tokenToIndex.find(tok) != _tokenToIndex.end())
		throw InternalInconsistencyException("XMLSerializedDocTheory::registerTokenIndex",
			"Token's index has already been registered.");
	_tokenToIndex[tok] = tokno;
	_tokenToSentNo[tok] = sentno;
}

size_t XMLSerializedDocTheory::lookupTokenIndex(const ::Token* tok) {
	typedef std::map<const ::Token*, size_t>::iterator TokenMapIter;
	TokenMapIter iter = _tokenToIndex.find(tok);
	if (iter == _tokenToIndex.end())
		throw InternalInconsistencyException("XMLSerializedDocTheory::lookupTokenIndex",
			"Token's index has not been registered.");
	else
		return iter->second;
}

size_t XMLSerializedDocTheory::lookupTokenSentNo(const ::Token* tok) {
	typedef std::map<const ::Token*, size_t>::iterator TokenMapIter;
	TokenMapIter iter = _tokenToSentNo.find(tok);
	if (iter == _tokenToSentNo.end())
		throw InternalInconsistencyException("XMLSerializedDocTheory::lookupTokenIndex",
			"Token's index has not been registered.");
	else
		return iter->second;
}

// Record the fact that we used a lexical entry (during serialization)
// The ids we generate for lexical entries are verbose and have no parent id.
void XMLSerializedDocTheory::registerLexicalEntry(const LexicalEntry* le) {
	// Add it to our list of lexical entries.  If it's new, then generate an XML id for it.
	if (_lexicalEntries.insert(le).second == true) {
		_idMap.generateId(le, 0, true);
		// Register any nested lexical entries as well.
		for(int i=0; i<le->getNSegments(); ++i)
			registerLexicalEntry(le->getSegment(i));
	}
}

} // namespace SerifXML
