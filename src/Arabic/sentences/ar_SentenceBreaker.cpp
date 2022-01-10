// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/common/ar_ArabicSymbol.h"

#include "Generic/common/limits.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8Token.h"

#include "Arabic/BuckWalter/ar_MorphologicalAnalyzer.h"
#include "Generic/morphSelection/MorphSelector.h"
#include "Arabic/partOfSpeech/ar_PartOfSpeechRecognizer.h"
#include "Arabic/sentences/ar_SentenceBreaker.h"
#include "Arabic/names/ar_NameRecognizer.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/LexicalTokenSequence.h"
#include "Arabic/tokens/ar_Tokenizer.h"

#include "math.h"
#include <time.h>
#include <wchar.h>
#include <boost/scoped_ptr.hpp>
#define BREAK_DATELINES          true
#define USE_AFP_DATELINE         true
#define USE_XINHUA_DATELINE         true
#define CONJ_SENTENCE_TOKENS_MIN		8 //10 // minimum tokens for sentence break at conjunction
#define CONJ_LAST_SENTENCE_TOKENS_MIN	8 //10 // minimum tokens to leave  for conjoined last sentence (else append them)
#define LAST_SENTENCE_TOKENS_MIN		10 // minimum tokens to leave  for last sentence (else append them)
#define ELLIPSIS_BREAKS 4 // this many or more dots means a sentence break after ellipsis
#define ELLIPSIS_MIN_DOTS 2  // minimum to be ellipsis -- some say two will do ...
hash_set<Symbol,	ArabicSentenceBreaker::HashKey, ArabicSentenceBreaker::EqualKey> ArabicSentenceBreaker::_nonEOSWords(100);
		
Symbol ArabicSentenceBreaker::ARABIC_AND_SYMBOL = Symbol(L"\x0648"); // letter "waw"
Symbol ArabicSentenceBreaker::ARABIC_BUT_SYMBOL = Symbol(L"\x0648\x0644\x0643\x0646");
Symbol ArabicSentenceBreaker::ARABIC_XINHUA_SYMBOL = Symbol(L"(\x0634\x064a\x0646\x062e\x0648\x0627)");
// waw,lam,kah,nuun -> "wa - lakin", meaning "and-but"


ArabicSentenceBreaker::ArabicSentenceBreaker():
	_tokenizer(0), _tokenSequenceBuf(0), _nameRecognizer(0), _nameTheoryBuf(0),
	_morphAnalysis(0), _morphSelector(0), _posRecognizer(0), _partOfSpeechSequenceBuf(0)
{	
	_breakLongSentencesUsingHeuristicRule = false;
	_breakLongSentencesUsingStatisticalModel = false;

	_breakLongSentences = ParamReader::isParamTrue("break_long_sentences");
	if(_breakLongSentences){
		std::string not_eos_word_file = ParamReader::getParam("not_eos_word_file");
		if (!not_eos_word_file.empty()) {
			boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
			UTF8InputStream& uis(*uis_scoped_ptr);
			uis.open(not_eos_word_file.c_str());
			int numlines;
			uis >> numlines;
			UTF8Token word;
			for(int i = 0; i< numlines; i++){
				uis >> word;
				_nonEOSWords.insert(word.symValue());
			}
			uis.close();
		}

		_max_token_breaks = ParamReader::getOptionalIntParamWithDefaultValue("ar_sentence_breaker_max_token_breaks", 60);

		_breakLongSentencesUsingHeuristicRule = ParamReader::isParamTrue("break_long_sentences_heuristic_rule");
		_breakLongSentencesUsingStatisticalModel = ParamReader::isParamTrue("break_long_sentences_statistical_model");
		if (_breakLongSentencesUsingHeuristicRule || _breakLongSentencesUsingStatisticalModel) {
			_tokenizer = Tokenizer::build();
			_tokenSequenceBuf = _new TokenSequence*[MAX_SUB_SENTENCES_WRAPPER_SIZE];

			//Always Initialize the Morphological Analyzer,
			//Because we need access to the lexion in all stages
			// requires the parameter 'BWMorphDict' to be defined
			_morphAnalysis = MorphologicalAnalyzer::build();

			//Take care of 'bigram' morph selection with tokenization
			_morphSelector = _new MorphSelector();

			// POS
			_posRecognizer = PartOfSpeechRecognizer::build();
			_partOfSpeechSequenceBuf = _new PartOfSpeechSequence*[MAX_SUB_SENTENCES_WRAPPER_SIZE];

			// NAMES
			_nameRecognizer = (ArabicNameRecognizer*)NameRecognizer::build();
			_nameTheoryBuf = _new NameTheory*[MAX_SUB_SENTENCES_WRAPPER_SIZE];

			for (int i = 0; i < MAX_SUB_SENTENCES_WRAPPER_SIZE; i++) {
				// so we may safely delete these later
				_tokenSequenceBuf[i] = 0;
				_partOfSpeechSequenceBuf[i] = 0;
				_nameTheoryBuf[i] = 0;
			}
		}
	}
}

ArabicSentenceBreaker::~ArabicSentenceBreaker() {
	delete _tokenizer;
	delete _nameRecognizer;
	delete _morphAnalysis;
	delete _morphSelector;
	delete _posRecognizer;

	delete [] _tokenSequenceBuf;
	delete [] _partOfSpeechSequenceBuf;
	delete [] _nameTheoryBuf;
}


