// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "KbaStreamCorpus/KbaStreamCorpusChunk.h"

#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/BoostUtil.h"

#ifndef WIN32
#include <netinet/in.h>
#endif
#include <fcntl.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TDenseProtocol.h>
#include <thrift/protocol/TJSONProtocol.h>
#pragma warning(push, 0)
#include <thrift/transport/TTransportUtils.h>
#pragma warning(pop)
#include <thrift/transport/TFDTransport.h>

#include "KbaStreamCorpus/thrift-cpp-gen/streamcorpus_types.h"
#include <sstream>
#include <boost/lexical_cast.hpp>

#ifdef WIN32
#include <io.h>
#include <sys/stat.h>
static const int S_IRUSR = int(_S_IREAD);
static const int S_IWUSR = int(_S_IWRITE);
static const int S_IXUSR = 0x00400000;
#endif

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

KbaStreamCorpusChunk::KbaStreamCorpusChunk(const char* path, int flags)
 : _path(path)
{
    boost::filesystem::path bpath(path);
	_filename = BOOST_FILESYSTEM_PATH_GET_FILENAME(bpath);
	int fd = open(path,  flags, S_IRUSR | S_IWUSR | S_IXUSR);
	if (-1 == fd) {
		throw UnexpectedInputException("KbaStreamCorpusChunk::KbaStreamCorpusChunk",
					   "Unable to open file ", path);
	}
	innerTransport.reset(_new TFDTransport(fd));
	transport.reset(_new TBufferedTransport(innerTransport));
	protocol.reset(_new TBinaryProtocol(transport));
	transport->open();
}

void KbaStreamCorpusChunk::writeItem(const streamcorpus::StreamItem &item) {
	item.write(protocol.get());
}

bool KbaStreamCorpusChunk::eof() {
	return (!transport->peek());
}

bool KbaStreamCorpusChunk::readItem(streamcorpus::StreamItem &item) {
	try {
		if (eof())
			return false;
		// Make sure the passed-in item is cleared before reading, otherwise an empty
		// field in the chunk will fail to overwrite previous contents
		streamcorpus::StreamItem emptyItem;
		swap(item, emptyItem);
		item.read(protocol.get());
		return true;
	} catch (std::exception &e) {
		SessionLogger::err("streamcorpus") 
			<< "Error while reading item from streamcorpus chunk: " 
			<< e.what();
		return false;
	}
}
	
void KbaStreamCorpusChunk::close() {
	transport->flush();
	transport->close();
}

namespace {
	static const int ITEM_NUM_WIDTH = 8;
}

std::string KbaStreamCorpusChunk::getSerifXMLItemFile(int item_num, const char *root)
{
	std::ostringstream out;
	if (root)
		out << root << SERIF_PATH_SEP;
	out << _filename << "-" << std::setfill('0') 
		<< std::setw(ITEM_NUM_WIDTH) << item_num << ".xml";
	return out.str();
}

std::pair<std::string, int> parseSerifXMLItemFile(std::string path) {
    boost::filesystem::path bpath(path);
    std::string filename = BOOST_FILESYSTEM_PATH_GET_FILENAME(bpath);
	if (filename.size() < (ITEM_NUM_WIDTH+1))
		return std::pair<std::string, int>("",-1);
	try {
		int item_num = boost::lexical_cast<int>(filename.substr(filename.size()-ITEM_NUM_WIDTH));
		std::string chunk_name = filename.substr(0, filename.size()-ITEM_NUM_WIDTH-1);
		return std::pair<std::string, int>(chunk_name, item_num);
	} catch (std::exception) {
		return std::pair<std::string, int>("",-1);
	}
}


