// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "theories/Event.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Value.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/ParamReader.h"
#include "common/SessionLogger.h"

#include "state/StateSaver.h"
#include "state/StateLoader.h"
#include "state/ObjectIDTable.h"
#include "state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

Event::Event(Event &other)
{
	// does not copy consolidated relations!
	_ID = other._ID;
	_type = other._type;
	_annotationID = other._annotationID;
	_modality = other._modality;
	_tense = other._tense;
	_polarity = other._polarity;
	_genericity = other._genericity;
	_filters = other._filters;
	LinkedEventMention *ments = other.getEventMentions();
	if (ments != 0) {
		_eventMentions = _new LinkedEventMention(_new EventMention(*ments->eventMention));
		ments = ments->next;
		LinkedEventMention *iter = _eventMentions;
		while (ments != 0) {
			iter->next = _new LinkedEventMention(_new EventMention(*ments->eventMention));
			iter = iter->next;
			ments = ments->next;
		}
	}
}		

Event::Event(const Event &other, int set_offset, int sent_offset, std::vector<EventMentionSet*> mergedEventMentionSets)
{
	_ID = other._ID + set_offset;
	_type = other._type;
	_annotationID = other._annotationID;
	_modality = other._modality;
	_tense = other._tense;
	_polarity = other._polarity;
	_genericity = other._genericity;
	_filters = other._filters;
	LinkedEventMention *ments = other.getEventMentions();
	if (ments != 0) {
		EventMentionUID emUID = ments->eventMention->getUID();
		_eventMentions = _new LinkedEventMention(mergedEventMentionSets[emUID.sentno() + sent_offset]->getEventMention(emUID.index()));
		ments = ments->next;
		LinkedEventMention *iter = _eventMentions;
		while (ments != 0) {
			emUID = ments->eventMention->getUID();
			iter->next = _new LinkedEventMention(mergedEventMentionSets[emUID.sentno() + sent_offset]->getEventMention(emUID.index()));
			iter = iter->next;
			ments = ments->next;
		}
	}
}		

Event::~Event() {
	delete _eventMentions;
}


void Event::addEventMention(EventMention *ment) {
	if (_eventMentions == 0) {
		_eventMentions = _new LinkedEventMention(ment);
		_type = ment->getEventType();
	}
	else {
		LinkedEventMention *last = _eventMentions;
		while (last->next != 0)
			last = last->next;
		last->next = _new LinkedEventMention(ment);
	}
	ment->setEventID(getID());
}

void Event::addEventMentionPointer(EventMention *ment) {
	if (_eventMentions == 0) {
		_eventMentions = _new LinkedEventMention(ment);
	}
	else {
		LinkedEventMention *last = _eventMentions;
		while (last->next != 0)
			last = last->next;
		last->next = _new LinkedEventMention(ment);
	}
}

// After you do this, you ought to delete the other event
void Event::mergeInEvent(Event *otherEvent) {
	LinkedEventMention *last = _eventMentions;
	while (last->next != 0)
		last = last->next;
	last->next = otherEvent->getEventMentions();
	otherEvent->removeEventMentions();
	last = last->next;
	while (last != 0) {
		last->eventMention->setEventID(getID());
		last = last->next;		
	}
}

std::wstring Event::toString() {
	std::wstring result = _type.to_string();
	result += L": ";
	LinkedEventMention *iter = _eventMentions;
	while (iter != 0) {
		result += iter->eventMention->toString();
		iter = iter->next;
	}
	return result;
}

std::string Event::toDebugString() {
	std::string result = _type.to_debug_string();
	result += ": ";
	LinkedEventMention *iter = _eventMentions;
	while (iter != 0) {
		result += iter->eventMention->toDebugString();
		iter = iter->next;
	}
	return result;
}

void Event::dump(UTF8OutputStream &out, int indent) {
	out << L"Event " << _ID << L": ";
	out << this->toString();
}
void Event::dump(std::ostream &out, int indent) {
	out << "Event " << _ID << ": ";
	out << this->toDebugString();
}

