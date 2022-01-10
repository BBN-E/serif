// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "kr_Klex.h"
#include "common/ParamReader.h"
#include "common/Symbol.h"
#include "common/UnexpectedInputException.h"
#include "common/InternalInconsistencyException.h"
#include "common/SessionLogger.h"
#include "theories/TokenSequence.h"
#include "theories/Token.h"
#include "theories/LexicalEntry.h"
#include "theories/Lexicon.h"
#include "Korean/common/UnicodeEucKrEncoder.h"
#include <wchar.h>
#include <windows.h> 

/** The maximum number of morphs for a LexicalEntry: */
#define MAXIMUM_MORPH_SEGMENTS 10

UnicodeEucKrEncoder* Klex::_encoder = 0;
HANDLE Klex::hChildProcess = NULL;

char Klex::_cmd_str[1000] = "";
unsigned char Klex::_euc_buffer[Klex::max_sentence_chars];
wchar_t Klex::_buffer[Klex::max_sentence_chars];
Token* Klex::_tokenBuffer[MAX_SENTENCE_TOKENS];
LexicalEntry* Klex::_lexBuffer[MAX_ENTRIES_PER_SYMBOL];

int Klex::analyzeSentence(TokenSequence *tokens, Lexicon *lexicon) {
	wchar_t utf8_entry[MAX_TOKEN_SIZE];
	LexicalEntry* lex_entries[MAXIMUM_MORPH_ANALYSES];
	int n_lex_entries = 0;
	LexicalEntry *lexPtr; 
	size_t len = 0;

	// concatenate all tokens into one string
	wcscpy(_buffer, L"");
	for (int i = 0; i < tokens->getNTokens(); i++) {
		const Token* t = tokens->getToken(i);
		wcsncat(_buffer, t->getSymbol().to_string(), max_sentence_chars - len);
		len += wcslen(t->getSymbol().to_string());
		wcsncat(_buffer, L"\n", max_sentence_chars - len);
		len += 1;
	}

	_encoder = UnicodeEucKrEncoder::getInstance();
	_encoder->unicode2EUC(_buffer, _euc_buffer, max_sentence_chars);

	std::string euc_results = runKlex();
	std::wstring utf8_results = _encoder->euc2Unicode(euc_results);
	len = utf8_results.length();

	size_t j = 0;
	int curr_tok = 0;
	const Token *currToken = tokens->getToken(curr_tok);

	while (j < len) {

		int k = 0; 
		while (j < len && k < MAX_TOKEN_SIZE - 1 && utf8_results.at(j) != '\n') {
			utf8_entry[k++] = utf8_results.at(j++);
		}
		utf8_entry[k] = '\0';

		// make sure currToken matches the beginning of utf8_entry
		if (isAllWhitespace(utf8_entry) || wcsstr(utf8_entry, currToken->getSymbol().to_string()) == 0) {

			if (curr_tok < tokens->getNTokens()) {
				_tokenBuffer[curr_tok++] = _new Token(currToken->getStartOffset(), currToken->getEndOffset(), 
											curr_tok, currToken->getSymbol(), 
											n_lex_entries, lex_entries);
			}
			else {
				(*SessionLogger::logger).beginWarning();
				(*SessionLogger::logger) << "Number of tokens in KLEX analysis results exceeds original number of tokens -- truncating.\n";
				n_lex_entries = 0;
				break;
			}

			//std::cout << ".";
			if (curr_tok < tokens->getNTokens()) {
				currToken = tokens->getToken(curr_tok);
				n_lex_entries = lexicon->getEntriesByKey(currToken->getSymbol(), lex_entries, MAXIMUM_MORPH_ANALYSES);
			}
			else {
				n_lex_entries = 0; // okay, this must be the last token in sentence
			}

			// move past any following whitespace
			while (j < len && iswspace(utf8_results.at(j)))
				j++;
		}
		else {	
			lexPtr = createLexicalEntry(lexicon, utf8_entry);

			if (lexPtr != 0) {
				
				if (n_lex_entries < MAXIMUM_MORPH_ANALYSES) {
					// check to make sure this option doesn't already exist in the lexicon
					LexicalEntry **existingEntries = _new LexicalEntry*[MAX_ENTRIES_PER_SYMBOL];
					int n_existing = lexicon->getEntriesByKey(lexPtr->getKey(), existingEntries, MAX_ENTRIES_PER_SYMBOL);
					bool already_exists = false;
					if (n_existing > 0) {
						for (int m = 0; m < n_existing; m++) {
							if ((*lexPtr) == (*existingEntries[m])) {
								already_exists = true;
								break;
							}
						}
					}
					if (!already_exists) 
						lexicon->addDynamicEntry(lexPtr);
					lex_entries[n_lex_entries++] = lexPtr;
				}
			}
			j++;
		}	
	}

	if (n_lex_entries > 0) {
		if (curr_tok < tokens->getNTokens()) {
			_tokenBuffer[curr_tok++] = _new Token(currToken->getStartOffset(), currToken->getEndOffset(), 
												curr_tok, currToken->getSymbol(), 
												n_lex_entries, lex_entries);
		}
		else {
			(*SessionLogger::logger).beginWarning();
			(*SessionLogger::logger) << "Number of tokens in KLEX analysis results exceeds original number of tokens -- truncating.\n";
		}
	}

	
	if (curr_tok < tokens->getNTokens()) {
		(*SessionLogger::logger).beginWarning();
		(*SessionLogger::logger) << "Number of tokens in KLEX morph analysis results "
								 << "is less than original number of tokens.\n";
	}

	tokens->retokenize(curr_tok, _tokenBuffer);
	return curr_tok;
}


