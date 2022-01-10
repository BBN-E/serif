// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef WIKIPEDIA_CLUSTER_CLASS_H
#define WIKIPEDIA_CLUSTER_CLASS_H

#include <wchar.h>
#include <string.h>
#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/WikipediaClustering/WikipediaClusterTable.h"

class WikipediaClusterClass {

public:
	WikipediaClusterClass(Symbol word, bool lowercase = false) {
		//std::wcout<<word<<std::endl;
 		_c4 = prefix(word, 4, lowercase);
		_c8 = prefix(word, 8, lowercase);
		_c12 = prefix(word, 12, lowercase);
		_c16 = prefix(word, 16, lowercase);
		_c20 = prefix(word, 20, lowercase);
		_distance = WikipediaClusterTable::getDistance(word);
		_distanceType = WikipediaClusterTable::getDistanceType(word);
		_distanceCenter = WikipediaClusterTable::getCenter(word);
	}

	WikipediaClusterClass(const WikipediaClusterClass &other) {
		_c4 = other._c4;
		_c8 = other._c8;
		_c12 = other._c12;
		_c16 = other._c16;
		_c20 = other._c20;
		_distance = other._distance;
		_distanceType = other._distanceType;
		_distanceCenter = other._distanceCenter;
	}

	static WikipediaClusterClass nullCluster() {
		WikipediaClusterClass c;
		c._c4.empty();
		c._c8.empty();
		c._c12.empty();
		c._c16.empty();
		c._c20.empty();
		c._distance = -1;
		//c._distanceType
		return c;
	}

	std::vector<std::wstring> c4() const { return _c4; }
	std::vector<std::wstring> c8() const { return _c8; }
	std::vector<std::wstring> c12() const { return _c12; }
	std::vector<std::wstring> c16() const { return _c16; }
	std::vector<std::wstring> c20() const { return _c20; }
	int getDistance() const { return _distance; }
	Symbol getDistanceType() const {return _distanceType;}
	Symbol getCenter() const {return _distanceCenter;}

	bool operator==(const WikipediaClusterClass &other) const {
		return (_c4 == other._c4  &&
				_c8 == other._c8 &&
				_c12 == other._c12 &&
				_c16 == other._c16 &&
				_c20 == other._c20 && 
				_distance == other._distance &&
				_distanceType == other._distanceType && 
				_distanceCenter == other._distanceCenter);
	}

	WikipediaClusterClass &operator=(const WikipediaClusterClass &other) {
		_c4 = other._c4;
		_c8 = other._c8;
		_c12 = other._c12;
		_c16 = other._c16;
		_c20 = other._c20;
		_distance = other._distance;
		_distanceType = other._distanceType;
		_distanceCenter = other._distanceCenter;
		return *this;
	}

private:
	std::vector<std::wstring> _c4;
	std::vector<std::wstring> _c8;
	std::vector<std::wstring> _c12;
	std::vector<std::wstring> _c16;
	std::vector<std::wstring> _c20;
	int _distance;
	Symbol _distanceType;
	Symbol _distanceCenter;
	WikipediaClusterClass() {}

	static std::vector<std::wstring> prefix(Symbol word, int num_bits, bool lowercase = false) {
		/* Wiki paths are of the form #-#-#-#-#  with each number corresponding to a level 
			of the path
		*/
		std::vector<std::wstring> returnVector;
		std::vector<Symbol> *codes = WikipediaClusterTable::get(word, lowercase);
		size_t codesSize = 0;
		if(codes != NULL){
			codesSize = codes->size();
		}
		for (size_t i=0; i < codesSize; i++){
			Symbol word_code = codes->at(i);
			const wchar_t* prefix = word_code.to_string();
			int dashCount = 0;
			size_t count = 0;
			std::wstring strPrefix = prefix;			
			size_t prefixLength = strPrefix.length();
			std::wstring strReturn = L"";

			while (count <prefixLength){
				size_t index = strPrefix.find(L"-",count+1);
				if( index != std::string::npos){
					dashCount++;
					if (dashCount == num_bits-1){
						strReturn = strPrefix.substr(0);
					}
					else if(dashCount == num_bits){
						strReturn = strPrefix.substr(0,index);
						index = prefixLength;
					}
					count = index;
				}
				else{
					count = prefixLength;
				}

			}
			
			
			//std::wcout<<num_bits<<" "<<strReturn.c_str()<<std::endl;
			if (strReturn.compare(L"") != 0){
				//check if already in vector
				bool isNew = true;
				size_t vecSize = returnVector.size();
				for (size_t i= 0; i<vecSize; i++){
					if (strReturn.compare(returnVector.at(i)) == 0){
						isNew = false;
					}
				}
				if(isNew){
					double maxPerplexity = WikipediaClusterTable::getMaxPerplexity();
					if(WikipediaClusterTable::getPerplexity(Symbol(strReturn.c_str())) <= maxPerplexity){
						returnVector.push_back(strReturn);
					}
				}
			}
		}
		return returnVector;
	}
};
#endif
