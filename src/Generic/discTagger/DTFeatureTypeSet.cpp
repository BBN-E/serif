// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/discTagger/DTWordNameListFeatureType.h"
#include "Generic/discTagger/DTMaxMatchingListFeatureType.h"
#include "Generic/discTagger/DTWordLCNameListFeatureType.h"
#include "Generic/discTagger/IdFListBasedContextFeatureType.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/ParamReader.h"
#include <sstream>
#include <string>
#include <set>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

void strcpy_wc(char *Dest, const wchar_t *Src);
std::string wString2String(const std::wstring& ws);
std::wstring string2WString(const std::string& s);

DTFeatureTypeSet::DTFeatureTypeSet(int n_feature_types)
	: _n_feature_types(0), _array_size(n_feature_types)
{
	_featureTypes = _new DTFeatureType*[n_feature_types];
}

DTFeatureTypeSet::DTFeatureTypeSet(const char *features_file, Symbol modeltype, DTFeatureTypeSet *superset)
	: _n_feature_types(0), _featureTypes(0)
{
	readFeatures(features_file, modeltype, superset);
}

DTFeatureTypeSet::DTFeatureTypeSet(const char *features_file, Symbol modeltype)
	: _n_feature_types(0), _featureTypes(0)
{
	readFeatures(features_file, modeltype, 0);
}


