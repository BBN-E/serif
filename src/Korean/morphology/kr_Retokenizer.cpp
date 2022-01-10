// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/Lexicon.h"
#include "Generic/theories/LexicalEntry.h"
#include "Generic/morphAnalysis/SessionLexicon.h"
#include "Korean/morphSelection/kr_Retokenizer.h"

void KoreanRetokenizer::KoreanRetokenizer() {
	lex = SessionLexicon::getInstance().getLexicon();
	reset();
}

void KoreanRetokenizer::reset() {
	for (int i = 0; i < MAX_SENTENCE_TOKENS; i++) {
		newTokenLexEntryCounts[i] = 0;
	}
}

void KoreanRetokenizer::Retokenize(TokenSequence *origTS, Token** newTokens, int n_new) {
	int nOrigTok = origTS->getNTokens();
	SessionLogger *logger = SessionLogger::logger;

	if (n_new < nOrigTok) {
		_snprintf(error_message_buffer, ERROR_MESSAGE_SIZE, "%s (%d) %s (%d)",
				"Retokenize(): Number of New Tokens", n_new,
				" Less than Number of Original Tokens", nOrigTok);

		logger->beginError();
		(*logger) << error_message_buffer << "\n";
		(*logger) << "Old Token Sequence: \n";
		for (int i = 0; i < nOrigTok; i++)
			(*logger) << origTS->getToken(i)->getSymbol().to_string() << " ";
		(*logger) << "\n";

		throw UnrecoverableException("KoreanRetokenizer::Retokenize()", error_message_buffer);
	}

	int startPrevTok = 0;
	int origTokCount = 0;
	int newTokCount = 0;
	int currNewTok = 0;
	int startNewTok = 0;
	
	while (currNewTok < n_new) {
		dbg = false;
		origTokCount = 0;
		newTokCount = 0;
		int orig_tok_ind = newTokens[currNewTok]->getOriginalTokenIndex();
		
		// get the previous set of tokens this token refers to
		origTokCount = getCorrespondingTokens(prevTokenBuffer, origTS, 
												orig_tok_ind, startPrevTok);
		
		if (origTokCount == 0) {
			_snprintf(error_message_buffer, ERROR_MESSAGE_SIZE,
				"Retokenize(): No Original Tokens Match the new Token! Token: %s OrigID: %d", 
				newTokens[currNewTok]->getSymbol().to_debug_string(), 
				newTokens[currNewTok]->getOriginalTokenIndex()); 

			logger->beginWarning();
			(*logger) << error_message_buffer << "\n";
			(*logger) << "Old Token Sequence: \n";
			for (int i = 0; i < nOrigTok; i++)
				(*logger) << origTS->getToken(i)->getSymbol().to_string() << " ";
			(*logger) << "\n";
			
			throw UnrecoverableException("KoreanRetokenizer::Retokenize()", error_message_buffer);
		}

		startPrevTok += origTokCount;
		startNewTok = currNewTok;
		//get all of the new tokens that refer back to this set
		newTokCount = getCorrespondingTokens(newTokenBuffer, newTokens, 
											  orig_tok_ind, currNewTok, n_new);
		currNewTok += newTokCount;

		int nPrevLE = prevTokenBuffer[0]->getNLexicalEntries();

		// special case, one to one mapping, lexical entries will be the same
		if ((origTokCount == 1) && (newTokCount == 1)) {
			if (dbg) {
				std::cout << "Retokenize Original Token: " << prevTokenBuffer[0]->getSymbol().to_debug_string() <<
					" with New Token: " << newTokenBuffer[0]->getSymbol().to_debug_string() << " - Identical "
					<< std::endl;
			}
			for (int j = 0; j < nPrevLE; j++) {
				newTokenLexEntryBuffer[startNewTok][j] = prevTokenBuffer[0]->getLexicalEntry(j);
				newTokenLexEntryCounts[startNewTok]++;
			}
		}				
		// align lexical entries
		else if (origTokCount == 1) {
			if (dbg) {
				std::cout << "Retokenize Original Token: " << prevTokenBuffer[0]->getSymbol().to_debug_string()
					<< " with New Tokens: ";
				for (int z = 0; z < newTokCount; z++) 
					std::cout << newTokenBuffer[z]->getSymbol().to_debug_string() << " ";
				std::cout << std::endl;
				std::cout << "Num old lex entries: " << nPrevLE << std::endl;
			}
			
			for (int j = 0; j < nPrevLE; j++) {
				alignLexEntry(prevTokenBuffer[0]->getLexicalEntry(j), startNewTok, newTokCount);
			}
		}
		else if (origTokCount == newTokCount) { // this takes care of the names case
			for (int i = 0; i < origTokCount; i++) {
                nPrevLE = prevTokenBuffer[i]->getNLexicalEntries();
				for (int j = 0; j < nPrevLE; j++) {
					newTokenLexEntryBuffer[startNewTok+i][j] = prevTokenBuffer[i]->getLexicalEntry(j);
					newTokenLexEntryCounts[startNewTok+i]++;
				}
			}
		}
		else { // this should never happen, throw an exception
			if (dbg) {
				std::cout << "Orig Count = " << origTokCount << " OrigInd: " << orig_tok_ind << " Skipping Lex entries for: " << std::endl;
				for (int z = 0; z < newTokCount; z++) {
					std::cout << newTokenBuffer[z]->getSymbol().to_debug_string() << std::endl;
				}
			}
			_snprintf(error_message_buffer, ERROR_MESSAGE_SIZE, 
				"More than 1 original token! OrigTokCount: %d OrigTokInd: %d",
				origTokCount, orig_tok_ind);
			throw UnrecoverableException("KoreanRetokenizer::Retokenize()", error_message_buffer);
		}

	}
	LexicalEntry* temp_buffer[MAX_LEX_ENTRIES];
	for (int i = 0; i < n_new; i++) {
		Token* t = newTokens[i];
		for (int j = 0; j < newTokenLexEntryCounts[i]; j++)
			temp_buffer[j] = newTokenLexEntryBuffer[i][j];
		if (dbg) {
			std::cout << "KoreanRetokenizer add Token " << i << ": " << t->getStartOffset()
				<< " " << t->getEndOffset()
				<< " " << t->getSymbol().to_debug_string()
				<< " " << newTokenLexEntryCounts[i] << std::endl;
		}
		final_token_buffer[i] = _new Token(t->getStartOffset(), t->getEndOffset(), 
			t->getOriginalTokenIndex(), t->getSymbol(), newTokenLexEntryCounts[i], 
			temp_buffer);
	}
	origTS->retokenize(n_new, final_token_buffer);			
	
	// clean up count chart
	for (int j = 0; j < n_new; j++) {
		newTokenLexEntryCounts[j] = 0;
	}
}

