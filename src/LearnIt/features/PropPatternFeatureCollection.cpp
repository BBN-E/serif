#pragma warning( disable: 4996 )
#include "Generic/common/leak_detection.h"
#include <map>
#include <utility>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "Generic/common/Symbol.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "ActiveLearning/alphabet/FeatureAlphabet.h"
#include "LearnIt/features/PropPatternFeatureCollection.h"
#include "LearnIt/features/BrandyPatternFeature.h"

using std::wstring; using std::vector; using std::make_pair;
using boost::make_shared;
using boost::split; using boost::is_any_of;
using boost::match_results; using boost::regex_search; using boost::wregex;


PropPatternFeatureCollection::PropPatternFeatureCollection(
	Target_ptr target, const std::vector<BrandyPatternFeature_ptr>& features) :
_features(features.begin(), features.end())
{
	static wregex predicates_pattern(L"\\(predicate ([^)]*)\\)");
	wstring::const_iterator start, end;
	match_results<wstring::const_iterator> what;
	vector<wstring> predicates;

	int feature_idx = 0;
	BOOST_FOREACH(BrandyPatternFeature_ptr feature, features) {
		predicates.clear();
		start = feature->patternString().begin();
		end = feature->patternString().end();

		while(regex_search(start, end, what, predicates_pattern)) {
            std::wstring wstr(what[1].first, what[1].second);
			split(predicates, wstr, is_any_of(L" "));
			start = what[0].second;
		}
		if (predicates.empty()) {
			std::wstringstream err;
			err << L"Could not extract predicates from PropPatternFeature: "
				<< feature->patternString();
			throw UnexpectedInputException(
					"PropPatternFeatureCollection::PropPatternFeatureCollection",
					err);
		}
		BOOST_FOREACH(const wstring& predicate, predicates) {
				_predicatesToPatterns.insert(make_pair(Symbol(predicate), feature_idx));
			}
		++feature_idx;
	}
}

PropPatternFeatureCollection_ptr 
PropPatternFeatureCollection::create(Target_ptr target, FeatureAlphabet_ptr alphabet,
									  double threshold, bool include_negatives) 
{
	FeatureList_ptr tmpFeatures = Feature::featuresOfClass(alphabet, 
			L"PropPatternFeature", target, threshold, include_negatives);
	std::vector<BrandyPatternFeature_ptr> features;
	BOOST_FOREACH(const Feature_ptr& f, *tmpFeatures) {
		BrandyPatternFeature_ptr bpfp = boost::dynamic_pointer_cast<BrandyPatternFeature>(f);
		if (bpfp) {
			features.push_back(bpfp);
		} else {
			throw UnrecoverableException("PropPatternFeatureCollection",
				"Internal inconsistency: features do not result in objects "
				"of the right type");
		}
	}
	return make_shared<PropPatternFeatureCollection>(target, features);
}

FeatureCollectionIterator<BrandyPatternFeature>
PropPatternFeatureCollection::applicableFeatures(const SlotFillersVector& slotFillersVector,
	   const DocTheory* dt, int sent_no)
{
	_applicableFeatures.clear();
	PropositionSet* propSet = dt->getSentenceTheory(sent_no)->getPropositionSet();

	for (int i = 0; i<propSet->getNPropositions(); ++i) {
		SymbolCanopy::Features possible_patterns = 
			_predicatesToPatterns.equal_range(propSet->getProposition(i)->getPredSymbol());
		for (SymbolCanopy::KeyToFeatures::const_iterator it = possible_patterns.first;
			it!=possible_patterns.second; ++it) 
		{
			_applicableFeatures.insert(it->second);
		}
	}		
		
	return FeatureCollectionIterator<BrandyPatternFeature>
		(_features, _applicableFeatures, true);
}
