// Copyright (c) 2009 by BBNT Solutions LLC
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include <vector>
#include <set>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/filesystem.hpp>
#include "Generic/common/hash_map.h"
#include "Generic/common/Segment.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/BoostUtil.h"
#include "Generic/common/XMLUtil.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

#include <xercesc/dom/DOM.hpp>

using namespace std;

XERCES_CPP_NAMESPACE_USE;

///
/// Global variable to control the mode of operation
///
/// In GNG mode, we output a file in this format:
/// COVERED COVERING #trigrams-in-covered #trigrams-in-covering coverage-score
///
/// In all other modes, we output a file that lists all uncovered document IDs.
///
/// In PROTOTYPE mode, we also check to see whether a document is covered via WMS page-id, both with
/// itself and compared to documents in previous epochs.
///

enum { GNG, PROTOTYPE, ICEWS, AWAKE };
int MODE;
int DATE_WINDOW_IN_DAYS;

///
/// TRIGRAM HASH INFORMATION
///
/// It seemed like these were giving us memory problems. I imagine that
/// if we were to run this on too many documents at a time, that problem
/// might re-appear. (Liz Boschee, 11/30/2010)
///

struct trigram {
	std::string t[3];
};

// Parameters for hash table
// In general, bucket_size * min_buckets should be in the middle of the range of expected hash table entries
// In TrigramHasher, the comparison function is relatively slow, so bucket_size should be low
// If the program is running out of memory, make MIN_BUCKETS smaller (should always be a factor of 2)
const int BUCKET_SIZE = 2;
const int MIN_BUCKETS = 2048;

inline size_t str_hash (const char* s) {
	unsigned long h = 0; 
	for ( ; *s; ++s)
		h = 5*h + *s;

	return size_t(h);
}

struct TrigramHashKey {
	size_t operator()(const trigram &t) const {
		return (str_hash( t.t[0].c_str() ) << 9) +
			(str_hash( t.t[1].c_str() ) << 5) +
			str_hash( t.t[2].c_str() );
	}
};

struct TrigramEqualKey {
	bool operator()(const trigram &t1, const trigram &t2) const {
		return (t1.t[0] == t2.t[0]) && (t1.t[1] == t2.t[1]) && (t1.t[2] == t2.t[2]);
	}
};

typedef serif::hash_map<trigram, int, TrigramHashKey, TrigramEqualKey> trigram_hashmap;

///
/// END TRIGRAM HASH INFORMATION
///

std::string removePrefixesFromDocid(std::string docID)
{
	std::string coreDocID = docID;
	for (int i = 0; i < 4; i++) {
		size_t pos = coreDocID.find("-");
		if (pos != std::string::npos)
			coreDocID = coreDocID.substr(pos + 1);
	}
	return coreDocID;
}

class DupDocInfo {
public:	
	DupDocInfo(boost::filesystem::path docPath, bool is_current) {	
		path = docPath;
		docID = BOOST_FILESYSTEM_PATH_GET_FILENAME(docPath);

		// Get date and source from filename, where possible
		std::string sdate = "";
		if (MODE == ICEWS) {
			// Expected format = YYYYMMDD.docid.xml
			if (docID.size() > 8)
				sdate = docID.substr(0,8);
			source = "XXX";
		} else if (MODE == AWAKE) {
			// Expected format = AWAKE_YYYYMMDD.docid.docsplit.xml
			if (docID.size() > 14)
				sdate = docID.substr(6,14);
			source = "AWAKE";
		} else if (MODE == GNG || MODE == PROTOTYPE) {
			std::string smallDocID = removePrefixesFromDocid(docID);
			source = smallDocID.substr( 0, 3 );
			sdate = smallDocID.substr( 3, 8 );
		} else {
			std::cerr << "Mode not supported in DD creation\n";
			exit(1);
		}

		// This might throw an error; that's OK, we'll catch it up top
		docDate = boost::gregorian::date(boost::gregorian::from_undelimited_string(sdate));
		trigrams = 0;
		trigram_count = 0;
		loaded = false;
		is_current_epoch = is_current;
	}
	~DupDocInfo() { delete trigrams; }
		
