// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/partOfSpeech/PartOfSpeechRecognizer.h"
#include "Generic/partOfSpeech/xx_PartOfSpeechRecognizer.h"




boost::shared_ptr<PartOfSpeechRecognizer::Factory> &PartOfSpeechRecognizer::_factory() {
	static boost::shared_ptr<PartOfSpeechRecognizer::Factory> factory(new GenericPartOfSpeechRecognizerFactory());
	return factory;
}
