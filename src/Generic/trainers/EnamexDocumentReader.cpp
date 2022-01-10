// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/reader/SGMLTag.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Metadata.h"
#include "Generic/parse/Constraint.h"
#include "Generic/preprocessors/EnamexSpanCreator.h"
#include "Generic/preprocessors/SentenceSpanCreator.h"
#include "Generic/preprocessors/TimexSpanCreator.h"
#include "Generic/preprocessors/NumexSpanCreator.h"
#include "Generic/preprocessors/PronounSpanCreator.h"
#include "Generic/preprocessors/HeadSpanCreator.h"
#include "Generic/preprocessors/ListSpanCreator.h"
#include "Generic/preprocessors/DescriptorNPSpanCreator.h"
#include "Generic/trainers/EnamexDocumentReader.h"

namespace DataPreprocessor {
class EnamexDocumentReader;

// HACK: a lot of this was copied/pasted from en_DocumentReader

// Assumptions (i.e., preconditions):
// 1. The document is divided into either <DOCUMENT> or <DOC> sections, but not both
// 2. Each document section contains exactly one <TEXT> section

EnamexDocumentReader::EnamexDocumentReader(DocumentInputStream& in)
{
	_source = NULL;
	_in = &in;
	_untokenizer = Untokenizer::build();
}

EnamexDocumentReader::EnamexDocumentReader(LocatedString* source)
{
	_source = source;
	_in = NULL;
	_untokenizer = Untokenizer::build();
}

EnamexDocumentReader::~EnamexDocumentReader()
{
	delete _untokenizer;
}

Document* EnamexDocumentReader::readNextDocument()
{
	// Nowhere to get the document from.
	if ((_source == NULL) && (_in == NULL)) {
		return NULL;
	}

	Symbol docName = Symbol(L"DataPreprocessor");

	// Load the next document from the input stream.
	if (_source == NULL) {
		LocatedString *docSource = _in->getDocument();
		if (docSource == NULL) {
			return NULL;
		}

		// Find the DOCNO/DOCID/FILEID (if it exists).
		SGMLTag tag = SGMLTag::findOpenTag(*docSource, Symbol(L"DOCNO"), 0);
		if (tag.notFound()) {
			tag = SGMLTag::findOpenTag(*docSource, Symbol(L"DOCID"), 0);
		}
		if (tag.notFound()) {
			tag = SGMLTag::findOpenTag(*docSource, Symbol(L"FILEID"), 0);
		}
		if (!tag.notFound()) {
			int end_open_docno = tag.getEnd();

			tag = SGMLTag::findCloseTag(*docSource, tag.getName(), end_open_docno);
			if (tag.notFound()) {
				throw UnexpectedInputException("EnamexDocumentReader::readNextDocument()", "Document number tag not terminated");
			}
			int start_close_docno = tag.getStart();

			LocatedString *nameString = docSource->substring(end_open_docno, start_close_docno);
			nameString->trim();
			nameString->replace(L"(", L"-LRB-");
			nameString->replace(L")", L"-RRB-");
			docName = nameString->toSymbol();
			delete nameString;
		}

		// Find the beginning of the text section of this document.
		Symbol textSymbol = Symbol(L"TEXT");
		SGMLTag textOpenTag = SGMLTag::findOpenTag(*docSource, textSymbol, 0);
		if (textOpenTag.notFound()) {
			delete docSource;
			throw UnexpectedInputException(
				"EnamexDocumentReader::readNextDocument()",
				"no <TEXT> tag found in document");
		}

		// Find the end of the text section of this document.
		SGMLTag textCloseTag = SGMLTag::findCloseTag(*docSource, textSymbol, textOpenTag.getEnd());
		if (textCloseTag.notFound()) {
			delete docSource;
			throw UnexpectedInputException(
				"EnamexDocumentReader::readNextDocument()",
				"no closing <TEXT> tag found within document");
		}

		// Extract the document source and extract the metadata.
		_source = docSource->substring(textOpenTag.getEnd(), textCloseTag.getStart());
		delete docSource;
	}

	Symbol sentenceSymbol = Symbol(L"SENT");
	Symbol enamexSymbol = Symbol(L"ENAMEX");
	Symbol timexSymbol = Symbol(L"TIMEX");
	Symbol numexSymbol = Symbol(L"NUMEX");
	Symbol pronounSymbol = Symbol(L"PRONOUN");
	Symbol headSymbol = Symbol(L"HEAD");
	Symbol listSymbol = Symbol(L"LIST");
	Symbol descSymbol = Symbol(L"DESCRIPTOR");

	// Extract the metadata from the SGML tags.
	Metadata *metadata = _new Metadata();

	metadata->addSpanCreator(sentenceSymbol, _new SentenceSpanCreator());
	metadata->addSpanCreator(enamexSymbol, _new EnamexSpanCreator());
	metadata->addSpanCreator(timexSymbol, _new TimexSpanCreator());
	metadata->addSpanCreator(numexSymbol, _new NumexSpanCreator());
	metadata->addSpanCreator(pronounSymbol, _new PronounSpanCreator());
	metadata->addSpanCreator(headSymbol, _new HeadSpanCreator());
	metadata->addSpanCreator(listSymbol, _new ListSpanCreator());
	metadata->addSpanCreator(descSymbol, _new DescriptorNPSpanCreator());

	markSpans(*metadata, *_source, sentenceSymbol);
	markSpans(*metadata, *_source, enamexSymbol);
	markSpans(*metadata, *_source, timexSymbol);
	markSpans(*metadata, *_source, numexSymbol);
	markSpans(*metadata, *_source, pronounSymbol);
	markSpans(*metadata, *_source, headSymbol);
	markSpans(*metadata, *_source, listSymbol);

	// Now remove all the SGML tags.
	SGMLTag::removeSGMLTags(*_source);

	// Untokenize the data so as not to confuse the tokenizer.
	_untokenizer->untokenize(*_source);

	// Create a document with the extracted metadata.
	Document *result = _new Document(docName, 1, &_source);
	result->setMetadata(metadata);

	// Clear the document source for the next call.
	delete _source;
	_source = NULL;

	// Return the parsed document.
	return result;
}

void EnamexDocumentReader::markSpans(Metadata& metadata, LocatedString& source, Symbol spanTypeSymbol)
{
	SGMLTag openTag, closeTag;
	int start_name = 0;
	do {
		openTag = SGMLTag::findOpenTag(source, spanTypeSymbol, start_name);
		if (!openTag.notFound()) {
			EDTOffset span_start, span_end;

			int p = openTag.getEnd();
			while (iswspace(source.charAt(p)) || source.charAt(p) == '<') {
				if (source.charAt(p) == '<') {
					while (source.charAt(p) != '>')
						p++;
				}
				p++;
			}

			// The span starts after the open tag.
			span_start = source.firstStartOffsetStartingAt<EDTOffset>(p);

			// The span ends before the close tag.
			closeTag = SGMLTag::findCloseTag(source, spanTypeSymbol, openTag.getEnd());
			if (closeTag.notFound()) {
				p = source.length() - 1;
				while (iswspace(source.charAt(p)) || source.charAt(p) == '>') {
					if (source.charAt(p) == '>') {
						while (source.charAt(p) != '<')
							p--;
					}
					p--;
				}
				span_end = source.lastEndOffsetEndingAt<EDTOffset>(p);
				start_name = source.length();
			}
			else {
				p = closeTag.getStart() - 1;
				while (iswspace(source.charAt(p)) || source.charAt(p) == '>') {
					if (source.charAt(p) == '>') {
						while (source.charAt(p) != '<')
							p--;
					}
					p--;
				}
				span_end = source.lastEndOffsetEndingAt<EDTOffset>(p);
				start_name = closeTag.getEnd();
			}

			metadata.newSpan(
				spanTypeSymbol,
				span_start,
				span_end,
				openTag.getAttributes());
		}
	} while (!openTag.notFound());
}

} // namespace DataPreprocessor
