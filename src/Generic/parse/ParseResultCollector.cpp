// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/ParseResultCollector.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/OStringStream.h"
#include "Generic/theories/SynNode.h"

using namespace std;

void ParseResultCollector::loadDocTheory(DocTheory* theory) {
	_docTheory = theory;
}

void ParseResultCollector::produceOutput(const wchar_t *output_dir,
									     const wchar_t *doc_filename)
{
	wstring output_file_parse = wstring(output_dir) + LSERIF_PATH_SEP + wstring(doc_filename) + L".parse";
	wstring output_file_token = wstring(output_dir) + LSERIF_PATH_SEP + wstring(doc_filename) + L".tokens";

	UTF8OutputStream stream;
	stream.open(output_file_parse.c_str());
	produceParseOutput(stream);
	stream.close();

	UTF8OutputStream tokenStream;
	tokenStream.open(output_file_token.c_str());
	produceTokenOutput(tokenStream);
	tokenStream.close();
}


void ParseResultCollector::produceParseOutput(OutputStream &stream) {
	for (int i = 0; i < _docTheory->getNSentences(); i++) {
		Parse *parse = _docTheory->getSentenceTheory(i)->getPrimaryParse();
		stream << parse->getRoot()->toPrettyParse(0);
		stream << L"\n\n";
	}
}

void ParseResultCollector::produceTokenOutput(OutputStream &stream) {
	for (int i = 0; i < _docTheory->getNSentences(); i++) {
		TokenSequence *tokens = _docTheory->getSentenceTheory(i)->getTokenSequence();
		stream << tokens->toStringWithOffsets();
		stream << L"\n\n";
	}
}
