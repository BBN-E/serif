// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/SymbolSet.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/maxent/OldMaxEntEventSet.h"
#include "Generic/maxent/OldMaxEntFeatureTable.h"
#include "Generic/maxent/OldMaxEntEvent.h"

const int OldMaxEntEventSet::INIT_TABLE_SIZE = 100;
const int OldMaxEntEventSet::EVENT_TABLE_SIZE = 10007;  // hash table size -- should be prime
const float OldMaxEntEventSet::targetLoadingFactor = static_cast<float>(0.7);

OldMaxEntEventSet::OldMaxEntEventSet(Symbol *outcomes, int n_outcomes) : MAX_OUTCOMES(n_outcomes),
	_features(0), _predicates(0), _outcomes(0), _eventTable(0)
{ 

	int numBuckets = static_cast<int>(INIT_TABLE_SIZE / targetLoadingFactor);

	if (numBuckets < 5)
		numBuckets = 5;
	_predicates = _new SymbolSetMap(numBuckets);
	_n_predicates = 0;

	_n_outcomes = n_outcomes;
	_outcomes = _new Symbol[n_outcomes];
	for (int i = 0; i < n_outcomes; i++) 
		_outcomes[i] = outcomes[i];

	_features = _new OldMaxEntFeatureTable(INIT_TABLE_SIZE);
	_eventTable = _new EventTable(EVENT_TABLE_SIZE, MAX_OUTCOMES);
}

OldMaxEntEventSet::~OldMaxEntEventSet() {
	delete _predicates;
	delete _outcomes;
	delete _features;
	delete _eventTable;
}

void OldMaxEntEventSet::addPriorFeatures() {
	SymbolSet emptyPredicate;
	for (int i = 0; i < _n_outcomes; i++) 
		_features->add(_outcomes[i], emptyPredicate, 0);
}

void OldMaxEntEventSet::addEvent(OldMaxEntEvent *event, int count, DebugStream &debug) {
	SymbolSet *sorted_predicates = _new SymbolSet[event->getNContextPredicates()];
	int *sorted_ids = _new int[event->getNContextPredicates()];
	
	for (int j = 0; j < event->getNContextPredicates(); j++) {
		SymbolSet current_predicate = event->getContextPredicate(j);
		int current_index;
		
		// add predicate to the predicate set
		int *iter = _predicates->get(current_predicate);
		if (iter == 0) {
			current_index = _n_predicates;
			(*_predicates)[current_predicate] = _n_predicates++;
		}
		else 
			current_index = (*iter);

		// insert in sorted array of predicates
		bool inserted = false;
		for (int k = 0; k < j; k++) {
			if (current_index < sorted_ids[k]) {
				for (int m = j; m > k; m--) {
					sorted_predicates[m] = sorted_predicates[m-1];
					sorted_ids[m] = sorted_ids[m-1];
				}
				sorted_predicates[k] = current_predicate;
				sorted_ids[k] = current_index;
				inserted = true;
				break;
			}
		}
		if (!inserted) {
			sorted_predicates[j] = current_predicate;
			sorted_ids[j] = current_index;
		}
		// increment current feature count
		_features->add(event->getOutcome(), event->getContextPredicate(j), count);
	}
		
	// add event to the event table
	_eventTable->add(event->getOutcome(), event->getNContextPredicates(), sorted_predicates, count);
	_n_training_events += count;

	delete [] sorted_predicates;
	delete [] sorted_ids;
}

