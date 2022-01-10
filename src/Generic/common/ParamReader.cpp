// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
//
// See ParamReader.h for a summary of the syntax of parmeter files.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"

#include <iostream>
#include <fstream>
#if defined(_WIN32)
#include <direct.h>
#include <io.h> // for _access()
#endif
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <boost/algorithm/string.hpp> 
#include <boost/lexical_cast.hpp> 
#include <boost/foreach.hpp>

#include "Generic/linuxPort/serif_port.h"
#include "Generic/common/InputUtil.h"
#include "Generic/common/UnicodeUtil.h"

// Determines whether an unexpected parameter override is a fatal error
#define DIE_ON_UNEXPECTED_OVERRIDE false

// static member initialization:
std::string ParamReader::_basePath = "";
size_t ParamReader::_paramCount = 0;
bool ParamReader::_output_used_param_names = false;

//ParamReader::HashTable *ParamReader::_params = 0;
//map<string, int> *ParamReader::_includes = 0;

// Some private pointers.  These could be kept in ParamReader as static
// variables, but that would require us to recompile a whole bunch of
// stuff any time we change them. 
namespace {
	ParamReader::HashTable *_params_ptr = 0;
	static std::map<std::string, int> *_includes_ptr = 0;
}

/** Return the global parameter table.  The parameter table is 
  * constructed (as an empty table) the first time this is called. */
ParamReader::HashTable &ParamReader::_params() {
	if (!_params_ptr) {
		SessionLogger::err("param") 
			<< "ParamReader parameters accessed before the first call to ParamReader::readParamFile()";
		_params_ptr = _new HashTable();
	}
	return *_params_ptr;
}

/** Return the global include map.  This map is constructed (as
  * an empty map) the first time it is called. */
std::map<std::string, int> &ParamReader::_includes() {
	if (!_includes_ptr) {
		SessionLogger::err("param") 
			<< "ParamReader parameters accessed before the first call to ParamReader::readParamFile()";
		_includes_ptr = _new std::map<std::string, int>;
	}
	return *_includes_ptr;
}

namespace {
	template<typename T> std::string defaultValToString(T v) {
		return boost::lexical_cast<std::string>(v); }
	template<> std::string defaultValToString<bool>(bool v) {
		return v?"true":"false"; }

	class ParamTrackers {
		std::set<std::string> _used;
		ParamReader::HashTable _defaultValues;
		bool _enabled;
	public:
		ParamTrackers(): _enabled(false) {}
		void enable() { _enabled = true; }
		void markUsed(std::string p) { 
			_used.insert(p); 
		}
		template<typename T>
		void markDefault(std::string p, T v) { 
			if (_enabled)
				_defaultValues[p] = defaultValToString(v);
		}
		~ParamTrackers() {
			if (_enabled) {
				ParamReader::HashTable params = ParamReader::getAllParams();
				ParamReader::HashTable::iterator iter;
				for (iter = params.begin(); iter != params.end(); iter++) {
					const std::string &param = (*iter).first;
					if (_used.find(param) != _used.end())
						std::cout << "  Explicit param: " << param << std::endl;
				}
				BOOST_FOREACH(const std::string &param, _used) {
					if (params.find(param) == params.end()) {
						std::cout << "   Default param: " 
							<< std::setiosflags(std::ios::left) << std::setw(40)
							<< param << std::resetiosflags(std::ios::left);
						iter = _defaultValues.find(param);
						if (iter != _defaultValues.end())
							std::cout << " (" << (*iter).second << ")";
						std::cout << std::endl;
					}
				}
				for (iter = params.begin(); iter != params.end(); iter++) {
					const std::string &param = (*iter).first;
					if (_used.find(param) == _used.end())
						std::cout << "    Unused param: " << param << std::endl;
				}
			}
		}
	};
	ParamTrackers _paramTracker;
}

/** Delete the parameter table & includes map.  They will be created
  * again if they are ever accessed again. */
void ParamReader::finalize()
{
	delete _params_ptr; 
	_params_ptr = 0;
	delete _includes_ptr; 
	_includes_ptr = 0;
}

