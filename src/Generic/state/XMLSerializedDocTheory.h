// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.
//
// XMLSerializedDocTheory: serialization & deserialization of SERIF's annotations to XML

#ifndef XML_SERIALIZED_DOC_THEORY_H
#define XML_SERIALIZED_DOC_THEORY_H

#include <string>
#include <fstream>
#include <set>
#include <xercesc/util/XercesDefs.hpp>
#include "Generic/state/XMLIdMap.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/LexicalEntry.h"

// Forward declarations.
XERCES_CPP_NAMESPACE_BEGIN
	class DOMDocument;
	class DOMElement;
XERCES_CPP_NAMESPACE_END
class LocatedString;
class DocTheory;
class Region;
class ValueMention;
class Token;
class Document;
class Lexicon;
class LexicalEntry;

namespace SerifXML {

/** An XML-serialized version of a DocTheory object (which holds all
  * of SERIF's annotations for a single document).  XMLSerializedDocTheory can
  * be used for both serializing and deserializing DocTheory objects:
  *
  *   - Serialize:     XMLSerializedDocTheory(docTheory).save(filename)
  *   - Deserialize:   XMLSerializedDocTheory(filename).generateDocTheory()
  *
  * XMLSerializedDocTheory is implemented as a wrapper around a 
  * xercesc::DOMDocument object (owned by the XMLSerializedDocTheory).  
  */
class XMLSerializedDocTheory {
public:
	/** This version number is used to mark any serialized files.  
	  * During deserialization, we can check the version number in
	  * order to provide backwards-compatible behavior. */
	static const size_t SERIF_XML_VERSION = 22;

	~XMLSerializedDocTheory();

//==================== Serialization Interface ========================

	/** Create an XML serialization for a document theory. */
	XMLSerializedDocTheory(const DocTheory* docTheory);

	XMLSerializedDocTheory(const DocTheory* docTheory, const XMLTheoryElement::SerializationOptions::OptionMap &optionMap);

	/** Save this XMLSerializedDocTheory to disk. */
	void save(const char* filename) const;
	void save(const wchar_t* filename) const;
	void save(std::wstring filename) const { save(filename.c_str()); }
	void save(std::string filename) const { save(filename.c_str()); }
	void save(std::ostream &out) const;

	/** Return a pointer to the DOMDocument object used to hold the contents
	  * of the serialized DocTheory.  The returned DOMDocument is owned by
	  * the XMLSerializedDocTheory, and the returned pointer is invalidated
	  * when the XMLSerializedDocTheory is deleted. */
	xercesc::DOMDocument* getXercesXMLDoc() { return _xercesDOMDocument; }

	/** Adopt (i.e., take ownership of) the DOMDocument object that is
	  * owned by this XMLSerializedDocTheory.  The caller becomes responsible
	  * for releasing the memory of the returned DOMDocument (by calling
	  * DOMDocument::release()).  The XMLSerializedDocTheory's own pointer
	  * is set to NULL.  It is an error to use the XMLSerializedDocTheory
	  * in any way (other than calling its destructor) after this method 
	  * is called. */
	xercesc::DOMDocument* adoptXercesXMLDoc();

	/** Return an XMLTheoryElement pointing at the root of this serialized
	  * DocTheory. */
	XMLTheoryElement getDocumentElement();

//=================== Deserialization Interface =======================

	/** Load a serialized document from disk.  If the file does not contain
	  * well-formed XML, then throw an UnexpectedInputException.  The
	  * contents of the XML document are *not* read, nor converted to a
	  * DocTheory object, util generateDocTheory() is called. */
	XMLSerializedDocTheory(const char* filename);
	XMLSerializedDocTheory(const wchar_t* filename);

	/** Create a serialized document based on an existing 
	  * xercesc::DOMDocument.  The XMLSerializedDocTheory takes ownership
	  * of its argument.  The DocTheory is not actually deserialized when
	  * this constructor is called -- that happens during the call to 
	  * generateDocTheory(). */
	XMLSerializedDocTheory(xercesc::DOMDocument *domDocument);

