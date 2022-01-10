// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include <iostream>
#include <boost/make_shared.hpp>
#include "SpanishModule.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/common/version.h"
#include "Spanish/common/es_WordConstants.h"
#include "Spanish/parse/es_LanguageSpecificFunctions.h"
#include "Spanish/parse/ParserTrainer/es_ParserTrainerLanguageSpecificFunctions.h"
#include "Spanish/parse/es_WordFeatures.h"
#include "Spanish/trainers/es_HeadFinder.h"
#include "Spanish/parse/es_STags.h"
#include "Spanish/parse/es_NodeInfo.h"
#include "Spanish/parse/es_SignificantConstitOracle.h"
#include "Spanish/descriptors/es_DescriptorClassifier.h"
#include "Spanish/descriptors/DescriptorClassifierTrainer/es_DescriptorClassifierTrainer.h"
#include "Spanish/descriptors/discmodel/es_P1DescFeatureTypes.h"
#include "Spanish/descriptors/es_CompoundMentionFinder.h"
#include "Spanish/descriptors/es_PronounClassifier.h"
#include "Generic/edt/ReferenceResolver.h"

#include "Spanish/edt/es_DescLinkFeatureFunctions.h"
#include "Spanish/edt/es_DescLinkFunctions.h"
#include "Spanish/edt/es_Guesser.h"
#include "Spanish/edt/es_NameLinkFunctions.h"
#include "Spanish/edt/es_PronounLinker.h"
#include "Spanish/edt/es_PronounLinkerUtils.h"
#include "Spanish/edt/es_RuleDescLinker.h"
#include "Spanish/edt/es_RuleNameLinker.h"
#include "Spanish/edt/discmodel/es_DTCorefFeatureTypes.h"
#include "Spanish/edt/MentionGroups/es_MentionGroupConfiguration.h"

#include "Spanish/propositions/es_LinearPropositionFinder.h"
#include "Spanish/propositions/es_PropositionStatusClassifier.h"
#include "Spanish/propositions/es_SemTreeBuilder.h"

#include "Spanish/relations/es_RelationModel.h"
#include "Spanish/relations/es_RelationFinder.h"
#include "Spanish/relations/es_RelationUtilities.h"
#include "Spanish/relations/es_FamilyRelationFinder.h"
#include "Spanish/relations/discmodel/es_P1RelationFeatureTypes.h"

//#include "Spanish/edt/es_DescLinker.h"
//#include "Spanish/edt/es_PreLinker.h"

// Plugin setup function
extern "C" DLL_PUBLIC void* setup_Spanish() {

	LanguageSpecificFunctions::setImplementation<SpanishLanguageSpecificFunctions>();
	WordConstants::setImplementation<SpanishWordConstants>();

	// ======== Parser ============
	STags::setImplementation<SpanishSTags>();
	WordFeatures::setFactory(boost::shared_ptr<WordFeatures::Factory>
                                       (new SpanishWordFeaturesFactory()));
	HeadFinder::setFactory(boost::shared_ptr<HeadFinder::Factory>
                                       (new SpanishHeadFinderFactory()));
	ParserTrainerLanguageSpecificFunctions::setImplementation
		<SpanishParserTrainerLanguageSpecificFunctions>();
	NodeInfo::setImplementation<SpanishNodeInfo>();

	// ======== Descriptors (aka Mentions) ============
	CompoundMentionFinder::setImplementation<SpanishCompoundMentionFinder>();
	PronounClassifier::setFactory(boost::shared_ptr<PronounClassifier::Factory>
                                       (new SpanishPronounClassifierFactory()));
	SignificantConstitOracle::setFactory(boost::shared_ptr<SignificantConstitOracle::Factory>
                                       (new SpanishSignificantConstitOracleFactory()));

	DescriptorClassifier::setFactory(boost::shared_ptr<DescriptorClassifier::Factory>
                                       (new SpanishDescriptorClassifierFactory()));
	DescriptorClassifierTrainer::setFactory(boost::shared_ptr<DescriptorClassifierTrainer::Factory>
                                       (new SpanishDescriptorClassifierTrainerFactory()));
	P1DescFeatureTypes::setFactory(boost::shared_ptr<P1DescFeatureTypes::Factory>
                                       (new SpanishP1DescFeatureTypesFactory()));

	// ======== EDT (aka coref) ============
	DescLinkFeatureFunctions::setImplementation<SpanishDescLinkFeatureFunctions>();
	DescLinkFunctions::setFactory(boost::shared_ptr<DescLinkFunctions::Factory>
                                       (new SpanishDescLinkFunctionsFactory()));
	Guesser::setImplementation<SpanishGuesser>();
	NameLinkFunctions::setImplementation<SpanishNameLinkFunctions>();
	PronounLinker::setFactory(boost::shared_ptr<PronounLinker::Factory>
                                       (new SpanishPronounLinkerFactory()));
	PronounLinkerUtils::setImplementation<SpanishPronounLinkerUtils>();
	RuleDescLinker::setFactory(boost::shared_ptr<RuleDescLinker::Factory>
                                       (new SpanishRuleDescLinkerFactory()));
	RuleNameLinker::setFactory(boost::shared_ptr<RuleNameLinker::Factory>
                                       (new SpanishRuleNameLinkerFactory()));
	DTCorefFeatureTypes::setFactory(boost::shared_ptr<DTCorefFeatureTypes::Factory>
                                       (new SpanishDTCorefFeatureTypesFactory()));
	MentionGroupConfiguration::setFactory(boost::shared_ptr<MentionGroupConfiguration::Factory>
                                       (new SpanishMentionGroupConfigurationFactory()));

	// ==============Props ==================
	LinearPropositionFinder::setFactory(boost::shared_ptr<LinearPropositionFinder::Factory>
                                       (new SpanishLinearPropositionFinderFactory()));
	PropositionStatusClassifier::setFactory(boost::shared_ptr<PropositionStatusClassifier::Factory>
                                       (new SpanishPropositionStatusClassifierFactory()));
	SemTreeBuilder::setFactory(boost::shared_ptr<SemTreeBuilder::Factory>
                                       (new SpanishSemTreeBuilderFactory()));



	SerifVersion::setSerifLanguage(Language::SPANISH);

	// ==============Relations ==================
	RelationFinder::setFactory(boost::shared_ptr<RelationFinder::Factory>
		(new SpanishRelationFinderFactory()));
	RelationModel::setFactory(boost::shared_ptr<RelationModel::Factory>
		(new SpanishRelationModelFactory()));
	RelationUtilities::setFactory(boost::shared_ptr<RelationUtilities::Factory>
		(new SpanishRelationUtilitiesFactory()));
	FamilyRelationFinder::setFactory(boost::shared_ptr<FamilyRelationFinder::Factory>
		(new SpanishFamilyRelationFinderFactory()));

	// relations/discmodel
	P1RelationFeatureTypes::setFactory(boost::shared_ptr<P1RelationFeatureTypes::Factory>
		(new SpanishP1RelationFeatureTypesFactory()));

	return FeatureModule::setup_return_value();
}
