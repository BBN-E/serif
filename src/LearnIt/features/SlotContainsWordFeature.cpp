// disable bogus Microsoft warnings that (gasp!) Boost is using the 
// standard library instead of Microsoft proprietary extensions
#pragma warning(disable:4996)
#include <vector>
#include <string>
#include <sstream>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "Generic/common/leak_detection.h"
#include "Generic/theories/DocTheory.h"
#include "LearnIt/Target.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/SlotFiller.h"
#include "LearnIt/features/SlotContainsWordFeature.h"

using std::wstring;

SlotContainsWordFeature::SlotContainsWordFeature(Target_ptr target,
	const wstring& name, const wstring& metadata) : Feature() 
{ 
	std::vector<wstring> metadata_parts;
	boost::split(metadata_parts, metadata, boost::is_any_of(L"\t"));
	if (metadata_parts.size() == 2) {
		_slot_num = boost::lexical_cast<int>(metadata_parts[0]);
		_word = metadata_parts[1];
	} else {
		std::stringstream errMsg; 
		errMsg << "Invalid metadata for SlotContainsWordFeature: '" << 
			metadata << "'" << std::endl;
		throw UnexpectedInputException("SlotContainsWordFeature::SlotContainsWordFeature",
			errMsg.str().c_str());
	}
}

bool SlotContainsWordFeature::matchesMatch(const SlotFillerMap& match,
	const SlotFillersVector& slotFillersVector,
	const DocTheory* doc, int sent_no) const 
{
	SlotFillerMap::const_iterator probe = match.find(_slot_num);

	if (probe != match.end()) {
		const wstring bn = probe->second->getBestName();
		// first check string is found in the slot...
		size_t pos;
		if ((pos = bn.find(_word)) !=wstring::npos) {
			// then make sure it isn't inside another word by ensuring
			// each side of it either touches the edge of the string or is
			// bordered by a space
			size_t end_pos = pos + _word.length() -1;
			if ((pos == 0 || bn[pos-1] == ' ') &&
				(end_pos == bn.length()-1 || bn[end_pos+1] == ' '))
			{
				return true;
			}
		}
	}
	return false;
}
