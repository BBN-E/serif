/**
 * Utility class for tracking relation/individual
 * counts throughout.
 *
 * @file PredFinderCounts.h
 * @author gkratkiew@bbn.com
 **/

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"

#include "PredFinderCounts.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include <iostream>
#include <cassert>

using namespace std;


PredFinderCounts* PredFinderCounts::getPredFinderCounts(bool sort_output) {
	PredFinderCounts* pi;
	if (sort_output) {
		pi = new PredFinderCountsSort;
	} else {
		pi = new PredFinderCountsNonsort;
	}
	pi->start();
	return pi;
}

void PredFinderCounts::doc_start_message(const DocTheory* doc_theory) {
	wostringstream local_ostr;
	local_ostr << "Processing " << doc_theory->getDocument()->getName() << "...\n";
	SessionLogger::info("doc_info_start") << local_ostr.str();

	_ostr.str(L"");
}

void PredFinderCounts::add_total(const ElfDocument_ptr elf_info, int type) {
	_totals_cr.Add(type, elf_info->get_n_relations(), elf_info->get_n_individuals());
}

void PredFinderCounts::titles() {
	_ostr.str(L"");
	write_doc_id(L"DOCUMENT_ID");
	write_rel_and_indiv_count<const char*>("PELF_RL", "PELF_IN");
	write_rel_and_indiv_count<const char*>("SELF_RL", "SELF_IN");
	write_rel_and_indiv_count<const char*>("MSELF_RL", "MSELF_IN");
	write_rel_and_indiv_count<const char*>("RELF_RL", "RELF_IN");
	write_rel_and_indiv_count<const char*>("MRELF_RL", "MRELF_IN");
	SessionLogger::info("doc_title_0") << _ostr.str();
}

void PredFinderCounts::totals() {
	//titles();
	totals(L"TOTAL PELF", "PELF", CountRecord::PELF);
	totals(L"TOTAL SELF", "SELF", CountRecord::SELF);
	totals(L"TOTAL MERGED-SELF", "MERGED-SELF", CountRecord::MERGED_SELF);
	totals(L"TOTAL RELF", "RELF", CountRecord::RELF);
	totals(L"TOTAL MERGED-RELF", "MERGED-RELF", CountRecord::MERGED_RELF);
}

void PredFinderCounts::totals(const wstring & title, const string & tag, const int type) {
	_ostr.str(L"");
	write_doc_id(title);
	write_rel_and_indiv_count<int>(_totals_cr.GetRelationCount(type), _totals_cr.GetIndividualCount(type));
	_ostr << right << std::endl << std::endl;
	SessionLogger::info("total_0") << _ostr.str();
	if (_totals_cr.GetRelationCount(type) == 0) {
		SessionLogger::warn("no_rel_0") << "No " << tag << " relations found!\n";
	} 
	if (type != CountRecord::RELF && type != CountRecord::MERGED_RELF && _totals_cr.GetIndividualCount(type) == 0) {
		SessionLogger::warn("no_ind_0") << "No " << tag << " individuals found!\n";
	}
}

void PredFinderCounts::write_doc_id(const wstring & doc_id) {
	const int ID_WIDTH(35);
	_ostr.width(ID_WIDTH);
	_ostr << left << doc_id;
}

template <class T> 
void PredFinderCounts::write_rel_and_indiv_count(T relation_count, T individual_count) {
	const int NUM_WIDTH(8);
	_ostr << " ";
	_ostr.width(NUM_WIDTH);
	_ostr << right << relation_count;
	_ostr << " ";
	_ostr.width(NUM_WIDTH);
	_ostr << right << individual_count;
}

void PredFinderCountsSort::doc_start(const DocTheory* doc_theory) {
	doc_start_message(doc_theory);

	_current_cr.Reset();
	_current_cr.SetDocId(doc_theory->getDocument()->getName().to_string());
}

void PredFinderCountsSort::add(const ElfDocument_ptr elf_info, int type) {
	add_total(elf_info, type);
	_current_cr.Set(type, elf_info->get_n_relations(), elf_info->get_n_individuals());
}

void PredFinderCountsSort::doc_end() {
	_count_record_set.insert(_current_cr);
}

void PredFinderCountsSort::finish() {
	titles();
	BOOST_FOREACH(CountRecord cr, _count_record_set) {
		_ostr.str(L"");
		write_doc_id(cr.GetDocId());
		write_rel_and_indiv_count<int>(cr.GetRelationCount(CountRecord::PELF), cr.GetIndividualCount(CountRecord::PELF));
		write_rel_and_indiv_count<int>(cr.GetRelationCount(CountRecord::SELF), cr.GetIndividualCount(CountRecord::SELF));
		write_rel_and_indiv_count<int>(cr.GetRelationCount(CountRecord::MERGED_SELF), cr.GetIndividualCount(CountRecord::MERGED_SELF));
		write_rel_and_indiv_count<int>(cr.GetRelationCount(CountRecord::RELF), cr.GetIndividualCount(CountRecord::RELF));
		write_rel_and_indiv_count<int>(cr.GetRelationCount(CountRecord::MERGED_RELF), cr.GetIndividualCount(CountRecord::MERGED_RELF));
		SessionLogger::info("doc_id_1") << _ostr.str();
	}
	SessionLogger::info("doc_id_2") << "\n\n";
	totals();
}

void PredFinderCountsNonsort::doc_start(const DocTheory* doc_theory) {
	doc_start_message(doc_theory);
	write_doc_id(doc_theory->getDocument()->getName().to_string());
}

void PredFinderCountsNonsort::add(const ElfDocument_ptr elf_info, int type) {
	add_total(elf_info, type);
	write_rel_and_indiv_count<int>(elf_info->get_n_relations(), elf_info->get_n_individuals());
}


void PredFinderCountsNonsort::doc_end() {
	_ostr << std::endl;
	SessionLogger::info("doc_info_0") << _ostr.str();
}

