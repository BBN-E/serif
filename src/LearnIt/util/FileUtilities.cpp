#include "Generic/common/leak_detection.h"
#include "FileUtilities.h"

#include <vector>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/DocTheory.h"
#include "LearnIt/MainUtilities.h"

// Studio is over-paranoid about bounds checking for the boost string 
// classification module, so we wrap its import statement with pragmas
// that tell studio to not display warnings about it.  For more info:
// <http://msdn.microsoft.com/en-us/library/ttcz0bys.aspx>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif
#include "boost/algorithm/string/classification.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

using std::string;
using std::make_pair;
using std::wstringstream;

std::string FileUtilities::getCacheLocation() {
	std::string drive="";

	struct stat st;
	std::vector<std::string> drives;
	drives.push_back("D");
	drives.push_back("C");
	drives.push_back("F");
	//Add extra drive possibilities here.  Everything else will
	//automatically take them into account.
	
	for( size_t i = 0; i < drives.size(); ++i )	{
		if (stat(std::string(drives[i] + ":\\text-group-local\\LearnItData").c_str(), &st) == 0) {
			drive = drives[i] + ":\\text-group-local\\LearnItData\\";
			break;
		}
	}
	
	return drive;
}

void FileUtilities::checkCache(std::vector<DocIDName> &filenames) {
	std::string drive=getCacheLocation();

	if (drive.size() > 0) {
		for( size_t i = 0; i < filenames.size(); ++i ) {
			string& filename = filenames[i].second;
			if( filename.find("\\gigaword\\") != std::string::npos ||
				filename.find("//gigaword//") != std::string::npos) {
				filename.replace(0, filename.rfind("gigaword")+9, drive);
			}
		}
	}
}

std::vector<DocIDName > FileUtilities::readDocumentList(const std::string& filename) {
	std::ifstream docs_stream(filename.c_str());

	if (!docs_stream.good()) {
		std::wstringstream err;
		err << "Error trying to open document list " 
			<< std::wstring(filename.begin(), filename.end());
		throw UnexpectedInputException("FileUtilities::readDocumentList", err);
	}

	std::string line;
	std::vector<DocIDName > document_list;
	while (getline(docs_stream, line)) {
		std::string::size_type space_index = line.find(" ");
		std::string docid;
		std::string w_doc_file;
		if (space_index == std::string::npos) {
			docid = "0";
			w_doc_file = line;
			space_index = 0;
		} else {
			std::string::size_type extension_index = line.find(".7z");
			if (extension_index == std::string::npos) {
				extension_index = line.find(".xml");
			}
			docid = line.substr(0, space_index);
			w_doc_file = line.substr(space_index+1, line.length()-space_index);
		}
		
		document_list.push_back(make_pair<std::string, std::string>(docid, w_doc_file));
	}

	if (!ParamReader::getOptionalTrueFalseParamWithDefaultVal("ignore_cache", false)) {
		checkCache(document_list);
	}
	return document_list;
}

std::vector<AlignedDocSet_ptr > FileUtilities::readDocumentListToAligned(const std::string& filename) {
	std::ifstream docs_stream(filename.c_str());

	if (!docs_stream.good()) {
		std::wstringstream err;
		err << "Error trying to open document list " 
			<< std::wstring(filename.begin(), filename.end());
		throw UnexpectedInputException("FileUtilities::readDocumentList", err);
	}

	std::string line;
	std::vector<AlignedDocSet_ptr > document_list;
	while (getline(docs_stream, line)) {
		AlignedDocSet_ptr docset = boost::make_shared<AlignedDocSet>();

		std::string::size_type space_index = line.find(" ");
		std::string::size_type extension_index = line.find(".7z");
		if (extension_index == std::string::npos) {
			extension_index = line.find(".xml");
		}
		std::string docid = line.substr(0, space_index);
		std::string w_doc_file = line.substr(space_index+1, line.length()-space_index);

		docset->setDefaultLanguageVariant(LanguageVariant::getLanguageVariant());
		docset->loadDocument(LanguageVariant::getLanguageVariant(),w_doc_file);
		//SessionLogger::info("testing") << "HEY" << docset->getDefaultDocFilename() << std::endl;
		document_list.push_back(docset);
	}

	return document_list;
}

