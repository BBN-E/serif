// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// StandaloneSentenceBreaker.cpp : Defines the entry point for the console application.
//
#include "common/leak_detection.h"

#include <stdio.h>
#if defined(WIN32) || defined(WIN64)
#include <crtdbg.h>
#endif

#include "common/UnrecoverableException.h"
#include "common/ParamReader.h"
#include "common/HeapChecker.h"
#include "theories/Document.h"
#include "theories/DocTheory.h"
#include "sentences/SentenceBreaker.h"
#include "common/UTF8InputStream.h"
#include "common/TeeSessionLogger.h"
#include "common/UTF8Token.h"
#include "DocumentReader.h"
#include "common/StringTransliterator.h"
#include <boost/scoped_ptr.hpp>

//using namespace DataPreprocessor;
//using Metadata::SpanList;

typedef Symbol::HashMap<Symbol> TypeTable;

TypeTable * _types;
SpanVector *_spans;

Document* loadDocument(DocumentReader &reader) {

	Document* doc = reader.readNextDocument();
	if (doc == 0)
		return doc;

	SpanVector * name_spans = reader.getNameSpans();
	_spans = _new SpanVector();
	for (size_t i = 0; i < name_spans->size(); i++) {
		_spans->push_back(name_spans->at(i));
	}

	/*
	boost::scoped_ptr<UTF8InputStream> docStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& docStream(*docStream_scoped_ptr);
	docStream.open(document_file);
	Document* doc = _documentReader->readDocument(docStream);
	docStream.close();
*/
	return doc;
}

void replaceWhiteSpace(std::wstring str){
	for (std::wstring::iterator it=str.begin(); it != str.end(); ++it) {
		if(iswspace((*it)) && (*it)!='\n'){
			(*it) = L' ';
		}
	}
}

