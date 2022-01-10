#include "Generic/common/leak_detection.h"

#pragma warning(disable: 4996)

#include "PredFinder/inference/EIFilterManager.h"
#include "PredFinder/inference/EIUtils.h"
#include "PredFinder/inference/EIFilter.h"
#include "PredFinder/inference/EIDocData.h"


void EIFilterManager::registerFilter(EIFilter* filter) {
	_filters[filter->getFilterName()] = boost::shared_ptr<EIFilter>(filter);
}

/**
 * Retrieve just the filename from a full path as a wstring
 **/
std::wstring getName(std::string path) {
	size_t pos = path.find_last_of("/");
	if (pos == std::string::npos)
		pos = path.find_last_of("\\");
	if (pos == std::string::npos) {
		SessionLogger::err("EIFilterManager") << "Invalid path for superfitler: " << path << "\n";
		return L"";
	}

	size_t end_pos = path.find_last_of(".");
	if (end_pos == std::string::npos)
		end_pos = path.length();

	std::string sname(path.substr(pos+1,end_pos-pos-1));
	std::wstring wname;
	wname.assign(sname.begin(),sname.end());
	return wname;
}

/**
 * Reads in the subfilters from the given superfilter file as a vector of wstrings
 **/
std::vector<std::wstring> getSubfilters(std::string filename) {
	std::vector<std::wstring> ret;
	std::wifstream input(filename.c_str());

    if (! input.is_open()) {
		SessionLogger::err("EIFilterManager") << "Can't open superfilter file: " << filename << "\n";
		return ret;
	}
	//Read each line of the file
    while (! input.eof() ) {
		std::wstring buffer;           // Temporary line buffer
		//Get Current Line
		std::getline(input, buffer);
		//Strip leading and trailing white space
		boost::algorithm::trim(buffer);
		// Skip blank lines.
		if (buffer.empty())
			continue;
		// Skip comments.
		if (buffer[0] == '#')
			continue;
		ret.push_back(buffer);
	}
	return ret;
}

//registers all superfilters (subsequences of filters) from the given superfilter files
void EIFilterManager::registerSuperFilters(std::vector<std::string> paths) {
	SuperfilterLoadMap superfilters_to_load;
	BOOST_FOREACH(std::string filename, paths) {
		std::wstring name = getName(filename);
		std::vector<std::wstring> sub = getSubfilters(filename);
		if (name.length() > 0 && sub.size() > 0)
			superfilters_to_load[name] = sub;
	}

	for (SuperfilterLoadMap::iterator it = superfilters_to_load.begin();it!=superfilters_to_load.end();it++) {
		getFilters(it->first,superfilters_to_load,std::set<std::wstring>());
	}
}

//recursively loads superfilters into the filter sequence
std::vector<EIFilter_ptr> EIFilterManager::getFilters(std::wstring name, SuperfilterLoadMap others, std::set<std::wstring> traversal) {
	std::vector<EIFilter_ptr> ret;
	if (traversal.find(name) != traversal.end()) {
		SessionLogger::err("EIFilterManager") << L"Superfilter infinite loop at " << name << L"\n";
		return ret;
	}

	if (_filters.find(name) != _filters.end()) {
		ret.push_back(_filters[name]);
		return ret;
	}

	if (_superfilters.find(name) != _superfilters.end()) {
		BOOST_FOREACH(EIFilter_ptr filter, _superfilters[name])
			ret.push_back(filter);
		return ret;
	}

	if (others.find(name) == others.end()) {
		SessionLogger::err("EIFilterManager") << L"Filter " << name << L" was not registered or found.\n";
		return ret;
	}

	traversal.insert(name);
	std::vector<EIFilter_ptr> to_make;
	BOOST_FOREACH(std::wstring subfilter, others[name]) {
		std::vector<EIFilter_ptr> sub = getFilters(subfilter,others,traversal);
		if (!sub.empty()) {
			BOOST_FOREACH (EIFilter_ptr filt, sub)
				to_make.push_back(filt);
		}
	}
	_superfilters[name] = to_make;
	return to_make;
}