void ParamReader::readParamFile(const std::string &param_file) {
	std::map<std::string,std::string> overrides;
	ParamReader::readParamFile(param_file.c_str(), overrides);
}
void ParamReader::readParamFile(const char *param_file) {
	std::map<std::string,std::string> overrides;
	ParamReader::readParamFile(param_file, overrides);
}

void ParamReader::readParamFile(const std::string &param_file, const std::map<std::string,std::string>& overrides){
	ParamReader::readParamFile(param_file.c_str(), overrides);
}

void ParamReader::readParamFile(const char *param_file, const std::map<std::string,std::string>& overrides) {
	// Initialize globals, if they're not already initialized.
	if (!_params_ptr)
		_params_ptr = _new HashTable();
	if (!_includes_ptr)
		_includes_ptr = _new std::map<std::string, int>;
	std::string param_file_str = std::string(param_file);
	_basePath = InputUtil::getBasePath(param_file_str);

    //A list of Prefixes to Process
	HashTable prefix_list;

	//Open file for reading
	std::ifstream inputFile (param_file);

    if (! inputFile.is_open())
    {
		std::string s = "Can't open parameter file: " + param_file_str;
		throw UnexpectedInputException("ParamReader::readParamFile()", s.c_str());
		return;
	}

	int linecnt = 0;

	//Read each line of the file
    while (! inputFile.eof() )
    {
		std::string buffer;           // Temporary line buffer
       
		//Get Current Line
		std::getline(inputFile, buffer);
		linecnt++;

		//Strip leading and trailing white space
		boost::algorithm::trim(buffer);

		// Skip blank lines.
		if (buffer.empty())
			continue;

		// Skip comments.
		if (buffer[0] == '#')
			continue;

		//Expand variable parameters enclosed by % sign.
		//We choose % rather than + so that runjobs never tries to replace these variables.
		buffer = expand(buffer, "%", overrides);
		if (buffer.empty())
			continue;

		if (buffer[0] == '@' || boost::algorithm::starts_with(buffer, "INCLUDE ")) 
		{
			try {
				if (buffer[0] == '@')
					includeFile(buffer.c_str()+strlen("@"), overrides);
				else
					includeFile(buffer.c_str()+strlen("INCLUDE "), overrides);
			} catch (UnexpectedInputException &e) {
				std::ostringstream err;
				err << "In file imported from " << param_file << " (line " << linecnt << "):\n";
				e.prependToMessage(err.str().c_str());
				throw;
			}
		}
		else
		{
			try {
				// Check if there's an override delcaration.  If so, then this also strips it
				// from the input line.
				bool override_value = false;
				if (boost::algorithm::starts_with(buffer, "OVERRIDE ")) {
					override_value = true;
					buffer = buffer.substr(strlen("OVERRIDE "));
				}
				
				if (boost::algorithm::starts_with(buffer, "UNSET ")) {
					buffer = buffer.substr(strlen("UNSET "));
					boost::algorithm::trim(buffer);
					unsetParam(buffer.c_str());
					continue;
				}

				// Parse the parameter line (which has the form "param: value") into two strings.
				// If the line is not valid, parseParamLine will throw an UnexpectedInputException.
				std::string param;
				std::string value;
				if (!parseParamLine(buffer.c_str(), param, value)) {
					std::ostringstream ostr;
					ostr << "Line does not contain a valid 'param: value' pair:\n"
						<< buffer << std::endl;
					throw UnexpectedInputException("ParamReader::parseParamFile()", 
						ostr.str().c_str());
				}

				// If the value contains the special string "$DATA_DRIVE$", then it is replaced
				// with the data drive.  Otherwise, if the value begins with ".\" or "./" or
				// "..\" or "../", then it is converted to an absolute path (relative to the
				// directory containing the parameter file).
				if (hasDataDriveString(value)) 
					replaceDataDriveString(value);
				else
					InputUtil::rel2AbsPath(value,_basePath.c_str());

				// If it's a prefix parameter, then add it to the list of prefixes.
				if (boost::algorithm::ends_with(param, ".prefix"))
					readPrefixParameter(param.c_str(), value.c_str(), prefix_list, override_value);

				// If it's a list parameter, then select the appropriate value (based on 
				// the value of 'parallel'), and set the parameter to that value.
				else if (boost::algorithm::ends_with(param,".list"))
					readListParameter(param.c_str(), value.c_str(), prefix_list, override_value);

				// Otherwise, set the given parameter to the given value.
				else
					readParameter(param.c_str(), value.c_str(), prefix_list, override_value);			
			}
			catch (UnexpectedInputException &e) 
			{
				std::ostringstream err;
				err << "In " << param_file << " (line " << linecnt << "):\n";
				e.prependToMessage(err.str().c_str());
				throw;
			}
		}
	}

    //Close the Parameter File
	inputFile.close();

	// Check that all prefix parameters were consumed with a matching list parameter
	BOOST_FOREACH(HashTable::value_type prefix, prefix_list) {
		if (!hasParam(prefix.first)) {
			std::ostringstream err;
			err << "In " << param_file << ": prefix parameter " << prefix.first
				<< " has no matching list parameter";
			throw UnexpectedInputException("ParamReader::readParamFile()", err.str().c_str());
		}
	}

	// Disabled due to side-effects in non-ATEA code after param reading - ncw 2010.04.20
	//int rv = _chdir(_basePath.c_str());
	
	// Check if we are outputting used param names
	_output_used_param_names = isParamTrue("param_reader_output_used_param_names");
	if (isParamTrue("track_parameters")) {
		_paramTracker.enable();
	}

    // If the USE_CORRECT_ANSWERS preprocessor flag is true, then define a 
    // use_correct_answers parameter.  This can go away when we get rid
	// of the USE_CORRECT_ANSWERS flag.
    #ifdef USE_CORRECT_ANSWERS
	setParam("use_correct_answers", "true");
    #endif
}

