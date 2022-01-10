// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include <boost/make_shared.hpp>
#include "Generic/theories/TokenSequence.h"

#include "Generic/sentences/SentenceBreakerFactory.h"
#include "Korean/sentences/kr_SentenceBreaker.h"
#include "Korean/names/kr_NameRecognizer.h"
#include "Korean/edt/kr_RuleNameLinker.h"
#include "Korean/partOfSpeech/kr_PartOfSpeechRecognizer.h"
#include "Generic/tokens/TokenizerFactory.h"
#include "Korean/tokens/kr_Tokenizer.h"
#include "Korean/tokens/kr_Untokenizer.h"
#include "Korean/common/kr_StringTransliterator.h"
#include "Korean/common/kr_version.h"
#include "Korean/common/kr_WordConstants.h"

#include "Korean/morphology/kr_MorphologicalAnalyzer.h"
#include "Korean/morphSelection/kr_MorphDecoder.h"
#include "Korean/morphSelection/kr_MorphModel.h"
#include "Korean/morphSelection/kr_ParseSeeder.h"
#include "Korean/parse/kr_Parser.h"
#include "Korean/parse/kr_NodeInfo.h"
#include "Korean/theories/kr_FeatureValueStructure.h"
#include "Korean/descriptors/kr_CompoundMentionFinder.h"
#include "Generic/common/FeatureModule.h"
#include "KoreanModule.h"

// Plugin setup function
extern "C" DLL_PUBLIC void* setup_Korean() {

	NodeInfo::setImplementation<KoreanNodeInfo>();
	CompoundMentionFinder::setImplementation<KoreanCompoundMentionFinder>();

	// Use LexicalTokens
	TokenSequence::setTokenSequenceTypeForStateLoading(TokenSequence::LEXICAL_TOKEN_SEQUENCE);

	SentenceBreaker::setFactory(boost::shared_ptr<SentenceBreaker::Factory>
								(new SentenceBreakerFactory<KoreanSentenceBreaker>()));

	NameRecognizer::setFactory(boost::shared_ptr<NameRecognizer::Factory>
                                       (new KoreanNameRecognizerFactory()));

	RuleNameLinker::setFactory(boost::shared_ptr<RuleNameLinker::Factory>
                                       (new KoreanRuleNameLinkerFactory()));

	PartOfSpeechRecognizer::setFactory(boost::shared_ptr<PartOfSpeechRecognizer::Factory>
                                       (new KoreanPartOfSpeechRecognizerFactory()));

	Tokenizer::setFactory(boost::shared_ptr<Tokenizer::Factory>
						  (new TokenizerFactory<KoreanTokenizer>()));

	Untokenizer::setFactory(boost::shared_ptr<Untokenizer::Factory>
                                       (new KoreanUntokenizerFactory()));

	StringTransliterator::setFactory(boost::shared_ptr<StringTransliterator::Factory>
                                       (new KoreanStringTransliteratorFactory()));

	SerifVersion::setInstance(boost::shared_ptr<SerifVersion>(new KoreanSerifVersion()));

	WordConstants::setImplementation<KoreanWordConstants>();

	MorphologicalAnalyzer::setFactory(boost::shared_ptr<MorphologicalAnalyzer::Factory>
                                       (new KoreanMorphologicalAnalyzerFactory()));

	MorphDecoder::setFactory(boost::shared_ptr<MorphDecoder::Factory>
                                       (new KoreanMorphDecoderFactory()));

	MorphModel::setFactory(boost::shared_ptr<MorphModel::Factory>
                                       (new KoreanMorphModelFactory()));

	ParseSeeder::setFactory(boost::shared_ptr<ParseSeeder::Factory>
                                       (new KoreanParseSeederFactory()));

	Parser::setFactory(boost::shared_ptr<Parser::Factory>
                                       (new KoreanParserFactory()));

	FeatureValueStructure::setFactory(boost::shared_ptr<FeatureValueStructure::Factory>
                                       (new KoreanFeatureValueStructureFactory()));

	return FeatureModule::setup_return_value();
}
