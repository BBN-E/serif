// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOC_VALUE_PROCESSOR_H
#define DOC_VALUE_PROCESSOR_H

class DocTheory;
class ValuePromoter;
class DeprecatedEventValueRecognizer;
class PatternEventValueRecognizer;
class TemporalNormalizer;

class DocValueProcessor {
public:
	DocValueProcessor();
	~DocValueProcessor();

	void doDocValues(DocTheory *docTheory);

private:
	ValuePromoter *_valuePromoter;
	DeprecatedEventValueRecognizer *_deprecatedEventValueRecognizer;
	PatternEventValueRecognizer *_patternEventValueRecognizer;
	TemporalNormalizer *_temporalNormalizer;

	bool _use_preexisting_event_values;

	bool _use_correct_answers;
};

#endif