//given a string, an offset in that string and an offset return the token associated with that offset

int ArabicSentenceBreaker::getTokenFromOffset(const LocatedString* source, const TokenSequence* toks, int offset){
	return getTokenFromOffset(source, toks, offset,  0);
}

//given a string, an offset in that string and an offset return the token associated with that offset
//that is after startTok
int ArabicSentenceBreaker::getTokenFromOffset(const LocatedString* source, const TokenSequence* toks,
					int offset, int startTok){

	SessionLogger &logger = *SessionLogger::logger;
	CharOffset tokCharStart, tokCharEnd;
	CharOffset stringCharStartOffset, stringCharEndOffset;
	stringCharStartOffset = source->start<CharOffset>(offset);
	stringCharEndOffset = source->end<CharOffset>(offset);
	//logger<<"edtStart = "<<stringCharStartOffset<<" edtEnd = "<<stringCharEndOffset<<"\n";
	//std::cout<<"edtStart = "<<stringCharStartOffset<<" edtEnd = "<<stringCharEndOffset<<std::endl;
	if(stringCharStartOffset != stringCharEndOffset){
		logger.reportError()<<"getTokenFromOffset() offsets don't agree!\n";
		//std::cerr<<"getTokenFromOffset() offsets don't agree!\n";
	}

	for(int i =startTok; i< toks->getNTokens(); i++){
        const Token* t = toks->getToken(i);
		tokCharStart = t->getStartCharOffset();
		tokCharEnd = t->getEndCharOffset();
		if( (tokCharStart >= stringCharStartOffset) &&
			(stringCharStartOffset <= tokCharStart )){
				return i;
			}
	}
	return -1;
}
bool  ArabicSentenceBreaker::isMidName(int currTok, const NameTheory* nt){
	int n_names = nt->getNNameSpans();
	for (int i = 0; i < n_names; i++){
		int start = nt->getNameSpan(i)->start;
		int end = nt->getNameSpan(i)->end;
		if( (start >= currTok) &&
			(end <= currTok ) ){
				return true;
			}
	}
	return false;
}

bool  ArabicSentenceBreaker::isMidName(int currTok, NameTheoriesWrapper &nameWrapper) {
	for (int i=0; i<nameWrapper.getNSequences(); i++) {
		if(isMidName(currTok, nameWrapper.getSequence(i)))
			return true;
	}
	//else
	return false;
}

bool  ArabicSentenceBreaker::isBeforeName(int currTok, const NameTheory* nt){
	int nextToken = currTok +1;
	int n_names = nt->getNNameSpans();
	for (int i = 0; i < n_names; i++){
		int start = nt->getNameSpan(i)->start;
		if( start == nextToken ){
				return true;
		}
	}
	return false;
}

void ArabicSentenceBreaker::resetForNewDocument(const Document *doc) {
	_curDoc = doc;
	_cur_sent_no = 0;
	_curRegion = 0;
}

