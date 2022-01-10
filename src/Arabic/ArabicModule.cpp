// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include <iostream>
#include <boost/make_shared.hpp>
#include "Generic/theories/TokenSequence.h"
#include "Generic/edt/ReferenceResolver.h"
#include "Arabic/relations/ar_RelationFinder.h"
#include "Arabic/relations/ar_RelationModel.h"
#include "Arabic/relations/ar_RelationUtilities.h"
#include "Arabic/relations/discmodel/ar_P1RelationFeatureTypes.h"

#include "Arabic/values/ar_ValueRecognizer.h"
#include "Arabic/clutter/ar_ClutterFilter.h"


#include "Arabic/descriptors/ar_DescClassModifiers.h"
#include "Arabic/descriptors/ar_DescriptorClassifier.h"
#include "Arabic/descriptors/DescriptorClassifierTrainer/ar_DescriptorClassifierTrainer.h"
#include "Arabic/descriptors/ar_PronounClassifier.h"
#include "Arabic/sentences/ar_SentenceBreaker.h"
#include "Generic/sentences/SentenceBreakerFactory.h"
#include "Arabic/names/ar_NameRecognizer.h"
//#include "Arabic/reader/ar_DocumentReader.h"
#include "Arabic/parse/ar_WordFeatures.h"
#include "Arabic/edt/ar_DescLinker.h"
#include "Arabic/edt/ar_PronounLinker.h"
#include "Arabic/edt/ar_RuleDescLinker.h"
#include "Arabic/metonymy/ar_MetonymyAdder.h"
#include "Arabic/parse/ar_SignificantConstitOracle.h"
#include "Arabic/partOfSpeech/ar_PartOfSpeechRecognizer.h"
#include "Generic/tokens/TokenizerFactory.h"
#include "Arabic/tokens/ar_Tokenizer.h"
#include "Arabic/descriptors/discmodel/ar_P1DescFeatureTypes.h"
#include "Arabic/edt/discmodel/ar_DTCorefFeatureTypes.h"
#include "Arabic/common/ar_NationalityRecognizer.h"
#include "Arabic/results/ar_ResultCollectionUtilities.h"
#include "Arabic/common/ar_StringTransliterator.h"
#include "Arabic/trainers/ar_HeadFinder.h"
#include "Arabic/common/ar_WordConstants.h"
#include "Arabic/BuckWalter/ar_LexiconFactory.h"
#include "Arabic/BuckWalter/ar_Retokenizer.h"
#include "Arabic/BuckWalter/ar_MorphologicalAnalyzer.h"
#include "Arabic/morphSelection/ar_MorphModel.h"
#include "Arabic/morphSelection/ar_MorphDecoder.h"
#include "Arabic/BuckWalter/ar_ParseSeeder.h"
#include "Arabic/normalizer/ar_MTNormalizer.h"
#include "Arabic/parse/ar_Parser.h"
#include "Arabic/relations/ar_PotentialRelationCollector.h"
#include "Arabic/BuckWalter/ar_FeatureValueStructure.h"
#include "Arabic/parse/ar_NodeInfo.h"
#include "Arabic/parse/ar_STags.h"
#include "Arabic/common/ar_SymbolUtilities.h"
#include "Arabic/descriptors/ar_CompoundMentionFinder.h"
#include "Arabic/edt/ar_DescLinkFeatureFunctions.h"
#include "Arabic/edt/ar_NameLinkFunctions.h"
#include "Arabic/edt/ar_PreLinker.h"
#include "Arabic/names/ar_IdFWordFeatures.h"
#include "Arabic/parse/ar_LanguageSpecificFunctions.h"
#include "Arabic/parse/ParserTrainer/ar_ParserTrainerLanguageSpecificFunctions.h"
#include "Arabic/relations/ar_ExpFeatureVectorFilter.h"
#include "Generic/relations/xx_FeatureVectorFilter.h"
#include "Generic/relations/specific_relation_vector_models.h"
#include "Generic/theories/SentenceItemIdentifier.h"
#include "Generic/common/FeatureModule.h"
#include "ArabicModule.h"

