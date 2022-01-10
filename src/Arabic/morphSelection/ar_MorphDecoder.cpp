// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/ParamReader.h"
#include "Arabic/morphSelection/ar_MorphDecoder.h"
#include "Arabic/BuckWalter/ar_MorphologicalAnalyzer.h"
#include "Generic/theories/LexicalToken.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/LexicalTokenSequence.h"
#include "Arabic/BuckWalter/ar_ParseSeeder.h"
#include <boost/foreach.hpp>

void ArabicMorphDecoder::putTokenSequenceInAtomizedSentence(const LocatedString &sentenceString, TokenSequence *ts) {
	LexicalTokenSequence *lts = dynamic_cast<LexicalTokenSequence*>(ts);
	if (!lts) throw InternalInconsistencyException("MorphDecoder::putTokenSequenceInAtomizedSentence",
		"This MorphDecoder requires a LexicalTokenSequence.");

	_num_tokens = lts->getNTokens();
	for(int i=0; i<lts->getNTokens();i++){
		_atomized_sentence[i].possibilities.clear();
		ps->processToken(sentenceString, lts->getToken(i));
		const LexicalToken* t = lts->getToken(i);

		/*	//debug printing
		int nle =t->getNLexicalEntries(
		);
		for(int j=0; j<nle; j++){
			t->getLexicalEntry(j)->dump(std::cout);
		}
		std::cout<<"Add "<<_ps->_n_lex_entries<<" possibilities to token: "<<i<<std::endl;
		_ps->dumpBothCharts(std::cout);
		*/

		for(int j =0; j< ps->_n_lex_entries; j++){
			int num_seg = ps->_mergedNumSeg[j];
			//need to check if an identical segment has already been added
			bool add = true;
			for (size_t k = 0; k < _atomized_sentence[i].possibilities.size(); k++) {
				TokenChunking possibility = _atomized_sentence[i].possibilities[k];
				if(possibility.size() == (size_t)num_seg){
					bool match_all = true;
					for(int n=0; n<num_seg; n++){
						if(possibility[n].symbol != ps->_mergedWordChart[n][j].normString){
							match_all =false;
							break;
						}
					}
					if(match_all){
						add = false;
					}
				}
				if(add == false){
					break;
				}
			}

			if(add){
				if(_atomized_sentence[i].possibilities.size() < 50){
					_atomized_sentence[i].possibilities.push_back(TokenChunking());
					/** This is no longer a hard limit: */
					// if(num_seg >MAX_CHUNKS_PER_WORD){
					//
					// 	std::cerr<<"Morphdecoder: NUM_SEG= "<<num_seg<<std::endl;
					// 	t->dump(std::cerr);
					// 	int nle = t->getNLexicalEntries();
					// 	for(int q=0; q<nle; q++){
					// 		const LexicalEntry *le =t->getLexicalEntry(q);
					// 		le->dump(std::cerr);
					// 		std::cerr<<std::endl;
					// 	}
					// 	ps->dumpBothCharts(std::cerr);
					// 	std::cerr<<std::endl;
					// 	throw UnrecoverableException(
					// 		"MorphDecoder::putTokenSequenceInAtomizedSentence()",
					// 		"Too many atoms!");
					//
					// }
					TokenChunking &chunking = _atomized_sentence[i].possibilities.back();
					for(int k =0; k< num_seg; k++){
						chunking.push_back(Chunk(ps->_mergedWordChart[k][j].normString,
												 ps->_mergedWordChart[k][j].start_offset,
												 ps->_mergedWordChart[k][j].end_offset));

					}
				}
			}
		}
	}
}