int Klex::analyzeWord(const wchar_t* input, Lexicon *lexicon, LexicalEntry** result, int max_results) {
	LexicalEntry *lexPtr; 
	int n_results = 0;
	
	unsigned char euc_entry[MAX_TOKEN_SIZE];
	wchar_t utf8_entry[MAX_TOKEN_SIZE];

	_encoder = UnicodeEucKrEncoder::getInstance();
	_encoder->unicode2EUC(input, _euc_buffer, max_sentence_chars);

	std::string euc_result = runKlex();

	size_t i = 0;
	size_t len = euc_result.length();

	while (i < len && n_results < max_results - 1) {
		int j = 0; 
		while (i < len && j < MAX_TOKEN_SIZE - 1 && euc_result.at(i) != '\n') {
			euc_entry[j++] = euc_result.at(i);
			i++;
		}
		euc_entry[j] = '\0';

		_encoder->euc2Unicode(euc_entry, utf8_entry, MAX_TOKEN_SIZE);
		
		lexPtr = createLexicalEntry(lexicon, utf8_entry);
		if (lexPtr == 0)
			break;

		// check to make sure this option doesn't already exist in the lexicon
		LexicalEntry **existingEntries = _new LexicalEntry*[MAX_ENTRIES_PER_SYMBOL];
		int n_existing = lexicon->getEntriesByKey(lexPtr->getKey(), existingEntries, MAX_ENTRIES_PER_SYMBOL);
		bool already_exists = false;
		if (n_existing > 0) {
			for (int m = 0; m < n_existing; m++) {
				if ((*lexPtr) == (*existingEntries[m])) {
					already_exists = true;
					break;
				}
			}
		}
		if (!already_exists) 
			lexicon->addDynamicEntry(lexPtr);
		result[n_results++] = lexPtr;
		
		i++;
	}
	return n_results;
}

