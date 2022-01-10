// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/Relation.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"
#include <boost/foreach.hpp>

Relation::Relation(Relation &other, int set_offset, int entity_offset, int sent_offset, std::vector<RelMentionSet*> mergedRelMentionSets) {
	_ID = other._ID + set_offset;
	_left_entity_ID = other._left_entity_ID + entity_offset;
	_right_entity_ID = other._right_entity_ID + entity_offset;
	_type = other._type;
	_modality = other._modality;
	_tense = other._tense;
	_filters = other._filters;
	const LinkedRelMention *ments = other.getMentions();
	if (ments != 0) {
		RelMentionUID rmUID = ments->relMention->getUID();
		_relMentions = _new LinkedRelMention(mergedRelMentionSets[rmUID.sentno() + sent_offset]->getRelMention(rmUID.index()));
		ments = ments->next;
		LinkedRelMention *iter = _relMentions;
		while (ments != 0) {
			RelMentionUID rmUID = ments->relMention->getUID();
			iter->next = _new LinkedRelMention(mergedRelMentionSets[rmUID.sentno() + sent_offset]->getRelMention(rmUID.index()));
			iter = iter->next;
			ments = ments->next;
		}
	}
}		

Symbol Relation::getRawType() const {
	if (_type != RelationConstants::RAW) {
		throw InternalInconsistencyException("Relation::getRawType()",
			"Attempt to get raw relation string of cooked relation.");
	}

	return _relMentions->relMention->getRawType();
}

std::wstring Relation::toString() const {
	std::wstringstream wss;
	wss << _type.to_string();
	wss << L": e";
	wss << _left_entity_ID;
	wss << L" & e";
	wss << _right_entity_ID;
	return wss.str();
}
std::string Relation::toDebugString() const {
	std::stringstream ss;
	ss << _type.to_debug_string();
	ss << ": e";
	ss << _left_entity_ID;
	ss << " & e";
	ss << _right_entity_ID;
	return ss.str();
}

void Relation::dump(UTF8OutputStream &out, int indent) const {
	out << L"Relation " << _ID << L": ";
	out << this->toString();
	LinkedRelMention *mentionIter = _relMentions;
	while (mentionIter != 0) {
		out << L"\n" << L"- ";
		mentionIter->relMention->dump(out, indent+2);
		mentionIter = mentionIter->next;
	}
}
void Relation::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);
	out << "Relation " << _ID << ": ";
	out << this->toDebugString();
	LinkedRelMention *mentionIter = _relMentions;
	while (mentionIter != 0) {
		out << newline << "- ";
		mentionIter->relMention->dump(out, indent+2);
		mentionIter = mentionIter->next;
	}
}

void Relation::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);
}

void Relation::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"Relation", this);

	stateSaver->saveInteger(_ID);
	stateSaver->saveSymbol(_type);
	stateSaver->saveInteger(_left_entity_ID);
	stateSaver->saveInteger(_right_entity_ID);
	stateSaver->saveInteger(_modality.toInt());
	stateSaver->saveInteger(_tense.toInt());

	LinkedRelMention *mentionIter = _relMentions;
	int count = 0;
	while (mentionIter != 0) {
		count++;
		mentionIter = mentionIter->next;
	}

	stateSaver->saveInteger(count);
	stateSaver->beginList(L"Relation::_relMentions");
	mentionIter = _relMentions;
	while (mentionIter != 0) {
		stateSaver->savePointer(mentionIter->relMention);
		mentionIter = mentionIter->next;
	}
	stateSaver->endList();

	stateSaver->endList();

}
// For loading state:
Relation::Relation(StateLoader *stateLoader) {

	int id = stateLoader->beginList(L"Relation");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	_ID = stateLoader->loadInteger();
	_type = stateLoader->loadSymbol();
	_left_entity_ID = stateLoader->loadInteger();
	_right_entity_ID = stateLoader->loadInteger();
	_modality = ModalityAttribute::getFromInt(stateLoader->loadInteger());
	_tense = TenseAttribute::getFromInt(stateLoader->loadInteger());

	int count = stateLoader->loadInteger();
	stateLoader->beginList(L"Relation::_relMentions");
	for (int i = 0; i < count; i++) {
		RelMention *rm = (RelMention *) stateLoader->loadPointer();
		if (i == 0)
			_relMentions = _new LinkedRelMention(rm);
		else addRelMention(rm);
	}
	stateLoader->endList();

	stateLoader->endList();

}
void Relation::resolvePointers(StateLoader * stateLoader) {
	LinkedRelMention *mentionIter = _relMentions;
	while (mentionIter != 0) {
		mentionIter->relMention = (RelMention *) stateLoader->getObjectPointerTable().getPointer(mentionIter->relMention);
		mentionIter = mentionIter->next;
	}
		
}

