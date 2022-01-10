// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_OBSERVATION_H
#define UT_OBSERVATION_H

#include "Generic/discTagger/DTObservation.h"

#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "LinkAllMentions.h"

class UTObservation : public DTObservation {

private:
   static const Symbol className;

   const SynNode *leftNode;
   const SynNode *rightNode;
   LinkAllMentions *lam;

public:
   UTObservation(LinkAllMentions *lam);
   ~UTObservation();
   virtual DTObservation *makeCopy();
   void resetForNewSentence();
   void populate(const SynNode &leftNode, const SynNode &rightNode);
   const SynNode &getLeftNode() { return *leftNode; }
   const SynNode &getRightNode() { return *rightNode; }
   LinkAllMentions &getLam() { return *lam; }
};

#endif
