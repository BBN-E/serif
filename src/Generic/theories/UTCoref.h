// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

/* This class represents a theory about coreference (untyped) for a
 * single document.  It consists of a list of entities. Each entity is
 * just a list of pointers to nodes in parse trees in this document
   that are all coreferent.
 */

#ifndef UT_COREF_H
#define UT_COREF_H

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

#include <iostream>
#include "Generic/theories/Theory.h"
#include "Generic/common/GrowableArray.h"

#define UT_COREF_VERSION 0.1

class SynNode;
class StateLoader;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif


class SERIF_EXPORTED UTCoref : public Theory {
private:
   /* A set of entities. Each entity is a set of mentions, which
    * are just parse nodes
    */
   GrowableArray< GrowableArray<const SynNode*> * > entities;

   GrowableArray<const SynNode*> *getEntityFor(const SynNode &mention) const;

   void addEntity(GrowableArray<const SynNode*> *entity);

public:

   UTCoref();
   ~UTCoref();

   bool areLinked(const SynNode &leftMention, const SynNode &rightMention) const;
   void addLink(const SynNode &leftMention, const SynNode &rightMention);

   UTCoref(StateLoader *stateLoader);
   void updateObjectIDTable() const;
   void saveState(StateSaver *stateSaver) const;
   void loadState(StateLoader *stateLoader);
   void resolvePointers(StateLoader *stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit UTCoref(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;

   const GrowableArray< GrowableArray<const SynNode*> * >* getEntities();
};

#endif
