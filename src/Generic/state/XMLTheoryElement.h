// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.
//
// XMLTheoryElement: serialization & deserialization of SERIF's theory objects
// to XML

#ifndef XML_THEORY_ELEMENT_H
#define XML_THEORY_ELEMENT_H

#include "Generic/state/XMLElement.h"
#include "Generic/common/Offset.h"

class Theory;
class Symbol;
class LocatedString;
XERCES_CPP_NAMESPACE_BEGIN
	class DOMElement;
XERCES_CPP_NAMESPACE_END
namespace SerifXML { 
	class XMLSerializedDocTheory; 
	class XMLIdMap;
}



namespace SerifXML {
/** A single element in the XML tree used to serialize SERIF's annotations
  * for a document (i.e., its "Theory" objects).  Each XMLTheoryElement 
  * corresponds with a single node in the XML tree.  Typically, each node in 
  * the tree also corresponds with a single theory object.  However, there
  * are cases where a single XML node corresponds to multiple theory objects;
  * and there are XML nodes that do not correspond with any theory object.
  *
  * Each XMLTheoryElement keeps a pointer to the XMLSerializedDocTheory that
  * contains the element.  This can be used to translate between theory
  * pointers and XML identifiers when serializing or deserializing a
  * document theory.
  *
  * XMLElement objects are meant to be passed by value.  (They are basically
  * a wrapper for a pair of pointers, so they are cheap to copy).  As a 
  * consequence, they should not be used polymorphically and no methods
  * should be made virtual.
  */
class XMLTheoryElement : public XMLElement {
public:
	XMLTheoryElement(); // Constructs the "NULL" XMLTheoryElement.
	~XMLTheoryElement() {};

	XMLTheoryElement(const XMLTheoryElement& other); // CopyConstructor
	XMLTheoryElement& operator=(const XMLTheoryElement& other); // Assignment operator

//=========================== XML Children ===============================

	/** Return a pointer to the XMLSerializedDocTheory that contains 
	* this XMLTheoryElement. */
	XMLSerializedDocTheory* getXMLSerializedDocTheory() const;

	/** Return a vector containing the child elements of this element,
	* in order. */
	std::vector<XMLTheoryElement> getChildElements() const;

	/** Return the child element with the given tag name.  If no child
	  * element has the given tag name, or if multiple child elements
	  * have the given tag name, then throw an UnexpectedInputException. */
	XMLTheoryElement getUniqueChildElementByTagName(const XMLCh* tag) const;

	/** Return the child element with the given tag name.  If no child
	  * element has the given tag name, return a NULL XMLTheoryElement.  If
	  * multiple child elements have the given tag name, then throw an 
	  * UnexpectedInputException. */
	XMLTheoryElement getOptionalUniqueChildElementByTagName(const XMLCh* tag) const;

	/** Return a vector containing the child elements with the given tag
	  * name.  By default, a warning will be issued if this element 
	  * contains any children that do *not* have the specified tag name, 
	  * since the typical use case in our XML serialization is to have 
	  * lists consist of elements with the same tag.  If you wish to allow
	  * for children with a different tag name, then set the optional
	  * warn_if_other_tags_found parameter to false. */
	std::vector<XMLTheoryElement> getChildElementsByTagName(const XMLCh* tag, 
		bool warn_if_other_tags_found=true) const;

	/** Return the single child element of this element.  If this element
	  * has no children, or multiple children, then throw an 
	  * UnexpectedInputException. */
	XMLTheoryElement getUniqueChildElement() const;

	/** Add a new child node to this element with the given tag, and return
	  * an XMLTheoryElement pointing to it.  The new element will be empty (no
	  * attributes or children). */
	XMLTheoryElement addChild(const XMLCh* tag);

	//=========================== Serialization ===============================

	/** Add a new child node to this element with the given tag, and use it
	  * to serialize the given theory object (by calling child->saveXML()).
	  * Return a pointer to the new element.  An "id" attribute will automatically
	  * be generated for the new child, and registerd in the identifier map,
	  * unless child->hasXMLId() is false. */
	XMLTheoryElement saveChildTheory(const XMLCh* tag, const Theory* child, const Theory* context=0);

	/** Add an attribute with the given name whose value is the identifier
	  * for the specified theory object. */
	void saveTheoryPointer(const XMLCh* attrName, const Theory* theory);
	void saveTheoryPointerList(const XMLCh* attrName, const std::vector<const Theory*>& theories);

	/** Generate and register an identifier for a child theory of this 
	  * element.  This is automatically called by saveChildTheory(), but
	  * it should be called directly if you are serializing a collection
	  * of objects that may have pointers between individual items
	  * (before calling saveChildTheory() on those items). 
	  *
	  * This method does *not* set the id property of the child theory's
	  * XMLTheoryElement -- that is done by saveChildTheory(). */
	xstring generateChildId(const Theory *child) const; 

