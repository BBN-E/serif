// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "KbaStreamCorpus/KbaStreamCorpusDocumentDriver.h"
#include "KbaStreamCorpus/KbaStreamCorpusItemWriter.h"
#include "KbaStreamCorpus/KbaStreamCorpusItemReader.h"
#include "KbaStreamCorpus/KbaStreamCorpusChunk.h"

#include "Generic/common/SessionLogger.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Document.h"
#include "Generic/reader/DocumentSplitter.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/driver/Stage.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/results/SerifXMLResultCollector.h"
#include "Generic/common/HeapStatus.h"
#include <fcntl.h>

#include "KbaStreamCorpus/thrift-cpp-gen/streamcorpus_types.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

typedef std::pair<const Document*, DocTheory*> DocPair;
typedef boost::scoped_ptr<KbaStreamCorpusChunk> KbaStreamCorpusChunk_ptr;

KbaStreamCorpusDocumentDriver::KbaStreamCorpusDocumentDriver()
	: _read_from_chunk(false), _write_to_chunk(false), _write_to_serifxml(false) {
	_max_splits_to_process = ParamReader::getOptionalIntParamWithDefaultValue("max_splits_to_process", 0);
}

void KbaStreamCorpusDocumentDriver::cleanupChunk(KbaStreamCorpusChunk_ptr& src,
												 KbaStreamCorpusChunk_ptr &dst) 
{
	if (src) {
		src->close();
		src.reset();
	}
	if (dst) {
		dst->close();
		dst.reset();
	}
}


void KbaStreamCorpusDocumentDriver::processItem(streamcorpus::StreamItem &item,
												std::string &srcSerifxml,
												std::string &dstSerifxml) {
	_localSessionLogger->updateContext(DOCUMENT_CONTEXT, item.doc_id.c_str());
	if (!_itemReader.skipItem(item)) {
		HeapStatus heapStatus;
		heapStatus.takeReading("Before document");
		// Read the serifxml input.  This input can come from either the 
		// chunk file (if we're reading from a chunk) or from a serifxml
		// file (if we're resuming processing from the output of a 
		// previous process). We split the document if necessary.
		GenericTimer docTimer;
		docTimer.startTimer();
		std::vector<DocTheory*> docTheories;
		if (_read_from_chunk) {
			DocPair docPair = _itemReader.itemToDocPair(item);
			std::vector<Document*> documents = _documentSplitter->splitDocument(const_cast<Document*>(docPair.first));

            // ONLY re-initialize the doctheory if we've split the document
            if (documents.size() > 1) {
              delete docPair.second;
              BOOST_FOREACH(Document* document, documents) {
				docTheories.push_back(_new DocTheory(document));
              }
            } else {
              docTheories.push_back(docPair.second);
            }
		} else {
			// [xx] What if the file does not exist?
			SerifXML::XMLSerializedDocTheory serialized(srcSerifxml.c_str());
			DocPair docPair = serialized.generateDocTheory();
			docTheories.push_back(docPair.second);
		}
		
		// Process each split document; most of the time this will be a single document
		try {
			for (size_t d = 0; d < docTheories.size(); d++) {
				// Optionally process only the first few split documents, to keep memory usage down
				if (_max_splits_to_process > 0 && d >= _max_splits_to_process) {
					_localSessionLogger->updateContext(DOCUMENT_CONTEXT, item.doc_id.c_str());
					_localSessionLogger->dbg("kba-stream-corpus") << "Truncating document after " << d << "/" << docTheories.size() << " subdocuments";
					break;
				}

				// Process this split document
				DocTheory* docTheory = docTheories.at(d);
				_localSessionLogger->updateContext(DOCUMENT_CONTEXT, 
												   docTheory->getDocument()->getName().to_string());
				if (_localSessionLogger->dbg_or_msg_enabled("kba-stream-corpus-item"))
					_localSessionLogger->dbg("kba-stream-corpus-item") << "[memory] Before subdocument: " << heapStatus.getHeapSize();
				runOnDocTheory(docTheory);
			}
		} catch (...) {
			BOOST_FOREACH(DocTheory* docTheory, docTheories) {
				delete docTheory->getDocument();
				delete docTheory;
			}
			throw;
		}

		// Recombine the split document
		if (_localSessionLogger->dbg_or_msg_enabled("kba-stream-corpus-item"))
			_localSessionLogger->dbg("kba-stream-corpus-item") << "[memory] Before merge: " << heapStatus.getHeapSize();
		DocTheory* mergedDocTheory = _documentSplitter->mergeDocTheories(docTheories);
		if (_localSessionLogger->dbg_or_msg_enabled("kba-stream-corpus-item"))
			_localSessionLogger->dbg("kba-stream-corpus-item") << "[memory] After merge: " << heapStatus.getHeapSize();
		_localSessionLogger->updateContext(DOCUMENT_CONTEXT, mergedDocTheory->getDocument()->getName().to_string());
		if (_write_to_chunk) {
			// If we are writing to a chunk, then add SERIF's output 
			// to the streamcorpus item.
			_itemWriter.addDocTheoryToItem(mergedDocTheory, &item);
		} 
		if (_write_to_serifxml) {
			SerifXML::XMLSerializedDocTheory(mergedDocTheory).save(dstSerifxml);
		}
		docTimer.stopTimer();
		_localSessionLogger->info("kba-stream-corpus") << "Processed " << mergedDocTheory->getDocument()->getOriginalText()->length() << " characters in " << docTimer.getTime() << " msec";
		delete mergedDocTheory->getDocument();
		delete mergedDocTheory;

		heapStatus.takeReading("After document");

		if (_localSessionLogger->dbg_or_msg_enabled("kba-stream-corpus-item")) {
			heapStatus.displayReadings();
		}
	}
}

