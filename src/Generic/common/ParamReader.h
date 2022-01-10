// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARAMREADER_H
#define PARAMREADER_H

/** Parameter File Syntax Summary
  *
  *   - Parameter files are processed line-at-a-time, from top to bottom.
  *
  *   - Lines that start with "#" are comments.  Comments and blank
  *     lines are ignored.
  *
  *   - If a line contains a name surrounded by percent signs (eg
  *     "%serif_data%"), then that substring is replaced by the value
  *     of the named parameter.  This is done after checking for
  *     comments, but before all other processing.  If the named
  *     parameter is undefined, then an error is raised.
  *
  *   - Lines that have the form "name: value" define parameters.  If
  *     the same name is assigned a value on multiple lines, then
  *     either a warning or an error is reported; and only the first
  *     value is used.
  *
  *   - Lines that have the form "name.prefix: value" are used to define
  *     a prefix string that will be combined with the named parameter's
  *     value.  The prefix line must come before the line that defines
  *     the parameter; otherwise, it is ignored.  Typically, this is used
  *     in conjunction with a line of the form "name.list: values"
  *
  *   - Lines that have the form "name.list: values" are used to define
  *     a parameter whose value depends on the value of the "parallel"
  *     parameter.
  *
  *   - Lines that have the form "OVERRIDE name: value" can be used to
  *     override the value of a parameter that already has a value.
  *
  *   - If the value of a parameter begins with "$DATA_DRIVE$", then
  *     the value is assumed to be a path, and "$DATA_DRIVE$" is
  *     replaced with the first windows drive (c:, d:, e:, or f:) that
  *     makes the entire path point to an existing file.
  *
  *   - If the value of a parameter begins with "./" or ".\" or "../"
  *     or "..\", then that value is assumed to be a relative path,
  *     and is converted to an absolute path (relative to the
  *     directory containing the parameter file).  Note: the
  *     implementation for this is a series of brittle hacks.  It will
  *     fail if any of these substrings (such as "../") occur anywhere
  *     other than at the beginning of the value string.  Use with
  *     care for now; eventually, this should be fixed, or at least
  *     made to fail rather than silently giving the wrong value.
  *
  *   - Lines that start with either "@" or "INCLUDE " are include
  *     statements.  The "@" or "INCLUDE " should be followed by a
  *     path to a file.  If the path begins with "./" or ".\" or "../"
  *     or "..\", then it is treated as a relative path, relative to
  *     the directory containing the parameter file with the include
  *     statement.
  */

#include <iterator>
#include <string>
#include <vector>
#include <map>

#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SessionLogger.h"

class Symbol;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

#define LOCAL_DATA_DRIVE_WILDCARD "$DATA_DRIVE$"

/** This class is never instantiated -- it's just a place to
  * put functions. */
class SERIF_EXPORTED ParamReader {

 public:

	 typedef std::map<std::string, std::string> HashTable;

	/** returns true if readParamFile has been called */
	static bool isInitialized()
	{
		return _params != 0;
	}

	/** First the driver calls this, according to what was specified
	  * on the command line: */
	static void readParamFile(const std::string &param_file);
	static void readParamFile(const char *param_file);
	static void readParamFile(const std::string &param_file, const std::map<std::string,std::string>& overrides);
	static void readParamFile(const char *param_file, const std::map<std::string,std::string>& overrides);

	/** A module that needs a parameter with a char* value calls this.
	  * If the param isn't found, it returns bool and Value == "" */
    static bool getParam(const char* paramName, char* paramValue, size_t paramValueSize);

	/** A module that needs to change an existing parameter with a char* value calls this.
	  * If the param isn't found, it will be added to the list"  Does not support existing
	  * .prefixes or .list, but will convert relative paths.*/
    static void setParam(const char* paramName, const char* paramValue);

	/** Often you'll want to get regular old (narrow) char*
	  * param. This one throws an exception if the param isn't found. */
	static void getRequiredParam(const char* paramName, char* paramValue, size_t paramValueSize);

	// overloads using std::string
	static std::string getParam(const std::string &paramName, const char* defaultValue=0);
	static std::wstring getWParam(const std::string &paramName, const char* defaultValue=0);
	static std::string getRequiredParam(const std::string &paramName);
	static std::wstring getRequiredWParam(const std::string &paramName);

	/** Takes any parameter name bordered with + and/or % signs and replaces it with the value
	  * of that parameter. Throws an exception if parameter cannot be found. */
	static std::string expand( const std::string &input );

	/** Takes any parameter name bordered with boundaryChar and replaces it with the value
	  * of that parameter. Throws an exception if parameter cannot be found. */
	static std::string expand( const std::string &input, const std::string &boundaryChar, const std::map<std::string,std::string>& overrides);

	/** A module that needs a boolean parameter calls this.
	  * If the parameter = 'true', it returns true. If 'false', false.
	  * If anything else happens, it throws an exception.
	  */
	static bool getRequiredTrueFalseParam(const char* paramName);

	/** A module that needs a boolean parameter calls this.
	  * If the parameter = 'true', it returns true. If 'false', false.
	  * If the parameter is not defined, it returns the inputed default value.
	  * If anything else happens, it throws an exception.
	  */
	static bool getOptionalTrueFalseParamWithDefaultVal(const char* paramName, bool defaultVal);

	/** A module that needs an integer parameter calls this.
	  * If the parameter exists, it returns atoi(param).
	  * If not, it throws an exception.
	  */
	static int getRequiredIntParam(const char* paramName);

