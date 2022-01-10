// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef IDF_DOCUMENT_WRITER_H
#define IDF_DOCUMENT_WRITER_H

#include "common/UTF8OutputStream.h"
#include "sentences/SentenceBreaker.h"
#include "theories/Document.h"
#include "theories/Metadata.h"
#include "theories/Token.h"
#include "theories/TokenSequence.h"
#include "tokens/SymbolSubstitutionMap.h"
#include "tokens/Tokenizer.h"
#include "preprocessors/EntityTypes.h"
#include "preprocessors/IdfSpan.h"
#include "preprocessors/ResultsCollector.h"

namespace DataPreprocessor {

	/**
	 * The class responsible for writing the tokenized contents of a document
	 * to an output file in the sexp format recognized by the IdF trainer.
	 *
	 * @author Dave Herman
	 */
	class DocumentWriter {
	public:
		/// Constructs a new DocumentWriter.
		DocumentWriter(ResultsCollector &results);

		/// Destroys this DocumentWriter.
		~DocumentWriter();

		/// Writes the contents of the document to the output file.
		void writeNextDocument(const EntityTypes *entityTypes, Document *doc);

		/// Returns the span that covers the given token, if any.
		IdfSpan* getSpan(Document *doc, const Token *token);

	private:
		/// A filter class for finding Idf spans at a document offset.
		class IdfSpanFilter : public Metadata::SpanFilter {
		public:
			IdfSpanFilter() {}

			/// Determines whether a span is an Idf span.
			virtual bool keep(Span *span) const;
		};

		/// Sentence breaker.
		SentenceBreaker *_sentenceBreaker;

		/// Tokenizer.
		Tokenizer *_tokenizer;

		/// The token sequence that will hold each tokenized sentence.
		TokenSequence *_tokenSequence;

		/// The results collector.
		ResultsCollector* _results;
	};

} // namespace DataPreprocessor

#endif