int ArabicSentenceBreaker::getSentencesRaw(Sentence **results, int max_sentences, const Region* const* regions, int num_regions) {
	if (_breakLongSentencesUsingHeuristicRule || _breakLongSentencesUsingStatisticalModel) {
		return getSentencesRawNew(results, max_sentences, regions, num_regions);
	} else {
		return getSentencesRawOld(results, max_sentences, regions, num_regions);
	}
}
int ArabicSentenceBreaker::getSentencesRawNew(Sentence **results, int max_sentences, const Region* const* regions, int num_regions) {
	// First try to find a dateline sentence at the beginning.
//	int text_index = getDatelineSentences(results, max_sentences, text[0]);
	
	int last_headline_sent_no = max_sentences;  // Used to do intelligent dateline processing
	for (int i = 0; i < num_regions; i++) {
		int text_index = 0;
		_curRegion = regions[i];
		LocatedString * textCopy  = _new LocatedString(*_curRegion->getString());

		// These use the generic code, so  specialize it if needed		
		int subs = Tokenizer::removeHarmfulUnicodes(textCopy);
		if (subs > 0) {
			SessionLogger::dbg("unicode") << "removed harmful Unicode " << subs << " chars in region "<<i<<"\n";
		}
		subs = Tokenizer::replaceNonstandardUnicodes(textCopy);
		if (subs > 0) {
			SessionLogger::dbg("unicode") << "replaced non-standard Unicode " << subs << " chars in region "<<i<<"\n";
		}

		// Replace any windows-style newlines (\r\n) with simple \n newlines
		textCopy->replace(L"\r\n", L"\n");

		// Get rid of pesky lines that don't contain any Arabic characters
		ArabicTokenizer::removeLinesWithoutArabicCharacters(textCopy);

		// don't want to just do this on the first region... in case the first region is null...
		if (_cur_sent_no == 0 || (_cur_sent_no-1) == last_headline_sent_no) {
			// First try to find a dateline sentence at the beginning.
			text_index = getDatelineSentences(results, max_sentences, textCopy);
		}
		while ( text_index < textCopy->length()) {
			text_index = getNextSentence(results[_cur_sent_no], text_index, textCopy);
	
			//Cut down on processing time by splitting very long sentences into pieces
			if(_breakLongSentences && results[_cur_sent_no] && (results[_cur_sent_no]->getNChars() > 0)){
				Sentence* thisSentence = results[_cur_sent_no];
				int tokenBreaksCount = countPossibleTokenBreaks(thisSentence->getString());
				if(tokenBreaksCount > _max_token_breaks){
					// debug std::cerr<<"DB: ar_sentBreaker: sent no "<<_cur_sent_no<<" has too many ("<<whiteSpaceCount<<") white spaces\n";
					doTokenizationAndMorphologicalAnalysis(results[_cur_sent_no], _cur_sent_no);
					time_t start_time;
					time(&start_time);
	
					if(tokenBreaksCount > (MAX_SENTENCE_TOKENS -20)){
						SessionLogger::warn("too_many_tokens")<< "Sentence exceeds token limit of "
							<<MAX_SENTENCE_TOKENS<<"; white space ("<<tokenBreaksCount <<")\n";
					}
					
					int origSentLength = thisSentence->getString()->length();
					int numSplitSent = static_cast <int>(ceil((double (tokenBreaksCount))/_max_token_breaks));
					int avgNumChars = origSentLength/numSplitSent; 
					int start = 0;
	
					int currToken = 0;
					int n_sent_tokens = _tokenSequenceWrapper.getNTokens();
					int n_token_since_last_sent = 0;

					while(currToken < n_sent_tokens){
						const Token *token = _tokenSequenceWrapper.getToken(currToken);	
						bool breakNow = false;
						bool conj_break = false;

						if((n_token_since_last_sent > CONJ_SENTENCE_TOKENS_MIN) && // don't break too soon
							((token->getSymbol()==ARABIC_AND_SYMBOL) ||  // "and"
							 (token->getSymbol()==ARABIC_BUT_SYMBOL))){  // "but"
							const PartOfSpeech *poss = _partOfSpeechWrapper.getPOS(currToken+1);

							for (int j=0; j<poss->getNPOS(); j++) {
								Symbol pos = poss->getLabel(j);
								const wchar_t *pos_str = pos.to_string();
								if(!(isMidName(currToken, _nameTheoriesWrapper) && 
									currToken< n_sent_tokens-1 && isMidName(currToken+1, _nameTheoriesWrapper)) &&
									((wcsstr(pos_str, L"VERB")!=NULL) || (wcsstr(pos_str, L"PART")!=NULL))) {
									conj_break = true;
									break;
								}
							}//for
							if(conj_break) { //break the sentence here
								breakNow = true;
								// std::cerr<<"DB-ar_sentBreaker: decided to split before CONJ new current_sent ="<<_cur_sent_no<<
								// 	" and current Token "<< currToken<<"\n";
							}
							
						}//if  (CONJ case)

						// in case we don't find a CONJ we split by length
						if (!breakNow && 
							((n_token_since_last_sent > _max_token_breaks)  || // too many accumulated
							(n_sent_tokens-currToken < LAST_SENTENCE_TOKENS_MIN))){ // too few remaining 

							CharOffset prevTokenEndOff =_tokenSequenceWrapper.getToken(currToken-1)->getEndCharOffset();
							CharOffset currTokenStartOff = token->getStartCharOffset();
							bool separation_between_tokens = (currTokenStartOff > CharOffset(prevTokenEndOff.value()+1));
							int nextToken = currToken+1;
							while (nextToken <n_sent_tokens && !separation_between_tokens){
								prevTokenEndOff = _tokenSequenceWrapper.getToken(nextToken-1)->getEndCharOffset();
								currTokenStartOff = _tokenSequenceWrapper.getToken(nextToken)->getStartCharOffset();
								separation_between_tokens = ( currTokenStartOff > CharOffset(prevTokenEndOff.value() + 1));
								nextToken++;
							}
							currToken = nextToken-1;
							token = _tokenSequenceWrapper.getToken(currToken);
							breakNow = true;
	
						} // end if too many tokens since last split

						if (breakNow){
							int currChar;
							// try to prevent two falvors of small leftover sentences
							if ((conj_break && (n_sent_tokens - currToken < CONJ_LAST_SENTENCE_TOKENS_MIN)) ||
								(!conj_break && (n_sent_tokens - currToken < LAST_SENTENCE_TOKENS_MIN))) {
								currToken = n_sent_tokens-1;
								token = _tokenSequenceWrapper.getToken(currToken);
								currChar = thisSentence->getString()->length();  // open-ended indexing on right hand...
							}
							else {
								CharOffset currStartOffset = token->getStartCharOffset();
								currChar = thisSentence->getString()->positionOfStartOffset(currStartOffset); // break just before next token
							}
							LocatedString* shortSentenceString = thisSentence->getString()->substring(start,currChar);
							results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, shortSentenceString);
							if (_curRegion->getRegionTag() == Symbol(L"HEADLINE")) {
								last_headline_sent_no = _cur_sent_no;
							}
							_cur_sent_no++;


							start = currChar;
							delete shortSentenceString;
							n_token_since_last_sent = 0;
							
						}

						// wrap up 
						currToken++;
						n_token_since_last_sent++;
					}//end while loop over current sentence tokens

					
					if(_cur_sent_no > 0 && 
						results[(_cur_sent_no-1)]->getString()->end<CharOffset>() 
						< thisSentence->getString()->end<CharOffset>())
					{
							int lastStart = start;
							int tCC = origSentLength-1;
							std::stringstream warning;
							warning <<"\tsubstring problem ar_SentBreaker.sentRawNew: last sent (" << _cur_sent_no-1 <<") last offset "<<
								results[(_cur_sent_no-1)]->getString()->end<CharOffset>() 
								<<" full sentence last offset "<<
								thisSentence->getString()->end<CharOffset>()<<"\n";
							//debug std::cerr<<"DB-ar_sentBreaker \tsubstring problem sentRawNew: last sent (" << _cur_sent_no-1 <<") last offset "<<
							//	results[(_cur_sent_no-1)]->getString()->end<CharOffset>() 
							//	<<" full sentence last offset "<<
							//	thisSentence->getString()->end<CharOffset>()<<"\n";

							while((lastStart < origSentLength) && iswspace(thisSentence->getString()->charAt(lastStart))){
								lastStart++;
							}
							while((tCC > lastStart) && iswspace(thisSentence->getString()->charAt(tCC))){
								tCC--;
							}
							if(tCC > lastStart){
								warning <<"**Final Substring from: "<<lastStart<<" to "<<tCC<<"\n";
								// debug std::cerr<<"**Final Substring from: "<<lastStart<<" to "<<tCC<<std::endl;
								LocatedString* shortSentenceString =
									thisSentence->getString()->substring(lastStart);
								results[_cur_sent_no]=
									_new Sentence(_curDoc, _curRegion, _cur_sent_no, shortSentenceString);
								delete shortSentenceString;
								if (_curRegion->getRegionTag() == Symbol(L"HEADLINE")) {
									last_headline_sent_no = _cur_sent_no;
								}
								_cur_sent_no++;
								SessionLogger::warn("substring_problem") << warning.str();								
							}

					}

					// doTokenizationAndMorphologicalAnalysis() may have put objects in these arrays
					for (int i = 0; i < MAX_SUB_SENTENCES_WRAPPER_SIZE; i++) {
						delete _tokenSequenceBuf[i];
						_tokenSequenceBuf[i] = 0;

						delete _partOfSpeechSequenceBuf[i];
						_partOfSpeechSequenceBuf[i] = 0;

						delete _nameTheoryBuf[i];
						_nameTheoryBuf[i] = 0;
					}

					delete thisSentence;
				} // end if too many white space regions
				else{
					if (_curRegion->getRegionTag() == Symbol(L"HEADLINE")) {
						last_headline_sent_no = _cur_sent_no;
					}
					_cur_sent_no++;
				}
			}//end if results from nextSentence  and break long sentences
			else if (results[_cur_sent_no] && (results[_cur_sent_no]->getNChars() > 0)) {
				if (_curRegion->getRegionTag() == Symbol(L"HEADLINE")) {
					last_headline_sent_no = _cur_sent_no;
				}
				_cur_sent_no++;
			}
			if (_cur_sent_no == max_sentences) {
				// TODO : raise error/warning to indicate that input is being ignored
				throw UnexpectedInputException("ArabicSentenceBreaker::getSentences()",
											   "Document contains more than max_sentences sentences");
			}
		}// end while  loop for length of text that calls nextSentence

		text_index = 0;
		delete textCopy;
	}//end for loop over regions

	return _cur_sent_no;
}


