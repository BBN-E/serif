// Copyright (c) 2014 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef SECTOR_FACT_FINDER_H
#define SECTOR_FACT_FINDER_H

#include "Generic/patterns/PatternMatcher.h"

class FactPatternManager;

class SectorFactFinder {

public:
	SectorFactFinder();
	~SectorFactFinder();

	void findFacts(DocTheory *docTheory);

private:

	static const std::wstring FF_SECTOR;
	static const std::wstring FF_ROLE;
	static const std::wstring FF_AGENT1;

	static const Symbol FF_ACTOR_SECTOR;
	static const Symbol FF_SECTOR_SYM;
	static const Symbol FF_ACTOR;

	const FactPatternManager* _patternManager;
	void processPatternSet(PatternMatcher_ptr pm, Symbol patSetEntityTypeSym, Symbol extraFlagSymbol);
	void processEntity(PatternMatcher_ptr pm, int entity_id);
	void addFactsFromFeatureSet(PatternMatcher_ptr pm, int entity_id, PatternFeatureSet_ptr featureSet);


};

#endif
