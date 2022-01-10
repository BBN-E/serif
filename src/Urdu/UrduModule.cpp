// Copyright 2013 by Raytheon BBN Technologies
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include <iostream>
#include <boost/make_shared.hpp>
#include "UrduModule.h"
#include "Generic/common/FeatureModule.h"
#include "Urdu/sentences/ur_SentenceBreaker.h"
#include "Generic/sentences/SentenceBreakerFactory.h"
#include "Urdu/tokens/ur_Tokenizer.h"
#include "Generic/tokens/TokenizerFactory.h"
#include "Urdu/partOfSpeech/ur_PartOfSpeechRecognizer.h"
#include "Generic/partOfSpeech/PartOfSpeechRecognizer.h"
#include "Urdu/common/ur_WordConstants.h"
#include "Urdu/parse/ur_LanguageSpecificFunctions.h"
#include "Urdu/parse/ParserTrainer/ur_ParserTrainerLanguageSpecificFunctions.h"
#include "Urdu/trainers/ur_HeadFinder.h"
#include "Urdu/parse/ur_STags.h"
#include "Urdu/parse/ur_WordFeatures.h"
#include "Urdu/parse/ur_NodeInfo.h"

// Plugin setup function
extern "C" DLL_PUBLIC void* setup_Urdu() {

  SentenceBreaker::setFactory(boost::shared_ptr<SentenceBreaker::Factory>
                              (new SentenceBreakerFactory<UrduSentenceBreaker>()));

  Tokenizer::setFactory(boost::shared_ptr<Tokenizer::Factory>
                        (new TokenizerFactory<UrduTokenizer>()));

  PartOfSpeechRecognizer::setFactory(boost::shared_ptr<PartOfSpeechRecognizer::Factory>
		(new UrduPartOfSpeechRecognizerFactory()));

  LanguageSpecificFunctions::setImplementation<UrduLanguageSpecificFunctions>();
  WordConstants::setImplementation<UrduWordConstants>();
  STags::setImplementation<UrduSTags>();
  WordFeatures::setFactory(boost::shared_ptr<WordFeatures::Factory> 
                           (new UrduWordFeaturesFactory()));
  HeadFinder::setFactory(boost::shared_ptr<HeadFinder::Factory>
                         (new UrduHeadFinderFactory()));
  ParserTrainerLanguageSpecificFunctions::setImplementation<UrduParserTrainerLanguageSpecificFunctions>();
  //  NodeInfo::setImplementation<UrduNodeInfo>();

  return FeatureModule::setup_return_value();
}
