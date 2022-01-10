// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/InputUtil.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SessionLogger.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

#include <iostream>
#include <fstream>
#if defined(_WIN32)
#include <direct.h>
#include <io.h> // for _access()
#endif
#include <stdlib.h>
#include <stdio.h>
#include <sstream>

namespace { // Private namespace

// Common templated implementation that can read in a set/vector of strings or a set/vector of Symbols.
template<typename ContainerType>
inline ContainerType readFileInImpl(std::string filename, bool allow_multiword_entries, bool convert_to_lowercase) 
{
	// If passed an empty string, this function will return an empty set/vector
	// However, if passed a non-empty string, it will fail if the file cannot be opened

	// Use a set if you need to guarantee uniqueness, or a vector to guarantee order as read

	// This function always:
	// * Skips empty lines
	// * Skips anything after a # on a line
	// * Trims whitespace off the front and back of entries

	// Options:
	// * allow_multiword_entries: set to false if you do not expect to see spaces in your entries
	// * convert_to_lowercase: set to true to convert all entries to lowercase

	ContainerType result;
	if (filename.empty()) return result;
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(filename.c_str()));
	UTF8InputStream& stream(*stream_scoped_ptr);
	if (stream.fail()) {
		std::ostringstream err;
		err << "Problem opening " << filename;
		throw UnexpectedInputException("InputUtil::readFileInImpl", err.str().c_str());
	}
	while (!(stream.eof() || stream.fail())) {
		std::wstring line;
		std::getline(stream, line);
		line = line.substr(0, line.find_first_of(L'#')); // Remove comments.
		boost::trim(line);
		if (!line.empty()) {
			if (!allow_multiword_entries && line.find_first_of(' ') != std::wstring::npos) { 
				std::ostringstream err;
				std::string line_str(line.begin(), line.end()); // poor man's conversion for error output
				err << "Multi-word entry (not allowed) in " << filename << ": \"" << line_str << "\"";
				throw UnexpectedInputException("InputUtil::readFileInImpl", err.str().c_str());
			}
			if (convert_to_lowercase)
				boost::to_lower(line);
			result.insert(result.end(), line);
		}
	}
	return result;
}

} // End of private namespace

std::set<std::wstring> InputUtil::readFileIntoSet(std::string filename, bool allow_multiword_entries, bool convert_to_lowercase) {
	return readFileInImpl< std::set<std::wstring> >(filename, allow_multiword_entries, convert_to_lowercase);
}

std::set<Symbol> InputUtil::readFileIntoSymbolSet(std::string filename, bool allow_multiword_entries, bool convert_to_lowercase) {
	return readFileInImpl< std::set<Symbol> >(filename, allow_multiword_entries, convert_to_lowercase);
}

std::vector<std::wstring> InputUtil::readFileIntoVector(std::string filename, bool allow_multiword_entries, bool convert_to_lowercase) {
	return readFileInImpl< std::vector<std::wstring> >(filename, allow_multiword_entries, convert_to_lowercase);
}

std::set< std::vector<std::wstring> > InputUtil::readColumnFileIntoSet(std::string filename, bool convert_to_lowercase, std::wstring column_delimiter) {
	// Read the lines as before; allow whitespace since the column delimiter might be
	std::set<std::wstring> lines = readFileIntoSet(filename, true, convert_to_lowercase);

	// Split each row
	std::set< std::vector<std::wstring> > rows;
	BOOST_FOREACH(std::wstring line, lines) {
		std::vector<std::wstring> columns;
		boost::split(columns, line, boost::is_any_of(column_delimiter));
		rows.insert(columns);
	}

	return rows;
}

std::vector< std::vector<std::wstring> > InputUtil::readColumnFileIntoVector(std::string filename, bool convert_to_lowercase, std::wstring column_delimiter) {
	// Read the lines in order; allow whitespace since the column delimiter might be
	std::vector<std::wstring> lines = readFileIntoVector(filename, true, convert_to_lowercase);

	// Split each row
	std::vector< std::vector<std::wstring> > rows;
	BOOST_FOREACH(std::wstring line, lines) {
		std::vector<std::wstring> columns;
		boost::split(columns, line, boost::is_any_of(column_delimiter));
		rows.push_back(columns);
	}

	return rows;
}

std::vector< std::vector<std::wstring> > InputUtil::readTabDelimitedFile(std::string filename) {
	return readColumnFileIntoVector(filename, false, L"\t");
}

