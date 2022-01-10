#ifndef _LIT_FILE_UTILITIES_H_
#define _LIT_FILE_UTILITIES_H_

#include <vector>
#include <set>
#include "Generic/common/bsp_declare.h"
#include "Generic/patterns/multilingual/AlignedDocSet.h"
#include "Generic/patterns/multilingual/LanguageVariant.h"
#include "Generic/patterns/multilingual/AlignmentTable.h"
#include "Generic/patterns/multilingual/AlignedDocSet.h"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

BSP_DECLARE(DistillationDoc)

typedef std::pair<std::string, std::string> DocIDName;

namespace FileUtilities {
	/** Read the list of documents that we need to process from the given file,
	  * and return it as a vector of strings.  Each line of this file consists
	  * of a score (currently ignored), followed by a filename.  The filename
	  * is assumed to end with the extension ".7z"; and this extension is stripped
	  * off of each filename. */
	std::vector<DocIDName  > readDocumentList(const std::string& filename);
	std::vector<AlignedDocSet_ptr> readDocumentListToAligned(const std::string& filename);

	/** Read the list of documents across multiple languages from the given file and return 
	  * it as a vector of aligned documents. The file format require pairs of lines
	  * First line: docid language1 variant1 documentfilename1 language2 variant2 documentfilename2 ...
	  * Second line: sourcelanguage1:sourcevariant1=targetlanguage1:targetvariant1 alignmentfile
	  *      separated by commas for each alignment file. */
	std::vector<AlignedDocSet_ptr> readAlignedDocumentList(const std::string& filename);

	/* Checks locally in the three possible folders for a LearnItData Cache
	 * If it finds one, it returns its location. Otherwise it returns "" 
	 */
	std::string getCacheLocation();


	/* Checks locally in the three possible folders for a LearnItData Cache.
	* If it finds a cache, then it replaces all the network paths with local paths.
	*/
	void checkCache(std::vector<DocIDName> &filename);

	std::set<boost::filesystem::path> getLearnItDatabasePaths(
		const std::string& path);
	void getLearnItDatabasePaths(const std::string& path,
						std::set<boost::filesystem::path>& db_paths);
	void getLearnItDatabasePathsFromList(
			const boost::filesystem::path & list_file, 
			std::set<boost::filesystem::path>& db_paths);
	void getLearnItDatabasePathsFromDir(const boost::filesystem::path & dir, 
							std::set<boost::filesystem::path>& db_paths);
};

#endif
