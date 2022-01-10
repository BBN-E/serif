// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/relations/en_FamilyRelationFinder.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/Symbol.h"
#include <algorithm>
#include <boost/scoped_ptr.hpp>
UTF8OutputStream EnglishFamilyRelationFinder::_debugStream;
bool EnglishFamilyRelationFinder::DEBUG = false;


EnglishFamilyRelationFinder::EnglishFamilyRelationFinder()
{
	std::string debug_buffer = ParamReader::getParam("family_debug");
	if (!debug_buffer.empty()) {
		_debugStream.open(debug_buffer.c_str());
		DEBUG = true;
	}

	std::string family_relations = ParamReader::getRequiredParam("family_relations");
	boost::scoped_ptr<UTF8InputStream> famrelstream_scoped_ptr(UTF8InputStream::build(family_relations.c_str()));
	UTF8InputStream& famrelstream(*famrelstream_scoped_ptr);
	Sexp* familyRelations = _new Sexp(famrelstream);
	_num_famrels = familyRelations->getNumChildren();
	_fRelations = _new FamilyRelation[_num_famrels];

	for (int m = 0; m < _num_famrels; m++)
		fillRelationArrays(familyRelations->getNthChild(m), m);
	delete familyRelations;
	
	std::string gender_evidence = ParamReader::getRequiredParam("gender_evidence");
	boost::scoped_ptr<UTF8InputStream> evStream_scoped_ptr(UTF8InputStream::build(gender_evidence.c_str()));
	UTF8InputStream& evStream(*evStream_scoped_ptr);
	Sexp* genEvidence = _new Sexp(evStream);
	_num_evidence = genEvidence->getNumChildren();
	_evidenceArrays = _new Evidence;
	for (int n = 0; n < _num_evidence; n++)
		fillGenderEvidenceArrays(genEvidence->getNthChild(n));
	delete genEvidence;

}

EnglishFamilyRelationFinder::~EnglishFamilyRelationFinder()
{
	delete _evidenceArrays;
	delete _fRelations;
	_debugStream.close();
}

void EnglishFamilyRelationFinder::resetForNewSentence()
{
}

RelMention* EnglishFamilyRelationFinder::getFixedFamilyRelation(RelMention *rm, DocTheory *docTheory) {
	// get the entities in this document
	EntitySet *es = docTheory->getEntitySet();
	
	// Get the entity corresponding to the mention of each arg
	const Mention *arg1 = rm->getLeftMention();
	MentionUID mentionId = arg1->getUID();
	Entity *arg1Entity = es->getEntityByMention(mentionId);
	const Mention *arg2 = rm->getRightMention();
	mentionId = arg2->getUID();
	Entity *arg2Entity = es->getEntityByMention(mentionId);
	
	// get the heads of all mentions associated with both entities
	// put them in a single array, arg1 then arg2 mentions
	int nArg1Mentions = arg1Entity->getNMentions();
	int nArg2Mentions = arg2Entity->getNMentions();
	int nAllMentions = nArg1Mentions + nArg2Mentions;
	Symbol *mentionheads = _new Symbol[nAllMentions];
	
	int j;
	for (j = 0; j < nArg1Mentions; j++) 
		mentionheads[j] = getMentionHeadFromEntity(arg1Entity, j, docTheory);
	for ( ; j < nAllMentions; j++) 
		mentionheads[j] = getMentionHeadFromEntity(arg2Entity, j-nArg1Mentions, docTheory);
	
	if (DEBUG){
		std::string docname = docTheory->getDocument()->getName().to_debug_string();	
		_debugStream << "\nWorking on document " << docname << "\n";
	}
	
	std::string arg1gender = getArgGender(mentionheads, 0, nArg1Mentions);
	std::string arg2gender = getArgGender(mentionheads, nArg1Mentions, nAllMentions);
	
	std::vector<Symbol> *frlist = _new std::vector<Symbol>;
	FamilyRelation fr;
	int k=0;
	bool relationFound(0);

	while (k < nArg1Mentions){
		relationFound = findFamilyRelation(fr, mentionheads[k]);
		if (relationFound) {
			frlist->push_back(fr.neutralCorrelate);
		}
		k++;
	}
	while (k < nAllMentions) {
		relationFound = findFamilyRelation(fr, mentionheads[k]);
		if (relationFound) {
			relationFound = findFamilyRelation(fr, fr.reverseRelation);
			frlist->push_back(fr.neutralCorrelate);
		}
		k++;
	}
	
	getMostCommonRelation(fr, frlist);

	changeRelationGender(fr, arg1gender);

	if (fr.simpleName == Symbol(L"spouse"))
		fixSpouseRelation(fr, arg2gender);
	

	rm->setType(fr.formalName);
	delete frlist;	
	return rm;
}



bool EnglishFamilyRelationFinder::findFamilyRelation(FamilyRelation &fr, Symbol name)
{
	for (int i = 0; i < _num_famrels; i++) {
		if (_fRelations[i].simpleName == name){
			fr = _fRelations[i];
			return true;
		}
	}
	return false;
}
void EnglishFamilyRelationFinder::changeRelationGender(FamilyRelation &fr, std::string argGender)
{
	if (argGender == "male" && fr.gender != "male") {
		if (DEBUG){_debugStream << "argument is male, change relation from " << fr.simpleName.to_debug_string() << " to " << fr.maleCorrelate.to_debug_string() << "\n";}
		bool found = findFamilyRelation(fr, fr.maleCorrelate);
		if (!found){
			throw UnrecoverableException("EnglishFamilyRelationFinder::changeRelationGender()", "Could not find FamilyRelation for male version of original");
		}
	}
	else if (argGender == "female" && fr.gender != "female") {
		if (DEBUG){_debugStream << "argument is female, change relation from " << fr.simpleName.to_debug_string() << " to " << fr.femaleCorrelate.to_debug_string() << "\n";}
		bool found = findFamilyRelation(fr, fr.femaleCorrelate);
		if (!found){
			throw UnrecoverableException("EnglishFamilyRelationFinder::changeRelationGender()", "Could not find FamilyRelation for female version of original");
		}
	}
}

