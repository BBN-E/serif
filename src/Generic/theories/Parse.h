// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARSE_H
#define PARSE_H

#include "Generic/theories/SynNode.h"
#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/theories/SentenceTheory.h"

#include <boost/tokenizer.hpp>
#include <iostream>

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;
namespace SerifXML { 
	class XMLIdMap; 
	typedef std::basic_string<XMLCh, std::char_traits<XMLCh>, std::allocator<XMLCh> > xstring;
}

// Conceptually, this class represents a parse theory.
// It consists of a pointer to the root of the tree and a score.


#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED Parse : public SentenceSubtheory {
public:
	Parse(const TokenSequence *tokSequence);
	Parse(const TokenSequence *tokSequence, SynNode *root, float score);
	Parse(const Parse &other, const TokenSequence *tokSequence);

	~Parse() { delete _root; }

	virtual SentenceTheory::SubtheoryType getSubtheoryType() const
	{ return SentenceTheory::PARSE_SUBTHEORY; }

	const SynNode * getRoot() const { return _root; }
	const SynNode * getSynNode( int ID, const SynNode * = 0 ) const;

	float getScore() const { return _score; }
	void setScore(float score) { _score = score; }

	const TokenSequence* getTokenSequence() const { return _tokenSequence; }
	void setTokenSequence(const TokenSequence* tokenSequence); // for backwards compatible state files

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out, const Parse &it)
		{ it.dump(out, 0); return out; }


	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	Parse(StateLoader *stateLoader, bool is_dependency_parse = false);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit Parse(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;
	
	bool isDefaultParse() const { return _is_default_parse; }
	void setDefaultParse(bool def) { _is_default_parse = def; }

	std::wstring toTreebankString(bool mark_heads) const;
	static SynNode* fromTreebankString(const std::wstring &src);
protected:
	SynNode *_root;
	float _score;
	bool _is_default_parse;
	const TokenSequence *_tokenSequence;

	bool determineIfDefaultParse(const SynNode *node);
	void writeTreebankString(const SynNode *node, std::wstringstream &out, int &tok_pos, bool mark_heads, bool is_head) const;
	void assignTreebankNodeIds(const SynNode *node, SerifXML::XMLIdMap *idMap, const std::wstring &prefix, int &node_id_counter) const;

	typedef boost::char_separator<wchar_t> wchar_seperator;
	typedef boost::tokenizer<wchar_seperator, std::wstring::const_iterator, std::wstring> wchar_tokenizer;
	static SynNode* fromTreebankString(wchar_tokenizer::const_iterator &tok_iter, 
		wchar_tokenizer::const_iterator &end, int &node_id_counter, int &tok_pos, bool &is_head);

};

#endif
