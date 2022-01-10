#include "Generic/common/leak_detection.h"

#include <sstream>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "ActiveLearning/alphabet/FeatureAlphabet.h"
#include "LearnIt/Target.h"
#include "LearnIt/features/Feature.h"
#include "LearnIt/features/BrandyPatternFeature.h"
#include "LearnIt/features/slot_identity/SlotsIdentityFeature.h"
#include "LearnIt/features/slot_identity/SlotIdentityFeature.h"
#include "LearnIt/features/SlotContainsWordFeature.h"
#include "LearnIt/features/YYMMDDFeature.h"
#include "LearnIt/features/TopicFeature.h"
#include "LearnIt/features/SentenceBiasFeature.h"
#include "LearnIt/features/SlotBiasFeature.h"

using std::wstring; using std::vector;
using boost::make_shared;
using boost::dynamic_pointer_cast;

bool Feature::_loadedTriggeringClasses = false;
std::set<std::wstring> Feature::_triggeringFeatures;
std::set<std::wstring> Feature::_nonTriggeringFeatures;

Feature::Feature(bool trigger) :
_trigger(trigger), _weight(0.0), _expectation(0.0), _annotated(false) {}

Feature_ptr Feature::createFromTableRow(Target_ptr target,
										const vector<wstring>& row) {
	// columns passed in are are name, metadata, weight
	vector<wstring>::const_iterator iter = row.begin();
	unsigned int index = boost::lexical_cast<unsigned int>(*iter++);
	wstring name = *iter++;
	wstring metadata = *iter++;
	double weight = boost::lexical_cast<double>(*iter++);

	size_t parenIdx = name.find(L'(');

	if (parenIdx != wstring::npos) {
		wstring featureClass = name.substr(0, parenIdx);
		Feature_ptr feature;

		if (L"PropPatternFeature" == featureClass ||
			L"TextPatternFeature" == featureClass ||
			L"EventFeature" == featureClass ||
			L"RelationFeature" == featureClass ||
			L"KeywordPatternFeature" == featureClass) 
		{
			feature = make_shared<BrandyPatternFeature>(target, name, metadata, true);
		} else if (L"KeywordInSentenceFeature" == featureClass) { 
			feature = make_shared<BrandyPatternFeature>(target, name, metadata, false);
		} else if (L"SlotsIdentityFeature" == featureClass) {
			feature = make_shared<SlotsIdentityFeature>(target, name, metadata);
		} else if (L"YYMMDDFeature" == featureClass) {
			feature = make_shared<YYMMDDFeature>(target, name, metadata);
		} else if (L"NotYYMMDDFeature" == featureClass) {
			feature = make_shared<NotYYMMDDFeature>(target, name, metadata);
		} else if (L"SlotIdentityFeature" == featureClass) {
			feature = make_shared<SlotIdentityFeature>(target, name, metadata);
		} else if (L"SlotContainsWordFeature" == featureClass) {
			feature = make_shared<SlotContainsWordFeature>(target, name, metadata);
		} else if (L"TopicFeature" == featureClass) {
			feature = make_shared<TopicFeature>(target, name, metadata);
		} else if (L"SentenceBiasFeature" == featureClass) {
			feature = make_shared<SentenceBiasFeature>();
		} else if (L"SlotBiasFeature" == featureClass) {
			feature = make_shared<SlotBiasFeature>();
		} else {
			std::wstringstream err;
			err << "Don't know how to create feature of type " << featureClass;
			throw UnexpectedInputException("Feature::createFromTableRow", err);
		}

		feature->_trigger = isTriggeringFeatureClass(featureClass);
		feature->_weight = weight;
		feature->_index = index;
		return feature;
	} else {
		throw UnexpectedInputException("Feature::createFromTableRow", 
			"Can't interpret name as feature");
	}
}

double Feature::weight() const { return _weight; }
double Feature::expectation() const {return _expectation; }
bool Feature::annotated() const {return _annotated; }
bool Feature::trigger() const { return _trigger; }
int Feature::index() const { return _index; }