void ParamReader::includeFile(const char* filename, const std::map<std::string,std::string>& overrides) {
	// Convert the given (possibly relative) filename to an absolute path.
	std::string filepath(filename);
	boost::algorithm::trim(filepath);
	std::string previous_base_path = _basePath;
	InputUtil::rel2AbsPath(filepath, _basePath.c_str());
	// Issue a warning if we've already included this file.
	if (checkInclude(filepath.c_str()) != 0)
		std::cerr << "Warning: file \"" << filename << "\" is imported twice." << std::endl;

	readParamFile(filepath.c_str(), overrides);
	// readParamFile may alter the _basePath, so we need to restore it
	_basePath = previous_base_path;
	_includes()[filepath] = 2;
}

void ParamReader::outputUsedParamName(const char* param_name) {
	_paramTracker.markUsed(param_name);
	if (_output_used_param_names)
		std::cout << "ParamReader::outputUsedParamName(" << param_name << ")\n";
}

// Result is returned in the 'param_name' and 'value' parameters.
// Return true on success, false on failure.
bool ParamReader::parseParamLine(const char* buffer, std::string& param, std::string& value) {
	// Find the first ":" in the line.
	const char* splitpos = strpbrk(buffer, ":");
	if (splitpos == NULL)
		return false; // Invalid parameter line.

	// Copy what's to the left of the ":" into param, and what's to its right into value.
	param.assign(buffer, splitpos-buffer);
	value.assign(splitpos+1);

	//trim whitespace from left and right of param and value
	boost::algorithm::trim(param);
	boost::algorithm::trim(value);

	// Fail if the parameter name contains a space.
	if (strpbrk(param.c_str(), " \n\r\t"))
		return false; // Invalid parameter line.

	// Return success if both param & value are non-empty.
	return (!(param.empty() || value.empty()));
}

void ParamReader::readPrefixParameter(const char* param, const char* value, 
									  HashTable &prefix_list, bool override_value)
{
	size_t param_len  = strlen(param);
	size_t prefix_len = strlen(".prefix");
	size_t prefix_pos = param_len - prefix_len;

	//make sure prefix is at the end
	if (strncmp(param + prefix_pos,".prefix",prefix_len) != 0)
	{
		std::ostringstream ostr;
		ostr << "Param name '" << param << "' does not end with '.prefix'.";
		throw UnexpectedInputException("ParamReader::readPrefixParameter()", 
			ostr.str().c_str());
	}

	//make sure the param name isnt just ".prefix"
	if (param_len <= prefix_len) 
	{
		throw UnexpectedInputException("ParamReader::readPrefixParameter()", 
			"Param name must have one or more characters before '.prefix'.");
	}

	std::string strName(param, 0, prefix_pos);
	std::string strVal(value); 
	if (!override_value && (prefix_list.find(strName) != prefix_list.end() || hasParam(strName))) {
		reportOverrideError(param, value);
	} else {
		prefix_list[strName] = strVal;
	}
}


