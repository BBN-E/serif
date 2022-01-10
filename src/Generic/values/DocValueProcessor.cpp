// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/values/DocValueProcessor.h"
#include "Generic/values/DeprecatedEventValueRecognizer.h"
#include "Generic/values/PatternEventValueRecognizer.h"
#include "Generic/values/ValuePromoter.h"
#include "Generic/values/TemporalNormalizer.h"


#include "Generic/CASerif/correctanswers/CorrectAnswers.h"

DocValueProcessor::DocValueProcessor() {
	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");
	_valuePromoter = _new ValuePromoter();
	//Backwards compatable. Please remove the first grouping when parameter files are changed.
	_deprecatedEventValueRecognizer = 0;
	_patternEventValueRecognizer = 0;
	std::string parameter = ParamReader::getParam("event_model_type");
	if(parameter != "") {
		//use the depricated ValueRecognizer, if it is being used
		if(parameter != "NONE") {
			if(ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_depricated_event_patterns", false)) {
				_deprecatedEventValueRecognizer = DeprecatedEventValueRecognizer::build();
			} else {
				_patternEventValueRecognizer = PatternEventValueRecognizer::build();
			}
		}
	}
	else {
		//use the BPL Pattern event value recognizer, if we are using the event pattern finder
		if(ParamReader::isParamTrue("use_event_patterns") || ParamReader::isParamTrue("use_event_mode")) {
			_patternEventValueRecognizer = PatternEventValueRecognizer::build();
		}
	}

	_temporalNormalizer = TemporalNormalizer::build();

	_use_preexisting_event_values = ParamReader::isParamTrue("use_preexisting_event_only_values");

}

DocValueProcessor::~DocValueProcessor() {
	delete _valuePromoter;
	delete _deprecatedEventValueRecognizer;
	delete _patternEventValueRecognizer;
	delete _temporalNormalizer;
}


void DocValueProcessor::doDocValues(DocTheory *docTheory) {

	CorrectAnswers *correctAnswers = (_use_correct_answers ?
									  &CorrectAnswers::getInstance() : 0);

	if (_use_preexisting_event_values)
		docTheory->takeDocumentValueMentionSet(_new ValueMentionSet(NULL, 0));
	else if (_use_correct_answers && correctAnswers->usingCorrectEvents()) 
		docTheory->takeDocumentValueMentionSet(_new ValueMentionSet(NULL, 0));
	else 
		if(_deprecatedEventValueRecognizer)
			_deprecatedEventValueRecognizer->createEventValues(docTheory);
		else if(_patternEventValueRecognizer)
			_patternEventValueRecognizer->createEventValues(docTheory);
	
	_valuePromoter->promoteValues(docTheory);

	if (_use_correct_answers) {
		correctAnswers->assignCorrectTimexNormalizations(docTheory);
		return;
	}
	_temporalNormalizer->normalizeTimexValues(docTheory);
}
