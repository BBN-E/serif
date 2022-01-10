#include "Generic/common/leak_detection.h"
#include "ProblemDefinition.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"

using std::wstring;
using std::string;
using std::vector;
using std::find;
using std::wstringstream;
using std::wifstream;
using boost::split;
using boost::is_any_of;
using boost::make_shared;
using boost::lexical_cast;
using boost::wregex;
using boost::match_results;
using boost::regex_match;

unsigned int ProblemDefinition::classNumber(const wstring& className) const {
	vector<wstring>::const_iterator it = find(_classNames.begin(), 
			_classNames.end(), className);

	if (it != _classNames.end()) {
		return it - _classNames.begin();
	} else {
		std::wcout << className << std::endl;
		wstringstream err;
		err << className << L" is not a known class name.";
		throw UnexpectedInputException("ProblemDefinition::classNumbers", 
				err);
	}
}

struct BadLineException {
	BadLineException(size_t num, const std::wstring& line) : num(num), line(line) {}
	std::wstring line;
	size_t num;
};

void processClassesLine(const wstring& classesLine, vector<wstring>& classNames,
		vector<ConstraintDescription>& constraintData, size_t line_no)
{
	 //Artifact[ORG,VEH,WEA,FAC];Buyer[PER,ORG,GPE];Seller[PER;ORG];gbg_p[PER];gbg_o[ORG,WEA,VEH];gbg_l[LOC,FAC] 
	vector<wstring> parts;
	split(parts, classesLine, is_any_of(L";"));

	BOOST_FOREACH(const std::wstring& part, parts) {
		static const wregex part_re(L"(\\w+)\\[([A-Z_,]+)\\]");
		match_results<wstring::const_iterator> what;
		if (regex_match(part, what, part_re)) {
			wstring className(what[1].first, what[1].second);
			classNames.push_back(className);
			SessionLogger::info("role_name") << L"Read role name " << className;
			constraintData.push_back(ConstraintDescription(1.0, L"LegalEntities", part));
		} else {
			throw BadLineException(line_no, part);
		}
	}
}

void processDecoderLine(const wstring& decoderLine, ProblemDefinition& problem,
		std::vector<ProblemDefinition::DecoderGuide>& decoderStrings, size_t line_no)
{
	// Artifact[HR];Buyer[H];Seller[L]   
	vector<wstring> parts;
	split(parts, decoderLine, is_any_of(L";"));

	BOOST_FOREACH(const std::wstring& part, parts) {
		static const wregex part_re(L"(\\w+)\\[([HMLR]+)\\]");
		match_results<wstring::const_iterator> what;
		if (regex_match(part, what, part_re)) {
			wstring className(what[1].first, what[1].second);
			wstring decoderString(what[2].first, what[2].second);
			decoderStrings.push_back(make_pair(problem.classNumber(className), 
						decoderString));
		} else {
			throw BadLineException(line_no, part);
		}
	}
}