void OldMaxEntEventSet::prune(int threshold, DebugStream &debug) {
	
	// if no need to prune, return
	if (threshold <= 1)
		return;

	_features->prune(threshold);

	// Recreate the event table after pruning:
	// now that some features have been eliminated, any bins (contexts) that differ
	// only in features that are no longer used must be merged and any that no longer
	// contain any features must be eliminated.
	EventTable *new_table = _new EventTable(EVENT_TABLE_SIZE, MAX_OUTCOMES);

	int n_new_predicates;
	SymbolSet *new_predicates = _new SymbolSet[_n_predicates];
	ContextIter cIter(_eventTable);
	EventContext *context = cIter.findNext();
	while (! cIter.done()) {
		n_new_predicates = 0;
		for (int i = 0; i < context->_n_predicates; i++) {
			bool feature_encountered = false;
			// JCS 1/28/04 - I think we want to keep every predicate that occurs in any feature
			// across the range of all outcomes, not just the outcomes that actually ocurred with
			// this context in a training event.
			/*for (int j = 0; j < context->getNOutcomes(); j++) {
				// if this event occurs at least once 
				if (context->getOutcomeCount(j) > 0) {
					if (_features->lookup(context->getOutcome(j), context->_predicates[i]) != 0) {
						feature_encountered = true;
						break;
					}
				}
			}*/
			for (int j = 0; j < _n_outcomes; j++) {
				if (_features->lookup(_outcomes[j], context->_predicates[i]) != 0) {
					feature_encountered = true;
					break;
				}
			}
			if (feature_encountered)
				new_predicates[n_new_predicates++] = context->_predicates[i];
		}
		if (n_new_predicates != 0) {
			new_table->merge(context, n_new_predicates, new_predicates);
		} else {
			throw UnexpectedInputException("OldMaxEntEventSet::prune()", 
				  "No active predicates in training event after pruning");
		}
		context = cIter.findNext();
	}
	delete [] new_predicates;

	delete _eventTable;
	_eventTable = new_table;

	// JCS 1/28/04 - 
	// Old version zeros out cells for events (context + outcome) that don't have any used predicates -
	// GIS requires that each possible context + outcome pair have at least one feature.
	// Current version just checks to make sure the requirement holds.
	ContextIter cIter2(_eventTable);
	context = cIter2.findNext();
	while (! cIter2.done()) {
		for (int i = 0; i < context->getNOutcomes(); i++) {
			if (context->getOutcomeCount(i) > 0) {
				// if this event has been encountered...
				if (context->getOutcomeCount(i) >= threshold) {
					// this event happened more than threshold times, so clearly
					// all the predicate it consists of occurred more than
					// threshold times, so there is no need to check
					continue;
				}
				else {
					// throw exception when no predicates meet threshold
					bool at_least_one_feature_used = false;
					for (int j = 0; j < context->_n_predicates; j++) {
						if (_features->lookup(context->getOutcome(i), context->_predicates[j]) != 0) {
							at_least_one_feature_used = true;
							break;
						}
					}
					if (!at_least_one_feature_used) {
						//context->setOutcomeCount(i, 0);
						throw UnexpectedInputException("OldMaxEntEventSet::prune()", 
							 "No active features in training event after pruning");
					}
				}
			}
		}
		context = cIter2.findNext();
	}

}

Symbol OldMaxEntEventSet::getOutcome(int i) {
	if (i >= 0 && i < _n_outcomes)
		return _outcomes[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"OldMaxEntEventSet::getOutcome()", _n_outcomes, i);
}

int OldMaxEntEventSet::getMaxContextFeatures() {
	int current_max = 0;
	ContextIter cIter(_eventTable);
	EventContext *context = cIter.findNext();
	while (!cIter.done()) {
		EventIter eIter(this, context);
		OldMaxEntEvent *event = eIter.findNext();
		while (!eIter.done()) {
			if (event->getNContextPredicates() > current_max)
				current_max = event->getNContextPredicates();
			event = eIter.findNext();
		}
		context = cIter.findNext();
	}
	return current_max;
}

int OldMaxEntEventSet::getFeatureID(Symbol outcome, SymbolSet predicate) {
	OldMaxEntEventSet::FeatureIter iter = _features->get_element(outcome, predicate);
	if (iter == _features->get_end())
		return -1;
	else
		return (*iter).first->getID();
}

