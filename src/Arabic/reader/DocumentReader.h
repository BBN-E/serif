// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_DOCUMENT_READER_H
#define AR_DOCUMENT_READER_H

#include "Generic/theories/Document.h"
#include "Generic/common/UTF8InputStream.h"


class OldArabicDocumentReader {
public:
	OldArabicDocumentReader() {
		_end_tag = _new wchar_t[MAX_TAG_LENGTH];
		_curr_tag = _new wchar_t[MAX_TAG_LENGTH];
		_doc_name = _new wchar_t[100];
		_text= _new wchar_t[MAX_DOC_LENGTH+1];
	}
    
	/**
	 * This does the work. It reads one document from 
	 * the input stream and returns a Document object.
	 * The client is responsible for deleting
	 * the Document.
     **/
	Document* readDocument(UTF8InputStream &strm);

private:
	static const int MAX_DOC_LENGTH = 20000; //TODO: remove this
	static const int MAX_TAG_LENGTH = 100;
	wchar_t* _end_tag;
	wchar_t* _doc_name;
	wchar_t* _curr_tag;
	wchar_t* _text;
	bool matchTag(const wchar_t* tag1, const wchar_t* tag2);
	void readTag(wchar_t* tag, UTF8InputStream &uis);
	void readToEnd(wchar_t* text, int text_size, wchar_t* start_tag, UTF8InputStream &uis);

};


#endif
