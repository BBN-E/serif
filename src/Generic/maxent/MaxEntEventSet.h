// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MAXENT_EVENT_SET_H
#define MAXENT_EVENT_SET_H
#include "Generic/common/Symbol.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/maxent/MaxEntFeatureTable.h"

class DTFeature;
class DTObservation;
class DTTagSet;
class DTFeatureTypeSet;

class ObservationInfo {
	friend class MaxEntEventSet;
	friend class NewEventTable;
	friend class ObsIter;
public:
	ObservationInfo() : _n_outcomes(0), _n_features(0), _total_count(0), next(0), 
						 _features(0), _outcome_counts(0), _outcomes(0) {};
	ObservationInfo(int max_outcomes, int n_features, DTFeature **features);
	ObservationInfo(const ObservationInfo &x);
	~ObservationInfo();
	const ObservationInfo &operator=(const ObservationInfo &x);

	bool same(int n_features, DTFeature **features) const;
	void increment(int outcome, int count);
	void merge(ObservationInfo *old_entry);

	int getTotalCount() const { return _total_count; }
	int getNFeatures() const { return _n_features; }
	DTFeature *getFeature(int i) const;
	int getNOutcomes() const { return _n_outcomes; }
	int getOutcomeCount(int i) const;
	int getOutcomeCountByTagID(int i) const;
	int getOutcome(int i) const;
	void dump(UTF8OutputStream &out) const;

private:
	void setOutcomeCount(int i, int val);

	int MAX_OUTCOMES;
	int _n_outcomes;
	int _n_features;
	DTFeature **_features;
	int _total_count;
	int *_outcome_counts;
	int *_outcomes;

	ObservationInfo *next;
};

class NewEventTable {
	friend class ObsIter;
public:
	NewEventTable(int init_size, int max_outcomes); // size should be prime of course
	~NewEventTable();

	void add(int outcome, int n_features, DTFeature **features, int count);
	void merge(ObservationInfo *old_entry, int n_features, DTFeature **features);

	int getSize() const { return _size; }
	int getNBins() const { return _n_bins; }
	int getNEvents() const { return _n_events; }

	void dump(UTF8OutputStream &out) const;

private:
	int hash(int n_features, DTFeature **features);

	int _n_outcomes;

	int _size;		// number of slots
	int _n_bins;	// number of ObservationInfos
	int _n_events;	// sum of counts for all bins 

	ObservationInfo **_slots;
};

class ObsIter {
public:
	ObsIter(NewEventTable *table);
	ObservationInfo *findNext();
	bool done() { return _entry == 0; }
private:
	NewEventTable *_table;
	int _slot;
	ObservationInfo *_entry;
};

class MaxEntEventSet {
public:
	typedef MaxEntFeatureTable::Table::iterator FeatureIter;
private:
	NewEventTable *_eventTable;
	MaxEntFeatureTable *_activeFeatures;
	DTTagSet *_tagSet;
	DTFeatureTypeSet *_featureTypes;

	// determines whether or not to add new features to _activeFeatures
	bool _build_feature_set;

	static const int EVENT_TABLE_SIZE;
	static const int INIT_TABLE_SIZE;

	// for printing out vector training data
	bool _print_vector_data;
	UTF8OutputStream _vectorStream;

public:
	MaxEntEventSet(DTTagSet *tags, DTFeatureTypeSet *featureTypes, 
					  bool build_feature_set = true, const char *vector_file = 0);
	~MaxEntEventSet();

	void deallocateFeatures();

	void addEvent(DTObservation *obs, int correct_answer, int count = 1);
	void addEvent(DTState state, int count = 1);  // more generalized version of the above

	void prune(int threshold, UTF8OutputStream &debug);

	int getNFeatures() const { return _activeFeatures->get_size(); }
	int getNEvents() const { return _eventTable->getNEvents(); } 
	int getNObservations() const { return _eventTable->getNBins(); }

	FeatureIter featureIterBegin() const { return _activeFeatures->get_start(); }
	FeatureIter featureIterEnd() const { return _activeFeatures->get_end(); }
	ObsIter observationIter() const { return ObsIter(_eventTable); }

	// used to find constant C
	int getMaxActiveFeatures();
	int getFeatureID(DTFeature *feature);

	void dump(UTF8OutputStream &out) const;
};


#endif
