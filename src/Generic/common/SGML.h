// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef SGML_H
#define SGML_H

#include <string>
#include <vector>

#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/LocatedString.h"

class SGML {
public:
	
	// Given a null-terminated input string, decodes min( len(decode(input)) + 1, output_buffer_size )
	// null-terminated wchar_t's into output. Returns the number of chars written into output, not counting trailing \0
	static int decode(const wchar_t * input, wchar_t * output, size_t output_buffer_size) {
		
		size_t i = 0, j = 0;
		for (i = 0; input[i] && j+1 < output_buffer_size; ){
			if (wcsncmp(&(input[i]), L"&amp;", 5) == 0) {
				output[j] = L'&';
				i += wcslen(L"&amp;");
				j += 1;
				continue;
			}
			if (wcsncmp(&(input[i]), L"&quot;", 6) == 0) {
				output[j] = L'\"';
				i += wcslen(L"&quot;");
				j += 1;
				continue;
			}
			if (wcsncmp(&(input[i]), L"&apos;", 6) == 0) {
				output[j] = L'\'';
				i += wcslen(L"&apos;");
				j += 1;
				continue;
			}
			if (wcsncmp(&(input[i]), L"&lt;", 4) == 0) {
				output[j] = L'<';
				i += wcslen(L"&lt;");
				j += 1;
				continue;
			}
			if (wcsncmp(&(input[i]), L"&gt;", 4) == 0) {
				output[j] = L'>';
				i += wcslen(L"&gt;");
				j += 1;
				continue;
			}
			if (wcsncmp(&(input[i]), L"&#", 2) == 0) {
				
				size_t k = i + 1;
				do{
					if( input[++k] == L';' ) break;
				} while( input[k+1] && input[k] >= L'0' && input[k] <= L'9' );
				
				// try and do the conversion
				if( input[k] == L';' && (i + 4) <= k ){
					int value = _wtoi( std::wstring( input + i + 2, input + k ).c_str() );
					if( value != 0 ){
						output[j] = (wchar_t) value;
						i = k + 1;
						j += 1;
						continue;
					}
				}
				
				// abort! we didn't have at least 2 digits with a closing semicolon
				output[j] = input[i];
				i += 1; j+= 1;
				continue;
			}
			output[j] = input[i];
			i += 1;
			j += 1;
		}
		output[j] = L'\0';
		
		return (int) j;
	}
	
	
	// Given a null-terminated input string, encodes min( len(encode(input)) + 1, output_buffer_size )
	// null-terminated wchar_t's for XML into output. Returns the number of chars written into output.
	// Since this method expands the size of the input, the output buffer should be larger
	static int encode(const wchar_t * input, wchar_t * output, size_t output_buffer_size) {
		
		if (input == output)
			throw UnexpectedInputException("SGML::encode", "Input and output arguments cannot be the same buffer");
		
		size_t i = 0, j = 0;
		// ensure there's enough space for the largest encoding (6 chars + null char)
		for (i = 0; input[i] && j+7 < output_buffer_size; i++) {
			switch (input[i]) {
				case L'&':
				wcsncpy((output + j), L"&amp;", 5);
				j += wcslen(L"&amp;");
				break;
				case L'"':
				wcsncpy((output + j), L"&quot;", 6);
				j += wcslen(L"&quot;");
				break;
				case L'\'':
				wcsncpy((output + j), L"&apos;", 6);
				j += wcslen(L"&apos;");
				break;
				case L'<':
				wcsncpy((output + j), L"&lt;", 4);
				j += wcslen(L"&lt;");
				break;
				case L'>':
				wcsncpy((output + j), L"&gt;", 4);
				j += wcslen(L"&gt;");
				break;
				case L'\n':
				wcsncpy((output + j), L"&#10;", 5);
				j += wcslen(L"&#10;");
				break;
				case L'\r':
				wcsncpy((output + j), L"&#13;", 5);
				j += wcslen(L"&#13;");
				break;
				default:
				output[j] = input[i];
				j++;
				break;
			}
		}
		output[j] = L'\0';
		
		return (int) j;
	}
	
	static std::wstring encode(const std::wstring & input){
		
		// inefficient, but safe
		std::vector<wchar_t> out_buf( input.size() * 6 + 1 );
		encode( input.c_str(), &(out_buf[0]), out_buf.size() );
		return std::wstring( &(out_buf[0]) );
	}
	
	static std::wstring decode(const std::wstring & input){
		
		// inefficient, but safe
		std::vector<wchar_t> out_buf( input.size() + 1 );
		decode( input.c_str(), &(out_buf[0]), out_buf.size() );
		return std::wstring( &(out_buf[0]) );
	}
	
	// Decodes the given lstring in place (so make a copy before-hand if you need it)
	static void decode(LocatedString * lstr) {
		
		lstr->replace(L"&quot;", L"\"");
		lstr->replace(L"&apos;", L"'");
		lstr->replace(L"&lt;",   L"<");
		lstr->replace(L"&gt;",   L">");
		
		// replace numeric SGML entities
		for (int i = 0; i != -1; i = lstr->indexOf(L"&#",i)) {
			int k = lstr->indexOf(L";",i);
					
			for (int p = i + 2; k != -1 && p < k; p++) {
				if( !( lstr->toString()[p] >= L'0' && lstr->toString()[p] <= L'9' ) )
					k = -1;
			}

			// didn't find a valid char encoding, abort
			if(i + 2 >= k || k == -1) { i += 1; continue; }
			
			// evaluate the wchar_t value, and replace the escape
			wchar_t replacement[] = { (wchar_t) _wtoi( std::wstring( lstr->toString() + i + 2, lstr->toString() + k ).c_str() ), 0 };
			lstr->replace( i, 1 + k - i, replacement );
		}

		// must be last, so we don't fully decode doubly-decoded escapes
		lstr->replace(L"&amp;",  L"&");

		return;
	}
	
	// when encode is needed, write it here!
	
private:
	// non-constructible class
	SGML(){}
};


#endif // #ifndef SGML_H