std::vector<AlignedDocSet_ptr> FileUtilities::readAlignedDocumentList(const std::string& filename) {
	std::ifstream docs_stream(filename.c_str());

	if (!docs_stream.good()) {
		std::wstringstream err;
		err << "Error trying to open document list " 
			<< std::wstring(filename.begin(), filename.end());
		throw UnexpectedInputException("FileUtilities::readDocumentList", err);
	}

	std::string doc_line;
	std::string alignment_line;
	std::vector<AlignedDocSet_ptr> document_list;
	while (getline(docs_stream, doc_line) && getline(docs_stream, alignment_line)) {
		std::string::size_type space_index = doc_line.find(" ");
		std::string docid = doc_line.substr(0, space_index);
		std::string w_doc_files = doc_line.substr(space_index+1, doc_line.length()-space_index);

		std::vector<std::string> doc_split;
		boost::split(doc_split, w_doc_files, boost::is_any_of(","));

		std::vector<std::string> alignment_split;
		split(alignment_split, alignment_line, boost::is_any_of(","));

		AlignedDocSet_ptr docset = boost::make_shared<AlignedDocSet>();

		for (size_t i=0;i<doc_split.size();i++) {
			std::vector<std::string> single_doc;
			boost::split(single_doc, doc_split[i], boost::is_any_of(" "));
			if (single_doc.size() != 3) { //maybe we want to throw something?
				break;
			}
			
			Symbol language = Symbol(std::wstring(single_doc[0].begin(),single_doc[0].end()));
			Symbol variant = Symbol(std::wstring(single_doc[1].begin(),single_doc[1].end()));
			LanguageVariant_ptr lv = boost::make_shared<LanguageVariant>(language,variant);
			
			if (i==0) 
				docset->setDefaultLanguageVariant(lv);

			docset->loadDocument(lv,single_doc[2]);
		}

		for (size_t i=0;i<alignment_split.size();i++) {
			std::vector<std::string> single_alignment;
			boost::split(single_alignment, alignment_split[i], boost::is_any_of(" "));
			if (single_alignment.size() != 2) { //maybe we want to throw something
				break; 
			}
			
			docset->loadAlignmentFile(single_alignment[1],single_alignment[0]);
		}

		document_list.push_back(docset);
	}

	return document_list;
}

std::set<boost::filesystem::path> FileUtilities::getLearnItDatabasePaths(const std::string& path) {
	std::set<boost::filesystem::path> ret;
	getLearnItDatabasePaths(path, ret);
	return ret;
}

void FileUtilities::getLearnItDatabasePaths(const std::string& path,
								std::set<boost::filesystem::path>& db_paths) 
{
	boost::filesystem::path full_path;
	bool is_simple_file(false);
	if (path.empty()) {
		std::ostringstream out;
		out << "LearnIt database input is empty";
		throw std::runtime_error(out.str().c_str());
	}
	if (boost::algorithm::ends_with(path, ".db")) {
		is_simple_file = true;		
	}
	full_path = boost::filesystem::system_complete(boost::filesystem::path(path));

	if (!boost::filesystem::exists(full_path)) {
		std::ostringstream out;
		out << "Specified LearnIt database input (" << full_path << ") doesn't exist";
		throw std::runtime_error(out.str().c_str());
	}

	// Read database file, or list of database files, or walk through a directory of database files
	if (is_simple_file) {
		if (!boost::filesystem::is_regular_file(full_path)) {
			std::ostringstream out;
			out << "LearnIt DB database input (" << full_path << ") is not a regular file";
			throw std::runtime_error(out.str().c_str());
		}
		db_paths.insert(full_path);
	} else if (boost::filesystem::is_directory(full_path)) {
		// Read patterns iteratively directory by directory
		getLearnItDatabasePathsFromDir(full_path, db_paths);
	} else {
		getLearnItDatabasePathsFromList(full_path, db_paths);
	}
}

void FileUtilities::getLearnItDatabasePathsFromList(
	const boost::filesystem::path & list_file, std::set<boost::filesystem::path>& db_paths) 
{
	std::string line;
	std::ifstream dbs_stream(list_file.string().c_str());

	// Try to load each specified database
	SessionLogger::info("li_db_list_0") << "Reading LearnIt DB list " << list_file.string() << "..." << std::endl;
	
	while (dbs_stream.is_open() && getline(dbs_stream, line)) {
		boost::algorithm::trim(line);
		if (line != "" && line[0] != '#') {
			// Clean up the path
			boost::filesystem::path full_path = boost::filesystem::system_complete(boost::filesystem::path(line));
			if (!boost::filesystem::exists(full_path)) {
				std::ostringstream out;
				out << "LearnIt database input (" << full_path << ") doesn't exist; ensure that list (" << list_file << ") contains one valid filename per line ";
				throw std::runtime_error(out.str().c_str());
			}

			db_paths.insert(full_path);
		}
	}
}

void FileUtilities::getLearnItDatabasePathsFromDir(const boost::filesystem::path & dir, 
							std::set<boost::filesystem::path>& db_paths) 
{
	// Read each item from this directory in turn
	boost::filesystem::directory_iterator end;
	SessionLogger::dbg("li_db_dir_0") << "Reading LearnIt database dir " << dir.string() << "..." << std::endl;
	for (boost::filesystem::directory_iterator dir_item(dir); dir_item != end; dir_item++) {
		if (boost::filesystem::is_directory(*dir_item)) {
			// Recurse
			getLearnItDatabasePathsFromDir(*dir_item, db_paths);
		} else {
			// Filter based on file suffix
			if (boost::algorithm::ends_with(BOOST_FILESYSTEM_DIR_ITERATOR_GET_PATH(dir_item), ".db")) {
				db_paths.insert(*dir_item);
			}
		}
	}
}
