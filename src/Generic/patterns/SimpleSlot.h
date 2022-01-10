// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef SIMPLE_SLOT_H
#define SIMPLE_SLOT_H

#include "Generic/patterns/AbstractSlot.h"
#include "Generic/PropTree/PropNodeMatch.h"
#include "Generic/PropTree/PropEdgeMatch.h"
#include "Generic/PropTree/PropFullMatch.h"

class Mention;
class PatternFeatureSet;
class PatternMatcher;

typedef std::map<std::wstring,double> NameSynonyms;
typedef std::map<std::wstring,NameSynonyms> NameDictionary;

class SimpleSlot: public AbstractSlot {
public:
	SimpleSlot(const Symbol& label, DocTheory* docTheory, int start_token_offset, const NameDictionary& equivalent_names, std::map<std::wstring, float> &weights);	
	~SimpleSlot();
	Symbol getLabel() const;
	std::wstring getRegexNameString(float min_eqn_score) const;
	bool hasName() const;

	// Methods declared on our parent class
	std::wstring getBackoffRegexText() const;
	boost::shared_ptr<PropMatch> getMatcher(MatchType matchType) const;
	const SentenceTheory* getSlotSentenceTheory() const;
	bool requiresProptrees() const;
private:
	std::wstring constructBackoffRegexText();
	std::vector<PropNode_ptr> constructSlotForest();
	void enumeratePropNodes(PropNodes::const_iterator begin, PropNodes::const_iterator end, PropNodes & container);	
	std::vector<std::wstring> getEquivalentNames(const std::wstring& name, double min_eqn_score) const;
	std::wstring getRegexText() const;
	void printEquivalentNames() const;
	void setMainMention();
	void setNameMention();
	void setNameString();
	void setParseNodes();

	std::map<PropNode_ptr, float> getNodeWeights(PropNode::PropNodes slot_nodes, std::map<std::wstring, float> &weights);

	// Things we are passed in
	Symbol _label;
	DocTheory* _docTheory;
	int _start_token_offset;
	const std::map<std::wstring,std::map<std::wstring,double> > _equivalent_names;
	
	// Things we construct from what we are passed in	
	// Serif analysis
	const Mention* _mainMention;
	const Mention* _nameMention;
	std::wstring _name_string;
	std::vector<const SynNode*> _parseNodes;

	// Regular expressions
	std::wstring _backoff_regex_text;

	// Proptree matchers
	boost::shared_ptr<PropNodeMatch> _nodeMatcher;
	boost::shared_ptr<PropEdgeMatch> _edgeMatcher;
	boost::shared_ptr<PropFullMatch> _fullMatcher;
};

#endif
