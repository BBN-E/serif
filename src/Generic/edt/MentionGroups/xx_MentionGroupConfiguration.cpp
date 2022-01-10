// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/MentionGroups/xx_MentionGroupConfiguration.h"

#include "Generic/common/Symbol.h"
#include "Generic/edt/AbbrevTable.h"
#include "Generic/edt/discmodel/DTCorefFeatureTypes.h"
#include "Generic/wordClustering/WordClusterTable.h"

// Mergers
#include "Generic/edt/MentionGroups/mergers/AcronymMerger.h"
#include "Generic/edt/MentionGroups/mergers/AppositiveMerger.h"
#include "Generic/edt/MentionGroups/mergers/CombinedP1RankingMerger.h"
#include "Generic/edt/MentionGroups/mergers/CompositeMentionGroupMerger.h"
#include "Generic/edt/MentionGroups/mergers/CopulaMerger.h"
#include "Generic/edt/MentionGroups/mergers/DummyMentionGroupMerger.h"
#include "Generic/edt/MentionGroups/mergers/FeatureExactMatchMerger.h"
#include "Generic/edt/MentionGroups/mergers/FeatureUniqueMatchMerger.h"
#include "Generic/edt/MentionGroups/mergers/FirstPersonPronounToSpeakerMerger.h"
#include "Generic/edt/MentionGroups/mergers/P1RankingMerger.h"
#include "Generic/edt/MentionGroups/mergers/SecondPersonPronounMerger.h"
#include "Generic/edt/MentionGroups/mergers/SecondPersonPronounToSpeakerMerger.h"
#include "Generic/edt/MentionGroups/mergers/TitleMerger.h"

// Constraints
#include "Generic/edt/MentionGroups/constraints/CompositeMentionGroupConstraint.h"
#include "Generic/edt/MentionGroups/constraints/DummyMentionGroupConstraint.h"
#include "Generic/edt/MentionGroups/constraints/EntityTypeConstraint.h"
#include "Generic/edt/MentionGroups/constraints/GenderClashConstraint.h"
#include "Generic/edt/MentionGroups/constraints/GlobalGPEAffiliationConstraint.h"
#include "Generic/edt/MentionGroups/constraints/LocalGPEAffiliationConstraint.h"
#include "Generic/edt/MentionGroups/constraints/NumberClashConstraint.h"
#include "Generic/edt/MentionGroups/constraints/OperatorClashConstraint.h"
#include "Generic/edt/MentionGroups/constraints/PairFeatureExistenceConstraint.h"
#include "Generic/edt/MentionGroups/constraints/PartitiveConstraint.h"
#include "Generic/edt/MentionGroups/constraints/ActorMatchConstraint.h"
#include "Generic/edt/MentionGroups/constraints/LocationNameOverlapConstraint.h"

// Mention extractors
#include "Generic/edt/MentionGroups/extractors/AcronymExtractor.h"
#include "Generic/edt/MentionGroups/extractors/GenderFeatureExtractor.h"
#include "Generic/edt/MentionGroups/extractors/GPEAffiliationExtractor.h"
#include "Generic/edt/MentionGroups/extractors/NormalizedNameFeatureExtractor.h"
#include "Generic/edt/MentionGroups/extractors/NumberFeatureExtractor.h"
#include "Generic/edt/MentionGroups/extractors/PropositionExtractor.h"
#include "Generic/edt/MentionGroups/extractors/HeadWordExtractor.h"
#include "Generic/edt/MentionGroups/extractors/ConfidentActorMatchExtractor.h"

// MentionPair extractors
#include "Generic/edt/MentionGroups/extractors/EditDistanceExtractor.h"
#include "Generic/edt/MentionGroups/extractors/SharedPropositionExtractor.h"

GenericMentionGroupConfiguration::GenericMentionGroupConfiguration() {
	WordClusterTable::ensureInitializedFromParamFile();
	DTCorefFeatureTypes::ensureFeatureTypesInstantiated();
	AbbrevTable::initialize();
}