void addAnnotation(UTF8OutputStream &uos, Sentence * sent) {
	if (_spans->size() == 0)
		return;

	EDTOffset sent_first_edt_offset = sent->getString()->firstStartOffsetStartingAt<EDTOffset>(0);
	EDTOffset sent_last_edt_offset = sent->getString()->lastEndOffsetEndingAt<EDTOffset>(sent->getNChars() - 1);
	while(_spans->size() > 0) {
		IdfSpan * span = _spans->at(0);
		TypeTable::iterator iter;
		iter = _types->find(span->getType());
		if (iter == _types->end()) {
			// remove span with a type that's not in the list of provided types
			_spans->erase(_spans->begin());
			continue;
		}
		if ((span->getStartOffset() >= sent_first_edt_offset) && (span->getStartOffset() <= sent_last_edt_offset)) {
			int sent_index = 0;
			// find the starting character of the span in the sentence
			while ((sent->getString()->firstStartOffsetStartingAt<EDTOffset>(sent_index) < span->getStartOffset()) && (sent_index < sent->getNChars())) {
				sent_index++;
			}
			if (sent_index == sent->getNChars()) {
				// This condition should never happen, but just in case. Bad span.
				_spans->erase(_spans->begin());
				break;
			}

			int start_ann = sent_index;
			int end_ann = sent_index;
			while ((sent_index < sent->getNChars()) && (sent->getString()->lastEndOffsetEndingAt<EDTOffset>(sent_index) <= span->getEndOffset())) {
				// find the ending character of the span in the sentence
				sent_index++;
			}

			end_ann = sent_index - 1;
			uos << "<ANNOTATION TYPE=\"" << span->getType().to_debug_string() << "\" START_OFFSET=\""
				<< start_ann << "\" END_OFFSET=\"" << end_ann << "\"/>\n";
			_spans->erase(_spans->begin());
		} else
			// _spans is sorted by start offset
			break;
	}

}

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "StandaloneSentenceBreaker.exe should be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}
	try {
		ParamReader::readParamFile(argv[1]);
		//_documentReader = new DocumentReader();

		std::string log_file = ParamReader::getRequiredParam("sb_session_log");
		std::wstring log_file_as_wstring(log_file.begin(), log_file.end());

		//SessionLogger* _sessionLogger = new FileSessionLogger(log_file_as_wstring.c_str(), 0,0);
		//SessionLogger::logger = _sessionLogger; // Make it the default logger
		
		std::vector<std::wstring>  context_level_names;
	    TeeSessionLogger * _sessionLogger = _new TeeSessionLogger(log_file_as_wstring.c_str(), context_level_names, 0);
	    SessionLogger::setGlobalLogger(_sessionLogger, /*store_old_logger=*/false);

		SentenceBreaker* _sentenceBreaker = SentenceBreaker::build();

		int total_parallel = ParamReader::getOptionalIntParamWithDefaultValue("total_parallel", 1);
		int parallel = ParamReader::getOptionalIntParamWithDefaultValue("parallel", 0);

		char document_filelist[500];
		if(!ParamReader::getParam("sb_document_filelist",document_filelist, 500)){
			throw UnexpectedInputException(
			"StandaloneSentenceBreaker::Main()",
			"Missing Parameter: sb-document-filelist");
		}

		char tag_set_file[500];
		if(!ParamReader::getParam("pidf_tag_set_file",tag_set_file, 500)) {
			std::cerr << "Warning! Missing Parameter: pidf-tag-set-file. All annotation will be removed.\n";
		} else {
			// load name tags
			char msg[1000];
			boost::scoped_ptr<UTF8InputStream> instream_scoped_ptr(UTF8InputStream::build());
			UTF8InputStream& instream(*instream_scoped_ptr);
			if(instream.fail()){
				strncpy(msg, "Couldn't open names tag list: ",1000);
				strcat(msg, tag_set_file);
				throw UnexpectedInputException("StandaloneSentenceBreaker::Main()", msg);
			}
			instream.open(tag_set_file);
			int ntags;
			instream >> ntags;
			_types = _new TypeTable();
			UTF8Token token;
			for(int i = 0; i < ntags; i++){
				if (instream.eof())
					throw UnexpectedInputException("StandaloneSentenceBreaker::Main()", "fewer name tags than specified in file");
				instream >> token;
				(*_types)[token.symValue()] = Symbol();
			}
			instream.close();
		}

		char outfile[500];
		if(!ParamReader::getParam("sb_outfile",outfile, 500)){
			throw UnexpectedInputException(
			"StandaloneSentenceBreaker::Main()",
			"Missing Parameter: sb-outfile");
		}
		UTF8OutputStream uos;
		char msg[1000];

		uos.open(outfile);
		if(uos.fail()){
			strcpy(msg, "Couldn't open document output file: ");
			strcat(msg, outfile);
			throw UnexpectedInputException(
			"StandaloneSentenceBreaker::Main()",
			msg);
		}
		boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& uis(*uis_scoped_ptr);
		if(uis.fail()){
			strcpy(msg, "Couldn't open document file list: ");
			strcat(msg, document_filelist);
			throw UnexpectedInputException(
			"StandaloneSentenceBreaker::Main()",
			msg);
		}
		uis.open(document_filelist);
		int nfiles;
		uis >> nfiles;
		UTF8Token token;
		Sentence *sentences[MAX_DOCUMENT_SENTENCES];

		for(int i =0; i<nfiles; i++){
			if (uis.eof())
				throw UnexpectedInputException("StandaloneSentenceBreaker::Main()",
					"fewer document files than specified in file");
			uis >> token;
			char filename[1000];
			StringTransliterator::transliterateToEnglish(filename, token.symValue().to_string(), 1000);
			
			if (i % total_parallel != parallel)
				continue;

			cout<<"Loading documents from: " << filename << endl;
			
			DocumentInputStream in(filename);
			DocumentReader reader(in, filename);

			Document* document;

			while ((document = loadDocument(reader)) != 0) {
				_sentenceBreaker->resetForNewDocument(document);
				int n_sentences = _sentenceBreaker->getSentences(
					sentences, MAX_DOCUMENT_SENTENCES,
					document->getRegions(), document->getNRegions());
				cerr<<"n_sentences: "<<n_sentences<<endl;
				if (n_sentences > 0) {
					uos << L"<DOC>\n<DOCID>"<<document->getName().to_string()<<"</DOCID>\n";
					for(int j =0; j< n_sentences; j++){
						uos << "<SENTENCE ID=\"" << document->getName().to_string() << "-" << j << "\">\n";
						uos << " <DISPLAY_TEXT>\n";
						std::wstring txt_buff(sentences[j]->getString()->toWString());
						replaceWhiteSpace(txt_buff);
						uos << txt_buff;
						uos << "\n </DISPLAY_TEXT>\n";
						addAnnotation(uos, sentences[j]);
						uos << "</SENTENCE>\n";
					}
					for(int j =0; j< n_sentences; j++){
						delete sentences[j];
						sentences[j] = 0;
					}
					uos<<"</DOC>\n";
				}
				delete document;
			}
			cout<<"Finished loading" << endl;
			in.close();
		}
		delete _types;
		delete _sentenceBreaker;
	}

	catch (UnrecoverableException &e) {
		std::cerr << "\n" << e.getSource() << ": " << e.getMessage() << "\n";
		HeapChecker::checkHeap("main(); About to exit due to error");

#ifdef _DEBUG
		std::cerr << "Press enter to exit....\n";
		getchar();
#endif

		return -1;
	}

	HeapChecker::checkHeap("main(); About to exit after successful run");

#if 0
	ParamReader::finalize();

	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtDumpMemoryLeaks();
#endif

#ifdef _DEBUG
	std::cerr << "Press enter to exit....\n";
	getchar();
#endif

	return 0;
}
