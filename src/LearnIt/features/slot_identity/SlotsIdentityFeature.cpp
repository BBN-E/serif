// disable bogus Microsoft warnings that (gasp!) Boost is using the 
// standard library instead of Microsoft proprietary extensions
#pragma warning(disable:4996)
#include "Generic/common/leak_detection.h"
#include <iostream>
#include <string>
#include <vector>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "Generic/theories/DocTheory.h"
#include "LearnIt/Seed.h"
#include "LearnIt/Target.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/SlotFiller.h"
#include "LearnIt/features/slot_identity/SlotsIdentityFeature.h"

SlotsIdentityFeature::SlotsIdentityFeature(Target_ptr target,
										   const std::wstring& name,
										   const std::wstring& metadata)
										   : Feature(),
				_seed(_new Seed(target, metadataToSlots(metadata),true, L""))
{ }

std::vector<std::wstring> SlotsIdentityFeature::metadataToSlots(const std::wstring& metadata) {
	std::vector<std::wstring> ret;
	boost::split(ret, metadata, boost::is_any_of(L"\t"));
	return ret;
}

bool SlotsIdentityFeature::matchesSentence(const SlotFillersVector& slotFillersVector,
		const AlignedDocSet_ptr doc_set, Symbol docid, int sent_no, std::vector<SlotFillerMap>& matches) const
{
	return _seed->findInSentence(slotFillersVector, doc_set->getDefaultDocTheory(), docid, sent_no,
		matches);
}

const std::wstring& SlotsIdentityFeature::slotName(int slot_num) const {
	return _seed->getSlot(slot_num)->name();
}