//loads the order of the filters
void EIFilterManager::loadFilterOrder(std::string inputFilename, bool dependency) {
	std::wifstream input(inputFilename.c_str());

    if (! input.is_open()) {
		std::string s = "Can't open filter ordering file: " + inputFilename;
		throw UnexpectedInputException("EIFilterManager::loadFilterOrder()", s.c_str());
		return;
	}

	//Read each line of the file
	std::wstring previousFilter = L"";
    while (! input.eof() ) {
		std::wstring buffer;           // Temporary line buffer
       
		//Get Current Line
		std::getline(input, buffer);

		//Strip leading and trailing white space
		boost::algorithm::trim(buffer);

		// Skip blank lines.
		if (buffer.empty())
			continue;

		// Skip comments.
		if (buffer[0] == '#')
			continue;

		if (!addFilterToStructure(buffer,dependency)) {
			SessionLogger::err("EIFilterManager") << L"Filter " << buffer << L" is not a registered filter! Skipping...\n";
		}
	}

	//check if dependency structure that the structure is valid
	if (dependency && !logValidStructure()) {
		throw std::runtime_error("Invalid Filter Structure: One or more specified filters have dependencies that cannot be met.");
	} else if (!dependency) {
		SessionLogger::info("EIFilterManager") << "Filters will run in the following order:\n";
		BOOST_FOREACH(EIFilter_ptr filt,_filter_order) {
			SessionLogger::info("EIFilterManager") << filt->getOrderPrintout() << L"\n";
		}
	}
}

//for dependency structures (not yet supported)
bool EIFilterManager::addFilterToStructure(std::wstring filterName, bool dependency) {
	if (_filters.find(filterName) != _filters.end()) {
		//Loading a dependency structure is not supported yet. Can be completed if needed. mshafir
		/*if (dependency) { //dependency based
			if (previousFilter.compare(L"") != 0) {
				_filters[buffer]->addPrerequisite(previousFilter,
					_active_filters[_filters[previousFilter]]);
			}
			EIFilter_ptr temp = _filters[buffer];
			if (_active_filters.find(temp) == _active_filters.end())
				_active_filters[temp] = 0;
			_active_filters[temp] += 1;
			previousFilter = buffer; */
		_filter_order.push_back(_filters[filterName]);
		return true;
	} else if (_superfilters.find(filterName) != _superfilters.end()) {
		BOOST_FOREACH (EIFilter_ptr filter, _superfilters[filterName])
			_filter_order.push_back(filter);
		return true;
	} else {
		return false;
	}
}

//logs a dependency structure (not yet supported)
bool EIFilterManager::logValidStructure() {
	std::map<EIFilter_ptr,int> remainingFilters(_active_filters);
	std::map<std::wstring,int> completedFilters;
	SessionLogger::info("EIFilterManager") << "Filter ordering:\n";
	int order = 1;

	while (!remainingFilters.empty()) {
		//get
		std::set<EIFilter_ptr> relationFilters;
		std::set<EIFilter_ptr> individualFilters;
		std::set<EIFilter_ptr> processAllFilters;
		getMetFilters(completedFilters,remainingFilters,relationFilters,individualFilters,processAllFilters);
		if (relationFilters.empty() && individualFilters.empty() && processAllFilters.empty()) {
			//!!error. inaccessible Filters left on stack
			return false;
		}
		//log
		SessionLogger::info("EIFilterManager") << "Batch " << boost::lexical_cast<std::wstring>(order) << ":\n";
		if (!relationFilters.empty()) {
			SessionLogger::info("EIFilterManager") << "\tRelation Filters:\n";
			BOOST_FOREACH(EIFilter_ptr filter,relationFilters) {
				SessionLogger::info("EIFilterManager") << L"\t\t" << filter->getFilterName() << L"\n";
			}
		}
		if (!individualFilters.empty()) {
			SessionLogger::info("EIFilterManager") << "\tIndividual Filters:\n";
			BOOST_FOREACH(EIFilter_ptr filter,individualFilters) {
				SessionLogger::info("EIFilterManager") << L"\t\t" << filter->getFilterName() << L"\n";
			}
		}
		if (!processAllFilters.empty()) {
			SessionLogger::info("EIFilterManager") << "Run Once, Process All Filters:\n";
			BOOST_FOREACH(EIFilter_ptr filter,processAllFilters) {
				SessionLogger::info("EIFilterManager") << L"\t\t" << filter->getFilterName() << L"\n";
			}
		}
		order++;
	}
	return true;
}

