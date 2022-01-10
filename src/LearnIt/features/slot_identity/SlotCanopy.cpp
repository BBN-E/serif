#include "Generic/common/leak_detection.h"
#include <utility>
#include <algorithm>
#include <boost/foreach.hpp>
#include "Generic/common/SessionLogger.h"
#include "LearnIt/SlotConstraints.h"
#include "LearnIt/SlotFiller.h"
#include "LearnIt/features/slot_identity/SlotsIdentityFeature.h"
#include "LearnIt/features/slot_identity/SlotCanopy.h"

using std::make_pair;
using std::wstring;

SlotCanopy::SlotCanopy(const SlotConstraints& constraints, int slot, 
		   const std::vector<SlotsIdentityFeature_ptr>& features)
{
	setUnconstraining(constraints, slot);

	// add best names to the canopy
	int idx = 0;
	BOOST_FOREACH(const SlotsIdentityFeature_ptr& feature, features) {
		_name_to_feature.insert(make_pair(feature->slotName(slot), idx));
		++idx;
	}
}

bool SlotCanopy::unconstraining() const {
	return _unconstraining;	
}

void SlotCanopy::setUnconstraining(const SlotConstraints& constraints, int slot) {
	_unconstraining = false;
	// if "partial_match_allowed" in SlotFiller::slotMatchScore is true, 
	// then we don't canopy right now
	if (constraints.getMentionType() == SlotConstraints::MENTION &&
		constraints.getBrandyConstraints().empty()) 
	{
		_unconstraining = true;
		SessionLogger::info("SlotCanopy") << "SlotCanopy for slot " << slot << 
			"set to unconstraining because partial matches are allowed";
	} else if (constraints.getMentionType() == SlotConstraints::VALUE
		&& constraints.getSeedType() == L"yymmdd") 
	{
		_unconstraining = true;
		SessionLogger::info("SlotCanopy") << "SlotCanopy for slot " << slot << 
			"set to unconstraining  because dates are involved";
	}
}

// guarantees output will be sorted
void SlotCanopy::potentialMatches(const SlotFillers& fillers, const DocTheory* docTheory, std::set<int>& output)
{
	// TODO: check normalization
	output.clear();
	BOOST_FOREACH(const SlotFiller_ptr& sfp, fillers) {
		// for each slot filler, we also need to check for other strings
		// in the same entity which would be matched by
		// SlotFiller::matchSeedStringToEntity
		SlotFiller::EntityStringsIterator probe = sfp->getEntityStrings(docTheory);
		if (probe != SlotFiller::noEntityStrings()) {
			BOOST_FOREACH(const wstring& str, probe->second) {
				StringCanopy::Features entity_matches = _name_to_feature.equal_range(str);
				for (StringCanopy::KeyToFeatures::const_iterator it=entity_matches.first;
					it!=entity_matches.second; ++it) 
				{
					output.insert(it->second);
				}
			}
			// most of the time the best name will be in the entity string set
			// and will therefore already be taken care of. If not, we need
			// to check explicitly.  We also need to then sort output because
			// this new addition may be out of order
			if (probe->second.find(sfp->name()) == probe->second.end()) {
				StringCanopy::Features best_name_matches = _name_to_feature.equal_range(sfp->name());
				for (StringCanopy::KeyToFeatures::const_iterator it=best_name_matches.first;
					it != best_name_matches.second; ++it) 
				{
						output.insert(it->second);
				}
			}
		} else {
			// similarly, if for some reason there are no entity strings,
			// we need to check the best name explicitly
			StringCanopy::Features best_name_matches = _name_to_feature.equal_range(sfp->name());
			for (StringCanopy::KeyToFeatures::const_iterator it=best_name_matches.first;
				it != best_name_matches.second; ++it) 
			{
					output.insert(it->second);
			}	
		}
	}	
}
