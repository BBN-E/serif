// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef OLD_MAXENT_EVENT_SET_H
#define OLD_MAXENT_EVENT_SET_H
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolSet.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/DebugStream.h"
#include "Generic/maxent/OldMaxEntFeatureTable.h"

class OldMaxEntEvent;

class EventContext {
public:
	EventContext() : _n_outcomes(0), _n_predicates(0), _total_count(0), next(0), 
		_predicates(0), _outcome_counts(0), _outcomes(0) {};
	EventContext(int max_outcomes, int n_predicates, const SymbolSet *predicates);
	EventContext(const EventContext &x);
	~EventContext();
	const EventContext &operator=(const EventContext &x);

	bool same(int n_predicates, const SymbolSet *predicates) const;
	void increment(Symbol outcome, int count);
	void merge(EventContext *old_entry);

	int getTotalCount() const { return _total_count; }
	int getNPredicates() const { return _n_predicates; }
	int getNOutcomes() const { return _n_outcomes; }
	int getOutcomeCount(int i) const;
	Symbol getOutcome(int i) const;
	SymbolSet getPredicate(int i) const;
	void setOutcomeCount(int i, int val);

	void dump(UTF8OutputStream &out) const;
	void dump(DebugStream &out) const;


	int MAX_OUTCOMES;
	int _n_outcomes;
	int _n_predicates;
	SymbolSet *_predicates;
	int _total_count;
	int *_outcome_counts;
	Symbol *_outcomes;

	EventContext *next;
};

class EventTable {
	friend class ContextIter;
public:
	EventTable(int init_size, int max_outcomes); // size should be prime of course
	~EventTable();

	void add(Symbol outcome, int n_predicates, const SymbolSet *predicates, int count);
	void merge(EventContext *old_entry, int n_predicates, const SymbolSet *predicates);

	int getSize() const { return _size; }
	int getNBins() const { return _n_bins; }
	int getNEvents() const { return _n_events; }

	void dump(UTF8OutputStream &out) const;
	void dump(DebugStream &out) const;

private:
	int hash(int n_predicates, const SymbolSet *predicates);

	int _n_outcomes;

	int _size;		// number of slots
	int _n_bins;	// number of EventContexts
	int _n_events;	// sum of counts for all bins 

	EventContext **_slots;
};

class ContextIter {
public:
	ContextIter(EventTable *table);
	EventContext *findNext();
	bool done() { return _entry == 0; }
private:
	EventTable *_table;
	int _slot;
	EventContext *_entry;
};

class OldMaxEntEventSet;

class EventIter {
	friend class OldMaxEntEventSet;
public:
	EventIter(const OldMaxEntEventSet* eventSet, EventContext *context);
	~EventIter();
	OldMaxEntEvent* findNext();
	bool done();
private:
	const OldMaxEntEventSet *_eventSet;
	EventContext *_context;
	int _outcome_index;
	OldMaxEntEvent *_event;
};

class OldMaxEntEventSet {
	friend class EventIter;
private:
	static const float targetLoadingFactor;
	struct HashKey {
		size_t operator()(const SymbolSet& s) const {
			return s.hash_value();
		}
	};
    struct EqualKey {
        bool operator()(const SymbolSet& s1, const SymbolSet& s2) const {
            return s1 == s2;
        }
    };

public:
	typedef serif::hash_map<SymbolSet, int, HashKey, EqualKey> SymbolSetMap;
	typedef OldMaxEntFeatureTable::Table::iterator FeatureIter;
private:
	SymbolSetMap *_predicates;
	Symbol *_outcomes;
	EventTable *_eventTable;
	OldMaxEntFeatureTable *_features;

	const int MAX_OUTCOMES;
	int _n_predicates;
	int _n_outcomes;
	int _n_training_events;

	static const int EVENT_TABLE_SIZE;
	static const int INIT_TABLE_SIZE;

public:
	OldMaxEntEventSet(Symbol *outcomes, int n_outcomes);
	~OldMaxEntEventSet();

	void addPriorFeatures();
	void addEvent(OldMaxEntEvent *event, DebugStream &debug) { addEvent(event, 1, debug); }
	void addEvent(OldMaxEntEvent *event, int count, DebugStream &debug);
	void prune(int threshold, DebugStream &debug);

	int getNOutcomes() const { return _n_outcomes; }
	int getNFeatures() const { return _features->get_size(); }
	int getNContexts() const { return _eventTable->getNBins(); }
	int getNEvents() const { return _eventTable->getNEvents(); } // should this really be _n_training_events?
																 // (total before pruning)
	Symbol getOutcome(int i);
	int getNEventsInBins() const { return _eventTable->getNEvents(); }
	FeatureIter featureIterBegin() const { return _features->get_start(); }
	FeatureIter featureIterEnd() const { return _features->get_end(); }
	ContextIter contextIter() const { return ContextIter(_eventTable); }
	EventIter eventIter(EventContext *context) const { return EventIter(this, context); }

	// used to find constant C
	int getMaxContextFeatures();
	int getFeatureID(Symbol outcome, SymbolSet predicate);

	void dump(UTF8OutputStream &out) const;
	void dump(DebugStream &out) const;
};


#endif
