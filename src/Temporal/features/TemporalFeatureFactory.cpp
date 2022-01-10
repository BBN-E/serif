#include "Generic/common/leak_detection.h"
#include "TemporalFeatureFactory.h"

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include "Generic/common/UnexpectedInputException.h"
#include "Temporal/features/ParticularFeatures.h"
#include "Temporal/features/TemporalWordInSentenceFeature.h"
#include "Temporal/features/TemporalEventFeature.h"
#include "Temporal/features/TemporalRelationFeature.h"
#include "Temporal/features/DatePropSpine.h"
#include "Temporal/features/TemporalBiasFeature.h"
#include "Temporal/features/PropFeatures.h"
//#include "Temporal/features/RelationSourceFeature.h"
#include "Temporal/features/InstanceSourceFeature.h"
#include "Temporal/features/TemporalAdjacentWordsFeature.h"
#include "Temporal/features/TemporalTreePathFeature.h"
#include "Temporal/features/InterveningDatesFeature.h"

// Studio is over-paranoid about bounds checking for the boost string 
// classification module, so we wrap its import statement with pragmas
// that tell studio to not display warnings about it.  For more info:
// <http://msdn.microsoft.com/en-us/library/ttcz0bys.aspx>
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4996)
# include "boost/algorithm/string/classification.hpp"
# pragma warning(pop)
#else
# include "boost/algorithm/string/classification.hpp"
#endif

using std::wstring;
using boost::split;
using boost::make_shared;

TemporalFeature_ptr TemporalFeatureFactory::create(const std::wstring& metadata) {
	if (metadata.empty()) {
		throw UnexpectedInputException("TemporalFeatureFactory::TemporalFeatureFactory",
				"Passed empty metadata");
	}

	std::vector<std::wstring> parts;

	split(parts, metadata, boost::is_any_of(L"\t"));

	if (parts.size() < 2) {
		std::wstringstream err;
		err << L"Expected at least two tab delimited strings, but got \""
			<< metadata << "\"";
		throw UnexpectedInputException("TemporalFeatureFactory::TemporalFeatureFactory",
				err);
	}

	wstring typeName = parts[0];
	Symbol relation = Symbol();
	int erase_limit = 2;

	try {
		int hasRel = boost::lexical_cast<int>(parts[1]);

		if (hasRel == 1) {
			if (parts.size() < 3) {
				std::wstringstream err;
				err << L"Expected third argument to be present since second is 1"
					<< L", but got \"" << metadata << L"\"";
				throw UnexpectedInputException("TemporalFeatureFactory::TemporalFeatureFactory",
						err);
			} else {
				relation = parts[2];
				erase_limit = 3;
			}
		}
	} catch (boost::bad_lexical_cast) {
		std::wstringstream err;
		err << L"Expected second field to be 0 or 1 but got " << parts[1];
		throw UnexpectedInputException("TemporalFeatureFactory::TemporalFeatureFactory",
				err);
	}

	parts.erase(parts.begin(), parts.begin() + erase_limit);
	if (L"WordInSentence" == typeName) {
		return TemporalWordInSentenceFeature::create(parts, relation);
	} else if (L"PropSpine" == typeName) {
		return DatePropSpineFeature::create(parts, relation);
	} else if (L"EventTime" == typeName) {
		return TemporalEventFeature::create(parts, relation);
	} else if (L"RelationTime" == typeName) {
		return TemporalRelationFeature::create(parts, relation);
	} else if (L"ParticularDayFeature" == typeName) {
		return make_shared<ParticularDayFeature>(relation);
	} else if (L"ParticularMonthFeature" == typeName) {
		return make_shared<ParticularMonthFeature>(relation);
	} else if (L"ParticularYearFeature" == typeName) {
		return make_shared<ParticularYearFeature>(relation);
	} else if (L"Bias" == typeName) {
		return TemporalBiasFeature::create(parts, relation);
	} /*else if (L"Source" == typeName) {
		return RelationSourceFeature::create(parts, relation);
	} */ else if (L"InstanceSource" == typeName) {
		return InstanceSourceFeature::create(parts, relation);
	} else if (L"Att" == typeName) {
		return PropAttWord::create(parts, relation);
	} else if (L"Adjacent" == typeName) {
		return TemporalAdjacentWordsFeature::create(parts, relation);
	} else if (L"TreePath" == typeName) {
		return TemporalTreePathFeature::create(parts, relation);
	} else if (L"InterveningDates" == typeName) {
		return InterveningDatesFeature::create(parts, relation);
	} else {
		std::wstringstream err;
		err << L"Unknown temporal feature type: " << typeName;
		throw UnexpectedInputException("TemporalFeatureFactory::create", err);
	}
}

