// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include <iostream>
#include <boost/make_shared.hpp>
#include "Chinese/relations/ch_RelationFinder.h"
#include "Chinese/relations/ch_RelationModel.h"
#include "Chinese/relations/ch_RelationUtilities.h"
#include "Chinese/relations/discmodel/ch_P1RelationFeatureTypes.h"
#include "Chinese/values/ch_ValueRecognizer.h"
#include "Chinese/values/ch_DeprecatedEventValueRecognizer.h" 
#include "Chinese/generics/ch_GenericsFilter.h"

#include "Chinese/propositions/ch_SemTreeBuilder.h"
#include "Chinese/descriptors/ch_DescClassModifiers.h"
#include "Chinese/descriptors/ch_DescriptorClassifier.h"
#include "Chinese/descriptors/DescriptorClassifierTrainer/ch_DescriptorClassifierTrainer.h"
#include "Chinese/descriptors/ch_PronounClassifier.h"
#include "Chinese/sentences/ch_SentenceBreaker.h"
#include "Generic/sentences/SentenceBreakerFactory.h"
#include "Chinese/names/ch_NameRecognizer.h"
//#include "Chinese/reader/ch_DocumentReader.h"
#include "Chinese/parse/ch_WordFeatures.h"
#include "Chinese/edt/ch_PronounLinker.h"
#include "Chinese/edt/ch_RuleDescLinker.h"
#include "Chinese/events/ch_EventLinker.h"
#include "Chinese/metonymy/ch_MetonymyAdder.h"
#include "Chinese/parse/ch_SignificantConstitOracle.h"
#include "Generic/tokens/TokenizerFactory.h"
#include "Chinese/tokens/ch_Tokenizer.h"
#include "Chinese/tokens/ch_Untokenizer.h"
#include "Chinese/descriptors/discmodel/ch_P1DescFeatureTypes.h"
#include "Chinese/docRelationsEvents/ch_EventLinkFeatureTypes.h"
#include "Chinese/docRelationsEvents/ch_RelationTimexArgFeatureTypes.h"
#include "Chinese/edt/discmodel/ch_DTCorefFeatureTypes.h"
#include "Chinese/events/stat/ch_EventAAFeatureTypes.h"
#include "Chinese/events/stat/ch_EventTriggerFeatureTypes.h"
#include "Chinese/common/ch_StringTransliterator.h"
#include "Chinese/edt/ch_DescLinkFunctions.h"
#include "Chinese/trainers/ch_HeadFinder.h"
#include "Chinese/common/ch_WordConstants.h"
#include "Chinese/relations/ch_PotentialRelationCollector.h"
#include "Chinese/parse/ch_NodeInfo.h"
#include "Chinese/parse/ch_STags.h"
#include "Chinese/common/ch_SymbolUtilities.h"
#include "Chinese/descriptors/ch_CompoundMentionFinder.h"
#include "Chinese/edt/ch_DescLinkFeatureFunctions.h"
#include "Chinese/edt/ch_Guesser.h"
#include "Chinese/edt/ch_NameLinkFunctions.h"
#include "Chinese/edt/ch_PronounLinkerUtils.h"
#include "Chinese/events/ch_EventUtilities.h"
#include "Chinese/names/ch_IdFWordFeatures.h"
#include "Chinese/parse/ch_LanguageSpecificFunctions.h"
#include "Chinese/parse/ParserTrainer/ch_ParserTrainerLanguageSpecificFunctions.h"
#include "Chinese/relations/ch_ExpFeatureVectorFilter.h"
#include "Generic/relations/xx_FeatureVectorFilter.h"
#include "Generic/relations/specific_relation_vector_models.h"
#include "Generic/theories/SentenceItemIdentifier.h"
#include "Generic/common/FeatureModule.h"
#include "ChineseModule.h"

