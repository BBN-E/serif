// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/theories/Lexicon.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/state/XMLTheoryElement.h"

#include <algorithm>
#include <boost/foreach.hpp>

Lexicon::Lexicon(size_t starting_size){
	_start_size = starting_size;

	//the entries stored by their key (symbol) value 
	_entries_by_symbol_static = _new SymbolHashMap;
	_entries_by_symbol_dynamic = _new SymbolHashMap;
}

Lexicon::~Lexicon(){
	BOOST_FOREACH(LexicalEntry* entry, _entries_by_id)
		delete entry;
	delete _entries_by_symbol_static;
	delete _entries_by_symbol_dynamic;

};

Lexicon* Lexicon::_sessionLexicon = 0;
Lexicon *Lexicon::getSessionLexicon() {
	if (!_sessionLexicon) {
		_sessionLexicon = _factory()->build();
	}
	return _sessionLexicon;
}

void Lexicon::destroySessionLexicon() {
	delete _sessionLexicon;
	_sessionLexicon = 0;
}

boost::shared_ptr<Lexicon::Factory> &Lexicon::_factory() {
	static boost::shared_ptr<Lexicon::Factory> factory(new DefaultLexiconFactory());
	return factory;
}

//returns the number of entries with the given key and 
//fills the result array with the corresponding entries
//MRK: fixed
int Lexicon::getEntriesByKey(Symbol key, LexicalEntry** result, int size){
	int nResults = 0;

	// add static matches
    nResults = addEntriesByKey(key, result, size, nResults, true);
	// add dynamic matches
    nResults = addEntriesByKey(key, result, size, nResults, false);
	return nResults;
}

// add results from either the static or dynamic table to the "results" array, up to a max of maxResults, 
// and starting at the index "firstIndex". Return the *total* resulting size of the array.
// MRK: fixed
int Lexicon::addEntriesByKey(Symbol key, 
							 LexicalEntry** results, 
							 int maxResults, 
							 int firstIndex, 
							 bool useStaticTable) 
{
	int count = firstIndex;

	SymbolHashMap *table = (useStaticTable ? _entries_by_symbol_static : _entries_by_symbol_dynamic);
	bool hasKey = (table->get(key) != NULL);

	if (hasKey){
		BOOST_FOREACH(LexicalEntry *entry, (*table)[key]) {
			if (entry == NULL) {
				throw UnrecoverableException("Lexicon::getEntriesByKey", 
					std::string("NULL entry inside array for key: ") + key.to_debug_string());
			}
			if(count >= maxResults){
				//skip lexical entries
				//Todo: log in session logger
			}
			else results[count++] = entry;
		}
	}
	return count;
}

// MRK: fixed
int Lexicon::getNEntriesByKey(Symbol key){
	int nResults = 0;

	if(_entries_by_symbol_static->get(key) != NULL)
		nResults += static_cast<int>((*_entries_by_symbol_static)[key].size());

	if(_entries_by_symbol_dynamic->get(key) != NULL)
		nResults += static_cast<int>((*_entries_by_symbol_dynamic)[key].size());

	return nResults;
}

// MRK: fixed
LexicalEntry* Lexicon::getEntryByID(size_t id){
	
	if(id < MAX_ENTRIES){
		return _entries_by_id[id];
	}else{
		return NULL;
	}
}

size_t Lexicon::getNextID(){
	return _entries_by_id.size();
}


void Lexicon::addStaticEntryDBG(LexicalEntry* le) {
	SessionLogger::info("SERIF")<<"In addStaticEntryDBG() "<<std::endl;
	SessionLogger::info("SERIF")<<"Current Entries_by_symbol_static size: "<<static_cast<int>(_entries_by_symbol_static->size())<<std::endl;

	// make sure we're still populating the static part of the array
	if (le->getID() >= _start_size){
		SessionLogger::err("SERIF")<<"Throw Inconsistent Exception "<<std::endl;
		std::stringstream message;
		message << "LexicalEntry ID, " << le->getID() << ", is not a static ID.";
		throw UnrecoverableException("Lexicon::addStaticEntry", message.str());
	}

	addEntryDBG(le, true);
}

