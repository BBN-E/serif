// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_SEARCH_NODE_H
#define EVENT_SEARCH_NODE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/MemoryPool.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/EntityType.h"
#include <iostream>

class EventPatternNode;
class Proposition;
class Mention;
class EventMention;
class MentionSet;
class ValueMentionSet;
class EntitySet;
class SynNode;

class EventSearchNode {
public:
	EventSearchNode(const Mention *ment) : _type(MENTION), _mention(ment), 
		_prop(0), _children(0), _next(0), _overriding_type(EntityType::getOtherType()),
		_ontology_label(SymbolConstants::nullSymbol) {}
	EventSearchNode(const Proposition *prop) : _type(PROP), _mention(0), 
		_prop(prop), _children(0), _next(0), _overriding_type(EntityType::getOtherType()),
		_ontology_label(SymbolConstants::nullSymbol) {}
	
	bool isProp() { return _type == PROP; }
	bool isMention() { return _type == MENTION; }
	EventSearchNode *getNext() { return _next; }
	void setNext(EventSearchNode *n) { _next = n; }

	void populateEventMention(EventMention *m, MentionSet *mentions,
										   ValueMentionSet *values);

	void setLabel(Symbol sym) { _ontology_label = sym; }
	void setOverridingEntityType(EntityType type) { _overriding_type = type; }

	void setChild(EventSearchNode *child);

	const SynNode *findNodeForOntologyLabel(Symbol label);

private:
	typedef enum { PROP, MENTION } Type;
	Type _type;

	Symbol _ontology_label;
	EntityType _overriding_type;

	const Proposition *_prop;
	const Mention *_mention;

	EventSearchNode *_children;
	EventSearchNode *_next;

	void changeMentionType(MentionSet *mentions, const Mention *ment, EntityType type);

public:
	~EventSearchNode()
    {
        delete _children;
        delete _next;
    }
	

	static void* operator new(size_t size) {
		return nodePool.allocate(size);
	}
	static void operator delete(void* object, size_t size) {
		nodePool.deallocate(static_cast<EventSearchNode*>(object), size);
	}


	std::wstring toString(int indent = 0);
	std::string toDebugString();

	static void allowMentionSetChanges() { _allow_mention_set_changes = true; }
	static void disallowMentionSetChanges() { _allow_mention_set_changes = false; }
	
private:
	static bool _allow_mention_set_changes;

    static const size_t NODES_PER_ALLOCATION = 100;

	typedef MemoryPool<EventSearchNode> EventSearchNodePool;
	static EventSearchNodePool nodePool;

};

#endif
