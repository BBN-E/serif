#ifndef _DOCUMENT_TABLE_H_
#define _DOCUMENT_TABLE_H_

#include <string>
#include <utility>
#include <vector>
#include <set>

namespace DocumentTable {
	typedef std::pair<std::string,std::string> DocumentPath;
	typedef std::vector<DocumentPath> DocumentTable;

	// This version of readDocumentTable will read all paths from the file.
	DocumentTable readDocumentTable(const std::string & filename);
	// This version of readDocumentTable will read only those paths from the file
	// that are specified in set_of_files_to_match (provided that it is nonempty)
	// and that are NOT specified in set_of_files_to_block.
	DocumentTable readDocumentTable(const std::string & filename, 
		const std::set<std::string> & set_of_files_to_match, 
		const std::set<std::string> & set_of_files_to_block = std::set<std::string>());
};

#endif

