
// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ANNOTATED_PARSE_DOCUMENT_WRITER_H
#define ANNOTATED_PARSE_DOCUMENT_WRITER_H

class UTF8OutputStream;
class SentenceBreaker;
class Document;
class DocTheory;
class Token;
class TokenSequence;
class NameTheory;
class Parse;
class MentionSet;
class SymbolSubstitutionMap;
class Tokenizer;
class Parser;
class DescriptorRecognizer;
class SynNode;
class Constraint;
class NPChunkFinder;
class NPChunkTheory;
class Span;
class Metadata;

namespace DataPreprocessor {

	class CorefSpan;
	class IdfSpan;
	class EntityTypes;
	class ResultsCollector;

	/**
	 * The class responsible for writing the tokenized and parsed contents of a document
	 * to an output file in the Annotated Parse sexp format.
	 *
	 */
	class DocumentProcessor {
	public:
		/// Constructs a new DocumentProcessor.
		DocumentProcessor(const EntityTypes *entityTypes);

		/// Destroys this DocumentProcessor.
		~DocumentProcessor();

		/// Reset the results collector used to collect processor output.
		void setResultsCollector(ResultsCollector &results);

		/// Processes and writes the contents of the document to the output file.
		void processNextDocument(Document *doc);

		/// Returns the coref span that covers the given token, if any.
		CorefSpan* getCorefSpan(const Token *token);

		/// Returns the idf span that covers the given token, if any.
		IdfSpan* getIdfSpan(const Token *token);

		/// Returns the idf name span that covers the given token, if any.
		IdfSpan* getIdfNameSpan(const Token *token);

		/// Returns the non-name span that covers the given token, if any.
		CorefSpan* getNonNameSpan(const Token *token);

		/// Returns the span that exactly covers the given node, if any.
		CorefSpan* getSpanByNode(const SynNode *node);

		/// Loads _nameTheory with names from the metadata
		void loadNameTheory();

		/// Loads _constraints with non-name coref spans from metadata
		int loadConstraints();

		/// Creates descriptor spans for annotated descriptor heads 
		void addDescriptorNPSpans();

		/// Returns a pretty-printable parse string
		std::wstring printParse(const SynNode *node, int indent);
	
		/// Returns the EDT-type/MENT-type type string for the given span and node
		std::wstring getTypeString(CorefSpan *span, const SynNode *node);

		/// Returns the ID string for the given span
		std::wstring getIDString(CorefSpan *span);

		// Returns true if span is a type of span that DocumentProcessor cares about
		bool isValid(Span *span);

	private:

		/// The document currently being processed
		Document *_doc;
		DocTheory *_docTheory;

		/// Metadata associated with current document
		Metadata *_metadata;

		/// Entity types to use in processing
		const EntityTypes *_entityTypes;

		/// Sentence breaker.
		SentenceBreaker *_sentenceBreaker;

		/// Tokenizer.
		Tokenizer *_tokenizer;

		/// The token sequence that will hold each tokenized sentence.
		TokenSequence *_tokenSequence;

		/// NameTheory to hold the names read in
		NameTheory *_nameTheory;

		/// Array to hold non-name constraints
		Constraint *_constraints;

		/// Parser.
		Parser *_parser;

		/// The parse
		Parse *_parse;

		/// The NPChunker
		NPChunkFinder *_npChunkFinder;

		/// The NPChunkTheory
		NPChunkTheory *_npChunkTheory;

		/// Descriptor Recognizer
		DescriptorRecognizer *_descriptorRecognizer;

		/// The mention set
		MentionSet *_mentionSet;

		/// The results collector.
		ResultsCollector* _results;

		/// A filter class for finding Idf spans at a document offset.
		class IdfSpanFilter : public Metadata::SpanFilter {
		public:
			IdfSpanFilter() {}

			/// Determines whether a span is an Idf span.
			virtual bool keep(Span *span) const;
		};

		/// A filter class for finding Idf name spans at a document offset.
		class IdfNameSpanFilter : public Metadata::SpanFilter {
		public:
			IdfNameSpanFilter() {}

			/// Determines whether a span is an Idf name span.
			virtual bool keep(Span *span) const;
		};

		/// A filter class for finding Coref spans at a document offset.
		class CorefSpanFilter : public Metadata::SpanFilter {
		public:
			CorefSpanFilter() {}

			/// Determines whether a span is a Coref span.
			virtual bool keep(Span *span) const;
		};

		/// A filter class for finding non-name spans at a document offset.
		class NonNameSpanFilter : public Metadata::SpanFilter {
		public:
			NonNameSpanFilter() {}

			/// Determines whether a span is a non-name span.
			virtual bool keep(Span *span) const;
		};

		/// A filter class for finding non-constituent spans within a given node.
		class NonConstitSpanFilter : public Metadata::SpanFilter {
		public:
			NonConstitSpanFilter(const SynNode *node, const TokenSequence *tokens);

			/// Determines whether a span is a non-name span.
			virtual bool keep(Span *span) const;
		private:
			const SynNode *_node;
			const TokenSequence *_tokens;
			int _start, _end;
		};
	};

} // namespace DataPreprocessor

#endif