void ArabicSentenceBreaker::doTokenizationAndMorphologicalAnalysis(Sentence *sentence, int sent_num) {
	// reset the tokenizer and morphology modules
	_tokenizer->resetForNewSentence(NULL, sentence->getSentNumber());
	_morphAnalysis->resetForNewSentence();
	_morphSelector->resetForNewSentence();
	_posRecognizer->resetForNewSentence();
	_nameRecognizer->resetForNewSentence(sentence);

	_tokenSequenceWrapper.cleanup();
	_partOfSpeechWrapper.cleanup();
	_nameTheoriesWrapper.cleanup();

	const LocatedString *sentenceString = sentence->getString();
	int origSentLength = sentenceString->length();

	// Here we assume MAX_SENTENCE_TOKENS is generously large.  At the time of writing, it was 1000.  Average Arabic word length is 4.7.
	int numSplitSent = static_cast <int>(ceil((double (origSentLength)) / MAX_SENTENCE_TOKENS));
	int avgNumChars = static_cast<int>(floor(static_cast<float>(origSentLength) / numSplitSent));
	int end = 0;
	int subIndex = 0;
	while(end < origSentLength && (subIndex < MAX_SUB_SENTENCES_WRAPPER_SIZE)) {
		int start = end;
		end += avgNumChars;
		if(end > origSentLength)
			end = origSentLength;
		while(end < origSentLength && !iswspace(sentenceString->charAt(end)))
			end++;

		// tokenize
		LocatedString *subString = sentenceString->substring(start, end);
		_tokenizer->getTokenTheories(_tokenSequenceBuf+subIndex, 1, subString);
		delete subString;

		// If there are no tokens in subString (eg if subString just contains 
		// whitespace), then skip it.  This can happen for trailing whitespace.
		if (_tokenSequenceBuf[subIndex]->getNTokens() == 0)
			continue;

		// select morphology
		_morphAnalysis->getMorphTheories(_tokenSequenceBuf[subIndex]);
		_morphSelector->selectTokenization(sentenceString, _tokenSequenceBuf[subIndex]);

		// get POS
		_posRecognizer->getPartOfSpeechTheories(
						_partOfSpeechSequenceBuf+subIndex, 1,
						_tokenSequenceBuf[subIndex]);
		// get names
		_nameRecognizer->getNameTheoriesForSentenceBreaking(
						_nameTheoryBuf+subIndex, 1,
						_tokenSequenceBuf[subIndex]);

		// add to wrappers (this has to be done after the selectTokenization occurred)
		_tokenSequenceWrapper.addSequence(_tokenSequenceBuf[subIndex]);
		_partOfSpeechWrapper.addSequence(_partOfSpeechSequenceBuf[subIndex]);
		_nameTheoriesWrapper.addSequence(_nameTheoryBuf[subIndex],
			_tokenSequenceBuf[subIndex]->getNTokens());

		subIndex++;
	}//while
	if(subIndex==MAX_SUB_SENTENCES_WRAPPER_SIZE) {
		SessionLogger::warn("max_sub_sentences_wrapper_size")
			<< "ArabicSentenceBreaker::doTokenization(), "
			<< "possible loss of sentences, due to primal sentence longer than "
			<< MAX_SUB_SENTENCES_WRAPPER_SIZE;
	}
}


