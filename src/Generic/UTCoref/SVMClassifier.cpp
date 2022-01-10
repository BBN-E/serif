// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "SVMClassifier.h"

// for debugging
#include "UTObservation.h"
#include "LinkAllMentions.h"

SVMClassifier::SVMClassifier(DTObservation * obs, DTTagSet *tagSet, DTFeatureTypeSet *featureTypes) :
   AbstractClassifier(obs,tagSet,featureTypes), out(0)
{

}

SVMClassifier::~SVMClassifier() {
   if (out != 0) {
	  delete out;
   }
}

void SVMClassifier::startTraining(const char* model_file_name) {
   out = _new UTF8OutputStream();
   out->open(model_file_name);

   if (out->fail()) {
	  throw UnexpectedInputException("SVMClassifier::startTraining",
									 "examples file won't open");
   }
}

void SVMClassifier::startClassifying(const char* model_file_name, Symbol model_type) {
   SessionLogger::info("SERIF") << "SVMClassifier: Would load model_file" << std::endl;
}

void SVMClassifier::constructFeatureVector(std::wstring &featureVector) {
   featureVector.clear();

   // for debugging
   {
	  UTObservation* uobs = dynamic_cast<UTObservation*>(obs);
	  LinkAllMentions &lam = uobs->getLam();
	  const SynNode &left_node = uobs->getLeftNode();
	  const SynNode &right_node = uobs->getRightNode();

	  std::wstringstream wss;
	  wss << L"X ";
	  wss << lam.lookupParseNumber(right_node, *lam.parses);
	  wss << L":";
	  wss << lam.lookupNodeNumber(right_node);
	  wss << L" ";
	  wss << lam.lookupParseNumber(left_node, *lam.parses);
	  wss << L":";
	  wss << lam.lookupNodeNumber(left_node);
	  wss << L" ";

	  featureVector += wss.str();
   }

   if (featureVector.find(L'\0') != featureVector.npos) {
	  throw UnexpectedInputException("SVMClassifier::trainOnExample",
                                     "metadata null");
   }

   for (int i = 0; i < featureTypes->getNFeaturesTypes(); i++) {
	  if (i != 0) {
		 featureVector += L' '; /* space separated list */
	  }

	  DTState state(Yes /* doesn't matter; will be ignored later */, Symbol(), Symbol(), 0, std::vector<DTObservation*>(1, obs));
	  int n_features = featureTypes->getFeatureType(i)->extractFeatures(state, featureBuffer);
	  if (n_features != 1) {
		 throw UnexpectedInputException("LinkAllMentions::constructFeatureVector",
										"a feature type returned multiple feature values");
	  }

	  featureVector += (static_cast<DTBigramFeature*>(featureBuffer[0]))->getNonTag().to_string();

	  if (featureVector.find(L'\0') != featureVector.npos) {
		 SessionLogger::info("SERIF") << i << std::endl;
		 throw UnexpectedInputException("SVMClassifier::trainOnExample",
										"null at feature");
	  }
   }
}

double SVMClassifier::classifyExample() {
   std::wstring featureVector;
   constructFeatureVector(featureVector);
   SessionLogger::info("SERIF") << "SVMClassifier: Would classify example" << std::endl;
   return 1.0; /* this should actually invoke the classifier */
}

void SVMClassifier::trainOnExample(bool is_positive) {

   if (out == 0) {
	  throw UnexpectedInputException("SVMClassifier::trainOnExample",
									 "call startTraining before trainOnExample");
   }

   std::wstring featureVector;
   constructFeatureVector(featureVector);

   if (featureVector.find(L'\0') != featureVector.npos) {
	  throw UnexpectedInputException("SVMClassifier::trainOnExample",
                                     "constructFeatureVector gave us a null");
   }

   out->write(featureVector.c_str(), featureVector.size());
   out->write(L" ", 1);

   const wchar_t* is_positive_str = is_positive ? Yes.to_string() : No.to_string();
   out->write(is_positive_str, wcslen(is_positive_str));
   out->write(L"\n", 1);
}

void SVMClassifier::finishTraining() {
   if (out != 0) {
	  out->close();
   }
}

