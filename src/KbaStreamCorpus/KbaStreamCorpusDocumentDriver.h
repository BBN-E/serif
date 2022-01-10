// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KBA_STREAM_CORPUS_DOCUMENT_DRIVER_H
#define KBA_STREAM_CORPUS_DOCUMENT_DRIVER_H

#include "Generic/driver/DocumentDriver.h"
#include "KbaStreamCorpus/KbaStreamCorpusItemReader.h"
#include "KbaStreamCorpus/KbaStreamCorpusItemWriter.h"
#include <boost/scoped_ptr.hpp>

class KbaStreamCorpusChunk;
namespace streamcorpus { class StreamItem; }

class KbaStreamCorpusDocumentDriver: public DocumentDriver {
public:
	KbaStreamCorpusDocumentDriver();
	void runOnStreamCorpusChunks();

private:
	KbaStreamCorpusItemWriter _itemWriter;
	KbaStreamCorpusItemReader _itemReader;
	size_t _max_splits_to_process;
	bool _write_to_chunk;
	bool _read_from_chunk;
	bool _write_to_serifxml;
	std::string _outputDir;
	std::string _serifxmlInputDir;

	typedef boost::scoped_ptr<KbaStreamCorpusChunk> KbaStreamCorpusChunk_ptr;
	void processItem(streamcorpus::StreamItem &item,
					 std::string &srcSerifxml, std::string &dstSerifxml);
	void cleanupChunk(KbaStreamCorpusChunk_ptr& src, KbaStreamCorpusChunk_ptr &dst);
	void processChunk(const std::string &inputPath);

};

#endif
