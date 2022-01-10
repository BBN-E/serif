// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/OutputUtil.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/Segment.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Region.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

#if defined(_WIN32)
#include <direct.h>
#include <windows.h>
#endif
#include "Generic/linuxPort/serif_port.h"

char OutputUtil::_converted_string[MAX_CONVERTED_STRING_LEN+1];

std::wstring OutputUtil::escapeXML(const std::wstring & str_in) {
	std::wstring str(str_in);
	// Special case: the input string may contain quotation marks that are 
	// already XML-escaped.  If so, then don't double-escape them.
	boost::algorithm::replace_all(str, "&quot;", "\"");
	boost::algorithm::replace_all(str, L"&", L"&amp;");
	boost::algorithm::replace_all(str, L"<", L"&lt;");
	boost::algorithm::replace_all(str, L">", L"&gt;");
	boost::algorithm::replace_all(str, L"'", L"&apos;");
	boost::algorithm::replace_all(str, L"\"", L"&quot;");

	return str;
}

std::wstring OutputUtil::untokenizeString(const std::wstring & str) {
	
	std::wstring temp = str;

	// parens and braces
	boost::replace_all(temp, L" ]",  L"]");
	boost::replace_all(temp, L" }",  L"}");
	boost::replace_all(temp, L" )",  L")");
	boost::replace_all(temp, L"[ ",  L"[");
	boost::replace_all(temp, L"{ ",  L"{");
	boost::replace_all(temp, L"( ",  L"(");
	boost::replace_all(temp, L"-LRB- ", L"(");
	boost::replace_all(temp, L"-LRB-",  L"(");
	boost::replace_all(temp, L" -RRB-", L")");
	boost::replace_all(temp, L"-RRB-",  L")");
	boost::replace_all(temp, L"-LCB- ", L"{");
	boost::replace_all(temp, L"-LCB-",  L"{");
	boost::replace_all(temp, L" -RCB-", L"}");
	boost::replace_all(temp, L"-RCB-",  L"}");
	boost::replace_all(temp, L"-LSB- ", L"[");
	boost::replace_all(temp, L"-LSB-",  L"[");
	boost::replace_all(temp, L" -RSB-", L"]");
	boost::replace_all(temp, L"-RSB-",  L"]");
	
	// get rid of leading spaces
	boost::replace_all(temp, L" .", L".");
	boost::replace_all(temp, L" ?", L"?");
	boost::replace_all(temp, L" !", L"!");
	boost::replace_all(temp, L" ,", L",");
	boost::replace_all(temp, L" ...", L"...");
	boost::replace_all(temp, L" -", L"-");
	boost::replace_all(temp, L" :", L":");
	boost::replace_all(temp, L" ;", L";");
	boost::replace_all(temp, L" n't", L"n't");
	boost::replace_all(temp, L" n&apos;t", L"n&apos;t");

	// random wacky things
	boost::replace_all(temp, L"&LR;", L"");
	boost::replace_all(temp, L"&lr;", L"");
	boost::replace_all(temp, L"&UR;", L"");
	boost::replace_all(temp, L"&ur;", L"");
	boost::replace_all(temp, L"&MD;", L"--");
	boost::replace_all(temp, L"&md;", L"--");
	boost::replace_all(temp, L"  ", L" ");

	// get rid of trailing spaces	
	boost::replace_all(temp, L"- ", L"-");
	boost::replace_all(temp, L"$ ", L"$");

	// quotation marks
	boost::replace_all(temp, L"`` ", L"``");
	boost::replace_all(temp, L"` ", L"`");
	boost::replace_all(temp, L" ''", L"''");

	// apostrophes
	boost::replace_all(temp, L" '", L"'");
	boost::replace_all(temp, L" &apos;", L"&apos;");

	return temp;
}

int OutputUtil::convertSerifSentenceToPassageId(const DocTheory *docTheory, int serif_sent_no) {
	
	Document *document = docTheory->getDocument();
	std::vector<WSegment>& segments = document->getSegments();
	TokenSequence *ts = docTheory->getSentenceTheory(serif_sent_no)->getTokenSequence();
	if (ts->getNTokens() == 0)
		return -1;
	EDTOffset start_offset = ts->getToken(0)->getStartEDTOffset();	

	if (segments.size() == 0) {
		// We don't have segments. Just return the sentence ID instead.
		return serif_sent_no;
	} else {
		// For segment files, regions and segments are the same. So, we can find the region_id and use it
		//  to get the segment. This is horribly brittle and should be fixed in a deeper way later.
		size_t docNumRegions = document->getNRegions();
		if (segments.size() != docNumRegions){
			throw UnrecoverableException("OutputUtil::convertSerifSentenceToPassageId", "got number of regions different from number of segments\n");
		}
		int found_region_id = -1;
		for (size_t region_id = 0; region_id < docNumRegions; region_id++) {
			const Region *region = document->getRegions()[region_id];
			// Start and end offsets should be inclusive, I believe
			if (start_offset >= region->getStartEDTOffset() &&
				start_offset <= region->getEndEDTOffset())
			{
				found_region_id = static_cast<int>(region_id);
				break;
			}
		}
		if (found_region_id == -1) {
			return -1;
		} else {
			WSegment correctSegment = segments.at(found_region_id);
			if (correctSegment.segment_attributes().find(L"passage-id") != correctSegment.segment_attributes().end())
				return _wtoi(correctSegment.segment_attributes()[L"passage-id"].c_str());
			else {
				return found_region_id;
			}
		}
	}
}

