// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/UnexpectedInputException.h"
#include "common/UTF8InputStream.h"
#include "common/ParamReader.h"
#include "theories/Document.h"
#include "theories/Metadata.h"
#include "reader/SGMLTag.h"
#include "preprocessors/EnamexSpanCreator.h"
#include "preprocessors/SentenceSpanCreator.h"
#include "preprocessors/TimexSpanCreator.h"
#include "preprocessors/NumexSpanCreator.h"
#include "DocumentReader.h"

//namespace DataPreprocessor {

class DocumentReader;

// HACK: a lot of this was copied/pasted from en_DocumentReader

// Assumptions (i.e., preconditions):
// 1. The document is divided into either <DOCUMENT> or <DOC> sections, but not both
// 2. Each document section contains exactly one <TEXT> section

DocumentReader::DocumentReader(DocumentInputStream& in, const char * filename)
{
	_source = NULL;
	_spans = _new SpanVector(0);
	_in = &in;
	_untokenizer = Untokenizer::build();
	_filename = filename;

	_do_untokenization = ParamReader::getOptionalTrueFalseParamWithDefaultVal("do_untokenization", true);
}

DocumentReader::DocumentReader(LocatedString* source, const char * filename)
{
	_source = source;
	_spans = _new SpanVector(0);
	_in = NULL;
	_untokenizer = Untokenizer::build();
	_filename = filename;
}

DocumentReader::~DocumentReader()
{
	delete _untokenizer;
	delete _spans;
}