void Event::consolidateEERelations(EntitySet *entitySet)
{
	LinkedEventMention *mentionIter = _eventMentions;

	while (mentionIter != 0) {
		EventMention *em = mentionIter->eventMention;
		for (int i = 0; i < em->getNArgs(); i++) {
			Entity *entity 
				= entitySet->getEntityByMention(em->getNthArgMention(i)->getUID());
			Symbol relation_type = em->getNthArgRole(i);
			if (entity == 0) {
				SessionLogger::warn("null_entity_in_event")
					<< "Null entity in Event!\n"
					<< "Event:\n" << toString() << "\n"
					<< "Mention "<< i << ":\n"
					<< em->getNthArgMention(i)->getNode()->toString(0) << "\n";

				continue;
			}
			int entity_id = entity->getID();
			bool found_match = false;
			for (int j = 0; j < (int)_consolidatedRelations.size(); j++) {
				if (_consolidatedRelations[j]->getEntityID() == entity_id) {

					if (ParamReader::isParamTrue("use_correct_answers")) {

						_consolidatedRelations[j]->addMention(em,
											em->getNthArgMention(i));
						found_match = true;
					} else {
						if (_consolidatedRelations[j]->getType() == relation_type) {
							_consolidatedRelations[j]->addMention(em,
												em->getNthArgMention(i));
							found_match = true;
						} 
					}
				}
			}
			if (!found_match) {
                		EventEntityRelation* tmp_ptr = 
					_new EventEntityRelation(em->getNthArgRole(i), em, _ID, 
					em->getNthArgMention(i), entity_id);
				_consolidatedRelations.push_back(tmp_ptr);
			}

		}
		mentionIter = mentionIter->next;
	}

}

float Event::getScore() {
	float score = 0;
	
	LinkedEventMention *mentionIter = _eventMentions;
	while (mentionIter != 0) {
		EventMention *em = mentionIter->eventMention;
		score += em->getNArgs();
		mentionIter = mentionIter->next;
	}
	return score;
}

const Mention *Event::getFirstMentionForSlot(Symbol slotName) {
	LinkedEventMention *mentionIter = _eventMentions;
	while (mentionIter != 0) {
		const Mention *result = mentionIter->eventMention->getFirstMentionForSlot(slotName);
		if (result != 0)
			return result;
		mentionIter = mentionIter->next;
	}
	return 0;
}

void Event::setModalityFromMentions() { 
	LinkedEventMention *mentionIter = _eventMentions;
	while (mentionIter != 0) {
		if (getEventMentions()->eventMention->getModality().is_defined())
			_modality = getEventMentions()->eventMention->getModality();
		mentionIter = mentionIter->next;
	}
}
void Event::setTenseFromMentions() { 
	LinkedEventMention *mentionIter = _eventMentions;
	while (mentionIter != 0) {
		if (getEventMentions()->eventMention->getTense().is_defined())
			_tense = getEventMentions()->eventMention->getTense();
		mentionIter = mentionIter->next;
	}
}
void Event::setPolarityFromMentions() { 
	LinkedEventMention *mentionIter = _eventMentions;
	while (mentionIter != 0) {
		if (getEventMentions()->eventMention->getPolarity().is_defined())
			_polarity = getEventMentions()->eventMention->getPolarity();
		mentionIter = mentionIter->next;
	}
}
void Event::setGenericityFromMentions() { 
	LinkedEventMention *mentionIter = _eventMentions;
	while (mentionIter != 0) {
		if (getEventMentions()->eventMention->getGenericity().is_defined())
			_genericity = getEventMentions()->eventMention->getGenericity();
		mentionIter = mentionIter->next;
	}
}

void Event::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);
}

