// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include <iostream>
#include <boost/make_shared.hpp>
#include "Generic/edt/ReferenceResolver.h"
#include "English/relations/en_RelationFinder.h"
#include "English/relations/en_RelationModel.h"
#include "English/relations/en_RelationUtilities.h"
#include "English/relations/en_FamilyRelationFinder.h"
#include "English/relations/discmodel/en_P1RelationFeatureTypes.h"
#include "English/values/en_ValueRecognizer.h"
#include "English/values/en_PatternEventValueRecognizer.h"
#include "English/values/en_DeprecatedEventValueRecognizer.h"
#include "English/generics/en_GenericsFilter.h"
#include "English/clutter/en_ClutterFilter.h"
#include "English/propositions/en_LinearPropositionFinder.h"
#include "English/propositions/en_PropositionStatusClassifier.h"
#include "English/propositions/en_SemTreeBuilder.h"
#include "English/descriptors/en_DescClassModifiers.h"
#include "English/descriptors/en_DescriptorClassifier.h"
#include "English/descriptors/DescriptorClassifierTrainer/en_DescriptorClassifierTrainer.h"
#include "English/descriptors/en_NomPremodClassifier.h"
#include "English/descriptors/en_PronounClassifier.h"
#include "English/sentences/en_SentenceBreaker.h"
#include "Generic/sentences/SentenceBreakerFactory.h"
#include "English/names/en_NameRecognizer.h"
//#include "English/reader/en_DocumentReader.h"
#include "English/timex/en_TemporalNormalizer.h"
#include "English/values/en_NumberConverter.h"
#include "English/parse/en_WordFeatures.h"
#include "English/docRelationsEvents/en_StructuralRelationFinder.h"
#include "English/edt/en_DescLinker.h"
//#include "English/PNPChunking/en_NPChunkFinder.h"
#include "English/UTCoref/en_LinkAllMentions.h"
#include "English/edt/en_PronounLinker.h"
#include "English/edt/en_RuleDescLinker.h"
#include "English/edt/en_RuleNameLinker.h"
#include "English/events/en_EventLinker.h"
#include "English/metonymy/en_MetonymyAdder.h"
#include "English/parse/en_SignificantConstitOracle.h"
#include "English/partOfSpeech/en_PartOfSpeechRecognizer.h"
#include "Generic/tokens/TokenizerFactory.h"
#include "English/tokens/en_Tokenizer.h"
#include "English/tokens/en_Untokenizer.h"
#include "English/descriptors/discmodel/en_P1DescFeatureTypes.h"
#include "English/discourseRel/en_DiscourseRelFeatureTypes.h"
#include "English/docRelationsEvents/en_EventLinkFeatureTypes.h"
#include "English/docRelationsEvents/en_RelationTimexArgFeatureTypes.h"
#include "English/edt/discmodel/en_DTCorefFeatureTypes.h"
#include "English/edt/MentionGroups/en_MentionGroupConfiguration.h"
#include "English/edt/MentionGroups/en_DTRAMentionGroupConfiguration.h"
#include "English/events/stat/en_EventAAFeatureTypes.h"
#include "English/events/stat/en_EventModalityFeatureTypes.h"
#include "English/events/stat/en_EventTriggerFeatureTypes.h"
#include "English/common/en_AdeptWordConstants.h"
#include "English/common/en_CBRNEWordConstants.h"
#include "English/common/en_WordConstants.h"
#include "English/common/en_NationalityRecognizer.h"
#include "English/results/en_ResultCollectionUtilities.h"
#include "English/descriptors/DescriptorClassifierTrainer/en_PartitiveFinder.h"
#include "English/common/en_StringTransliterator.h"
#include "English/eeml/en_GroupFnGuesser.h"
#include "English/common/en_EvalSpecificRules.h"
#include "English/edt/en_DescLinkFunctions.h"
#include "English/trainers/en_HeadFinder.h"
#include "English/parse/en_NodeInfo.h"
#include "English/parse/en_STags.h"
#include "English/common/en_SymbolUtilities.h"
#include "English/descriptors/en_CompoundMentionFinder.h"
#include "English/descriptors/en_TemporalIdentifier.h"
#include "English/edt/en_DescLinkFeatureFunctions.h"
#include "English/edt/en_Guesser.h"
#include "English/edt/en_NameLinkFunctions.h"
#include "English/edt/en_PreLinker.h"
#include "English/edt/en_PronounLinkerUtils.h"
#include "English/events/en_EventUtilities.h"
#include "English/names/en_IdFWordFeatures.h"
#include "English/parse/en_LanguageSpecificFunctions.h"
#include "English/parse/ParserTrainer/en_ParserTrainerLanguageSpecificFunctions.h"
#include "English/relations/en_FeatureVectorFilter.h"
#include "English/relations/en_ExpFeatureVectorFilter.h"
#include "Generic/relations/specific_relation_vector_models.h"
#include "English/confidences/en_ConfidenceEstimator.h"
#include "Generic/theories/SentenceItemIdentifier.h"
#include "Generic/common/cleanup_hooks.h"
#include "Generic/common/FeatureModule.h"
#include "EnglishModule.h"

