// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "morphSelection/KoreanParseSeeder.h"
#include "common/UnrecoverableException.h"
#include "common/UTF8OutputStream.h"
#include "common/SessionLogger.h"
#include "common/WordConstants.h"
#include "theories/Token.h"
#include "Korean/common/UnicodeEucKrEncoder.h"

/**
 * Reset all charts in preparation for processing a new token.
**/
void KoreanParseSeeder::reset() {
	int t = _n_lex_entries;
	if (t > MAX_POSSIBILITIES_PER_WORD) {
		t = MAX_POSSIBILITIES_PER_WORD;
	}
	for (int i = 0; i < t; i++) {
		_wordNumSeg[i] = 0;
		_mergedNumSeg[i] = 0;
	}
	_n_lex_entries = 0;
	_num_grouped = 0;
}

/**
 * Populate _wordChart, _mergedWordChart, and _groupedWordChart with all
 * lexical analyses of t.
**/
void ParseSeeder::processToken(const LocatedString& sentenceString, const Token* t) throw (UnrecoverableException) {
	reset();

	_n_lex_entries = t->getNLexicalEntries();
	if (_n_lex_entries > MAX_POSSIBILITIES_PER_WORD) {
		(*SessionLogger::logger).beginWarning();
		(*SessionLogger::logger) << "Warning: Truncating token analysis possibilities to " 
				  << MAX_POSSIBILITIES_PER_WORD << ". Actual number of possibilities is "
				  << _n_lex_entries << "\n";
		_n_lex_entries = MAX_POSSIBILITIES_PER_WORD;
	}

	populateWordChart(t);
	alignWordChart();
	mergeSegments();
	addOffsets(t);
	groupParseTokens();
}

void KoreanParseSeeder::dumpAllCharts(UTF8OutputStream &out) {
	for (int i = 0; i < _n_lex_entries; i++) {
		out << "\nLex Entry " << i << ": \n";
		out << "################Word Chart###################\n";
		int n_seg = _wordNumSeg[i];
		out << "NumSeg: " << n_seg << "\n ";
		out.flush();
		int j;
		for (j = 0; j < n_seg; j++) {
			ParseToken p = _wordChart[j][i];
			out << "\t" << j << " START: " << p.start << " END: " << p.end;
			out.flush();
			out << " Word: " << p.normString.to_string()
				<< " " << p.word_part<< "\n";
			out.flush();
			//out << "\nLEX ENTRY: ";
			//p.lex_ents[0]->dump(out);
		}
		out << "################Merged Chart###################\n";
		n_seg = _mergedNumSeg[i];
		out << "NumSeg: " << n_seg << "\n ";
		out.flush();
		for (j = 0; j < n_seg; j++) {
			ParseToken p = _mergedWordChart[j][i];
			out << "\t" << j << " START: " << p.start << " END: " << p.end;
			out << " Word: " << p.normString.to_string()
				<< " " << p.word_part << " " << "StartOffset: "
				<< p.start_offset << " EndOffset: " << p.end_offset << "\n";
			out.flush();
			// out << "\nLEX ENTRY: ";
			/*
			for (int k = 0; k < p.num_lex_ent; k++) {
				p.lex_ents[k]->dump(out);
			}
			*/
		}
		out.flush();
		out << "###############################################\n";
	}
	out << "\n-------------------------------\n\n";
	out << "################ Grouped Chart###################\n";
	for (int j = 0; j < _num_grouped; j++) {
		out << "\nGROUP " << j << ": \n";
		SameSpanParseToken sspt = _groupedWordChart[j];
		for (int k = 0; k < sspt.num_ent; k++) {
			ParseToken p = sspt.pt[k];
			out << "\t" << j << " START: " << p.start << " END: " << p.end << "\n";
			out.flush();
			out << "\t\tWord: " << p.normString.to_string()
				<< " "<< p.word_part << " " <<" StartOffset: "
				<< p.start_offset << " EndOffset: " << p.end_offset << "\n";
		}
	}
	out << "###############################################\n";
	out.flush();
	out << "-------------------------------\n";
}