void DTFeatureTypeSet::readFeatures(const char *features_file, Symbol modeltype, DTFeatureTypeSet *superset)
{	
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(features_file);
	if (in.fail()) {
		std::string message = "Could not open features file: ";
		message += features_file;
		throw UnexpectedInputException("DTFeatureTypeSet::DTFeatureTypeSet()", message.c_str());
	}


	// prepare a map to check that the featureTypes are in the superset
	set<Symbol> super_feature_set;
	//cerr<<"superset:"<<superset<<endl;
	if(superset!=0){
		//cerr<<"inserting features into the set"<<endl;
		for(int i=0; i<superset->getNFeaturesTypes(); i++){
//			cerr<<"inserting feature: "<<superset->getFeatureType(i)->getName().to_debug_string()<<endl;
			super_feature_set.insert(superset->getFeatureType(i)->getName().to_string());
		}
	}

	in >> _array_size;
	delete _featureTypes; // delete the old array (if any)
	_featureTypes = _new DTFeatureType*[_array_size];
	std::wstring line1;
	in.getLine(line1); // take out the trailing newline

	UTF8Token token;
	std::wstring dict_name, dict_file_name;
	std::string dict_file_name_str;
	while (!in.eof()) {
/*		in >> token;*/
		std::wstring line;
		in.getLine(line);
		//std::wcerr<<L"line is: "<<line<<L"\n";
		if (line[0] == L'\0')
			break;

		if (_n_feature_types == _array_size) {
			throw UnexpectedInputException("DTFeatureTypeSet::DTFeatureTypeSet()",
				"Feature type count in features file is too low.");
		}
		bool result=false;
		if((int)line.find(L"wlist")>=0) {
			//std::cerr << use_context << " " << line << std::endl;
			std::wstringstream oss(line);

			// Check what type of wordlist it is:
			std::wstring list_type;
			oss >> list_type;
			boost::replace_all(list_type, "-", "_");
			bool is_lowercase_list = false;
			bool use_context = false;
			bool do_hybrid = false;
			int offset = 0;
			if (boost::iequals(list_type, "wlist")) {
				// default
			} else if (boost::iequals(list_type, "lc_wlist")) {
				is_lowercase_list = true;
			} else if (boost::iequals(list_type, "wlist_with_context")) {
				use_context = true;
			} else if (boost::iequals(list_type, "lc_wlist_with_context")) {
				use_context = true;
				is_lowercase_list = true;
			} else if (boost::iequals(list_type, "wlist_hybrid")) {
				do_hybrid = true;
			} else if (boost::iequals(list_type, "lc_wlist_hybrid")) {
				do_hybrid = true;
				is_lowercase_list = true;
			} else if (boost::iequals(list_type, "lc_wlist_next")) {
				is_lowercase_list = true;
				offset = 1;
			} else if (boost::iequals(list_type, "lc_wlist_prev")) {
				is_lowercase_list = true;
				offset = -1;
			} else if (boost::iequals(list_type, "lc_wlist_with_context_next")) {
				use_context = true;
				is_lowercase_list = true;
				offset = 1;
			} else if (boost::iequals(list_type, "lc_wlist_with_context_prev")) {
				use_context = true;
				is_lowercase_list = true;
				offset = -1;
			} else if (boost::iequals(list_type, "lc_wlist_hybrid_next")) {
				do_hybrid = true;
				is_lowercase_list = true;
				offset = 1;
			} else if (boost::iequals(list_type, "lc_wlist_hybrid_prev")) {
				do_hybrid = true;
				is_lowercase_list = true;
				offset = -1;
			} else {
				throw UnexpectedInputException("DTFeatureTypeSet::DTFeatureTypeSet()",
											   "Unexpected wlist type ", 
											   Symbol(line).to_debug_string());
			}

			oss >> dict_name; // get the actual dictionary name
			oss >> dict_file_name; // file-name containing the word list

			dict_file_name_str = wString2String(dict_file_name);
			dict_file_name_str = ParamReader::expand(dict_file_name_str);
			dict_file_name = string2WString(dict_file_name_str);

			//std::wcerr<<L"wlist-name: "<<dict_name<<L"\n";
			//std::wcerr<<L"wlist-file: "<<dict_file_name<<L"\n";
			wstring listName = DTWordNameListFeatureType::getNameWorldListName(Symbol(dict_name.c_str()), 
																		is_lowercase_list, use_context, do_hybrid, offset);
			if (superset!=0 && super_feature_set.find(listName.c_str()) == super_feature_set.end()) {
				char message[500];
				char *cdict_name = _new char[100];
				strcpy(message, "Dictionary Feature Type is not in the feature superset: '");
				strcpy_wc(cdict_name,dict_name.c_str());
				strncat(message, cdict_name, 400);
				strcat(message, "'");
				throw UnexpectedInputException("DTFeatureTypeSet::DTFeatureTypeSet()",
					message);
			}
			result = addDictionaryFeaturetype(modeltype, Symbol(dict_name.c_str()), Symbol(dict_file_name.c_str()), 
											  is_lowercase_list, use_context, do_hybrid, offset);
		} else if((int)line.find(L"mm_list")>=0) {
			//std::cerr << use_context << " " << line << std::endl;
			std::wstringstream oss(line);

			std::wstring list_type;
			oss >> list_type;
			boost::replace_all(list_type, "-", "_");

			oss >> dict_name; // get the actual dictionary name
			oss >> dict_file_name; // file-name containing the word list

			dict_file_name_str = wString2String(dict_file_name);
			dict_file_name_str = ParamReader::expand(dict_file_name_str);
			dict_file_name = string2WString(dict_file_name_str);

			//std::wcerr<<L"wlist-name: "<<dict_name<<L"\n";
			//std::wcerr<<L"wlist-file: "<<dict_file_name<<L"\n";
			wstring listName = dict_name;
			if (superset!=0 && super_feature_set.find(listName.c_str()) == super_feature_set.end()) {
				char message[500];
				char *cdict_name = _new char[100];
				strcpy(message, "Dictionary Feature Type is not in the feature superset: '");
				strcpy_wc(cdict_name,dict_name.c_str());
				strncat(message, cdict_name, 400);
				strcat(message, "'");
				throw UnexpectedInputException("DTFeatureTypeSet::DTFeatureTypeSet()",
					message);
			}
			result = addMaxMatchingListFeaturetype(modeltype, Symbol(dict_name.c_str()), Symbol(dict_file_name.c_str()));
		} else if((int)line.find(L"list-context")>=0) {
			std::wstringstream oss(line);
			std::wstring feat_type;
			oss >> feat_type;
			bool is_lowercase = false;
			if (boost::iequals(feat_type, "list-context")) {
				//default
			} else if (boost::iequals(feat_type, "lc_list-context")) {
				is_lowercase = true;
			} else {
				throw UnexpectedInputException("DTFeatureTypeSet::DTFeatureTypeSet()",
											   "Unexpected list-context type ", 
											   Symbol(line).to_debug_string());
			}

			oss >> dict_name;

			std::wstring list_file;
			oss >> list_file;

			std::string list_file_str = wString2String(list_file);
			list_file_str = ParamReader::expand(list_file_str);
			list_file = string2WString(list_file_str);

			wstring featName = IdFListBasedContextFeatureType::getFeatureName(
				Symbol(dict_name.c_str()), is_lowercase);

			if (superset!=0 && super_feature_set.find(featName) == super_feature_set.end()) {
				char message[500];
				char *cdict_name = _new char[100];
				strcpy(message, "Dictionary Feature Type is not in the feature superset: '");
				strcpy_wc(cdict_name,featName.c_str());
				strncat(message, cdict_name, 400);
				strcat(message, "'");
				throw UnexpectedInputException("DTFeatureTypeSet::DTFeatureTypeSet()", message);
			}

			result = addListContextFeatureType(modeltype, Symbol(dict_name.c_str()), Symbol(list_file.c_str()), is_lowercase);

		} else {
			if (superset!=0 && super_feature_set.find(Symbol(line.c_str()))==super_feature_set.end()) {
				char message[500];
				char *feature_name = _new char[100];
				strcpy(message, "Feature Type is not in the feature superset: '");
				strcpy_wc(feature_name,line.c_str());
				strncat(message, feature_name, 400);
				strcat(message, "'");
				throw UnexpectedInputException("DTFeatureTypeSet::DTFeatureTypeSet()",
					message);
			}
			//std::wcerr<<"adding feature: "<<line.c_str()<<std::endl;
			result = addFeatureType(modeltype, Symbol(line.c_str()));
		}
		if (result == false) {
			char message[500];
			strcpy(message, "Unable to resolve feature type: '");
			strncat(message, Symbol(line.c_str()).to_debug_string(), 400);
			strcat(message, "'");
			throw UnexpectedInputException("DTFeatureTypeSet::DTFeatureTypeSet()",
				message);
		}
		//std::cerr<<endl<<endl;
	}
	//std::cerr<<"finished reading the features types...\n";
	//std::cerr.flush();
	if (_n_feature_types != _array_size) {
		throw UnexpectedInputException("DTFeatureTypeSet::DTFeatureTypeSet()",
			"Feature type count in features file is inaccurate.");
	}

	in.close();
}

