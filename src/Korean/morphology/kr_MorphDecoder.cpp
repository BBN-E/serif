// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/ParamReader.h"
#include "Korean/morphSelection/kr_MorphDecoder.h"
#include "Korean/morphSelection/kr_ParseSeeder.h"
#include "Korean/morphAnalysis/kr_MorphologicalAnalyzer.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/TokenSequence.h"

void MorphDecoder::putTokenSequenceInAtomizedSentence(const LocatedString &sentenceString, TokenSequence *ts) {
	_num_tokens = ts->getNTokens();
	for (int i = 0; i < ts->getNTokens(); i++) {
		ps->processToken(ts->getToken(i));
		const Token* t = ts->getToken(i);

		//debug printing
		/*int nle = t->getNLexicalEntries();
		for (int j = 0; j < nle; j++) {
			t->getLexicalEntry(j)->dump(std::cout);
		}
		std::cout << "Add " << _ps->_n_lex_entries << " possibilities to token: " << i << std::endl;
		_ps->dumpBothCharts(std::cout);
		*/

		_atomized_sentence[i].num_choices =  ps->_n_lex_entries;

		int num_added = 0;
		for (int j = 0; j < ps->_n_lex_entries; j++) {
			int num_seg = ps->_mergedNumSeg[j];
			//need to check if an identical segment has already been added
			bool add = true;
			for (int m = 0; m < num_added; m++) {
				if (_atomized_sentence[i].num_chunks[m] == num_seg) {
					bool match_all = true;
					for (int n = 0; n < num_seg; n++) {
						if (_atomized_sentence[i].possibilities[m][n] != ps->_mergedWordChart[n][j].normString) {
							match_all = false;
							break;
						}
					}
					if (match_all) {
						add = false;
					}
				}
				if (add == false) {
					break;
				}

			}
			if (add) {
				if (num_added < 50) {
					/** This is no longer a hard limit: */
					// if (num_seg > MAX_CHUNKS_PER_WORD) {
					//
					// 	std::cerr << "KoreanMorphDecoder: NUM_SEG= " << num_seg << std::endl;
					// 	t->dump(std::cerr);
					// 	int nle = t->getNLexicalEntries();
					// 	for (int q = 0; q < nle; q++) {
					// 		const LexicalEntry *le =t->getLexicalEntry(q);
					// 		le->dump(std::cerr);
					// 		std::cerr << std::endl;
					// 	}
					// 	ps->dumpBothCharts(std::cerr);
					// 	std::cerr<<std::endl;
					// 	throw UnrecoverableException("KoreanMorphDecoder::putTokenSequenceInAtomizedSentence()",
					// 								 "Too many atoms!");
					//
					// }
					_atomized_sentence[i].num_chunks[num_added] = num_seg;
					for (int k = 0; k < num_seg; k++) {
						_atomized_sentence[i].possibilities[num_added][k] = ps->_mergedWordChart[k][j].normString;
						_atomized_sentence[i].start_offsets[num_added][k] = ps->_mergedWordChart[k][j].start_offset;
						_atomized_sentence[i].end_offsets[num_added][k] = ps->_mergedWordChart[k][j].end_offset;
					}
					num_added++;
				}
			}
		}
		_atomized_sentence[i].num_choices =  num_added;
	}
}