// Plugin setup function
extern "C" DLL_PUBLIC void* setup_Arabic() {

	MentionUID_tag::REAL_MAX_SENTENCE_MENTIONS = 500;

	NodeInfo::setImplementation<ArabicNodeInfo>();
	STags::setImplementation<ArabicSTags>();
	SymbolUtilities::setImplementation<ArabicSymbolUtilities>();
	CompoundMentionFinder::setImplementation<ArabicCompoundMentionFinder>();
	DescLinkFeatureFunctions::setImplementation<ArabicDescLinkFeatureFunctions>();
	NameLinkFunctions::setImplementation<ArabicNameLinkFunctions>();
	PreLinker::setImplementation<ArabicPreLinker>();
	IdFWordFeatures::setImplementation<ArabicIdFWordFeatures>();
	LanguageSpecificFunctions::setImplementation<ArabicLanguageSpecificFunctions>();
	ParserTrainerLanguageSpecificFunctions::setImplementation
		<ArabicParserTrainerLanguageSpecificFunctions>();

	TypeFeatureVectorModel::setModelAndFilter<BackoffProbModel<2>,GenericFeatureVectorFilter>();
	TypeB2PFeatureVectorModel::setModelAndFilter<BackoffProbModel<2>,ArabicExpFeatureVectorFilter>();

	// Use LexicalTokens
	TokenSequence::setTokenSequenceTypeForStateLoading(TokenSequence::LEXICAL_TOKEN_SEQUENCE);

	ReferenceResolver::setDo2PSpeakerMode(true);

	//clutter
	ClutterFilter::setFactory(boost::shared_ptr<ClutterFilter::Factory>
		(new ArabicClutterFilterFactory()));

	// relations
	RelationFinder::setFactory(boost::shared_ptr<RelationFinder::Factory>
		(new ArabicRelationFinderFactory()));
	RelationModel::setFactory(boost::shared_ptr<RelationModel::Factory>
		(new ArabicRelationModelFactory()));
	RelationUtilities::setFactory(boost::shared_ptr<RelationUtilities::Factory>
		(new ArabicRelationUtilitiesFactory()));

	// relations/discmodel
	P1RelationFeatureTypes::setFactory(boost::shared_ptr<P1RelationFeatureTypes::Factory>
		(new ArabicP1RelationFeatureTypesFactory()));

	// values
	ValueRecognizer::setFactory(boost::shared_ptr<ValueRecognizer::Factory>
		(new ArabicValueRecognizerFactory()));

	DescClassModifiers::setFactory(boost::shared_ptr<DescClassModifiers::Factory>
                                       (new ArabicDescClassModifiersFactory()));
	DescriptorClassifier::setFactory(boost::shared_ptr<DescriptorClassifier::Factory>
                                       (new ArabicDescriptorClassifierFactory()));
	DescriptorClassifierTrainer::setFactory(boost::shared_ptr<DescriptorClassifierTrainer::Factory>
                                       (new ArabicDescriptorClassifierTrainerFactory()));
	PronounClassifier::setFactory(boost::shared_ptr<PronounClassifier::Factory>
                                       (new ArabicPronounClassifierFactory()));

	SentenceBreaker::setMaxSentenceChars(1000);
	SentenceBreaker::setFactory(boost::shared_ptr<SentenceBreaker::Factory>
								(new SentenceBreakerFactory<ArabicSentenceBreaker>()));

	NameRecognizer::setFactory(boost::shared_ptr<NameRecognizer::Factory>
                                       (new ArabicNameRecognizerFactory()));

	//DocumentReader::setFactory(boost::shared_ptr<DocumentReader::Factory>
    //                                   (new ArabicDocumentReaderFactory()));

	WordConstants::setImplementation<ArabicWordConstants>();

	WordFeatures::setFactory(boost::shared_ptr<WordFeatures::Factory>
                                       (new ArabicWordFeaturesFactory()));

	DescLinker::setFactory(boost::shared_ptr<DescLinker::Factory>
                                       (new ArabicDescLinkerFactory()));

	PronounLinker::setFactory(boost::shared_ptr<PronounLinker::Factory>
                                       (new ArabicPronounLinkerFactory()));

	RuleDescLinker::setFactory(boost::shared_ptr<RuleDescLinker::Factory>
                                       (new ArabicRuleDescLinkerFactory()));

	MetonymyAdder::setFactory(boost::shared_ptr<MetonymyAdder::Factory>
                                       (new ArabicMetonymyAdderFactory()));

	SignificantConstitOracle::setFactory(boost::shared_ptr<SignificantConstitOracle::Factory>
                                       (new ArabicSignificantConstitOracleFactory()));

	PartOfSpeechRecognizer::setFactory(boost::shared_ptr<PartOfSpeechRecognizer::Factory>
                                       (new ArabicPartOfSpeechRecognizerFactory()));

	Tokenizer::setFactory(boost::shared_ptr<Tokenizer::Factory>
						  (new TokenizerFactory<ArabicTokenizer>()));

	ArabicP1DescFeatureTypes::setFactory(boost::shared_ptr<ArabicP1DescFeatureTypes::Factory>
                                       (new ArabicP1DescFeatureTypesFactory()));

	DTCorefFeatureTypes::setFactory(boost::shared_ptr<DTCorefFeatureTypes::Factory>
                                       (new ArabicDTCorefFeatureTypesFactory()));

	NationalityRecognizer::setFactory(boost::shared_ptr<NationalityRecognizer::Factory>
                                       (new ArabicNationalityRecognizerFactory()));

	ResultCollectionUtilities::setFactory(boost::shared_ptr<ResultCollectionUtilities::Factory>
                                       (new ArabicResultCollectionUtilitiesFactory()));

	StringTransliterator::setFactory(boost::shared_ptr<StringTransliterator::Factory>
                                       (new ArabicStringTransliteratorFactory()));

	HeadFinder::setFactory(boost::shared_ptr<HeadFinder::Factory>
                                       (new ArabicHeadFinderFactory()));

	SerifVersion::setSerifLanguage(Language::ARABIC);

	Lexicon::setFactory(boost::shared_ptr<Lexicon::Factory>
						(new ArabicLexiconFactory()));

	Retokenizer::setImplementation<ArabicRetokenizer>();

	MorphologicalAnalyzer::setFactory(boost::shared_ptr<MorphologicalAnalyzer::Factory>
                                       (new ArabicMorphologicalAnalyzerFactory()));

	MorphModel::setFactory(boost::shared_ptr<MorphModel::Factory>
                                       (new ArabicMorphModelFactory()));

	ParseSeeder::setFactory(boost::shared_ptr<ParseSeeder::Factory>
                                       (new ArabicParseSeederFactory()));

	MorphDecoder::setFactory(boost::shared_ptr<MorphDecoder::Factory>
                                       (new ArabicMorphDecoderFactory()));

	MTNormalizer::setFactory(boost::shared_ptr<MTNormalizer::Factory>
                                       (new ArabicMTNormalizerFactory()));

	Parser::setFactory(boost::shared_ptr<Parser::Factory>
                                       (new ArabicParserFactory()));

	PotentialRelationCollector::setFactory(boost::shared_ptr<PotentialRelationCollector::Factory>
                                       (new ArabicPotentialRelationCollectorFactory()));

	FeatureValueStructure::setFactory(boost::shared_ptr<FeatureValueStructure::Factory>
                                       (new ArabicFeatureValueStructureFactory()));

	return FeatureModule::setup_return_value();
}