	/** Add attributes that record the offsets for a given substring
	  * of the original text.  Which offset(s) we report are controlled
	  * by serialization options.  The original text string may be used
	  * to translate one offset type to another. */
	void saveOffsets(OffsetGroup start_offset, OffsetGroup end_offset);

	/** Return a substring of the original text, spanning over the given
	  * pair of offsets.  The start and end character offsets are required
	  * to be defined; as is the original text itself. */
	std::wstring getOriginalTextSubstring(
		const OffsetGroup &start, const OffsetGroup &end) const;

//========================== Deserialization ==============================

	/** Deserialize a theory object with the given class (TheoryClass) 
	  * from the unique child element with specified tag.  If no such
	  * child element is found, throw an UnexpectedInputException.  This
	  * templated method can accept one to three extra arguments, which
	  * are passed on to the TheoryClass constructor.  The caller assumes
	  * ownership of the returned Theory object. */
	template<typename TheoryClass>
	TheoryClass *loadChildTheory(const XMLCh* tag) {
		return new TheoryClass(getUniqueChildElementByTagName(tag)); }
	template<typename TheoryClass, typename C1>
	TheoryClass *loadChildTheory(const XMLCh* tag, C1 c1) {
		return new TheoryClass(getUniqueChildElementByTagName(tag), c1); }
	template<typename TheoryClass, typename C1, typename C2>
	TheoryClass *loadChildTheory(const XMLCh* tag, C1 c1, C2 c2) {
		return new TheoryClass(getUniqueChildElementByTagName(tag), c1, c2); }

	/** Deserialize a theory object with the given class (TheoryClass) 
	  * from the unique child element with specified tag.  If no such
	  * child element is found, return NULL.  This templated method can 
	  * accept one to three extra arguments, which are passed on to the 
	  * TheoryClass constructor. The caller assumes ownership of the 
	  * returned Theory object. */
	template<typename TheoryClass>
	TheoryClass *loadOptionalChildTheory(const XMLCh* tag) {
		XMLTheoryElement childElem = getOptionalUniqueChildElementByTagName(tag);
		return (childElem ? new TheoryClass(childElem) : 0); }
	template<typename TheoryClass, typename C1>
	TheoryClass *loadOptionalChildTheory(const XMLCh* tag, C1 c1) {
		XMLTheoryElement childElem = getOptionalUniqueChildElementByTagName(tag);
		return (childElem ? new TheoryClass(childElem, c1) : 0); }
	template<typename TheoryClass, typename C1, typename C2>
	TheoryClass *loadOptionalChildTheory(const XMLCh* tag, C1 c1, C2 c2) {
		XMLTheoryElement childElem = getOptionalUniqueChildElementByTagName(tag);
		return (childElem ? new TheoryClass(childElem, c1, c2) : 0); }

	/** Return the theory pointed to by the specified attribute.  If the
	  * specified attribute is not found, raise an UnexpectedInputException. 
	  * If the specified attribute points to the wrong type of object,
	  * raise an UnexpectedInputException. */
	template<typename TheoryType>
	const TheoryType* loadTheoryPointer(const XMLCh* attrName) const {
		const TheoryType *t = dynamic_cast<const TheoryType*>(loadTheoryPointerHelper(attrName, false));
		if (t == 0) throw reportBadTheoryPointerType<TheoryType>(attrName);
		return t;
	}

	/** Return the theory pointed to by the specified attribute.  If the
	  * specified attribute is not found, return NULL.  If the specified 
	  * attribute points to the wrong type of object, raise an 
	  * UnexpectedInputException. */
	template<typename TheoryType>
	const TheoryType* loadOptionalTheoryPointer(const XMLCh* attrName) const {
		const Theory* t1 = loadTheoryPointerHelper(attrName, true);
		if (t1 == 0) return 0;
		const TheoryType *t2 = dynamic_cast<const TheoryType*>(t1);
		if (t2 == 0) throw reportBadTheoryPointerType<TheoryType>(attrName);
		return t2;
	}

	template<typename TheoryType>
	const std::vector<const TheoryType*> loadTheoryPointerList(const XMLCh* attrName) const {
		std::vector<const TheoryType*> result;
		// Get a list of id strings.
		std::vector<xstring> idList;
		loadTheoryPointerListHelper(attrName, idList);
		// Look up each id in the id map.
		for (size_t i=0; i<idList.size(); ++i) {
			const TheoryType* item = dynamic_cast<const TheoryType*>(getTheoryFromIdMap(idList[i].c_str()));
			if (item == 0) throw reportBadTheoryPointerType<TheoryType>(attrName);
			result.push_back(item);
		}
		return result;
	}