void KoreanParseSeeder::dumpAllCharts(std::ostream &out) {
	for (int i = 0; i < _n_lex_entries; i++) {
		out << "\nLex Entry " << i << ": \n";
		out << "################Word Chart###################\n";
		int n_seg = _wordNumSeg[i];
		out << "NumSeg: " << n_seg << "\n ";
		out.flush();
		int j;
		for (j = 0; j < n_seg; j++) {
			ParseToken p = _wordChart[j][i];
			out << "\t" << j << " START: " << p.start << " END: " << p.end;
			out.flush();
			out << " Word: " << p.normString.to_debug_string()
				<< " " << p.word_part<< "\n";
			out.flush();
			//out << "\nLEX ENTRY: ";
			//p.lex_ents[0]->dump(out);
		}
		out << "################Merged Chart###################\n";
		n_seg = _mergedNumSeg[i];
		out << "NumSeg: " << n_seg << "\n ";
		out.flush();
		for (j = 0; j < n_seg; j++) {
			ParseToken p = _mergedWordChart[j][i];
			out << "\t" << j << " START: " << p.start << " END: " << p.end;
			out << " Word: " << p.normString.to_debug_string()
				<< " " << p.word_part << " " << "StartOffset: "
				<< p.start_offset << " EndOffset: " << p.end_offset << "\n";
			out.flush();
			// out << "\nLEX ENTRY: ";
			/*
			for (int k = 0; k < p.num_lex_ent; k++) {
				p.lex_ents[k]->dump(out);
			}
			*/
		}
		out.flush();
		out << "###############################################\n";
	}
	out << "\n-------------------------------\n\n";
	out << "################ Grouped Chart###################\n";
	for (int j = 0; j < _num_grouped; j++) {
		out << "\nGROUP " << j << ": \n";
		SameSpanParseToken sspt = _groupedWordChart[j];
		for (int k = 0; k < sspt.num_ent; k++) {
			ParseToken p = sspt.pt[k];
			out << "\t" << j << " START: " << p.start << " END: " << p.end << "\n";
			out.flush();
			out << "\t\tWord: " << p.normString.to_debug_string()
				<< " "<< p.word_part << " " <<" StartOffset: "
				<< p.start_offset << " EndOffset: " << p.end_offset << "\n";
		}
	}
	out << "###############################################\n";
	out.flush();
	out << "-------------------------------\n";
}

/**
 * Populate _wordChart with segments from all lexical entries for t.
 * Also record number of segments and length of concatenation of
 * segments for each lexical entry.
**/
void KoreanParseSeeder::populateWordChart(const Token* t) {
	for (int i = 0; i < _n_lex_entries; i++) {
		LexicalEntry* le = t->getLexicalEntry(i);
		int nseg = le->getNSegments();	
		int word_length = 0;
		if (nseg == 0) { // special case
			_wordChart[0][i].normString = t->getSymbol();
			_wordChart[0][i].lex_ents[0] = le;
			word_length += static_cast<int>(wcslen(t->getSymbol().to_string()));
			_wordNumSeg[i] = 1;
		}
		else {
			for (int j = 0; j < nseg; j++) {
				_wordChart[j][i].normString = le->getSegment(j)->getKey();
				_wordChart[j][i].lex_ents[0] = le->getSegment(j);
				word_length += static_cast<int>(wcslen(_wordChart[j][i].normString.to_string()));
			}
			_wordNumSeg[i] = nseg;
		}
		_wordLength[i] = word_length;
	}
}