	boost::filesystem::path path;
	std::string docID;
	std::string source;
	boost::gregorian::date docDate;
	trigram_hashmap *trigrams;
	int trigram_count;
	bool loaded;
	bool is_current_epoch;
};
typedef boost::shared_ptr<DupDocInfo> DupDocInfo_ptr;

bool docInfoSortPredicate(const DupDocInfo_ptr& a, const DupDocInfo_ptr& b) {
	// first sort by date, then by ID (which really means source, then raw ID)
	if (a->docDate < b->docDate) { 
		return true;
	} else if (a->docDate > b->docDate) {
		return false;
	} else {
		return a->docID < b->docID;
	}
}

// Convert a document to a string so that we can extract its tokens
// Assumes a set of "fields" to be processed. These are assumed to be non-overlapping;
//   the contents of each will be added. For instance, for ICEWS, the only field is "sentence".
//   For another task, the fields might be "HEADLINE" and "TEXT".
std::string xml_to_string( boost::filesystem::path path, std::vector<std::string>& field_values ){
	DOMDocument* doc = XMLUtil::loadXercesDOMFromFilename(path.string().c_str());
	std::stringstream text_str;
	BOOST_FOREACH(std::string field_value, field_values) {
		SerifXML::xstring x_field_value = SerifXML::transcodeToXString(field_value.c_str());
		DOMNodeList *fields = doc->getElementsByTagName(x_field_value.c_str());		
		for (size_t i = 0; i < fields->getLength(); i++) {
			DOMElement *field = (DOMElement *)fields->item(i);
			std::string scontent = SerifXML::transcodeToStdString(field->getTextContent());
			text_str << scontent;			
		}
		text_str << "\n";
	}
	return text_str.str();
}

// Convert a document  to a string so that we can extract its tokens
std::string segments_to_string( boost::filesystem::path path, std::wstring input_field1, std::wstring input_field2 ){
	std::wifstream fin( path.string().c_str() );

	std::stringstream doc_stream; 
	while( fin ){
		WSegment segment;
		try {
			fin >> segment; 
		} catch (std::runtime_error &e) {
			SessionLogger::info("DocSim") << " Skipping segment in " << path.string() << endl;
			SessionLogger::info("DocSim") << e.what();
			continue;
		}

		if( !fin ) break;

		// Look for our input field
		WSegment::const_iterator field_it = segment.find(input_field1);

		// Back off to our second input field if we can't find the first
		if (field_it == segment.end()) {
			field_it = segment.find(input_field2);
		}

		std::wstring wsentence = field_it->second.at(0).value;
		std::string sentence(wsentence.begin(), wsentence.end());

		doc_stream << sentence;
	}

	return doc_stream.str(); 
}


// Load an actual document
void load_doc( DupDocInfo_ptr ddInfo ){

	// This should have already been checked, but it never hurts to be careful
	if (!boost::filesystem::exists(ddInfo->path)) {
		SessionLogger::warn("DocSim") << "Couldn't load " << ddInfo->path.string() << "\n";
		ddInfo->loaded = false;
		ddInfo->trigram_count = 0;
		return;
	}

	std::stringstream dstrm;
	
	if (MODE == ICEWS) {
		std::vector<std::string> field_values;
		field_values.push_back("sentence");
		dstrm << xml_to_string(ddInfo->path, field_values);
	} else if (MODE == PROTOTYPE) {
		std::string narrow_field_name = ParamReader::getRequiredParam("segment_input_field");
		std::wstring field_name(narrow_field_name.begin(), narrow_field_name.end());
		dstrm << segments_to_string(ddInfo->path, field_name, L"stt");
	} else if (MODE == GNG) {		
		std::cerr << "Not sure how to read in a GNG file anymore. Is it segments? XML? Tell me how to do it before running this program.";
		exit(1);
	} else if (MODE == AWAKE) {
		std::vector<std::string> field_values;
		field_values.push_back("passage");
		dstrm << xml_to_string(ddInfo->path, field_values);
	} else {
		std::cerr << "Mode not supported in load_doc\n";
		exit(1);
	}

	ddInfo->trigrams = new trigram_hashmap();

	trigram t;
	ddInfo->trigram_count = 0;

	t.t[0] = "";
	t.t[1] = "";
	t.t[2] = "";

	while( dstrm ){
		std::string tok;
		dstrm >> tok;
		if( !tok.length() )  continue;

		t.t[2] = t.t[1];
		t.t[1] = t.t[0];
		t.t[0] = tok;

		trigram_hashmap::iterator trigramHashIter = ddInfo->trigrams->find(t);

		if (trigramHashIter != ddInfo->trigrams->end()) {
			(*trigramHashIter).second++;
		}
		else {
			ddInfo->trigrams->insert(pair<trigram, int>(t, 1));
		}

		ddInfo->trigram_count++;
	}

	ddInfo->loaded = true;
	return;
}


