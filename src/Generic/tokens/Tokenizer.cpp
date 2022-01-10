// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.
#include "Generic/common/leak_detection.h"
#include "Generic/tokens/Tokenizer.h"
#include "Generic/tokens/DefaultTokenizer.h"
#include "Generic/theories/Sentence.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/tokens/TokenizerFactory.h"


#include <wchar.h>
#include <boost/scoped_ptr.hpp>

int Tokenizer::removeHarmfulUnicodes(LocatedString *string){
	int tweaks = 0;
	int len = string->length();
	for (int index = len-1; index >=0; index--) {
		wchar_t wch = string->charAt(index);
		if ((wch < 0x0009) ||   // 0x09 is tab, 
			// 0x0a is linefeed
			// 0x0b is vertical tab, 0x0c is form feed
			(wch == 0x000b)  || (wch == 0x000c) ||  
			// 0x0d is carriage return, others are C0 ctls
			((wch > 0x000d)  && (wch < 0x0020)) || // 0x20 is blank
			// 0x7f is delete, 0x85 is unicode newline
			((wch >= 0x007f) && (wch < 0x0085)) || 
			// 0x80 -> 0x9f are Unicode controls C1
			((wch >= 0x0086) && (wch <= 0x009f)) ||
			(wch == 0x00ad) || //0xad is "sly/shy hyphen"
			(wch == 0x034f) || //0x34f is combining grapheme joiner
			(wch == 0x0640) || //Arabic "tatweel" token elongation char
		// change these to spaces for token separation
			//((wch >= 0x200c) && (wch <= 0x200f)) || // typography (include ltr and rtl)
			//((wch >= 0x202a) && (wch <= 0x202e)) || // embedding direction to print
			((wch >= 0x206a) && (wch <= 0x206f)) || // Arabic typography
			((wch >= 0x2ff0) && (wch <= 0x2ffb)) || // ideographic char desc
			(wch == 0x303e) ||  // ideographic variation
			((wch >= 0xfdd0) && (wch <= 0xfdef)) || // non-codes
			((wch >= 0xfe00) && (wch <= 0xfe0f)) || // variation selectors
			(wch == 0xfeff) ||  // ByteOrderMark or "endian indicator" added to front 
			((wch >= 0xfff9) && (wch <= 0xfffc)) || // non-codes
			(wch == 0xffff) ){  // "eof" tag accidentally appended 

			char msg[200];
			sprintf_s(msg, "removeHarmfulUnicodes removing char U+%x at index %d\n", wch, index);
			SessionLogger::dbg("unicode") << msg;
			string->remove(index,index+1);
			tweaks++;
		}else if (wch == 0xfffe){
			SessionLogger::dbg("unicode")
				<< "removeHarmfulUnicodes removing U+fffe which may mean file was read with wrong endian\n"
				<< "    from " << string->toString() << "\n";
			string->remove(index,index+1);
			tweaks++;
		}
	}
	return tweaks;
}
int Tokenizer::replaceNonstandardUnicodes(LocatedString *string){
	int tweaks = 0;
	int len = string->length();
	char msg[200];
	for (int index = 0; index < len; index++) {
		wchar_t wch = string->charAt(index);
		msg[0] = 0;
		if (// "horizontal tab"   -- keep this one unchanged for segmenting -- 
			// (wch == 0x0009) ||  
			(wch == 0x00a0) ||  // "no-break space"
			((wch >= 0x2000) && (wch <= 0x200f)) || // 2000-200b are fancy white spaces
		  // 0x200b is "no break space" used for some languages
		  // 200c-200f are typography (include ltr and rtl)
		    ((wch >= 0x202a) && (wch <= 0x202e)) || // embedding direction to print
		// these typography codes fall between tokens so we replace with whitespace
			(wch == 0x202f) ||  // "narrow no-break space"
		  // 0x205f is mathematical space
		  // 0x2060 is "word joiner" 0-width no-break space
			((wch >= 0x205f) && (wch <= 0x2063))){ // 0x2061-0x2063 math separators
				// replace with vanilla space
				sprintf_s(msg, "U+%x with blank at index %d\n",
				  wch, index);
				string->replace(index,1,L" ");
				tweaks++;
		}else if ((wch == 0x0085) ||  // new Unicode "next line"
				  (wch == 0x2028))  { // "line separator"	
			// replace with old-fashioned line feed (Unix new line)
				sprintf_s(msg, "U+%x with line feed at index %d\n",
						wch, index);
				string->replace(index,1,L"\x0a");
				tweaks++;
		}else if (wch == 0x2029){ // "paragraph separator"
				sprintf_s(msg, "U+%x with double line feed at index %d\n",
						wch, index);
				string->replace(index,1,L"\x0a\x0a"); // two new lines
				len++;
				tweaks++;
		}else if (((wch >=0x2010) && (wch <= 0x2015)) || //hyphens and dashes
				   (wch == 0x2043)) {  // bullet hyphen
				sprintf_s(msg, "U+%x with dash at index %d\n",
						wch, index);
				string->replace(index,1,L"-");
				tweaks++;
		}else if (((wch >= 0x2018) && (wch <=0x201b)) || // apostrophes
				  (wch == 0x2032) || // sharp apos
				  (wch == 0x2035)){ // back apos
				sprintf_s(msg, "U+%x with tick at index %d\n",
						wch, index);
				string->replace(index,1,L"'");
				tweaks++;
		}else if ((wch >= 0x201c) && (wch <=0x201f)) {// double quotes
				sprintf_s(msg, "U+%x with double quote at index %d\n",
						wch, index);
				string->replace(index,1,L"\"");
				tweaks++;
		}else if (wch == 0x2026){ // ellipses
				sprintf_s(msg, "U+%x with spaces and three dots at index %d\n",
						wch, index);
				string->replace(index,1,L" ... ");
				len = len + 2;
				tweaks++;
		}else if ((wch == 0x2033) || // double sharp apos
				  (wch == 0x2036)){ // double back apos
				sprintf_s(msg, "U+%x with double tick at index %d\n",
						wch, index);
				string->replace(index,1,L"''");
				len++;
				tweaks++;
		}else if ((wch == 0x2034) || // triple sharp apos
				  (wch == 0x2037)){ // triple back apos
				sprintf_s(msg, "U+%x with triple tick at index %d\n",
						wch, index);
				string->replace(index,1,L"'''");
				len = len + 2;
				tweaks++;
		}else if (wch == 0x2044) { // fraction slash
				sprintf_s(msg, "U+%x with slash at index %d\n",
						wch, index);
				string->replace(index,1,L"/");
				tweaks++;
		}else if ((wch == 0xfeed) ||  // isolated variant arabic "waw"
			      (wch == 0xfeee)) {  // terminal variant arabic "waw"
				sprintf_s(msg, "U+%x with Arabic waw at index %d\n",
						wch, index);
				string->replace(index,1,L"\x0648");
				tweaks++;
		}
		if (msg[0] != 0){
			//std::cerr<<msg;
			SessionLogger::dbg("unicode") <<"replaceNonstandardUnicodes replacing char " <<msg;
			//std::cerr<<"debug tokenizer: replaceNonstandardUnicodes replacing char " <<msg;
		}
	}
	return tweaks;
}

