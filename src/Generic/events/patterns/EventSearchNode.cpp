// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/events/patterns/EventSearchNode.h"
#include "Generic/events/patterns/DeprecatedPatternEventFinder.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/SynNode.h"

EventSearchNode::EventSearchNodePool EventSearchNode::nodePool(NODES_PER_ALLOCATION);
bool EventSearchNode::_allow_mention_set_changes = false;

void EventSearchNode::setChild(EventSearchNode *child) { 
	if (_children == 0)
		_children = child;
	else {
		EventSearchNode *iter = _children;
		while (iter->getNext() != 0)
			iter = iter->getNext();
		iter->setNext(child);
	}
}

void EventSearchNode::populateEventMention(EventMention *m,
										   MentionSet *mentions,
										   ValueMentionSet *values) 
{

	if (_ontology_label != SymbolConstants::nullSymbol) {
		if (isProp()) {
			m->setEventType(_ontology_label);
			m->setAnchor(_prop);
		} else {
			if (_mention->getEntityType().isRecognized())
				m->addArgument(_ontology_label, _mention);
			else {
				for (int valid = 0; valid < values->getNValueMentions(); valid++) {
					const ValueMention *val = values->getValueMention(valid);
					if (_mention->getNode()->getStartToken() == val->getStartToken() &&
						_mention->getNode()->getEndToken() == val->getEndToken())
					{
						m->addValueArgument(_ontology_label, val);
						break;
					}
				}
			}

			if (_allow_mention_set_changes && _overriding_type != EntityType::getOtherType()) {
				changeMentionType(mentions, _mention, _overriding_type);

				if (DeprecatedPatternEventFinder::DEBUG) {
					DeprecatedPatternEventFinder::_debugStream << L"***Changing mention type of: ";
					DeprecatedPatternEventFinder::_debugStream << _mention->getNode()->toTextString() << L" to " ;
					DeprecatedPatternEventFinder::_debugStream << _overriding_type.getName().to_string();
					DeprecatedPatternEventFinder::_debugStream << L"***\n";
				}
			}
			// EMB 6/22/05: don't need to do this if we haven't done coreference yet
			// Entity *entity 	= entitySet->getEntityByMention(_mention->getUID());
			// if (entity == 0) {
			//	entitySet->addNew(_mention->getUID(), _mention->getEntityType());
			// }
		}
	}

	if (_next != 0)
		_next->populateEventMention(m, mentions, values);

	if (_children != 0)
		_children->populateEventMention(m, mentions, values);
}

const SynNode *EventSearchNode::findNodeForOntologyLabel(Symbol label) {

	if (_ontology_label == label) {
		if (isProp()) {
			const SynNode *node = _prop->getPredHead();
			while (node != 0 && node->getParent() != 0 && node->getParent()->getHead() == node)
				node = node->getParent();
			return node;
		}
		if (isMention())
			return _mention->getNode();
	}

	const SynNode *result = 0;

	if (_next != 0)
		result = _next->findNodeForOntologyLabel(label);

	if (result)
		return result;

	if (_children != 0)
		result = _children->findNodeForOntologyLabel(label);

	return result;
}

void EventSearchNode::changeMentionType(MentionSet *mentions, const Mention *ment, EntityType type) {
	mentions->changeEntityType(ment->getUID(), type);
}

std::wstring EventSearchNode::toString(int indent) {
	std::wstring str = L"";
	for (int i = 0; i < indent; i++)
		str += L" ";
	if (_ontology_label != SymbolConstants::nullSymbol) {
		str += _ontology_label.to_string();
		str += L" -- ";		
	} 			
	if (isProp()) {
		str += _prop->toString();
		str += L"\n";
	} else {
		if (_mention->getMentionType() == Mention::LIST) {
			str += L"SET\n";
		} else {
			str += _mention->getHead()->toTextString();		
			str += L"\n";		
		}
	}
	if (_next != 0)
		str += _next->toString(indent);
	if (_children !=0)
		str += _children->toString(indent + 2);
	return str;
}

std::string toDebugString() { return ""; }
