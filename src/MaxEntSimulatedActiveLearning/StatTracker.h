// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STAT_TRACKER_H
#define STAT_TRACKER_H

#include <iostream>

using namespace std;

class StatTracker {

public:
	// EVALUATION COUNTS
	int _correct;
	int _missed;
	int _spurious;
	int _wrong_type;

	StatTracker()
		: _initialMode(false), 
		_correct(0), _missed(0), _spurious(0), _wrong_type(0),
		_numInitialTotal(0), _numInitialNone(0),
		_numTrainingNone(0), _numNoneSkipped(0), _numTrainingTotal(0)
	{
	}

	void setInitialMode(bool value) {
		_initialMode = value;
	}

	// returns the maximum number of NONE relations the model can have at this iteration given 
	// the percentage constraint
	bool canAddNone(int percentage) {
		if (_initialMode)
			return true;  // NONEs are not thrown out for initial training
		else {
			double percent = percentage / 100.0;
			int allowed = (int) (percent * _numTrainingTotal);
			/* printf ("percent is %.2f, total instances is %d, %d are currently none, allowed is %d\n", 
				percent, _numTrainingTotal, _numTrainingNone, allowed);
			*/
			return (_numTrainingNone <= allowed);
		}
	}

	void addedNone() {
		if (_initialMode) {
			_numInitialNone++;
			_numInitialTotal++;
		} else {
			_numTrainingNone++;
			_numTrainingTotal++;
		}
	}

	void addedNonNone() {
		if (_initialMode) {
			_numInitialTotal++;
		} else {
			_numTrainingTotal++;
		}
	}

	void skippedNone() {  // should really not happen in initialMode
		if (!_initialMode)
			_numNoneSkipped++;
	}

	int numTotal() const { 
		return _numInitialTotal + _numTrainingTotal;
	}

	int numTrainingTotal() const {
		return _numTrainingTotal;
	}

	int numInitialTotal() const {
		return _numInitialTotal;
	}

	double getRecall() {
		int denominator = _spurious + _wrong_type + _correct;
		if (denominator == 0)
			return 0;
		else
			return (double) _correct / denominator;
	}

	double getPrecision() {
		int denominator = _spurious + _wrong_type + _correct;
		if (denominator == 0)
			return 0;
		else
			return (double) _correct / denominator;
	}

	double getFMeasure() {
		double denominator = getRecall() + getPrecision();
		if (denominator == 0)
			return 0;
		else
			return (2 * getRecall() * getPrecision()) / denominator;
	}

	void printPerformanceStats(std::ostream& out) {
		out << "CORRECT: " << _correct << "<br>\n";
		out << "MISSED: " << _missed << "<br>\n";
		out << "SPURIOUS: " << _spurious << "<br>\n";
		out << "WRONG TYPE: " << _wrong_type << "<br>\n<br>\n";
		out << "RECALL: " << getRecall() << "<br>\n";
		out << "PRECISION: " << getPrecision() << "<br>\n";
		out << "F-MEASURE: " << getFMeasure() << "<br>\n";
	}

	void printPerformanceStats(UTF8OutputStream& out) {
		out << "CORRECT: " << _correct << "<br>\n";
		out << "MISSED: " << _missed << "<br>\n";
		out << "SPURIOUS: " << _spurious << "<br>\n";
		out << "WRONG TYPE: " << _wrong_type << "<br>\n<br>\n";
		out << "RECALL: " << getRecall() << "<br>\n";
		out << "PRECISION: " << getPrecision() << "<br>\n";
		out << "F-MEASURE: " << getFMeasure() << "<br>\n";
	}

	void printInstanceStats(std::ostream& out) {
		double percentNone = ((double) _numInitialNone / _numInitialTotal) * 100;
		out << "\nINITIAL SEED INSTANCES\n";
		out << "Total instances: " << _numInitialTotal << "\n";
		out << "NONE instances: " << _numInitialNone << "\n";
		out << "Percentage of NONE relations (none were skipped): " << percentNone << "\n";

		percentNone = ((double) _numTrainingNone / _numTrainingTotal) * 100;
		out << "\nTRAINING INSTANCES\n";
		out << "NONE relations skipped in accordance with maxent-sal-max-none-percentage: "
			<< _numNoneSkipped << "\n";
		out << "NONE relations included in training: " << _numTrainingNone << "\n";
		out << "Total relations included in training: " << _numTrainingTotal << "\n";
		out << "Percentage of NONE relations: " << percentNone << "\n";

		percentNone = ((double) (_numTrainingNone + _numInitialNone) / numTotal()) * 100;
		out << "\nOverall percentage of NONE relations: " << percentNone << "\n";
	}

	void printInstanceStats(UTF8OutputStream& out) {
		double percentNone = ((double) _numInitialNone / _numInitialTotal) * 100;
		out << "INITIAL SEED INSTANCES\n";
		out << "Total instances: " << _numInitialTotal << "\n";
		out << "NONE instances: " << _numInitialNone << "\n";
		out << "Percentage of NONE relations (none were skipped): " << percentNone << "\n";

		percentNone = ((double) _numTrainingNone / _numTrainingTotal) * 100;
		out << "\nTRAINING INSTANCES\n";
		out << "NONE relations skipped in accordance with maxent-sal-max-none-percentage: "
			<< _numNoneSkipped << "\n";
		out << "NONE relations included in training: " << _numTrainingNone << "\n";
		out << "Total relations included in training: " << _numTrainingTotal << "\n";
		out << "Percentage of NONE relations: " << percentNone << "\n";

		percentNone = ((double) (_numTrainingNone + _numInitialNone) / numTotal()) * 100;
		out << "\nOverall percentage of NONE relations: " << percentNone << "\n";
	}

private:
	bool _initialMode;    // if true, count separate stats for initial training pool

	// INITIAL SEED INSTANCES ADDED
	int _numInitialTotal;
	int _numInitialNone;  // we don't skip any NONE relations for the initial pool

	// AL INSTANCES ADDED
	int _numTrainingNone;   // number of NONE relations in training set
	int _numNoneSkipped; // number of NONE relations NOT added
	int _numTrainingTotal;
};

#endif