/**
 * Normalize the length of all lexical analyses by adding extra null 
 * characters to first segment of segmentations with less than the
 * max number of characters.  Next, identify the locations of all
 * potential segmentation breaks.  For instance, given the following
 * segmentations:
 *
 * ab cd e
 *	abc de
 *	ab cd f
 * 0ab cd
 *
 * the final chart should allow for breaks after the following indicies:
 * 1, 2, 3, 4
**/
void KoreanParseSeeder::alignWordChart() {
	int n_poss = _n_lex_entries;
	int maxWordLen = 0;
	int minWordLen = MAX_CHAR_PER_WORD;

	for (int n = 0; n < n_poss; n++) {
		if (_wordLength[n] > maxWordLen) {
			maxWordLen = _wordLength[n];
		}
		if (_wordLength[n] < minWordLen) {
			minWordLen = _wordLength[n];
		}
	}
	if (maxWordLen >= MAX_CHAR_PER_WORD) {
		throw UnrecoverableException("KoreanParseSeeder::WordChart()",
									 "Word is more than MAX_WORD_PER_CHAR characters");
	}

	//breaks go in the character preceeding a break
	//maximum number of words +1 for composite!
	int segmentBreaks[MAX_CHAR_PER_WORD][MAX_POSSIBILITIES_PER_WORD+1];
	//intialize char Matrix
	for (int j = 0; j < MAX_CHAR_PER_WORD; j++) {
		for (int k = 0; k <= MAX_POSSIBILITIES_PER_WORD; k++) {
			segmentBreaks[j][k] = 0;
		}
	}

	int place = 0;
	int add_to_stem = 0; // number of characters to add to make all words equal length
	int i;
	for (i = 0; i < n_poss; i++) {
		place = 0;
		add_to_stem = maxWordLen - _wordLength[i];
		for (int j = 0; j < _wordNumSeg[i]; j++) { 
			int thisSegLen = static_cast<int>(wcslen(_wordChart[j][i].normString.to_string()));
			if (j == 0) { 
				thisSegLen += add_to_stem;
				add_to_stem = 0;	//only do this once!
			}
			//std::cout << "Segment: " << j << " length: " << thisSegLen << ", ";
			if (segmentBreaks[place+thisSegLen-1][n_poss] >=  
				segmentBreaks[place+thisSegLen][n_poss])
			{
				for (int k = place+thisSegLen; k <= maxWordLen; k++) {
					segmentBreaks[k][n_poss]++;
				}
			}
			for (int k = place+thisSegLen; k <= maxWordLen; k++) {
				segmentBreaks[k][i]++;
			}
			place += thisSegLen;

		}
		//std::cout << std::endl;
	}
	
	if (DBG) {
		std::cout << "\nSegment Breaks:\n";
		for (int i= 0; i <= n_poss; i++) {
			for (int j = 0; j <= maxWordLen; j++) {
				std::cout << segmentBreaks[j][i] << "\t";
			}
			std::cout << "\n";
		}
	}
	

	int startChar, endChar  = 0;
	for (i = 0; i < n_poss; i++) {
		place = 0;
		add_to_stem = maxWordLen - _wordLength[i];
		for (int j = 0; j < _wordNumSeg[i]; j++) {
			int length = static_cast<int>(wcslen((_wordChart[j][i]).normString.to_string()));
			if (j == 0) { 
				length += add_to_stem;
				add_to_stem = 0;	//only do this once!
			}
			startChar = place;
			endChar = length+place-1;
			//std::cout << "start char: " << startChar << " end char: " << endChar << std::endl;
			int startSeg = segmentBreaks[startChar][n_poss];
			int endSeg = segmentBreaks[endChar][n_poss];
			//std::cout<< "start seg: " << startSeg << " end seg: " << endSeg << std::endl;

			_wordChart[j][i].start = startSeg;
			_wordChart[j][i].end = endSeg;
			place += length;
		}
	}

	if (DBG) {
		std::cout << "\nWord Chart (final):\n";
		for (int i = 0; i < n_poss; i++) {
			for (int j = 0; j < _wordNumSeg[i]; j++) {
				std::cout << _wordChart[j][i].start << ":" << _wordChart[j][i].end << "\t";
			}
			std::cout << "\n";
		}
	}
}

/**
 * Merge the final segments from _wordChart into _mergedWordChart
 * with all lexical, pos and segment break information.
**/
void KoreanParseSeeder::mergeSegments() {
	int merged_count;

	for (int i = 0; i < _n_lex_entries; i++) {
		merged_count = 0;
		for (int j = 0; j < _wordNumSeg[i]; j++) { 
			LexicalEntry* le = _wordChart[j][i].lex_ents[0];
			KoreanFeatureValueStructure* fvs = le->getFeatures();

			Symbol cat; // = fvs->getCategory();
			Symbol pos = fvs->getPartOfSpeech();

			_mergedWordChart[merged_count][i].normString = _wordChart[j][i].normString;
			_mergedWordChart[merged_count][i].start = _wordChart[j][i].start;
			_mergedWordChart[merged_count][i].end = _wordChart[j][i].end;
			_mergedWordChart[merged_count][i].lex_ents[0] = _wordChart[j][i].lex_ents[0];
			_mergedWordChart[merged_count][i].num_lex_ent = 1;
			_mergedWordChart[merged_count][i].word_part = _wordChart[j][i].word_part;
			_mergedWordChart[merged_count][i].cat = cat;
			_mergedWordChart[merged_count][i].pos = pos;
			merged_count++;
		}
		_mergedNumSeg[i] = merged_count;
	}
}

