// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/reader/DocumentReader.h"
#include "Generic/common/UnexpectedInputException.h"

Document* OldArabicDocumentReader::readDocument(UTF8InputStream &uis){
	
	int cnt = 0;
	bool inText = false;
	//wchar_t* docName = _new wchar_t*[40];
	//wchar_t* currTag = _new wchar_t*[MAX_TAG_LENGTH];
	wchar_t ch;
	while ((!uis.eof()) && (cnt < MAX_DOC_LENGTH)) {
			ch = uis.get();
			if(ch == L'<'){
				uis.putBack(ch);
				readTag(_curr_tag, uis);
				if(matchTag(_curr_tag, L"<DOCNO>"))
					readToEnd(_doc_name, 100, _curr_tag, uis);
				if(matchTag(_curr_tag, L"<TEXT>"))
					readToEnd(_text, MAX_DOC_LENGTH, _curr_tag, uis);
			}
	}
	LocatedString** docText = _new LocatedString*[1];
	docText[0] = _new LocatedString(_text, 0);
	Document* d = _new Document(Symbol(_doc_name),1,docText);
	return d;
}

bool OldArabicDocumentReader::matchTag(const wchar_t* tag1, const wchar_t* tag2){
	int pos =0;
		while((tag1[pos] != L'>') && (tag2[pos] != L'>') && (tag1[pos] == tag2[pos]))
			pos++;
		return (tag1[pos] == tag2[pos]);
}
void OldArabicDocumentReader::readTag(wchar_t* tag, UTF8InputStream &uis){
	int pos = 0;
	while (!uis.eof()&& (pos<MAX_TAG_LENGTH)){
		tag[pos]=uis.get();
		if(tag[pos] == L'>')
			break;
		else 
			pos++;
	}

	if(uis.eof())
		throw UnexpectedInputException("OldArabicDocumentReader::readTag()", "Document ended before \'>\' ");
	if(pos == MAX_TAG_LENGTH)
		throw UnexpectedInputException("OldArabicDocumentReader::readTag()", "Tag longer than MAX_TAG_LENGTH");
	tag[pos+1]=L'\0';
}

void OldArabicDocumentReader::readToEnd(wchar_t* text, int text_size, wchar_t* start_tag, UTF8InputStream &uis){
	//need to find closing tag <...... XXXXX XXX> -> </....... 
	
	int pos =1;
	_end_tag[0]= L'<';
	_end_tag[1]= L'/';
	while((start_tag[pos] != L' ')&&(start_tag[pos] != L'>')){
		_end_tag[pos+1]=start_tag[pos];
		pos++;
	}
	_end_tag[pos+1]=L'>';
	pos = 0;
	wchar_t ch;
	while ((!uis.eof()) && (pos < text_size)) {
			ch = uis.get();
			if(ch == L'<'){
				uis.putBack(ch);
				readTag(_curr_tag, uis);
				if(matchTag(_curr_tag, _end_tag))
					break;
				else{
					int i =0;
					while(_curr_tag[i]!=L'\0'){
						text[pos] = _curr_tag[i];
						pos++;
						i++;
					}
				}
			}
			else{
				text[pos] = ch;
				pos++;
			}
	}
	if(uis.eof()){
		//throw UnexpectedInputException("OldArabicDocumentReader::readToEnd()", "Document ended before "+_end_tag);
		throw UnexpectedInputException("DocuementReader::readToEnd()", "Document ended before ending tag");
	}
	if(pos == text_size){
		//throw UnexpectedInputException("OldArabicDocumentReader::readTag()", start_tag+ " to "+ end_tag +" longer than MAX_TAG_LENGTH");
		throw UnexpectedInputException("DocuementReader::readToEnd()", "Tag Span longer than "+MAX_TAG_LENGTH);
	}
	
	text[pos+1]=L'\0';
}
	








		





