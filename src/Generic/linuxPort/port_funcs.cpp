// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/linuxPort/serif_port.h"
#include <wchar.h>
#include <stddef.h>
#include <cstdlib>

#if !defined(_WIN32)

//# include "linuxPort/utf8_codecvt_facet.h"

wchar_t * wstrupr(wchar_t *str) {
	wchar_t *p = str;
	for (; *p != '\0'; p++) {
	*p = toupper(*p);
	}
	return str;
} 

char * strupr(char *str) {
	char *p = str;
	for (; *p != '\0'; p++) {
	*p = toupper(*p);
	}
	return str;
} 

void strreverse(char* begin, char* end) {
	char aux;
	while(end>begin)
		aux=*end, *end--=*begin, *begin++=aux;
}
	
char* itoa(int value, char* str, int base) {
	
	static char num[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	char* wstr=str;
	int sign;
	div_t res;
	
	// Validate base
	if (base<2 || base>35){ *wstr='\0'; return wstr; }
	
	// Take care of sign
	if ((sign=value) < 0) value = -value;
	
	// Conversion. Number is reversed.
	do {
		res = div(value,base);
		*wstr++ = num[res.rem];
	} while ((value = res.quot));
	if(sign<0) *wstr++='-';
	*wstr='\0';
	
	// Reverse string
	strreverse(str,wstr-1);
	
	return str;
}

#endif