void Tokenizer::standardizeNonASCII(LocatedString *string) {

	string->replace(L"\xc0", L"A");
	string->replace(L"\xc1", L"A");
	string->replace(L"\xc2", L"A");
	string->replace(L"\xc3", L"A");
	string->replace(L"\xc4", L"A");
	string->replace(L"\xc5", L"A");
	string->replace(L"\xc6", L"AE");

	// this one often means currency? don't change it...
	//string->replace(L"\xc7", L"C");

	string->replace(L"\xc8", L"E");
	string->replace(L"\xc9", L"E");
	string->replace(L"\xca", L"E");
	string->replace(L"\xcb", L"E");
	string->replace(L"\xcc", L"I");
	string->replace(L"\xcd", L"I");
	string->replace(L"\xce", L"I");
	string->replace(L"\xcf", L"I");
	string->replace(L"\xd1", L"N");
	string->replace(L"\xd2", L"O");
	string->replace(L"\xd3", L"O");
	string->replace(L"\xd4", L"O");
	string->replace(L"\xd5", L"O");
	string->replace(L"\xd6", L"O");
	string->replace(L"\xd8", L"O");
	string->replace(L"\xd9", L"U");
	string->replace(L"\xda", L"U");
	string->replace(L"\xdb", L"U");
	string->replace(L"\xdc", L"U");
	string->replace(L"\xdd", L"Y");
	string->replace(L"\xdf", L"ss");
	string->replace(L"\xe1", L"a");
	string->replace(L"\xe2", L"a");
	string->replace(L"\xe3", L"a");
	string->replace(L"\xe4", L"a");
	string->replace(L"\xe5", L"a");
	string->replace(L"\xe6", L"ae");
	string->replace(L"\xe7", L"c");
	string->replace(L"\xe8", L"e");
	string->replace(L"\xe9", L"e");
	string->replace(L"\xea", L"e");
	string->replace(L"\xeb", L"e");
	string->replace(L"\xec", L"i");
	string->replace(L"\xed", L"i");
	string->replace(L"\xee", L"i");
	string->replace(L"\xef", L"i");
	string->replace(L"\xf1", L"n");
	string->replace(L"\xf2", L"o");
	string->replace(L"\xf3", L"o");
	string->replace(L"\xf4", L"o");
	string->replace(L"\xf5", L"o");
	string->replace(L"\xf6", L"o");
	string->replace(L"\xf8", L"o");
	string->replace(L"\xf9", L"u");
	string->replace(L"\xfa", L"u");
	string->replace(L"\xfb", L"u");
	string->replace(L"\xfc", L"u");
	string->replace(L"\xfd", L"y");
	string->replace(L"\xff", L"y");
}

