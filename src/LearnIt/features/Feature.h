#ifndef _LEARNIT_FEATURE_H_
#define _LEARNIT_FEATURE_H_

#include <map>
#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "Generic/patterns/multilingual/AlignedDocSet.h"
#include "../SlotFillerTypes.h"
#include "../MentionToEntityMap.h"

BSP_DECLARE(Feature);
BSP_DECLARE(Target)
BSP_DECLARE(SentenceMatchableFeature);
BSP_DECLARE(MatchMatchableFeature);
BSP_DECLARE(FeatureAlphabet)
typedef std::vector<Feature_ptr> FeatureList;
typedef boost::shared_ptr<FeatureList> FeatureList_ptr;

class Feature {
public:
	bool trigger() const;
	double weight() const;
	bool annotated() const;
	double expectation() const;
	int index() const;
	
	static Feature_ptr createFromTableRow(Target_ptr target, 
		const std::vector<std::wstring>& row);
	virtual ~Feature() {};

	static std::vector<SentenceMatchableFeature_ptr> getTriggerFeatures(
			FeatureAlphabet_ptr alphabet, Target_ptr target,
			double threshold, bool include_negatives);
	static std::vector<MatchMatchableFeature_ptr> getNonTriggerFeatures(
			FeatureAlphabet_ptr alphabet, Target_ptr target, double threshold, 
			bool include_negatives);
	static FeatureList_ptr getFeatures(FeatureAlphabet_ptr alphabet,
			Target_ptr target,	double threshold, bool include_negatives, 
			const std::wstring& constraints = L"");
	static FeatureList_ptr featuresOfClass(FeatureAlphabet_ptr alphabet,
			const std::wstring& feature_class_name, Target_ptr target, 
			double threshold, bool include_negatives);
protected:
	Feature(bool trigger = false);
private:
	int _index;
	bool _trigger;
	double _weight;
	double _expectation;
	bool _annotated;
	
	static bool isTriggeringFeatureClass(const std::wstring& featureClass);
	static bool _loadedTriggeringClasses;
	static std::set<std::wstring> _triggeringFeatures;
	static std::set<std::wstring> _nonTriggeringFeatures;
};

class SentenceMatchableFeature : virtual public Feature {
protected:
	SentenceMatchableFeature();
public:
	virtual bool matchesSentence(const SlotFillersVector& slotFillersVector,
		const AlignedDocSet_ptr doc_set, Symbol docid, int sent_no, std::vector<SlotFillerMap>& matches) const = 0;
};

class MatchMatchableFeature : virtual public Feature {
public:
	virtual bool matchesMatch(const SlotFillerMap& match,
		const SlotFillersVector& slotFillersVector,
		const DocTheory* doc, int sent_no) const = 0;
protected:
	MatchMatchableFeature() ;
};
#endif