void KbaStreamCorpusDocumentDriver::processChunk(const std::string& inputPath) 
{
    boost::filesystem::path inputBPath(inputPath);
    boost::filesystem::path outputBPath(_outputDir);
	std::string outputPath = (outputBPath / inputBPath.filename()).string();
	KbaStreamCorpusChunk_ptr src;
	KbaStreamCorpusChunk_ptr dst;
	streamcorpus::StreamItem item;
	try {
		src.reset(_new KbaStreamCorpusChunk(inputPath.c_str(), CHUNK_READ_FLAGS));
		if (_write_to_chunk)
			dst.reset(_new KbaStreamCorpusChunk(outputPath.c_str(), CHUNK_WRITE_FLAGS));
		for (int item_num = 0; src->readItem(item); ++item_num) {
			try {
				// Decide which serifxml files to use (if any)
				std::string srcSerifxml, dstSerifxml;
				if (!_read_from_chunk)
					srcSerifxml=src->getSerifXMLItemFile(item_num, _serifxmlInputDir.c_str());
				if (_write_to_serifxml)
					dstSerifxml = src->getSerifXMLItemFile(item_num, _outputDir.c_str());
				// Process the item.
				processItem(item, srcSerifxml, dstSerifxml);
				// Save the result.
				if (_write_to_chunk) 
					dst->writeItem(item);
			} catch (UnrecoverableException &e) {
				if (_ignore_errors) {
					_localSessionLogger->reportError() << e;
					std::cout << "Skipping over " << inputPath << ":" << item_num
						<< " due to ignore_errors=true\n";
				} else {
					throw;
				}
			} catch (std::exception &e) {
				if (_ignore_errors) {
					_localSessionLogger->reportError() << e.what();
					std::cout << "Skipping over " << inputPath << ":" << item_num
						<< " due to ignore_errors=true\n";
				} else {
					throw;
				}
			} catch (...) {
				if (_ignore_errors) {
					std::cout << "Skipping over " << inputPath << ":" << item_num
						 << " due to ignore_errors=true\n";
				} else {
					throw;
				}
			}
		}
		cleanupChunk(src, dst);
	} catch (UnrecoverableException &e) {
		cleanupChunk(src, dst);
		if (_ignore_errors) {
			_localSessionLogger->reportError() << e;
			std::cout << "Skipping over " << inputPath << " due to ignore_errors=true\n";
		} else {
			throw;
		}
	} catch (std::exception &e) {
		cleanupChunk(src, dst);
		if (_ignore_errors) {
			_localSessionLogger->reportError() << e.what();
			std::cout << "Skipping over " << inputPath << " due to ignore_errors=true\n";
		} else {
			throw;
		}
	} catch (...) {
		cleanupChunk(src, dst);
		if (_ignore_errors) {
			std::cout << "Skipping over " << inputPath << " due to ignore_errors=true\n";
		} else {
			throw;
		}
	}
}

void KbaStreamCorpusDocumentDriver::runOnStreamCorpusChunks() {
	bool ignore_errors = ParamReader::isParamTrue("ignore_errors");

	HeapStatus heapStatus;
	heapStatus.takeReading("Before batch");

    logSessionStart();
	_outputDir = UnicodeUtil::toUTF8StdString(_sessionProgram->getOutputDir());

	if (!_sessionProgram->hasExperimentDir())
		throw UnexpectedInputException("KbaStreamCorpusDocumentDriver::runOnStreamCorpusChunks",
					   "The KBA streamcorpus document driver requires an experiment dir.");

	_read_from_chunk = ((_sessionProgram->getStartStage() <= Stage("START")) ||
						ParamReader::isParamTrue("kba_read_serifxml_from_chunk"));
	_write_to_chunk = ((_sessionProgram->getEndStage() >= Stage("output")) ||
					   ParamReader::isParamTrue("kba_write_serifxml_to_chunk"));
	_write_to_serifxml = (!_write_to_chunk || ParamReader::isParamTrue("kba_force_serifxml_output"));

	if (!(_read_from_chunk||_write_to_chunk))
		throw UnexpectedInputException("KbaStreamCorpusDocumentDriver::runOnStreamCorpusChunks",
					   "The KBA streamcorpus document driver should be used with a start "
					   "stage of 'START' or an end stage of 'output'.");

	if (!_read_from_chunk) {
		_serifxmlInputDir = ParamReader::getRequiredParam("serifxml_input_directory");
		if (!boost::iequals(ParamReader::getParam("source_format"), "serifxml"))
			throw UnexpectedInputException("KbaStreamCorpusDocumentDriver::runOnStreamCorpusChunks",
						   "The KBA streamcorpus document driver requires source_format=serifxml");
	} else {
		_serifxmlInputDir = "";
	}

    try {
		const Batch* batch = _sessionProgram->getBatch();
		// each "document" in the batch is actually a chunk file that
		// contains a series of documents to process ("items").
		for (size_t doc_num = 0; doc_num < batch->getNDocuments(); ++doc_num) {
			std::string inputPath(UnicodeUtil::toUTF8StdString(batch->getDocumentPath(doc_num)));
			processChunk(inputPath);
		}
    } catch (UnrecoverableException &e) {
        _localSessionLogger->reportError() << e;
        throw;
	} catch (std::exception &e) {
		_localSessionLogger->reportError() << e.what();
		throw;
	}

	heapStatus.takeReading("After batch");
	_localSessionLogger->updateContext(SESSION_CONTEXT, _sessionProgram->getSessionName());
	heapStatus.displayReadings();
	heapStatus.flushReadings();
    _localSessionLogger->displaySummary();
}





