// disable bogus Microsoft warnings that (gasp!) Boost is using the 
// standard library instead of Microsoft proprietary extensions
#pragma warning(disable:4996)
#include "Generic/common/leak_detection.h"

#include <string>
#include <vector>
#include <sstream>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/make_shared.hpp>
#include "Generic/theories/DocTheory.h"
#include "LearnIt/Target.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/SlotFiller.h"
#include "LearnIt/features/slot_identity/SlotIdentityFeature.h"

using boost::make_shared;

SlotIdentityFeature::SlotIdentityFeature(Target_ptr target,
										   const std::wstring& name,
										   const std::wstring& metadata) :
	Feature() 
{ 
	std::vector<std::wstring> metadata_parts;
	boost::split(metadata_parts, metadata, boost::is_any_of(L"\t"));
	if (metadata_parts.size() == 2) {
		_slot_num = boost::lexical_cast<int>(metadata_parts[0]);
		_slot_constraints = target->getSlotConstraints(_slot_num);
		_slot_val = make_shared<AbstractLearnItSlot>(_slot_constraints, metadata_parts[1]);
	} else {
		std::stringstream errMsg; 
		errMsg << "Invalid metadata for SlotIdentityFeature: '" << 
			metadata << "'" << std::endl;
		throw UnexpectedInputException("SlotIdentityFeature::SlotIdentityFeature",
			errMsg.str().c_str());
	}
}

bool SlotIdentityFeature::matchesMatch(const SlotFillerMap& match,
	const SlotFillersVector& slotFillersVector,
	const DocTheory* doc, int sent_no) const 
{
	SlotFillerMap::const_iterator probe = match.find(_slot_num);

	if (probe != match.end()) {
		if (SlotFiller::isGoodSlotMatch(probe->second, *_slot_val, _slot_constraints))
		{
			return true;
		}
	}
	return false;
}

const std::wstring& SlotIdentityFeature::slotName() const {
	return _slot_val->name();
}
