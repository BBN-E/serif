// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <wchar.h>
#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include <boost/scoped_ptr.hpp>

const float WordClusterTable::targetLoadingFactor = static_cast<float>(0.7);
const int WordClusterTable::numBuckets = static_cast<int>(64000 / targetLoadingFactor);

WordClusterTable::ClusterTablePair WordClusterTable::_clusterTable;
WordClusterTable::ClusterTablePair WordClusterTable::_domainClusterTable;
WordClusterTable::ClusterTablePair WordClusterTable::_secondaryClusterTable;

WordClusterTable::ClusterTablePair WordClusterTable::initializeTable(const char *bits_file_param, const char *lc_bits_file_param)
{
	WordClusterTable::ClusterTablePair result;

	std::string bits_file = ParamReader::getRequiredParam(bits_file_param);
	std::string lc_bits_file = ParamReader::getParam(lc_bits_file_param);
	bool lc_active = (!lc_bits_file.empty());
	result._mixedcaseTable = _new ClusterTable(numBuckets<5?5:numBuckets);

	bool warningprinted = false;
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(bits_file.c_str()));
	UTF8InputStream& in(*in_scoped_ptr);
	UTF8Token token;
	wchar_t bitstring[200];
	int linecount =0;
	while (!in.eof()) {
		in >> token;
		Symbol word = token.symValue();
		in >> token;
		wcsncpy(bitstring, token.chars(), 200);
		int bitstring_len = static_cast<int>(wcslen(bitstring));
		linecount++;
		//only print this warning the first time since these errors tend to propagate
		if((bitstring_len >  1) && (wcstol(bitstring, 0, 2) == 0)){
			if(!warningprinted){
				SessionLogger::warn("word_cluster_table") <<
					"**WordClusterClass::init(), error in cluster file at line "
					<< linecount << " bitstring " << bitstring << " is not a number "
					<< "(word: " << word.to_debug_string() << ")" <<  std::endl;
			}
			warningprinted = true;
		}
		else{
			warningprinted = false;
		}
		if (bitstring_len < 32)
			(*(result._mixedcaseTable))[word] = wcstol(bitstring, 0, 2);
		else {
			/*wchar_t message[500];
			swprintf(message,
					L"WordClusterClass::init(), Bitstring for word %ls too long - truncating to 32 bits.",
					word.to_string());
			*SessionLogger::logger << message << "\n";*/
			bitstring[31] = '\0';
			(*(result._mixedcaseTable))[word] = wcstol(bitstring, 0, 2);
		}
		wcscpy(bitstring, L"\0");
	}
	in.close();

	if (lc_active)  {
		result._lowercaseTable = _new ClusterTable(numBuckets<5?5:numBuckets);
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(lc_bits_file.c_str()));
		UTF8InputStream& in(*in_scoped_ptr);
		UTF8Token token;
		wchar_t bitstring[200];

		while (!in.eof()) {
			in >> token;
			Symbol word = token.symValue();
			in >> token;
			wcsncpy(bitstring, token.chars(), 200);
			int bitstring_len = static_cast<int>(wcslen(bitstring));
			if (bitstring_len < 32)
				(*(result._lowercaseTable))[word] = wcstol(bitstring, 0, 2);
			else {
				bitstring[31] = '\0';
				(*(result._lowercaseTable))[word] = wcstol(bitstring, 0, 2);
			}
		}
		in.close();
	}
	return result;
}

void WordClusterTable::ensureInitializedFromParamFile() {
	if (isInitialized())
		return;

	// Load the word cluster file.
	_clusterTable = initializeTable("word_cluster_bits_file",
									"lc_word_cluster_bits_file");

	// Load the domain-specific clusters
	if (ParamReader::hasParam("domain_word_cluster_bits_file")) {
		_domainClusterTable = initializeTable("domain_word_cluster_bits_file",
											  "domain_lc_word_cluster_bits_file");
	}

	// Load the secondary-specific clusters
	if (ParamReader::hasParam("secondary_word_cluster_bits_file")) {
		_secondaryClusterTable = initializeTable("secondary_word_cluster_bits_file",
												 "secondary_lc_word_cluster_bits_file");
	}
}

int* WordClusterTable::ClusterTablePair::get(Symbol word, bool lowercase) { 
	if (_mixedcaseTable == NULL)
		throw UnexpectedInputException("WordClusterTable::get()",
									   "Attempt to access uninitialized word clusters - was pidf_use_clusters inadvertently set to false?");
	if (lowercase && (_lowercaseTable != NULL))
		return _lowercaseTable->get(word); 
	else
		return _mixedcaseTable->get(word); 
}
