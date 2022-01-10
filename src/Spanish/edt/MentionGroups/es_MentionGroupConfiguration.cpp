// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/ParamReader.h"

#include "Spanish/edt/MentionGroups/es_MentionGroupConfiguration.h"
#include "Generic/common/Symbol.h"
#include <boost/make_shared.hpp>

// Mergers
#include "Generic/edt/MentionGroups/mergers/CombinedP1RankingMerger.h"
#include "Generic/edt/MentionGroups/mergers/CompositeMentionGroupMerger.h"
#include "Generic/edt/MentionGroups/mergers/FeatureExactMatchMerger.h"
#include "Generic/edt/MentionGroups/mergers/FeatureUniqueMatchMerger.h"
#include "Generic/edt/MentionGroups/mergers/MentionPointerMerger.h"
#include "Generic/edt/MentionGroups/mergers/SameDefiniteDescriptionMerger.h"
//#include "Spanish/edt/MentionGroups/mergers/es_SpecialCaseMerger.h"
//#include "Spanish/edt/MentionGroups/mergers/es_PersonNameMerger.h"
//#include "Spanish/edt/MentionGroups/mergers/es_HighPrecisionPersonNameMerger.h"

// Constraints
#include "Generic/edt/MentionGroups/constraints/CompositeMentionGroupConstraint.h"
#include "Generic/edt/MentionGroups/constraints/PairFeatureExistenceConstraint.h"
#include "Spanish/edt/MentionGroups/constraints/es_NameParseClashConstraint.h"


// Mention extractors
#include "Spanish/edt/MentionGroups/extractors/es_PersonNameParseExtractor.h"
#include "Spanish/edt/MentionGroups/extractors/es_ParseLinkExtractor.h"
//#include "Spanish/edt/MentionGroups/extractors/es_OrgNameVariationsExtractor.h"
//#include "Spanish/edt/MentionGroups/extractors/es_GPENameVariationsExtractor.h"
//#include "Spanish/edt/MentionGroups/extractors/es_AliasExtractor.h"
//#include "Spanish/edt/MentionGroups/extractors/es_TitleExtractor.h"

// MentionPair extractors
//#include "Spanish/edt/MentionGroups/extractors/es_WHQLinkExtractor.h"

SpanishMentionGroupConfiguration::SpanishMentionGroupConfiguration() {}

/* High-precision mergers should be added first, followed by lower
 * precision mergers. */
MentionGroupMerger* SpanishMentionGroupConfiguration::buildMergers() {
	// Number from 0 to 100, indicating how aggressive we should be in EDT merging:
	int aggressiveness = ParamReader::getOptionalIntParamWithDefaultValue("coref_aggressiveness", 80);

	boost::shared_ptr<MentionGroupConstraint> defaultConstraints 
		= buildConstraints(MentionGroupConstraint::DEFAULT);
	boost::shared_ptr<MentionGroupConstraint> highPrecisionConstraints 
		= buildConstraints(MentionGroupConstraint::HIGH_PRECISION);

	CompositeMentionGroupMerger *result = _new CompositeMentionGroupMerger();
	result->add(MentionGroupMerger_ptr(_defaultConfiguration.buildMergers(defaultConstraints, highPrecisionConstraints)));

	//result->add(boost::make_shared<SpanishSpecialCaseMerger>(defaultConstraints));
	// SpanishRuleNameLinker::linkPERMention()

	if (aggressiveness > 5) 
		result->add(boost::make_shared<FeatureExactMatchMerger>
					(Symbol(L"Mention-es-person-name-parse"), 
					 SpanishPersonNameParseExtractor::GIVEN_SURNAME1_FEATURE,
					 defaultConstraints));
	if (aggressiveness > 10) 
		result->add(boost::make_shared<MentionPointerMerger>
					(Symbol(L"Mention-es-parse-link"),
					 SpanishParseLinkExtractor::X_AND_HIS_Y,
					 defaultConstraints));
	if (aggressiveness > 10) 
		result->add(boost::make_shared<MentionPointerMerger>
					(Symbol(L"Mention-es-parse-link"),
					 SpanishParseLinkExtractor::PAREN_DEF,
					 defaultConstraints));
	if (aggressiveness > 15) 
		result->add(boost::make_shared<MentionPointerMerger>
					(Symbol(L"Mention-es-parse-link"),
					 SpanishParseLinkExtractor::PSEUDO_APPOSITIVE,
					 defaultConstraints));

	if (aggressiveness > 20) 
		result->add(boost::make_shared<FeatureExactMatchMerger>
					(Symbol(L"Mention-es-person-name-parse"), 
					 SpanishPersonNameParseExtractor::SURNAME1_FEATURE,
					 defaultConstraints));
	if (aggressiveness > 25) 
		result->add(boost::make_shared<FeatureUniqueMatchMerger>
					(Symbol(L"Mention-es-person-name-parse"), 
					 SpanishPersonNameParseExtractor::GIVEN_FEATURE,
					 defaultConstraints));

	if (aggressiveness > 30) 
		result->add(boost::make_shared<MentionPointerMerger>
					(Symbol(L"Mention-es-parse-link"),
					 SpanishParseLinkExtractor::X_IS_Y,
					 defaultConstraints));

	//result->add(boost::make_shared<SpanishHighPrecisionPersonNameMerger>(highPrecisionConstraints));
	//result->add(boost::make_shared<SpanishPersonNameMerger>(defaultConstraints));
	// SpanishRuleNameLinker::linkORGMention()
	if (aggressiveness > 40) 
		result->add(boost::make_shared<FeatureExactMatchMerger>
					(Symbol(L"Mention-org-name-variations"), 
					 Symbol(L"fullest"), defaultConstraints));
	if (aggressiveness > 45) 
		result->add(boost::make_shared<FeatureExactMatchMerger>
					(Symbol(L"Mention-org-name-variations"), 
					 Symbol(L"no-designator"), defaultConstraints));
	if (aggressiveness > 50) 
		result->add(boost::make_shared<FeatureExactMatchMerger>
					(Symbol(L"Mention-org-name-variations"), 
					 Symbol(L"single-word"), defaultConstraints));
	// SpanishRuleNameLinker::linkGPEMention()
	if (aggressiveness > 40) 
		result->add(boost::make_shared<FeatureExactMatchMerger>
					(Symbol(L"Mention-gpe-name-variations"), 
					 Symbol(L"fullest"), defaultConstraints));
	if (aggressiveness > 50) 
		result->add(boost::make_shared<FeatureExactMatchMerger>
					(Symbol(L"Mention-gpe-name-variations"), 
					 Symbol(L"single-word"), defaultConstraints));

	if (aggressiveness > 60) 
		result->add(boost::make_shared<SameDefiniteDescriptionMerger>(defaultConstraints));

	if (aggressiveness > 90) 
	result->add(boost::make_shared<CombinedP1RankingMerger>(defaultConstraints));

	return result;
}

