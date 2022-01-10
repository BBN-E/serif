// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/RelMention.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"
#include <algorithm>

#include <boost/foreach.hpp>

using namespace std;

RelMentionSet::RelMentionSet(const RelMentionSet &other, int sent_offset, const MentionSet* mentionSet, const ValueMentionSet* valueMentionSet)
: _score(other._score)
{
	BOOST_FOREACH(RelMention* otherRelMention, other._rmentions) {
		_rmentions.push_back(_new RelMention(*otherRelMention, sent_offset, mentionSet, valueMentionSet));
	}
}

RelMentionSet::RelMentionSet(std::vector<RelMentionSet*> splitRelMentionSets, std::vector<int> sentenceOffsets, std::vector<MentionSet*> mergedMentionSets)
{
	// Take the highest set score when merging
	_score = 0.0;
	for (size_t rms = 0; rms < splitRelMentionSets.size(); rms++) {
		RelMentionSet* splitRelMentionSet = splitRelMentionSets[rms];
		if (splitRelMentionSet != NULL) {
			if (splitRelMentionSet->_score > _score)
				_score = splitRelMentionSet->_score;

			// Do the cross-sentence mention lookup here instead of implementing a different RelMention deep copier
			for (int rm = 0; rm < splitRelMentionSet->getNRelMentions(); rm++) {
				RelMention* splitRelMention = splitRelMentionSet->getRelMention(rm);
				MentionUID lhUID = MentionUID(splitRelMention->getLeftMention()->getUID().sentno() + sentenceOffsets[rms], splitRelMention->getLeftMention()->getUID().index());
				Mention* lhs = mergedMentionSets[lhUID.sentno()]->getMention(lhUID);
				MentionUID rhUID = MentionUID(splitRelMention->getRightMention()->getUID().sentno() + sentenceOffsets[rms], splitRelMention->getRightMention()->getUID().index());
				Mention* rhs = mergedMentionSets[lhUID.sentno()]->getMention(rhUID);
				_rmentions.push_back(_new RelMention(lhs, rhs, splitRelMention->getType(), static_cast<int>(mergedMentionSets.size()), static_cast<int>(_rmentions.size()), splitRelMention->getScore()));
			}
		}
	}
}

RelMentionSet::~RelMentionSet() {
	for (size_t i = 0; i < _rmentions.size(); i++)
		delete _rmentions.at(i);
	_rmentions.clear();
}

void RelMentionSet::takeRelMention(RelMention *rm) {
	// Update the UID to correspond predictably to the sentno/relno.
	int sentno = rm->getUID().sentno();
	int index = static_cast<int>(_rmentions.size());

    const Mention* left = rm->getLeftMention();
    const Mention* right = rm->getRightMention();
    Symbol type = rm->getType();
    Symbol time = rm->getTimeRole();

    //Check whether the relMention already exists
    //(We had some problems with duplicates when we used RelationPatternFinder)
    //If speed matters, we should get from a hash_set() instead of looping through the vector
    for (int i=static_cast<int>(_rmentions.size())-1; i>=0; i--){
      RelMention* oldRM = getRelMention(i);
      if (oldRM->getLeftMention()==left){
        if (oldRM->getRightMention()==right){
          if (oldRM->getType()==type){
            if (oldRM->getTimeRole()==time){
              //We have a match
              //Replace the old entry if the new one has a higher score
              if (rm->getScore() > oldRM->getScore()){
                _score -= oldRM->getScore();
                _score += rm->getScore();
                _rmentions.erase(_rmentions.begin() + i);
                rm->setUID(sentno, index);
                _rmentions.push_back(rm);
              }
              //Stop at the first match
              return;
            }
          }
        }
      }
    }
    //If the relMention does not exist yet, add it.
    rm->setUID(sentno, index);
    _score += rm->getScore();
    _rmentions.push_back(rm);
}

void RelMentionSet::takeRelMentions(RelMentionSet *relmentionset) {
	for (int i = 0; i < relmentionset->getNRelMentions(); i++) {
		takeRelMention(relmentionset->getRelMention(i));
	}
	relmentionset->clear();
}