void unload_doc( DupDocInfo_ptr ddInfo ){
	delete ddInfo->trigrams;
	ddInfo->trigrams = 0;
	ddInfo->trigram_count = 0;
	ddInfo->loaded = false;
	return;
}

std::pair<std::string, int> parse_pageid_from_docid(const std::string &s1) {

	// This code is only relevant for the WMS prototype
	if (MODE != PROTOTYPE)
		return std::make_pair("", 0);

	// Expected format: BLAHBLAHBLAH.pageid.pageversion.BLAHBLAHBLAH
	
	size_t first_index = s1.find(".");

	std::string sub_s1 = s1.substr(first_index+1, s1.length()-1);
	size_t second_index = sub_s1.find(".");
	std::string pageid = sub_s1.substr(0, second_index);
	
	std::string sub_s2 = sub_s1.substr(second_index+1, sub_s1.length()-1);
	size_t third_index = sub_s2.find(".");	
	std::string version = sub_s2.substr(0, third_index);
	int version_int = atoi(version.c_str());

	return std::make_pair(pageid, version_int);
}

bool has_same_page_id(const std::string &docid1, const std::string &docid2) {
	
	// This code is only relevant for the WMS prototype
	if (MODE != PROTOTYPE)
		return false;

	std::pair<std::string, int> pair1 = parse_pageid_from_docid(docid1);
	std::pair<std::string, int> pair2 = parse_pageid_from_docid(docid2);
	
	return (pair1.first.compare(pair2.first) == 0);
}

// if coverage is greater than threshold, return score; otherwise 0.0. Allow shortcut to return 1 for any match, for PROTOTYPE mode
double getCoverageScore( DupDocInfo_ptr doc, DupDocInfo_ptr covered_by, double coverage_threshold, bool return_zero_or_one) {

	int count = 0;
	int missed = 0;

	int coverable = (int)floor(doc->trigram_count * coverage_threshold);
	int missable = doc->trigram_count - coverable;

	for (trigram_hashmap::iterator it = doc->trigrams->begin(); it != doc->trigrams->end(); ++it) {
		trigram_hashmap::iterator it2 = covered_by->trigrams->find( (*it).first );
		if( it2 != covered_by->trigrams->end() )
			count += min( (*it).second, (*it2).second );
		else missed += (*it).second;

		// speed-up
		if (return_zero_or_one && count >= coverable) 
			return 1.0;

		// speed-up
		if (missed > missable)
			return 0.0;
	}
	
	double ret_value = float(count) / doc->trigram_count;

	if (ret_value >= coverage_threshold) {
		if (return_zero_or_one)
			return 1.0;
		else return ret_value;
	}

	return 0.0f; // didn't match threshold
}

void gdb_print_trigrams( DupDocInfo_ptr di ){

	for( trigram_hashmap::iterator it = di->trigrams->begin(); it != di->trigrams->end(); ++it ){
		SessionLogger::info("DocSim") << (*it).first.t[0] << " " <<  (*it).first.t[1] << " " <<  (*it).first.t[2] << " " <<  (*it).second << endl;    
	}
	SessionLogger::info("DocSim") << di->trigram_count << " total" << endl;

	return;
}

