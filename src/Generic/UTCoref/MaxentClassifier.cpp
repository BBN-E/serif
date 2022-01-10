// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "boost/foreach.hpp"

#include "Generic/common/ParamReader.h"
#include "Generic/maxent/MaxEntModel.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTFeature.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/common/UnexpectedInputException.h"

#include "MaxentClassifier.h"

MaxentClassifier::MaxentClassifier(DTObservation *obs, DTTagSet *tagSet, DTFeatureTypeSet *featureTypes) :
   AbstractClassifier(obs,tagSet,featureTypes), maxent(0), weights(0) {
   weights = _new DTFeature::FeatureWeightMap(50000);
}


MaxentClassifier::~MaxentClassifier() {   
   if (maxent != 0) {
	  delete maxent;
   }
   delete weights;
}

void MaxentClassifier::startTraining(const char* model_file_name) {
   this->model_file_name = model_file_name;
   maxent = _new MaxEntModel(tagSet, featureTypes, weights,
							 MaxEntModel::SCGIS,
							 ParamReader::getRequiredIntParam("utcoref_maxent_trainer_percent_held_out"),
							 ParamReader::getRequiredIntParam("utcoref_maxent_trainer_n_iterations"),
							 ParamReader::getRequiredIntParam("utcoref_maxent_trainer_gaussian_variance"),
							 ParamReader::getRequiredIntParam("utcoref_maxent_trainer_min_likelihood_delta"),
							 ParamReader::getRequiredIntParam("utcoref_maxent_trainer_stop_check_frequency"),
							 "", "");
}

void MaxentClassifier::startClassifying(const char* model_file_name, Symbol model_type) {
   DTFeature::readWeights(*weights, model_file_name, model_type);
   maxent = _new MaxEntModel(tagSet, featureTypes, weights);
}

void MaxentClassifier::saveWeights() {
   UTF8OutputStream out;
   out.open(model_file_name.c_str());
   if (out.fail()) {
      throw UnexpectedInputException("MaxentClassifier::finishTraining",
                                     "Could not open model file for writing");
   }

   for (DTFeature::FeatureWeightMap::iterator 
		   iter = weights->begin(); iter != weights->end(); ++iter)
   {
      DTFeature *feature = (*iter).first;
      out << L"((" << feature->getFeatureType()->getName().to_string() << L" ";
      feature->write(out);
      out << L") " << (*(*iter).second) << L")\n";
   }
}

void MaxentClassifier::finishTraining() {
   maxent->deriveModel(0);
   saveWeights();
}

/* observation should be populated first */
double MaxentClassifier::classifyExample() {
   if (maxent == 0) {
	  throw UnexpectedInputException("MaxentClassifier::classifyExample",
                                     "call startClassifying before classifyExample");
   }

   double scores[3];

   int best_tag;
   maxent->decodeToDistribution(obs, scores, 3, &best_tag);
   double probability_of_linking = scores[1];
   double probability_of_not_linking = scores[2];
   double how_positive = probability_of_linking - probability_of_not_linking;

   if (best_tag == 0) {
	  throw UnexpectedInputException("MaxentClassifier::classifyExample",
									 "best_tag is 0");
   }

   if ((best_tag == 1 && how_positive < 0) ||
	   (best_tag == 2 && how_positive > 0)) 
   {
	  throw UnexpectedInputException("MaxentClassifier::classifyExample",
									 "best_tag and how_positive disagree");
   }

   return how_positive;
}

void MaxentClassifier::trainOnExample(bool is_positive) {
   if (maxent == 0) {
	  throw UnexpectedInputException("MaxentClassifier::trainOnExample",
									 "call startTraining before trainOnExample");
   }

   maxent->addToTraining(obs, is_positive ? yes : no);
}