void MorphDecoder::putTokenSequenceInAtomizedSentence(const LocatedString &sentenceString, TokenSequence *ts,
													   CharOffset *constraints,
													   int n_constraints)
{
	int break_offsets[MAX_TOKEN_SIZE];
	int n_break_offsets;

	_num_tokens = ts->getNTokens();
	for(int i=0; i<ts->getNTokens();i++){
		ps->processToken(ts->getToken(i));
		const Token* t = ts->getToken(i);

		CharOffset tok_start = t->getStartCharOffset();
		CharOffset tok_end = t->getEndCharOffset();

		n_break_offsets = 0;
		for (int b = 0; b < n_constraints; b++) {
			if (constraints[b] > tok_start && constraints[b] <= tok_end)
				break_offsets[n_break_offsets++] = constraints[b];
		}

		//debug printing
		/*int nle =t->getNLexicalEntries();
		for(int j=0; j<nle; j++){
			t->getLexicalEntry(j)->dump(std::cout);
		}
		std::cout << "Add " << _ps->_n_lex_entries << " possibilities to token: " << i << std::endl;
		_ps->dumpBothCharts(std::cout);
		*/

		_atomized_sentence[i].num_choices =  ps->_n_lex_entries;

		int num_added =0;
		bool found_poss_seg = false;
		for (int j = 0; j < ps->_n_lex_entries; j++) {
			int num_seg = ps->_mergedNumSeg[j];
			//need to check if an identical segment has already been added
			bool add = true;
			for (int m = 0; m < num_added; m++) {
				if (_atomized_sentence[i].num_chunks[m] == num_seg) {
					bool match_all = true;
					for (int n = 0; n < num_seg; n++) {
						if (_atomized_sentence[i].possibilities[m][n] != ps->_mergedWordChart[n][j].normString) {
							match_all = false;
							break;
						}
					}
					if (match_all) {
						add = false;
					}
				}
				if (add == false) {
					break;
				}

			}
			// check to make sure it breaks at any constraints
			if (add) {
				for (int a = 0; a < n_break_offsets; a++) {
					for (int n = 0; n < num_seg; n++) {
						if (ps->_mergedWordChart[n][j].start_offset.value<CharOffset>() < break_offsets[a] &&
							ps->_mergedWordChart[n][j].end_offset.value<CharOffset>() >= break_offsets[a])
						{
							add = false;
							break;
						}
					}
				}
			}
			// we haven't found any perfect possibilities, so add it anyway
			if (!found_poss_seg && !add) {
				add = true;
			}
			else if (!found_poss_seg && add) {
				found_poss_seg = true;
				num_added = 0;
			}
			if (add) {
				if (num_added < 50) {
					/** This is no longer a hard limit: */
					// if (num_seg >MAX_CHUNKS_PER_WORD) {
					// 	std::cerr << "KoreanMorphDecoder: NUM_SEG= " << num_seg << std::endl;
					// 	t->dump(std::cerr);
					// 	int nle = t->getNLexicalEntries();
					// 	for (int q = 0; q < nle; q++) {
					// 		const LexicalEntry *le = t->getLexicalEntry(q);
					// 		le->dump(std::cerr);
					// 		std::cerr<<std::endl;
					// 	}
					// 	ps->dumpBothCharts(std::cerr);
					// 	std::cerr << std::endl;
					// 	throw UnrecoverableException("KoreanMorphDecoder::putTokenSequenceInAtomizedSentence()",
					// 		                         "Too many atoms!");
					//
					// }
					_atomized_sentence[i].num_chunks[num_added] = num_seg;
					for (int k = 0; k < num_seg; k++) {
						_atomized_sentence[i].possibilities[num_added][k] = ps->_mergedWordChart[k][j].normString;
						_atomized_sentence[i].start_offsets[num_added][k] = ps->_mergedWordChart[k][j].start_offset;
						_atomized_sentence[i].end_offsets[num_added][k] = ps->_mergedWordChart[k][j].end_offset;
					}
					num_added++;
				}
			}
		}
		if (!found_poss_seg) {
			cerr << "Never found a possible segmentation for sentence " << ts->getSentenceNumber();
			cerr << " token # " << i;
			cerr << " [" << t->getStartOffset() << "," << t->getEndOffset() << "] ";
			cerr << "\n";
		}
		if (num_added == 0)
			throw InternalInconsistencyException("KoreanMorphDecoder::_putTokenSequenceInAtomizedSentence",
												 "Zero segment possibilities added for token.");
		_atomized_sentence[i].num_choices =  num_added;
	}
}
