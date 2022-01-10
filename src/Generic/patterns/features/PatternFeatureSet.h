// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PATTERN_FEATURE_SET_H
#define PATTERN_FEATURE_SET_H

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "Generic/common/Attribute.h"
#include "Generic/common/BoostUtil.h"
#include "Generic/patterns/multilingual/AlignedDocSet.h"
#include "Generic/patterns/features/PatternFeature.h"

class SentenceTheory;
class SynNode;
class MentionReturnPFeature;

typedef boost::shared_ptr<MentionReturnPFeature> MentionReturnPFeature_ptr;

class PatternFeatureSet;
typedef boost::shared_ptr<PatternFeatureSet> PatternFeatureSet_ptr;

/**
  * Despite the use of the name "set", this data structure does not 
  * eliminate duplicates! -- Correction: As of 12/8/11 should remove duplicates
  */
class PatternFeatureSet: private boost::noncopyable {
private:
	PatternFeatureSet();
	BOOST_MAKE_SHARED_0ARG_CONSTRUCTOR(PatternFeatureSet);

public:	
	/** Return the sentence number for the first sentence whose text is 
	  * included in this feature set (inclusive).  If no specific sentence
	  * is included, then return -1. */
	int getStartSentence() const { return _start_sentence; }

	/** Return the sentence number for last sentence whose text is 
	  * included in this feature set (inclusive).  If no specific sentence
	  * is included, then return -1. */
	int getEndSentence() const { return _end_sentence; }

	/** Return the start token index for the region of text that was matched 
	  * by this feature's pattern.  This is an inclusive index in the token 
	  * sequence of the sentence indicated by getstartSentence().  If no 
	  * specific token sequence was matched, return -1. */
	int getStartToken() const { return _start_token; }

	/** Return the end token index for the region of text that was matched 
	  * by this feature's pattern.  This is an inclusive index in the token 
	  * sequence of the sentence indicated by getendSentence().  If no 
	  * specific token sequence was matched, return -1. */
	int getEndToken() const { return _end_token; }

	bool equals(PatternFeatureSet_ptr other);

	/** Return the score for this feature set. */
	float getScore() const { return _score; }

	/** Return the best score_group for this feature set. */
	int getBestScoreGroup() const;

	/** Return the number of features in this set. */
	size_t getNFeatures() const { return _features.size(); }

	/** Return the n-th feature in this set. */
	PatternFeature_ptr getFeature(size_t n) const;

	/** Return whether the feature is in the set */
	bool hasFeature(PatternFeature_ptr feature);

	/** Add the given feature to this set. */
	void addFeature(PatternFeature_ptr feature);         // replaces takeFeature() and duplicateFeature()

	/** Add all features in the given feature set to this feature set. */
	void addFeatures(boost::shared_ptr<PatternFeatureSet> source);   // replaces takeFeatures() and duplicateFeatures()

	void removeFeature(size_t n);

	/** Replace the n-th feature in this set with the given feature. */
	void replaceFeature(size_t n, PatternFeature_ptr feature);

	/** Set the score for this feature set. */
	void setScore(float score) { _score = score; }

	void setCoverage(const DocTheory * docTheory);
	
	void setCoverage(const AlignedDocSet_ptr& docSet);

	void setCoverage(const PatternMatcher_ptr patternMatcher);

	/** Manually set the coverage and confidence of this feature set.*/
	void setCoverage(int start_sentence, int end_sentence, int start_token, int end_token);

	/** Return true if the set of MentionReturnPFeatures with the label
	  * "ANSWER" in this feature set are identical to the set of 
	  * PatternReturnFeatures with the label "ANSWER" in otherFS.  (All
	  * other features are ignored during this comparison.)  */
	bool hasSameAnswerFeatures(PatternFeatureSet_ptr otherFS);

	/** Gets the first top-level pattern label it comes across */
	Symbol getTopLevelPatternLabel();

	void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;

	PatternFeatureSet(SerifXML::XMLElement elem, const SerifXML::XMLIdMap *idMap);

private:
	std::vector<PatternFeature_ptr> _features;

	int _start_sentence; // -1 = none/uninitialized
	int _end_sentence;   // -1 = none/uninitialized
	int _start_token;    // -1 = none/uninitialized
	int _end_token;      // -1 = none/uninitialized
	float _score;
	std::wstring _text;  // This gets set by setCoverage(const DocTheory * docTheory)

	/** Remove all features from this feature set.  The score 
	  * is reset to its default value. */
	void clear();

	/** Helper method for hasSameAnswerFeatures. */
	void gatherAnswerFeatures(std::vector<MentionReturnPFeature_ptr>& returnFeatures);
	static const SynNode* getBestCoveringNode(LanguageAttribute language, SentenceTheory* st, int start, int end);
};

#endif