	/** A module that needs an integer parameter calls this.
	  * If the parameter exists, it returns atoi(param).
	  * If not, it assigns it a default value.
	  */
	static int getOptionalIntParamWithDefaultValue(const char* paramName, int defaultVal);

	/** A module that needs a float parameter calls this.
	  * If the parameter exists, it returns atof(param).
	  * If not, it throws an exception.
	  */
	static double getRequiredFloatParam(const char* paramName);

	/** A module that needs a float parameter calls this.
	  * If the parameter exists, it returns atof(param).
	  * If not, it returns the defaultValue
	  */
	static double getOptionalFloatParamWithDefaultValue(const char* paramName, double defaultVal);

	/** optional argument, so returns that size of the array and fills
	* the supplied one up to the max size indicated.
	* array elements are character sequences separated by commas
	*/
	static size_t getSymbolArrayParam(const char *paramName, Symbol *sarray, int maxSize);

	/** Return the value of a parameter as a vector of symbols.  The parameter
	  * should contain a comma-separated list.  If the parameter is not defined,
	  * then return the empty vector. */
	static std::vector<Symbol> getSymbolVectorParam(const char* paramName);

	/** Return the value of a parameter as a vector of strings.  The parameter
	  * should contain a comma-separated list.  If the parameter is not defined,
	  * then return the empty vector. */
	static std::vector<std::string> getStringVectorParam(const char* paramName);

	/** Return the value of a parameter as a vector of ints.  The parameter
	  * should contain a comma-separated list.  If the parameter is not defined,
	  * then return the empty vector. */
	static std::vector<int> getIntVectorParam(const char* paramName);

	/** Return the value of a parameter as a vector of wstrings.  The parameter
	  * should contain a comma-separated list.  If the parameter is not defined,
	  * then return the empty vector. */
	static std::vector<std::wstring> getWStringVectorParam(const char* paramName);

	/** A module that needs a non-required boolean parameter calls this.
	  * If the parameter exists and is 1 or = 'true', it returns true. 
	  * Anything else, it returns false
	  */
	static bool isParamTrue(const char* paramName);

	/** Outputs parameters to session log */
	static void logParams();

	/** The only purpose of this is to deallocate everything so that
	  * MS's CRT's stupid leak detection doesn't think that our
	  * objects were leaked. */
	static void finalize();

	//Temporary(for migration), remove by 9/1/05
	/** A module that needs a parameter calls this.
	  * If the param isn't found, it returns Symbol(). */
	static Symbol getParam(Symbol key);

	/** A module that needs a parameter calls this.
	  * If the parameter = 'true', it returns true. If 'false', false.
	  * If anything else happens, it throws an exception.
	  */
	static bool getRequiredTrueFalseParam(Symbol key);

	/** A module that needs a parameter calls this.
	  * If the parameter exists, it returns atoi(param).
	  * If not, it throws an exception.
	  */
	static int getRequiredIntParam(Symbol key);

	/** Often you'll want to get regular old (narrow) char*
	  * param. This one returns false if the param isn't found. */
	static bool getNarrowParam(char *result, Symbol key, int max_len);

	/** Often you'll want to get regular old (narrow) char*
	  * param. This one throws an exception if the param isn't found. */
	static void getRequiredNarrowParam(char *result, Symbol key, int max_len);

	/** returns total number of parameters stored */
	static size_t getParameterCount();

	/** returns zero based Nth Paramater important for iterations */
	static bool getNthParam(size_t n, char* paramName, char* paramValue);
	
	static const std::map<std::string,std::string> getAllParams()           { return (_params()); }
	static void setAllParams( const std::map<std::string,std::string> & p ) { _params()=p; }

	static bool hasParam(const char* paramName);
	static bool hasParam(const std::string &paramName);

	static void unsetParam(const char* paramName);

 private:

	//Hash containing Parameters and their Values
	static HashTable &_params();

	//List of Include Files to Process
	static std::map<std::string, int> &_includes();
    
	//The path of the base configuration file.  This value will be used to convert
	//Relative Paths to Absolute Paths. All Parameter values with relative paths must
	//make the value relative to the base path.
	static std::string _basePath;
	static size_t _paramCount;

	// If true, print out all parameters that get used, prefixed by "ParamReader::outputUsedParamName"
	static bool _output_used_param_names;

	static void checkForIllegalDashes(std::string str);
	static int checkInclude(const char* incName);
	static bool hasDataDriveString(const std::string& str);
	static void includeFile(const char* line, const std::map<std::string,std::string>& overrides);
	static void outputUsedParamName(const char* param_name);
	static void replaceDataDriveString(std::string& str);	
	

public:
	// Result is returned in the 'param_name' and 'value' parameters.
	static bool parseParamLine(const char* buffer, std::string& param_name, std::string& value);

private:
	static void readParameter(const char* param, const char* value, HashTable &prefix_list, bool override_value);
	static void readPrefixParameter(const char* param, const char* value, HashTable &prefix_list, bool override_value);
	static void readListParameter(const char* param, const char* value, HashTable &prefix_list, bool override_value);
	static UnexpectedInputException reportOverrideError(const char* paramName, const char* newParamValue);
	static UnexpectedInputException reportMissingParamError(const char* paramName);
	static UnexpectedInputException reportBadValueError(const char* paramName, const char* expectedValueType);	
};

#endif