void Event::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"Event", this);

	stateSaver->saveInteger(_ID);
	stateSaver->saveSymbol(_type);
	stateSaver->saveInteger(_modality.toInt());
	stateSaver->saveInteger(_tense.toInt());
	stateSaver->saveInteger(_polarity.toInt());
	stateSaver->saveInteger(_genericity.toInt());

	LinkedEventMention *mentionIter = _eventMentions;
	int count = 0;
	while (mentionIter != 0) {
		count++;
		mentionIter = mentionIter->next;
	}

	stateSaver->saveInteger(count);
	stateSaver->beginList(L"Event::_eventMentions");
	mentionIter = _eventMentions;
	while (mentionIter != 0) {
		stateSaver->savePointer(mentionIter->eventMention);
		mentionIter = mentionIter->next;
	}
	stateSaver->endList();

	stateSaver->endList();

}
// For loading state:
Event::Event(StateLoader *stateLoader) : _eventMentions(0), 
	_annotationID(SymbolConstants::nullSymbol), _filters()
{

	int id = stateLoader->beginList(L"Event");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	_ID = stateLoader->loadInteger();
	_type = stateLoader->loadSymbol();
	_modality = ModalityAttribute::getFromInt(stateLoader->loadInteger());
	_tense = TenseAttribute::getFromInt(stateLoader->loadInteger());
	_polarity = PolarityAttribute::getFromInt(stateLoader->loadInteger());
	_genericity = GenericityAttribute::getFromInt(stateLoader->loadInteger());

	int count = stateLoader->loadInteger();
	stateLoader->beginList(L"Event::_eventMentions");
	for (int i = 0; i < count; i++) {
		EventMention *em = (EventMention *) stateLoader->loadPointer();
		addEventMentionPointer(em);
	}
	stateLoader->endList();

	stateLoader->endList();

}
void Event::resolvePointers(StateLoader * stateLoader) {
	LinkedEventMention *mentionIter = _eventMentions;
	while (mentionIter != 0) {
		mentionIter->eventMention = (EventMention *) stateLoader->getObjectPointerTable().getPointer(mentionIter->eventMention);
		mentionIter->eventMention->setEventID(getID());
		mentionIter = mentionIter->next;
	}

		
}

void
Event::applyFilter(const std::string& filterName, EventClutterFilter *filter)
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
Event::isFiltered(const std::string& filterName) const
{
	std::map<std::string, double>::const_iterator filter (_filters.find(filterName));
	return filter != _filters.end();
}

double
Event::getFilterScore(const std::string& filterName) const
{
	double score (0.);
	std::map<std::string, double>::const_iterator filter (_filters.find(filterName));
	if (filter != _filters.end()) {
		score = filter->second;
	}
	return score;
}

const wchar_t* Event::XMLIdentifierPrefix() const {
	return L"event";
}

void Event::saveXML(SerifXML::XMLTheoryElement eventElem, const Theory *context) const {
	using namespace SerifXML;
	const DocTheory *docTheory = dynamic_cast<const DocTheory*>(context);
	
	if (context == 0)
		throw InternalInconsistencyException("Event::saveXML", "Expected context to be a DocTheory");

	eventElem.setAttribute(X_event_type, _type);
	if (!_annotationID.is_null() && _annotationID != SymbolConstants::nullSymbol)
		eventElem.setAttribute(X_annotation_id, _annotationID);
	eventElem.setAttribute(X_genericity, _genericity.toString());
	eventElem.setAttribute(X_polarity, _polarity.toString());
	eventElem.setAttribute(X_tense, _tense.toString());
	eventElem.setAttribute(X_modality, _modality.toString());
	
	// Mentions
	std::vector<const Theory*> eventMentionList;
	for (const LinkedEventMention *lem=_eventMentions; lem != 0; lem = lem->next) {
		eventMentionList.push_back(lem->eventMention);
		if (eventElem.getOptions().include_mentions_as_comments)
			eventElem.addComment(lem->eventMention->toString());
	}
	eventElem.saveTheoryPointerList(X_event_mention_ids, eventMentionList);

	saveArgumentEntitiesXML(eventElem, docTheory);
}

