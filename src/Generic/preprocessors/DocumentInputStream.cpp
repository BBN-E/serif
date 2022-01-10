// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <iostream>
#include <string>
#include "Generic/common/LocatedString.h"
#include "Generic/common/Symbol.h"
#include "Generic/preprocessors/DocumentInputStream.h"
#include "Generic/linuxPort/serif_port.h"

using namespace std;

namespace DataPreprocessor {

LocatedString* DocumentInputStream::getDocument()
{
	wstring buffer, tagName;

	// [XX] This is a hack -- the _offset is really a character offset, so using it for
	// the byte offset and edt offset values gives misleading values.  But it preserves
	// the old (broken) behavior, which is what I'm trying to do for now.
	OffsetGroup start_offset((ByteOffset(_offset)), (CharOffset(_offset)), (EDTOffset(_offset)), (ASRTime()));

	if (findOpenTag(buffer, tagName)) {
		findCloseTag(buffer, tagName);
		return _new LocatedString(buffer.c_str(), start_offset);
	}
	else {
		return NULL;
	}
}

wchar_t DocumentInputStream::get()
{
	_offset++;
	return UTF8InputStream::get();
}

bool DocumentInputStream::findOpenTag(wstring& buffer, wstring& tagName)
{
	// NOTE: The behavior of UTF8InputStream::get() changed in fall 2006
	//       It used to return 0 when it failed; now it just is passed through to the regular
	//         ifstream behavior, and so it just sets the fail state in the stream (EMB 12/12/06)
	
	wchar_t c;

	tagName.erase(0, tagName.length());

	// Skip everything until we get to a tag.
	do {
		c = get();
		if (this->fail()) return false;
	} while (c != L'<');

	wstring fullTagBuffer;

	// Save the open angle bracket.
	fullTagBuffer.insert(fullTagBuffer.end(), c);

	while (true) {
		c = get();
		if (this->fail()) return false;
		if (iswspace(c))
			fullTagBuffer.insert(fullTagBuffer.end(), c);
		else break;
	} 

	// Save the tag name.
	do {
		fullTagBuffer.insert(fullTagBuffer.end(), c);
		tagName.insert(tagName.end(), c);
		c = get();
		if (this->fail()) return false;
	} while (iswalnum(c));
	fullTagBuffer.insert(fullTagBuffer.end(), c);

	// Get the rest of the tag.
	while (c != L'>') {
		c = get();
		if (this->fail()) return false;
		fullTagBuffer.insert(fullTagBuffer.end(), c);
	}

	// If this tag was an open tag, we're done.
	if ((_wcsicmp(tagName.c_str(), L"DOC") == 0) ||
		(_wcsicmp(tagName.c_str(), L"DOCUMENT") == 0))
	{
		buffer.append(fullTagBuffer);
		return true;
	}

	// Otherwise, keep trying.
	return findOpenTag(buffer, tagName);
}

void DocumentInputStream::findCloseTag(wstring& buffer, const wstring& tagName)
{
	wchar_t c;

	// Skip everything until we get to a tag.
	do {
		c = get();
		if (this->fail()) return;
		buffer.insert(buffer.end(), c);
	} while (c != L'<');

	wstring tagNameBuffer;

	// Get to the slash.
	do {
		c = get();
		if (this->fail()) return;
		buffer.insert(buffer.end(), c);
	} while (iswspace(c));

	// If this isn't a close tag, skip it and keep searching.
	if (c != L'/') {
		do {
			c = get();
			if (this->fail()) return;
			buffer.insert(buffer.end(), c);
		} while (c != L'>');
		findCloseTag(buffer, tagName);
		return;
	}

	// Get to the tag name.
	while (true) {
		c = get();
		if (this->fail()) return;
		if (iswspace(c)) 
			buffer.insert(buffer.end(), c);
		else break;
	}

	// Save the tag name.
	do {
		buffer.insert(buffer.end(), c);
		tagNameBuffer.insert(tagNameBuffer.end(), c);
		c = get();
		if (this->fail()) return;
	} while (iswalnum(c));
	buffer.insert(buffer.end(), c);

	// Get the rest of the tag.
	while (c != L'>') {
		c = get();
		if (this->fail()) return;
		buffer.insert(buffer.end(), c);
	}

	// If this tag was not a document close tag, keep trying.
	if (_wcsicmp(tagNameBuffer.c_str(), tagName.c_str()) != 0) {
		findCloseTag(buffer, tagName);
	}
}

} // namespace DataPreprocessor
