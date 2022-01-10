#include "Generic/common/leak_detection.h"
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include "Generic/theories/DocTheory.h"
#include "ActiveLearning/alphabet/FeatureAlphabet.h"
#include "LearnIt/features/slot_identity/SlotsIdentityFeatureCollection.h"
#include "LearnIt/features/slot_identity/SlotsIdentityFeature.h"
#include "LearnIt/features/slot_identity/SlotsCanopy.h"

using boost::make_shared;

SlotsIdentityFeatureCollection::SlotsIdentityFeatureCollection(
	Target_ptr target, const std::vector<SlotsIdentityFeature_ptr>& features) :
_features(features.begin(), features.end()), _canopy(target, features)
{
}

SlotsIdentityFeatureCollection_ptr 
SlotsIdentityFeatureCollection::create(Target_ptr target, FeatureAlphabet_ptr alphabet,
									  double threshold, bool include_negatives) 
{
	FeatureList_ptr tmpFeatures = Feature::featuresOfClass(alphabet, 
			L"SlotsIdentityFeature", target, threshold, include_negatives);
	std::vector<SlotsIdentityFeature_ptr> features;
	BOOST_FOREACH(const Feature_ptr& f, *tmpFeatures) {
		SlotsIdentityFeature_ptr sifp = boost::dynamic_pointer_cast<SlotsIdentityFeature>(f);
		if (sifp) {
			features.push_back(sifp);
		} else {
			throw UnrecoverableException("SlotsIdentityFeatureCollection",
				"Internal inconsistency: features do not result in objects "
				"of the right type");
		}
	}
	return make_shared<SlotsIdentityFeatureCollection>(target, features);
}

FeatureCollectionIterator<SlotsIdentityFeature>
SlotsIdentityFeatureCollection::applicableFeatures(const SlotFillersVector& slotFillersVector,
	   const DocTheory* dt, int sent_no)
{
	_applicableFeatures.clear();
	if (_canopy.unconstraining()) {
		return FeatureCollectionIterator<SlotsIdentityFeature>
			(_features, _applicableFeatures, false);
	} else {
		_canopy.potentialMatches(slotFillersVector, dt, _applicableFeatures);
		return FeatureCollectionIterator<SlotsIdentityFeature>
			(_features, _applicableFeatures, true);
	}
}