void OldMaxEntEventSet::dump(UTF8OutputStream &out) const {
	out << L"Predicates (total:" << _n_predicates << L")\n";
	for (SymbolSetMap::iterator iter = _predicates->begin(); iter != _predicates->end(); ++iter)
		out << (*iter).first <<  L"\n";
	out << L"Event Table:\n";
	out << L"Number of Bins: " << _eventTable->getNBins() << L"\n";
	out << L"Number of Events: " << _eventTable->getNEvents() << L"\n";
	_eventTable->dump(out);
	out << L"Feature Table:\n";
	_features->print_to_open_stream(out);
}

void OldMaxEntEventSet::dump(DebugStream &out) const {
	out << L"Predicates (total:" << _n_predicates << L")\n";
	for (SymbolSetMap::iterator iter = _predicates->begin(); iter != _predicates->end(); ++iter)
		if ((*iter).second != -1)
			out << (*iter).first.to_string() << L"\n";
	out << L"Event Table:\n";
	out << L"Number of Bins: " << _eventTable->getNBins() << L"\n";
	out << L"Number of Events: " << _eventTable->getNEvents() << L"\n";
	_eventTable->dump(out);
	out << L"Feature Table:\n";
	_features->print_to_open_stream(out);
}

EventContext::EventContext(int max_outcomes, int n_predicates, const SymbolSet *predicates) :
   MAX_OUTCOMES(max_outcomes), _outcome_counts(0), _outcomes(0), _predicates(0), 
   _n_predicates(n_predicates), _total_count(0), next(0)
{
	int i;

	/* copy array of predicates */
	_predicates = _new SymbolSet[n_predicates];
	for (i = 0; i < n_predicates; i++)
		_predicates[i] = predicates[i];

	/* initialize new array of counts for each outcome */
	_n_outcomes = 0;
	_outcomes = _new Symbol[MAX_OUTCOMES];
	_outcome_counts = _new int[MAX_OUTCOMES];
	for (i = 0; i < MAX_OUTCOMES; i++) {
		_outcomes[i] = Symbol();
		_outcome_counts[i] = 0;
	}
}

EventContext::EventContext(const EventContext &other) : 
	MAX_OUTCOMES(0), _outcome_counts(0), _outcomes(0), _predicates(0), 
	_n_predicates(0), _total_count(0), next(0) 
{
	(*this) = other;
}

const EventContext & EventContext::operator=(const EventContext &other) {
	int i;

	if (this != &other) {
	
		if (_n_predicates != other._n_predicates) {
			delete [] _predicates;
			_n_predicates = other._n_predicates;
			_predicates = _new SymbolSet[_n_predicates];
		}

		/* copy array of predicates */
		for (i = 0; i < _n_predicates; i++)
			_predicates[i] = other._predicates[i];

		if (MAX_OUTCOMES != other.MAX_OUTCOMES) {
			delete [] _outcome_counts;
			delete [] _outcomes;
			MAX_OUTCOMES = other.MAX_OUTCOMES;
			_outcome_counts = _new int[MAX_OUTCOMES];
			_outcomes = _new Symbol[MAX_OUTCOMES];
		}

		_n_outcomes = other._n_outcomes;
		/* copy array of counts for each outcome */
		for (i = 0; i < _n_outcomes; i++) {
			_outcomes[i] = other._outcomes[i];
			_outcome_counts[i] = other._outcome_counts[i];
		}

		_total_count = other._total_count;

		next = 0; // we do *not* have the same chain successor as x
	}

	return *this;
}

EventContext::~EventContext() {
	delete [] _outcomes;
	delete [] _outcome_counts;   
	delete [] _predicates;
	delete next;
}

bool EventContext::same(int n_predicates, const SymbolSet *predicates) const {
	if (_n_predicates == n_predicates) {
		for (int i = 0; i < _n_predicates; i++) {
			if (_predicates[i] != predicates[i])
				return false;
		}
		return true;
	}
	else
		return false;
}

