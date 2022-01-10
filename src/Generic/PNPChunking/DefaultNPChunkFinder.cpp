// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/PNPChunking/DefaultNPChunkFinder.h"

#include "Generic/theories/NameTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/PNPChunking/PNPChunkSentence.h"
#include "Generic/PNPChunking/PNPChunkDecoder.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/common/InternalInconsistencyException.h"

DefaultNPChunkFinder::DefaultNPChunkFinder(){
	_decoder = _new PNPChunkDecoder();
	
}

int DefaultNPChunkFinder::getNPChunkTheories(NPChunkTheory **results, 
                                             const int max_theories, 
                                             const TokenSequence* ts,
                                             const Parse* p,
                                             const NameTheory* nt)
{
	if (ts->getNTokens() == 0) {
		results[0] = _new NPChunkTheory(ts);
		results[0]->takeParse(_new Parse(ts));
		return 1;
	}

	//get the POS array from the parse
	Symbol pos[MAX_SENTENCE_TOKENS];
	int npos = p->getRoot()->getPOSSymbols(pos, MAX_SENTENCE_TOKENS);
	
	if (npos != ts->getNTokens()) {
		throw InternalInconsistencyException("DefaultNPChunkFinder::getNPChunkTheories()",
								"Number of parts of speech does not match number of sentence tokens.");
	}

	PNPChunkSentence* sentence = 0;
	_decoder->decode(ts, pos, sentence);
	results[0] = _new NPChunkTheory(ts);

	//make NP chunk theory
	fillNPChunkTheory(ts->getNTokens(), sentence, results[0], nt);

	// sentence (which was created in _decoder->decode) is no longer used 
	// after fillNPChunkTheory
	delete sentence;

	results[0]->setParse(getSynNode(ts, pos, results[0], nt));

	return 1;
}

int DefaultNPChunkFinder::getNPChunkTheories(NPChunkTheory **results,
                                             const int max_theories,
                                             const TokenSequence* ts,
                                             const PartOfSpeechSequence* p,
                                             const NameTheory* nt)
{
	//get the POS array from the POS sequence
	Symbol pos[MAX_SENTENCE_TOKENS];
	int npos = 0;
	for (int n = 0; n < p->getNTokens(); ++n) {
		PartOfSpeech* parts = p->getPOS(n);
		for (int m = 0; m < parts->getNPOS(); ++m) {
			pos[npos] = parts->getLabel(m);
		}
		++npos;
		if (npos == MAX_SENTENCE_TOKENS) { 
			break;
		}
	}        
	PNPChunkSentence* sentence = 0;

	_decoder->decode(ts, pos, sentence);
	results[0] = _new NPChunkTheory(ts);

	//make NP chunk theory
	fillNPChunkTheory(ts->getNTokens(), sentence, results[0], nt);

	// sentence (which was created in _decoder->decode) is no longer used 
	// after fillNPChunkTheory
	delete sentence;

	results[0]->setParse(getSynNode(ts, pos, results[0], nt));

	return 1;
}

void DefaultNPChunkFinder::fillNPChunkTheory(const int nwords, const PNPChunkSentence* sentence,
                                             NPChunkTheory* chunkTheory)
{
	//make an np chunk theory
	if(chunkTheory ==0){
		SessionLogger::info("SERIF")<<"uninitialized chunktheory"<<std::endl;
		return;
	}
	chunkTheory->n_npchunks = 0;
	int npst = _decoder->getTagSet()->getTagIndex(Symbol(L"NP-ST"));
	int npco =  _decoder->getTagSet()->getTagIndex(Symbol(L"NP-CO"));
	int innp = 0;
	int i = 0;
	for(i =0; i<nwords; i++){
		int currtag = sentence->getTag(i);
		if(innp && (currtag != npco)){
			chunkTheory->npchunks[chunkTheory->n_npchunks][1] = i-1;
			chunkTheory->n_npchunks++;
			innp = 0;
		}
		if(currtag == npst){
			chunkTheory->npchunks[chunkTheory->n_npchunks][0] = i;
			innp = 1;
		}
	}
	if(innp){
		chunkTheory->npchunks[chunkTheory->n_npchunks][1] = i-1;
		chunkTheory->n_npchunks++;
	}
}

