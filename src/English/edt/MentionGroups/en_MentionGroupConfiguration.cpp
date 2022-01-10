// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/edt/MentionGroups/en_MentionGroupConfiguration.h"
#include "Generic/common/Symbol.h"

// Mergers
#include "Generic/edt/MentionGroups/mergers/CombinedP1RankingMerger.h"
#include "Generic/edt/MentionGroups/mergers/CompositeMentionGroupMerger.h"
#include "Generic/edt/MentionGroups/mergers/FeatureExactMatchMerger.h"
#include "Generic/edt/MentionGroups/mergers/FeatureUniqueMatchMerger.h"
#include "English/edt/MentionGroups/mergers/en_SpecialCaseMerger.h"
#include "English/edt/MentionGroups/mergers/en_PersonNameMerger.h"
#include "English/edt/MentionGroups/mergers/en_HighPrecisionPersonNameMerger.h"

// Constraints
#include "Generic/edt/MentionGroups/constraints/CompositeMentionGroupConstraint.h"
#include "Generic/edt/MentionGroups/constraints/PairFeatureExistenceConstraint.h"

// Mention extractors
#include "Generic/edt/MentionGroups/extractors/EnglishPersonNameVariationsExtractor.h"
#include "English/edt/MentionGroups/extractors/en_OrgNameVariationsExtractor.h"
#include "English/edt/MentionGroups/extractors/en_GPENameVariationsExtractor.h"
#include "English/edt/MentionGroups/extractors/en_AliasExtractor.h"
#include "English/edt/MentionGroups/extractors/en_OperatorExtractor.h"
#include "English/edt/MentionGroups/extractors/en_TitleExtractor.h"

// MentionPair extractors
#include "English/edt/MentionGroups/extractors/en_WHQLinkExtractor.h"
#include "English/edt/MentionGroups/extractors/en_PersonNamePairExtractor.h"

EnglishMentionGroupConfiguration::EnglishMentionGroupConfiguration() {}

MentionGroupMerger* EnglishMentionGroupConfiguration::buildMergers() {
	boost::shared_ptr<MentionGroupConstraint> defaultConstraints = buildConstraints(MentionGroupConstraint::DEFAULT);
	boost::shared_ptr<MentionGroupConstraint> highPrecisionConstraints = buildConstraints(MentionGroupConstraint::HIGH_PRECISION);

	CompositeMentionGroupMerger *result = _new CompositeMentionGroupMerger();
	result->add(MentionGroupMerger_ptr(_defaultConfiguration.buildMergers(defaultConstraints, highPrecisionConstraints)));
	
	result->add(MentionGroupMerger_ptr(_new EnglishSpecialCaseMerger(defaultConstraints)));
	// EnglishRuleNameLinker::linkPERMention()
	result->add(MentionGroupMerger_ptr(_new FeatureExactMatchMerger(Symbol(L"Mention-person-name-variations"), Symbol(L"fullest"), defaultConstraints)));
	result->add(MentionGroupMerger_ptr(_new FeatureExactMatchMerger(Symbol(L"Mention-person-name-variations"), Symbol(L"first-mi-last"), defaultConstraints)));
	result->add(MentionGroupMerger_ptr(_new FeatureExactMatchMerger(Symbol(L"Mention-person-name-variations"), Symbol(L"first-last"), defaultConstraints)));
	result->add(MentionGroupMerger_ptr(_new FeatureUniqueMatchMerger(Symbol(L"Mention-person-name-variations"), Symbol(L"last"), defaultConstraints)));
	result->add(MentionGroupMerger_ptr(_new EnglishHighPrecisionPersonNameMerger(highPrecisionConstraints)));
	result->add(MentionGroupMerger_ptr(_new EnglishPersonNameMerger(defaultConstraints)));
	// EnglishRuleNameLinker::linkORGMention()
	result->add(MentionGroupMerger_ptr(_new FeatureExactMatchMerger(Symbol(L"Mention-org-name-variations"), Symbol(L"fullest"), defaultConstraints)));
	result->add(MentionGroupMerger_ptr(_new FeatureExactMatchMerger(Symbol(L"Mention-org-name-variations"), Symbol(L"no-designator"), defaultConstraints)));
	result->add(MentionGroupMerger_ptr(_new FeatureExactMatchMerger(Symbol(L"Mention-org-name-variations"), Symbol(L"single-word"), defaultConstraints)));
	// EnglishRuleNameLinker::linkGPEMention()
	result->add(MentionGroupMerger_ptr(_new FeatureExactMatchMerger(Symbol(L"Mention-gpe-name-variations"), Symbol(L"fullest"), defaultConstraints)));
	result->add(MentionGroupMerger_ptr(_new FeatureExactMatchMerger(Symbol(L"Mention-gpe-name-variations"), Symbol(L"single-word"), defaultConstraints)));

	result->add(MentionGroupMerger_ptr(_new CombinedP1RankingMerger(defaultConstraints)));

	return result;
}

MentionGroupConstraint_ptr EnglishMentionGroupConfiguration::buildConstraints(MentionGroupConstraint::Mode mode) {
	CompositeMentionGroupConstraint *result = _new CompositeMentionGroupConstraint();
	result->add(_defaultConfiguration.buildConstraints(mode));
	if (mode == MentionGroupConstraint::DEFAULT) {
		result->add(MentionGroupConstraint_ptr(_new PairFeatureExistenceConstraint(Symbol(L"MentionPair-person-name"), Symbol(L"last-clash"))));
		result->add(MentionGroupConstraint_ptr(_new PairFeatureExistenceConstraint(Symbol(L"MentionPair-person-name"), Symbol(L"first-clash"))));
		result->add(MentionGroupConstraint_ptr(_new PairFeatureExistenceConstraint(Symbol(L"MentionPair-person-name"), Symbol(L"suffix-clash"))));
	}
	return MentionGroupConstraint_ptr(result);
}

std::vector<AttributeValuePairExtractor<Mention>::ptr_type> EnglishMentionGroupConfiguration::buildMentionExtractors() {
	std::vector<AttributeValuePairExtractor<Mention>::ptr_type> result = _defaultConfiguration.buildMentionExtractors();
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new EnglishTitleExtractor()));
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new EnglishPersonNameVariationsExtractor()));
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new EnglishOrgNameVariationsExtractor()));
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new EnglishGPENameVariationsExtractor()));
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new EnglishAliasExtractor()));
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new EnglishOperatorExtractor()));
	return result;	
}

std::vector<AttributeValuePairExtractor<MentionPair>::ptr_type> EnglishMentionGroupConfiguration::buildMentionPairExtractors() {
	std::vector<AttributeValuePairExtractor<MentionPair>::ptr_type> result = _defaultConfiguration.buildMentionPairExtractors();
	result.push_back(AttributeValuePairExtractor<MentionPair>::ptr_type(_new EnglishWHQLinkExtractor()));
	result.push_back(AttributeValuePairExtractor<MentionPair>::ptr_type(_new EnglishPersonNamePairExtractor()));
	return result;	
}