ProblemDefinition_ptr ProblemDefinition::load(const string& filename) {
	if (!boost::filesystem::exists(filename)) {
		std::stringstream str;
		str << "Problem definition file " << filename <<
			" does not exist.";
		throw UnexpectedInputException("ProblemDefinition::load",
				str.str().c_str());
	}

	size_t line_no = 0;
	try {
		vector<wstring> classNames;
		vector<wstring> priorsStrings;
		vector<double> priors;
		vector<ConstraintDescription> constraintData;
		vector<InstanceConstraintDescription> instanceConstraintData;

		wifstream in(filename.c_str());

		wstring eventType;
		getline(in, eventType); ++line_no;
		wstring psgSize;
		getline(in, psgSize); ++line_no;
		unsigned int max_passage_size = 0;
		try {
			max_passage_size=lexical_cast<unsigned int>(psgSize);
		} catch (boost::bad_lexical_cast&) {
			throw BadLineException(line_no, psgSize);
		}

		wstring classesLine;
		getline(in,classesLine); ++line_no;
		processClassesLine(classesLine, classNames, constraintData, line_no);
		wstring decoderLine;
		getline(in, decoderLine); ++line_no;
		// processed after creating the problem definition object


		wstring priorsLine;
		
		getline(in, priorsLine); ++line_no;
		split(priorsStrings,priorsLine,is_any_of(L","));
		try {
			BOOST_FOREACH(const std::wstring& ps, priorsStrings) {
				priors.push_back(lexical_cast<double>(ps));
			}} catch (boost::bad_lexical_cast&) {
				throw BadLineException(line_no, priorsLine);
			}

		wstring constraintLine;
		while(getline(in, constraintLine)) {
			++line_no;
			if (!constraintLine.empty() && constraintLine[0]!=L'#') {
				std::vector<std::wstring> splitOnTabs;
				split(splitOnTabs, constraintLine, is_any_of(L"\t"));

				// instance constraints all begin with "instance"
				if (splitOnTabs[0] == L"instance") {
					if (splitOnTabs.size() == 4) {
						instanceConstraintData.push_back(
								InstanceConstraintDescription(splitOnTabs[1],
									splitOnTabs[2], splitOnTabs[3]));
					} else {
						throw BadLineException(line_no, constraintLine);
					}
				} else if (splitOnTabs[0] == L"entity") {
					// entity role distributions all begin with "entity"
					// example:
					// entity  PER     0.6<=gbg_p      0.1<=Buyer<=0.8 0.05<=Seller
					if (splitOnTabs.size() >=3) {
						std::wstring entity = splitOnTabs[1];
						for (size_t i=2; i<splitOnTabs.size(); ++i) {
							static const wregex boundRE(
									L"(([0-9.]+)<=)?(\\w+)(<=([0-9.]+))?");
							match_results<wstring::const_iterator> what;
							if (regex_match(splitOnTabs[i], what, boundRE)) {
								std::wstring lowerBound(what[2].first, what[2].second);
								std::wstring className(what[3].first, what[3].second);
								std::wstring upperBound(what[5].first, what[5].second);

								if (className.empty()) {
									throw BadLineException(line_no, splitOnTabs[i]);
								}

								std::wstringstream data;
								data << className << L";" << entity;

								double lowerBoundNum;
								try {
									lowerBoundNum = lexical_cast<double>(lowerBound);
								} catch (boost::bad_lexical_cast&) {
									throw BadLineException(line_no, splitOnTabs[i]);
								}
								constraintData.push_back(ConstraintDescription(
										lowerBoundNum, L"EntityTypeImpliesClass",
											data.str()));

								if (!upperBound.empty()) {
									std::wstringstream err;
									err << L"In line " << line_no << 
										L", upper bounds not yet supported.";
									throw UnexpectedInputException(
											"ProblemDefinition::load", err);
								}
							} else {
								throw BadLineException(line_no, splitOnTabs[i]);
							}
						}
					} else {
						throw BadLineException(line_no, constraintLine);
					}
				} else { // while corpus constraints begin with a weight
					if (splitOnTabs.size() == 3) {
						wstring constraintName = splitOnTabs[1];
						wstring rest = splitOnTabs[2];

						try {
							constraintData.push_back(
									ConstraintDescription(lexical_cast<double>(splitOnTabs[0]),
										constraintName, rest));
						} catch (boost::bad_lexical_cast&) {
							throw BadLineException(line_no, constraintLine);
						}
					} else {
						throw BadLineException(line_no, constraintLine);
					}
				}
			}
		}
		ProblemDefinition_ptr ret = make_shared<ProblemDefinition>(eventType, 
				max_passage_size, classNames, priors, 
				constraintData, instanceConstraintData);
		processDecoderLine(decoderLine, *ret, ret->_decoderStrings, line_no);
		return ret;
	} catch (const BadLineException& ble) {
		wstringstream err;
		err << L"Bad line " << ble.num << L" in problem definition: " << ble.line;
		throw UnexpectedInputException("ProblemDefinition::load", err);
	}
}

const std::wstring& ProblemDefinition::className(unsigned int idx) const {
	return _classNames[idx];
}