std::string Klex::runKlex() {

	HANDLE hOutputReadTmp,hOutputRead,hOutputWrite;
    HANDLE hInputWriteTmp,hInputRead,hInputWrite;
    HANDLE hErrorReadTmp,hErrorRead,hErrorWrite;
    HANDLE hThread;
    DWORD ThreadId;
    SECURITY_ATTRIBUTES sa;

	// Set up the security attributes struct.
    sa.nLength= sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    // Create the child output pipe.
    if (!CreatePipe(&hOutputReadTmp,&hOutputWrite,&sa,0))
       throw InternalInconsistencyException("Klex::runKlex()", "Create child output pipe");

	 // Create the child error pipe.
    if (!CreatePipe(&hErrorReadTmp,&hErrorWrite,&sa,0))
       throw InternalInconsistencyException("Klex::runKlex()", "Create child error pipe");

	// Create the child input pipe.
    if (!CreatePipe(&hInputRead,&hInputWriteTmp,&sa,0))
		throw InternalInconsistencyException("Klex::runKlex()", "Create child input pipe");


    // Create new output read handle and the input write handles. Set
    // the Properties to FALSE. Otherwise, the child inherits the
    // properties and, as a result, non-closeable handles to the pipes
    // are created.
    if (!DuplicateHandle(GetCurrentProcess(),hOutputReadTmp,
                         GetCurrentProcess(),
                         &hOutputRead, // Address of new handle.
                         0,FALSE, // Make it uninheritable.
                         DUPLICATE_SAME_ACCESS))
         throw InternalInconsistencyException("Klex::runKlex()", "Duplicate output read Handle");
	
	if (!DuplicateHandle(GetCurrentProcess(),hErrorReadTmp,
                         GetCurrentProcess(),
                         &hErrorRead, // Address of new handle.
                         0,FALSE, // Make it uninheritable.
                         DUPLICATE_SAME_ACCESS))
		 throw InternalInconsistencyException("Klex::runKlex()", "Duplicate error read Handle");


    if (!DuplicateHandle(GetCurrentProcess(),hInputWriteTmp,
                         GetCurrentProcess(),
                         &hInputWrite, // Address of new handle.
                         0,FALSE, // Make it uninheritable.
                         DUPLICATE_SAME_ACCESS))
		 throw InternalInconsistencyException("Klex::runKlex()", "Duplicate input write Handle");

	// Close inheritable copies of the handles you do not want to be inherited.
    if (!CloseHandle(hOutputReadTmp)) 
		throw InternalInconsistencyException("Klex::runKlex()", "Close output read tmp Handle");
	if (!CloseHandle(hErrorReadTmp)) 
		throw InternalInconsistencyException("Klex::runKlex()", "Close error read tmp Handle");
    if (!CloseHandle(hInputWriteTmp)) 
		throw InternalInconsistencyException("Klex::runKlex()", "Close input write tmp Handle");

	prepAndLaunchKlex(hOutputWrite,hInputRead,hErrorWrite);

	// Close pipe handles (do not continue to modify the parent).
    // You need to make sure that no handles to the write end of the
    // output pipe are maintained in this process or else the pipe will
    // not close when the child process exits and the ReadFile will hang.
    if (!CloseHandle(hOutputWrite))
		throw InternalInconsistencyException("Klex::runKlex()", "Close output write Handle");
    if (!CloseHandle(hInputRead )) 
		throw InternalInconsistencyException("Klex::runKlex()", "Close input read Handle");
    if (!CloseHandle(hErrorWrite)) 
		throw InternalInconsistencyException("Klex::runKlex()", "Close error write Handle");

	// Launch the thread that gets the input and sends it to the child.
    hThread = CreateThread(NULL,0,sendInputThread,
                            (LPVOID)hInputWrite,0,&ThreadId);
    if (hThread == NULL) 
		throw InternalInconsistencyException("Klex::runKlex()", "Create send input Thread");

    // Read the child's output.
	std::string result = readAndHandleOutput(hOutputRead);
    // Redirection is complete

    if (WaitForSingleObject(hThread,INFINITE) == WAIT_FAILED)
       throw InternalInconsistencyException("Klex::runKlex()", "WaitForSingleObject");

	if (!CloseHandle(hOutputRead)) 
		throw InternalInconsistencyException("Klex::runKlex()", "Close output read Handle");
    //if (!CloseHandle(hInputWrite)) 
	//	throw InternalInconsistencyException("Klex::runKlex()", "Close input write Handle");

	return result;
}

void Klex::prepAndLaunchKlex(HANDLE hChildStdOut, HANDLE hChildStdIn, HANDLE hChildStdErr) {
	PROCESS_INFORMATION pi;
    STARTUPINFO si;

    // Set up the start up info struct.
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hChildStdOut;
    si.hStdInput  = hChildStdIn;
    si.hStdError  = hChildStdErr;
    si.wShowWindow = SW_HIDE;

	if (strcmp(_cmd_str, "") == 0) {
		char buffer[500];
		ParamReader::getRequiredParam("morph_xfst_path", buffer, 500);
		strncpy(_cmd_str, buffer, 400);
		strncat(_cmd_str, " ", 1); 
		ParamReader::getRequiredParam("morph_klex_path", buffer, 500);
		strncat(_cmd_str, buffer, 400);
		strncat(_cmd_str, " -flags mbTT", 12);
	}

	if (!CreateProcess(NULL, _cmd_str,
					   NULL,NULL,TRUE,
                       CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi))
		throw InternalInconsistencyException("Klex::prepAndLaunchKlex()", "CreateProcess");

    // Set global child process handle to cause threads to exit.
    hChildProcess = pi.hProcess;

    // Close any unnecessary handles.
    if (!CloseHandle(pi.hThread)) 
		throw InternalInconsistencyException("Klex::prepAndLaunchKlex()", "Close child thread Handle");
}

std::string Klex::readAndHandleOutput(HANDLE hPipeRead) {
	std::string result;
	CHAR lpBuffer[256];
    DWORD nBytesRead;

    while (TRUE) {
		if (!ReadFile(hPipeRead,lpBuffer,sizeof(lpBuffer),
						&nBytesRead,NULL) || !nBytesRead)
		{
			if (GetLastError() == ERROR_BROKEN_PIPE)
				break; // pipe done - normal exit path.
            else
				throw InternalInconsistencyException("Klex::readAndHandleOutput()", "ReadFile"); 
         }

		 result.append(lpBuffer, nBytesRead);
	}
	return result;
}

