#include "Generic/common/leak_detection.h"

#include "FeatureImpliesClassConstraint.h"
#include <sstream>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "Generic/common/UnexpectedInputException.h"

using std::wstringstream;
using boost::split;
using boost::is_any_of;

GraphicalModel::FICCDescription GraphicalModel::parseFICCDescription(
		const std::wstring& constraintData) 
{
	GraphicalModel::FICCDescription desc;

	std::vector<std::wstring> lineParts;

	split(lineParts, constraintData, is_any_of(L";"));
	if (lineParts.size() != 2) {
		wstringstream err;
		err << L"String needed 2 parts, but has " << lineParts.size()
			<< L": " << constraintData;
		std::wcerr << 
		 L"String needed 2 parts, but has " << lineParts.size()
			<< L": " << constraintData << std::endl;
		std::wcerr << lineParts[0] << std::endl;
		throw UnexpectedInputException("createConstraints", err);
	}

	split(desc.classes, lineParts[0], is_any_of(L","));
	split(desc.features, lineParts[1], is_any_of(L","));

	return desc;
}

