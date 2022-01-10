#ifndef XX_POTENTIAL_RELATION_COLLECTOR_H
#define XX_POTENTIAL_RELATION_COLLECTOR_H

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/relations/PotentialRelationCollector.h"


class DocTheory;
class PotentialTrainingRelation;
class PotentialRelationInstance;

class GenericPotentialRelationCollector : public PotentialRelationCollector {
private:
	friend class GenericPotentialRelationCollectorFactory;

public:

	virtual void loadDocTheory(DocTheory *docTheory) {};

	virtual void produceOutput(const wchar_t *output_dir, const wchar_t *document_filename) {};
	virtual void produceOutput(std::wstring *results) { PotentialRelationCollector::produceOutput(results); }
	virtual void outputPacketFile(const char *output_dir, const char *packet_name) {};

	virtual int getNRelations() { return 0; }
	virtual PotentialTrainingRelation* getPotentialTrainingRelation(int i) { return 0; }
	virtual PotentialRelationInstance* getPotentialRelationInstance(int i) { return 0; } 
	
private:
	GenericPotentialRelationCollector() {}

};

class GenericPotentialRelationCollectorFactory: public PotentialRelationCollector::Factory {
	virtual PotentialRelationCollector *build(int collectionMode) { return _new GenericPotentialRelationCollector(); }
	virtual PotentialRelationCollector *build(int collectionMode, RelationTypeMap *relationTypes) { return _new GenericPotentialRelationCollector(); }
};


#endif