void Event::getArgumentEntities(EventArguments* eventArgs,
                                const EntitySet* entitySet,
                                const ValueSet* valueSet,
                                std::set<MentionUID> printedMentions) const
{
	// rewrite of APF4GenericResultCollector::_printAPFEventHeader

	Symbol *printedEventEntityRole = _new Symbol[entitySet->getNEntities()];
	Symbol *printedEventValueRole  = _new Symbol[valueSet->getNValues()];

	for (int k=0; k < entitySet->getNEntities(); k++)
      printedEventEntityRole[entitySet->getEntity(k)->getID()] = Symbol();
	for (int m=0; m < valueSet->getNValues(); m++)
      printedEventValueRole[valueSet->getValue(m)->getID()] = Symbol();
	
	for (const LinkedEventMention *lem = _eventMentions; lem != 0; lem = lem->next) {
		EventMention* em = lem->eventMention;
		for (int i = 0; i < em->getNArgs(); i++) {
			Symbol role = em->getNthArgRole(i);
			const Mention *mention = em->getNthArgMention(i);
			if (!_isPrintedMention(mention, printedMentions)) {
				SessionLogger::warn("no_entity_for_mention")
 				<< "No entity associated with event argument mention, skipping:\n"
                << mention->getUID() << ", " << mention->getNode()->toDebugTextString();
				continue;
			}
			const Entity *entity = entitySet->getEntityByMentionWithoutType(mention->getUID());
			if (entity == 0 && mention->getMentionType() == Mention::NONE) {
				const Mention *m = mention;				
				while (m != 0 && m->getMentionType() == Mention::NONE && m->getParent() != 0) {
					m = m->getParent();
				}
				entity = entitySet->getEntityByMentionWithoutType(m->getUID());
			}
			// If a LIST is part of an entity, the members of that LIST will get written out as part of the entity
			if (entity == 0 && mention->getParent() && mention->getParent()->getMentionType() == Mention::LIST) {
				entity = entitySet->getEntityByMentionWithoutType(mention->getParent()->getUID());
			}
			if (entity == 0) {
 				// as above, return just a warning and continue to next mention, do not fail.
				SessionLogger::warn("no_entity_for_mention")
					<< "No entity (or incorrectly-typed entity) for event argument mention, skipping:\n"
                    << mention->getUID() << ", " << mention->getNode()->toDebugTextString();
				continue;  

            }

			if (!ParamReader::isParamTrue("use_correct_answers")) {
				if (entity->isGeneric())
					continue;
			}
            int ent_id = entity->getID();
            if (printedEventEntityRole[ent_id].is_null() && 
                _isPrintedMention(mention, printedMentions)) {
              printedEventEntityRole[ent_id] = role;
              eventArgs->entities.push_back(entity);
              eventArgs->entity_roles.push_back(role);
			}
		}
		for (int i = 0; i < em->getNValueArgs(); i++) {
			Symbol role = em->getNthArgValueRole(i);
			const ValueMention *valueMention = em->getNthArgValueMention(i);
			if (valueMention == 0) {				
              SessionLogger::logger->reportInternalInconsistencyError().with_id("rel_print_20")
                << "Value mention for an event is null";
              continue;
			}
			Value *value = valueSet->getValueByValueMention(valueMention->getUID());
			if (value == 0) {
              SessionLogger::logger->reportInternalInconsistencyError().with_id("rel_print_20")
                << "No value for value mention \n"
                << valueMention->getUID().toInt();
              continue;
			}
            int value_id = value->getID();
            if (printedEventValueRole[value_id].is_null()) {
              printedEventValueRole[value_id] = role;
              eventArgs->values.push_back(value);
              eventArgs->value_roles.push_back(role);
			}
		}
	}
	delete[] printedEventValueRole;
	delete[] printedEventEntityRole;
}