UnexpectedInputException ParamReader::reportMissingParamError(const char* paramName) {
	std::ostringstream err;
		err << "Required param \"" << paramName << "\" not defined";
		throw UnexpectedInputException("ParamReader::readParamFile()", err.str().c_str());
}

UnexpectedInputException ParamReader::reportBadValueError(const char* paramName, const char* expectedValueType) {
	std::ostringstream err;
		err << "Param \"" << paramName << "\"'s value must be " << expectedValueType;
		throw UnexpectedInputException("ParamReader::readParamFile()", err.str().c_str());
}

UnexpectedInputException ParamReader::reportOverrideError(const char* paramName, const char* newParamValue)
{
	std::ostringstream err;
	std::string existingValue = getParam(paramName);
	err << "Attempt to override parameter \"" << paramName 
		<< "\" without using an explicit OVERRIDE declaration "
		<< "(original: " << getParam(paramName) << "; ignored: " << newParamValue << ")";
	if (DIE_ON_UNEXPECTED_OVERRIDE) {
		throw UnexpectedInputException("ParamReader::readParamFile()", err.str().c_str());
	} else {
		// Only log if there's a real problem
		if (existingValue.compare(newParamValue) != 0)
			SessionLogger::warn_user("parameter_override") << err.str();
		return UnexpectedInputException("ParamReader::readParamFile()", err.str().c_str());
	}
}

void ParamReader::readListParameter(const char* param, const char* value, HashTable &prefix_list, bool override_value)
{
	size_t param_len  = strlen(param);
	size_t list_len = strlen(".list");
	size_t list_pos = param_len - list_len;

	//make sure prefix is at the end
	if (strncmp(param + list_pos,".list",list_len) != 0)
	{
		std::ostringstream ostr;
		ostr << "Param name '" << param << "' does not end with '.list'.";
		throw UnexpectedInputException("ParamReader::readListParameter()", 
			ostr.str().c_str());
	}
	//make sure the param name isnt just ".list"
	if (param_len == list_len)
	{
		throw UnexpectedInputException("ParamReader::readListParameter()", 
			"Param name must contain one or more characters before '.list'.");
	}

	std::string strName(param, 0, list_pos);
	size_t nth;  //parallelization index
    
	//if parallel param exists and is a valid 3 digit 000-999 convert to Nth
    //Add the Param to the list if it doesnt already exist
	std::string parallelValue = getParam("parallel");

    if (!parallelValue.empty())
    {
        if (parallelValue.size() != 3)
		{
			std::ostringstream ostr;
			ostr << "Invalid value for parameter 'parallel': " << parallelValue 
				<< "; must be 3-digit number, 000-999.";
            throw UnexpectedInputException("ParamReader::readListParameter()", 
				ostr.str().c_str());
		}

		nth = atoi(parallelValue.c_str());
    }
	else  //else default to use first list item, no parallel param exists
	{
		nth = 0;
	}

	std::vector<std::string> listItems;
	boost::split(listItems, value, boost::is_any_of(" "), boost::algorithm::token_compress_on);

	//if nth > list len, error
    if (nth >= listItems.size())
	{
		std::ostringstream ostr;
		ostr << "Invalid parallelization value (" << nth << "); "
			 << "exceeds number of items in list (" << listItems.size() << ") for parameter: " << param;
		throw UnexpectedInputException("ParamReader::readListParameter()", 
			ostr.str().c_str());
	}
	
	std::string strVal = listItems[nth];

	//Check if prefix exists for Parameter Name
	std::map <std::string,std::string>::iterator myIterator = prefix_list.find(strName);
	
	if (myIterator != prefix_list.end())
	{
		//Its in the list, lets prepend the prefix value to the param value
		std::string prefix = myIterator->second;
		strVal = prefix + strVal;
	} else {
		std::ostringstream ostr;
		ostr << "List parameter " << param << " specified with no prefix";
		throw UnexpectedInputException("ParamReader::readListParameter()", 
			ostr.str().c_str());
	}

	//Add Parameter and Value to Hash if it doesnt already exist
	if (!override_value && hasParam(strName)) {
		reportOverrideError(strName.c_str(), strVal.c_str());
	} else {
		checkForIllegalDashes(strName);
		_params()[strName] = strVal;
		_paramCount++;
	}
}