// Plugin setup function
extern "C" DLL_PUBLIC void* setup_Chinese() {

	MentionUID_tag::REAL_MAX_SENTENCE_MENTIONS = 250;

	NodeInfo::setImplementation<ChineseNodeInfo>();
	STags::setImplementation<ChineseSTags>();
	SymbolUtilities::setImplementation<ChineseSymbolUtilities>();
	CompoundMentionFinder::setImplementation<ChineseCompoundMentionFinder>();
	DescLinkFeatureFunctions::setImplementation<ChineseDescLinkFeatureFunctions>();
	Guesser::setImplementation<ChineseGuesser>();
	NameLinkFunctions::setImplementation<ChineseNameLinkFunctions>();
	PronounLinkerUtils::setImplementation<ChinesePronounLinkerUtils>();
	EventUtilities::setImplementation<ChineseEventUtilities>();
	IdFWordFeatures::setImplementation<ChineseIdFWordFeatures>();
	LanguageSpecificFunctions::setImplementation<ChineseLanguageSpecificFunctions>();
	ParserTrainerLanguageSpecificFunctions::setImplementation
		<ChineseParserTrainerLanguageSpecificFunctions>();

	// Note: it's possible that TypeB2PFeatureVectorModel should be using
	// ChineseExpFeatureVectorFilter instead of GenericFeatureVectorFilter; 
	// but if so, then the relation models would probably need to be retrained.
	TypeFeatureVectorModel::setModelAndFilter<BackoffProbModel<2>,GenericFeatureVectorFilter>();
	TypeB2PFeatureVectorModel::setModelAndFilter<BackoffProbModel<2>,GenericFeatureVectorFilter>();

	//generics
	GenericsFilter::setFactory(boost::shared_ptr<GenericsFilter::Factory>
		(new ChineseGenericsFilterFactory()));

	//relations
	RelationFinder::setFactory(boost::shared_ptr<RelationFinder::Factory>
		(new ChineseRelationFinderFactory()));
	RelationModel::setFactory(boost::shared_ptr<RelationModel::Factory>
		(new ChineseRelationModelFactory()));
	RelationUtilities::setFactory(boost::shared_ptr<RelationUtilities::Factory>
		(new ChineseRelationUtilitiesFactory()));

	// relations/discmodel
	P1RelationFeatureTypes::setFactory(boost::shared_ptr<P1RelationFeatureTypes::Factory>
		(new ChineseP1RelationFeatureTypesFactory()));

	// values
	ValueRecognizer::setFactory(boost::shared_ptr<ValueRecognizer::Factory>
		(new ChineseValueRecognizerFactory()));
	DeprecatedEventValueRecognizer::setFactory(boost::shared_ptr<DeprecatedEventValueRecognizer::Factory>
		(new ChineseDeprecatedEventValueRecognizerFactory()));

	SemTreeBuilder::setFactory(boost::shared_ptr<SemTreeBuilder::Factory>
                                       (new ChineseSemTreeBuilderFactory()));

	DescClassModifiers::setFactory(boost::shared_ptr<DescClassModifiers::Factory>
                                       (new ChineseDescClassModifiersFactory()));
	DescriptorClassifier::setFactory(boost::shared_ptr<DescriptorClassifier::Factory>
                                       (new ChineseDescriptorClassifierFactory()));
	DescriptorClassifierTrainer::setFactory(boost::shared_ptr<DescriptorClassifierTrainer::Factory>
                                       (new ChineseDescriptorClassifierTrainerFactory()));
	PronounClassifier::setFactory(boost::shared_ptr<PronounClassifier::Factory>
                                       (new ChinesePronounClassifierFactory()));

	SentenceBreaker::setMaxSentenceChars(300);
	SentenceBreaker::setFactory(boost::shared_ptr<SentenceBreaker::Factory>
								(new SentenceBreakerFactory<ChineseSentenceBreaker>()));

	NameRecognizer::setFactory(boost::shared_ptr<NameRecognizer::Factory>
                                       (new ChineseNameRecognizerFactory()));

	//DocumentReader::setFactory(boost::shared_ptr<DocumentReader::Factory>
    //                                   (new ChineseDocumentReaderFactory()));

	WordConstants::setImplementation<ChineseWordConstants>();

	WordFeatures::setFactory(boost::shared_ptr<WordFeatures::Factory>
                                       (new ChineseWordFeaturesFactory()));

	PronounLinker::setFactory(boost::shared_ptr<PronounLinker::Factory>
                                       (new ChinesePronounLinkerFactory()));

	RuleDescLinker::setFactory(boost::shared_ptr<RuleDescLinker::Factory>
                                       (new ChineseRuleDescLinkerFactory()));

	EventLinker::setFactory(boost::shared_ptr<EventLinker::Factory>
                                       (new ChineseEventLinkerFactory()));

	MetonymyAdder::setFactory(boost::shared_ptr<MetonymyAdder::Factory>
                                       (new ChineseMetonymyAdderFactory()));

	SignificantConstitOracle::setFactory(boost::shared_ptr<SignificantConstitOracle::Factory>
                                       (new ChineseSignificantConstitOracleFactory()));

	Tokenizer::setFactory(boost::shared_ptr<Tokenizer::Factory>
						  (new TokenizerFactory<ChineseTokenizer>()));

	Untokenizer::setFactory(boost::shared_ptr<Untokenizer::Factory>
                                       (new ChineseUntokenizerFactory()));

	P1DescFeatureTypes::setFactory(boost::shared_ptr<P1DescFeatureTypes::Factory>
                                       (new ChineseP1DescFeatureTypesFactory()));

	EventLinkFeatureTypes::setFactory(boost::shared_ptr<EventLinkFeatureTypes::Factory>
                                       (new ChineseEventLinkFeatureTypesFactory()));

	RelationTimexArgFeatureTypes::setFactory(boost::shared_ptr<RelationTimexArgFeatureTypes::Factory>
                                       (new ChineseRelationTimexArgFeatureTypesFactory()));

	DTCorefFeatureTypes::setFactory(boost::shared_ptr<DTCorefFeatureTypes::Factory>
                                       (new ChineseDTCorefFeatureTypesFactory()));

	EventAAFeatureTypes::setFactory(boost::shared_ptr<EventAAFeatureTypes::Factory>
                                       (new ChineseEventAAFeatureTypesFactory()));

	EventTriggerFeatureTypes::setFactory(boost::shared_ptr<EventTriggerFeatureTypes::Factory>
                                       (new ChineseEventTriggerFeatureTypesFactory()));

	StringTransliterator::setFactory(boost::shared_ptr<StringTransliterator::Factory>
                                       (new ChineseStringTransliteratorFactory()));

	DescLinkFunctions::setFactory(boost::shared_ptr<DescLinkFunctions::Factory>
                                       (new ChineseDescLinkFunctionsFactory()));

	HeadFinder::setFactory(boost::shared_ptr<HeadFinder::Factory>
                                       (new ChineseHeadFinderFactory()));

	SerifVersion::setSerifLanguage(Language::CHINESE);


	PotentialRelationCollector::setFactory(boost::shared_ptr<PotentialRelationCollector::Factory>
                                       (new ChinesePotentialRelationCollectorFactory()));

	return FeatureModule::setup_return_value();
}
