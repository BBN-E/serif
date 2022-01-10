// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
#ifndef SGML_TAG_H
#define SGML_TAG_H

#include "Generic/common/Symbol.h"
#include "Generic/preprocessors/Attributes.h"
#include <boost/shared_ptr.hpp>
#include <map>
#include <string>

class LocatedString;
typedef boost::shared_ptr<LocatedString> LSPtr;
typedef std::map<std::wstring,LSPtr> AttributeMap;

class SGMLTag {

public:
		static const int NO_ID = -1;

		SGMLTag(); 
		SGMLTag(const SGMLTag& other);
		SGMLTag(const LocatedString& source, Symbol name, int start, int end, bool close);
		SGMLTag(const LocatedString& source, Symbol name, Symbol type, 
				int start, int end, bool close);
		SGMLTag(const LocatedString& source, Symbol name, 
				int id, int start, int end, bool close);
		SGMLTag(const LocatedString& source, Symbol name, Symbol type, 
				int id, int start, int end, bool close);

		static SGMLTag notFound(const LocatedString& source) {
			SGMLTag tag(source);
			return tag;
		}

		Symbol toSymbol();

		bool notFound();
		bool isOpenTag() { return _open; }
		bool isCloseTag() { return _close; }
		int getStart() { return _start; }
		int getEnd() { return _end; }
		Symbol getName() { return _name; }

		Symbol getType() { return _attributes.type; }
		// some documents has their id inside a tag
		Symbol getDocumentID() { return _attributes.docID; }
		void setDocumentID(Symbol docID) { _attributes.setDocumentID(docID); }

		int getCorefID() { return _attributes.coref_id; }
		const Attributes* getAttributes() { return &_attributes; }

		Symbol getAttributeValue(const wchar_t *attr) const;
		Symbol getAttributeValue(const std::wstring& attr) const;

		const AttributeMap& getTagAttributes() const;

		static SGMLTag findNextSGMLTag(const LocatedString& input, int start);
		static SGMLTag findOpenTag(const LocatedString& input, Symbol name, int start);
		static SGMLTag findCloseTag(const LocatedString& input, Symbol name, int start);
		static bool mustBeCloseTag(const LocatedString& input, Symbol name, int start);
		static void removeSGMLTags(LocatedString& source);
		/// tests whether the tags are nested properly,
		//     rejects docs with stray "<" and/or ">" chars
		// returns -1 for failures, else max stack depth 
		static int isParsableSGML(const LocatedString& document);
		// only the tags in relevantTags matter
		static int isParsableSGML(const LocatedString& document, Symbol relTags[], int n_relTags);

		int findNextSGMLWhiteSpace(int pos);
		bool isSGMLTagWhiteSpace(wchar_t c) const;
		

private:
		SGMLTag(const LocatedString& source);

		static LocatedString* getTagType(const LocatedString& source, int start, int& end, int max);
		static LocatedString* getTagCorefID(const LocatedString& source, int start, int max);
		static LocatedString* getTagDocID(const LocatedString& source, int start, int& end, int max);

		const LocatedString *_source;
		Symbol _name;
		Attributes _attributes;
		int _start;
		int _end;
		bool _close;
		bool _open;  // some tags are both open and close
		AttributeMap _tagAttributes;
		void computeTagAttributes();
		int locateAndSetAttributeValue(std::wstring& attributeName, int eqLoc);
		bool foundWithinBounds(int pos) const ;

}; 

#endif