	/** Return the theory pointed to by the specified attribute as a non-const
	  * pointer.  (This is just a const_cast around loadTheoryPointer.) */
	template<typename TheoryType>
	TheoryType* loadNonConstTheoryPointer(const XMLCh* attrName) const {
		return const_cast<TheoryType*>(loadTheoryPointer<TheoryType>(attrName));
	}

	/** Return the theory pointed to by the specified attribute as a non-const
	  * pointer.  (This is just a const_cast around loadOptionalTheoryPointer.) */
	template<typename TheoryType>
	TheoryType* loadOptionalNonConstTheoryPointer(const XMLCh* attrName) const {
		return const_cast<TheoryType*>(loadOptionalTheoryPointer<TheoryType>(attrName));
	}

	template<typename TheoryType>
	const std::vector<TheoryType*> loadNonConstTheoryPointerList(const XMLCh* attrName) const {
		std::vector<TheoryType*> result;
		// Get a list of id strings.
		std::vector<xstring> idList;
		loadTheoryPointerListHelper(attrName, idList);
		// Look up each id in the id map.
		for (size_t i=0; i<idList.size(); ++i) {
			const TheoryType* item = dynamic_cast<const TheoryType*>(getTheoryFromIdMap(idList[i].c_str()));
			if (item == 0) throw reportBadTheoryPointerType<TheoryType>(attrName);
			result.push_back(const_cast<TheoryType*>(item));
		}
		return result;
	}

	/** Read information about the offsets of a substring from the attributes
	  * of this element, and use that information to fill in the given pair
	  * of OffsetGroups.  If the original text is available, it will be used
	  * to fill in any missing offset information (by converting between 
	  * offset types).  If no offset information is found, raise an 
	  * UnexpectedInputException. */
	void loadOffsets(OffsetGroup &start_offsets, OffsetGroup &end_offsets) const;

	/** Check if this element has an "id" attribute, and if so, then register it
	  * in the identifier map as an id for the given theory object.  Return true
	  * if an "id" attribute is found.  The "id" attribute is never required 
	  * during deserialization (but it's not possible to point to an object unless 
	  * it has an id.) */
	bool loadId(Theory* theory);

//======================= Serialization Options =======================

	struct SerializationOptions {
		bool external_original_text;
		bool include_asr_times;
		bool include_byte_offsets;
		bool include_char_offsets;
		bool include_edt_offsets;
		bool include_spans_as_elements;
		bool include_spans_as_comments;
		bool include_mentions_as_comments;
		bool include_mention_transliterations;
		bool include_mention_confidences;
		bool include_name_transliterations;
		bool include_canonical_names;
		Symbol input_type; // sgm or text or auto
		bool skip_dummy_pos_tags;
		bool use_condensed_offsets;
		bool use_hierarchical_ids;
		bool use_implicit_tokens;
		bool use_treebank_tree;
		bool use_verbose_ids;
	private:
		SerializationOptions();
		typedef std::map<SerifXML::xstring, SerifXML::xstring> OptionMap;
		SerializationOptions(const OptionMap &optionMap);
		void setOptionsToDefaultValues();
		void setOptionsFromParamReader();
		friend class XMLSerializedDocTheory;
		void setBooleanOption(const OptionMap &optionMap, bool &option, xstring option_name);
	};

	const SerializationOptions& getOptions() const;

private:
//======================== Member Variables ============================

	// Pointer to our document
	XMLSerializedDocTheory* _xmlSerializedDocTheory;

//======================= Private Constructor ===========================

	// Constructor is private -- only XMLSerializedDocTheory and XMLTheoryElement should be creating
	// new XMLTheoryElement objects.
	XMLTheoryElement(XMLSerializedDocTheory *document, xercesc::DOMElement *element);
	friend class XMLSerializedDocTheory;


//======================== Helper Methods ============================

	void fillInMissingOffsets(OffsetGroup& start, OffsetGroup& end) const;
	int getOriginalStringPositionOfStartOffset(const OffsetGroup &offsets) const;
	int getOriginalStringPositionOfEndOffset(const OffsetGroup &offsets) const;

	// Add attribute(s) describing the given pair of attributes.
	template<typename OffsetType>
	void saveOffsetsHelper(OffsetType start_offset, OffsetType end_offset);

	template<typename OffsetType>
	void loadOffsetsHelper(OffsetType &start_offset, OffsetType &end_offset) const;

	const Theory* loadTheoryPointerHelper(const XMLCh* attrName, bool optional) const;
	void loadTheoryPointerListHelper(const XMLCh* attrName, std::vector<xstring> &idList) const;

	XMLIdMap* getIdMap() const;
	const Theory* getTheoryFromIdMap(const XMLCh* id) const;
};


typedef std::vector<XMLTheoryElement> XMLTheoryElementList;

} // namespace SerifXML

#endif