void ParamReader::readParameter(const char* param, const char* value, HashTable &prefix_list, bool override_value) 
{
	std::string strVal(value);
	std::string strName = param;

	//Add Parameter and Value to Hash if it doesnt already exist
	if (!override_value && hasParam(strName)) {
		reportOverrideError(strName.c_str(), value);
	} else {
		checkForIllegalDashes(strName);
		_params()[strName] = strVal;
		_paramCount++;
	}
}

size_t ParamReader::getParameterCount()
{
	return _paramCount;
}
bool ParamReader::getNthParam(size_t n, char* paramName, char* paramValue)
{
	checkForIllegalDashes(std::string(paramName));
	std::map <std::string,std::string>::iterator myIterator = _params().begin();

	size_t i = 0;

    for (myIterator = _params().begin(); myIterator != _params().end(); myIterator++)
    {
        if (i == n)
		{
		    strcpy(paramName,myIterator->first.c_str()); 
			strcpy(paramValue,myIterator->second.c_str());
			return true;
		}

		i++;
	}

	strcpy(paramName,""); 
	strcpy(paramName,"");
	return false;

}
void ParamReader::setParam(const char* paramName, const char* paramValue)
{
	checkForIllegalDashes(std::string(paramName));

	//Check if prefix exists for Parameter Name
	std::map <std::string,std::string>::iterator myIterator = _params().find(paramName);
	
    if (myIterator != _params().end())
    {
		myIterator->second = paramValue;
	}
	else
	{
		std::string strName(paramName);
		std::string strVal(paramValue);
		InputUtil::rel2AbsPath(strVal,_basePath.c_str());
		
       _params()[strName] = strVal;
	   _paramCount++;
	}

	// Check if it's a special meta-parameter:
	if (paramName == std::string("param_reader_output_used_param_names"))
		_output_used_param_names = true;
	if (paramName == std::string("track_parameters"))
		_paramTracker.enable();

}

void ParamReader::unsetParam(const char* paramName)
{
	//Check if prefix exists for Parameter Name
	std::map <std::string,std::string>::iterator myIterator = _params().find(paramName);	
    if (myIterator != _params().end())
    {
        _params().erase(myIterator);
	}

}
/**
	Pre-conditions: 
		paramName != NULL
		paramValue != NULL		
	
	Returns:
		true if paramName exists in _params and _params[paramName].length < paramValueSize
		false otherwise.

	Exception:
		Throws InternalInconsistencyException if _params[paramName].length < paramValueSize is not satisfied

	Output:
		Updates paramValue with _params[paramName]
		

*/
bool ParamReader::getParam(const char* paramName, char* paramValue, size_t paramValueSize) {
	outputUsedParamName(paramName);

	//Initialize the value to an empty string in case no value is returned
	//strcpy(paramValue,"");
	paramValue[0] = '\0';
	checkForIllegalDashes(std::string(paramName));

    //Check if prefix exists for Parameter Name
	std::map <std::string,std::string>::iterator myIterator = _params().find(paramName);
	
	if(myIterator != _params().end())
	{
		std::string value = myIterator->second;

		//Make Sure we have a big enough buffer
		if (value.length() < paramValueSize)
		{
			strcpy(paramValue,value.c_str());
			//cout << "getParam Called with paramName: " << paramName 
			//     << "and value returned is: " << paramValue << "\n";
			return true;
		}
		else
		{
			std::ostringstream ostr;
			ostr << "Parameter value buffer (length: " << paramValueSize << ") is not large enough "
				<< " to hold the full stored value (length: " << value.length() << ") "
				<< "for this parameter: " << paramName;
			//Have to cut off part of the value, so fail
			throw InternalInconsistencyException("ParamReader::getParam(const char*, char*, size_t)", 
				ostr.str().c_str());
		}
	}	
	return false;
}