Document* DocumentReader::readNextDocument()
{
	// Nowhere to get the document from.
	if ((_source == NULL) && (_in == NULL)) {
		return NULL;
	}
	
	Symbol name = Symbol(L"0");
	_spans->clear();

	// Load the next document from the input stream.
	if (_source == NULL) {
		LocatedString *docSource = _in->getDocument();
		if (docSource == NULL) {
			return NULL;
		}

		// For Ace2004, some of the annotation files use DOCID instead... sigh...
		// Find the file ID.
		SGMLTag tag;
		bool docno_tag = false;
		bool docid_tag = false;
		tag = SGMLTag::findOpenTag(*docSource, Symbol(L"DOCNO"), 0);
		if (!tag.notFound()) {
			docno_tag = true;
		}
		if (!docno_tag) {	
			tag = SGMLTag::findOpenTag(*docSource, Symbol(L"DOCID"), 0);
			if (!tag.notFound()) {
				docid_tag = true;
			}
		}
		if (docno_tag || docid_tag) {
			int end_open_docno = tag.getEnd();

			if (docno_tag)
				tag = SGMLTag::findCloseTag(*docSource, Symbol(L"DOCNO"), end_open_docno);
			else tag = SGMLTag::findCloseTag(*docSource, Symbol(L"DOCID"), end_open_docno);
			
			if (tag.notFound()) {
				throw UnexpectedInputException("DocumentReader::readDocument()", 
					"DOCNO or DOCID tag not terminated");
			}
			int start_close_docno = tag.getStart();

			LocatedString *nameString = docSource->substring(end_open_docno, start_close_docno);
			nameString->trim();
			name = nameString->toSymbol();
			delete nameString;
		}else { // try <DOC id="...">
			tag = SGMLTag::findOpenTag(*docSource, Symbol(L"DOC"), 0);
			if(!tag.notFound()){
				name = tag.getDocumentID();
			}
		}

		if (name == Symbol(L"0")) {
			name = cleanFileName(_filename);
			name = removeFileExtension(name.to_debug_string());
		}

		// Find the beginning of the text section of this document.
		Symbol textSymbol = Symbol(L"TEXT");
		SGMLTag textOpenTag = SGMLTag::findOpenTag(*docSource, textSymbol, 0);
		if (textOpenTag.notFound()) {
			delete docSource;
			throw UnexpectedInputException(
				"DocumentReader::readNextDocument()",
				"no <TEXT> tag found in document");
		}

		// Find the end of the text section of this document.
		SGMLTag textCloseTag = SGMLTag::findCloseTag(*docSource, textSymbol, textOpenTag.getEnd());
		if (textCloseTag.notFound()) {
			delete docSource;
			throw UnexpectedInputException(
				"DocumentReader::readNextDocument()",
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

	// Extract the metadata from the SGML tags.
	Metadata *metadata = _new Metadata();

	metadata->addSpanCreator(sentenceSymbol, _new SentenceSpanCreator());
	metadata->addSpanCreator(enamexSymbol, _new EnamexSpanCreator());
	metadata->addSpanCreator(timexSymbol, _new TimexSpanCreator());
	metadata->addSpanCreator(numexSymbol, _new NumexSpanCreator());

	markSpans(*metadata, *_source, sentenceSymbol);
	markSpans(*metadata, *_source, enamexSymbol);
	markSpans(*metadata, *_source, timexSymbol);
	markSpans(*metadata, *_source, numexSymbol);

	// Now remove all the SGML tags.
	SGMLTag::removeSGMLTags(*_source);

	// Untokenize the data so as not to confuse the tokenizer.
	if (_do_untokenization)
		_untokenizer->untokenize(*_source);

	// Create a document with the extracted metadata.
	Document *result = _new Document(name, 1, &_source);
	result->setMetadata(metadata);
	//result->setSourceType(doctype);

	// Clear the document source for the next call.
	delete _source;
	_source = NULL;

	// Return the parsed document.
	return result;
}

void DocumentReader::markSpans(Metadata& metadata, LocatedString& source, Symbol spanTypeSymbol)
{
	SGMLTag openTag, closeTag;
	int start_name = 0;
	do {
		openTag = SGMLTag::findOpenTag(source, spanTypeSymbol, start_name);
		if (!openTag.notFound()) {
			EDTOffset span_start, span_end;

			// The span starts after the open tag.
			span_start = source.firstStartOffsetStartingAt<EDTOffset>(openTag.getEnd());

			// The span ends before the close tag.
			closeTag = SGMLTag::findCloseTag(source, spanTypeSymbol, openTag.getEnd());
			if (closeTag.notFound()) {
				span_end = source.lastEndOffsetEndingAt<EDTOffset>(source.length() - 1);
				start_name = source.length();
			}
			else {
				span_end = source.lastEndOffsetEndingAt<EDTOffset>(closeTag.getStart() - 1);
				start_name = closeTag.getEnd();
			}

			// Note that this will pass L"" as the fourth parameter
			// for <SENT> tags, but this will be harmlessly ignored.
			IdfSpan * span = (IdfSpan*)metadata.newSpan(
				spanTypeSymbol,
				span_start,
				span_end,
				openTag.getAttributes());

			// Name types: insert span elements into a vector sorted by the start offset
			if ((wcscmp(spanTypeSymbol.to_string(), L"ENAMEX") == 0) ||
				(wcscmp(spanTypeSymbol.to_string(), L"TIMEX") == 0) ||
				(wcscmp(spanTypeSymbol.to_string(), L"NUMEX") == 0)) {
					if (_spans->size() == 0)
						_spans->push_back(span);
					else {
						bool inserted = false;
						for (size_t i = 0; i < _spans->size(); i++) {
							IdfSpan * cur_span = _spans->at(i);
							if (cur_span->getStartOffset() > span->getStartOffset()) {
								_spans->insert(_spans->begin() + i, span);
								inserted = true;
								break;
							}
						}
						if (!inserted)
							_spans->push_back(span);
					}
				}
		}
	} while (!openTag.notFound());
}

Symbol DocumentReader::cleanFileName(const char* file) const {
	int i, j;
	wchar_t* w_file = _new wchar_t[strlen(file)+1];
	
	for (i = static_cast<int>(strlen(file)); i > 1; i--) {
		if (file[i-1] == '/' || file[i-1] == '\\') 
			break;
	}
	for (j = 0; j + i < static_cast<int>(strlen(file)); j++)
		w_file[j] = (wchar_t)file[j+i];
	w_file[j] = L'\0';
	
	Symbol name = Symbol(w_file);
	delete [] w_file;

	return name;
}

Symbol DocumentReader::removeFileExtension(const char* file) const {
	size_t i, j;
	wchar_t* w_file = _new wchar_t[strlen(file)+1];
	
	for (i = strlen(file); i > 1; i--) {
		if (file[i-1] == '.' ) 
			break;
	}
	for (j = 0; j < (i-1); j++)
		w_file[j] = (wchar_t)file[j];
	w_file[j] = L'\0';
	
	Symbol name = Symbol(w_file);
	delete [] w_file;

	return name;
}


//} // namespace DataPreprocessor
