// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/UTF8Token.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/UnexpectedInputException.h"
#include <cctype>

using namespace std;


const size_t UTF8Token::buffer_size = UTF8_TOKEN_BUFFER_SIZE;

// If you change this function for any reason, please also change Sexp::getNextTokenIncludingComments(), as it does a very similar thing.
std::wistream& operator>>(std::wistream& stream, UTF8Token& token)
        throw(UnexpectedInputException)
{
    // we are reading a new token so Symbol has to be recreated now.
	token.symbol_is_valid = false;

	wchar_t wch;
	wch = stream.get();
	if (stream.eof()) {
		wchar_t* p = token.buffer;
		*p = L'\0';
		return stream;
	}
	if (wch == 0x00) 
		throw UnexpectedInputException("UTF8Token::operator>>",
			"Unexpected NULL (0x00) character in stream");
	while (iswspace(wch)) {
		wch = stream.get();
		if (stream.eof()) {
			wchar_t* p = token.buffer;
			*p = L'\0';
			return stream;    
		}
		if (wch == 0x00) 
			throw UnexpectedInputException("UTF8Token::operator>>",
				"Unexpected NULL (0x00) character in stream");
	}
    wchar_t* p = token.buffer;
    *p++ = wch;
    if ((wch == L'(') || (wch == L')')) {
        *p = L'\0';
        return stream;
    }
    size_t i = 1;	
	wch = stream.get();
    while (stream.good() && !iswspace(wch) && wch != L'(' && wch != L')') {
        if (i < (token.buffer_size - 1)) {
            *p++ = wch;
            i++;
        } else {
			if (*p != L'\0') {
				*p = L'\0';
				cerr << "Token too long ("
					<< (int) i << "/" << (int) token.buffer_size << "): "
					<< OutputUtil::convertToChar(token.buffer) << "\n";
			}
			//throw UnexpectedInputException("UTF8Token::operator>>()", "token too long");
        }
		wch = stream.get();
    }
    *p = L'\0';
    if (!stream.eof() && (wch == L'(' || wch == L')')) {
        stream.putback(wch);
    }
    return stream;
}


