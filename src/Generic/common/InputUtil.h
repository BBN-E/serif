// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef INPUT_UTIL_H
#define INPUT_UTIL_H

// InputUtil is for miscellaneous input utilities

#include <string>
#include <set>
#include <vector>
#include "Generic/common/Symbol.h"


class InputUtil {
public:

	static std::set<std::wstring> readFileIntoSet(std::string filename, bool allow_multiword_entries, bool convert_to_lowercase);
	static std::set<Symbol> readFileIntoSymbolSet(std::string filename, bool allow_multiword_entries, bool convert_to_lowercase);
	static std::vector<std::wstring> readFileIntoVector(std::string filename, bool allow_multiword_entries, bool convert_to_lowercase);
	static std::set< std::vector<std::wstring> > readColumnFileIntoSet(std::string filename, bool convert_to_lowercase, std::wstring column_delimiter);
	static std::vector< std::vector<std::wstring> > readColumnFileIntoVector(std::string filename, bool convert_to_lowercase, std::wstring column_delimiter);
	static std::vector< std::vector<std::wstring> > readTabDelimitedFile(std::string filename);
	static std::string getBasePath(std::string file_path);
	/** Converts a Relative Path to an Absolute one if the buffer contains a relative
      * path.  Otherwise the buffer is left alone.  An exception is thrown if the
      * relative path attempts to go up more levels than contained in the base path */
	static void rel2AbsPath(std::string& buffer, const char *absoluteBasePath);
	static void normalizePathSeparators(std::string& foo);

private:
};

#endif
