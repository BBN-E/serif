#include "boost/foreach.hpp"
#include "boost/make_shared.hpp"
#include "Generic/common/Offset.h"
#include "Generic/common/bsp_declare.h"
#include "Generic/common/SessionLogger.h"
#include "PredFinder/elf/ElfRelation.h"
#include "PredFinder/inference/EIUtils.h"
#include "boost/tuple/tuple.hpp"
#include <set>
#include <string>
#include <vector>
#include <map>

#include "PredFinder/inference/EIFilter.h"

BSP_DECLARE(EIFilter);
BSP_DECLARE(EIDocData);
#pragma once

typedef std::map<std::wstring,std::vector<std::wstring> > SuperfilterLoadMap;

/**
 * This class controls the execution of the EIFilters. 
 * After all filters and superfilers (subsequences of filters) are registered,
 * one loads the filter order giving a sequence file (line delimited sequence of filters to run)
 * and then call processFiltersSequence to run through the sequence. (Dependency is not yet supported)
 *
 * @author mshafir@bbn.com
 * @date 2012.1.11
 */
class EIFilterManager {
public:
	EIFilterManager() {}
	~EIFilterManager() {}

	void registerFilter(EIFilter* filter);
	void registerSuperFilters(std::vector<std::string> paths);

	void loadFilterOrder(std::string inputFilename, bool dependency=false);
	void processFiltersSequence(EIDocData_ptr docData);
	void processFiltersDependency(EIDocData_ptr docData);

private:
	bool addFilterToStructure(std::wstring filterName, bool dependency);
	// Recursively loads the subfilters for superfilters so as to allow superfilters to build off each other
	std::vector<EIFilter_ptr> getFilters(std::wstring name, SuperfilterLoadMap others, std::set<std::wstring> traversal);

	bool logValidStructure();

	void getMetFilters(std::map<std::wstring,int> &completed, std::map<EIFilter_ptr,int> &remaining, 
						std::set<EIFilter_ptr> &relationFilters, std::set<EIFilter_ptr> &individualFilters, std::set<EIFilter_ptr> &processAllFilters);

	void applyChanges(EIDocData_ptr docData, std::set<ElfRelation_ptr> relationsToAdd, std::set<ElfRelation_ptr> relationsToRemove, 
						std::set<ElfIndividual_ptr> individualsToAdd, std::set<ElfIndividual_ptr> individualsToRemove, std::wstring source=L"EIFilterManager");
	void updateChanges(EIFilter_ptr filter, std::set<ElfRelation_ptr> &relationsToAdd, std::set<ElfRelation_ptr> &relationsToRemove, 
						std::set<ElfIndividual_ptr> &individualsToAdd, std::set<ElfIndividual_ptr> &individualsToRemove);
	void cleanupFilters(std::set<EIFilter_ptr> filters);
	void cleanupFilter(EIFilter_ptr filter);

	std::map<std::wstring, std::vector<EIFilter_ptr> > _superfilters;
	std::map<EIFilter_ptr,int> _active_filters;
	std::map<std::wstring, EIFilter_ptr> _filters;
	std::vector<EIFilter_ptr> _filter_order;

};
