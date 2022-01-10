#ifndef CH_DOCUMENT_READER_H
#define CH_DOCUMENT_READER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/theories/Document.h"
#include "Generic/reader/DocumentReader.h"
#include "Generic/common/InputStream.h"

#define MAX_DOC_REGIONS 100

class ChineseDocumentReader: public DocumentReader {
private:
	friend class ChineseDocumentReaderFactory;

public:


	/**
	  * Reads from strm until it reaches the end of 
	  * file and returns a pointer to a document object
	  * containing the located raw text of the file, 
	  * ignoring only text surrounded by '<' and '>'.
	  * The client is responsible for deleting the
	  * document.
	  *
	  * @param stream the input stream to read from.
	  */
	virtual Document* readDocument(InputStream &stream);
	virtual ~ChineseDocumentReader();

protected:

	class XMLTag {
	public:
		XMLTag(const LocatedString *source, Symbol name, int start, int end, bool close) {
			_source = source;
			_name = name;
			_start = start;
			_end = end;
			_close = close;
		}

		XMLTag(const LocatedString *source) {
			_source = source;
			_name = Symbol(L"");
			_start = source->length();
			_end = source->length();
			_close = false;
		}

		XMLTag() {}

		Symbol toSymbol() {
			LocatedString *sub = _source->substring(_start, _end);
			Symbol result = sub->toSymbol();
			delete sub;
			return result;
		}

		bool notFound() {
			return _start >= _source->length();
		}

		bool isOpenTag() {
			return !_close;
		}

		bool isCloseTag() {
			return _close;
		}

		int getStart() {
			return _start;
		}

		int getEnd() {
			return _end;
		}

		Symbol getName() {
			return _name;
		}

	private:
		const LocatedString *_source;
		Symbol _name;
		int _start;
		int _end;
		bool _close;
	};

	static LocatedString *_regions[MAX_DOC_REGIONS];

	XMLTag findNextXMLTag(const LocatedString *input, int start);
	XMLTag findOpenTag(const LocatedString *input, Symbol name, int start);
	XMLTag findCloseTag(const LocatedString *input, Symbol name, int start);

	/// Removes annotations and unrecognized tags and translates SGML entities.
	virtual void cleanRegion(LocatedString *region);
	
	/// Removes path from file name and returns as Symbol
	Symbol cleanFileName(const char* file) const;
private:
	ChineseDocumentReader() {};

};

class ChineseDocumentReaderFactory: public DocumentReader::Factory {
	virtual DocumentReader *build() { return _new ChineseDocumentReader(); }
};


#endif