int ArabicSentenceBreaker::getSentencesRawOld(Sentence **results, int max_sentences, const Region* const* regions, int num_regions) {
	// First try to find a dateline sentence at the beginning.
	int text_index = getDatelineSentences(results, max_sentences, regions[0]->getString());
	//std::cout<<"Get Sentences "<<std::endl;
	for (int i = 0; i < num_regions; i++) {
		_curRegion = regions[i];
		LocatedString *textCopy = _new LocatedString(*_curRegion->getString());

		// Replace any windows-style newlines (\r\n) with simple \n newlines
		textCopy->replace(L"\r\n", L"\n");

		while ( text_index < textCopy->length()) {
			text_index = getNextSentence(results[_cur_sent_no], text_index, textCopy);
			//Cut down on processing time by splitting very long sentences into pieces

			if(results[_cur_sent_no] && _breakLongSentences){
				SessionLogger &logger = *SessionLogger::logger;
				Sentence* thisSentence = results[_cur_sent_no];
				int tokenBreaksCount = countPossibleTokenBreaks(thisSentence->getString());
				if(tokenBreaksCount > _max_token_breaks){
					time_t start_time;
					time(&start_time);
					SessionLogger::LogMessageMaker warning = logger.reportWarning();
					warning
						<< "ArabicSentenceBreaker::getSentencesRawOld(),"
						<< "ARTIFICIALLY BREAKING A SENTENCE- \n"; 
					//std::cerr<<"ARTIFICIALLY BREAKING A SENTENCE- "<<std::endl;
					if(tokenBreaksCount > (MAX_SENTENCE_TOKENS -20)){
						logger.reportError()<< "ar_SentenceBreaker-old- input has more than MAX_SENTENCE_TOKENS ("
							<<MAX_SENTENCE_TOKENS<<") white space ("<<tokenBreaksCount <<")\n";
					}
					//logger<<"Sentence: \n"<<thisSentence->getString()->toString()<<"\n";
					//disable this for now need morph analysis to find names!

					/* Just break at a white space point
					*/
					
					int origSentLength = thisSentence->getString()->length();
					int numSplitSent = static_cast <int>(ceil((double (tokenBreaksCount))/_max_token_breaks));
					int avgNumChars = origSentLength/numSplitSent; 
					int currChar =avgNumChars;
					int start = 0;
					warning << "Total Characters "<<origSentLength<<"\n";
					warning << "AVG Characters "<<avgNumChars<<"\n";


					while((currChar < origSentLength) && (_cur_sent_no < max_sentences) ){
						//if at white space, get the subsentence
						if(iswspace(thisSentence->getString()->charAt(currChar))){
							warning<<"---Substring from: "<<start<<" to "<<currChar<<"\n";
							//std::cerr<<"---Substring from: "<<start<<" to "<<currChar<<std::endl;
							LocatedString* shortSentenceString =
								thisSentence->getString()->substring(start,currChar);
							results[_cur_sent_no]=
								_new Sentence(_curDoc, _curRegion, _cur_sent_no, shortSentenceString);
							_cur_sent_no++;
							start = currChar+1;
							delete shortSentenceString;
							//if there are more subsentences to be found
							if(currChar + avgNumChars < origSentLength){
									currChar +=avgNumChars;
									warning<<"\tcurrChar moved to: "<<currChar<<"\n";
									//std::cout<<"\tcurrChar moved to: "<<currChar<<std::endl;
							}
							//otherwise put the remaining text in a subsentence
							else if(start <origSentLength){
								shortSentenceString =
									thisSentence->getString()->substring(start);
								warning<<"Final Substring from: "<<start<<" to end"<<"\n";
								//std::cerr<<"Final Substring from: "<<start<<" to end"<<std::endl;

								results[_cur_sent_no]=
									_new Sentence(_curDoc, _curRegion, _cur_sent_no, shortSentenceString);
								_cur_sent_no++;
								currChar = origSentLength+1;
								start = origSentLength+1;

								delete shortSentenceString;
							}
							else{
								currChar++;
							}
						}
						else{
							currChar++;
						}
					}// end while chopper loop
					
					if(_cur_sent_no > 0 &&
						results[(_cur_sent_no-1)]->getString()->end<CharOffset>() 
						< thisSentence->getString()->end<CharOffset>())
					{
							int lastStart = start;
							int tCC = currChar-1;
							warning<<"\tsubstring problem sentRawOld: last sent ("<< _cur_sent_no-1<<") last offset "<<
								results[(_cur_sent_no-1)]->getString()->end<CharOffset>() 
								<<" full sentence last offset "<<
								thisSentence->getString()->end<CharOffset>()<<"\n";
							//std::cout<<"\tsubstring problem: last sent last offset"<<
							//	results[(_cur_sent_no-1)]->getString()->end<CharOffset>() 
							//	<<" full sentence last offset"<<
							//	thisSentence->getString()->end<CharOffset>()<<std::endl;

							//int lastStart = results[(_cur_sent_no-1)]->getString()->end<CharOffset>() +1;
							
							while((lastStart < origSentLength) && iswspace(thisSentence->getString()->charAt(lastStart))){
								lastStart++;
							}
							while((tCC > lastStart) && iswspace(thisSentence->getString()->charAt(tCC))){
								tCC--;
							}
							if(tCC > lastStart){
								warning<<"**Final Substring from: "<<lastStart<<" to "<<tCC<<"\n";
								//std::cerr<<"**Final Substring from: "<<lastStart<<" to "<<tCC<<std::endl;
								LocatedString* shortSentenceString =
									thisSentence->getString()->substring(lastStart);
								results[_cur_sent_no]=
									_new Sentence(_curDoc, _curRegion, _cur_sent_no, shortSentenceString);
								_cur_sent_no++;
							}

					}


					delete thisSentence;
				}
				else{
					_cur_sent_no++;
				}
			}
			else if(results[_cur_sent_no])
				_cur_sent_no++;
			if (_cur_sent_no == max_sentences) {
				// TODO : raise error/warning to indicate that input is being ignored
				throw UnexpectedInputException("ArabicSentenceBreaker::getSentences()",
											   "Document contains more than max_sentences sentences");
			}
		}

		text_index = 0;
		delete textCopy;
	}
	return _cur_sent_no;
}