void Lexicon::addDynamicEntryDBG(LexicalEntry* le) {
	SessionLogger::info("SERIF")<<"In addDynamicEntryDBG() "<<std::endl;
	SessionLogger::info("SERIF")<<"Current Entries_by_symbol_dynamic size: "<<static_cast<int>(_entries_by_symbol_dynamic->size())<<std::endl;

	// make sure we're populating the dynamic part of the array
	if (le->getID() < _start_size){
		SessionLogger::err("SERIF")<<"Throw Inconsistent Exception "<<std::endl;
		std::stringstream message;
		message << "LexicalEntry ID, " << le->getID() << ", is not a dynamic ID.";
		throw UnrecoverableException("Lexicon::addDynamicEntry", message.str());
	}

    addEntryDBG(le, false);
}

// MRK: fixed
void Lexicon::addEntryDBG(LexicalEntry* le, bool isStaticEntry) throw (UnrecoverableException){
	SessionLogger::info("SERIF")<<"In addEntryDBG() "<<std::endl;
	if (le->getID() != _entries_by_id.size()){
		std::cerr<<"Throw Inconsistent Exception "<<std::endl;
		std::stringstream message;
		message << "LexicalEntry ID, " << le->getID() << ", not allowed. Please choose use, " << _entries_by_id.size();
		throw UnrecoverableException("Lexicon::addEntry", message.str());
	}else if (le->getID() >= MAX_ENTRIES){
		SessionLogger::err("SERIF")<<"Throw Too high Exception "<<std::endl;
		std::stringstream message;
		message << "LexicalEntry ID, " << le->getID() << ", exceeds maximum number of entries allowed, " << MAX_ENTRIES;
//this wasn't caught???	
		SessionLogger::err("SERIF")<<"Exception: "<<message.str()<<std::endl;
		throw UnrecoverableException("Lexicon::addEntry", message.str());
	}else{
		SessionLogger::info("SERIF")<<"adding lexentry id: "<<static_cast<int>(le->getID())<<std::endl;
		_entries_by_id.push_back(le);
		SessionLogger::info("SERIF")<<"added to entries by id"<<std::endl;

		SymbolHashMap *table = (isStaticEntry ? _entries_by_symbol_static : _entries_by_symbol_dynamic);
		bool hasKey = (table->get(le->getKey()) != NULL);

		//if the key exists
		if(hasKey){
			SessionLogger::info("SERIF")<<"try adding to known key"<<std::endl;
			if((*table)[le->getKey()].size() >= MAX_ENTRIES_PER_SYMBOL){
				std::stringstream message;
				message << "LexicalEntry with symbol, " << le->getKey().to_debug_string() << ", cannot be added to array exceeding maximum number of entries per key, " << MAX_ENTRIES_PER_SYMBOL;
				SessionLogger::info("SERIF")<<"too many exceptions"<<std::endl;
				throw UnrecoverableException("Lexicon::addEntry", message.str());
			}else{
				(*table)[le->getKey()].push_back(le);
			}
			SessionLogger::info("SERIF")<<"added"<<std::endl;
		//else if the key doesn't exist yet
		}else{
			SessionLogger::info("SERIF")<<"try adding to unknown key"<<std::endl;
			SessionLogger::info("SERIF")<<"key: "<<le->getKey().to_debug_string()
				<<" HashCode: "<<static_cast<int>(le->getKey().hash_code())<<std::endl;
			SessionLogger::info("SERIF")<<"Current Entries_by_symbol size: "<<static_cast<int>(table->size())<<std::endl;
			
			//add the new lexical item
			(*table)[le->getKey()].push_back(le);
			SessionLogger::info("SERIF")<<"added"<<std::endl;

		}		
	}

	return;
}

void Lexicon::addStaticEntry(LexicalEntry* le) throw (UnrecoverableException){
	// make sure we're still populating the static part of the array
	if (le->getID() >= _start_size){
		SessionLogger::err("SERIF")<<"Throw Inconsistent Exception "<<std::endl;
		std::stringstream message;
		message << "LexicalEntry ID, " << le->getID() << ", is not a static ID.";
		throw UnrecoverableException("Lexicon::addStaticEntry", message.str());
	}

	addEntry(le, true);
}

