// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DT_RELATION_SET_H
#define DT_RELATION_SET_H

#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/RelMention.h"

/** matrix of relation names for each pair of mention indexes (NONE if no relation is present for the pair)
  */

class DTRelationSet {
public:
	DTRelationSet(int n, const RelMentionSet *rmSet, Symbol noneSymbol)
		: _n_entity_mentions(n), NONE(noneSymbol)
	{
		_relations = _new Symbol*[_n_entity_mentions];
		for (int i = 0; i < _n_entity_mentions; i++) {
			_relations[i] = _new Symbol[_n_entity_mentions];
			for (int j = 0; j < _n_entity_mentions; j++) {
				_relations[i][j] = NONE;
			}
		}
		for (int k = 0; k < rmSet->getNRelMentions(); k++) {
			RelMention *rm = rmSet->getRelMention(k);
			int left = rm->getLeftMention()->getIndex();
			int right = rm->getRightMention()->getIndex();
			_relations[left][right] = rm->getType();
		}
	}

	Symbol getRelation(int i, int j) const {
		if (i < 0 || i >= _n_entity_mentions)
			throw InternalInconsistencyException::arrayIndexException(
				"DTRelationSet::getRelation()", _n_entity_mentions, i);
		if (j < 0 || j >= _n_entity_mentions)
			throw InternalInconsistencyException::arrayIndexException(
				"DTRelationSet::getRelation()", _n_entity_mentions, j);
		// we don't know which way it's stored
		if (_relations[i][j] != NONE)
			return _relations[i][j];
		else return _relations[j][i];
	}

	bool hasReversedRelation(int i, int j) const {
		if (_relations[i][j] != NONE)
			return false;
		else if (_relations[j][i] != NONE)
			return true;
		else return false;
	}

	Symbol getNoneSymbol() const {
		return NONE;
	}

private:
	Symbol **_relations;
	int _n_entity_mentions;
	Symbol NONE;

};

#endif