void KoreanRetokenizer::RetokenizeForNames(TokenSequence *origTS, Token** newTokens, int n_new) {

/*  THIS IS THE UNCHANGED ARABIC VERSION

	int nOrigTok = origTS->getNTokens();
	SessionLogger *logger = SessionLogger::logger;

	if (n_new < nOrigTok) {
		_snprintf(_error_message_buffer, ERROR_MESSAGE_SIZE, "%s (%d) %s (%d)",
				"RetokenizeForNames(): Number of New Tokens", n_new,
				" Less than Number of Original Tokens", nOrigTok);

		logger->beginError();
		(*logger) << _error_message_buffer << "\n";
		(*logger) << "Old Token Sequence: \n";
		for (int i = 0; i < nOrigTok; i++)
			(*logger) << origTS->getToken(i)->getSymbol().to_string() << " ";
		(*logger) << "\n";

		throw UnrecoverableException("KoreanRetokenizer::RetokenizeForNames()", _error_message_buffer);
	}

	int startPrevTok = 0;
	int origTokCount = 0;
	int newTokCount = 0;
	int currNewTok = 0;
	int startNewTok = 0;

	while (currNewTok < n_new) {
		origTokCount = 0;
		newTokCount = 0;

		if (_dbg) {
			std::cout << "Current Token: " << currNewTok << " Total New Tokens: " << n_new << std::endl;
		}
		int orig_tok_ind = newTokens[currNewTok]->getOriginalTokenIndex();
		//get the previous set of tokens this token refers to
		origTokCount = _getCorrespondingTokens(_prevTokenBuffer, origTS, 
												orig_tok_ind, startPrevTok);
		if (origTokCount == 0)	{
			_snprintf(_error_message_buffer, ERROR_MESSAGE_SIZE,
				"RetokenizeForNames(): No Original Tokens Match the new Token! Token: %s OrigID: %d",
				newTokens[currNewTok]->getSymbol().to_debug_string(), 
				newTokens[currNewTok]->getOriginalTokenIndex()); 

			logger->beginError();
			(*logger) << _error_message_buffer << "\n";
			(*logger) << "Old Token Sequence: \n";
			for (int i = 0; i < nOrigTok; i++)
				(*logger) << origTS->getToken(i)->getSymbol().to_string() << " ";
			(*logger) << "\n";

			throw UnrecoverableException("KoreanRetokenizer::RetokenizeForNames()", _error_message_buffer);
		}
		startPrevTok += origTokCount;
		
		// get all of the new tokens that refer back to this set
		newTokCount = _getCorrespondingTokens(_newTokenBuffer, newTokens, 
											orig_tok_ind, currNewTok, n_new);

		if ((origTokCount == 1) && (newTokCount == 1)) {
			if (_dbg) {
				std::cout << "Retokenize Original Token: " << _prevTokenBuffer[0]->getSymbol().to_debug_string()
				<< " with New Token: " << _newTokenBuffer[0]->getSymbol().to_debug_string() << " - Identical "
				<< std::endl;
			}
			int nPrevLE = _prevTokenBuffer[0]->getNLexicalEntries();
			for (int j = 0; j < nPrevLE; j++) {
				_newTokenLexEntryBuffer[currNewTok][j] = _prevTokenBuffer[0]->getLexicalEntry(j);
				_newTokenLexEntryCounts[currNewTok]++;
			}
		}
		else if ((origTokCount == 1) && (newTokCount ==2)) {
			int nPrevLE = _prevTokenBuffer[0]->getNLexicalEntries();
			Symbol tok1 = newTokens[currNewTok]->getSymbol();
			const wchar_t* tok1Str = tok1.to_string();
			Symbol tok2 = newTokens[currNewTok+1]->getSymbol();
			const wchar_t* tok2Str = tok1.to_string();
			if (_dbg) {
				std::cout << "Retokenize Original Token: " << _prevTokenBuffer[0]->getSymbol().to_debug_string()
					<< " with New Tokens: " << tok1.to_debug_string() << " " << tok2.to_debug_string()
					<< std::endl;			
				std::cout << "Num old lex entries: " << nPrevLE << std::endl;
			}
			LexicalEntry* matched[MAX_LEX_ENTRIES];
			LexicalEntry* addToTokenBuffer[MAX_LEX_ENTRIES];
			//prevent overflow, ignore lex entries above MAX
			int max = nPrevLE;
			if (max > MAX_LEX_ENTRIES) {
				max = MAX_LEX_ENTRIES;
				// To do, log skipping in session logger!
			}
			int num_matched = 0;
			for (int j = 0; j < max; j++) {
				LexicalEntry* prevLE = _prevTokenBuffer[0]->getLexicalEntry(j);
				int numLe = BuckwalterFunctions::pullLexEntriesUp(_prevLexEntryBuffer, MAX_LEX_ENTRIES, prevLE);
				if (numLe < 2) 
					continue;
				LexicalEntry* first_le = _prevLexEntryBuffer[0];
				// since we only want clitics, ignore lex entries whose sub parts aren't 0
				if (first_le->getNSegments() != 0) 
					continue;
				Symbol lenv = first_le->getKey();
				const wchar_t* leStr = lenv.to_string();
				
				int new_place = _matchStringInToken(leStr, tok1Str, 0);
				int len = static_cast<int>(wcslen(leStr));

				if (new_place == len) {
					matched[num_matched++] = prevLE;
				}
			}
			if (_dbg) {
				std::cout << "Num Matched: " << num_matched << std::endl;
				std::cout << "Pt1-Num Lex Entries for token 0: " << _newTokenLexEntryCounts[0] << std::endl;
			}
			if (num_matched > 0) {
				for (int i = 0; i < num_matched; i++) {
					LexicalEntry* le = matched[i];
					int numLe = BuckwalterFunctions::pullLexEntriesUp(
						_prevLexEntryBuffer, MAX_LEX_ENTRIES, le);
					_newTokenLexEntryBuffer[currNewTok][i] = _prevLexEntryBuffer[0];
					_newTokenLexEntryCounts[currNewTok]++;
					if (_dbg) {
						std::cout << "Tok: " << currNewTok << " lex entry: "
							<< static_cast<int>(_prevTokenBuffer[0]->getLexicalEntry(i)->getID())
							<< std::endl;
							<< "Number of Lexcial Entries: " << numLe << std::endl;
					}
					if (numLe == 2) {
						_newTokenLexEntryBuffer[currNewTok+1][i] = _prevLexEntryBuffer[1];
						_newTokenLexEntryCounts[currNewTok+1]++;
					}
					else {
						for (int j = 1; j < numLe; j++) {
							addToTokenBuffer[j-1] = _prevLexEntryBuffer[j];
						}
						size_t id;
						std::cout << "Pt2-Num Lex Entries for token 0: " << _newTokenLexEntryCounts[0] << std::endl;

						bool found = _matchLexEntry(tok2, addToTokenBuffer, numLe-1, id);
						std::cout << "Pt3-Num Lex Entries for token 0: " << _newTokenLexEntryCounts[0] << std::endl;

						if (!found) {
							id = _addLexEntry(tok2, addToTokenBuffer,numLe-1);
						}
						std::cout << "Pt4-Num Lex Entries for token 0: " << _newTokenLexEntryCounts[0] << std::endl;

						int count = _newTokenLexEntryCounts[currNewTok+1];
						_newTokenLexEntryBuffer[currNewTok+1][count] = _lex->getEntryByID(id);
						_newTokenLexEntryCounts[currNewTok+1]++;
					}
				}
			}
			else{

				// match entries
				LexicalEntry** results = _tempLexEntryBuffer;			
				int nres = _lex->getEntriesByKey(tok1, results, MAX_LEX_ENTRIES);
				// maybe should prune for only clitics here
				int j = 0;
				for (int i = 0; i < nres; i++) {
					// since we only want clitics, ignore lex entries whose sub parts aren't 0
					if (results[i]->getNSegments() != 0) 
						continue;
					
					LexicalEntry* temp = results[i];
					_newTokenLexEntryBuffer[currNewTok][j] = results[i];
					_newTokenLexEntryCounts[currNewTok]++;
					j++;
				}
				size_t id = _addLexEntry(tok2, 0, 0);
				_newTokenLexEntryBuffer[currNewTok+1][0] = _lex->getEntryByID(id);
				_newTokenLexEntryCounts[currNewTok+1]++;
			}
		}
		else {
			std::cout << "Old Token Sequence: " << std::endl;
			origTS->dump(std::cout);
			std::cout << std::endl;

			_snprintf(_error_message_buffer,ERROR_MESSAGE_SIZE,
				"RetokenizeForNames() Wrong Token Counts: orig: %d new: %d -  new token: %s, original index: %d", 
				origTokCount, newTokCount, 
				newTokens[currNewTok]->getSymbol().to_debug_string(), 
				newTokens[currNewTok]->getOriginalTokenIndex());
			throw UnrecoverableException("KoreanRetokenizer::RetokenizeForNames()", _error_message_buffer);
		}
		if (_dbg) {
			std::cout << "Num Lex Entries for token 0: " << _newTokenLexEntryCounts[0] 
				<< std::endl << "Num lex entries for curr token- " << currNewTok << ": "
				<< _newTokenLexEntryCounts[0] << std::endl;
		}
		currNewTok += newTokCount;
	}
	LexicalEntry* temp_buffer[MAX_LEX_ENTRIES];
	if (_dbg) {
		std::cout << "Put New Tokens in Temp Buffer" << std::endl;
	}
	for (int i = 0; i < n_new; i++) {
		Token* t = newTokens[i];
		if (_dbg) {
			std::cout << "Get Lex Entries for Token: " << i << " Num Lex Entries: "
				<< _newTokenLexEntryCounts[i] << std::endl;
		}
		for (int j = 0; j < _newTokenLexEntryCounts[i]; j++) {
			//if(_dbg){
			//	std::cout<<"Lex Entry: "<<j<<std::endl;
			//}
			temp_buffer[j] = _newTokenLexEntryBuffer[i][j];
		}
		if (_dbg) {
			std::cout << "KoreanRetokenizer add Token " << i << ": " << t->getStartOffset() 
				<< " " << t->getEndOffset() << " " << t->getSymbol().to_debug_string()
				<< " " << _newTokenLexEntryCounts[i] << std::endl;
		}
		_final_token_buffer[i] = _new Token(t->getStartOffset(), t->getEndOffset(), 
			t->getOriginalTokenIndex(), t->getSymbol(), _newTokenLexEntryCounts[i], 
			temp_buffer);
	}
	origTS->retokenize(n_new, _final_token_buffer);	
	// clean up count chart
	for (int j = 0; j < n_new; j++) {
		_newTokenLexEntryCounts[j] = 0;
	}
	*/
}

