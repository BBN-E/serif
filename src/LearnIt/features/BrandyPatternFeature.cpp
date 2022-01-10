#include "Generic/common/leak_detection.h"

#include <boost/foreach.hpp>
#include "Generic/theories/DocTheory.h"
#include "LearnIt/Target.h"
#include "LearnIt/LearnItPattern.h"
#include "LearnIt/MatchInfo.h"
#include "LearnIt/SlotFiller.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/features/BrandyPatternFeature.h"

BrandyPatternFeature::BrandyPatternFeature(Target_ptr target,
									   const std::wstring& name, 
									   const std::wstring& metadata,
									   bool multi) 
	: Feature(), 
	_pattern(LearnItPattern::create(target, name, metadata, true, false, 0.0, 0.0, multi))
{}

bool BrandyPatternFeature::matchesSentence(const SlotFillersVector& slotFillersVector,
		const AlignedDocSet_ptr doc_set, Symbol docid, int sent_no, std::vector<SlotFillerMap>& matches) const
{
	bool foundAnything = false;
	MatchInfo::PatternMatches results = MatchInfo::findPatternInSentence(doc_set,
		doc_set->getDefaultDocTheory()->getSentenceTheory(sent_no), _pattern);

	BOOST_FOREACH(const MatchInfo::PatternMatch& match, results) {
		const std::vector<SlotFiller_ptr> slots = match.slots;
		SlotFillerMap slotFillerMap;

		for (size_t i=0; i<slots.size(); ++i) {
			SlotFiller_ptr slotMatch = slots[i];
			if (slotMatch->unmetConstraints()) {
				return false;
			} else {
				SlotFiller_ptr ret = SlotFiller_ptr();

				if (slotMatch->unfilledOptional()) {
					ret = slotMatch;
				} else {
					for (size_t j=0; j<slotFillersVector[i]->size(); ++j) {
						if ((*slotFillersVector[i])[j]->sameReferent(*slotMatch)) {
							ret = (*slotFillersVector[i])[j];
							//break;
						}
					}
				}

				if (ret) {
					slotFillerMap.insert(std::make_pair(static_cast<int>(i), ret));
					foundAnything = true;
				} else {
					throw UnexpectedInputException("BrandyPatternFeature::matchesSentence", 
						"Unable to find SlotFiller for matched non-optional slot");
				}
			}
		}

		matches.push_back(slotFillerMap);
	}

	return foundAnything;
}

const std::wstring& BrandyPatternFeature::patternString() const {
	return _pattern->getPatternString();
}