/**
 * Generalizes the functionality of several old match methods.
 *
 * @param string The string on which matching is performed.
 * @param index The inclusive index at which the match will start.
 * @param character The wide character to match against.
 *
 * @return The number of matching characters in a row occurring
 * in string starting at index; if the index is out of bounds or
 * the character doesn't occur at index, the count will be 0.
 *
 * @author nward@bbn.com
 * @date 2013.11.14
 **/
int Tokenizer::matchCharacter(const LocatedString *string, int index, wchar_t character) {
	int count = 0;
	for (int i = index; i < string->length(); i++) {
		if (string->charAt(i) == character)
			count++;
		else
			break;
	}
	return count;
}

/**
 * N-ary replacement for matchTwoOpenQuotes and matchThreeOpenQuotes.
 **/
int Tokenizer::matchOpenQuotes(const LocatedString *string, int index) {
	return matchCharacter(string, index, L'`');
}

/**
 * N-ary replacement for matchTwoCloseQuotes and matchThreeCloseQuotes.
 **/
int Tokenizer::matchCloseQuotes(const LocatedString *string, int index) {
	return matchCharacter(string, index, L'\'');
}

bool Tokenizer::matchURL(const LocatedString *string, int index) {	
	// walk backwards to the last whitespace
	// then, if we can find "http" or "www." between there and where we are now,
	// let's assume we are in the middle of a URL
	int prev_whitespace = 0;
	int i = 0;
	for (i = index; i >= 0; i--) {
		if (iswspace(string->charAt(i))) {
			prev_whitespace = i;
			break;
		}
	}

	//Make sure that a URL complies with RFC 2396
	//  RFC 2396 by Tim "Worldwide" Berners-Lee et al.
	//  http://tools.ietf.org/html/rfc2396
	//
	//From RFC 2396, Section 2.4.3 - Excluded US-ASCII Characters
	//  control = <0x00-0x1F, 0x7F>
	//  space = <0x20>
	//  delims = "<" | ">" | "#" | "%" | <">
	//  unwise = "{" | "}" | "|" | "\" | "^" | "[" | "]" | "`"
	//
	//We assume there aren't control characters in the input. We're already
	//looking for non-whitespace. We allow # and % because they might be an
	//HTML anchor or an escaped hexadecimal character.
	//
	// { } | \ ^ [ ] ` < > "
	//
#define URL_ILLEGAL_CHARS L"{}|\\^[]`<>\""

	//Additionally, we have characters that often indicate the end of a URL
	//in our inputs, even though the characters are technically allowed to
	//be in a URL.
	//
	//We see parenthetical references to URLs, and we want the terminal
	//parenthesis to be its own token.
	//
	// )
	//
#define URL_UNDESIRED_CHARS L"()"
	// added open paren to disallow tokens with embedded open parens

	// find start of URL, if any
	int web = string->indexOf(L"http", prev_whitespace);
	if (web < 0 || web > index) {
		web = string->indexOf(L"HTTP", prev_whitespace);
		if (web < 0 || web > index) {
			web = string->indexOf(L"www.", prev_whitespace);
			if (web < 0 || web > index) {
				web = string->indexOf(L"WWW.", prev_whitespace);
				if (web < 0 || web > index) {
					web = string->indexOf(L"ftp:", prev_whitespace);
					if (web < 0 || web > index) {
						web = string->indexOf(L"FTP:", prev_whitespace);
						if (web < 0 || web > index) {
						return false;
						}
					}
				}
			}
		}
	}

	// check if the URL contains illegal characters
	//   if it does, we probably have URL + non-whitespace junk,
	//   so we are past the end of the URL
	for (i = prev_whitespace; i <= index; i++) {
		// check for technically illegal characters
		if (wcschr(URL_ILLEGAL_CHARS, string->charAt(i)) != NULL)
			return false;
		// check for characters we don't want in a URL
		if (wcschr(URL_UNDESIRED_CHARS, string->charAt(i)) != NULL)
			return false;
	}

	// valid URL
	return true;
}

