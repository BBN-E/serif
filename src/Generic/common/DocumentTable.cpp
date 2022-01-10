#include "Generic/common/leak_detection.h"

#include "DocumentTable.h"
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "Generic/common/UnexpectedInputException.h"

//Pass-through to the next overloaded form of readDocumentTable with an empty set of files to explicitly match
//(i.e., use all files whose names are specified in the file).
DocumentTable::DocumentTable DocumentTable::readDocumentTable(
		const std::string & filename) 
{
	std::set< std::string> empty_set_of_files_to_match;
	return readDocumentTable(filename, empty_set_of_files_to_match);
}

// If set_of_files_to_match is empty (the usual case), then it will be ignored. Otherwise,
// only the lines in filename that contain filenames found in set_of_files_to_match will
// be included in the DocumentTable that is constructed.
DocumentTable::DocumentTable DocumentTable::readDocumentTable(
		const std::string & filename, 
		const std::set< std::string> & set_of_files_to_match,
		const std::set<std::string> & set_of_files_to_block /*= std::set<std::string>()*/) 
{
	if (!boost::filesystem::exists(filename)) {
		std::stringstream err;
		err << "Document table file: " << filename << " does not exist.";
		throw UnexpectedInputException("DocumentTable::readDocumentTable",
				err.str().c_str());
	}

	std::ifstream docs_stream(filename.c_str());
	std::string line;
	DocumentTable document_table;
	while (getline(docs_stream, line)) {
		boost::algorithm::trim(line);
		// Assume that initial # indicates a comment. If you are intent on using a 
		// file whose name really starts with #, prepend "./" to its name in the list.
		if (line.empty() || line[0] == '#') {
			continue;
		}
		std::string::size_type space_index = line.find("\t");
		std::string docid = line.substr(0, space_index);
		// If the files_to_match set is not empty, then it contains all the files we care about. Skip the others.
		if (!set_of_files_to_match.empty() && set_of_files_to_match.find(docid) == set_of_files_to_match.end()) {
			continue;
		}
		if (!set_of_files_to_block.empty() && set_of_files_to_block.find(docid) != set_of_files_to_block.end()) {
			continue;
		}
		std::string doc_file = line.substr(space_index + 1);
		DocumentPath path(docid, doc_file);
		document_table.push_back(path);
	}
	return document_table;
}


