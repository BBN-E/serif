// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KBA_STREAM_CORPUS_ITEM_READER_H
#define KBA_STREAM_CORPUS_ITEM_READER_H

#include "KbaStreamCorpus/thrift-cpp-gen/streamcorpus_types.h"
#include "Generic/reader/DocumentReader.h"
#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>

class Document;
class DocTheory;
class DocumentReader;

class KbaStreamCorpusItemReader {
public:
	KbaStreamCorpusItemReader();

	typedef std::pair<const Document*, DocTheory*> DocPair;
    DocPair itemToDocPair(streamcorpus::StreamItem& item);
    bool skipItem(streamcorpus::StreamItem& item);

	std::string getSourceName(const streamcorpus::StreamItem& item) const;

 private:

	typedef enum {RAW, CLEAN_HTML, CLEAN_VISIBLE, NO_SOURCE} Source;
	std::string getSourceName(Source source) const;
	std::pair<Source, const std::string*> selectSource(const streamcorpus::StreamItem& item) const;

	std::map<Source, boost::shared_ptr<DocumentReader> > _documentReaders;
	std::vector<Source> _sourceList;

	bool _skip_docs_with_empty_language_code;
	bool _read_serifxml_from_chunk;
};

#endif