/** 
 * Using the segment breaks calculated in alignWordChart, assign
 * APF offsets to all segments in _mergedWordChart.
**/
void KoreanParseSeeder::addOffsets(const Token* t) {
	wchar_t full_seg_str[MAX_TOKEN_SIZE];
	const wchar_t* tokstr = t->getSymbol().to_string();
	int tok_start_offset = t->getStartOffset();
	int tok_end_offset = t->getEndOffset();

	for (int i = 0; i < _n_lex_entries; i++) {
		int tok_place = 0;
		int curr_len = 0;
		wcscpy(full_seg_str, L"");
		int tok_len = static_cast<int>(wcslen(tokstr));
		// token only has one segment, so we already know the offsets
		if (_mergedNumSeg[i] == 1) {
			_mergedWordChart[0][i].start_offset = tok_start_offset;
			_mergedWordChart[0][i].end_offset = tok_end_offset;
		}
		// token has more than one segment
		else { 
			int j = 0;
			for (j = 0; j < _mergedNumSeg[i]; j++) {
				wcsncat(full_seg_str, _mergedWordChart[j][i].normString.to_string(), MAX_TOKEN_SIZE - curr_len);
				curr_len = static_cast<int>(wcslen(full_seg_str));
			}
			// token string is equivalent to concatenation of segments
			if (wcscmp(tokstr, full_seg_str) == 0) {
				for (j = 0; j < _mergedNumSeg[i]; j++) { 
					const wchar_t* seg_str = _mergedWordChart[j][i].normString.to_string();
					int seg_len = static_cast<int>(wcslen(seg_str));
					_mergedWordChart[j][i].start_offset = tok_place + tok_start_offset;
					_mergedWordChart[j][i].end_offset = seg_len + tok_place + tok_start_offset-1;
					tok_place += seg_len;
				}
			}
			// token string does not match concatenation of segments
			else {
				int map[MAX_TOKEN_SIZE];
				alignTokenWithLexicalSegments(tokstr, full_seg_str, map);
				for (j = 0; j < _mergedNumSeg[i]; j++) { 
					const wchar_t* seg_str = _mergedWordChart[j][i].normString.to_string();
					int seg_len = static_cast<int>(wcslen(seg_str));
					int s = map[tok_place] + tok_start_offset;
					int e = map[tok_place + seg_len - 1] + tok_start_offset;
					_mergedWordChart[j][i].start_offset = s;
					// force end to be >= start
					if (e > s) 
						_mergedWordChart[j][i].end_offset = e;
					else
						_mergedWordChart[j][i].end_offset = s;
					tok_place += seg_len;
				}
			}
			// Error handling - last new offset doesn't match last token offset
			if (_mergedWordChart[j-1][i].end_offset != tok_end_offset) {
				std::cerr << "ERROR: wrong token offset! " << t->getSymbol().to_debug_string() << std::endl;
				std::cerr << "tok_end_offset: " << tok_end_offset << std::endl;
				for (int m = 0; m < _mergedNumSeg[i]; m++) {
					std::cerr << "String: " << _mergedWordChart[m][i].normString.to_debug_string()
						<< " StartOffset: " << _mergedWordChart[m][i].start_offset << " "
						<< " EndOffset: " << _mergedWordChart[m][i].end_offset
						<< std::endl;
				}
				_mergedWordChart[j-1][i].end_offset = tok_end_offset;
			}
			// Error handling - first new offset doesn't match start token offset
			if (_mergedWordChart[0][i].start_offset != tok_start_offset) {
				std::cerr << "ERROR: wrong token offset! " << t->getSymbol().to_debug_string() << std::endl;
				std::cerr << "tok_start_offset: " << tok_start_offset << std::endl;
				for (int m = 0; m < _mergedNumSeg[i]; m++) {
					std::cerr << "String: " << _mergedWordChart[m][i].normString.to_debug_string()
						<< " StartOffset: " << _mergedWordChart[m][i].start_offset << " "
						<< " EndOffset: " << _mergedWordChart[m][i].end_offset
						<< std::endl;
				}
				_mergedWordChart[0][i].start_offset = tok_start_offset;
			}
		}
	}
}