//This is the main call that executes all the filters in the sequence.
void EIFilterManager::processFiltersSequence(EIDocData_ptr docData) {
	BOOST_FOREACH(EIFilter_ptr filter, _filter_order) {
		std::set<ElfRelation_ptr> _relationsToAdd;
		std::set<ElfRelation_ptr> _relationsToRemove;
		std::set<ElfIndividual_ptr> _individualsToAdd;
		std::set<ElfIndividual_ptr> _individualsToRemove;

		if (filter->isRelationFilter()) {
			BOOST_FOREACH(ElfRelation_ptr relation, docData->get_relations()) {
				if (filter->matchesRelation(docData, relation)) {
					filter->handleRelation(docData, relation);
				}
			}
		}

		if (filter->isIndividualFilter()) {
			BOOST_FOREACH(ElfIndividual_ptr individual, docData->get_individuals_by_type()) {
				if (filter->matchesIndividual(docData, individual)) {
					filter->handleIndividual(docData, individual);
				}
			}
		}

		if (filter->isProcessAllFilter())
			filter->handleAll(docData);

		updateChanges(filter,_relationsToAdd,_relationsToRemove,_individualsToAdd,_individualsToRemove);
		applyChanges(docData,_relationsToAdd,_relationsToRemove,_individualsToAdd,_individualsToRemove,filter->getDecoratedFilterName());
		cleanupFilter(filter);
	}
}

//not yet fully supported
void EIFilterManager::processFiltersDependency(EIDocData_ptr docData) {
	std::map<EIFilter_ptr,int> remainingFilters(_active_filters);
	std::map<std::wstring,int> completedFilters;

	while (!remainingFilters.empty()) {
		//get
		std::set<EIFilter_ptr> relationFilters;
		std::set<EIFilter_ptr> individualFilters;
		std::set<EIFilter_ptr> processAllFilters;
		getMetFilters(completedFilters,remainingFilters,relationFilters,individualFilters,processAllFilters);

		if (relationFilters.empty() && individualFilters.empty() && processAllFilters.empty()) {
			//!!error. inaccessible Filters left on stack
			break;
		}
	
		std::set<ElfRelation_ptr> _relationsToAdd;
		std::set<ElfRelation_ptr> _relationsToRemove;
		std::set<ElfIndividual_ptr> _individualsToAdd;
		std::set<ElfIndividual_ptr> _individualsToRemove;

		//handle Relations
		if (!relationFilters.empty()) {
			BOOST_FOREACH(ElfRelation_ptr relation, docData->get_relations()) {
				BOOST_FOREACH(EIFilter_ptr filter, relationFilters) {
					if (filter->matchesRelation(docData, relation)) {
						filter->handleRelation(docData, relation);
						updateChanges(filter,_relationsToAdd,_relationsToRemove,_individualsToAdd,_individualsToRemove);
					}
				}
			}
			applyChanges(docData,_relationsToAdd,_relationsToRemove,_individualsToAdd,_individualsToRemove);
			cleanupFilters(relationFilters);
		}

		//handle Individuals
		if (!individualFilters.empty()) {
			BOOST_FOREACH(ElfIndividual_ptr individual, docData->get_individuals_by_type()) {
				BOOST_FOREACH(EIFilter_ptr filter, individualFilters) {
					if (filter->matchesIndividual(docData, individual)) {
						filter->handleIndividual(docData, individual);
						updateChanges(filter,_relationsToAdd,_relationsToRemove,_individualsToAdd,_individualsToRemove);
					}
				}
			}
			applyChanges(docData,_relationsToAdd,_relationsToRemove,_individualsToAdd,_individualsToRemove);
			cleanupFilters(individualFilters);
		}

		//handle run once process all
		if (!processAllFilters.empty()) {
			BOOST_FOREACH(EIFilter_ptr filter, processAllFilters) {
				filter->handleAll(docData);
				updateChanges(filter,_relationsToAdd,_relationsToRemove,_individualsToAdd,_individualsToRemove);
			}
			applyChanges(docData,_relationsToAdd,_relationsToRemove,_individualsToAdd,_individualsToRemove);
			cleanupFilters(processAllFilters);
		}
		
	}
}

