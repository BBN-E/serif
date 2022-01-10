// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/LocatedString.h"
#include "Generic/tokens/Tokenizer.h"
#include "Generic/theories/Sentence.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Region.h"
#include "Generic/sentences/SentenceBreaker.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/common/ConsoleSessionLogger.h"
#include <boost/scoped_ptr.hpp>

#if defined(WIN32) || defined(WIN64)
#include <direct.h>
#endif
  
using namespace std;

const int max_token_sequences = 1;

/* For the Cluster project, the limits.h was modified to the following values:
** The maximum number of sentences per document:
** #define MAX_DOCUMENT_SENTENCES 200000
** The maximum number of wchar_t chars per sentence:
** #define MAX_SENTENCE_CHARS 5000
** The maximum number of tokens per sentence:
** #define MAX_SENTENCE_TOKENS 5000
** The maximum number of wchar_t chars per token:
** #define MAX_TOKEN_SIZE 500
*/

int main(int argc, char **argv) {
	if ((argc != 4) && (argc != 5))  {
		cerr << "USAGE: StandaloneTokenizer.exe <param_file> <input_file> <output_file> <append_to_output_bit>\n";
		return -1;
	}
	try {
		const char *param_file = argv[1];
		const char *input_file = argv[2];
		const char *output_file = argv[3];
		bool append = false;
		if (argc == 5)
			append = (atoi(argv[4]) == 1);

		ParamReader::readParamFile(argv[1]);

		std::vector<std::wstring>  context_level_names;
		ConsoleSessionLogger logger(context_level_names, L"[StandaloneTokenizer]");
		SessionLogger::setGlobalLogger(&logger);

		SentenceBreaker * sentenceBreaker = SentenceBreaker::build();
		Sentence **sentences = _new Sentence*[MAX_DOCUMENT_SENTENCES];

		Tokenizer * _tokenizer = Tokenizer::build();
		TokenSequence ** _tokenSequenceBuf = _new TokenSequence*[max_token_sequences];

		// Read the input file
		SessionLogger::info("StandaloneTokenizer") << "Reading " << input_file << "...\n";
		boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build(input_file));
		UTF8InputStream& uis(*uis_scoped_ptr);
		if(uis.fail()){
			char msg[1000];		
			strcpy(msg, "Couldn't open input document: ");
			strcat(msg, input_file);
			throw UnexpectedInputException(
			"StandaloneTokenizer::Main()",
			msg);
		}
		LocatedString *text = _new LocatedString(uis, true);
		Document * doc = _new Document(Symbol(L"foodoc"));
		Region * region = _new Region(doc, Symbol(L"fooregion"), 0, text);

		// Open the output file
		UTF8OutputStream out(output_file);

		SessionLogger::info("StandaloneTokenizer") << "Breaking into sentences...\n";
		sentenceBreaker->resetForNewDocument(doc);
		int n_sentences = sentenceBreaker->getSentences(sentences, MAX_DOCUMENT_SENTENCES, &region, 1);

		SessionLogger::info("StandaloneTokenizer") << "Tokenizing...\n";
		for (int j = 0; j < n_sentences; j++) {
			_tokenizer->resetForNewSentence(doc, sentences[j]->getSentNumber());
			int _n_token_sequences = _tokenizer->getTokenTheories(
				_tokenSequenceBuf, max_token_sequences,
				sentences[j]->getString());
			for (int k = 0; k < _tokenSequenceBuf[0]->getNTokens(); k++) {
				if (k != 0)
					out << L" ";
				out << _tokenSequenceBuf[0]->getToken(k)->getSymbol().to_string();
			}
			out << L"\n";
			delete _tokenSequenceBuf[0];
			delete sentences[j];
		}

		delete region;
		delete doc;
		delete text;
		
		uis.close();
		out.close();

		delete _tokenizer;
		delete _tokenSequenceBuf;

		delete sentences;
		delete sentenceBreaker;
	}
	catch (UnrecoverableException &e) {
		cerr << "\n" << e.getMessage() << "\n";
		HeapChecker::checkHeap("main(); About to exit due to error");

		return -1;
	}
#ifdef ENABLE_LEAK_DETECTION
		ParamReader::finalize();

		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
		_CrtDumpMemoryLeaks();
#endif

#if defined(_DEBUG) || defined(_UNOPTIMIZED)
		printf("Press enter to exit....\n");
		getchar();
#endif

	HeapChecker::checkHeap("main(); About to exit after successful run");
	return 0;
}
