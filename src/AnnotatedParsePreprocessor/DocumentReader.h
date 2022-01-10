// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ANNOTATED_PARSE_DOCUMENT_READER_H
#define ANNOTATED_PARSE_DOCUMENT_READER_H

#include "common/LocatedString.h"
#include "common/Symbol.h"
#include "theories/Document.h"
#include "preprocessors/DocumentInputStream.h"
#include "tokens/Untokenizer.h"

// Back-doors for unit tests:
#ifdef _DEBUG
#define _protected    public
#define _private      public
#else
#define _protected    protected
#define _private      private
#endif

namespace DataPreprocessor {

	class DocumentReader {
	public:
		DocumentReader(DocumentInputStream& in);
		DocumentReader(LocatedString *source);
		~DocumentReader();

		Document* readNextDocument();

	
	private:
	_private:

		void markSpans(Metadata& metadata, LocatedString& source, Symbol spanTypeSymbol);
		void untokenize(LocatedString& source);

		LocatedString *_source;
		DocumentInputStream *_in;
		Untokenizer *_untokenizer;
	};

} // namespace DataPreprocessor

#endif
