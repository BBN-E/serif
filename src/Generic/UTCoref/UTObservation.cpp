// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "UTObservation.h"

const Symbol UTObservation::className(L"utcoref");
UTObservation::UTObservation(LinkAllMentions *lam) :
   DTObservation(className), lam(lam), leftNode(0), rightNode(0)
{

}

void UTObservation::populate(const SynNode &leftNode, const SynNode &rightNode) {
   this->leftNode = &leftNode;
   this->rightNode = &rightNode;
}


UTObservation::~UTObservation() {}

void resetForNewSentence() {}

DTObservation *UTObservation::makeCopy() {
   UTObservation* o = _new UTObservation(lam);
   o->populate(*leftNode, *rightNode);
   return o;
}
