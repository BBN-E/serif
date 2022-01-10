// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_TYPE_H
#define UT_FEATURE_TYPE_H

/*

UTCoref Feature Detection
-------------------------

All UTCoref Feature Types produce bigram features, and they always
produce exactly one bigram feature on each invocation.  This
superclass deals with extracting the relevant information from the
observation and then calling detect (which the subclass has overriden).

Binary Features should return the Symbols True and False.

Adding a feature
~~~~~~~~~~~~~~~~
A UTCoref Feature FooFeature has to do three things:
 - extend UTFeatureType
 - pass it's name to UTFeatureType during construction
 - redefine "Symbol detect(&leftNode, &rightNode, &lam)"
   - this means look at the two nodes in context and produce some symbol

For each new feature, other files also need modification:
 - UTFeatureType.cpp needs:
   - include FooFeature.h
   - _new FooFeature()
   - this deals with registering the features
 - CMakeLists needs FooFeature.h

*/



#include <string>
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/common/Symbol.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/UTCoref/UTObservation.h"

class UTFeatureType : public DTFeatureType {
public:
   static Symbol modeltype;
   static bool registered;

   static Symbol True;
   static Symbol False;
   static Symbol Unknown;
   static Symbol NotImplemented;
   static Symbol None;

   virtual DTFeature *makeEmptyFeature() const {
	  return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
   }
   static void initFeatures();
   UTFeatureType(Symbol name) : DTFeatureType(modeltype, name, InfoSource::OBSERVATION) {}

   virtual Symbol detect(const SynNode &leftNode, const SynNode &rightNode, const LinkAllMentions &lam) const = 0;

   virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const {

	  UTObservation *o = static_cast<UTObservation*>(state.getObservation(0));
	  const SynNode &leftNode = o->getLeftNode();
	  const SynNode &rightNode = o->getRightNode();
	  const LinkAllMentions &lam = o->getLam();

	  resultArray[0] = _new DTBigramFeature(this, state.getTag(), detect(leftNode, rightNode, lam));

	  return 1;
   }
};

#endif