bool print_duplicates() {
	std::string param = ParamReader::getRequiredParam("print_dup_or_nondup_to_file");
	if (param == "dup")
		return true;
	else if (param == "nondup")
		return false;
	else {
		std::cerr << "Parameter 'print_dup_or_nondup_to_file' must be 'dup' or 'nondup'\n";
		exit(1);
	}
}

void compute_similarity( vector<DupDocInfo_ptr> & previousDocs, vector<DupDocInfo_ptr> & docs, ofstream & out_file ){
	int front = 0;
	int back = 0;
	int prev_front = 0;
	int prev_back = 0;

	double coverage_threshold = ParamReader::getRequiredFloatParam("duplicate_document_coverage_threshold");

	// for use in prototype mode
	set<DupDocInfo_ptr> coveredDocs;

	// We only compute similarity within a X-day window, so sort by date (done by calling functoin)
	// and only keep documents loaded within that window. 
	for( int cur = 0; cur < (int)docs.size(); cur++ ) {
		DupDocInfo_ptr cur_doc = docs[cur];
		if (cur % 100 == 0)
			SessionLogger::info("DocSim") << "Computing similarity for document " << cur << " of " << docs.size() << "\n";

		boost::gregorian::date minDate = cur_doc->docDate - boost::gregorian::date_duration(DATE_WINDOW_IN_DAYS);
		boost::gregorian::date maxDate = cur_doc->docDate + boost::gregorian::date_duration(DATE_WINDOW_IN_DAYS);

		//
		// Load current documents as necessary & process them with respect to the current document
		//

		// allow cross-source comparisons if mode==prototype
		while (front < (int)docs.size() && docs[front]->docDate <= maxDate) {
			load_doc( docs[front] );
			front++;
		}

		while( back < cur ){
			if (docs[back]->loaded) {
				unload_doc( docs[back] );
			}
			back++;
		}

		if (cur % 100 == 0) {
			int n_loaded = front - back- 1;
			SessionLogger::info("DocSim") << "DEBUG Document id = " << cur_doc->docID << "; " << n_loaded << " current documents compared\n";
		}

		// back = cur
		for( int i = back+1; i < front; i++ ){
			DupDocInfo_ptr d1 = docs[i];
			DupDocInfo_ptr d2 = docs[cur];
			
			//SessionLogger::dbg("DocSim") <<"Comparing doc " << d1->docID << " with " << d2->docID << "\n";
			
			// these should always be loaded, since we are within the back-front window, but let's be cautious
			if (!d1->loaded || !d2->loaded)
				continue;

			// skip empty docs
			if (d1->trigram_count == 0 || d2->trigram_count == 0)
				continue;

			DupDocInfo_ptr shorter = d1;
			DupDocInfo_ptr longer = d2;

			// swap if need be
			if( d1->trigram_count > d2->trigram_count || (d1->trigram_count == d2->trigram_count && d1->path > d2->path) ){
				shorter = d2;
				longer = d1;
			}
			
			// we already know this is covered, that's all we care about, let's move on				
			if (MODE != GNG && coveredDocs.find(shorter) != coveredDocs.end())
				continue;

			// the covered doc must be *at least* 1/2 as long as the covering doc
			if( (float(shorter->trigram_count) / float(longer->trigram_count) < 0.5) )
				continue;

			double score = getCoverageScore(shorter, longer, coverage_threshold, MODE==PROTOTYPE);
			if (score >= coverage_threshold) {
				if (MODE == GNG) {
					out_file << removePrefixesFromDocid(shorter->docID) << " " << removePrefixesFromDocid(longer->docID) << " " << shorter->trigram_count << " " << longer->trigram_count << " " << score << "\n";
				} else {
					SessionLogger::info("DocSim") << shorter->docID << " is covered by a current doc " << longer->docID << " (" << score << ")\n";
					coveredDocs.insert(shorter);
				}
			}
		}
		
		// Skip to the next one if we've already discarded this as a duplicate and we're in any mode but GNG
		if (MODE != GNG && coveredDocs.find(cur_doc) != coveredDocs.end())
			continue;

		if (MODE == PROTOTYPE) {

			// Load documents with the right date range of cur_doc
			while (prev_front < (int)previousDocs.size() && previousDocs[prev_front]->docDate < minDate) {
				prev_front++;
			}
			while (prev_front < (int)previousDocs.size() && previousDocs[prev_front]->docDate <= maxDate) {
				load_doc( previousDocs[prev_front] );
				prev_front++;
			}
			while (prev_back < (int)previousDocs.size() && previousDocs[prev_back]->docDate < minDate) {
				if (previousDocs[prev_back]->loaded) {
					unload_doc( previousDocs[prev_back] );
				}
				prev_back++;
			}

			if (cur % 100 == 0) {
				int n_loaded = prev_front - prev_back;
				SessionLogger::info("DocSim") << "DEBUG Document id = " << cur_doc->docID << "; " << n_loaded << " previous documents compared\n";
			}

			for( int i = prev_back; i < prev_front; i++ ){
				DupDocInfo_ptr prev_doc = previousDocs[i];

				//SessionLogger::info("DocSim") <<"Comparing doc " << cur_doc->docID << " with " << prev_doc->docID << "\n";

				// these should always be loaded, since we are within the back-front window, but let's be cautious
				if (!prev_doc->loaded || !cur_doc->loaded)
					continue;

				// this will only be happening in PROTOTYPE mode
				if (getCoverageScore(cur_doc, prev_doc, coverage_threshold, true) >= coverage_threshold) {
					coveredDocs.insert(cur_doc);
					SessionLogger::info("DocSim") << cur_doc->docID << " is covered by a previous doc " << prev_doc->docID << "\n";
					break;
				}
			}
		}
	}

	if (MODE != GNG) {
		int covered_count = 0;
		int uncovered_count = 0;

		bool print_dups = print_duplicates();

		// print UNCOVERED docs
		BOOST_FOREACH(DupDocInfo_ptr dd, docs) {			
			if (coveredDocs.find(dd) != coveredDocs.end()) {
				if (print_dups)
					out_file << BOOST_FILESYSTEM_PATH_AS_STRING(dd->path) << "\n";
				covered_count++;
			} else {
				if (!print_dups)
					out_file << BOOST_FILESYSTEM_PATH_AS_STRING(dd->path) << "\n";
				uncovered_count++;
			}
		}	
		SessionLogger::info("DocSim") << covered_count << " duplicate documents; " << uncovered_count << " non-duplicate documents, after trigram pruning\n";
	} 
}

