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
#include "Generic/WikipediaClustering/WikipediaClusterTable.h"
#include <boost/scoped_ptr.hpp>

const float WikipediaClusterTable::targetLoadingFactor = static_cast<float>(0.7);
const int WikipediaClusterTable::numBuckets = static_cast<int>(64000 / targetLoadingFactor);
WikipediaClusterTable::ClusterTable WikipediaClusterTable::_table(numBuckets < 5 ? 5 : numBuckets);
WikipediaClusterTable::ClusterTable WikipediaClusterTable::_lowercaseTable(numBuckets < 5 ? 5 : numBuckets);
WikipediaClusterTable::PerplexityTable WikipediaClusterTable::_perplexTable(numBuckets < 5 ? 5 : numBuckets);
WikipediaClusterTable::DistanceTable WikipediaClusterTable::_distanceTable(numBuckets < 5 ? 5 : numBuckets);
WikipediaClusterTable::TypeTable WikipediaClusterTable::_typeTable(numBuckets < 5 ? 5 : numBuckets);
WikipediaClusterTable::TypeTable WikipediaClusterTable::_centerTable(numBuckets < 5 ? 5 : numBuckets);
bool WikipediaClusterTable::_is_initialized = false;
bool WikipediaClusterTable::_lc_active = false;
bool WikipediaClusterTable::_has_perplexities = false;
bool WikipediaClusterTable::_has_distance = false;
double WikipediaClusterTable::_maxPerplexity = 100;

void WikipediaClusterTable::initTable(const char *bits_file, const char *lc_bits_file) {
	if (!_is_initialized) {
		bool warningprinted = false;
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(bits_file));
		UTF8InputStream& in(*in_scoped_ptr);
		UTF8Token token;

		
		int linecount =0;
		while (!in.eof()) {
			in >> token;
			Symbol word = token.symValue();
			in >> token;
			Symbol path = token.symValue();
		

			int path_len = int (wcslen(path.to_string())) ;
			linecount++;

			//only print this warning the first time since these errors tend to propogate
			if (path_len <1){
				if(!warningprinted){
					if(SessionLogger::logger !=0){
						wchar_t message[500];
							swprintf(message,
									L"WikipediaClusterClass::init(), error in cluster file at line %d - no path.",
									linecount);
							*SessionLogger::logger << message << "\n";
					}
					else{
						SessionLogger::warn("SERIF") <<
							"**WikipediaClusterClass::init(), error in cluster file at line "
							<< linecount << " path " << path.to_debug_string() << " is not a string "
							<< "(word: " << word.to_debug_string() << ")" <<  std::endl;
					}
				}
				warningprinted = true;
			}
			else{
				warningprinted = false;
			}

			//wchar_t bitstring[200];
			//wcsncpy(bitstring, path.to_string(), 200);
			//std::wcout<<bitstring<<std::endl;
			Symbol bitstring  = Symbol(path.to_string());
			//std::wcout<<L"adding "<<word<<" "<<bitstring<<" to "<<_table[word].size()<<std::endl;
			_table[word].push_back(bitstring);
			path = Symbol(L"");
		}
		in.close();
		
		if (lc_bits_file != 0)  {
			boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(lc_bits_file));
			UTF8InputStream& in(*in_scoped_ptr);
			UTF8Token token;
			

			while (!in.eof()) {

				in >> token;
				Symbol word = token.symValue();
				in >> token;
				Symbol path = token.symValue();
				
				int path_len = int (wcslen(path.to_string())) ;
				linecount++;

				Symbol bitstring  = Symbol(path.to_string());
				_lowercaseTable[word].push_back(bitstring);
			}
			in.close();
			_lc_active = true;
		} else _lc_active = false;

		_is_initialized = true;
	}
	else {
		throw InternalInconsistencyException("WikipediaClusterClass::init()",
			"WikipediaClusterClass is already initialized.");
	}
	char perplexity_file[500];
	
	if (ParamReader::getParam("wikipedia_perplexity_file",perplexity_file,500))
	{
		initPerplexityTable(perplexity_file);
		_maxPerplexity = ParamReader::getRequiredFloatParam("maximum_perplexity");
		_has_perplexities = true;
	}

}

void WikipediaClusterTable::ensureInitializedFromParamFile() {
	if (_is_initialized)
		return;

	char bits_file[500];
	if (!ParamReader::getParam("wikipedia_cluster_file",bits_file, 500))	{
		return;
		
		/*throw UnexpectedInputException(
			"WikipediaClusterTable::ensureInitializedFromParamFile()",
			"Parameter 'wikipedia_cluster_file' not specified");
		*/
	}

	char lc_bits_file[500];
	if (ParamReader::getParam("lc_wikipedia_cluster_file",lc_bits_file, 500))	{
		initTable(bits_file, lc_bits_file);
	} else initTable(bits_file, 0);

	char distance_file[500];
	if (ParamReader::getParam("wikipedia_distance_file",distance_file, 500))	{
		initDistanceTables(distance_file);
		_has_distance = true;
	} 
 }


void WikipediaClusterTable::initPerplexityTable(const char *perplexity_file){
	bool warningprinted = false;
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(perplexity_file));
	UTF8InputStream& in(*in_scoped_ptr);
	UTF8Token token;
	float perplexity;
	int linecount =0;
	while (!in.eof()) {
		in >> token;
		Symbol path = token.symValue();
		in >> perplexity;
		//Symbol perplexity = token.symValue();
		
		
		//Symbol bitstring  = Symbol(path.to_string());
		//std::wcout<<L"adding "<<word<<" "<<bitstring<<" to "<<_table[word].size()<<std::endl;
		_perplexTable[path] = perplexity;
		path = Symbol(L"");
	}
	in.close();
}

void WikipediaClusterTable::initDistanceTables(const char *distance_file){
	bool warningprinted = false;
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(distance_file));
	UTF8InputStream& in(*in_scoped_ptr);
	UTF8OutputStream out("g:\\astevens\\Wikipedia\\Feature4-centerWord\\sample.out");
	UTF8Token token;
	int distance;
	int count = 0;
	int linecount =0;
	while (!in.eof()) {
		in >> token;
		Symbol path = token.symValue();
		in >> distance;
		//Symbol perplexity = token.symValue();
		in >> token;
		Symbol type = token.symValue();
		in>>token;
		Symbol center = token.symValue(); 	
		count++;
		
		out<<L"count = "<<count<<L"\tadding "<<path.to_debug_string()<<L" "<<distance<<L"\n";
		_distanceTable[path] = distance;
		_typeTable[path] = type;
		_centerTable[path] = center;
		path = Symbol(L"");
		type = Symbol(L"");
		center = Symbol(L"");
	}
	in.close();
}
