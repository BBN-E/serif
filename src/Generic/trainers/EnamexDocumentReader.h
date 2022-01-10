// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENAMEX_DOCUMENT_READER_H
#define ENAMEX_DOCUMENT_READER_H

#include "Generic/common/LocatedString.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Document.h"
#include "Generic/preprocessors/DocumentInputStream.h"
#include "Generic/tokens/Untokenizer.h"
#include "Generic/preprocessors/Attributes.h"

// Back-doors for unit tests:
#ifdef _DEBUG
#define _protected    public
#define _private      public
#else
#define _protected    protected
#define _private      private
#endif

namespace DataPreprocessor {
	class EnamexDocumentReader {
	public:
		EnamexDocumentReader(DocumentInputStream& in);
		EnamexDocumentReader(LocatedString *source);
		~EnamexDocumentReader();

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