DWORD WINAPI Klex::sendInputThread(LPVOID lpvThreadParam) {
	DWORD nBytesWrote;
    HANDLE hPipeWrite = (HANDLE)lpvThreadParam;
	
	// zero out any trailing data in the buffer
	size_t i = 0;
	while (i < max_sentence_chars && _euc_buffer[i] != '\0')
		i++;
	for (i; i < max_sentence_chars; i++)
		_euc_buffer[i] = '\0';

	if (!WriteFile(hPipeWrite, _euc_buffer, sizeof(_euc_buffer), &nBytesWrote, NULL)) {
		if (GetLastError() != ERROR_NO_DATA)
			throw InternalInconsistencyException("Klex::sendInputThread()", "WriteFile");
	}

	if (!CloseHandle(hPipeWrite)) 
		throw InternalInconsistencyException("Klex::sendInputThread()", "Close child write pipe Handle");

    return 1;
}

LexicalEntry* Klex::createLexicalEntry(Lexicon *lexicon, wchar_t *entry_str) {
	Symbol tmpMorphs[MAXIMUM_MORPH_SEGMENTS];
	Symbol tmpPOS[MAXIMUM_MORPH_SEGMENTS];
	size_t len = wcslen(entry_str);
	LexicalEntry *entry = 0;
	KoreanFeatureValueStructure *fvs = 0;
	int n_morphs = 0;
	size_t i = 0;
	
	while (i < len && (iswspace(entry_str[i]) || entry_str[i] == 0x09)) i++;

	if (len == 0 || i == len)
		return entry;

	// get original word
	int k = 0;
	while (i < len && i < MAX_TOKEN_SIZE - 1 && entry_str[i] != 0x09) {
		_buffer[k++] = entry_str[i++];
	}
	_buffer[k] = L'\0';
	Symbol origWord = Symbol(_buffer);

	while (i < len && (iswspace(entry_str[i]) || entry_str[i] == 0x09)) i++;

	size_t end = len - 1;
	while (end > 1 && (iswspace(entry_str[end]) || (entry_str[end] == 0x09))) end--;

	// check first for Klex failure
	if (entry_str[end-1] == '+' && entry_str[end] == '?') { 
		tmpMorphs[n_morphs] = origWord;
		tmpPOS[n_morphs] = Symbol(L":UNK"); 
		n_morphs++;
	}
	else {
		while (i < len) {
			int j = 0;
			while (i < len && entry_str[i] != L'/' && entry_str[i] != '^') {
				_buffer[j++] = entry_str[i];
				i++;
			}
			_buffer[j] = L'\0';
			tmpMorphs[n_morphs] = Symbol(_buffer);
			j = 0;
			i++;
			while (i < len && entry_str[i] != L'+' && !iswspace(entry_str[i])) {
				_buffer[j++] = entry_str[i];
				i++;
			}
			_buffer[j] = L'\0';
			tmpPOS[n_morphs] = Symbol(_buffer);
			n_morphs++;
			i++;
		}
	}
	
	if (n_morphs > 1) {
		LexicalEntry **morphEntries = _new LexicalEntry*[n_morphs];
		for (int k = 0; k < n_morphs; k++) {
			fvs = _new KoreanFeatureValueStructure(tmpPOS[k], true);
			morphEntries[k] = _new LexicalEntry(lexicon->getNextID(), tmpMorphs[k], fvs, NULL, 0);
			int n_existing = lexicon->getEntriesByKey(tmpMorphs[k], _lexBuffer, MAX_ENTRIES_PER_SYMBOL);
			bool found_match = false;
			for (int j = 0; j < n_existing; j++) {
				if ((*_lexBuffer[j]) == (*morphEntries[k])) {
					delete morphEntries[k];
					morphEntries[k] = _lexBuffer[j];
					found_match = true;
					break;
				}
			}
			if (!found_match) 
				lexicon->addDynamicEntry(morphEntries[k]);	
		}
		fvs = _new KoreanFeatureValueStructure(Symbol(L":ROOT"), true);
		entry = _new LexicalEntry(lexicon->getNextID(), origWord, fvs, morphEntries, n_morphs);
		delete [] morphEntries;
	}
	else if (n_morphs == 1) { 
		fvs = _new KoreanFeatureValueStructure(tmpPOS[0], true);
		entry = _new LexicalEntry(lexicon->getNextID(), origWord, fvs, NULL, 0);
	}
	else {
		(*SessionLogger::logger).beginWarning();
		(*SessionLogger::logger) << "No morphs found in KLEX entry: " << entry_str << "\n";
	}

	return entry;
}

bool Klex::isAllWhitespace(wchar_t *str) {
	size_t len = wcslen(str);
	for (size_t i = 0; i < len; i++) {
		if (!iswspace(str[i]))
			return false;
	}
	return true;
}