void DefaultNPChunkFinder::fillNPChunkTheory(const int nwords, PNPChunkSentence* sentence,
                                             NPChunkTheory* chunkTheory, const NameTheory* nt)
{
	fillNPChunkTheory(nwords, sentence, chunkTheory);

	//look for conflicting names
	int* problemnames = _new int[nt->getNNameSpans()];
	int* goodnames = _new int[nt->getNNameSpans()];
	int* chunklessnames = _new int[nt->getNNameSpans()];
	int nproblem =0;
	int ngood = 0;
	int nchunkless =0;
	for(int i=0; i<nt->getNNameSpans(); i++){
		int s= nt->getNameSpan(i)->start;
		int e = nt->getNameSpan(i)->end;
		int j = 0;
		for(j=0; j< chunkTheory->n_npchunks; j++){
			bool sin = chunkTheory->inChunk(s,j);
			bool ein = chunkTheory->inChunk(e,j);
			if(sin && ein){
				goodnames[ngood++] = i;
				break;
			}
			else if(sin || ein){
				problemnames[nproblem++] =i;
				break;
			}
		}
		if(j == chunkTheory->n_npchunks){
			chunklessnames[nchunkless++] = i;
		}
	}
	int npst = _decoder->getTagSet()->getTagIndex(Symbol(L"NP-ST"));
	int npco =  _decoder->getTagSet()->getTagIndex(Symbol(L"NP-CO"));

	for(int j=0; j<nproblem; j++){
		int s = nt->getNameSpan(problemnames[j])->start;
		int e = nt->getNameSpan(problemnames[j])->end;
		int stag = sentence->getTag(s);
		if((stag != npst) && (stag != npco)){
			sentence->setTag(s, npst);
		}
		for(int i=s+1; i<=e; i++){
			sentence->setTag(i, npco);
		}
	}
	for(int k=0; k<nchunkless; k++){
		int s = nt->getNameSpan(chunklessnames[k])->start;
		int e = nt->getNameSpan(chunklessnames[k])->end;
		//int schunk = chunkTheory->getChunk(s);
		//int echunk = chunkTheory->getChunk(e);
		int stag = sentence->getTag(s);
		if((stag != npst) && (stag != npco)){
			sentence->setTag(s, npst);
		}
		for(int i=s+1; i<=e; i++){
			sentence->setTag(i, npco);
		}
	}
	delete [] goodnames;
	delete [] chunklessnames;
	delete [] problemnames;
	chunkTheory->n_npchunks =0;
	//remake the np chunk theory adjusted so that all names are in a chunk
	fillNPChunkTheory(nwords, sentence, chunkTheory);
}

