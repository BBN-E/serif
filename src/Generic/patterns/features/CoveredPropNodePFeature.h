// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef COVERED_PROP_NODE_PFEATURE_H
#define COVERED_PROP_NODE_PFEATURE_H

#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/patterns/AbstractSlot.h"
#include "Generic/PropTree/PropNode.h"
#include "Generic/common/BoostUtil.h"
#include <boost/shared_ptr.hpp>

class Proposition;

/** A feature used to store information about a successful TopicPattern match,
  *   specifically one of the prop nodes covered by the proposition tree matching algorithm.
  */
class CoveredPropNodePFeature: public PatternFeature {
public:

	// To get around the argument limit on boost::make_shared()
	typedef struct {
		int pnode_sent_no;
		int pnode_start_token;
		int pnode_end_token;
		Symbol query_predicate;
		Symbol document_predicate;
		Symbol query_slot; 
		int matcher_index; 
		int matcher_total;
	} CPN_struct;

private:

	// Note that the pattern here is null, as these are created generically for many patterns at the same time
	CoveredPropNodePFeature(AbstractSlot::MatchType matchType, PropNode_ptr pnode, float score, Symbol query_slot, 
		Symbol query_predicate, int matcher_index, int matcher_total, const LanguageVariant_ptr& languageVariant)
		: PatternFeature(Pattern_ptr(),languageVariant), _pnode_sent_no(static_cast<int>(pnode->getSentNumber())), 
		_pnode_start_token(static_cast<int>(pnode->getHeadStartToken())), _pnode_end_token(static_cast<int>(pnode->getHeadEndToken())),
		_query_predicate(query_predicate), _document_predicate(Symbol(L"NULL")), _matcher_index(matcher_index), _matcher_total(matcher_total),
		_score(score), _matchType(matchType), _query_slot(query_slot)
	{
		if (pnode->getRepresentativePredicate() && !pnode->getRepresentativePredicate()->pred().is_null())
			_document_predicate = pnode->getRepresentativePredicate()->pred();	
	}
	BOOST_MAKE_SHARED_8ARG_CONSTRUCTOR(CoveredPropNodePFeature, AbstractSlot::MatchType, PropNode_ptr, float, Symbol, Symbol, int, int, const LanguageVariant_ptr&);
	
	// Note that the pattern here is null, as these are created generically for many patterns at the same time
	CoveredPropNodePFeature(AbstractSlot::MatchType matchType, PropNode_ptr pnode, float score, Symbol query_slot, 
		Symbol query_predicate, int matcher_index, int matcher_total)
		: PatternFeature(Pattern_ptr()), _pnode_sent_no(static_cast<int>(pnode->getSentNumber())), 
		_pnode_start_token(static_cast<int>(pnode->getHeadStartToken())), _pnode_end_token(static_cast<int>(pnode->getHeadEndToken())),
		_query_predicate(query_predicate), _document_predicate(Symbol(L"NULL")), _matcher_index(matcher_index), _matcher_total(matcher_total),
		_score(score), _matchType(matchType), _query_slot(query_slot)
	{
		if (pnode->getRepresentativePredicate() && !pnode->getRepresentativePredicate()->pred().is_null())
			_document_predicate = pnode->getRepresentativePredicate()->pred();	
	}
	BOOST_MAKE_SHARED_7ARG_CONSTRUCTOR(CoveredPropNodePFeature, AbstractSlot::MatchType, PropNode_ptr, float, Symbol, Symbol, int, int);

	// Note that the pattern here is null, as these are created generically for many patterns at the same time
	CoveredPropNodePFeature(AbstractSlot::MatchType matchType, float score, CPN_struct cpn_struct, const LanguageVariant_ptr& languageVariant) 
		: PatternFeature(Pattern_ptr(),languageVariant), _pnode_sent_no(cpn_struct.pnode_sent_no), 
		_pnode_start_token(cpn_struct.pnode_start_token), _pnode_end_token(cpn_struct.pnode_end_token), _query_predicate(cpn_struct.query_predicate),
		_document_predicate(cpn_struct.document_predicate),
		_matcher_index(cpn_struct.matcher_index), _matcher_total(cpn_struct.matcher_total), _query_slot(cpn_struct.query_slot),
		_score(score), _matchType(matchType) {}

	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(CoveredPropNodePFeature, AbstractSlot::MatchType, float, CPN_struct, const LanguageVariant_ptr&);
	
	// Note that the pattern here is null, as these are created generically for many patterns at the same time
	CoveredPropNodePFeature(AbstractSlot::MatchType matchType, float score, CPN_struct cpn_struct) 
		: PatternFeature(Pattern_ptr()), _pnode_sent_no(cpn_struct.pnode_sent_no), 
		_pnode_start_token(cpn_struct.pnode_start_token), _pnode_end_token(cpn_struct.pnode_end_token), _query_predicate(cpn_struct.query_predicate),
		_document_predicate(cpn_struct.document_predicate),
		_matcher_index(cpn_struct.matcher_index), _matcher_total(cpn_struct.matcher_total), _query_slot(cpn_struct.query_slot),
		_score(score), _matchType(matchType) {}

	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(CoveredPropNodePFeature, AbstractSlot::MatchType, float, CPN_struct);

public:
	int getPNodeSentenceNumber() const { return _pnode_sent_no; }
	int getPNodeStartToken() const { return _pnode_start_token; }
	int getPNodeEndToken() const { return _pnode_end_token; }
	Symbol getQueryPredicate() const { return _query_predicate; }
	Symbol getDocumentPredicate() const { return _document_predicate; }
	Symbol getQuerySlot() const { return _query_slot; }
	AbstractSlot::MatchType getMatchType() const { return _matchType; }
	float getScore() const { return _score; }

	// Overridden virtual methods.
	// We do NOT set these, because this covered prop node could be in another sentence than our answer, and
	//   that would affect the extent of our overall answer snippet (at least in Brandy)
	int getSentenceNumber() const { return -1; }
	int getStartToken() const { return -1; }
	int getEndToken() const { return -1; }
	bool equals(PatternFeature_ptr other) {
		boost::shared_ptr<CoveredPropNodePFeature> f = boost::dynamic_pointer_cast<CoveredPropNodePFeature>(other);
		return f && f->getPNodeSentenceNumber() == getPNodeSentenceNumber() &&
			f->getPNodeStartToken() == getPNodeStartToken() && 
			f->getPNodeEndToken() == getPNodeEndToken() &&
			f->getQueryPredicate() == getQueryPredicate() &&
			f->getDocumentPredicate() == getDocumentPredicate() &&
			f->getMatchType() == getMatchType() &&
			f->getQuerySlot() == getQuerySlot() &&
			f->getScore() == getScore();
	}
	virtual void setCoverage(const DocTheory * docTheory) {} // don't set coverage; could be from another sentence
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher) {} // don't set coverage; could be from another sentence
	virtual void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const;
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	CoveredPropNodePFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);

private:
	float _score;
	int _pnode_sent_no;
	int _pnode_start_token;
	int _pnode_end_token;
	Symbol _document_predicate;
	Symbol _query_predicate;
	int _matcher_index;
	int _matcher_total;
	Symbol _query_slot;
	AbstractSlot::MatchType _matchType;

};

typedef boost::shared_ptr<CoveredPropNodePFeature> CoveredPropNodePFeature_ptr;

#endif