void KoreanRetokenizer::alignLexEntry(LexicalEntry *prevLE, int startNewTok, int newTokCount) {
	int leTokMap[MAX_LEX_ENTRIES];
	int ntPlace = 0;
	int startLe = 0;
	

	int numLe = pullLexEntriesUp(prevLexEntryBuffer, MAX_LEX_ENTRIES, prevLE);
	if (dbg) {
		std::cout << "\tLexEntry has " << numLe << " subparts" << std::endl;
	}

	if (numLe < newTokCount) {
		// skip this lexical entry
		return;
	}
	if (dbg) {
		for (int s = 0; s < numLe; s++) 
			std::cout << "\t" << s << ": " << prevLexEntryBuffer[s]->getKey().to_debug_string() << std::endl;
	}


	while (ntPlace < newTokCount) {
		Symbol tokTxt = newTokenBuffer[ntPlace]->getSymbol();
		if (dbg) {
			std::cout << "Search for LexEntries for Token: " << tokTxt.to_debug_string() << std::endl;
		}
		const wchar_t* tokStr = tokTxt.to_string();
		int tokStrLen = static_cast<int>(wcslen(tokStr));
		int tokStrPlace = 0;
		int prevStartLe = startLe;
		int m = startLe;
		while (m < numLe) {
			LexicalEntry* thisLe = prevLexEntryBuffer[m];
			Symbol lenv = thisLe->getKey();
			if (dbg) {
				std::cout << "Normalized LexEntry: " << lenv.to_debug_string() << std::endl;
			}

			//TODO deal with seeing unknown in text
			if (lenv == Symbol()) {
				startLe++;
				leTokMap[m] = ntPlace;
				if (dbg) {
					std::cout << "Map Lexical Entry: " << m << " to Token: " << ntPlace << std::endl;
				}
				m++;
			}
			else {
				//match the current new token with this substring
				const wchar_t* leStr = lenv.to_string();
				int new_place = matchStringInToken(leStr, tokStr, tokStrPlace);
				if (new_place == -1) {
					if (dbg) {
						std::cout << "didn't match - starting at: " << tokStrPlace << " " <<
							Symbol(leStr).to_debug_string() << " " <<
							Symbol(tokStr).to_debug_string() << std::endl;
					}
					break;
				}
				else {
					startLe++;
					if(dbg) {
						std::cout << "\tMap Lexical Entry: " << m << " to Token: " << ntPlace << std::endl;
					}
					leTokMap[m] = ntPlace;
					tokStrPlace = new_place;
					m++;
				}
			}
		}
		//every token must match at least one lexical entry
		if (prevStartLe == startLe)
			break;
		else
			ntPlace++;
	}
	// If we've matched all lexical entries and all tokens, 
	// add the new lexical entry(ies) to the tokens result buffer
	if (dbg) {
		std::cout << "newTokCount: " << newTokCount << " newTokePlace: "
			<< ntPlace << " numLe: " << numLe << " startLE: "
			<< startLe << std::endl;
	}
	if ((ntPlace == newTokCount) && (startLe == numLe)) {
		if (dbg) {
			std::cout << "Add lex entries To Token LexicalEntries: " << std::endl;
		}
		LexicalEntry* addToTokenBuffer[MAX_LEX_ENTRIES];
		int map_place = 0;
		for (int k = 0; k < newTokCount; k++) {
			int c = 0;
			Symbol tokSym = newTokenBuffer[k]->getSymbol();
			if (dbg) {
				std::cout << "New Token: " << tokSym.to_debug_string() << std::endl;
			}
			while ((map_place < numLe) && ((leTokMap[map_place] == k))) {
				if (dbg) {
					std::cout << "\tEntry " << c << ": "
						<< static_cast<int>(prevLexEntryBuffer[map_place]->getID())
						<< " "
						<< prevLexEntryBuffer[map_place]->getKey().to_debug_string()
						<< std::endl;
				}
				addToTokenBuffer[c] = prevLexEntryBuffer[map_place];
				map_place++;
				c++;
			}
			if (c == 1) {
				if (dbg) {
					std::cout <<"\tAdd single LexEntry to buffer:"
						<< static_cast<int>(addToTokenBuffer[0]->getID())
						<< std::endl;
				}
				int count = newTokenLexEntryCounts[startNewTok+k];
				newTokenLexEntryBuffer[startNewTok+k][count] = addToTokenBuffer[0];
				newTokenLexEntryCounts[startNewTok+k]++;
			}
			else {
				size_t id;
				bool found = matchLexEntry(tokSym, addToTokenBuffer, c, id);
				if (!found) {
					id = addLexEntry(tokSym,addToTokenBuffer,c);
				}
				int count = newTokenLexEntryCounts[startNewTok+k];
				newTokenLexEntryBuffer[startNewTok+k][count] = lex->getEntryByID(id);
				newTokenLexEntryCounts[startNewTok+k]++;
			}
		}
	}
}