// modified to add whitespace around ellipsis, so now it works on a copy of LocatedString
int ArabicSentenceBreaker::getNextSentence(Sentence* &sentence, int offset, LocatedString *input) {
	int len = input->length();
	int i = offset;
	//get rid of initial white space
	while(i<len &&iswspace(input->charAt(i))){
		i++;
	}
	int start = i;
	if(start>=len){
		sentence = NULL;
		return start;
	}
	while(i < input->length()){
		wchar_t c = input->charAt(i);
		
		if(c == EOQ_MARK || c == EOS_MARK){
			int endElipse;
			int endAbbrev;
			i++;
			if( (i < input->length()) && (input->charAt(i) == L'"')){	//" break after the quotation mark
				i++;
				break;
			}else if(c == EOS_MARK){
				//TODO: What if abbrev ends sentence?
				if (isElipsePer(input,i,endElipse)){
					if ((i > 1) && !iswspace(input->charAt(i-2))){
						// break ellipsis start from token
						//std::cerr<<"ellipsis adding start blank at "<<i-1<<"\n";
						input->insert(L" ", i-1);
						endElipse++; 
						i++;
					}
					if ((endElipse - i) >= ELLIPSIS_BREAKS) {
						// a really long one is treated as ellipsis followed by period
						//std::cerr<<"ellipsis treating last  period as EOS at "<<endElipse-1<<"\n";
						endElipse--; // so we move the end back to leave last period
					}
					if ((endElipse < input->length()) && !iswspace(input->charAt(endElipse))) {
						// break ellipsis end from token
						//std::cerr<<"ellipsis adding end blank at "<<endElipse<<"\n";
						input->insert(L" ", endElipse);
						endElipse++;
					}
					i = endElipse;
				}
				else if(isAbrevPer(input, i, endAbbrev)){
					i = endAbbrev;
				}
				else{	//must be end of Sentence
					break;
				}
			}
			else
				break;
		}
		else if( c == NEW_LINE || c == CARRIAGE_RETURN){
			i++;
			if( i < input->length()){
				c = input->charAt(i);
				if( c == NEW_LINE || c == CARRIAGE_RETURN){
					i++;
					break;
				}
			}
		}

		else{
			i++;
		}
	}

	// remove any trailing white space at end of doc
	if (i >= input->length()) {
		i = input->length();
		while (iswspace(input->charAt(i - 1)))
			i--;
	}
	LocatedString *sentenceString = input->substring(start, i);

	sentence = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sentenceString);
	delete sentenceString;
	//next start will be i;  i has been moved to mark next start
	//return i+1;
	return i;
}