void Lexicon::addDynamicEntry(LexicalEntry* le) throw (UnrecoverableException){
	// make sure we're populating the dynamic part of the array
	if (le->getID() < _start_size){
		SessionLogger::err("SERIF")<<"Throw Inconsistent Exception "<<std::endl;
		std::stringstream message;
		message << "LexicalEntry ID, " << le->getID() << ", is not a dynamic ID.";
		throw UnrecoverableException("Lexicon::addDynamicEntry", message.str());
	}

	addEntry(le, false);
}

//TODO
void Lexicon::clearDynamicEntries() {
	//// delete and reinstantiate the dynamic symbol hash
	//delete _entries_by_symbol_dynamic;
	//_entries_by_symbol_dynamic = _new SymbolHashMap(MAX_DYNAMIC_ENTRIES);

	// deleting the whole hash is too slow. instead remove all the elements.
	_entries_by_symbol_dynamic->clear();

	// delete the dynamic part of the array
	_entries_by_id.erase(_entries_by_id.begin()+_start_size, _entries_by_id.end());
}

// MRK: fixed
void Lexicon::addEntry(LexicalEntry* le, bool isStaticEntry) throw (UnrecoverableException){

	// if it's not the correct next id, it's an error
	if (le->getID() != getNextID()){
		SessionLogger::err("SERIF")<<"Throw Inconsistent Exception "<<std::endl;
		std::stringstream message;
		message << "LexicalEntry ID, " << le->getID() << ", not allowed. Please choose use, " << getNextID();  
		throw UnrecoverableException("Lexicon::addEntry", message.str());
	}
	// if we don't have room for another entry, it's an error
	else if (le->getID() >= MAX_ENTRIES){
		SessionLogger::err("SERIF")<<"Throw Too high Exception "<<std::endl;
		std::stringstream message;
		message << "LexicalEntry ID, " << le->getID() << ", exceeds maximum number of entries allowed, " << MAX_ENTRIES;  
		//this wasn't caught???	
		SessionLogger::err("SERIF")<<"Exception: "<<message.str()<<std::endl;
		throw UnrecoverableException("Lexicon::addEntry", message.str());
	}
	// else we're ok
	else{
		// add to array
		// regardless of static or dynamic we add to the same array
		_entries_by_id.push_back(le);

		// add to table
		// specifically, add to the static or the dynamic table
		SymbolHashMap *table = (isStaticEntry ? _entries_by_symbol_static : _entries_by_symbol_dynamic);
		bool hasKey = (table->get(le->getKey()) != NULL);
        
		//if the key exists
		if(hasKey) {
			if((*table)[le->getKey()].size() >= MAX_ENTRIES_PER_SYMBOL){
				std::stringstream message;
				message << "LexicalEntry with symbol, " << le->getKey().to_debug_string() << ", cannot be added to array exceeding maximum number of entries per key, " << MAX_ENTRIES_PER_SYMBOL;  
				SessionLogger::info("SERIF")<<"skipping entry: "<<message.str()<<std::endl;
				//throw UnrecoverableException("Lexicon::addEntry", message.str());
			}else{
				(*table)[le->getKey()].push_back(le);
			}
		//else if the key doesn't exist yet
		}else{
			//add the new lexical item
			(*table)[le->getKey()].push_back(le);

		}		
	}

	return;
}

bool Lexicon::hasID(size_t id){
	return (id < _entries_by_id.size());
}

bool Lexicon::isDynamicEntry(size_t id) {
	return hasID(id) && (id >= _start_size);
}

bool Lexicon::isStaticEntry(size_t id) {
	return (id < _start_size);
}

