// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/theories/LexicalEntry.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include <iostream>
#include "Generic/state/XMLStrings.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/morphAnalysis/SessionLexicon.h"
#include <boost/foreach.hpp>

LexicalEntry::LexicalEntry(size_t id, Symbol key, FeatureValueStructure* feats, LexicalEntry** analysis, int analysis_length) throw (UnrecoverableException) {
	_id = id;
	_key = key;
	_feats = feats;

	Symbol empty;
	if(_key == empty){
		throw UnrecoverableException("LexicalEntry::LexicalEntry", "Attempting to instantiate a LexicalEntry with a NULL key");
	}
	if(_feats == NULL){
		throw UnrecoverableException("LexicalEntry::LexicalEntry", "Attempting to instantiate a LexicalEntry with a NULL FeatureValueStructure");
	}

	//copy the matrix and its length
	for(int n_ana = 0; n_ana < analysis_length; n_ana++){
		if(analysis[n_ana] == NULL){
			throw UnrecoverableException("LexicalEntry::LexicalEntry", "Attempting to point to a NULL sub segment");
		}
		_analysis.push_back(analysis[n_ana]);
	}
}
LexicalEntry::~LexicalEntry(){
	delete _feats;
}

LexicalEntry* LexicalEntry::getSegment(size_t segment_number) const {
	return _analysis[segment_number];
}

bool LexicalEntry::operator==(const LexicalEntry &other) const {
	if (other._analysis.size() != _analysis.size() || 
		other._key != _key || 
		(*other._feats) != (*_feats))
		return false;

	for (size_t i = 0; i < _analysis.size(); i++) {
		if (!(*(other._analysis[i]) == (*_analysis[i])))
			return false;
	}

	return true;
}

Symbol LexicalEntry::getKey() const{
	return _key;
}
FeatureValueStructure* LexicalEntry::getFeatures(){
	return _feats;	
}
const FeatureValueStructure* LexicalEntry::getFeatures() const{
	return _feats;	
}
size_t LexicalEntry::getID() const{
	return _id;
}

// Used when deserializing lexical entries from XML.
void LexicalEntry::setID(size_t new_id) {
	_id = new_id;
}


void LexicalEntry::dump(UTF8OutputStream &uos)const {
	uos << (int)_id << " " << _key.to_string() << " ";
	uos.flush();
	_feats->dump(uos);
	uos.flush();
	uos << " (";
	uos << (int)_analysis.size()<<" ";
	for(size_t n_ana = 0; n_ana < _analysis.size(); n_ana++){
		uos << (int)_analysis[n_ana]->getID() << " ";
	}
	uos << ")\n";
	uos.flush();
}
void LexicalEntry::dump(std::ostream &uos) const{
	uos << (int)_id << " " << _key.to_debug_string() << " ";
	uos.flush();
//	uos<<getFeatures()->getCategory().to_debug_string()<<" ";
//	uos<<getFeatures()->getVoweledString().to_debug_string()<<" ";

	uos.flush();
	uos << " (";
	uos << (int)_analysis.size()<<" ";
	for(size_t n_ana = 0; n_ana < _analysis.size(); n_ana++){
		uos << (int)_analysis[n_ana]->getID() << " ";
	}
	uos<<")\n";
	for(size_t n = 0; n < _analysis.size(); n++){
		uos <<"( "<< (int)_analysis[n]->getID() << " ";
		_analysis[n]->dump(uos);
	}
	uos << ")\n";
	uos.flush();
}

std::string LexicalEntry::toString() const {
	std::stringstream result;
	result << (int)_id;
	result << " ";
	result << _key.to_debug_string();
	result << " ";
	
	result << " (";
	result << (int)_analysis.size();
	result << " ";
	for(size_t n_ana = 0; n_ana < _analysis.size(); n_ana++){
		result << (int)_analysis[n_ana]->getID();
		result << " ";
	}
	result << ")\n";
	for(size_t n = 0; n < _analysis.size(); n++){
		result << "(";
		result << (int)_analysis[n]->getID();
		result << " ";
		result << _analysis[n]->toString();
	}
	result << ")\n";
	return result.str();
}

void LexicalEntry::updateObjectIDTable() const {
	throw InternalInconsistencyException("LexicalEntry::updateObjectIDTable()",
										"Using unimplemented method.");
}

void LexicalEntry::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("LexicalEntry::saveState()",
										"Using unimplemented method.");

}

void LexicalEntry::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("LexicalEntry::resolvePointers()",
										"Using unimplemented method.");

}

const wchar_t* LexicalEntry::XMLIdentifierPrefix() const {
	return L"le";
}

void LexicalEntry::saveXML(SerifXML::XMLTheoryElement lexicalEntryElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("LexicalEntry::saveXML", "Expected context to be NULL");
	if (_key != Symbol(L"NULL"))
		lexicalEntryElem.setAttribute(X_key, _key);
	_feats->saveXML(lexicalEntryElem);
	if (!_analysis.empty()) {
		std::vector<const Theory*> segments(_analysis.begin(), _analysis.end());
		lexicalEntryElem.saveTheoryPointerList(X_analysis, segments);
	}
}

LexicalEntry::LexicalEntry(SerifXML::XMLTheoryElement lexicalEntryElem, size_t id): _id(id)
{
	using namespace SerifXML;
	lexicalEntryElem.loadId(this);
	_key = lexicalEntryElem.getAttribute<Symbol>(X_key, Symbol(L"NULL"));
	_feats = FeatureValueStructure::build(lexicalEntryElem);
}

void LexicalEntry::resolvePointers(SerifXML::XMLTheoryElement lexicalEntryElem) {
	using namespace SerifXML;
	std::vector<const LexicalEntry*> segments = lexicalEntryElem.loadTheoryPointerList<LexicalEntry>(X_analysis);
	for (size_t i=0; i<_analysis.size(); ++i)
		_analysis.push_back(const_cast<LexicalEntry*>(segments[i]));
}