std::string InputUtil::getBasePath(std::string file_path){
	//get a string we can modify from the file path 
	std::string strFile = file_path;
	std::string base_path;
	// convert the file parameters according to the WIN or UNIX file conventions
	boost::algorithm::replace_all(strFile, "/", SERIF_PATH_SEP);
	boost::algorithm::replace_all(strFile, "\\", SERIF_PATH_SEP);

	
	if (strFile.find_first_of(":") == 1) //They gave us an absolute path (in Window named device char form) to the param file
	{
	   size_t pathEndPos = strFile.find_last_of(SERIF_PATH_SEP,strFile.size());
       base_path         = strFile.substr(0,pathEndPos);
	}
	else if ((strFile.find_first_of(SERIF_PATH_SEP) == std::string::npos))//They gave us a file with no path so use current dir
	{
		char curPath[1000];
		char buffer[1000];

        _getcwd(curPath,1000);
		sprintf(buffer,"%s"SERIF_PATH_SEP"%s",curPath,strFile.c_str());
		strFile           = buffer;
		size_t pathEndPos = strFile.find_last_of(SERIF_PATH_SEP,strFile.size());
        base_path          = strFile.substr(0,pathEndPos);
	}
	else  //got a relative path (or one that names host explictily)
	{
		char curPath[1000];
		_getcwd(curPath,1000);
        rel2AbsPath(strFile,curPath);
		size_t pathEndPos = strFile.find_last_of(SERIF_PATH_SEP,strFile.size());
		base_path          = strFile.substr(0,pathEndPos);
	}
	return base_path;
}

/* Modifies foo */
void InputUtil::normalizePathSeparators(std::string& foo) {
	boost::algorithm::replace_all(foo, "/", SERIF_PATH_SEP);
	boost::algorithm::replace_all(foo, "\\", SERIF_PATH_SEP);
}

/* Modifies buffer */
void InputUtil::rel2AbsPath(std::string& buffer, const char *absoluteBasePath)
{
	//current assumptions
	//basePath does not end with \\ 
	//basePath is absolute

	if (buffer.empty())
		return;

	//Make sure base path is absolute
	if (absoluteBasePath[0] == '.')
	{
		std::ostringstream ostr;
		ostr << "Absolute base path '" << absoluteBasePath << "' begins with '.'";
        throw UnexpectedInputException("ParamReader::rel2AbsPath()",
			ostr.str().c_str());
	}

	std::string basePath(absoluteBasePath);
	boost::algorithm::trim(basePath);

	// Normalize path separators (\\ and /) to the standard path separator,
	// which depends on the OS for which this was compiled.
	normalizePathSeparators(basePath);
	normalizePathSeparators(buffer);

	// Strip trailing path separator from the base path (if present).
	boost::algorithm::trim_right_if(basePath, boost::is_any_of(SERIF_PATH_SEP));
		
	//Detect Relative Paths and Convert Relative to Absolute
	if (boost::algorithm::starts_with(buffer, std::string(".")+SERIF_PATH_SEP))
	{
		buffer.replace(0,1,basePath);
	}
	else if (boost::algorithm::starts_with(buffer, std::string("..")+SERIF_PATH_SEP))
	{
		std::vector<std::string> relPathPieces;
		std::vector<std::string> basePathPieces;
		boost::split(relPathPieces, buffer, boost::is_any_of(SERIF_PATH_SEP));
		boost::split(basePathPieces, basePath, boost::is_any_of(SERIF_PATH_SEP));

		// Count the number of '..'s in the relative path.
		size_t relLevelCount = 0;
		while ((relLevelCount < relPathPieces.size()) && 
			   (relPathPieces[relLevelCount]==".."))
			++relLevelCount;

		// Make sure there are no '..'s after the first non-'..' piece -- e.g.,
		// we don't handle paths like ..\foo\..\bar
		for (size_t i=relLevelCount; i<relPathPieces.size(); ++i) {
			if (relPathPieces[i]=="..") {
				std::ostringstream ostr;
				ostr << "Only absolute paths that have all ..'s at the "
					 << "beginning are supported: \"" << buffer << "\"";
				throw UnexpectedInputException("ParamReader::rel2AbsPath()",
					ostr.str().c_str());
			}
		}

		// Make sure we didn't get too many '..'s.
		if (relLevelCount >= basePathPieces.size()) {
			std::ostringstream ostr;
			ostr << "relLevelCount (" << relLevelCount << ") >= baseLevelCount (" 
				 << basePathPieces.size() << ")\nUse full paths for parameter files to avoid."; 
			throw UnexpectedInputException("ParamReader::rel2AbsPath()",
				ostr.str().c_str());
		}

		// Construct the absolute path by joining the base path with the 
		// relative path (skipping directories corresponding to '..'s in 
		// the relative path.
		buffer.assign(basePathPieces[0]);
		for (size_t i=1; i<basePathPieces.size()-relLevelCount; ++i) {
			buffer.append(SERIF_PATH_SEP);
			buffer.append(basePathPieces[i]);
		}
		for (size_t i=relLevelCount; i<relPathPieces.size(); ++i) {
			buffer.append(SERIF_PATH_SEP);
			buffer.append(relPathPieces[i]);
		}
	}
}
