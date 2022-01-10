// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef OUTPUT_UTIL_H
#define OUTPUT_UTIL_H

// OutputUtil is for miscellaneous output utilities

#include <wchar.h>
#include <string>
#include <boost/shared_ptr.hpp>
class DocTheory;

const int INDENT_INCREMENT = 2;
const int MAX_CONVERTED_STRING_LEN = 1023;

class OutputUtil {
public:
	/** This produces the string that should be printed between
	  * lines of pretty-printed output to achieve the desired 
	  * indent level. Yup, that's all it does.
	  * Note that result must be delete[]ed! */
	static char *getNewIndentedLinebreakString(int indent);

	static char *convertToChar(const wchar_t *s);

	/** Encode each wchar_t in the given array as UTF8 and pack the 
	  * resulting bytes into the string that get returned. */
	static const std::string convertToUTF8BitString(const wchar_t* str);

	static void makeDir(const char* dirname);
	static void makeDir(const wchar_t* dirname);

	static void rmDir(const char* dirname);
	static void rmDir(const wchar_t* dirname);

	static bool isAbsolutePath(const char* dirname);
	static bool isAbsolutePath(const wchar_t* dirname);

	/** Return type for makeNamedTempFile(). */
	typedef std::pair<std::string, boost::shared_ptr<std::ofstream> > NamedTempFile;
	
	/** Create a new temporary file, and return a pair containing the file's
	  * name and a binary output file stream that can be used to write to the
	  * file.  If no tempDir is specified, then getTemporaryDirectory() 
	  * will be used. */
	static NamedTempFile makeNamedTempFile(std::string tempDir=std::string());

	/** Return a path that can be used for temporary files.  If the parameters
	  * windows_temp_dir or linux_temp_dir are set, then the directory specified
	  * by those parameters is used; otherwise, an OS-specific default temporary
	  * directory is returned. */
	static std::string getTempDir();
	
	/** Escape any characters in the given string that have special meaning in
	  * XML, and return the resulting escaped string. */
	static std::wstring escapeXML(const std::wstring & str);

	/** Do some crude untokenization of a wstring, for output purposes*/
	static std::wstring untokenizeString(const std::wstring & str);

	/** Given a Serif sentence ID, retrieve the segment-based "passage ID" for output */
	static int convertSerifSentenceToPassageId(const DocTheory *docTheory, int serif_sent_no);

private:
	static char _converted_string[];
};

#endif