std::string EnglishFamilyRelationFinder::getArgGender(Symbol *mheads, int beginIndex, int endIndex){
	int maleCount(0);
	int femaleCount(0);

	std::vector<Symbol> me = _evidenceArrays->maleEvidence;
	std::vector<Symbol> fe = _evidenceArrays->femaleEvidence;

	for (int i = beginIndex; i < endIndex; i++) {
		if (find(me.begin(), me.end(), mheads[i]) != me.end())
			maleCount++;
		else if (find(fe.begin(), fe.end(), mheads[i]) != fe.end())
			femaleCount++;
	}
	if (DEBUG) {_debugStream << "gender evidence: " << maleCount << " male, " << femaleCount << " female ";}
	if (maleCount == 0 && femaleCount == 0){
		return "neutral";
	}
	else if (maleCount >= femaleCount){
		return "male";
	}
	else{
		return "female";
	}
}

void EnglishFamilyRelationFinder::getMostCommonRelation(FamilyRelation &answer, std::vector<Symbol> *frlist)
{
	// create the set of symbols
	std::vector<Symbol> symbol_set;
	std::vector<Symbol>::iterator pos;
	for (size_t j=0; j< frlist->size(); j++){
		pos = find(symbol_set.begin(), symbol_set.end(), frlist->at(j));
		if (pos == symbol_set.end())
			symbol_set.push_back(frlist->at(j));
	}
	if (symbol_set.size() == 1)
		findFamilyRelation(answer, symbol_set[0]);
	
	if (symbol_set.size() > 1) {
		std::map<int,Symbol> sym_tal;
		std::vector<int> keys;
		int c;
		for (size_t k=0; k < symbol_set.size(); k++){
			c = (int) count(frlist->begin(), frlist->end(), symbol_set[k]);
			keys.push_back(c);
			if (sym_tal[c] == NULL)
				sym_tal[c] = symbol_set[k]; // prefer first relation over subsequent if tally is equal
		}
		std::vector<int>::iterator keymax;
		keymax = max_element(keys.begin(), keys.end());
		findFamilyRelation(answer, sym_tal[*keymax]);  //sets answer to most frequent family relation
	}
	if (symbol_set.size() == 0) 
		answer = _fRelations[0]; // none
}

void EnglishFamilyRelationFinder::fixSpouseRelation(FamilyRelation &fr, std::string gender)
{
	// assume that arg1 has no gender evidence,
	// so use opposite of arg2
	if (gender == "male")
		changeRelationGender(fr, "female");
	else
		if (gender == "female")
			changeRelationGender(fr, "male");
}

// need to take a circuitous route to get all the mentions out of an entity,
// here just get the headword of one mention of one entity
Symbol EnglishFamilyRelationFinder::getMentionHeadFromEntity(Entity *ent, int entityIndex, DocTheory *dt){
	MentionUID mentUID = ent->getMention(entityIndex);
	int sentNum = Mention::getSentenceNumberFromUID(mentUID);
	SentenceTheory *st = dt->getSentenceTheory(sentNum);
	MentionSet *ms = st->getMentionSet();
	Mention *m = ms->getMention(mentUID);
	return m->getHead()->getHeadWord();
}

void EnglishFamilyRelationFinder::fillGenderEvidenceArrays(Sexp* sexp) {
	if (!sexp->isList() || sexp->getNumChildren() < 3 || !sexp->getFirstChild()->isAtom())
		throw UnexpectedInputException("EnglishFamilyRelationFinder::fillGenderEvidenceArrays()", 
				"Ill-formed values-rule-set sexp");
	_evidenceArrays->maleEvidence.push_back(Symbol(sexp->getNthChild(0)->getValue()));
	_evidenceArrays->femaleEvidence.push_back(Symbol(sexp->getNthChild(1)->getValue()));
	_evidenceArrays->neutralEvidence.push_back(Symbol(sexp->getNthChild(2)->getValue()));
}
void EnglishFamilyRelationFinder::fillRelationArrays(Sexp* sexp, int i)
{
	if (!sexp->isList() || sexp->getNumChildren() < 7 || !sexp->getFirstChild()->isAtom())
		throw UnexpectedInputException("EnglishFamilyRelationFinder::fillRelationArrays()", 
				"Ill-formed values-rule-set sexp");
	_fRelations[i].simpleName = sexp->getNthChild(0)->getValue();
	_fRelations[i].formalName = sexp->getNthChild(1)->getValue();
	_fRelations[i].gender = std::string(sexp->getNthChild(2)->getValue().to_debug_string());
	_fRelations[i].reverseRelation = sexp->getNthChild(3)->getValue();
	_fRelations[i].maleCorrelate = sexp->getNthChild(4)->getValue();
	_fRelations[i].femaleCorrelate = sexp->getNthChild(5)->getValue();
	_fRelations[i].neutralCorrelate = sexp->getNthChild(6)->getValue();
}


void EnglishFamilyRelationFinder::printFamilyRelation(FamilyRelation fr)
{
	if (fr.simpleName != NULL)
		_debugStream << fr.simpleName << "/" << fr.formalName << "\n";
}