void EventContext::increment(Symbol outcome, int count) {
	bool found = false;
	for (int i = 0; i < _n_outcomes; i++) {
		if (outcome == _outcomes[i]) { 
			_outcome_counts[i] += count;
			found = true;
			break;
		}
	}
	// if not found, create new outcome
	if (!found) {
		if (_n_outcomes == MAX_OUTCOMES)
			throw InternalInconsistencyException("EventContext::increment()", "Outcomes exceeded MAX_OUTCOMES");
		_outcomes[_n_outcomes] = outcome;
		_outcome_counts[_n_outcomes++] = count;
	}
	_total_count += count;
}

void EventContext::merge(EventContext *old_entry) {
	for (int i = 0; i < old_entry->_n_outcomes; i++) {
		bool found = false;
		for (int j = 0; j < _n_outcomes; j++) {
			if (old_entry->_outcomes[i] == _outcomes[j]) {
				_outcome_counts[j] += old_entry->_outcome_counts[i];
				found = true;
				break;
			}
		}
		if (!found) {
			if (_n_outcomes == MAX_OUTCOMES)
				throw InternalInconsistencyException("EventContext::merge()", "Outcomes exceeded MAX_OUTCOMES");
			_outcomes[_n_outcomes] = old_entry->_outcomes[i];
			_outcome_counts[_n_outcomes++] = old_entry->_outcome_counts[i];
		}
	}
	_total_count += old_entry->_total_count;
}

int EventContext::getOutcomeCount(int i) const { 
	if (i >= 0 && i < _n_outcomes)
		return _outcome_counts[i]; 
	else
		throw InternalInconsistencyException("EventContext::getOutcomeCount()", "Array index out of bounds.");
}

Symbol EventContext::getOutcome(int i) const { 
	if (i >= 0 && i < _n_outcomes)
		return _outcomes[i]; 
	else
		throw InternalInconsistencyException("EventContext::getOutcome()", "Array index out of bounds.");
}

SymbolSet EventContext::getPredicate(int i) const { 
	if (i >= 0 && i < _n_predicates)
		return _predicates[i]; 
	else
		throw InternalInconsistencyException("EventContext::getPredicate()", "Array index out of bounds.");
}

void EventContext::setOutcomeCount(int i, int val) { 
	if (i >= 0 && i < _n_outcomes)
		_outcome_counts[i] = val; 
	else
		throw InternalInconsistencyException("EventContext::setOutcomeCount()", "Array index out of bounds.");
}

void EventContext::dump(UTF8OutputStream &out) const {
	int i;
	out << "* Event entry for context:";
	for (i = 0; i < _n_predicates; i++)
		out << " " << _predicates[i] << "; ";
	out << "\nOutcome counts:";
	for (i = 0; i < _n_outcomes; i++)
		out << " " << _outcomes[i].to_string() << ":" << _outcome_counts[i] << "; ";
	out << "Total: " << _total_count << "\n";
}

void EventContext::dump(DebugStream &out) const {
	int i;
	out << "* Event entry for context:";
	for (i = 0; i < _n_predicates; i++)
		out << " " << _predicates[i].to_string() << "; ";
	out << "\nOutcome counts:";
	for (i = 0; i < _n_outcomes; i++)
		out << " " << _outcomes[i].to_string() << ":" << _outcome_counts[i] << "; ";
	out << "Total: " << _total_count << "\n";
}

EventTable::EventTable(int init_size, int max_outcomes)
 : _size(init_size), _n_bins(0), _n_events(0)
{
	_n_outcomes = max_outcomes; 

	_slots = _new EventContext*[_size];
	for (int i = 0; i < _size; i++)
		_slots[i] = 0;
}

EventTable::~EventTable() {
	for (int i = 0; i < _size; i++)
		delete _slots[i]; // destructor for EventContext recursively delete its successors

	delete[] _slots;
}