bool Feature::isTriggeringFeatureClass(const wstring& featureClass) {
	if (!_loadedTriggeringClasses) {
		vector<wstring> triggeringFeatures =
			ParamReader::getWStringVectorParam("triggering_features");
		vector<wstring> nonTriggeringFeatures =
			ParamReader::getWStringVectorParam("non_triggering_features");

		BOOST_FOREACH(const wstring& featureClass, triggeringFeatures) {
			if (featureClass.length()>0) {
				_triggeringFeatures.insert(featureClass);
			}
		}
		
		BOOST_FOREACH(const wstring& featureClass, nonTriggeringFeatures) {
			if (featureClass.length()>0) {
				_nonTriggeringFeatures.insert(featureClass);
			}
		}
		_loadedTriggeringClasses = true;
	}

	if (_triggeringFeatures.find(featureClass) != _triggeringFeatures.end()) {
		return true;
	}
	if (_nonTriggeringFeatures.find(featureClass) != _nonTriggeringFeatures.end()) {
		return false;
	}

	std::wstringstream err;
	err << "Feature class '" << featureClass << "' appears on neither "
		<< "the triggering or non-triggering list.";
	throw UnexpectedInputException("Feature::isTriggeringFeatureClass",err);
}

// the calls to the parent constructor in the below are never actually done
// because of virtual inheritance
SentenceMatchableFeature::SentenceMatchableFeature() : Feature() {}
MatchMatchableFeature::MatchMatchableFeature() : Feature() {}

bool isTrigger(Feature_ptr f) {
	return f->trigger();
}

bool isNotTrigger(Feature_ptr f) {
	return !f->trigger();
}

FeatureList_ptr Feature::getFeatures(FeatureAlphabet_ptr alphabet, 
		Target_ptr target, double threshold, bool include_negatives,
		const wstring& constraints)
{
	FeatureList_ptr ret = make_shared<FeatureList>();

	DatabaseConnection::Table_ptr featureRows =
		alphabet->getFeatureRows(threshold, include_negatives, constraints);
	BOOST_FOREACH(const DatabaseConnection::TableRow& row, *featureRows) {
		ret->push_back(createFromTableRow(target, row));
	}

	return ret;
}

std::vector<SentenceMatchableFeature_ptr> Feature::getTriggerFeatures(
	FeatureAlphabet_ptr alphabet, Target_ptr target, double threshold, 
	bool include_negatives)
{
	FeatureList_ptr features = getFeatures(alphabet, target, threshold,
			include_negatives);

	features->erase(remove_if(features->begin(), features->end(), isNotTrigger),
		features->end());
	std::vector<SentenceMatchableFeature_ptr> ret;
	BOOST_FOREACH(Feature_ptr f, *features) {
		if (SentenceMatchableFeature_ptr smf = 
			dynamic_pointer_cast<SentenceMatchableFeature>(f)) 
		{
			ret.push_back(smf);
		} else {
			throw UnexpectedInputException("FeatureAlphabet::getTriggerFeatures",
				"Trigger feature does not implement SentenceMatchable interface");
		}
	}
	return ret;
}

std::vector<MatchMatchableFeature_ptr> Feature::getNonTriggerFeatures(
	FeatureAlphabet_ptr alphabet, Target_ptr target, double threshold, 
	bool include_negatives) 
{
	FeatureList_ptr features = getFeatures(alphabet, target, threshold,
			include_negatives);

	features->erase(remove_if(features->begin(), features->end(), isTrigger), 
		features->end());
	std::vector<MatchMatchableFeature_ptr> ret;
	BOOST_FOREACH(Feature_ptr f, *features) {
		if (MatchMatchableFeature_ptr mmf = dynamic_pointer_cast<MatchMatchableFeature>(f)) {
			ret.push_back(mmf);
		} else {
			throw UnexpectedInputException("FeatureAlphabet::getNonTriggerFeatures",
				"Non-trigger feature does not implement MatchMatchable interface");
		}
	}
	return ret;
}

FeatureList_ptr Feature::featuresOfClass(FeatureAlphabet_ptr alphabet,
	const std::wstring& cls, Target_ptr target, double threshold, 
	bool include_negatives) 
{
	std::wstringstream constraints;
	constraints << "name like \"" << cls << "%\"";
	return getFeatures(alphabet, target, threshold, 
			include_negatives, constraints.str());
}
