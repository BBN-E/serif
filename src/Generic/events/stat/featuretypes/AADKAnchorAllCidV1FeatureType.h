#ifndef AA_DK_ANCHORALLCID_V1_FEATURE_TYPE_H
#define AA_DK_ANCHORALLCID_V1_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTBigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <set>

class AADKAnchorAllCidV1FeatureType : public EventAAFeatureType {
private:
	typedef std::pair<Symbol, int> SymIntPair;

public:
	AADKAnchorAllCidV1FeatureType() : EventAAFeatureType(Symbol(L"aa-anchorallcid-v1")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramIntFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(state.getObservation(0));


		Symbol subSym = Symbol(L"sub");
		Symbol objSym = Symbol(L"obj");
		unsigned featureIndex = 0;

		set<int> anchorAllSubCids = o->getAADistributionalKnowledge().anchorAllSubCids();
		for(set<int>::iterator it=anchorAllSubCids.begin(); it!=anchorAllSubCids.end(); it++) {
			resultArray[featureIndex++] = _new DTBigramIntFeature(this, state.getTag(), subSym, (*it));
		}

		set<int> anchorAllObjCids = o->getAADistributionalKnowledge().anchorAllObjCids();
		for(set<int>::iterator it=anchorAllObjCids.begin(); it!=anchorAllObjCids.end(); it++) {
			resultArray[featureIndex++] = _new DTBigramIntFeature(this, state.getTag(), objSym, (*it));
		}

		return featureIndex;
	}
};

#endif

