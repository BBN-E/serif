// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef MAXENT_CLASSIFIER_H
#define MAXENT_CLASSIFIER_H

#include <string>

#include "AbstractClassifier.h"

class DTObservation;
class DTFeatureTypeSet;
class DTTagSet;

class MaxentClassifier : public AbstractClassifier {
private:
   DTFeature::FeatureWeightMap *weights;
   MaxEntModel *maxent;
   std::string model_file_name;

   void saveWeights();

public:
   MaxentClassifier(DTObservation *obs, DTTagSet *tagSet, DTFeatureTypeSet *featureTypes);
   ~MaxentClassifier();
   
   void startTraining(const char* model_file_name);
   void trainOnExample(bool is_positive);
   void finishTraining();

   void startClassifying(const char* model_file_name, Symbol model_type);
   double classifyExample();

};

#endif
