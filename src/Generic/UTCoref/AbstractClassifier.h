// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.


#ifndef ABSTRACT_CLASSIFIER_H
#define ABSTRACT_CLASSIFIER_H

/*

This class is for wrapping an SVM or MaxEnt classifier so the
particulars of using it are not something the caller has to worry
about.  See SVMClassifier and MaxentClassifier for implementation
details.

Using one of these requires calling a few functions in order:

For training:

 1. startTraining 
 2. for each example e, is_positive:
       obs.populate(e)
       trainOnExample(is_positive)
 3. finishTraining
 
For classifying:

 1. startClassifying model_file model_type
 2. for each example e:
       obs.populate e
      amt_positive = classifyExample

Populating the observation with information about the example is
indirect, but that's the way the interface works.

This only works for simple Yes/No classification.  The tagset file
should end up looking something like this:

  2
  A
  B

All featuretypes used with this classifier do not need to be
UTFeatureTypes but they do need to produce only DTBigramFeatures.
This is because for SVM we can only learn from a single symbol from
each featuretype; we don't work with bag of words features or
anything.  We also don't work with numeric features because the yamcha
system under the svm does treat them properly.  If you need a numeric
feature, bucket it. For example:

   GT_Z : x > 0
   Zero : x == 0
   LT_Z : x < 0

*/

class AbstractClassifier {

protected:
	DTObservation* obs;
   
	DTTagSet *tagSet;
	DTFeatureTypeSet *featureTypes;
   
	DTFeature *featureBuffer[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
   
	Symbol Yes, No;
	int yes, no;

public:

   /* we take responisibility for tagSet and featureTypes, and we delete them when we're deleted */
	AbstractClassifier(DTObservation* obs, DTTagSet *tagSet, DTFeatureTypeSet *featureTypes) : 
	  obs(obs), tagSet(tagSet), featureTypes(featureTypes) 
   {
	  std::cout << tagSet->getNTags() << std::endl;
	  if (tagSet->getNTags() != 3) {
		 throw UnexpectedInputException("AbstractClassifier::AbstractClassifier",
										"nTags is not 3: we expect the only choices to be yes and no.");
	  }
	  
	  yes = 1; Yes = tagSet->getTagSymbol(yes);
	  no = 2; No = tagSet->getTagSymbol(no);
   }

   virtual ~AbstractClassifier() {
	  delete tagSet;
	  delete featureTypes;
   }


   virtual void startTraining(const char* model_file_name) {};
   virtual void trainOnExample(bool is_positive) = 0;
   virtual void finishTraining() = 0;

   virtual void startClassifying(const char* model_file_name, Symbol model_type) {};
   virtual double classifyExample() = 0;


};


#endif
