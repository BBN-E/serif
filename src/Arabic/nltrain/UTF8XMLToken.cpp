// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/nltrain/UTF8XMLToken.h"
#include "Generic/common/UTF8InputStream.h"
#include <cctype>

using namespace std;


const size_t UTF8XMLToken::buffer_size = UTF8_TOKEN_BUFFER_SIZE;

UTF8InputStream& operator>>(UTF8InputStream& stream, UTF8XMLToken& token)
        throw(UnexpectedInputException)
{
	
	wchar_t wch;
	wch = stream.get();
	if (wch == 0x00) {
		wchar_t* p = token.buffer;
		*p++ = L'E';
		*p++ = L'O';
		*p++ = L'F';
		*p = L'\0';
		return stream;
	}
	while (iswspace(wch)) {
		wch = stream.get();
		if (wch == 0x00) {
			wchar_t* p = token.buffer;
			*p = L'E';
			*p = L'O';
			*p = L'F';
			*p = L'\0';
			return stream;    
		}
	}
    wchar_t* p = token.buffer;
    *p++ = wch;

    if (wch == L'<') {
		stream.putBack(wch);
		UTF8XMLToken::GetWholeTag( stream, token, 1);
		
        return stream;
    }
    size_t i = 1;	
	wch = stream.get();
    while (!iswspace(wch)) {
		if( wch == '<'){
			stream.putBack(wch);
			break;
		}
        if (i < (token.buffer_size - 1)) {
            *p++ = wch;
            i++;
        } else {
            *p = L'\0';
			throw UnexpectedInputException("UTF8Token:>>", "token too long");
        }
		wch = stream.get();
    }
    *p = L'\0';
	return stream;
};

void UTF8XMLToken::GetWholeTag(UTF8InputStream &stream, UTF8XMLToken& token, int size){
	wchar_t wch;
	wchar_t* p = token.buffer;
	wch = stream.get();
	int i = size;
	while(wch != L'>'){
		if(i <token.buffer_size-1){
			*p++ = wch;
			i++;
		}
		else{
			*p = L'\0';
			throw UnexpectedInputException("UTF8XMLToken:>>", "token too long");
		}
		wch = stream.get();
	}
	if(i <token.buffer_size-1){
		*p++ = wch;
		i++;
	}
	else{
		*p = L'\0';
		throw UnexpectedInputException("UTF8XMLToken:>>", "token too long");
	}
	*p = L'\0';
};
//TODO: REPLACE THIS WITH token/SymbolSubstitutionMap
Symbol UTF8XMLToken::SubstSymValue(){
	int j=0;
	for(int i = 0; i<buffer_size; i++){
		if(buffer[i]==L'('){
			_subst_buffer[j++]=L'-';
			_subst_buffer[j++]=L'L';
			_subst_buffer[j++]=L'B';
			_subst_buffer[j++]=L'R';
			_subst_buffer[j++]=L'-';
		}
		else if(buffer[i]==L')'){
			_subst_buffer[j++]=L'-';
			_subst_buffer[j++]=L'R';
			_subst_buffer[j++]=L'B';
			_subst_buffer[j++]=L'R';
			_subst_buffer[j++]=L'-';
		}
		else
			_subst_buffer[j++]=buffer[i];
	}
	return (Symbol)_subst_buffer;
}







	