bool ParamReader::hasParam(const char* paramName) {
	return _params().find(paramName) != _params().end();
}

bool ParamReader::hasParam(const std::string &paramName) {
	return _params().find(paramName) != _params().end();
}

std::string ParamReader::getParam( const std::string &paramName, const char* defaultValue ){
	outputUsedParamName(paramName.c_str());
	checkForIllegalDashes(paramName);
	std::map<std::string,std::string>::iterator it = _params().find(paramName);
	if( it == _params().end() ) {
		_paramTracker.markDefault(paramName, defaultValue?defaultValue:"");
		return std::string(defaultValue?defaultValue:"");
	}
	return it->second;
}

std::wstring ParamReader::getWParam(const std::string &paramName, const char* defaultValue) {
	std::string paramValue = getParam(paramName, defaultValue);
	if (paramValue.size() == 0) { return L""; }
	std::wstring temp(paramValue.begin(), paramValue.end());
	return temp;
}

std::string ParamReader::getRequiredParam( const std::string &paramName ){
	outputUsedParamName(paramName.c_str());
	checkForIllegalDashes(paramName);
	std::map<std::string,std::string>::iterator it = _params().find(paramName);
	if( it == _params().end() ) {
		reportMissingParamError(paramName.c_str());
	}
	return it->second;
}

std::wstring ParamReader::getRequiredWParam( const std::string &paramName ){
	std::string param = getRequiredParam(paramName);
	if (param.size() == 0)
		return L"";
	std::wstring temp(param.begin(), param.end());
	return temp;
}

void ParamReader::getRequiredParam(const char* paramName, char* paramValue, size_t paramValueSize) {
	getParam(paramName,paramValue,paramValueSize);
	if (strcmp(paramValue,"") == 0) {
		reportMissingParamError(paramName);
	}
}

std::string ParamReader::expand(const std::string &input) {
	std::map<std::string,std::string> overrides;
	return expand(expand(input, "+", overrides), "%", overrides);
}

std::string ParamReader::expand(const std::string &input, const std::string &boundaryChar, const std::map<std::string,std::string>& overrides) {
	size_t pos = 0;
	size_t start, end, nextSpace;
	std::string result(input);

	while((start = result.find_first_of(boundaryChar, pos)) != std::string::npos) {
		end = result.find_first_of(boundaryChar, start + 1);
		if (end == std::string::npos) return result;
		
		//if there is a space between the first boundaryChar and the second, don't try to expand
		nextSpace = result.find_first_of(' ', start+1);
		if (nextSpace != std::string::npos && nextSpace < end) {
			pos = start + 1;
			continue;
		}

		std::string paramName = result.substr(start + 1, end - start - 1);
		std::string paramValue;
		std::map<std::string,std::string>::const_iterator overrides_iterator(overrides.find(paramName));
		if (overrides_iterator != overrides.end()) {
			paramValue = overrides_iterator->second;
		} else {
			paramValue = getParam(paramName);
		}
		if (paramValue.length() == 0) {			
			std::stringstream msg; 
			msg << "Could not expand parameter '" << paramName << "'. " << 
				"If this is in a parameter file, make sure the variable parameter is defined first." ;
			throw UnexpectedInputException("ParamReader::expand", msg.str().c_str()); 
		}

		result = result.replace(start, end - start + 1, paramValue);
		pos = start+paramValue.length();
		_paramTracker.markUsed(paramName);
		//std::cout << "Expanded " << paramName << ": [" << result << "]" << std::endl;
		//std::cout << "   pos=" << pos << std::endl;
	}
	return result;
}

bool ParamReader::getOptionalTrueFalseParamWithDefaultVal(const char* paramName, bool defaultVal)
{
	std::string value = getParam(paramName);
	if (value.empty()) {
		_paramTracker.markDefault(paramName, defaultVal);
		return defaultVal;
	} else if (boost::iequals(value, "true")) {
		return true;
	} else if (boost::iequals(value, "false")) {
		return false;
	} else {
		throw reportBadValueError(paramName, "'true' or 'false'");
	}
}

bool ParamReader::getRequiredTrueFalseParam(const char* paramName)
{
	std::string value = getParam(paramName);
	if (value.empty()) {
		throw reportMissingParamError(paramName);
	} else if (boost::iequals(value, "true")) {
		return true;
	} else if (boost::iequals(value, "false")) {
		return false;
	} else {
		throw reportBadValueError(paramName, "'true' or 'false'");
	}
}

