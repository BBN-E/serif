// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef P1_RANKING_MERGER_H
#define P1_RANKING_MERGER_H

#include "Generic/edt/MentionGroups/MentionGroupMerger.h"
#include "Generic/discTagger/DTFeature.h"
#include "Generic/theories/Mention.h"

#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>

class LinkInfoCache;
class MentionGroup;
class P1Decoder;
class DocumentMentionInformationMapper;
class DTCorefObservation;
class DTFeatureTypeSet;
class DTTagSet;
class EntitySet;

/**
  *  Merges MentionGroups based on the decisions of a P1 Ranking model.
  *
  *  Each P1RankingMerger represents a P1 Ranking Model with a single target type,
  *  NAME, DESC, or PRON (created by the DTCorefTrainer).  The model_file, tag_set_file,
  *  features_file and overgen_threshold arguments to the constructor represent the 
  *  ParamReader parameters that should be checked to build this particular model. 
  */
class P1RankingMerger : public MentionGroupMerger {
public:

	/** Construct a P1RankingMerger for target type, using the specified parameter names. */
	P1RankingMerger(Mention::Type target, 
		std::string model_file, std::string tag_set_file, 
		std::string features_file, std::string overgen_threshold,
		MentionGroupConstraint_ptr constraints);

	/** Test and apply all merges indicated by this P1Ranking model. */
	void merge(MentionGroupList& groups, LinkInfoCache& cache);

protected:

	/** No-op for this MentionGroupMerger subclass: should never be called. */
	bool shouldMerge(const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const;

private:

	/** Feature set used for each link prediction */
	boost::scoped_ptr<DTFeatureTypeSet> _linkFeatureTypes;
	/** Feature set used for each no-link predicition */
	boost::scoped_ptr<DTFeatureTypeSet> _noneFeatureTypes;
	/** Overall set of feature sets: points to both link and no-link features */
	boost::scoped_array<DTFeatureTypeSet*> _featureTypesArray;

	/** P1 model tag set: always 'link' and 'no-link' */
	boost::scoped_ptr<DTTagSet> _tagSet;

	/** The weights that parameterize the P1 model */
	boost::scoped_ptr<DTFeature::FeatureWeightMap> _p1Weights;
	
	/** The decoder evaluates each link option given the weights. */
	boost::scoped_ptr<P1Decoder> _p1Decoder;

	/** Mention::NAME, Mention::DESC or Mention::PRON */
	Mention::Type _targetType;

	/** Parameter used by the P1 Ranking decoder to over-generate links. */
	double _overgen_threshold;

	/** Stores cached information about each Mention in a document. */
	boost::scoped_ptr<DocumentMentionInformationMapper> _infoMap;

	/** Represents all knkown information about a single merge. */ 
	boost::scoped_ptr<DTCorefObservation> _observation;

	/** Each record represents a possible merge and its associated score */  
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

	/** Constructs an approximation of the document EntitySet given the current set of MentionGroups. */
	EntitySet* buildFakeEntitySet(MentionGroupList& groups, const DocTheory *docTheory); 
};

#endif
