// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LEXICAL_ENTRY_H
#define LEXICAL_ENTRY_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Theory.h"
#include "Generic/theories/FeatureValueStructure.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UnrecoverableException.h"
#include <set>
#include <vector>

class LexicalEntry : public Theory {
private:
	size_t _id;
	Symbol _key;
	FeatureValueStructure* _feats;	

	//Symbol _category;
	//Symbol _PoS;
	std::vector<LexicalEntry*> _analysis;
public:

	LexicalEntry(size_t id, Symbol key, FeatureValueStructure* feats, LexicalEntry** analysis, int analysis_length) throw (UnrecoverableException);
	
	~LexicalEntry();

	int getNSegments() const {return static_cast<int>(_analysis.size());}
	LexicalEntry* getSegment(size_t segment_number) const;

	bool operator==(const LexicalEntry &other) const;

	Symbol getKey() const;
	FeatureValueStructure* getFeatures();
	const FeatureValueStructure* getFeatures() const;
	size_t getID() const;	
	void dump(UTF8OutputStream &uos) const ;
	void dump(std::ostream &uos) const;
	std::string toString() const;

	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit LexicalEntry(SerifXML::XMLTheoryElement elem, size_t id);
	const wchar_t* XMLIdentifierPrefix() const;
	void resolvePointers(SerifXML::XMLTheoryElement lexicalEntryElem);
	void setID(size_t new_id); // Used when deserializing lexical entries from XML.

	struct _LexicalEntryLessThan {
		bool operator()(const LexicalEntry* lhs, const LexicalEntry *rhs) {
			return (lhs->getID() < rhs->getID()); }};
	typedef std::set<const LexicalEntry*, _LexicalEntryLessThan> LexicalEntrySet;

};
#endif