int ParamReader::getRequiredIntParam(const char* paramName)
{
	std::string value = getParam(paramName);

	if (value.empty()) {
		throw reportMissingParamError(paramName);
	} else {
		try {
			return boost::lexical_cast<int>(value);
		} catch (boost::bad_lexical_cast) { 
			throw reportBadValueError(paramName, "integer");
		} 
	}
}

int ParamReader::getOptionalIntParamWithDefaultValue(const char* paramName, int defaultVal)
{
	std::string value = getParam(paramName);
	if (value.empty()) {
		_paramTracker.markDefault(paramName, defaultVal);
		return defaultVal;
	} else {
		try {
			return boost::lexical_cast<int>(value);
		} catch (boost::bad_lexical_cast) { 
			throw reportBadValueError(paramName, "integer");
		} 
	}
}


double ParamReader::getRequiredFloatParam(const char* paramName)
{
	std::string value = getParam(paramName);
	if (value.empty()) {
		throw reportMissingParamError(paramName);
	} else {
		try {
			return boost::lexical_cast<float>(value);
		} catch (boost::bad_lexical_cast) { 
			throw reportBadValueError(paramName, "float");
		} 
	}
}

double ParamReader::getOptionalFloatParamWithDefaultValue(const char* paramName, double defaultVal)
{
	std::string value = getParam(paramName);
	if (value.empty()) {
		_paramTracker.markDefault(paramName, defaultVal);
		return defaultVal;
	} else {
		try {
			return boost::lexical_cast<float>(value);
		} catch (boost::bad_lexical_cast) { 
			throw reportBadValueError(paramName, "float");
		} 
	}
}

std::vector<std::string> ParamReader::getStringVectorParam(const char *paramName)
{
	std::vector<std::string> result;
	std::string value = getParam(paramName);
	if (!value.empty())
		boost::split(result, value, boost::is_any_of(","));
	return result;
}

std::vector<int> ParamReader::getIntVectorParam(const char *paramName)
{
	std::vector<std::string> strResult;
	std::vector<int> result;
	std::string value = getParam(paramName);
	if (!value.empty())
		boost::split(strResult, value, boost::is_any_of(","));

	BOOST_FOREACH(std::string s, strResult)
		result.push_back(atoi(s.c_str()));

	return result;
}

std::vector<std::wstring> ParamReader::getWStringVectorParam(const char *paramName) {
	std::vector<std::wstring> result;
	std::string narrow_value = getParam(paramName);
	if (!narrow_value.empty()) {
		std::wstring value(narrow_value.begin(), narrow_value.end()); 
		boost::split(result, value, boost::is_any_of(","));
	}
	return result;
}

std::vector<Symbol> ParamReader::getSymbolVectorParam(const char *paramName)
{
	std::vector<Symbol> result;
	std::string value = getParam(paramName);
	if (!value.empty()) {
		std::vector<std::string> pieces;
		std::string paramVal = getParam(paramName);
		boost::split(pieces, paramVal, boost::is_any_of(","));
		for (size_t i=0; i<pieces.size(); ++i) {
			result.push_back(Symbol(UnicodeUtil::toUTF16StdString(pieces[i])));
		}
	}
	return result;
}

size_t ParamReader::getSymbolArrayParam(const char *paramName, Symbol *sarray, int maxSize)
{
	std::vector<Symbol> vec = getSymbolVectorParam(paramName);
	if (vec.size() >= static_cast<size_t>(maxSize)) {
		std::ostringstream ostr;
		ostr << "Param '" << paramName << "' is too large (" << vec.size() 
			 << ") for array size (" << maxSize << ")";
		throw UnexpectedInputException("ParamReader::getSymbolArrayParam()",
			ostr.str().c_str());
	}
	for (size_t i=0; i<vec.size(); ++i) {
		sarray[i] = vec[i];
	}
	return vec.size();
}

bool ParamReader::isParamTrue(const char* paramName)
{
	return getOptionalTrueFalseParamWithDefaultVal(paramName, false);
}