RelMention *RelMentionSet::getRelMention(int i) const {
	if ((unsigned) i < (unsigned) _rmentions.size())
		return _rmentions.at(i);
	else
		throw InternalInconsistencyException::arrayIndexException(
			"RelMentionSet::getRelMention()", _rmentions.size(), i);
}

RelMention *RelMentionSet::findRelMentionByUID(RelMentionUID uid) const {
	BOOST_FOREACH(RelMention *rm, _rmentions) {
		if (rm->getUID() == uid)
			return rm;
	}
	return 0;
}

void RelMentionSet::dump(ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Relation Mention Set:";

	if (_rmentions.size() == 0) {
		out << newline << "  (no relations)";
	}
	else {
		BOOST_FOREACH(RelMention *rm, _rmentions) {
			out << newline << "- ";
			rm->dump(out, indent + 2);
		}
	}

	delete[] newline;
}

void RelMentionSet::updateObjectIDTable() const {

	ObjectIDTable::addObject(this);
	BOOST_FOREACH(RelMention *rm, _rmentions) {
		rm->updateObjectIDTable();
	}

}

void RelMentionSet::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"RelMentionSet", this);

	stateSaver->saveInteger(_rmentions.size());
	stateSaver->beginList(L"RelMentionSet::_relations"); // has to be _relations rather than _rmentions for backwards-compatibility
	BOOST_FOREACH(RelMention *rm, _rmentions) { 
		rm->saveState(stateSaver);
	}
	stateSaver->endList();

	stateSaver->endList();
}
RelMentionSet::RelMentionSet(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"RelMentionSet");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	int n_rmentions = stateLoader->loadInteger();
	stateLoader->beginList(L"RelMentionSet::_relations"); // has to be _relations rather than _rmentions for backwards-compatibility
	for (int i = 0; i < n_rmentions; i++) {
		RelMention *rm = _new RelMention(0,0,Symbol(),0,0);
		rm->loadState(stateLoader);
		_rmentions.push_back(rm);
	}
	stateLoader->endList();

	stateLoader->endList();
}

void RelMentionSet::resolvePointers(StateLoader * stateLoader) {
	BOOST_FOREACH(RelMention *rm, _rmentions) { 
		rm->resolvePointers(stateLoader);
	}
}

const wchar_t* RelMentionSet::XMLIdentifierPrefix() const {
	return L"rmset";
}

namespace {
	inline bool relmention_id_less_than(const RelMention* lhs, const RelMention* rhs) {
		return lhs->getUID() < rhs->getUID(); 
	}
}

void RelMentionSet::saveXML(SerifXML::XMLTheoryElement relmentionsetElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("RelMentionSet::saveXML", "Expected context to be NULL");
	relmentionsetElem.setAttribute(X_score, _score);

	// Sort the relmentions by id (so we don't need to explicitly record the id).
	std::vector<RelMention*> sorted_rmentions(_rmentions);
	std::sort(sorted_rmentions.begin(), sorted_rmentions.end(), relmention_id_less_than);
	
	for (int i = 0; i < (int) sorted_rmentions.size(); ++i) {
		relmentionsetElem.saveChildTheory(X_RelMention, sorted_rmentions.at(i));
		if ((sorted_rmentions.at(i)->getUID().index()) != i)
			throw InternalInconsistencyException("RelMentionSet::saveXML", 
				"Unexpected RelMention UID value");
	}
}

RelMentionSet::RelMentionSet(SerifXML::XMLTheoryElement relMentionSetElem, int sentno)
: _score(0)
{
	using namespace SerifXML;
	relMentionSetElem.loadId(this);
	_score = relMentionSetElem.getAttribute<float>(X_score, 0);

	XMLTheoryElementList rmElems = relMentionSetElem.getChildElementsByTagName(X_RelMention);
	int n_rmentions = static_cast<int>(rmElems.size());
	_rmentions.resize(n_rmentions);
	for (int i=0; i<n_rmentions; ++i) 
		_rmentions.at(i) = _new RelMention(rmElems[i], sentno, i);
}