bool KoreanRetokenizer::matchLexEntry(Symbol key, LexicalEntry** le, int n, size_t& id) {
	int match = -1;
	if (lex->hasKey(key)) {
		//match entries
		LexicalEntry** results = tempLexEntryBuffer;
		int nres = lex->getEntriesByKey(key, results, MAX_LEX_ENTRIES);
		//std::cout << "_matchLexEntry Num Results = " < <nres << std::endl;
		for (int i = 0; i < nres; i++) {
			bool all_match = true;
			if (results[i]->getNSegments() == n) {
				for (int j = 0; j < n; j++) {
					if (results[i]->getSegment(j)->getID() != le[j]->getID()) {
						all_match = false;
						break;
					}
				}
				if (all_match) {
					id = results[i]->getID();
					return true;
				}
			}
		}
	}
	return false;
}

size_t KoreanRetokenizer::addLexEntry(Symbol key, LexicalEntry** le, int n) {
	//NOTE: Lexical Entries and FVS that are created here are only deleted when the 
	//the dictionary is deleted
	KoreanFeatureValueStructure* fvs = _new KoreanFeatureValueStructure(Symbol(L":ROOT"), true);
	LexicalEntry* new_le = _new LexicalEntry(lex->getNextID(), key, fvs, le, n);

	lex->addDynamicEntry(new_le);
	return new_le->getID();
}

