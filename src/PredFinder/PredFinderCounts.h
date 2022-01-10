/**
 * Utility class for tracking relation/individual
 * counts throughout.
 *
 * @file PredFinderCounts.h
 * @author gkratkiew@bbn.com
 **/

#pragma once

#include "PredFinder/elf/ElfDocument.h"
#include <set>
#include <string>
#include <vector>

// Autoversioned preprocessor using; trick
XERCES_CPP_NAMESPACE_USE

class CountRecord {
public:
	static const int NUM_ELES = 5;
	static const int PELF = 0;
	static const int SELF = 1;
	static const int MERGED_SELF = 2;
	static const int RELF = 3;
	static const int MERGED_RELF = 4;
private:
	std::wstring _doc_id;
	int _relation_count[NUM_ELES];
	int _individual_count[NUM_ELES];
public:
	CountRecord()  { Reset(); }
	void SetDocId(std::wstring doc_id) { _doc_id = doc_id; }
	void Set(int type, int relation_count, int individual_count) { _relation_count[type] = relation_count; _individual_count[type] = individual_count; }
	void Reset() {
		for (int n = 0; n < NUM_ELES; ++n) {
			_relation_count[n] = 0; _individual_count[n] = 0;
		}
	}
	void Add(int type, int relation_count, int individual_count) { _relation_count[type] += relation_count; _individual_count[type] += individual_count; }
	std::wstring GetDocId() const {return _doc_id;}
	int GetRelationCount(int type) const {return _relation_count[type];}
	int GetIndividualCount(int type) const {return _individual_count[type];}
};

struct CountRecord_less_than {
	bool operator()(const CountRecord & cr0, const CountRecord & cr1) const
	{
		if (cr0.GetRelationCount(CountRecord::PELF) < cr1.GetRelationCount(CountRecord::PELF)) {
			return true;
		}
		else if (cr0.GetRelationCount(CountRecord::PELF) > cr1.GetRelationCount(CountRecord::PELF)) {
			return false;
		}
		else if (cr0.GetIndividualCount(CountRecord::PELF) < cr1.GetIndividualCount(CountRecord::PELF)) {
			return true;
		}
		else if (cr0.GetIndividualCount(CountRecord::PELF) > cr1.GetIndividualCount(CountRecord::PELF)) {
			return false;
		}
		else if (cr0.GetDocId() < cr1.GetDocId()) {
			return true;
		}
		else if (cr0.GetDocId() > cr1.GetDocId()) {
			return false;
		}
		else {
			return false;
		}
	}
};

class PredFinderCounts {

private:
	CountRecord _totals_cr;
protected:
	std::wostringstream _ostr;

public:

	static PredFinderCounts* getPredFinderCounts(bool sort_output);
    virtual ~PredFinderCounts() { }
	virtual void start() = 0;
	virtual void doc_start(const DocTheory* doc_theory) = 0;
	virtual void add(const ElfDocument_ptr elf_info, int type) = 0;
	virtual void doc_end() = 0;
	virtual void finish() = 0;

	void doc_start_message(const DocTheory* doc_theory);
	void add_total(const ElfDocument_ptr elf_info, int type);
	void titles();
	void totals();
	void totals(const std::wstring & title, const std::string & tag, const int type);
	void write_doc_id(const std::wstring & doc_id);
	template <class T> 
	void write_rel_and_indiv_count(T relation_count, T individual_count);
};

class PredFinderCountsSort: public PredFinderCounts {
private:
	CountRecord _current_cr;
	std::set<CountRecord, CountRecord_less_than> _count_record_set;

public:
	void start() {}
	void doc_start(const DocTheory* doc_theory);
	void add(const ElfDocument_ptr elf_info, int type);
	void doc_end();
	void finish();

};

class PredFinderCountsNonsort: public PredFinderCounts {
public:
	void start() { titles(); }
	void doc_start(const DocTheory* doc_theory);
	void add(const ElfDocument_ptr elf_info, int type);
	void doc_end(); 
	void finish() { totals(); }
};