//MRK: commented out because it wasn't used
/*
void Lexicon::removeAddedEntries()throw (UnrecoverableException){
	if(!_clear_lexicon){
		throw UnrecoverableException("Lexicon::removeAddedEntries",
			"Try to remove added entries without setting _clear_lexicion");
	}
	if(_n_entries < (MAX_ENTRIES - 1000)){
		return;
	}
	UTF8OutputStream uos;
	char buffer[500];
	std::cout<<"Writing Old Lexicon"<<std::endl;
	//sprintf(buffer,"\\\\traid02\\users\\mfreedman\\lexicon.%d.txt",_reset_counter);
	sprintf(buffer,"C:\\clitic_sep\\lexicon.%d.txt",_reset_counter);
	uos.open(buffer);
	dump(uos);
	uos.close();
	std::cout<<"Removing Lexical Entries"<<std::endl;
	SymbolHashMap::iterator startIt = _entries_by_symbol->begin();
	SymbolHashMap::iterator endIt = _entries_by_symbol->end();
	Symbol remove_list[MAX_SYMBOLS];
	int removecount =0;
	for(SymbolHashMap::iterator it = startIt; it!=endIt; ++it){
		SymbolHashEntry* ents = &((SymbolHashEntry)(*it).second);
		int nent = ents->n_entries;
		for(int i=0; i<nent;i++){
			if(ents->entries[i]->getID() >_start_size){
				ents->entries[i] = NULL;
				ents->n_entries--;
			}
		}
		if(ents->n_entries == 0){
			remove_list[removecount] = (Symbol)(*it).first;
			removecount++;
		}
	}
	for(int i=0; i<removecount; i++){
		_entries_by_symbol->remove(remove_list[i]);
	}
	size_t s= _start_size;
	size_t end = _n_entries;
	for(size_t i=s;  i<end; i++){
		LexicalEntry* le = _entries_by_id[i];
		delete _entries_by_id[i];
	}
	_n_entries = _start_size;
	std::cout<<"Writing New Lexicon"<<std::endl;
	//sprintf(buffer,"\\\\traid02\\users\\mfreedman\\new_lexicon.%d.txt",_reset_counter);
	sprintf(buffer,"C:\\clitic_sep\\new_lexicon.%d.txt",_reset_counter);
	uos.open(buffer);
	dump(uos);
	uos.close();
	_reset_counter++;
	std::cout<<"done with reset"<<std::endl;



}
*/

// MRK: fixed
bool Lexicon::hasKey(Symbol key){
	return 
		(_entries_by_symbol_static->get(key) != NULL) ||
		(_entries_by_symbol_dynamic->get(key) != NULL);
}

size_t Lexicon::getNEntries(){
	return _entries_by_id.size();
}

LexicalEntry* Lexicon::findEntry(LexicalEntry* entry) {
	LexicalEntry *static_le = findEntryInTable(entry, _entries_by_symbol_static);
	if (static_le != NULL) 
		return static_le;
	else  
		return findEntryInTable(entry, _entries_by_symbol_static);
}

namespace {
	// Private class used by findEntryInTable to search for a lexical entry.
	struct LexicalEntryEquals {
		LexicalEntry* target;
		LexicalEntryEquals(LexicalEntry* target): target(target) {}
		bool operator()(LexicalEntry* entry) {
			return (*target) == (*entry);
		}
	};
}

LexicalEntry* Lexicon::findEntryInTable(LexicalEntry* entry, SymbolHashMap* table) {
	std::vector<LexicalEntry*> &row = (*table)[entry->getKey()];
	std::vector<LexicalEntry*>::iterator it = std::find_if(row.begin(), row.end(), LexicalEntryEquals(entry));
	if (it == row.end()) return 0;
	else return *it;
}

void Lexicon::dump(UTF8OutputStream &uos){
	uos << (int)	getNEntries() << "\n";
	
	//initialize the array to 0
	BOOST_FOREACH(LexicalEntry* entry, _entries_by_id) {
		entry->dump(uos);
	}
}

//MRK: ok
void Lexicon::dumpDynamicVocab(UTF8OutputStream &uos){
	size_t new_entries = getNEntries() - _start_size;
	if(new_entries > 0){
		uos<< static_cast<int>(new_entries)<<"\n";
		for(size_t entry = _start_size; entry < getNEntries(); entry++){
			_entries_by_id[entry]->dump(uos);
		}
	}
}

void Lexicon::updateObjectIDTable() const {
	throw InternalInconsistencyException("Lexicon::updateObjectIDTable()",
										"Using unimplemented method.");
}

void Lexicon::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("Lexicon::saveState()",
										"Using unimplemented method.");

}

void Lexicon::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("Lexicon::resolvePointers()",
										"Using unimplemented method.");

}

const wchar_t* Lexicon::XMLIdentifierPrefix() const {
	return L"lexicon";
}

void Lexicon::saveXML(SerifXML::XMLTheoryElement lexiconElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("Lexicon::saveXML", "Expected context to be NULL");
	SessionLogger::warn("unimplemented_method") << "saveXML method not filled in yet!" << std::endl; // [XX] FILL THIS IN [XX]
}