int KoreanRetokenizer::getCorrespondingTokens(const Token** results,  
										 const TokenSequence* tokens, 
										 int orig_id, int start)
{
	int count = 0;
	for (int j = start; j < tokens->getNTokens(); j++) {
		if (tokens->getToken(j)->getOriginalTokenIndex() != orig_id) 
			break;
		else
			results[count++] = tokens->getToken(j);
	}
	return count;
}

int KoreanRetokenizer::getCorrespondingTokens(Token** results, Token** tokens, 
										 int orig_id, int start, int max)
{
	int count = 0;
	for(int j = start; j < max; j++) {
		if (tokens[j]->getOriginalTokenIndex() != orig_id)
			break;
		else
			results[count++] = tokens[j];
	}
	return count;
}

int KoreanRetokenizer::matchStringInToken(const wchar_t* le_str, const wchar_t* tok_str, int place) {
	int leStrLen = static_cast<int>(wcslen(le_str));
	int tokStrLen = static_cast<int>(wcslen(tok_str));
	int leStrPlace = 0;
	int tokStrPlace = place;

	if (tokStrLen < leStrLen) return -1;
	if (tokStrPlace >= tokStrLen) return -1;
	
	while (leStrPlace < leStrLen) {
		if (le_str[leStrPlace] != tok_str[tokStrPlace]) {
			if (dbg) {
				std::cout << "No Match at leStrPlace: " << leStrPlace << " tokStrPlace: " 
						  << tokStrPlace << std::endl;
			}
			return -1;
		}
		else {
			leStrPlace++;
			tokStrPlace++;
		}
	}
	return tokStrPlace;
}

int KoreanRetokenizer::pullLexEntriesUp(LexicalEntry** result, int max_size, 
								   LexicalEntry* initialEntry) 
{
	int numSeg = initialEntry->getNSegments();
	if (numSeg == 0) {
		result[0] = initialEntry;
		return 1;
	}
	else {
		int count = 0;
//		LexicalEntry* temp_results[20];
		for (int i = 0; i < numSeg; i++) {
			LexicalEntry* curr_seg = initialEntry->getSegment(i);
/*			// Don't pull LexEntries up from more than one layer deep
			int num_ret = _pullLexEntriesUp(temp_results, 20, curr_seg);
			for (int j = 0; j < num_ret; j++) {
				if (count >= max_size) {
					throw UnrecoverableException("KoreanRetokenizer::_pullLexicalEntriesUp()",
						"number of lexical entries exceeds Max");
				}
				result[count++] = temp_results[j];
			}
*/
			if (count >= max_size) {
					throw UnrecoverableException("KoreanRetokenizer::_pullLexicalEntriesUp()",
						"number of lexical entries exceeds Max");
			}
			result[count++] = curr_seg;
		}
		return count;
	}

}
