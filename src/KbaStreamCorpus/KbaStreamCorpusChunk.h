// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KBA_STREAM_CORPUS_CHUNK_H
#define KBA_STREAM_CORPUS_CHUNK_H

#include "Generic/driver/DocumentDriver.h"
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

// Use binary mode to read/write chunks on Windows
#ifdef WIN32
#define CHUNK_READ_FLAGS O_RDONLY | O_BINARY
#define CHUNK_WRITE_FLAGS O_CREAT | O_TRUNC | O_WRONLY | O_BINARY
#else
#define CHUNK_READ_FLAGS O_RDONLY
#define CHUNK_WRITE_FLAGS O_CREAT | O_TRUNC | O_WRONLY
#endif

namespace apache { namespace thrift { namespace transport { 
			class TTransport;
			class TFDTransport;
			class TBufferedTransport; 
		}}}
namespace apache { namespace thrift { namespace protocol { 
			template <typename T> class TBinaryProtocolT; 
			typedef TBinaryProtocolT<apache::thrift::transport::TTransport> TBinaryProtocol;
		}}}
namespace streamcorpus { 
	class StreamItem; 
}


typedef boost::shared_ptr<apache::thrift::protocol::TBinaryProtocol> TBinaryProtocol_ptr;
typedef boost::shared_ptr<apache::thrift::transport::TBufferedTransport> TBufferedTransport_ptr;
typedef boost::shared_ptr<apache::thrift::transport::TFDTransport> TFDTransport_ptr;
 
class KbaStreamCorpusChunk: private boost::noncopyable {
public:
	KbaStreamCorpusChunk(const char* path, int flags);
	void writeItem(const streamcorpus::StreamItem &item);
	bool readItem(streamcorpus::StreamItem &item);
	bool eof();
	void close();
	const std::string &getPath() { return _path; }

	std::string getSerifXMLItemFile(int item_num, const char* root=0);
	static std::pair<std::string, int> parseSerifXMLItemFile(std::string filename);
private:
	std::string _path;
	std::string _filename; // just the file part of the path.
	TFDTransport_ptr innerTransport;
	TBufferedTransport_ptr transport;
	TBinaryProtocol_ptr protocol;
};

#endif