void Event::saveArgumentEntitiesXML(SerifXML::XMLTheoryElement eventElem, const DocTheory *docTheory) const
{
	using namespace SerifXML;
	const EntitySet* entitySet = docTheory->getEntitySet();
	const ValueSet* valueSet = docTheory->getValueSet();

	// If we haven't yet constructed the document-level entity set or
	// document set, then we don't need to generate argument entities
	// in the xml output; so just return.
	if ((entitySet==0) || (valueSet==0))
		return; 

	std::set<MentionUID> mentionSet = getDocumentMentionIDs(docTheory);

	EventArguments *eventArgs = _new EventArguments();
    getArgumentEntities(eventArgs, entitySet, valueSet, mentionSet);
	XMLTheoryElement argumentElem;

	if (eventArgs->entities.size() > 0) {
		for (size_t n=0; n < eventArgs->entities.size(); n++) {
          argumentElem = eventElem.addChild(X_EventArg);
          argumentElem.saveTheoryPointer(X_entity_id, eventArgs->entities[n]);
          argumentElem.setAttribute(X_role, eventArgs->entity_roles[n].to_debug_string());
		}
	}
	if (eventArgs->values.size() > 0) {
		for (size_t n=0; n < eventArgs->values.size(); n++) {
			argumentElem = eventElem.addChild(X_EventArg);
			argumentElem.saveTheoryPointer(X_value_id, eventArgs->values[n]);
			argumentElem.setAttribute(X_role, eventArgs->value_roles[n].to_debug_string());
		}
	}	
	delete eventArgs;
}

std::set<MentionUID> Event::getDocumentMentionIDs(const DocTheory* docTheory) const
{
	std::set<MentionUID> docMentions;
	int nSents = docTheory->getNSentences();
	for (int i=0; i<nSents; i++) {
		SentenceTheory* sentTheory = docTheory->getSentenceTheory(i);
		MentionSet* ms = sentTheory->getMentionSet();
		int nMentions = ms->getNMentions();
		for (int j=0; j<nMentions; j++) {
          Mention* m = ms->getMention(j);
          if (m->mentionType != Mention::LIST) {
            MentionUID muid = ms->getMention(j)->getUID();
            docMentions.insert(muid);
          }
        }
	}
	return docMentions;
}

bool Event::_isPrintedMention(const Mention *ment, std::set<MentionUID> docMentions) const
{
  if (docMentions.find(ment->getUID()) != docMentions.end())
    return true;
  else
    return false;
}

Event::Event(SerifXML::XMLTheoryElement eventElem, int event_id)
: _eventMentions(0), _filters()
{
    // ignoring the EventArg b/c they can be reconstructed from EventMentionArgs
    // and there is no theory object for them
	using namespace SerifXML;
	eventElem.loadId(this);
	_ID = event_id;
	_type = eventElem.getAttribute<Symbol>(X_event_type);
	_annotationID = eventElem.getAttribute<Symbol>(X_annotation_id, SymbolConstants::nullSymbol);
	_genericity = GenericityAttribute::getFromString(eventElem.getAttribute<std::wstring>(X_genericity).c_str());
	_polarity = PolarityAttribute::getFromString(eventElem.getAttribute<std::wstring>(X_polarity).c_str());
	_tense = TenseAttribute::getFromString(eventElem.getAttribute<std::wstring>(X_tense).c_str());
	_modality = ModalityAttribute::getFromString(eventElem.getAttribute<std::wstring>(X_modality).c_str());
	// Mentions
	std::vector<const EventMention*> eventMentionList = eventElem.loadTheoryPointerList<EventMention>(X_event_mention_ids);
	for (size_t i=0; i<eventMentionList.size(); ++i) {
		EventMention *eventMention = const_cast<EventMention*>(eventMentionList[i]);
		eventMention->setEventID(_ID);
		addEventMentionPointer(eventMention);
	}
}
