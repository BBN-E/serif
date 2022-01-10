// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/maxent/MaxEntEventSet.h"
#include "Generic/maxent/MaxEntFeatureTable.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTFeature.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/events/stat/EventAAObservation.h"

const int MaxEntEventSet::INIT_TABLE_SIZE = 50000;
const int MaxEntEventSet::EVENT_TABLE_SIZE = 10007;  // hash table size -- should be prime

MaxEntEventSet::MaxEntEventSet(DTTagSet *tags, DTFeatureTypeSet *featureTypes,
							   bool build_feature_set, const char *vector_file) 
							   : _activeFeatures(0), _eventTable(0), _tagSet(tags), _featureTypes(featureTypes),
							   _build_feature_set(build_feature_set)
{
	_activeFeatures = _new MaxEntFeatureTable(INIT_TABLE_SIZE);
	_eventTable = _new NewEventTable(EVENT_TABLE_SIZE, _tagSet->getNTags());

	_print_vector_data = false;
	if (vector_file != 0 && strcmp(vector_file, "") != 0) {
		_print_vector_data = true;
		_vectorStream.open(vector_file);
	}
}

MaxEntEventSet::~MaxEntEventSet() {
	delete _activeFeatures;
	delete _eventTable;

	if (_print_vector_data)
		_vectorStream.close();
}

void MaxEntEventSet::deallocateFeatures() {
	// deallocate hashing features in event table
	ObsIter iter = observationIter();
	ObservationInfo *info = iter.findNext();
	while (!iter.done()) {
		for (int i = 0; i < info->getNFeatures(); i++) {
			DTFeature *feature = info->getFeature(i);
			feature->deallocate();
		}
		info = iter.findNext();
	}
}



