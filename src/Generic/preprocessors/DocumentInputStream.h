// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOCUMENT_INPUT_STREAM_H
#define DOCUMENT_INPUT_STREAM_H

#include <string>
#include "Generic/common/LocatedString.h"
#include "Generic/common/UTF8InputStream.h"

using namespace std;

namespace DataPreprocessor {

	class DocumentInputStream : public UTF8InputStream {
	public:
		DocumentInputStream(const char* file, wchar_t delim=L'\n')
			: UTF8InputStream(file/*, delim*/), _offset(0) {}
		DocumentInputStream(const wchar_t *file, wchar_t delim=L'\n')
			: UTF8InputStream(file/*, delim*/), _offset(0) {}
		DocumentInputStream()
			: UTF8InputStream(), _offset(0) {}

		LocatedString* getDocument();
		wchar_t get();

	private:
		int _offset;

		bool findOpenTag(wstring& buffer, wstring& tagName);
		void findCloseTag(wstring& buffer, const wstring& tagName);

	};

} // namespace DataPreprocessor

#endif