SynNode* DefaultNPChunkFinder::getSynNode(const TokenSequence* ts, const Symbol* pos, const NPChunkTheory *chunkTheory, const NameTheory* nt) {
	//make a shallow parse tree.... phrases for NP chunks (NP) and subphrases for names (NPP)
	int currchunk = 0;
	int currchild= 0;
	int currname =0;
	int start = -1;
	int end =-1;
	if(currchunk < chunkTheory->n_npchunks){
		start =  chunkTheory->npchunks[currchunk][0];
		end =  chunkTheory->npchunks[currchunk][1];
	}
	int namestart =-1;
	int nameend =-1;
	if(currname < nt->getNNameSpans()){
		namestart = nt->getNameSpan(currname)->start;
		nameend = nt->getNameSpan(currname)->end;
	}
		
	//number of children
	int nc = ts->getNTokens() + chunkTheory->n_npchunks;
	for(int m=0; m<chunkTheory->n_npchunks;m++){
		int chunklength = (chunkTheory->npchunks[m][1] - chunkTheory->npchunks[m][0]+1);
		nc = nc - chunklength;
	}
	int nodeid = 0;
	SynNode* root = _new SynNode(nodeid++, 0, Symbol(L"FRAG"), nc);
	root->setTokenSpan(0,ts->getNTokens()-1);
	root->setHeadIndex(0);
	for(int l =0; l < ts->getNTokens();){
		if(l == start){
			//get the number of children 
			int nchildren = end -start +1;
			int tempcurrname = currname;
			int tempnamestart = namestart;
			int tempnameend = nameend;
			int j= start; 
			while(j<=end){
				if(j == tempnamestart){
					j = tempnameend+1;
					nchildren = nchildren +1 -(tempnameend -tempnamestart+1);
					tempcurrname++;
					if(tempcurrname < nt->getNNameSpans()){
						tempnamestart = nt->getNameSpan(tempcurrname)->start;
						tempnameend = nt->getNameSpan(tempcurrname)->end;
					}
				}
				else{
					j++;
				}
			}
			SynNode* np = _new SynNode(nodeid++, root, Symbol(L"NP"), nchildren);
			np->setTokenSpan(start, end);
			int c =0;
			while(l<=end){
				if( l == namestart){
					int nname = nameend - namestart+1;
					SynNode* nameNode = _new SynNode(nodeid++, np, Symbol(L"NPP"), nname);
					nameNode->setTokenSpan(namestart, nameend);
					int namec = 0;
					while(l <=nameend){
						SynNode* namepreterm = _new SynNode(nodeid++, nameNode, pos[l], 1);
						namepreterm->setTokenSpan(l,l);
						/*
						std::cout<<"\t\tAdd name child: "<< namec <<" of "<<nname<<
							" "<<ts->getToken(l)->getSymbol().to_debug_string()<< std::endl;
						*/
						nameNode->setChild(namec, namepreterm);
						SynNode* nameterm  = _new SynNode(nodeid++, namepreterm, 
							LanguageSpecificFunctions::getSymbolForParseNodeLeaf(ts->getToken(l)->getSymbol()), 0);
						namepreterm->setChild(0, nameterm);
						nameterm->setTokenSpan(l,l);
						nameterm->setHeadIndex(0);
						namec++;
						l++;
					}
					nameNode->setHeadIndex(LanguageSpecificFunctions::findNPHead(nameNode->getChildren(), namec));
					/*
					std::cout<<"\tAdd np child for name "<< currname<<
						" start "<<namestart <<" nameend "<<end<<
						" child: " << c<<" of "<<nchildren<<
						" NAME " <<std::endl;
					*/
					np->setChild(c, nameNode);
					c++;

					currname++;
					if(currname < nt->getNNameSpans()){
						namestart = nt->getNameSpan(currname)->start;
						nameend = nt->getNameSpan(currname)->end;
					}
					else{
						namestart = -1;
						nameend = -1;
					}
					
				}
				else{
					SynNode* preterm = _new SynNode(nodeid++, np, pos[l], 1);
					preterm->setTokenSpan(l,l);
					preterm->setHeadIndex(0);
					/*
					std::cout<<"\tAdd np child: "<< c<<" of "<<nchildren<<
						" "<<ts->getToken(l)->getSymbol().to_debug_string()<<std::endl;
					*/
					np->setChild(c, preterm);
					SynNode* term = _new SynNode(nodeid++, preterm, 
						LanguageSpecificFunctions::getSymbolForParseNodeLeaf(ts->getToken(l)->getSymbol()), 0);
					preterm->setChild(0, term);
					term->setTokenSpan(l,l);
					term->setHeadIndex(0);
					c++;
					l++;
				}
			}
			np->setHeadIndex(LanguageSpecificFunctions::findNPHead(np->getChildren(), c));

			/*
			std::cout<<"Add root child for chunk "<< currchunk<<
				" start "<<start <<" end "<<end<<
				" child: " << currchild<<" of "<<nc<<
				" NP "<<std::endl;
			*/
			currchunk++;
			if((currchunk < chunkTheory->n_npchunks) ){
				start = chunkTheory->npchunks[currchunk][0];
				end = chunkTheory->npchunks[currchunk][1];
			}
			else{
				start= -1;
				end = -1;
			}

			root->setChild(currchild, np);
			currchild++;
		}
		else{

			SynNode* preterm = _new SynNode(nodeid++, root, pos[l], 1);
			preterm->setTokenSpan(l,l);
			preterm->setHeadIndex(0);
			/*
			std::cout<<"Add root child: "<< currchild<<" of "<<nc<<
				" "<<ts->getToken(l)->getSymbol().to_debug_string()<<std::endl;
			*/
			root->setChild(currchild, preterm);
			SynNode* term = _new SynNode(nodeid++, preterm, 
				LanguageSpecificFunctions::getSymbolForParseNodeLeaf(ts->getToken(l)->getSymbol()), 0);
			preterm->setChild(0, term);
			term->setTokenSpan(l,l);
			term->setHeadIndex(0);
			currchild++;
			l++;
		}
	}
	return root;
}
