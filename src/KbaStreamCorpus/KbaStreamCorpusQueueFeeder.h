// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KBA_STREAM_CORPUS_QUEUE_FEEDER_H
#define KBA_STREAM_CORPUS_QUEUE_FEEDER_H

#ifndef WIN32
#include <netinet/in.h>
#endif
#include <fcntl.h>

#include <string>
#include "Generic/driver/QueueDriver.h"
#include "Generic/driver/Batch.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Document.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TDenseProtocol.h>
#include <thrift/protocol/TJSONProtocol.h>
#pragma warning(push, 0)
#include <thrift/transport/TTransportUtils.h>
#pragma warning(pop)
#include <thrift/transport/TFDTransport.h>

#include "KbaStreamCorpus/thrift-cpp-gen/streamcorpus_types.h"
#include "KbaStreamCorpus/KbaStreamCorpusItemReader.h"
#include "KbaStreamCorpus/KbaStreamCorpusChunk.h"

#include <boost/shared_ptr.hpp>

template<class QueueDriverBase>
class KbaStreamCorpusQueueFeeder: public QueueDriverBase {
public:
	KbaStreamCorpusQueueFeeder()
		: _next_chunk(0), _done(false), _src(), _item_num(0) {}
	~KbaStreamCorpusQueueFeeder() {}
	
private:
	size_t _next_chunk;
	int _item_num;
	bool _done;
	boost::scoped_ptr<KbaStreamCorpusChunk> _src;
	KbaStreamCorpusItemReader _reader;

	virtual bool dstIsFull() {
		// Once we start a chunk, we process the whole thing.
		return ((!_src->eof()) && QueueDriverBase::dstIsFull());
	}

	bool openNextChunk() {
		const Batch* batch = QueueDriverBase::_sessionProgram->getBatch();
		if (_next_chunk == batch->getNDocuments())
			return false; // no more documents.

		std::string path = UnicodeUtil::toUTF8StdString(batch->getDocumentPath(_next_chunk));
		_src.reset(_new KbaStreamCorpusChunk(path.c_str(), CHUNK_READ_FLAGS));
		++_next_chunk;
		_item_num = 0;
		return true;
	}

	virtual QueueDriver::Element_ptr next() {
		streamcorpus::StreamItem item;
		while (!_done) {
			// If there is no current chunk, then open the next one.  If
			// there is no next chunk, then we're done.
			if ((!_src) || (_src->eof())) {
				_done = !openNextChunk();
			}
			if (_src->readItem(item)) {
				if (!_reader.skipItem(item)) {
					KbaStreamCorpusItemReader::DocPair docPair = _reader.itemToDocPair(item);
					std::string docName(_src->getSerifXMLItemFile(_item_num));
					return boost::make_shared<QueueDriver::Element>(docName, docPair.first,
																	docPair.second);
				}
				++_item_num;
			} else {
				// error reading item from chunk; move on to the next one.
				_src.reset();
			}
		}
		return QueueDriver::Element_ptr();
	}
	
	/** This queue feeder reads directly from the Batch; it does not
	 * use a queue source directory.*/
	bool hasSource() { return false; }

	bool gotQuitSignal() {
		if (_done) {
			this->markDstAsDone();
			this->markSrcAsDone();
			return true;
		} else {
			return QueueDriverBase::gotQuitSignal();
		}
	}
};

#endif
