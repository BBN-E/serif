#include "boost/foreach.hpp"
#include "boost/make_shared.hpp"
#include "Generic/common/bsp_declare.h"
#include "PredFinder/inference/PlaceInfo.h"
#include "PredFinder/elf/ElfDocument.h"
#include <set>
#include <vector>
#include <map>

#pragma once

class EIDocData {
public:
	EIDocData() : _docTheory(NULL), _elfDoc(ElfDocument_ptr()) {}
	~EIDocData() {clear();}
	void clear() {
		setDocTheory(NULL);
		setElfDoc(ElfDocument_ptr());
		_document_place_info.clear();
		_place_name_to_entity.clear();
		_countries.clear();
		_cities.clear();
		_states.clear();
		_other.clear();
	}
	ElfDocument_ptr getElfDoc() const {return _elfDoc;}
	void setElfDoc(ElfDocument_ptr elfDoc) {_elfDoc = elfDoc;}
	std::vector<PlaceInfo> & getDocumentPlaceInfo() {return _document_place_info;}
	PlaceInfo & getDocumentPlaceInfo(std::vector<PlaceInfo>::size_type i) {return _document_place_info[i];}
	std::map<std::wstring, std::set<int> > & getPlaceNameToEntity() {return _place_name_to_entity;}
	std::vector<int> & getCountries() {return _countries;}
	std::vector<int> & getCities() {return _cities;}
	std::vector<int> & getStates() {return _states;}
	std::vector<int> & getOther() {return _other;}

	// convenience methods - DistillationDoc
	std::wstring getDocID() const {return std::wstring(_docTheory->getDocument()->getName().to_string());}
	const DocTheory * getDocTheory() const {return _docTheory;}
	void setDocTheory(const DocTheory* docTheory) {_docTheory = docTheory;}
	int getNSentences() const {return _docTheory->getNSentences();}
	class SentenceTheory * getSentenceTheory(int i) const {return _docTheory->getSentenceTheory(i);}
	Mention* getMention(MentionUID uid) const {return getSentenceTheory(Mention::getSentenceNumberFromUID(uid))->getMentionSet()->getMention(Mention::getIndexFromUID(uid)); }
	EntitySet* getEntitySet() const {return _docTheory->getEntitySet();}
	Entity* getEntity(int i) const {return _docTheory->getEntitySet()->getEntity(i);}
	Entity* getEntityByMention(const Mention* ment) const {return _docTheory->getEntityByMention(ment);}
	Entity* getEntityByMention(MentionUID uid) const {return _docTheory->getEntitySet()->getEntityByMention(uid);}
	Entity* getEntityByMention(MentionUID uid, EntityType type) const {
		return _docTheory->getEntitySet()->getEntityByMention(uid, type);}
	Document* getDocument() {return _docTheory->getDocument();}

	// convenience methods - ElfDocument
	std::set<ElfRelation_ptr> get_relations(void) const {return _elfDoc->get_relations();}
	void remove_relations(const std::set<ElfRelation_ptr> & relations) {return _elfDoc->remove_relations(relations);}
	void remove_individuals(const ElfIndividualSet & individuals) {return _elfDoc->remove_individuals(individuals);}
	ElfIndividual_ptr get_merged_individual_by_uri(const std::wstring & uri) const {return _elfDoc->get_merged_individual_by_uri(uri);}
	ElfRelationMap get_relations_by_individual(const ElfIndividual_ptr search_ind) {return _elfDoc->get_relations_by_individual(search_ind);}
	ElfIndividualSet get_individuals_by_type(const std::wstring & search_type = L"") {return _elfDoc->get_individuals_by_type(search_type);}
	ElfIndividualSet get_merged_individuals_by_type(const std::wstring & search_type = L"") {return _elfDoc->get_merged_individuals_by_type(search_type);}

protected:
	// Data members
	const DocTheory* _docTheory;
	ElfDocument_ptr _elfDoc;

	std::vector<PlaceInfo> _document_place_info;  //maps sub/super locations
	std::map<std::wstring, std::set<int> > _place_name_to_entity; //map of place-name-string --> entity id
	std::vector<int> _countries;
	std::vector<int> _cities;
	std::vector<int> _states;
	std::vector<int> _other;
};