// TODO: handle incomplete sentence input without period - use newlines?
/*
int ArabicSentenceBreaker::getNextSentence(Sentence* &sentence, int offset, LocatedString const *input) {

	wchar_t _buffer[getMaxSentenceChars()+1];
	int i = 0;
	while (true) {
		_buffer[i++] = input->charAt(offset + i);
		// check for eos or sentence limit to end
		if (_buffer[i-1] == EOS_MARK || _buffer[i-1] == EOQ_MARK || i >= getMaxSentenceChars() || (offset+i) >= input->length()) {
			//break if you're at the end
			if (i >= getMaxSentenceChars() || (offset+i) >= input->length()) {
				throw UnexpectedInputException("ArabicSentenceBreaker::getNextSentence()",
											   "Sentence contains more than max_sentences sentences");
				_buffer[i] = L'\0';
				break;
			}
			int endElipse;
			int endAbbrev;
			//TODO: if endElipse is after getMaxSentenceChars()!
			if(isElipsePer(input,offset+i,endElipse)){
				if(
				for(i; i<=(endElipse-offset); i++){
					_buffer[i] = input->charAt(offset+i);
				}
			}
			else if(isAbrevPer(input, offset+i, endAbbrev)){
				for(i; i<=(endAbbrev-offset); i++){
					_buffer[i] = input->charAt(offset+i);
				}
			}
			else if(isNumSeparatorPer(input, offset+i)){
				;
			}
			else{
				_buffer[i] = L'\0';
				break;
			}
		}
	}
	sentence = _new Sentence(_curDoc, _curRegion, _cur_sent_no, input->substring(offset, offset+i));
	return i;
}
*/
/*  return true if there  is at least one period contiguously following pos
    if true, newpos should point past  the ellipsis
*/
bool ArabicSentenceBreaker::isElipsePer(LocatedString const  *input, int pos, int& newpos){
	//input[pos-1] was a period
	newpos = pos;
	while ((newpos < input->length()) &&
			(input->charAt(newpos) == EOS_MARK)){
		newpos++;
	}
	bool elip = (newpos >= (pos + ELLIPSIS_MIN_DOTS -1));
	//if (elip) std::cerr<<"found elipsis from "<<pos-1 << " to " << newpos-1 <<"\n";
	return elip;
}
/*
return 0 if '.' is not a part of abbreviation
otherwise, return the position where the abbreviation ends.
PROBLEM: If a sentence ends in an abbreviation, the end of sentence will not be found
*/
bool ArabicSentenceBreaker::isAbrevPer(LocatedString const  *input, int pos, int& newpos){
	//input[pos - 1] was a period,
	//check to see if word(s) following this also end with periods
	newpos = pos;
	int peekpos = pos;
	bool abbrev = false;
	while(peekpos < input->length()){
		//get space before next word
		while(peekpos < input->length()&&(input->charAt(peekpos) == L' ')){
			peekpos++;
		}
		//read til '.' or ' '
		while(peekpos < input->length()){
			if(input->charAt(peekpos) == L' '){
				abbrev = false;
				break;
			}
			else if(input->charAt(peekpos) == EOS_MARK){
				abbrev = true;
				//next char may be space
                peekpos++;
                if(peekpos < input->length() && input->charAt(peekpos)==L' ') {
					peekpos++;
                }
				newpos = peekpos;
				break;
			}
			else{
				peekpos++;
			}
		}
		if(!abbrev){
			return newpos !=pos;
		}
	}
	return newpos != pos;
}

bool ArabicSentenceBreaker::isNumSeparatorPer(LocatedString const  *input, int pos){
	//pos -1  was a period
	//d.d if number separator period
	return iswdigit(input->charAt(pos-2))&&iswdigit(input->charAt(pos));
}

/**
 * @param results an output parameter containing an array of
 *                pointers to Sentences that will be filled in
 *                with the sentences found by the sentence breaker.
 * @param input the input string to read from.
 * @return the next index into the located string after the end of the
 *         found dateline sentences, or <code>0</code> if no dateline
 *         sentences were found.
 */

int ArabicSentenceBreaker::getDatelineSentences(Sentence **results, int max_sentences, const LocatedString *input) {
	int start = 0;
	int end = start;
	if (BREAK_DATELINES && (USE_AFP_DATELINE ||  USE_XINHUA_DATELINE)) {
		int pos = -1;
		//AFP date line as seen in treebank have format ....( Af b ) -,
		int nchars_in_news_source = 0;
		if (USE_AFP_DATELINE) {
			pos = input->indexOf(ArabicSymbol(L"Af b").to_string());
			if (pos != -1)
				nchars_in_news_source = 4;
			if(pos < 0){
				pos = input->indexOf(ArabicSymbol(L"> f b").to_string());
				if (pos != -1)
					nchars_in_news_source = 5;
			}
			if(pos < 0){
				pos = input->indexOf(ArabicSymbol(L">f b").to_string());
				if (pos != -1)
					nchars_in_news_source = 4;
			}
			if ((pos > 0) && (pos < 60)) {
				end = pos;
				pos = input->indexOf(L"-", end);
				if( (pos > end)  && (pos < 60) ){
					end = pos;
				}
				end = end+1;
				LocatedString *sub = input->substring(start, end);
				results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sub);
				if (_breakLongSentences) {
					attemptLongSentenceBreak(results, max_sentences);
				}
				delete sub;
				_cur_sent_no++;
			}
		}
		if (pos==-1 && USE_XINHUA_DATELINE) {
			pos = input->indexOf(ArabicSymbol(L"/$ynxwA/").to_string());
			if (pos == -1) {
				pos = input->indexOf(ArabicSentenceBreaker::ARABIC_XINHUA_SYMBOL.to_string());
			}
			if ((pos > 0) && (pos < 60)) {
				end = pos+8;
				LocatedString *sub = input->substring(start, end);
				results[_cur_sent_no] = _new Sentence(_curDoc, _curRegion, _cur_sent_no, sub);
				if (_breakLongSentences) {
					attemptLongSentenceBreak(results, max_sentences);
				}
				delete sub;
				_cur_sent_no++;
			}
		}
	}
	return end;
}