void
Relation::applyFilter(const std::string& filterName, RelationClutterFilter *filter)
{
	double score (0.);
	if (filter->filtered(this, &score)) {
		_filters[filterName] = score;
	}
	else {
		_filters.erase(filterName);
	}
}

bool
Relation::isFiltered(const std::string& filterName) const
{
	std::map<std::string, double>::const_iterator filter (_filters.find(filterName));
	return filter != _filters.end();
}

double
Relation::getFilterScore(const std::string& filterName) const
{
	double score (0.);
	std::map<std::string, double>::const_iterator filter (_filters.find(filterName));
	if (filter != _filters.end()) {
		score = filter->second;
	}
	return score;
}

float Relation::getConfidenceScore() const {
	float result = 0;
	for (const LinkedRelMention *lrm=_relMentions; lrm != 0; lrm = lrm->next) {
		result = std::max(lrm->relMention->getScore(), result);
	}
	return result;
}

const wchar_t* Relation::XMLIdentifierPrefix() const {
	return L"rel";
}

void Relation::saveXML(SerifXML::XMLTheoryElement relationElem, const Theory *context) const {
	using namespace SerifXML;
	const EntitySet *entitySet = dynamic_cast<const EntitySet*>(context);
	if (context == 0)
		throw InternalInconsistencyException("Relation::saveXML", "Expected context to be an EntitySet");
	// Attributes of the Relation
	relationElem.setAttribute(X_type, _type);
	//relationElem.setAttribute(X_rawType, getRawType());
	relationElem.setAttribute(X_tense, _tense.toString());
	relationElem.setAttribute(X_modality, _modality.toString());
	relationElem.setAttribute(X_confidence, getConfidenceScore());
	// Relmentions
	std::vector<const Theory*> relMentionList;
	for (const LinkedRelMention *lrm=_relMentions; lrm != 0; lrm = lrm->next) {
		relMentionList.push_back(lrm->relMention);
		if (relationElem.getOptions().include_mentions_as_comments)
			relationElem.addComment(lrm->relMention->toString());
	}
	relationElem.saveTheoryPointerList(X_rel_mention_ids, relMentionList);
	// Left & right enities
	Entity* left = entitySet->getEntity(_left_entity_ID);
	Entity* right = entitySet->getEntity(_right_entity_ID);
	relationElem.saveTheoryPointer(X_left_entity_id, left);
	relationElem.saveTheoryPointer(X_right_entity_id, right);
}

Relation::Relation(SerifXML::XMLTheoryElement relationElem, int relation_id)
: _modality(Modality::ASSERTED), 
  _tense(Tense::UNSPECIFIED) ,
  _filters(), _relMentions(0)
{
	using namespace SerifXML;
	relationElem.loadId(this);
	_type = relationElem.getAttribute<Symbol>(X_type);
	_ID = relation_id;
	_tense = TenseAttribute::getFromString(relationElem.getAttribute<std::wstring>(X_tense).c_str());
	_modality = ModalityAttribute::getFromString(relationElem.getAttribute<std::wstring>(X_modality).c_str());

	// Relmentions
	std::vector<const RelMention*> relMentionList = relationElem.loadTheoryPointerList<RelMention>(X_rel_mention_ids);
	for (size_t i=0; i<relMentionList.size(); ++i) {
		RelMention* relMention = const_cast<RelMention*>(relMentionList[i]);
		if (i == 0)
			_relMentions = _new LinkedRelMention(relMention);
		else
			addRelMention(relMention);
	}

	// Left & right entities
	_left_entity_ID = relationElem.loadTheoryPointer<Entity>(X_left_entity_id)->getID();
	_right_entity_ID = relationElem.loadTheoryPointer<Entity>(X_right_entity_id)->getID();

}