void ParamReader::logParams()
{
	//Check if prefix exists for Parameter Name
	std::map<std::string,std::string>::iterator myIterator = _params().begin();

    for (myIterator = _params().begin(); myIterator != _params().end(); myIterator++)
    {
		SessionLogger::info("log_params_0") 
			<< "  - " << myIterator->first.c_str() 
			<< ": " << myIterator->second.c_str();
	}
}

Symbol ParamReader::getParam(Symbol key)
{
	std::string value = getParam(key.to_debug_string());
	if (value.empty()) {
		return Symbol();
	} else {
		return Symbol(UnicodeUtil::toUTF16StdString(value));
	}
}

bool ParamReader::getRequiredTrueFalseParam(Symbol key)
{
	return getRequiredTrueFalseParam(key.to_debug_string());
}

int ParamReader::getRequiredIntParam(Symbol key)
{
	return getRequiredIntParam(key.to_debug_string());
}

bool ParamReader::getNarrowParam(char *result, Symbol key, int max_len) 
{
    return getParam(key.to_debug_string(),result,max_len);
}

void ParamReader::getRequiredNarrowParam(char *result, Symbol key, int max_len)
{
	getRequiredParam(key.to_debug_string(),result,max_len);
}

int ParamReader::checkInclude(const char* incName)
{
    //Check if prefix exists for Parameter Name
	std::map<std::string,int>::iterator myIterator = _includes().find(incName);
	
    if(myIterator != _includes().end())
    {
        //Its in the list, return true since it exists 1=unprocessed 2=processed
		return myIterator->second;
	}

	//Not found in list so return false
	return 0;
}



void ParamReader::checkForIllegalDashes(std::string paramName)
{
	if (paramName.find('-') != std::string::npos) {
		std::stringstream msg; 
		msg << "Illegal parameter '" << paramName << "' (dashes are not allowed)";
		throw UnexpectedInputException("ParamReader::checkForIllegalDashes()",msg.str().c_str());
	}
}

bool ParamReader::hasDataDriveString(const std::string& path)
{
	return (boost::algorithm::starts_with(path, LOCAL_DATA_DRIVE_WILDCARD));
}

void ParamReader::replaceDataDriveString(std::string& path)
{
#if defined(_WIN32)
	if (!hasDataDriveString(path))
	{
		std::stringstream msg; 
		msg << "Illegal path '" << path << "' (no data drive wildcard)";
		throw UnexpectedInputException("ParamReader::replaceDataDriveString()", msg.str().c_str());
	}

	if (!boost::algorithm::starts_with(path, LOCAL_DATA_DRIVE_WILDCARD)) {
		std::ostringstream err;
		err << "Illegal path '" << path << "'; the special symbol " << LOCAL_DATA_DRIVE_WILDCARD 
			<< " should only appear at the begining of a value.";
		throw UnexpectedInputException("ParamReader::replaceDataDriveString()", err.str().c_str());
	}

	// restOfPath: e.g., /SERIFData/2010.04.25-r3000/ace/.../.../...
	std::string restOfPath = path.substr(path.find(LOCAL_DATA_DRIVE_WILDCARD) + strlen(LOCAL_DATA_DRIVE_WILDCARD));

	// in the restOfPath, find the SERIFData folder
	// i.e., ignore the rest of the path (because it may not be a real path; it may be a prefix)
	std::string serifDataDir = restOfPath.substr(1, restOfPath.find("/"));

	std::string drives[] = {"c:", "d:", "e:", "f:"};
	for (int i = 0; i < sizeof(drives)/sizeof(std::string); i++) 
	{
		std::string localPath = drives[i] + serifDataDir;
		if (_access(localPath.c_str(), 0) == 0) 
		{
			path.assign(drives[i] + restOfPath);
			return;
		}
	}

	// no localized path found
	std::stringstream msg; 
	msg << "Localized path not found: " << path;
	//cout << msg.str() << endl;
	throw UnrecoverableException("ParamReader::replaceDataDriveString()", msg.str().c_str());
#else
	// ERROR; non-windows localization not supported
	std::stringstream msg; msg << "Non-Windows data localization not supported: " << path;
	throw UnexpectedInputException("ParamReader::replaceDataDriveString()", msg.str().c_str());
#endif
}