namespace {
	void english_module_cleanup() {
		// called if ENABLE_LEAK_DETECTION is turned on.
		TemporalIdentifier::freeTemporalHeadwordList();
	}
}

// Plugin setup function
extern "C" DLL_PUBLIC void* setup_English() {
	MentionUID_tag::REAL_MAX_SENTENCE_MENTIONS = 100;

	addCleanupHook(english_module_cleanup);

	ReferenceResolver::setDo2PSpeakerMode(true);

	NodeInfo::setImplementation<EnglishNodeInfo>();
	STags::setImplementation<EnglishSTags>();
	SymbolUtilities::setImplementation<EnglishSymbolUtilities>();
	CompoundMentionFinder::setImplementation<EnglishCompoundMentionFinder>();
	DescLinkFeatureFunctions::setImplementation<EnglishDescLinkFeatureFunctions>();
	Guesser::setImplementation<EnglishGuesser>();
	NameLinkFunctions::setImplementation<EnglishNameLinkFunctions>();
	PreLinker::setImplementation<EnglishPreLinker>();
	PronounLinkerUtils::setImplementation<EnglishPronounLinkerUtils>();
	EventUtilities::setImplementation<EnglishEventUtilities>();
	IdFWordFeatures::setImplementation<EnglishIdFWordFeatures>();
	LanguageSpecificFunctions::setImplementation<EnglishLanguageSpecificFunctions>();
	ParserTrainerLanguageSpecificFunctions::setImplementation
		<EnglishParserTrainerLanguageSpecificFunctions>();

	TypeFeatureVectorModel::setModelAndFilter<BackoffProbModel<2>,EnglishFeatureVectorFilter>();
	TypeB2PFeatureVectorModel::setModelAndFilter<BackoffProbModel<3>,EnglishExpFeatureVectorFilter>();

	//clutter
	ClutterFilter::setFactory(boost::shared_ptr<ClutterFilter::Factory>
		(new EnglishClutterFilterFactory()));

	// generics
	GenericsFilter::setFactory(boost::shared_ptr<GenericsFilter::Factory>
		(new EnglishGenericsFilterFactory()));

	// relations
	RelationFinder::setFactory(boost::shared_ptr<RelationFinder::Factory>
		(new EnglishRelationFinderFactory()));
	RelationModel::setFactory(boost::shared_ptr<RelationModel::Factory>
		(new EnglishRelationModelFactory()));
	RelationUtilities::setFactory(boost::shared_ptr<RelationUtilities::Factory>
		(new EnglishRelationUtilitiesFactory()));
	FamilyRelationFinder::setFactory(boost::shared_ptr<FamilyRelationFinder::Factory>
		(new EnglishFamilyRelationFinderFactory()));

	// relations/discmodel
	P1RelationFeatureTypes::setFactory(boost::shared_ptr<P1RelationFeatureTypes::Factory>
		(new EnglishP1RelationFeatureTypesFactory()));

	// values
	ValueRecognizer::setFactory(boost::shared_ptr<ValueRecognizer::Factory>
		(new EnglishValueRecognizerFactory()));
	PatternEventValueRecognizer::setFactory(boost::shared_ptr<PatternEventValueRecognizer::Factory>
		(new EnglishPatternEventValueRecognizerFactory()));
	DeprecatedEventValueRecognizer::setFactory(boost::shared_ptr<DeprecatedEventValueRecognizer::Factory>
		(new EnglishDeprecatedEventValueRecognizerFactory()));

	LinearPropositionFinder::setFactory(boost::shared_ptr<LinearPropositionFinder::Factory>
                                       (new EnglishLinearPropositionFinderFactory()));
	PropositionStatusClassifier::setFactory(boost::shared_ptr<PropositionStatusClassifier::Factory>
                                       (new EnglishPropositionStatusClassifierFactory()));
	SemTreeBuilder::setFactory(boost::shared_ptr<SemTreeBuilder::Factory>
                                       (new EnglishSemTreeBuilderFactory()));


	DescClassModifiers::setFactory(boost::shared_ptr<DescClassModifiers::Factory>
                                       (new EnglishDescClassModifiersFactory()));
	DescriptorClassifier::setFactory(boost::shared_ptr<DescriptorClassifier::Factory>
                                       (new EnglishDescriptorClassifierFactory()));
	DescriptorClassifierTrainer::setFactory(boost::shared_ptr<DescriptorClassifierTrainer::Factory>
                                       (new EnglishDescriptorClassifierTrainerFactory()));
	NomPremodClassifier::setFactory(boost::shared_ptr<NomPremodClassifier::Factory>
                                       (new EnglishNomPremodClassifierFactory()));
	PronounClassifier::setFactory(boost::shared_ptr<PronounClassifier::Factory>
                                       (new EnglishPronounClassifierFactory()));

	SentenceBreaker::setMaxSentenceChars(1000);
	SentenceBreaker::setFactory(boost::shared_ptr<SentenceBreaker::Factory>
								(new SentenceBreakerFactory<EnglishSentenceBreaker>()));

	NameRecognizer::setFactory(boost::shared_ptr<NameRecognizer::Factory>
                                       (new EnglishNameRecognizerFactory()));

	//DocumentReader::setFactory(boost::shared_ptr<DocumentReader::Factory>
    //                                   (new EnglishDocumentReaderFactory()));

	TemporalNormalizer::setFactory(boost::shared_ptr<TemporalNormalizer::Factory>
                                       (new EnglishTemporalNormalizerFactory()));

	NumberConverter::setFactory(boost::shared_ptr<NumberConverter::Factory>
									   (new EnglishNumberConverterFactory()));

	WordFeatures::setFactory(boost::shared_ptr<WordFeatures::Factory>
                                       (new EnglishWordFeaturesFactory()));

	StructuralRelationFinder::setFactory(boost::shared_ptr<StructuralRelationFinder::Factory>
                                       (new EnglishStructuralRelationFinderFactory()));

	DescLinker::setFactory(boost::shared_ptr<DescLinker::Factory>
                                       (new EnglishDescLinkerFactory()));

	// uses default
	//NPChunkFinder::setFactory(boost::shared_ptr<NPChunkFinder::Factory>
    //                                   (new EnglishNPChunkFinderFactory()));

	LinkAllMentions::setFactory(boost::shared_ptr<LinkAllMentions::Factory>
                                       (new EnglishLinkAllMentionsFactory()));

	PronounLinker::setFactory(boost::shared_ptr<PronounLinker::Factory>
                                       (new EnglishPronounLinkerFactory()));

	RuleDescLinker::setFactory(boost::shared_ptr<RuleDescLinker::Factory>
                                       (new EnglishRuleDescLinkerFactory()));

	RuleNameLinker::setFactory(boost::shared_ptr<RuleNameLinker::Factory>
                                       (new EnglishRuleNameLinkerFactory()));

	EventLinker::setFactory(boost::shared_ptr<EventLinker::Factory>
                                       (new EnglishEventLinkerFactory()));

	MetonymyAdder::setFactory(boost::shared_ptr<MetonymyAdder::Factory>
                                       (new EnglishMetonymyAdderFactory()));

	SignificantConstitOracle::setFactory(boost::shared_ptr<SignificantConstitOracle::Factory>
                                       (new EnglishSignificantConstitOracleFactory()));

	PartOfSpeechRecognizer::setFactory(boost::shared_ptr<PartOfSpeechRecognizer::Factory>
                                       (new EnglishPartOfSpeechRecognizerFactory()));

	Tokenizer::setFactory(boost::shared_ptr<Tokenizer::Factory>
						  (new TokenizerFactory<EnglishTokenizer>()));

	Untokenizer::setFactory(boost::shared_ptr<Untokenizer::Factory>
                                       (new EnglishUntokenizerFactory()));

	P1DescFeatureTypes::setFactory(boost::shared_ptr<P1DescFeatureTypes::Factory>
                                       (new EnglishP1DescFeatureTypesFactory()));

	DiscourseRelFeatureTypes::setFactory(boost::shared_ptr<DiscourseRelFeatureTypes::Factory>
                                       (new EnglishDiscourseRelFeatureTypesFactory()));

	EventLinkFeatureTypes::setFactory(boost::shared_ptr<EventLinkFeatureTypes::Factory>
                                       (new EnglishEventLinkFeatureTypesFactory()));

	RelationTimexArgFeatureTypes::setFactory(boost::shared_ptr<RelationTimexArgFeatureTypes::Factory>
                                       (new EnglishRelationTimexArgFeatureTypesFactory()));

	DTCorefFeatureTypes::setFactory(boost::shared_ptr<DTCorefFeatureTypes::Factory>
                                       (new EnglishDTCorefFeatureTypesFactory()));

	EventAAFeatureTypes::setFactory(boost::shared_ptr<EventAAFeatureTypes::Factory>
                                       (new EnglishEventAAFeatureTypesFactory()));

	EventModalityFeatureTypes::setFactory(boost::shared_ptr<EventModalityFeatureTypes::Factory>
                                       (new EnglishEventModalityFeatureTypesFactory()));

	EventTriggerFeatureTypes::setFactory(boost::shared_ptr<EventTriggerFeatureTypes::Factory>
                                       (new EnglishEventTriggerFeatureTypesFactory()));

	WordConstants::setImplementation<EnglishWordConstants>();

	AdeptWordConstants::setFactory(boost::shared_ptr<AdeptWordConstants::Factory>
                                       (new EnglishAdeptWordConstantsFactory()));

	CBRNEWordConstants::setFactory(boost::shared_ptr<CBRNEWordConstants::Factory>
                                       (new EnglishCBRNEWordConstantsFactory()));

	NationalityRecognizer::setFactory(boost::shared_ptr<NationalityRecognizer::Factory>
                                       (new EnglishNationalityRecognizerFactory()));

	ResultCollectionUtilities::setFactory(boost::shared_ptr<ResultCollectionUtilities::Factory>
                                       (new EnglishResultCollectionUtilitiesFactory()));

	PartitiveFinder::setFactory(boost::shared_ptr<PartitiveFinder::Factory>
                                       (new EnglishPartitiveFinderFactory()));

	StringTransliterator::setFactory(boost::shared_ptr<StringTransliterator::Factory>
                                       (new EnglishStringTransliteratorFactory()));

	GroupFnGuesser::setFactory(boost::shared_ptr<GroupFnGuesser::Factory>
                                       (new EnglishGroupFnGuesserFactory()));

	EvalSpecificRules::setFactory(boost::shared_ptr<EvalSpecificRules::Factory>
                                       (new EnglishEvalSpecificRulesFactory()));

	DescLinkFunctions::setFactory(boost::shared_ptr<DescLinkFunctions::Factory>
                                       (new EnglishDescLinkFunctionsFactory()));

	HeadFinder::setFactory(boost::shared_ptr<HeadFinder::Factory>
                                       (new EnglishHeadFinderFactory()));

	if (ParamReader::getParam("source_format") == "dtra") {
		MentionGroupConfiguration::setFactory(boost::shared_ptr<MentionGroupConfiguration::Factory>
										   (new EnglishDTRAMentionGroupConfigurationFactory()));

	} else {
		MentionGroupConfiguration::setFactory(boost::shared_ptr<MentionGroupConfiguration::Factory>
										   (new EnglishMentionGroupConfigurationFactory()));
	}

	ConfidenceEstimator::setFactory(boost::shared_ptr<ConfidenceEstimator::Factory>
										(new EnglishConfidenceEstimatorFactory()));

	SerifVersion::setSerifLanguage(Language::ENGLISH);

	return FeatureModule::setup_return_value();
}
