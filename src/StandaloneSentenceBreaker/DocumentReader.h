// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef IDF_DOCUMENT_READER_H
#define IDF_DOCUMENT_READER_H

#include <vector>
#include "common/LocatedString.h"
#include "common/Symbol.h"
#include "reader/SGMLTag.h"
#include "theories/Document.h"
#include "preprocessors/DocumentInputStream.h"
#include "tokens/Untokenizer.h"
#include "preprocessors/Attributes.h"
#include "preprocessors/IdfSpan.h"

// Back-doors for unit tests:
#ifdef _DEBUG
#define _protected    public
#define _private      public
#else
#define _protected    protected
#define _private      private
#endif

//namespace DataPreprocessor {
	using namespace DataPreprocessor;

	typedef std::vector<IdfSpan*> SpanVector;

	class DocumentReader {
	public:

		DocumentReader(DocumentInputStream& in, const char * filename);
		DocumentReader(LocatedString *source, const char * filename);
		~DocumentReader();

		Document* readNextDocument();
		SpanVector* getNameSpans() {return _spans;}

	
	private:
	_private:

		void markSpans(Metadata& metadata, LocatedString& source, Symbol spanTypeSymbol);
		void removeSGMLTags(LocatedString& source);
		void untokenize(LocatedString& source);

		/// Removes path from file name and returns as Symbol
		Symbol cleanFileName(const char* file) const;
		Symbol removeFileExtension(const char* file) const;

		LocatedString *_source;
		DocumentInputStream *_in;
		Untokenizer *_untokenizer;
		SpanVector *_spans;
		const char *_filename;
		
		bool _do_untokenization;
	};

//} // namespace DataPreprocessor

#endif