// Note: Requires that the predicates occur in a consistent ordering accross
// all calls to add.  Otherwise, counts for context sets that are identical 
// in all but ordering will not be merged.
void EventTable::add(Symbol outcome, int n_predicates, const SymbolSet *predicates,
					 int count)
{
	int slot = hash(n_predicates, predicates);
	EventContext **pentry = &(_slots[slot]);
	while (*pentry !=0) {
		if ((*pentry)->same(n_predicates, predicates))
			break;
		pentry = &((*pentry)->next);
	}
	if (*pentry == 0) {
		*pentry = _new EventContext(_n_outcomes, n_predicates, predicates);
		_n_bins++;
	}
	(*pentry)->increment(outcome, count);
	_n_events += count;
}

void EventTable::merge(EventContext *old_entry, int n_predicates, const SymbolSet *predicates) {
	int slot = hash(n_predicates, predicates);
	EventContext **pentry = &(_slots[slot]);
	while (*pentry != 0) {
		if ((*pentry)->same(n_predicates, predicates))
			break;
		pentry= &((*pentry)->next);

	}
	if (*pentry == 0) {
		*pentry = _new EventContext(_n_outcomes, n_predicates, predicates);
		_n_bins++;
	}
	(*pentry)->merge(old_entry);
	_n_events += old_entry->_total_count;
}

int EventTable::hash(int n_predicates, const SymbolSet *predicates) {
	unsigned int val = 0;
	for (int i = 0; i < n_predicates; i++)
		val = (val << 2) + (unsigned int)predicates[i].hash_value();
	return val % _size;
}

void EventTable::dump(UTF8OutputStream &out) const {
	for (int i = 0; i < _size; i++) {
		EventContext *entry = _slots[i];
		if (entry != 0) {
			out << "Slot " << i << ":\n";
			while (entry != 0) {
				entry->dump(out);
				entry = entry->next;
			}
		}
	}
}

void EventTable::dump(DebugStream &out) const {
	for (int i = 0; i < _size; i++) {
		EventContext *entry = _slots[i];
		if (entry != 0) {
			out << "Slot " << i << ":\n";
			while (entry != 0) {
				entry->dump(out);
				entry = entry->next;
			}
		}
	}
}

ContextIter::ContextIter(EventTable *table) : _table(table), _slot(0), _entry(0) {}

EventContext *ContextIter::findNext() {
	if (_entry == 0) {
		/* either this is the first call or we're at the end of a chain, so 
		 * find next non-empty slot */
		for (; _slot < _table->_size; _slot++) {
			if (_table->_slots[_slot] != 0) {
				_entry = _table->_slots[_slot];
				return _entry;
			}
		}
		/* no non-empty slots found, so set entry to 0 */
		_entry = 0;
		return _entry;
	}
	else {
		/* entry is not 0, so follow chain to next entry */
		_entry = _entry->next;

		/* if at end of chain, try again, starting at next slot */
		if (_entry == 0) {
			_slot++;
			// except of course if we've moved past the last slot:
			if (_slot >= _table->_size)
				return 0;
			return findNext();
		}
		else return _entry;
	}
}

EventIter::EventIter(const OldMaxEntEventSet *eventSet, EventContext *context)
 : _context(context), _eventSet(eventSet), _outcome_index(-1), _event(0)
{
	_event = _new OldMaxEntEvent(_eventSet->_features->get_size());
}

 EventIter::~EventIter() { delete _event; }


 OldMaxEntEvent* EventIter::findNext() {
	_outcome_index++;
	if (_outcome_index < _context->_n_outcomes) {
		_event->reset();
		_event->setOutcome(_context->_outcomes[_outcome_index]);
		_event->setCount(_context->_outcome_counts[_outcome_index]);
		for (int i = 0; i < _context->getNPredicates(); i++) {
			if (_eventSet->_features->lookup(_event->getOutcome(), _context->_predicates[i]))
				_event->addContextPredicate(_context->_predicates[i]);
		}
	}
	return _event;
}

bool EventIter::done() { 
	return (_outcome_index == _context->_n_outcomes); 
}