	/** Generate a new Document and DocTheory from the XML contents of this
	  * serialized DocTheory.  The new Document and DocTheory are returned
	  * as a std::pair.  The caller takes ownership of both the Document and
	  * the DocTheory (i.e., the caller is responsible for deleting them). 
	  *
	  * If the XML contained in this XMLSerializedDocTheory does not properly 
	  * describe a DocTheory, then throw an UnexpectedInputException. */
	std::pair<Document*, DocTheory*> generateDocTheory();

//=============== Methods Used While (De)Serializing ==================

	/** Return a pointer to the XML identifier map, which can be used to
	  * translate an identifier to a theory object, or vice versa. */
	XMLIdMap* getIdMap() { return &_idMap; }

	/** Record a pointer to the original text for this serialized DocTheory.
	  * This may only be called once, and originalText may not be NULL. */
	void setOriginalText(const LocatedString* originalText);

	/** Return the original (unmodified) text of the document, or NULL if
	  * the document text has not been loaded yet. */
	const LocatedString* getOriginalText() { return _originalText; }

	/** Register a mention's UID, allowing it to be looked up later by
	  * its UID.  This is used during serialization, because SERIF
	  * internally uses MentionUID's to point to mentions, but we need
	  * to get the actual Mention object, so we can look up its XML id. 
	  *
	  * [XX] WARNING: This would cause problems if we ever tried to use
	  * a beam to store multiple MentionTheories, since we would end up
	  * with multiple mentions with the same UID.  But currently we never
	  * do that, so we'll leave that problem to be solved later. */
	void registerMentionId(const Mention* mention);

	/** Return the Mention object with a given MentionUID.  If no
	  * mention has been registered with the given UID, then throw an
	  * InternalInconsistencyException. */
	const Mention* lookupMentionById(MentionUID id);

	/** Record the token-index for a given token (i.e., its position in 
	  * the TokenSequence that owns it).  This is used during 
	  * deserialization, since the XML serialization contains token
	  * pointers, but SERIF's internal data structures just use token
	  * indices. */
	void registerTokenIndex(const Token* tok, size_t sentno, size_t tokno);

	/** Return the token-index of a given token (i.e., its position in 
	  * the TokenSequence that owns it).  If the given token has not 
	  * had its index registered, then throw an InternalInconsistencyException. */
	size_t lookupTokenIndex(const Token* tok);
	size_t lookupTokenSentNo(const Token* tok);

	/** Return a list of lexical entries that have been used by tokens in this
	  * document.  These get serialized by DocTheory::saveXML(), after everything
	  * else has been serialized. */
	const LexicalEntry::LexicalEntrySet &getLexicalEntriesToSerialize() {
		return _lexicalEntries; }

	/** Record the fact that we used a lexical entry during serialization.  This
	  * adds a LexicalEntry to the set that will be returned by the 
	  * getLexicalEntriesToSerialize() method.  It is called when serializing
	  * lexical tokens. */
	void registerLexicalEntry(const LexicalEntry* le);

//======================= Serialization Options =======================

	const XMLTheoryElement::SerializationOptions& getOptions() const { return _options; }

	void setOptions(const XMLTheoryElement::SerializationOptions::OptionMap &optionMap);

	void copyOptionsFrom(const XMLSerializedDocTheory& source);

private:
//======================== Helper Methods ============================

	void load(const char* filename);
	void readDocTheory(const DocTheory* docTheory);

//======================== Member Variables ============================

	// Top-level DOM xml document.  This is needed to create new DOM elements.
	xercesc::DOMDocument *_xercesDOMDocument;

	// Original text of the document.  This is used to check consistency and get offsets
	// during serialization and deserialization.  Note: the XMLSerializedDocTheory does not own
	// the originalText object -- it is owned by the Document object that we are either
	// serializing or deserializing.  This should be NULL if we are not in the process
	// of serializing or deserializing a DocTheory.
	const LocatedString *_originalText;

	// Mapping between theory objects and XML identifier strings.
	XMLIdMap _idMap;

	// Mapping from token to location (sentence number and token number)
	std::map<const Token*, size_t> _tokenToIndex;
	std::map<const Token*, size_t> _tokenToSentNo;

	// Mapping from Mention UID to mention
	std::map<MentionUID, const Mention*> _mentionMap;

	// Serialization options
	XMLTheoryElement::SerializationOptions _options;

	// The set of lexical entries used in this document.
	LexicalEntry::LexicalEntrySet _lexicalEntries;
};

} // namespace SerifXML

#endif