void usage() {
	SessionLogger::info("DocSim") << "Usage: DocSim.exe param_file\n";
	exit(-1);
}

int main( int argc, char ** argv ){

	if (argc != 2) {
		usage();
	}

	try {
		// First argument must be param file
		ParamReader::readParamFile(argv[1]);

		std::string mode = ParamReader::getRequiredParam("duplicate_document_mode"); 

		if (mode.compare("GNG") == 0) {
			MODE = GNG;
		} else if (mode.compare("PROTOTYPE") == 0) {
			MODE = PROTOTYPE;
		} else if (mode.compare("ICEWS") == 0) {
			MODE = ICEWS;
		} else if (mode.compare("AWAKE") == 0) {
			MODE = AWAKE;
		} else {
			SessionLogger::info("DocSim") << "Mode must be AWAKE, ICEWS, GNG, or PROTOTYPE.\n";
			SessionLogger::info("DocSim") << "  (Note that GNG is deprecated until someone needs to use it again and updates it.)\n";
			usage();
		}

		DATE_WINDOW_IN_DAYS = ParamReader::getRequiredIntParam("date_window_in_days");

		// Variables	
		std::string current_epoch_str = "";
		std::string file_extension = "";
		std::string source_dir = "";
		boost::filesystem::path input_file;
		boost::filesystem::path output_file;
		bool compare_current_to_current;
		boost::gregorian::date earliest_date_to_consider;
		boost::gregorian::date latest_date_to_consider;

		// Mode-specific parameters
		if (MODE == GNG || MODE == ICEWS || MODE == AWAKE) {

			// These will all deal with absolute path names
			input_file = boost::filesystem::path(ParamReader::getRequiredParam("docids_file"));
			output_file = boost::filesystem::path(ParamReader::getRequiredParam("output_file"));
			compare_current_to_current = false;

			std::string earliest_date = ParamReader::getParam("earliest_date_to_consider");
			if (earliest_date != "")
				earliest_date_to_consider = boost::gregorian::date(boost::gregorian::from_undelimited_string(earliest_date));

			std::string latest_date = ParamReader::getParam("latest_date_to_consider");
			if (latest_date != "")
				latest_date_to_consider = boost::gregorian::date(boost::gregorian::from_undelimited_string(latest_date));			

			// Run just to make sure param is in place before we go to the trouble of doing all the work
			print_duplicates();

		} else if (MODE == PROTOTYPE) {
			source_dir = ParamReader::getRequiredParam("processing_base_dir");
			current_epoch_str = ParamReader::getRequiredParam("current_epoch");

			input_file = boost::filesystem::path(source_dir);
			input_file /= current_epoch_str;
			input_file /= ParamReader::getRequiredParam("cumulative_input_filename");

			output_file = boost::filesystem::path(source_dir);
			output_file /= current_epoch_str;
			output_file /= ParamReader::getRequiredParam("output_filename");
			
			compare_current_to_current = ParamReader::getRequiredTrueFalseParam("compare_current_to_current");

			std::cout << "DocSim input_file: " << input_file.string() << ", output_file: " << output_file.string() << ", current_epoch: " << current_epoch_str << "\n" <<flush;
		} 
		
		std::vector< DupDocInfo_ptr > current_docs_final; // only list used for non-PROTOTYPE modes
		
		// Extra stuff for PROTOTYPE mode
		std::vector< DupDocInfo_ptr > current_docs_initial; // raw input
		std::vector< DupDocInfo_ptr > previous_docs;
		std::set< std::string > existing_page_IDs;		
		
		SessionLogger::info("DocSim") << "Reading all documents from " << input_file.string() << "...\n";
		ifstream input_stream(input_file.string().c_str());
		int count = 0;		
		while( !input_stream.eof() ){

			boost::filesystem::path docPath;
			bool is_current = true;

			if (MODE == PROTOTYPE) {

				// Expected input: epoch_id filename
				docPath = boost::filesystem::path(source_dir);
				std::string epoch;
				input_stream >> epoch;			
				docPath /= epoch;
				docPath /= "Segments";
				std::string filename;
				input_stream >> filename;
				docPath /= filename;
				is_current = (epoch == current_epoch_str);

			} else if (MODE == AWAKE || MODE == ICEWS || MODE == GNG) {
				// Expected input: absolute_path_to_filename				
				std::string line = "";
				input_stream >> line;

				// skip empty lines
				if (line.empty()) 
					continue;

				docPath = boost::filesystem::path(line);
			} 

			DupDocInfo_ptr ddInfo;
			try {
				ddInfo = boost::make_shared<DupDocInfo>(docPath, is_current);
			} catch( ... ) {
				SessionLogger::err("DocSim") << "Error loading metadata from document " << docPath.string() << "\n";
				exit(1);
			}

			// This allows us to consider only a portion of our filelist; useful for parallelization
			if (!earliest_date_to_consider.is_not_a_date() && ddInfo->docDate < earliest_date_to_consider)
				continue;
			if (!latest_date_to_consider.is_not_a_date() && ddInfo->docDate > latest_date_to_consider)
				continue;

			if (MODE == GNG || MODE == ICEWS || MODE == AWAKE) {
				current_docs_final.push_back(ddInfo);
			} else if (MODE == PROTOTYPE) {
				// If the file path exists, this is a current document, and we keep its ddInfo object
				// Otherwise, we just keep its page id			
				if (ddInfo->is_current_epoch) {
					current_docs_initial.push_back(ddInfo);
				} else {
					previous_docs.push_back(ddInfo);
					std::pair<std::string,int> pageid = parse_pageid_from_docid(ddInfo->docID);
					existing_page_IDs.insert(pageid.first);
				}
			}
			count++;
		}
		input_stream.close();
		SessionLogger::info("DocSim") << "done reading " << count << " docids\n";
		
		if (MODE == PROTOTYPE) {

			SessionLogger::info("DocSim") << previous_docs.size() << " previous documents; " << current_docs_initial.size() << " new documents\n";
			std::vector< DupDocInfo_ptr > current_docs_intermediate; // after first pruning step		
		
			// Only keep documents that do not have a repeat page-id
			std::map<std::string, int> lowestPageVersion;
			BOOST_FOREACH(DupDocInfo_ptr ddInfo, current_docs_initial) {
				std::pair<std::string,int> pageid = parse_pageid_from_docid(ddInfo->docID);
				// keep documents that don't share a pageid with a previous epoch
				if (existing_page_IDs.find(pageid.first) == existing_page_IDs.end())
					current_docs_intermediate.push_back(ddInfo);
				else continue;

				// keep track of the lowest page version for each new page id
				if (lowestPageVersion.find(pageid.first) == lowestPageVersion.end()) {
					lowestPageVersion[pageid.first] = pageid.second;
				} else {
					if (lowestPageVersion[pageid.first] > pageid.second)
						lowestPageVersion[pageid.first] = pageid.second;
				}
			}			

			SessionLogger::info("DocSim") << current_docs_intermediate.size() << " new documents after previous-epoch page-id pruning\n";
			current_docs_initial.clear(); // as a sanity check to make coding errors more obvious

			if (compare_current_to_current) {
				SessionLogger::info("DocSim") << "comparing CURRENT docs with CURRENT docs now...";
				// Now look for documents in the same epoch with the same page id
				BOOST_FOREACH(DupDocInfo_ptr ddInfo, current_docs_intermediate) {
					std::pair<std::string,int> pageid = parse_pageid_from_docid(ddInfo->docID);
					if (lowestPageVersion.find(pageid.first) == lowestPageVersion.end()) {
						current_docs_final.push_back(ddInfo);
					} else {
						// only keep it if it is the lowest page version for this pageid
						if (lowestPageVersion[pageid.first] == pageid.second)
							current_docs_final.push_back(ddInfo);
					}
				}
				SessionLogger::info("DocSim") << current_docs_final.size() << " new documents after current-epoch page-id pruning\n";
				current_docs_intermediate.clear(); // as a sanity check to make coding errors more obvious
			} else {
				// We don't need to compare current with current because we already did it in a previous job
				// and we were passed an input file that contains already pruned current vs. current docs.
				// Thus here we are only comparing the final current docs with previous epoch docs.
				current_docs_final.insert(current_docs_final.end(), current_docs_intermediate.begin(),current_docs_intermediate.end());
				SessionLogger::info("DocSim") << current_docs_final.size() << " new documents read from current epoch no-duplicates file.\n";
			}

			current_docs_final.insert(current_docs_final.end(), current_docs_initial.begin(),current_docs_initial.end());
		}

		// All modes back together for this step, using current_docs_final
		ofstream out_stream(output_file.string().c_str());
		if (current_docs_final.size() > 0) {
			std::sort(current_docs_final.begin(), current_docs_final.end(), docInfoSortPredicate);
			if (previous_docs.size() > 0)
				std::sort(previous_docs.begin(), previous_docs.end(), docInfoSortPredicate);
			compute_similarity(previous_docs, current_docs_final, out_stream);
		}
		out_stream.close();

	}
	catch (UnrecoverableException & e) {
		SessionLogger::info("DocSim") << "DocSim caught UnrecoverableException '" << e.getMessage() << "' from '" << e.getSource() << "', hard failing." << endl;
		exit(-1);
	}
	catch( std::exception & e ){
		SessionLogger::info("DocSim") << "DocSim caught possibly recoverable '" << e.what() << ", hard failing." << endl;
		exit(-1);
	}
	catch( ... ){
		SessionLogger::info("DocSim") << "DocSim caught unknown exception, hard failing." << endl;
		exit(-1);
	}


}
