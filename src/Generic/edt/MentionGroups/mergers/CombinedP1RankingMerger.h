// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef COMBINED_P1_RANKING_MERGER_H
#define COMBINED_P1_RANKING_MERGER_H

#include "Generic/edt/MentionGroups/MentionGroupMerger.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/discmodel/DocumentMentionInformationMapper.h"
#include "Generic/discTagger/DTFeature.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/P1Decoder.h"
#include "Generic/theories/Mention.h"
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>

class LinkInfoCache;
class MentionGroup;
class EntitySet;

/**
  *  Merges MentionGroups on the basis of decisions made by a set
  *  of P1 Ranking models, one each for names, descriptors and 
  *  pronouns.
  *
  */
class CombinedP1RankingMerger : public MentionGroupMerger {
public:

	CombinedP1RankingMerger(MentionGroupConstraint_ptr constraints);

	void merge(MentionGroupList& groups, LinkInfoCache& cache);

protected:
	bool shouldMerge(const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const;

private:
	boost::scoped_array<DTFeatureTypeSet*> _nameFeatureTypesArray;
	boost::scoped_ptr<DTFeatureTypeSet> _nameLinkFeatureTypes;
	boost::scoped_ptr<DTFeatureTypeSet> _nameNoneFeatureTypes;

	boost::scoped_array<DTFeatureTypeSet*> _descFeatureTypesArray;
	boost::scoped_ptr<DTFeatureTypeSet> _descLinkFeatureTypes;
	boost::scoped_ptr<DTFeatureTypeSet> _descNoneFeatureTypes;

	boost::scoped_array<DTFeatureTypeSet*> _pronFeatureTypesArray;
	boost::scoped_ptr<DTFeatureTypeSet> _pronLinkFeatureTypes;
	boost::scoped_ptr<DTFeatureTypeSet> _pronNoneFeatureTypes;

	boost::scoped_ptr<DTTagSet> _tagSet;
	
	boost::scoped_ptr<DTFeature::FeatureWeightMap> _nameWeights;
	boost::scoped_ptr<P1Decoder> _nameDecoder;
	double _name_overgen_threshold;

	boost::scoped_ptr<DTFeature::FeatureWeightMap> _descWeights;
	boost::scoped_ptr<P1Decoder> _descDecoder;
	double _desc_overgen_threshold;

	boost::scoped_ptr<DTFeature::FeatureWeightMap> _pronWeights;
	boost::scoped_ptr<P1Decoder> _pronDecoder;
	double _pron_overgen_threshold;

	boost::scoped_ptr<DocumentMentionInformationMapper> _infoMap;
	boost::scoped_ptr<DTCorefObservation> _observation;

	MentionGroupConstraint_ptr _descOnlyConstraints;

	void processMention(const Mention *linkMention, P1Decoder *decoder, double overgen_threshold, MentionGroupList& groups, LinkInfoCache& cache);
	EntitySet* buildFakeEntitySet(MentionGroupList& groups, const DocTheory *docTheory, const Mention *linkMention) const; 
	int compareMentionTypes(const Mention *ment1, const Mention *ment2) const;
	int getHobbsDistance(const Mention *mention, MentionGroup_ptr group, const DocTheory *docTheory) const;

	bool isRepresentativeMention(const Mention *linkMention, MentionGroup_ptr group);
	
	struct RankingRecord {
	public:
		int tag_index;
		MentionGroup_ptr group;
		double score;

		RankingRecord(int t_index, MentionGroup_ptr g, double s) : tag_index(t_index), group(g), score(s) {}

		// Order by descending score.  
		bool operator<(const RankingRecord rhs) const {
			return (score > rhs.score);
		}
	};
};

#endif