void ArabicMorphDecoder::putTokenSequenceInAtomizedSentence(const LocatedString& sentenceString,
                                                       TokenSequence *ts,
													   CharOffset *constraints,
													   int n_constraints)
{
	LexicalTokenSequence *lts = dynamic_cast<LexicalTokenSequence*>(ts);
	if (!lts) throw InternalInconsistencyException("MorphDecoder::putTokenSequenceInAtomizedSentence",
		"This MorphDecoder requires a LexicalTokenSequence.");

	CharOffset break_offsets[MAX_TOKEN_SIZE];
	int n_break_offsets;

	_num_tokens = lts->getNTokens();
	for (int i=0; i<lts->getNTokens();i++){
		_atomized_sentence[i].possibilities.clear();
		ps->processToken(sentenceString, lts->getToken(i));
		const LexicalToken* t = lts->getToken(i);

		CharOffset tok_start = t->getStartCharOffset();
		CharOffset tok_end = t->getEndCharOffset();


		n_break_offsets = 0;
		for (int b = 0; b < n_constraints; b++) {
			if (constraints[b] > tok_start && constraints[b] < tok_end)
				break_offsets[n_break_offsets++] = constraints[b];
		}
		/*	//debug printing
		int nle =t->getNLexicalEntries(
		);
		for(int j=0; j<nle; j++){
			t->getLexicalEntry(j)->dump(std::cout);
		}
		std::cout<<"Add "<<_ps->_n_lex_entries<<" possibilities to token: "<<i<<std::endl;
		_ps->dumpBothCharts(std::cout);
		*/

		bool found_poss_seg = false;
		for(int j =0; j< ps->_n_lex_entries; j++){
			int num_seg = ps->_mergedNumSeg[j];
			//need to check if an identical segment has already been added
			bool add = true;
			for (size_t k = 0; k < _atomized_sentence[i].possibilities.size(); k++) {
				TokenChunking possibility = _atomized_sentence[i].possibilities[k];

				if(possibility.size() == (size_t)num_seg){
					bool match_all = true;
					for(int n=0; n<num_seg; n++){
						if(possibility[n].symbol != ps->_mergedWordChart[n][j].normString){
							match_all =false;
							break;
						}
					}
					if(match_all){
						add = false;
					}
				}
				if(add == false){
					break;
				}

			}
			// check to make sure it breaks at any constraints
			if (add) {
				for (int a = 0; a < n_break_offsets; a++) {
					for (int n = 0; n < num_seg; n++) {
						if (ps->_mergedWordChart[n][j].start_offset.value<CharOffset>() < break_offsets[a] &&
							ps->_mergedWordChart[n][j].end_offset.value<CharOffset>() > break_offsets[a])
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
				_atomized_sentence[i].possibilities.clear();
			}
			if(add){
				if(_atomized_sentence[i].possibilities.size() < 50){
					_atomized_sentence[i].possibilities.push_back(TokenChunking());
					/** This is no longer a hard limit: */
					// if(num_seg >MAX_CHUNKS_PER_WORD){
					//
					// 	std::cerr<<"Morphdecoder: NUM_SEG= "<<num_seg<<std::endl;
					// 	t->dump(std::cerr);
					// 	int nle = t->getNLexicalEntries();
					// 	for(int q=0; q<nle; q++){
					// 		const LexicalEntry *le =t->getLexicalEntry(q);
					// 		le->dump(std::cerr);
					// 		std::cerr<<std::endl;
					// 	}
					// 	ps->dumpBothCharts(std::cerr);
					// 	std::cerr<<std::endl;
					// 	throw UnrecoverableException(
					// 		"MorphDecoder::putTokenSequenceInAtomizedSentence()",
					// 		"Too many atoms!");
					//
					// }
					TokenChunking &chunking = _atomized_sentence[i].possibilities.back();
					for(int k =0; k< num_seg; k++){
						chunking.push_back(Chunk(ps->_mergedWordChart[k][j].normString,
												 ps->_mergedWordChart[k][j].start_offset,
												 ps->_mergedWordChart[k][j].end_offset));

					}
				}
			}
		}
		if (!found_poss_seg) {
			std::cerr << "Never found a possible segmentation for sentence " << lts->getSentenceNumber();
			std::cerr << " token # " << i << "\n";
		}
		if (_atomized_sentence[i].possibilities.size() == 0)
			throw InternalInconsistencyException("MorphDecoder::_putTokenSequenceInAtomizedSentence",
												 "Zero segment possibilities added for token.");
	}
}
