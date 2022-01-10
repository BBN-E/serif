// Copyright 2017 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CAUSE_EFFECT_RELATION_FINDER_H
#define CAUSE_EFFECT_RELATION_FINDER_H

#include "Generic/patterns/PatternSet.h"
#include "Generic/causeEffect/CauseEffectRelation.h"
#include <vector>

class DocTheory;
class SentenceTheory;
typedef boost::shared_ptr<PatternMatcher> PatternMatcher_ptr;

class CauseEffectRelationFinder {

public:
	CauseEffectRelationFinder();
	~CauseEffectRelationFinder();

	void findCauseEffectRelations(DocTheory *docTheory);
	

private:
	typedef std::pair<PatternSet_ptr, Symbol> PatternSetAndRelationType;
	std::vector<PatternSetAndRelationType> _causeEffectPatternSets;
	
	bool _do_cause_effect_relation_finding;
	std::string _outputDir;

	void createCauseEffectRelations(PatternFeatureSet_ptr match, std::vector<CauseEffectRelation_ptr> &results, const SentenceTheory* sentTheory, Symbol relationType);
	std::vector<PatternSetAndRelationType> loadPatternSets(const std::string &patternSetFileList);
};

#endif

