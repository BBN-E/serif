#pragma once

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include "Generic/patterns/multilingual/LanguageVariant.h"
#include "Generic/patterns/multilingual/AlignedDocSet.h"
#include "Generic/common/bsp_declare.h"
#include "LearnIt/MatchInfo.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/SlotFillerTypes.h"

class SentenceTheory;
class DocTheory;
BSP_DECLARE(LearnIt2Matcher);
BSP_DECLARE(Target)
BSP_DECLARE(LearnItDB)
BSP_DECLARE(Feature)
BSP_DECLARE(SentenceMatchableFeature)
BSP_DECLARE(MatchMatchableFeature)
BSP_DECLARE(PropPatternFeatureCollection)
BSP_DECLARE(TextPatternFeatureCollection)
BSP_DECLARE(BrandyPatternFeature)
BSP_DECLARE(FromDBFeatureAlphabet)
BSP_DECLARE(FeatureAlphabet)

class LearnIt2Matcher {
public:
	LearnIt2Matcher(LearnItDB_ptr db);
	MatchInfo::PatternMatches match(const DocTheory* dt, Symbol docid, 
		SentenceTheory* sent_theory);
	MatchInfo::PatternMatches match(const AlignedDocSet_ptr doc_set, Symbol docid,
		SentenceTheory* sent_theory);

	const Target_ptr target() const;
	const std::wstring& name() const;
private:
	Target_ptr _target;
	std::wstring _name;
	std::vector<MatchMatchableFeature_ptr> _nonTriggerFeatures;
	double _prob_threshold;
	bool _record_source;
	FeatureAlphabet_ptr _alphabet;

	PropPatternFeatureCollection_ptr _propFeatures;
	TextPatternFeatureCollection_ptr _textFeatures;
	TextPatternFeatureCollection_ptr _keywordInSentenceFeatures;
	std::vector<BrandyPatternFeature_ptr> _eventFeatures;
	std::vector<BrandyPatternFeature_ptr> _relationFeatures;

	class MatchRecord {
	public:
		MatchRecord(bool track_features);
		MatchRecord(Feature_ptr feature, FeatureAlphabet_ptr alphabet);
		void matchedByFeature(Feature_ptr feature, FeatureAlphabet_ptr alphabet);
		MatchProvenance_ptr createMatchProvenance() const;
		std::wstring sourceString() const;

		bool triggered;
		double score;
		typedef boost::tuple<unsigned int, std::wstring, double> FeatureScore;
		typedef std::vector<FeatureScore> FeatureScoreList;
		typedef boost::shared_ptr<FeatureScoreList> FeatureScoreList_ptr;
		FeatureScoreList_ptr matchedFeatures;
	};
	typedef std::map<SlotFillerMap, MatchRecord> ScoredMatches;
	typedef std::map<SlotFiller_ptr, SlotFiller_ptr> SlotFillerNormalizer;

	void scoreMatchForNonTriggerFeatures(
		const SlotFillerMap& match, MatchRecord& record, 
		const SlotFillersVector& slotFillersVector, const AlignedDocSet_ptr doc_set, 
		int sent_no);
	void processFeatureMatch(Feature_ptr feature, SlotFillerMap& featureMatch,
		const SlotFillerNormalizer& slotFillerNormalizer, ScoredMatches& returnMatches);
	SlotFillerNormalizer buildSlotFillerNormalizer(const SlotFillersVector& slotFillers);
	void normalizeMatch(SlotFillerMap& match, const SlotFillerNormalizer& normalizer);

	void initEventAndRelationFeatures(FromDBFeatureAlphabet_ptr alphabet, 
			double threshold);

	void removeDuplicateMatches(std::vector<SlotFillerMap>& matches);
};

