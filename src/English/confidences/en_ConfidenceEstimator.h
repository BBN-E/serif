#ifndef EN_CONFIDENCE_ESTIMATOR_H
#define EN_CONFIDENCE_ESTIMATOR_H

#include "Generic/confidences/ConfidenceEstimator.h"

#include <vector>
#include <boost/shared_ptr.hpp>

class Entity;
class EntitySet;
class IdFWordFeatures;
class Mention;
class RelMention;
class Proposition;

class EnglishConfidenceEstimator : public ConfidenceEstimator {
private:
	friend class EnglishConfidenceEstimatorFactory;

	bool _estimate_confidences;
	IdFWordFeatures *_wordFeatures;

public:
	void process (DocTheory* docTheory);

private:
	EnglishConfidenceEstimator();
	~EnglishConfidenceEstimator();

	void addMentionConfidenceScores(const Entity *entity, const DocTheory *docTheory) const;
	double calculateConfidenceScore(const Mention *mention, const DocTheory *docTheory) const;
	double calculateNameConfidenceScore(const Mention *mention, const DocTheory *docTheory) const;
	double calculateDescConfidenceScore(const Mention *mention, const DocTheory *docTheory) const;
	double calculateLinkConfidenceScore(const Mention *mention, const Mention *canonical, const DocTheory *docTheory) const;
	
	double calculateConfidenceScore(const RelMention *relMention) const;
		
	std::vector<Proposition*> verbPropositions(const Mention* mention, const DocTheory *docTheory) const;
	std::vector<RelMention*> relationMentions(const Mention* mention, const DocTheory *docTheory) const;

	const Mention* getParentMention(const Mention *mention) const;
	bool isSpeakerMention(const Mention* mention, const DocTheory *docTheory) const;
	const Mention* getBestMention(const Entity* entity, const DocTheory *docTheory) const;
	double mentionScore(const Mention* mention, const DocTheory *docTheory) const;
	std::vector<const Mention*> getEquivalentMentions(const Mention* mention, const EntitySet *entitySet) const;
	int getMinCharacterDistance(const Mention *mention, std::vector<const Mention*> &mentions, const DocTheory *docTheory) const;
	int getCharacterDistance(const Mention* mention1, const Mention* mention2, const DocTheory *docTheory) const;

	bool relationHasNestedArgs(const RelMention *relMention) const;
	int relationArgumentDist(const RelMention *relMention) const;

};

class EnglishConfidenceEstimatorFactory: public ConfidenceEstimator::Factory {
	virtual ConfidenceEstimator *build() { return _new EnglishConfidenceEstimator(); }
};

#endif