void EIFilterManager::applyChanges(EIDocData_ptr docData, std::set<ElfRelation_ptr> relationsToAdd, std::set<ElfRelation_ptr> relationsToRemove, 
								   std::set<ElfIndividual_ptr> individualsToAdd, std::set<ElfIndividual_ptr> individualsToRemove, std::wstring source) {
	//handle the relation changes
	docData->remove_relations(relationsToRemove);
	EIUtils::addNewRelations(docData, relationsToAdd, source);
	//handle the individual changes
	docData->remove_individuals(individualsToRemove);
	BOOST_FOREACH(ElfIndividual_ptr individual, individualsToAdd) {
		docData->getElfDoc()->insert_individual(individual);
	}
}

void EIFilterManager::cleanupFilters(std::set<EIFilter_ptr> filters) {
	BOOST_FOREACH(EIFilter_ptr filter, filters) {
		cleanupFilter(filter);
	}
}

void EIFilterManager::cleanupFilter(EIFilter_ptr filter) {
	filter->clearRelationsToAdd();
	filter->clearRelationsToRemove();
	filter->clearIndividualsToAdd();
	filter->clearIndividualsToRemove();
}

void EIFilterManager::updateChanges(EIFilter_ptr filter, std::set<ElfRelation_ptr> &relationsToAdd,
				   std::set<ElfRelation_ptr> &relationsToRemove, std::set<ElfIndividual_ptr> &individualsToAdd, std::set<ElfIndividual_ptr> &individualsToRemove) {
	BOOST_FOREACH(ElfRelation_ptr rel, filter->getRelationsToAdd())
		relationsToAdd.insert(rel);
	BOOST_FOREACH(ElfRelation_ptr rel, filter->getRelationsToRemove())
		relationsToRemove.insert(rel);
	BOOST_FOREACH(ElfIndividual_ptr ind, filter->getIndividualsToAdd())
		individualsToAdd.insert(ind);
	BOOST_FOREACH(ElfIndividual_ptr ind, filter->getIndividualsToRemove())
		individualsToRemove.insert(ind);
}

bool prereqsMet(std::set<Prerequisite> prereqs, std::map<std::wstring,int> completed) {
	BOOST_FOREACH(Prerequisite p, prereqs) {
		if (completed.find(p.first) == completed.end())
			return false;
		if (completed[p.first] < p.second)
			return false;
	}
	return true;
}

void transferFilterCount(EIFilter_ptr filter, std::map<std::wstring,int> &completed, std::map<EIFilter_ptr,int> &remaining) {
	std::set<int> s;
	
	if (completed.find(filter->getFilterName()) == completed.end()) {
		completed[filter->getFilterName()] = 0;
	}
	completed[filter->getFilterName()] = completed[filter->getFilterName()] + 1;
	remaining[filter] = remaining[filter] - 1;
	if (remaining[filter] == 0) {
		remaining.erase(filter);
	}
}

void EIFilterManager::getMetFilters(std::map<std::wstring,int> &completed, std::map<EIFilter_ptr,int> &remaining, 
								std::set<EIFilter_ptr> &relationFilters, std::set<EIFilter_ptr> &individualFilters, std::set<EIFilter_ptr> &processAllFilters) {
	for (std::map<EIFilter_ptr,int>::iterator iter = remaining.begin(); iter != remaining.end(); iter++) {
		if (prereqsMet(iter->first->getPrerequisites(),completed)) {
			if (iter->first->isRelationFilter())
				relationFilters.insert(iter->first);
			if (iter->first->isIndividualFilter())
				individualFilters.insert(iter->first);
			if (iter->first->isProcessAllFilter())
				processAllFilters.insert(iter->first);
		}
	}
	BOOST_FOREACH(EIFilter_ptr filter, relationFilters) {
		transferFilterCount(filter,completed,remaining);
	}
	BOOST_FOREACH(EIFilter_ptr filter, individualFilters) {
		transferFilterCount(filter,completed,remaining);
	}
	BOOST_FOREACH(EIFilter_ptr filter, processAllFilters) {
		transferFilterCount(filter,completed,remaining);
	}
}

