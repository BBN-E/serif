// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"

#include "Spanish/descriptors/es_DescriptorClassifier.h"
#include "Generic/descriptors/discmodel/P1DescriptorClassifier.h"

SpanishDescriptorClassifier::SpanishDescriptorClassifier()
{

	_p1DescriptorClassifier = 0;
	std::string model_type = ParamReader::getRequiredParam("desc_classify_model_type");
	if (model_type == "P1") {
		_classifier_type = P1;
		_p1DescriptorClassifier = _new P1DescriptorClassifier();
	} else {
		std::string error = model_type + "not a valid relation model type: use PM or P1";
		throw UnexpectedInputException("SpanishDescriptorClassifier::SpanishDescriptorClassifier()", error.c_str());
	}
}

SpanishDescriptorClassifier::~SpanishDescriptorClassifier() {
	delete _p1DescriptorClassifier;
}

int SpanishDescriptorClassifier::classifyDescriptor(MentionSet *currSolution, const SynNode* node,
											 EntityType types[], double scores[], int max_results)
{
	if (_classifier_type == P1) {
        return _p1DescriptorClassifier->classifyDescriptor(currSolution, node,
			types, scores, max_results);
	}

	return 0;
}
