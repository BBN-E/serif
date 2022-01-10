// Copyright (c) 2018 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/constraints/LocationNameOverlapConstraint.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"


#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/foreach.hpp>

#include <string>

LocationNameOverlapConstraint::LocationNameOverlapConstraint() {
	std::string prefixFile = ParamReader::getParam("contraining_location_name_prefixes");
	std::string suffixFile = ParamReader::getParam("contraining_location_name_suffixes");

	if (prefixFile.length() == 0 && suffixFile.length() == 0)
		return;

	if (prefixFile.length() > 0) 
		readConstraintFile(prefixFile, _constrainingPrefixes);

	if (suffixFile.length() > 0)
		readConstraintFile(suffixFile, _constrainingSuffixes);
}

void LocationNameOverlapConstraint::readConstraintFile(std::string& inputFile, std::vector<std::wstring>& constraintList) {
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(inputFile));
	UTF8InputStream& stream(*stream_scoped_ptr);
	std::wstring line;
	while (stream) {
		stream.getLine(line);
		if (!stream) 
			break;
		line = boost::algorithm::trim_copy(line);
		if (line.length() == 0 || line.at(0) == '#')
			continue;
		boost::algorithm::to_lower(line);
		constraintList.push_back(line);
	}
	stream.close();
}

bool LocationNameOverlapConstraint::violatesMergeConstraint(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	if (_constrainingPrefixes.size() == 0) 
		return false;

	// Only apply to locations
	if (!m1->getEntityType().matchesGPE() && !m1->getEntityType().matchesLOC())
		return false;
	if (!m2->getEntityType().matchesGPE() && !m2->getEntityType().matchesLOC())
		return false;

	std::wstring m1Name = boost::algorithm::trim_copy(m1->getAtomicHead()->toTextString());
	std::wstring m1NameLower = boost::algorithm::to_lower_copy(m1Name);
	std::wstring m2Name = boost::algorithm::trim_copy(m2->getAtomicHead()->toTextString());
	std::wstring m2NameLower = boost::algorithm::to_lower_copy(m2Name);
		
	BOOST_FOREACH(std::wstring prefix, _constrainingPrefixes) {
		std::wstring testName1(L"");
		testName1 += prefix;
		testName1 += L" ";
		testName1 += m1NameLower;

		std::wstring testName2(L"");
		testName2 += prefix;
		testName2 += L" ";
		testName2 += m2NameLower;

		//std::cout << "Comparing " << UnicodeUtil::toUTF8StdString(testName1) << " to " << UnicodeUtil::toUTF8StdString(m2NameLower) << " and \n";
		//std::cout << "Comparing " << UnicodeUtil::toUTF8StdString(testName2) << " to " << UnicodeUtil::toUTF8StdString(m1NameLower) << " and \n";

		if (m2NameLower.find(testName1) == 0 || m1NameLower.find(testName2) == 0) {
			//std::cout << "Returning true\n";
			return true;

		}
	}

	BOOST_FOREACH(std::wstring suffix, _constrainingSuffixes) {
		std::wstring testName1(L"");
		testName1 += m1NameLower;
		testName1 += L" ";
		testName1 += suffix;

		std::wstring testName2(L"");
		testName2 += m2NameLower;
		testName2 += L" ";
		testName2 += suffix;

		//std::cout << "Comparing " << UnicodeUtil::toUTF8StdString(testName1) << " to " << UnicodeUtil::toUTF8StdString(m2NameLower) << " and \n";
		//std::cout << "Comparing " << UnicodeUtil::toUTF8StdString(testName2) << " to " << UnicodeUtil::toUTF8StdString(m1NameLower) << " and \n";

		if (m2NameLower.find(testName1) == 0 || m1NameLower.find(testName2) == 0) {
			//std::cout << "Returning true\n";
			return true;

		}
	}
	
	return false;
}