// Adds an event represented by a DTState that has been pre-assembled elsewhere.  
// More general than the other addEvent method, which only knows how to make one kind of DTState for
// a single Observation.  [aga]
void MaxEntEventSet::addEvent(DTState state, int count) {
	int correct_answer = _tagSet->getTagIndex(state.getTag());
	if (correct_answer < 0) {
		throw UnexpectedInputException("MaxEntEventSet::addEven(DTState, int)", "Couldn't recover tag index from Symbol.");
	}
	DTFeature *featureBuffer[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	// Add correct answer features to the active feature set
	if (_build_feature_set) {
		for (int i = 0; i < _featureTypes->getNFeaturesTypes(); i++) {
			int n_features = _featureTypes->getFeatureType(i)->extractFeatures(state, featureBuffer);
			for (int j = 0; j < n_features; j++) {
				// the feature table now owns this feature
				_activeFeatures->add(featureBuffer[j], count);
			}
		}
	}

	// Collect all potential features for hashing this observation
	int max_features = _tagSet->getNTags() * DTFeatureType::MAX_FEATURES_PER_EXTRACTION * _featureTypes->getNFeaturesTypes();
	int n_total_features = 0;
	DTFeature **features = _new DTFeature*[max_features];

	for (int t = 0; t < _tagSet->getNTags(); t++) {
		DTState hashState(_tagSet->getTagSymbol(t), Symbol(), Symbol(), 
			state.getIndex(), state.getObservations());
		for (int i = 0; i < _featureTypes->getNFeaturesTypes(); i++) {
			int n_features = _featureTypes->getFeatureType(i)->extractFeatures(hashState, featureBuffer);
			for (int j = 0; j < n_features; j++) {
				features[n_total_features++] = featureBuffer[j];
			}
		}
	}

	// add event to the event table; 
	// event table now owns the features
	_eventTable->add(correct_answer, n_total_features, features, count);

	if (_print_vector_data) {
		//_vectorStream << obs->toString() << "\n";
		_vectorStream << _tagSet->getTagSymbol(correct_answer).to_string();
		DTState printState(Symbol(L""), Symbol(), Symbol(),
				state.getIndex(), state.getObservations());
		for (int i = 0; i < _featureTypes->getNFeaturesTypes(); i++) {
			int n_features = _featureTypes->getFeatureType(i)->extractFeatures(printState, featureBuffer);
			for (int j = 0; j < n_features; j++) {
				_vectorStream << " (";
				_vectorStream << featureBuffer[j]->getFeatureType()->getName().to_string() << " ";
				featureBuffer[j]->write(_vectorStream);
				_vectorStream << ")";
				featureBuffer[j]->deallocate();
			}
		}
		_vectorStream << "\n";
       // _vectorStream << "------------------------\n";
	}

	delete [] features;
}

void MaxEntEventSet::addEvent(DTObservation *obs, int correct_answer, int count) {
	DTFeature *featureBuffer[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	// Add correct answer features to the active feature set
	
	if (_build_feature_set) {
		DTState state(_tagSet->getTagSymbol(correct_answer), Symbol(), 
			Symbol(), 0, std::vector<DTObservation*>(1, obs));
		for (int i = 0; i < _featureTypes->getNFeaturesTypes(); i++) {
			int n_features = _featureTypes->getFeatureType(i)->extractFeatures(state, featureBuffer);
			for (int j = 0; j < n_features; j++) {
				// the feature table now owns this feature
				_activeFeatures->add(featureBuffer[j], count);
			}
		}
	}

	// Collect all potential features for hashing this observation
	int max_features = _tagSet->getNTags() * DTFeatureType::MAX_FEATURES_PER_EXTRACTION * _featureTypes->getNFeaturesTypes();
	int n_total_features = 0;
	DTFeature **features = _new DTFeature*[max_features];

	for (int t = 0; t < _tagSet->getNTags(); t++) {
		DTState hashState(_tagSet->getTagSymbol(t), Symbol(), Symbol(), 0, std::vector<DTObservation*>(1, obs));
		for (int i = 0; i < _featureTypes->getNFeaturesTypes(); i++) {
			int n_features = _featureTypes->getFeatureType(i)->extractFeatures(hashState, featureBuffer);
			for (int j = 0; j < n_features; j++) {
				features[n_total_features++] = featureBuffer[j];
			}
		}
	}

	// add event to the event table; 
	// event table now owns the features
	_eventTable->add(correct_answer, n_total_features, features, count);

	if (_print_vector_data) {
	
			EventAAObservation temp= *(EventAAObservation*) obs;
			//_vectorStream << temp.getTokenSequence()->toString();
			if(temp.getCandidateValueMention()!=0)
			_vectorStream << "ValueMention" << temp.getCandidateValueMention()->toCasedTextString(temp.getTokenSequence())<< "\n";
			else
				_vectorStream << "Mention" << temp.getCandidateArgumentMention()->node->toDebugTextString()<< "\n";

		_vectorStream << L"(" << _tagSet->getTagSymbol(correct_answer).to_string();
		DTState state(Symbol(L""), Symbol(), Symbol(), 0, std::vector<DTObservation*>(1, obs));
		for (int i = 0; i < _featureTypes->getNFeaturesTypes(); i++) {
			int n_features = _featureTypes->getFeatureType(i)->extractFeatures(state, featureBuffer);
			for (int j = 0; j < n_features; j++) {
				_vectorStream << " (";
				_vectorStream << featureBuffer[j]->getFeatureType()->getName().to_string() << " ";
				featureBuffer[j]->write(_vectorStream);
				_vectorStream << ")";
				featureBuffer[j]->deallocate();
			}
		}
		_vectorStream << ")\n";
		//_vectorStream << "------------------------\n";
	}

	delete [] features;
}

void MaxEntEventSet::prune(int threshold, UTF8OutputStream &debug) {

	// if no need to prune, return
	if (!_build_feature_set)
		return;
	if (threshold <= 1)
		return;

	_activeFeatures->prune(threshold);

	// Recreate the event table after pruning:
	// now that some features have been eliminated, any bins that differ
	// only in features that are no longer active must be merged and any that no longer
	// contain any features must be eliminated.
	NewEventTable *new_table = _new NewEventTable(EVENT_TABLE_SIZE, _tagSet->getNTags());

	int n_new_features;
	int max_features = _tagSet->getNTags() * DTFeatureType::MAX_FEATURES_PER_EXTRACTION * _featureTypes->getNFeaturesTypes();
	DTFeature **new_features = _new DTFeature*[max_features];
	ObsIter iter(_eventTable);
	ObservationInfo *block = iter.findNext();
	while (! iter.done()) {
		n_new_features = 0;
		for (int i = 0; i < block->_n_features; i++) {
			if (_activeFeatures->lookup(block->_features[i]) != 0) 
				new_features[n_new_features++] = block->_features[i];
			else
				block->_features[i]->deallocate();
		}
		//if (n_new_features != 0) {
		new_table->merge(block, n_new_features, new_features);
		/*} else {
		char message[300];
		sprintf(message, "No active features in training observation block after pruning `%s'", _tagSet->get);
		throw UnexpectedInputException("MaxEntEventSet::prune()", 
		"No active features in training observation block after pruning");
		}*/
		block = iter.findNext();
	}
	delete [] new_features;

	delete _eventTable;
	_eventTable = new_table;
}



int MaxEntEventSet::getMaxActiveFeatures() {
	int current_max = 0;
	ObsIter iter(_eventTable);
	ObservationInfo *block = iter.findNext();
	while (!iter.done()) {
		for (int i = 0; i < block->getNOutcomes(); i++) {
			int n_active_features = 0;
			for (int k = 0; k < block->getNFeatures(); k++) {
				DTFeature * feature = block->getFeature(k);
				if (_tagSet->getTagIndex(feature->getTag()) == block->getOutcome(i)) {
					int id = getFeatureID(feature);
					if (id != -1) {
						n_active_features++;
					}
				}
			}	
			if (n_active_features > current_max) 
				current_max = n_active_features;
		}
		block = iter.findNext();
	}
	return current_max;
}

int MaxEntEventSet::getFeatureID(DTFeature *feature) {
	MaxEntEventSet::FeatureIter iter = _activeFeatures->get_element(feature);
	if (iter == _activeFeatures->get_end())
		return -1;
	else
		return (*iter).first->getID();
}

void MaxEntEventSet::dump(UTF8OutputStream &out) const {
	out << L"Event Table:\n";
	out << L"Number of Bins: " << _eventTable->getNBins() << L"\n";
	out << L"Number of Events: " << _eventTable->getNEvents() << L"\n";
	_eventTable->dump(out);
	out << L"\n";
	out << L"Active Feature Table:\n";
	_activeFeatures->print_to_open_stream(out);
	out << L"\n";
}

NewEventTable::NewEventTable(int init_size, int max_outcomes)
: _size(init_size), _n_bins(0), _n_events(0)
{
	_n_outcomes = max_outcomes; 

	_slots = _new ObservationInfo*[_size];
	for (int i = 0; i < _size; i++)
		_slots[i] = 0;
}

NewEventTable::~NewEventTable() {
	for (int i = 0; i < _size; i++)
		delete _slots[i]; // destructor for ObservationInfo recursively delete its successors

	delete[] _slots;
}

void NewEventTable::add(int outcome, int n_features, DTFeature **features, int count) {
	int slot = hash(n_features, features);
	ObservationInfo **pentry = &(_slots[slot]);
	while (*pentry !=0) {
		if ((*pentry)->same(n_features, features))
			break;
		pentry = &((*pentry)->next);
	}
	if (*pentry == 0) {
		*pentry = _new ObservationInfo(_n_outcomes, n_features, features);
		_n_bins++;
	} else {
		for (int i = 0; i < n_features; i++)
			features[i]->deallocate();
	}
	(*pentry)->increment(outcome, count);
	_n_events += count;
}

void NewEventTable::merge(ObservationInfo *old_entry, int n_features, DTFeature **features) {
	int slot = hash(n_features, features);
	ObservationInfo **pentry = &(_slots[slot]);
	while (*pentry != 0) {
		if ((*pentry)->same(n_features, features))
			break;
		pentry= &((*pentry)->next);

	}
	if (*pentry == 0) {
		*pentry = _new ObservationInfo(_n_outcomes, n_features, features);
		_n_bins++;
	} else {
		for (int i = 0; i < n_features; i++)
			features[i]->deallocate();
	}
	(*pentry)->merge(old_entry);
	_n_events += old_entry->_total_count;
}

int NewEventTable::hash(int n_features, DTFeature **features) {
	unsigned int val = 0;
	for (int i = 0; i < n_features; i++)
		val = (val << 2) + (unsigned int)features[i]->getHashCode();
	return val % _size;
}

void NewEventTable::dump(UTF8OutputStream &out) const {
	for (int i = 0; i < _size; i++) {
		ObservationInfo *entry = _slots[i];
		if (entry != 0) {
			out << "Slot " << i << ":\n";
			while (entry != 0) {
				entry->dump(out);
				entry = entry->next;
			}
		}
	}
}

ObservationInfo::ObservationInfo(int max_outcomes, int n_features, DTFeature **features) :
MAX_OUTCOMES(max_outcomes), _outcome_counts(0), _outcomes(0), _features(0), 
_n_features(n_features), _total_count(0), next(0)
{
	int i;

	/* copy array of predicates */
	_features = _new DTFeature*[n_features];
	for (i = 0; i < n_features; i++)
		_features[i] = features[i];

	/* initialize new array of counts for each outcome */
	_n_outcomes = 0;
	_outcomes = _new int[MAX_OUTCOMES];
	_outcome_counts = _new int[MAX_OUTCOMES];
	for (i = 0; i < MAX_OUTCOMES; i++) {
		_outcomes[i] = -1;
		_outcome_counts[i] = 0;
	}
}

ObservationInfo::ObservationInfo(const ObservationInfo &other) : 
MAX_OUTCOMES(0), _outcome_counts(0), _outcomes(0), _features(0), 
_n_features(0), _total_count(0), next(0)
{
	(*this) = other;
}

const ObservationInfo & ObservationInfo::operator=(const ObservationInfo &other) {
	int i;

	if (this != &other) {

		if (_n_features != other._n_features) {
			delete [] _features;
			_n_features = other._n_features;
			_features = _new DTFeature*[_n_features];
		}

		/* copy array of predicates */
		for (i = 0; i < _n_features; i++)
			_features[i] = other._features[i];

		if (MAX_OUTCOMES != other.MAX_OUTCOMES) {
			delete [] _outcome_counts;
			delete [] _outcomes;
			MAX_OUTCOMES = other.MAX_OUTCOMES;
			_outcome_counts = _new int[MAX_OUTCOMES];
			_outcomes = _new int[MAX_OUTCOMES];
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

ObservationInfo::~ObservationInfo() {
	delete [] _outcomes;
	delete [] _outcome_counts;   
	delete [] _features;
	delete next;
}

bool ObservationInfo::same(int n_features, DTFeature **features) const {
	if (_n_features == n_features) {
		for (int i = 0; i < _n_features; i++) {
			if (!((*_features[i]) == (*features[i])))
				return false;
		}
		return true;
	}
	else
		return false;
}

void ObservationInfo::increment(int outcome, int count) {
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
			throw InternalInconsistencyException("ObservationInfo::increment()", "Outcomes exceeded MAX_OUTCOMES");
		_outcomes[_n_outcomes] = outcome;
		_outcome_counts[_n_outcomes++] = count;
	}
	_total_count += count;
}

void ObservationInfo::merge(ObservationInfo *old_entry) {
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
				throw InternalInconsistencyException("ObservationInfo::merge()", "Outcomes exceeded MAX_OUTCOMES");
			_outcomes[_n_outcomes] = old_entry->_outcomes[i];
			_outcome_counts[_n_outcomes++] = old_entry->_outcome_counts[i];
		}
	}
	_total_count += old_entry->_total_count;
}

int ObservationInfo::getOutcomeCount(int i) const { 
	if (i >= 0 && i < _n_outcomes)
		return _outcome_counts[i]; 
	else
		throw InternalInconsistencyException("ObservationInfo::getOutcomeCount()", "Array index out of bounds.");
}

int ObservationInfo::getOutcomeCountByTagID(int i) const {
	int index = 0;
	while (index < _n_outcomes && _outcomes[index] != i)
		index++;
	// i is not in the outcome list
	if (index == _n_outcomes)
		return 0;
	return _outcomes[index];
}

int ObservationInfo::getOutcome(int i) const { 
	if (i >= 0 && i < _n_outcomes)
		return _outcomes[i]; 
	else
		throw InternalInconsistencyException("ObservationInfo::getOutcome()", "Array index out of bounds.");
}

DTFeature* ObservationInfo::getFeature(int i) const { 
	if (i >= 0 && i < _n_features)
		return _features[i]; 
	else
		throw InternalInconsistencyException("ObservationInfo::getFeature()", "Array index out of bounds.");
}

void ObservationInfo::setOutcomeCount(int i, int val) { 
	if (i >= 0 && i < _n_outcomes)
		_outcome_counts[i] = val; 
	else
		throw InternalInconsistencyException("ObservationInfo::setOutcomeCount()", "Array index out of bounds.");
}

void ObservationInfo::dump(UTF8OutputStream &out) const {
	int i;
	out << "* Event entry for block:";
	for (i = 0; i < _n_features; i++) {
		out << " (" << _features[i]->getFeatureType()->getName().to_string() << " ";
		(*_features[i]).write(out); 
		out << "); ";
	}
	out << "\nOutcome counts:";
	for (i = 0; i < _n_outcomes; i++)
		out << " " << _outcomes[i] << ":" << _outcome_counts[i] << "; ";
	out << "Total: " << _total_count << "\n";
}

ObsIter::ObsIter(NewEventTable *table) : _table(table), _slot(0), _entry(0) {}

ObservationInfo *ObsIter::findNext() {
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

