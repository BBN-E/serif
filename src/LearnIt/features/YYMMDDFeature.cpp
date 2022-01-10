/*// disable bogus Microsoft warnings that (gasp!) Boost is using the 
// standard library instead of Microsoft proprietary extensions
#pragma warning(disable:4996)*/
#include <vector>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include "Generic/common/leak_detection.h"
#include "Generic/theories/DocTheory.h"
#include "LearnIt/Target.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/SlotFiller.h"
#include "LearnIt/features/YYMMDDFeature.h"

using std::wstring;
using std::isdigit;
using boost::lexical_cast;
using boost::bad_lexical_cast;

YYMMDDFeatureBase::YYMMDDFeatureBase(Target_ptr target,
	const wstring& name, const wstring& metadata, bool pos_direction) : 
Feature(), _pos_direction(pos_direction)
{ 
	try {
		_slot_num = lexical_cast<int>(metadata);
		const wstring brandy_constraints = 
			target->getSlotConstraints(_slot_num)->getBrandyConstraints();
		if (brandy_constraints.find(L"TIME") == wstring::npos &&
			brandy_constraints.find(L"DATE") == wstring::npos)
		{
			throw UnexpectedInputException("YYMMDDFeatureBase::YYMMDDFeatureBase",
				"YYMMDD-type features can only be applied to slots of type ",
				"TIME and/or DATE");
		}
	} catch (bad_lexical_cast) {
		std::stringstream errMsg; 
		errMsg << "Invalid metadata for YYMMDDFeatureBase: '" << 
			metadata << "' failed lexical cast to int" << std::endl;
		throw UnexpectedInputException("YYMMDDFeatureBase::YYMMDDFeatureBase",
			errMsg.str().c_str());
	}
}

bool YYMMDDFeatureBase::matchesMatch(const SlotFillerMap& match,
	const SlotFillersVector& slotFillersVector,
	const DocTheory* dt, int sent_no) const 
{
	SlotFillerMap::const_iterator probe = match.find(_slot_num);
	bool ret = true;

	if (probe != match.end()) {
		const wstring bn = probe->second->getBestName();
		if (bn.empty()) {
			return false;
		}
		// first check string is found in the slot...
		for (size_t i=0; i<bn.length(); ++i) {
			if (!isdigit(bn[i])) {
				ret = false;
				break;
			}
		}
		if (_pos_direction) {
			return ret;
		} else {
			return !ret;
		}
	}
	return false;
}

YYMMDDFeature::YYMMDDFeature(Target_ptr target, const wstring& name, 
							 const wstring& metadata)
	: Feature(), YYMMDDFeatureBase(target, name, metadata, true) {}

NotYYMMDDFeature::NotYYMMDDFeature(Target_ptr target, const wstring& name, 
							 const wstring& metadata)
							 : Feature(), YYMMDDFeatureBase(target, name, metadata, false) {}
