// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/expanders/DistillationExpander.h"

#include "LowercaseEverything.h"
#include "AggressiveMentionExpander.h"
#include "ExternalDictionaryExpander.h"
#include "PredicateStemmer.h"
#include "WordnetExpander.h"
#include "PredicateBooster.h"
#include "HyphenPartExpander.h"
#include "SequenceExpander.h"
#include "NominalizationExpander.h"
#include "AcronymPeriodDeleter.h"
#include "NameEquivalenceExpander.h"
#include "NameNormalizer.h"

float DistillationDocExpander::BOOST_MEMBER_PROB=1.0f;
float DistillationDocExpander::BOOST_UNKNOWN_PROB=0.0f; // do not use
float DistillationDocExpander::BOOST_POSS_PROB=0.0f; // do not use
float DistillationDocExpander::BOOST_OF_PROB=0.0f;  // do not use
float DistillationDocExpander::WN_STEM_PROB=0.9f;
float DistillationDocExpander::PORTER_STEM_PROB=0.4f; // rarely helps, but OK

DistillationDocExpander::DistillationDocExpander() :SequenceExpander() {
	addExpander(boost::make_shared<LowercaseEverything>());
	addExpander(boost::make_shared<HyphenPartExpander>());
	addExpander(boost::make_shared<AggressiveMentionExpander>(true));
	addExpander(boost::make_shared<AcronymPeriodDeleter>());
	addExpander(boost::make_shared<PredicateStemmer>
		(PORTER_STEM_PROB, WN_STEM_PROB));
	addExpander(boost::make_shared<PredicateBooster> 
		(BOOST_MEMBER_PROB,BOOST_UNKNOWN_PROB, BOOST_POSS_PROB, BOOST_OF_PROB));
	addExpander(boost::make_shared<NameNormalizer>());

	// Need to lowercase again at the end (mostly for AggressiveMentionExpander results)
	addExpander(boost::make_shared<LowercaseEverything>());	
}

float DistillationQueryExpander::BOOST_MEMBER_PROB=1.0f;
float DistillationQueryExpander::BOOST_UNKNOWN_PROB=0.5f;
float DistillationQueryExpander::BOOST_POSS_PROB=0.5f;
float DistillationQueryExpander::BOOST_OF_PROB=0.5f;
float DistillationQueryExpander::WN_STEM_PROB=0.9f;
float DistillationQueryExpander::WN_SYNONYM_PROB=0.7f;
float DistillationQueryExpander::PORTER_STEM_PROB=0.4f;
float DistillationQueryExpander::WN_HYPERNYM_PROB=0.3f;
float DistillationQueryExpander::WN_HYPONYM_PROB=0.5f; // rarely helps, but OK
float DistillationQueryExpander::NOMINALIZATION_PROB=0.8f; 

// not a cost; this is a threshold
float DistillationQueryExpander::EQUIV_NAME_THRESHOLD=0.4f;

DistillationQueryExpander::DistillationQueryExpander(const NameDictionary& equivNames) :
	SequenceExpander() 
{
	addExpander(boost::make_shared<LowercaseEverything>  ());
	addExpander(boost::make_shared<HyphenPartExpander>());
	addExpander(boost::make_shared<AcronymPeriodDeleter>());
	addExpander(boost::make_shared<PredicateStemmer> (PORTER_STEM_PROB,
		WN_STEM_PROB));
	addExpander(boost::make_shared<WordnetExpander> (WN_SYNONYM_PROB, 0.0f, WN_HYPONYM_PROB, 0.0f, WN_SYNONYM_PROB, 0.3f) );

	// We normalize twice to make sure eqn works right. Probably instead of this, we should be normalizing
	//   inside the NameEquivalenceExpander; do this after the eval.
	addExpander(boost::make_shared<NameNormalizer>());
	addExpander(boost::make_shared<NameEquivalenceExpander>(equivNames, EQUIV_NAME_THRESHOLD));
	addExpander(boost::make_shared<NameNormalizer>());
	
	// Nominalization is donea fter WordNet expansion. If it were done before, that could help.
	// But, if we do that, we should use a pruned nominalization table to make sure we don't get super-common stuff in there.
	addExpander(boost::make_shared<NominalizationExpander> (NOMINALIZATION_PROB));

	addExpander(boost::make_shared<ExternalDictionaryExpander> ());

	// Not sure if we need to lowercase again at the end, but better be careful
	addExpander(boost::make_shared<LowercaseEverything>  ());
	
}

