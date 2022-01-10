// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/theories/RelationSet.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/EntitySet.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

#include <boost/foreach.hpp>

using namespace std;

RelationSet::RelationSet(int n_relations)
	: _n_relations(n_relations)
{
	if (n_relations == 0) {
		_relations = 0;
	}
	else {
		_relations = _new Relation*[n_relations];
		for (int i = 0; i < n_relations; i++)
			_relations[i] = 0;
	}
}

RelationSet::RelationSet(std::vector<RelationSet*> splitRelationSets, std::vector<int> entityOffsets, std::vector<int> sentenceOffsets, std::vector<RelMentionSet*> mergedRelMentionSets) {
	_n_relations = 0;
	BOOST_FOREACH(RelationSet* splitRelationSet, splitRelationSets) {
		if (splitRelationSet != NULL)
			_n_relations += splitRelationSet->getNRelations();
	}
	if (_n_relations > 0)
		_relations = _new Relation*[_n_relations];
	else
		_relations = NULL;

	int relation_set_offset = 0;
	for (size_t rs = 0; rs < splitRelationSets.size(); rs++) {
		RelationSet* splitRelationSet = splitRelationSets[rs];
		if (splitRelationSet != NULL) {
			for (int r = 0; r < splitRelationSet->getNRelations(); r++) {
				_relations[relation_set_offset + r] = _new Relation(*(splitRelationSet->getRelation(r)), relation_set_offset, entityOffsets[rs], sentenceOffsets[rs], mergedRelMentionSets);
			}
			relation_set_offset += splitRelationSet->getNRelations();
		}
	}
}


RelationSet::~RelationSet() {
	for (int i = 0; i < _n_relations; i++)
		delete _relations[i];
	delete[] _relations;
}

void RelationSet::takeRelation(int i, Relation *relation) {
	if ((unsigned) i < (unsigned) _n_relations) {
		delete _relations[i];
		_relations[i] = relation;
	}
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"RelationSet::takeRelation()", _n_relations, i);
	}
}

Relation *RelationSet::getRelation(int i) const {
	if ((unsigned) i < (unsigned) _n_relations)
		return _relations[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"RelationSet::getRelation()", _n_relations, i);
}

void RelationSet::dump(ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Relation Set:";

	if (_n_relations == 0) {
		out << newline << "  (no relations)";
	}
	else {
		int i;
		for (i = 0; i < _n_relations; i++) {
			out << newline << "- ";
			_relations[i]->dump(out, indent + 2);
		}
	}

	delete[] newline;
}

void RelationSet::updateObjectIDTable() const {

	ObjectIDTable::addObject(this);
	for (int i = 0; i < _n_relations; i++)
		_relations[i]->updateObjectIDTable();

}

void RelationSet::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"RelationSet", this);

	stateSaver->saveInteger(_n_relations);
	stateSaver->beginList(L"RelationSet::_relations");
	for (int i = 0; i < _n_relations; i++)
		_relations[i]->saveState(stateSaver);
	stateSaver->endList();

	stateSaver->endList();
}

RelationSet::RelationSet(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"RelationSet");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	_n_relations = stateLoader->loadInteger();
	_relations = _new Relation*[_n_relations];
	stateLoader->beginList(L"RelationSet::_relations");
	for (int i = 0; i < _n_relations; i++)
		_relations[i] = _new Relation(stateLoader);
	stateLoader->endList();

	stateLoader->endList();
}

void RelationSet::resolvePointers(StateLoader * stateLoader) {
	for (int i = 0; i < _n_relations; i++)
		_relations[i]->resolvePointers(stateLoader);
}

const wchar_t* RelationSet::XMLIdentifierPrefix() const {
	return L"relset";
}

void RelationSet::saveXML(SerifXML::XMLTheoryElement relationsetElem, const Theory *context) const {
	using namespace SerifXML;
	const EntitySet *entitySet = dynamic_cast<const EntitySet*>(context);
	if (context == 0)
		throw InternalInconsistencyException("RelationSet::saveXML", "Expected context to be an EntitySet");
	for (int i = 0; i < _n_relations; i++) {
		relationsetElem.saveChildTheory(X_Relation, _relations[i], entitySet);
		if (_relations[i]->getID() != i)
			throw InternalInconsistencyException("RelationSet::saveXML", 
				"Unexpected RelationSet::_ID value");
	}
}

RelationSet::RelationSet(SerifXML::XMLTheoryElement relationSetElem)
{
	using namespace SerifXML;
	relationSetElem.loadId(this);

	XMLTheoryElementList rmElems = relationSetElem.getChildElementsByTagName(X_Relation);
	_n_relations = static_cast<int>(rmElems.size());
	_relations = _new Relation*[_n_relations];
	for (int i=0; i<_n_relations; ++i)
		_relations[i] = _new Relation(rmElems[i], i);
}