/** 
 * This method creates a single listing of all possible segments within a token.
 * For example, say we have 3 lexical entries:
 *	 ab cd e
 *	 abc de
 *	 ab cd f
 * 
 *  At the end of this method, _groupedWordChart should contain 6 groupings:
 *	ab, cd, e, abc, de, f
**/
void KoreanParseSeeder::groupParseTokens() {
	int num_grouped = 0;
	for (int i = 0; i < _n_lex_entries; i++) {
		for (int j = 0; j < _mergedNumSeg[i]; j++) { 
			int found_match = 0;
			int k = 0;
			for(k = 0; k < num_grouped; k++) {

				if( (_mergedWordChart[j][i].start == _groupedWordChart[k].start)&&
					 (_mergedWordChart[j][i].end == _groupedWordChart[k].end)&&
					 (_mergedWordChart[j][i].normString == _groupedWordChart[k].nvString))
					{
						_groupedWordChart[k].pt[_groupedWordChart[k].num_ent++] = _mergedWordChart[j][i];
						found_match = 1;
						break;
					}
			}
			if (!found_match) {

				_groupedWordChart[num_grouped].start = _mergedWordChart[j][i].start;
				_groupedWordChart[num_grouped].end = _mergedWordChart[j][i].end;
				_groupedWordChart[num_grouped].nvString = _mergedWordChart[j][i].normString;
				_groupedWordChart[k].pt[0] = _mergedWordChart[j][i];
				_groupedWordChart[k].num_ent = 1;

				num_grouped++;
			}
		}
	}
	_num_grouped = num_grouped;
}

/**
  * Given an original token string "abcde" and a string representing
  * the sum of its segments after lexical analysis "abfdge", return a
  * mapping of segment characters to original characters:
  *
  * map[0] = 0, map[1] = 1, map[2] = 2, map[3] = 3, map[4] = 3, map[5] = 4
  **/