bool Tokenizer::isSplittable(const LocatedString *string, int index) {
	// Disallow splits in the middle of the same "source" character
	if ((index > 0 && index < string->length()) && string->start<EDTOffset>(index) == string->start<EDTOffset>(index-1))
		return false;
	return true;
}

void Tokenizer::replaceHyphenGroups(LocatedString *input) {
	for (int i = 0; i < (input->length() - 3); i++) {
		if ((input->charAt(i) == L'-') &&
			(input->charAt(i + 1) == L'-') &&
			(input->charAt(i + 2) == L'-'))
		{
			int j = i + 3;
			while ((j < input->length()) && (input->charAt(j) == L'-')) {
				j++;
			}
			input->replace(i, j - i, L"--");
		}
	}
}

//Replace any punctuation mark that appears more than 3 times in a row
//with just 3 of that puncutation
//Is helpful with badly formed internet text, where people put excessive '!', '?' or '.'
void Tokenizer::replaceExcessivePunctuation(LocatedString *input) {
	int start = 0;
	int end;
	for(int start = 0; start < input->length(); start++) {
		if(iswpunct(input->charAt(start))) {
			wchar_t punct = input->charAt(start);
			for(end = start + 1; end < input->length() && input->charAt(end) == punct; end++) {}
			int punctStrLen = end-start;
			//end is currently 1 past the end of the punctuation string
			if(punctStrLen > 3) {
				//replace replaces from pos to pos+len inclusive, so we need to subtract 1 from the length
				input->replace(start, punctStrLen-1, wstring(3, punct).c_str());
			}
		}
	}
	
}

void Tokenizer::readReplacementsFile(const char* file_name, StringStringMap* replacements) {
	// Read the replacements file
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(file_name);

	// Loop over each line, one replacement per line
	const std::wstring tab(L"\t");
	std::wstring line;
	while (std::getline(in, line)) {
		std::wstring::size_type tab_offset = line.find(tab);
		std::wstring old_token, new_token;
		old_token.assign(line, 0, tab_offset);
		new_token.assign(line, tab_offset + 1, line.length() - tab_offset - 2);
		(*replacements)[old_token] = new_token;
	}

	// Done
	in.close();
}

/**
 * @param string a pointer to the input text.
 * @param index the index to start skipping from.
 * @return the first index beyond the end of the closing quotation marks.
 */
int Tokenizer::skipCloseQuotes(const LocatedString *string, int index) {
	int length = string->length();
	while ((index < length) && (string->charAt(index) == L'\''))
		index++;
	return index;
}

/**
 * @param string a pointer to the input text.
 * @param index the index to start skipping from.
 * @return the first index beyond the end of the whitespace.
 */
int Tokenizer::skipWhitespace(const LocatedString *string, int index) {
	int length = string->length();
	while ((index < length) && (iswspace(string->charAt(index))))
		index++;
	return index;
}


boost::shared_ptr<Tokenizer::Factory> &Tokenizer::_factory() {
	static boost::shared_ptr<Tokenizer::Factory> factory(new TokenizerFactory<DefaultTokenizer>());
	return factory;
}

