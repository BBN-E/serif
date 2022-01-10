// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATIONSET_H
#define RELATIONSET_H

#include <iostream>

#include "Generic/theories/Theory.h"

class Relation;
class RelMentionSet;

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED RelationSet : public Theory {
public:
	RelationSet(int n_relations);
	RelationSet(std::vector<RelationSet*> splitRelationSets, std::vector<int> entityOffsets, std::vector<int> sentenceOffsets, std::vector<RelMentionSet*> mergedRelMentionSets);
	~RelationSet();

	/** Add new relation to set. As the "take" here signifies, there
	  * is a transfer of ownership, meaning that it is now the
	  * RelationSet's responsibility to delete the Relation */
	void takeRelation(int i, Relation *relation);

	int getNRelations() const { return _n_relations; }
	Relation *getRelation(int i) const;

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out,
									 const RelationSet &it)
		{ it.dump(out, 0); return out; }

	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	RelationSet(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit RelationSet(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;

protected:
	int _n_relations;
	Relation **_relations;

};

#endif