MentionGroupConstraint_ptr SpanishMentionGroupConfiguration::buildConstraints(MentionGroupConstraint::Mode mode) {
	CompositeMentionGroupConstraint *result = _new CompositeMentionGroupConstraint();
	result->add(_defaultConfiguration.buildConstraints(mode));

	result->add(MentionGroupConstraint_ptr(_new SpanishNameParseClashConstraint()));

	if (mode == MentionGroupConstraint::DEFAULT) {
		//result->add(MentionGroupConstraint_ptr(_new SpanishNameParseClashConstraint()));
		/*
		result->add(MentionGroupConstraint_ptr(_new PairFeatureExistenceConstraint(Symbol(L"MentionPair-person-name"), Symbol(L"surname1-clash"))));
		result->add(MentionGroupConstraint_ptr(_new PairFeatureExistenceConstraint(Symbol(L"MentionPair-person-name"), Symbol(L"surname2-clash"))));
		result->add(MentionGroupConstraint_ptr(_new PairFeatureExistenceConstraint(Symbol(L"MentionPair-person-name"), Symbol(L"given-clash"))));
		result->add(MentionGroupConstraint_ptr(_new PairFeatureExistenceConstraint(Symbol(L"MentionPair-person-name"), Symbol(L"suffix-clash"))));
		*/
	}
	return MentionGroupConstraint_ptr(result);
}

std::vector<AttributeValuePairExtractor<Mention>::ptr_type> SpanishMentionGroupConfiguration::buildMentionExtractors() {
	std::vector<AttributeValuePairExtractor<Mention>::ptr_type> result = _defaultConfiguration.buildMentionExtractors();
	//result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new SpanishTitleExtractor()));
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new SpanishPersonNameParseExtractor()));
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new SpanishParseLinkExtractor()));

	//result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new SpanishOrgNameVariationsExtractor()));
	//result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new SpanishGPENameVariationsExtractor()));
	//result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new SpanishAliasExtractor()));
	return result;	
}

std::vector<AttributeValuePairExtractor<MentionPair>::ptr_type> SpanishMentionGroupConfiguration::buildMentionPairExtractors() {
	std::vector<AttributeValuePairExtractor<MentionPair>::ptr_type> result = _defaultConfiguration.buildMentionPairExtractors();
	//result.push_back(AttributeValuePairExtractor<MentionPair>::ptr_type(_new SpanishWHQLinkExtractor()));
	return result;	
}