char *OutputUtil::convertToChar(const wchar_t *s) {
	if (s == 0)
		return 0;

	int i;
	for (i = 0; i < MAX_CONVERTED_STRING_LEN - 1 && *s; i++)
		_converted_string[i] = (char) *s++;
	_converted_string[i] = '\0'; // ensure null terminator

	return _converted_string;
}
 
// note that result must be delete[]ed!
char *OutputUtil::getNewIndentedLinebreakString(int indent) {
	char *result = _new char[indent+2];
	int i = 0;
	result[i++] = '\n';
	for (int j = 0; j < indent; j++)
		result[i++] = ' ';
	result[i] = '\0';

	return result;
}

const std::string OutputUtil::convertToUTF8BitString(const wchar_t* str) {
	std::stringstream stringStrm;
	unsigned char c[4];
	unsigned char baseMask = 0x80;
	unsigned char lowMask = 0x3f;
	
	size_t len = wcslen(str);
	for (size_t i = 0; i < len; i++) {
		wchar_t wc = str[i];
		// 0xxxxxxx - use the lower byte only
		if (wc <= 0x007f) {
			c[0] = (unsigned char)wc;
			stringStrm << c[0];
		}
		// 110xxxxx 10xxxxxx
		else if (wc <= 0x07ff) {
			c[1] = (baseMask | (lowMask&((char) wc)));
			c[0] = (0xc0 | ((char)(wc >> 6)));
			stringStrm << c[0] << c[1];
		}
		// 1110xxxx 10xxxxxx 10xxxxxx
		else if (wc <= 0xffff) {
			c[2] = (baseMask | (lowMask&((char) wc)));
			c[1] = (baseMask | (lowMask&((char) (wc >> 6))));
			c[0] = (0xe0 | ((char)(wc >> 12)));
			stringStrm << c[0] << c[1] << c[2];
		}
	}
	return stringStrm.str();
}

void OutputUtil::makeDir(const char* dirname) {
	_mkdir(dirname);
}

void OutputUtil::makeDir(const wchar_t* dirname) {
	std::string dirname_as_string(convertToUTF8BitString(dirname));
	_mkdir(dirname_as_string.c_str());
}

void OutputUtil::rmDir(const char* dirname) {
	_rmdir(dirname);
}

void OutputUtil::rmDir(const wchar_t* dirname) {
	std::string dirname_as_string(convertToUTF8BitString(dirname));
	_rmdir(dirname_as_string.c_str());
}

bool OutputUtil::isAbsolutePath(const char* dirname) {
	if (dirname == 0) return false;
	int len = static_cast<int>(strlen(dirname));

	#ifdef _WIN32
		// Path names that start with "//" or "\\" point to a shared drive.
		if (len>=2 && dirname[0]=='/' && dirname[1]=='/') return true;
		if (len>=2 && dirname[0]=='\\' && dirname[1]=='\\') return true;
		// Path names such as "C:\foo" are absolute.
		if (len>=3 && dirname[1]==':' && dirname[2]=='\\') return true;
		if (len>=3 && dirname[1]==':' && dirname[2]=='/') return true;
	#else
		// Path names that start with '/' are absolute.
		if (len>=1 && dirname[0]=='/') return true;
	#endif

	// All other paths are not absolute.
	return false;
}

bool OutputUtil::isAbsolutePath(const wchar_t* dirname) {
	return isAbsolutePath(convertToUTF8BitString(dirname).c_str());
}

std::string OutputUtil::getTempDir() {
#ifdef _WIN32
	// If there's a parameter file parameter that says which temporary directory
	// to use, then use that directory.
	std::string paramTempDir = ParamReader::getParam("windows_temp_dir");
	if (!paramTempDir.empty())
		return paramTempDir;
	// Otherwise, use the system default.
	TCHAR tempPath[MAX_PATH];
    DWORD tempPathLen = GetTempPath(MAX_PATH, tempPath);
    if (tempPathLen > MAX_PATH || (tempPathLen == 0))
		throw UnexpectedInputException("makeNamedTempFile", "Unable to find temporary directory");
	return tempPath;
#else
	// If there's a parameter file parameter that says which temporary directory
	// to use, then use that directory.
	std::string paramTempDir = ParamReader::getParam("linux_temp_dir");
	if (!paramTempDir.empty())
		return paramTempDir;
	// Otherwise, use the system default.
	return "/tmp/";
#endif
}


OutputUtil::NamedTempFile OutputUtil::makeNamedTempFile(std::string tempDir) {
	if (tempDir.empty())
		tempDir = getTempDir();
#ifdef _WIN32
	// Generate a unique filename.
	TCHAR tchar_filename[MAX_PATH];
    UINT status = GetTempFileName(tempDir.c_str(), TEXT("SRF"), 0, tchar_filename);
	if (status == 0) {
		std::stringstream errMsg;
		errMsg << "Could not create temporary file in directory " << tempDir;
		throw UnexpectedInputException("OutputUtil::makeNamedTempFile", errMsg.str().c_str());
	}
	std::string filename(tchar_filename);
#else
	char *filenameTemplate = strdup((tempDir+"/serifdb-XXXXXX").c_str());
	int fd = mkstemp(filenameTemplate); // <- this fills in the XXXXs.
	std::string filename(filenameTemplate);
	free(filenameTemplate);
	// Now that we've created the file, there is no race condition with
	// another process creating it.  But there's no standard way to
	// convert a file descriptor to a stream, so we'll just close it and
	// then re-open it.
	close(fd);
#endif
	// Open the file for writing, and return
	return NamedTempFile(filename, boost::make_shared<std::ofstream>(filename.c_str(), std::fstream::binary));
}

