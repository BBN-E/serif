// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/LocatedString.h"
#include "common/UnrecoverableException.h"
#include "common/limits.h"
#include "sentences/SentenceBreaker.h"
#include "theories/Document.h"
#include "theories/DocTheory.h"
#include "theories/Metadata.h"
#include "theories/Sentence.h"
#include "theories/Span.h"
#include "theories/Token.h"
#include "tokens/SymbolSubstitutionMap.h"
#include "tokens/Tokenizer.h"
#include "preprocessors/EntityTypes.h"
#include "preprocessors/IdfSpan.h"
#include "DocumentWriter.h"

#include <iostream>
using namespace std;

namespace DataPreprocessor {

Sentence *sentences[MAX_DOCUMENT_SENTENCES];

static const Symbol ENAMEX_TYPE_NAME = Symbol(L"Enamex");
static const Symbol TIMEX_TYPE_NAME = Symbol(L"Timex");
static const Symbol NUMEX_TYPE_NAME = Symbol(L"Numex");

/**
 * @param span the span to examine.
 * @return <code>true</code> if the span is an Idf span,
 *         <code>false</code> if not.
 */
bool DocumentWriter::IdfSpanFilter::keep(Span *span) const {
	Symbol type = span->getSpanType();
	return (type == ENAMEX_TYPE_NAME) ||
		   (type == TIMEX_TYPE_NAME)  ||
		   (type == NUMEX_TYPE_NAME);
}

/**
 * @param results the results collector.
 */
DocumentWriter::DocumentWriter(ResultsCollector& results)
{
	_results = &results;
	_tokenizer = Tokenizer::build();
	_sentenceBreaker = SentenceBreaker::build();
}

DocumentWriter::~DocumentWriter()
{
	delete _tokenizer;
	delete _sentenceBreaker;
}

/**
 * @param doc the document to search.
 * @param token the token to search.
 * @return the IdF span that covers the given token, if any.
 */
IdfSpan* DocumentWriter::getSpan(Document *doc, const Token *token)
{
	DocumentWriter::IdfSpanFilter filter;
	IdfSpan *span = NULL;

	// Find any Idf spans at the token's start offset.
	Metadata::SpanList *list = doc->getMetadata()->getCoveringSpans(token->getStartEDTOffset(), &filter);
	if (list->length() > 0) {
		span = (IdfSpan *)((*list)[0]);
	}
	delete list;

	return span;
}

/**
 * @param entityTypes a map from SGML entity type names to their abbreviated forms.
 * @param doc the tokenized document.
 * @param output_file the name of the sexp file to write to.
 */
void DocumentWriter::writeNextDocument(const EntityTypes *entityTypes, Document *doc)
{
	Metadata *metadata = doc->getMetadata();

	// Break the document into sentences.
	_sentenceBreaker->resetForNewDocument(doc);
	int n_sentences = _sentenceBreaker->getSentences(
		sentences, MAX_DOCUMENT_SENTENCES,
		doc->getRegions(), doc->getNRegions());

	// Tokenize each sentence and output it to the sexp-file.
	for (int i = 0; i < n_sentences; i++) {
		// Tokenize the sentence.
		const LocatedString *source = sentences[i]->getString();
		_tokenizer->resetForNewSentence(doc, i);
		_tokenizer->getTokenTheories(&_tokenSequence, 1, source);
		(*_results) << L"(";

		IdfSpan *previousSpan = NULL;
		for (int j = 0; j < _tokenSequence->getNTokens(); j++) {
			const Token *token = _tokenSequence->getToken(j);

			// Find any metadata tags covering this token.
			IdfSpan *currentSpan = getSpan(doc, token);
			Symbol tokenType =
				currentSpan ? entityTypes->lookup(currentSpan->getType())
							: Symbol(L"NONE");

			if (j > 0) {
				(*_results) << L" ";
			}
			(*_results) << L"("
						<< token->getSymbol().to_string() << L" "
						<< tokenType.to_string() << L"-"
						<< (((j > 0) && (currentSpan == previousSpan)) ? L"CO" : L"ST")
						<< L")";

			previousSpan = currentSpan;
		}
		(*_results) << L")\n";

		delete sentences[i];
		sentences[i] = NULL;
		delete _tokenSequence;
	}
}

} // namespace DataPreprocessor