DTFeatureTypeSet::~DTFeatureTypeSet() {
	delete[] _featureTypes;
}

bool DTFeatureTypeSet::addFeatureType(Symbol model, Symbol name) {
	if (_n_feature_types >= _array_size) {
		throw InternalInconsistencyException(
			"DTFeatureTypeSet::addFeatureType()",
			"Caller tried to add too many feature types.");
	}

	DTFeatureType *featureType = DTFeatureType::getFeatureType(model, name);

	if (featureType == 0)
		return false;

	// check that the feature has the required params
	// enables a feature to do some initialization once only if it is used.
	featureType->validateRequiredParameters(); 

	_featureTypes[_n_feature_types++] = featureType;
	return true;
}

bool DTFeatureTypeSet::addDictionaryFeaturetype(Symbol model, Symbol dict_name, Symbol dict_filename, 
												bool is_lowercase_list, bool use_context, bool do_hybrid, int offset)
{
	if (_n_feature_types >= _array_size) {
		throw InternalInconsistencyException(
			"DTFeatureTypeSet::addFeatureType()",
			"Caller tried to add too many feature types.");
	}

	wstring listName = DTWordNameListFeatureType::getNameWorldListName(dict_name, 
		is_lowercase_list, use_context, do_hybrid, offset);

	DTFeatureType *featureType = DTFeatureType::getFeatureType(model, listName.c_str());

	if(featureType==0){
		featureType = _new DTWordNameListFeatureType(model, dict_name, dict_filename, 
													 is_lowercase_list, use_context, do_hybrid, offset);
	}
	

	// check that the feature has the required params
	featureType->validateRequiredParameters(); 
	
	if (featureType == 0)
		return false;

	_featureTypes[_n_feature_types++] = featureType;
	return true;
}


bool DTFeatureTypeSet::addMaxMatchingListFeaturetype(Symbol model, Symbol dict_name, Symbol dict_filename)
{
	if (_n_feature_types >= _array_size) {
		throw InternalInconsistencyException(
			"DTFeatureTypeSet::addFeatureType()",
			"Caller tried to add too many feature types.");
	}

	DTFeatureType *featureType = DTFeatureType::getFeatureType(model, dict_name.to_string());

	if(featureType==0){
		featureType = _new DTMaxMatchingListFeatureType(model, dict_name, dict_filename);
	}
	

	// check that the feature has the required params
	featureType->validateRequiredParameters(); 
	
	if (featureType == 0)
		return false;

	_featureTypes[_n_feature_types++] = featureType;
	return true;
}

bool DTFeatureTypeSet::addListContextFeatureType(Symbol model, Symbol dict_name, Symbol list, bool is_lowercase)
{
	if (_n_feature_types >= _array_size) {
		throw InternalInconsistencyException(
			"DTFeatureTypeSet::addListContextFeatureType()",
			"Caller tried to add too many feature types.");
	}

	wstring featName = IdFListBasedContextFeatureType::getFeatureName(dict_name, is_lowercase);

	DTFeatureType *featureType = DTFeatureType::getFeatureType(model, featName.c_str());
	if (featureType==0) {
		featureType = _new IdFListBasedContextFeatureType(dict_name, list, is_lowercase);
	}
	
	// check that the feature has the required params
	featureType->validateRequiredParameters(); 
	
	if (featureType == 0) return false;

	_featureTypes[_n_feature_types++] = featureType;
	return true;
}

/*
 * convert wchar to char
 */
void strcpy_wc(char *Dest, const wchar_t *Src) {
	while (*Src){
		*Dest++ = static_cast<char>(*Src++);
	}
	*Dest = 0;
}

/* 
* convert between wstring to string
*/

std::string wString2String(const std::wstring& ws)
{
	string s(ws.begin(), ws.end());
    s.assign(ws.begin(), ws.end());
	return s;
}

std::wstring string2WString(const std::string& s)
{
	std::wstring ws(s.length(),L' ');
	std::copy(s.begin(), s.end(), ws.begin());
	return ws;
}
