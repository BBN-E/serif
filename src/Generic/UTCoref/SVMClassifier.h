// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef SVM_CLASSIFIER_H
#define SVM_CLASSIFIER_H

#include <string>

#include "AbstractClassifier.h"

class DTObservation;
class DTFeatureTypeSet;
class DTTagSet;

class SVMClassifier : public AbstractClassifier {
private:
   UTF8OutputStream *out;
   void constructFeatureVector(std::wstring &featureVector);

public:
	SVMClassifier(DTObservation *obs, DTTagSet *tagSet, DTFeatureTypeSet *featureTypes);
   ~SVMClassifier();

   void startTraining(const char* model_file_name);
   void trainOnExample(bool is_positive);
   void finishTraining();

   void startClassifying(const char* model_file_name, Symbol model_type);
   double classifyExample();
};

#endif
