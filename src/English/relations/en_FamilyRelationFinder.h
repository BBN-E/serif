#ifndef EN_FAMILY_RELATION_FINDER_H
#define EN_FAMILY_RELATION_FINDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/SymbolSet.h"
#include "Generic/relations/FamilyRelationFinder.h"
#include <vector>

class Sexp;
class Entity;
class RelMention;
class DocumentTheory;

class EnglishFamilyRelationFinder : public FamilyRelationFinder {
private:
	friend class EnglishFamilyRelationFinderFactory;

public:
	~EnglishFamilyRelationFinder();
	void resetForNewSentence();
	
	RelMention *getFixedFamilyRelation(RelMention *rm, DocTheory *docTheory);

private:

	EnglishFamilyRelationFinder();

	static bool DEBUG;
	static UTF8OutputStream _debugStream;

	int _num_famrels;
	int _num_relgenders;

	typedef struct {
		Symbol simpleName;
		Symbol formalName;
		std::string gender;
		Symbol reverseRelation;
		Symbol maleCorrelate;
		Symbol femaleCorrelate;
		Symbol neutralCorrelate;
	} FamilyRelation;
	FamilyRelation *_fRelations;

	typedef struct {
		std::vector<Symbol> maleEvidence;
		std::vector<Symbol> femaleEvidence;
		std::vector<Symbol> neutralEvidence;
	} Evidence;
	Evidence *_evidenceArrays;
	int _num_evidence;


	void fillRelationArrays(Sexp *sexp, int i);
	void fillGenderEvidenceArrays(Sexp *sexp);
	bool findFamilyRelation(FamilyRelation &fr, Symbol name);
	void changeRelationGender(FamilyRelation &fr, std::string argGender);
	void fixSpouseRelation(FamilyRelation &fr, std::string gender);
	Symbol getMentionHeadFromEntity(Entity *ent, int entityIndex, DocTheory *dt);
	std::string getArgGender(Symbol *mheads, int beginIndex, int endIndex);
	void printFamilyRelation(FamilyRelation fr);
	void getMostCommonRelation(FamilyRelation &answer, std::vector<Symbol> *frlist);

};


// RelationFinder factory
class EnglishFamilyRelationFinderFactory: public FamilyRelationFinder::Factory {
	virtual FamilyRelationFinder *build() { return _new EnglishFamilyRelationFinder(); } 
};
#endif