bool KoreanParseSeeder::alignTokenWithLexicalSegments(const wchar_t *tokstr, 
												const wchar_t *segstr, 
												int *map) 
{
	UnicodeEucKrEncoder *_encoder = UnicodeEucKrEncoder::getInstance();

	wchar_t h_tokstr[MAX_TOKEN_SIZE];
	wchar_t h_segstr[MAX_TOKEN_SIZE];
	int h_tokmap[MAX_TOKEN_SIZE];
	int h_segmap[MAX_TOKEN_SIZE];
	int n_inserted = 0;
	
	_encoder->decomposeHangul(tokstr, h_tokstr, h_tokmap, MAX_TOKEN_SIZE);
	_encoder->decomposeHangul(segstr, h_segstr, h_segmap, MAX_TOKEN_SIZE);


	int i = 0, j = 0;
	int tok_len = static_cast<int>(wcslen(h_tokstr));
	int seg_len = static_cast<int>(wcslen(h_segstr));

	while (j < seg_len && i < tok_len) {
		if (h_tokstr[i] == h_segstr[j]) {
			map[h_segmap[j]] = h_tokmap[i];
			i++;
			j++;
		}
		// consonant doubling, with null and vowel in between
		else if (j > 0 && j + 2 < seg_len && h_segstr[j] == WordConstants::NULL_JAMO &&
				h_segstr[j+2] == h_segstr[j-1])
		{
			map[h_segmap[j]] = map[h_segmap[j+1]] = map[h_segmap[j+2]] = h_tokmap[i];
			j += 3;
			n_inserted += 3;
		}
		// special case:  0x1162 -> 0x1161 0x110b 0x1165
		else if (h_tokstr[i] == 0x1162 && j + 2 < seg_len &&
				h_segstr[j] == 0x1161 && h_segstr[j+1] == WordConstants::NULL_JAMO && h_segstr[j+2] == 0x1165)
		{
			map[h_segmap[j]] = map[h_segmap[j+1]] = map[h_segmap[j+2]] = h_tokmap[i];
			i++;
			j += 3;
			n_inserted += 2;
		}
		// special case:  0x116b -> 0x116c 0x110b 0x1165
		else if (h_tokstr[i] == 0x116b && j + 2 < seg_len &&
				h_segstr[j] == 0x116c && h_segstr[j+1] == WordConstants::NULL_JAMO && h_segstr[j+2] == 0x1165)
		{
			map[h_segmap[j]] = map[h_segmap[j+1]] = map[h_segmap[j+2]] = h_tokmap[i];
			i++;
			j += 3;
			n_inserted += 2;
		}
		// null jamo inserted -- ignore
		else if ((tok_len + n_inserted < seg_len) && (h_segstr[j] == WordConstants::NULL_JAMO)) {
			if (j > 0)
				map[h_segmap[j]] = map[h_segmap[j-1]];
			else
				map[h_segmap[j]] = h_tokmap[i];
			j++;
			n_inserted++;
		}
		// null jamo + medial vowel inserted -- ignore
		else if ((tok_len + n_inserted < seg_len) && 
				(j > 0) && (h_segstr[j-1] == WordConstants::NULL_JAMO) && 
				(h_segstr[j] >= 0x1160) && (h_segstr[j] <= 0x11a2)) // medial vowel
		{
			map[h_segmap[j]] = map[h_segmap[j-1]];
			j++;
			n_inserted++;
		}
		// final consonant + null + medial vowel -- ignore
		else if ((tok_len + n_inserted + 2 < seg_len) && (j + 2 < seg_len) && (j > 0) &&
				 (h_segstr[j] >= 0x11a8) && ( h_segstr[j] <= 0x11f9) && // final consonant
				 (h_segstr[j+1] == WordConstants::NULL_JAMO) && 
				 (h_segstr[j+2] >= 0x1160) && (h_segstr[j+2] <= 0x11a2)) // medial vowel
		{
			map[h_segmap[j]] = map[h_segmap[j+1]] = map[h_segmap[j+2]] = map[h_segmap[j-1]];
			j += 3;
			n_inserted += 3;
		}
		// if next two jamo match, let's ignore the substitution
		else if ((i + 1) < tok_len && (j + 1 < seg_len) && h_tokstr[i+1] == h_segstr[j+1]) {
			map[h_segmap[j]] = h_tokmap[i];
			i++;
			j++;
			map[h_segmap[j]] = h_tokmap[i];
			i++;
			j++;
		}
		// jamo inserted, next two jamo match -- ignore
		else if ((tok_len + n_inserted < seg_len) && (j + 1 < seg_len) && 
				(h_tokstr[i] == h_segstr[j+1]))
		{
			// if it's a final consonant, map to previous syllable
			if (i > 0 && h_segstr[j] >= 0x11a8 && h_segstr[j] <= 0x11f9)
				map[h_segmap[j]] = h_tokmap[i-1];
			// otherwise, map to next syllable
			else
				map[h_segmap[j]] = h_tokmap[i];
			map[h_segmap[j+1]] = h_tokmap[i];
			i++;
			j += 2;
			n_inserted++;
		}
		else {
			map[h_segmap[j]] = h_tokmap[i];
			i++;
			j++;
		}
	}

	while (j < seg_len) {
		map[h_segmap[j]] = h_tokmap[i-1];
		j++;
	}

	if (h_tokmap[i-1] != h_tokmap[tok_len-1]) {
		SessionLogger::logger->beginWarning();
		(*SessionLogger::logger) << "KoreanParseSeeder::alignTokenWithLexicalSegments(): " 
							     << "Error - alignment did not reach end of token.";
		std::cout << "KoreanParseSeeder::alignTokenWithLexicalSegments(): " 
				  << "Error - alignment did not reach end of token.";
		map[h_segmap[seg_len-1]] = h_tokmap[tok_len-1];
	}

	if (map[0] != 0) {
		SessionLogger::logger->beginWarning();
		(*SessionLogger::logger) << "KoreanParseSeeder::alignTokenWithLexicalSegments(): " 
							     << "Error - alignment does not cover beginning of token.";
		std::cout << "KoreanParseSeeder::alignTokenWithLexicalSegments(): " 
				  << "Error - alignment does not cover beginning of token.";
		map[0] = 0;
	}

	return true;
}