ArabicSentenceBreaker::TokenSequencesWrapper::TokenSequencesWrapper() {}

ArabicSentenceBreaker::TokenSequencesWrapper::~TokenSequencesWrapper(){}

void ArabicSentenceBreaker::TokenSequencesWrapper::addSequence(TokenSequence * sequence) {
	_sequences.push_back(sequence);
	_n_tokens_in_sequence.push_back(sequence->getNTokens());
	_n_sequences++;
	_n_tokens += sequence->getNTokens();
}
const Token* ArabicSentenceBreaker::TokenSequencesWrapper::getToken(int pos){
	if(pos > _n_tokens || pos < 0)
		throw InternalInconsistencyException("TokenSequencesWrapper::getToken()", "out of range");
	int sum_tokens = 0;
	for( int i=0; i< _n_sequences; i++) {
		if(sum_tokens+_n_tokens_in_sequence[i] > pos)
			return _sequences[i]->getToken(pos-sum_tokens);
		sum_tokens += _n_tokens_in_sequence[i];
	}
	throw InternalInconsistencyException("TokenSequencesWrapper::getToken()", "out of range");
}

void ArabicSentenceBreaker::TokenSequencesWrapper::cleanup() {
	_sequences.clear();
	_n_tokens_in_sequence.clear();
	_n_sequences = 0;
	_n_tokens = 0;	
}

int ArabicSentenceBreaker::TokenSequencesWrapper::getTokenFromOffset(const LocatedString* source, int offset) {
	int n_tokens = 0;
	for (int i=0; i<_n_sequences; i++) {
		int answer = ArabicSentenceBreaker::getTokenFromOffset(source, _sequences[i], offset,  0);
		if (answer != -1)
			return n_tokens+answer;
		else
			n_tokens += _n_tokens_in_sequence[i];
	}
	return -1;
}

ArabicSentenceBreaker::POSSequencesWrapper::POSSequencesWrapper() {}

ArabicSentenceBreaker::POSSequencesWrapper::~POSSequencesWrapper() {}

void ArabicSentenceBreaker::POSSequencesWrapper::addSequence(PartOfSpeechSequence * sequence) {
	_sequences.push_back(sequence);
	_n_tokens_in_sequence.push_back(sequence->getNTokens());
	_n_sequences++;
	_n_tokens += sequence->getNTokens();
}
const PartOfSpeech* ArabicSentenceBreaker::POSSequencesWrapper::getPOS(int pos){
	if(pos > _n_tokens || pos < 0)
		throw InternalInconsistencyException("PartOfSpeechSequence::getToken()", "out of range");
	int sum_tokens = 0;
	for( int i=0; i< _n_sequences; i++) {
		if(sum_tokens+_n_tokens_in_sequence[i] > pos)
			return static_cast<const PartOfSpeech *>(_sequences[i]->getPOS(pos-sum_tokens));
		sum_tokens += _n_tokens_in_sequence[i];
	}
	throw InternalInconsistencyException("PartOfSpeechSequence::getToken()", "out of range");
}
void ArabicSentenceBreaker::POSSequencesWrapper::cleanup() {
	_sequences.clear();
	_n_tokens_in_sequence.clear();
	_n_sequences = 0;
	_n_tokens = 0;
}

ArabicSentenceBreaker::NameTheoriesWrapper::NameTheoriesWrapper() {}
ArabicSentenceBreaker::NameTheoriesWrapper::~NameTheoriesWrapper() {}
void ArabicSentenceBreaker::NameTheoriesWrapper::addSequence(NameTheory * sequence, int n_tokens) {
	_sequences.push_back(sequence);
	_n_tokens_in_sequence.push_back(n_tokens);
	_n_sequences++;
	_n_tokens += n_tokens;
}
const NameTheory * ArabicSentenceBreaker::NameTheoriesWrapper::getSequence(int pos) {
	if(pos > _n_sequences)
		throw InternalInconsistencyException("NameSequencesWrapper::addSequence()", "out of range");
	return _sequences[pos]; 
}
void ArabicSentenceBreaker::NameTheoriesWrapper::cleanup() {
	_sequences.clear();
	_n_tokens_in_sequence.clear();
	_n_sequences = 0;
	_n_tokens = 0;
}