MentionGroupMerger* GenericMentionGroupConfiguration::buildMergers() {
	boost::shared_ptr<MentionGroupConstraint> defaultConstraints = buildConstraints(MentionGroupConstraint::DEFAULT);
	boost::shared_ptr<MentionGroupConstraint> highPrecisionConstraints = buildConstraints(MentionGroupConstraint::HIGH_PRECISION);
	return buildMergers(defaultConstraints, highPrecisionConstraints);
}

MentionGroupMerger* GenericMentionGroupConfiguration::buildMergers(MentionGroupConstraint_ptr defaultConstraints,
                                                                   MentionGroupConstraint_ptr highPrecisionConstraints)
{
	CompositeMentionGroupMerger *result = _new CompositeMentionGroupMerger();

	result->add(MentionGroupMerger_ptr(_new FeatureExactMatchMerger(Symbol(L"Mention-normalized-name"), Symbol(L"name"), highPrecisionConstraints)));

	result->add(MentionGroupMerger_ptr(_new FeatureExactMatchMerger(Symbol(L"Mention-operator"), Symbol(L"email"), defaultConstraints)));
	result->add(MentionGroupMerger_ptr(_new FeatureExactMatchMerger(Symbol(L"Mention-operator"), Symbol(L"ip"), defaultConstraints)));
	result->add(MentionGroupMerger_ptr(_new FeatureExactMatchMerger(Symbol(L"Mention-operator"), Symbol(L"phone"), defaultConstraints)));

	result->add(MentionGroupMerger_ptr(_new AppositiveMerger(highPrecisionConstraints)));
	result->add(MentionGroupMerger_ptr(_new CopulaMerger(highPrecisionConstraints)));
	result->add(MentionGroupMerger_ptr(_new TitleMerger(highPrecisionConstraints)));
	result->add(MentionGroupMerger_ptr(_new AcronymMerger(highPrecisionConstraints)));

	result->add(MentionGroupMerger_ptr(_new FirstPersonPronounToSpeakerMerger(defaultConstraints)));
	result->add(MentionGroupMerger_ptr(_new SecondPersonPronounToSpeakerMerger(defaultConstraints)));
	result->add(MentionGroupMerger_ptr(_new SecondPersonPronounMerger(defaultConstraints)));

	return result;
}

MentionGroupConstraint_ptr GenericMentionGroupConfiguration::buildConstraints(MentionGroupConstraint::Mode mode) {
	CompositeMentionGroupConstraint *result = _new CompositeMentionGroupConstraint();
	result->add(MentionGroupConstraint_ptr(_new EntityTypeConstraint()));
	if (mode == MentionGroupConstraint::DEFAULT) {
		result->add(MentionGroupConstraint_ptr(_new NumberClashConstraint()));
		result->add(MentionGroupConstraint_ptr(_new GenderClashConstraint()));
		result->add(MentionGroupConstraint_ptr(_new PartitiveConstraint()));
		result->add(MentionGroupConstraint_ptr(_new GlobalGPEAffiliationConstraint()));
		result->add(MentionGroupConstraint_ptr(_new LocalGPEAffiliationConstraint()));
		result->add(MentionGroupConstraint_ptr(_new OperatorClashConstraint()));
		result->add(MentionGroupConstraint_ptr(_new ActorMatchConstraint()));
		result->add(MentionGroupConstraint_ptr(_new LocationNameOverlapConstraint()));
	}
	return MentionGroupConstraint_ptr(result);
}

std::vector<AttributeValuePairExtractor<Mention>::ptr_type> GenericMentionGroupConfiguration::buildMentionExtractors() {
	std::vector<AttributeValuePairExtractor<Mention>::ptr_type> result;
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new AcronymExtractor()));
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new GenderFeatureExtractor()));
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new GPEAffiliationExtractor()));
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new NormalizedNameFeatureExtractor()));
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new NumberFeatureExtractor()));
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new PropositionExtractor()));
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new HeadWordExtractor()));
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new ConfidentActorMatchExtractor()));
	return result;	
}

std::vector<AttributeValuePairExtractor<MentionPair>::ptr_type> GenericMentionGroupConfiguration::buildMentionPairExtractors() {
	std::vector<AttributeValuePairExtractor<MentionPair>::ptr_type> result;
	result.push_back(AttributeValuePairExtractor<MentionPair>::ptr_type(_new SharedPropositionExtractor()));
	return result;	
}
