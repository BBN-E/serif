// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/PropFactory.h"
#include <algorithm>

#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>

#include "Generic/PropTree/Predicate.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"


PropFactory::PropFactory(const DocTheory * doc_theory, int sent_no)
		: sent_no(sent_no),
		  dt(*doc_theory),
		  st(*dt.getSentenceTheory(sent_no)),
		  pset(*st.getPropositionSet()),
		  mset(*st.getMentionSet()),
		  eset(*dt.getEntitySet()),
		  tseq(*st.getTokenSequence()),
		  _cMent(0),_cProp(0),_init_done(0)
{ }

PropFactory::PropFactory(const DocTheory * doc_theory, const SentenceTheory* sent_theory, int sent_no)
		: sent_no(sent_no),
		  dt(*doc_theory),
		  st(*sent_theory),
		  pset(*st.getPropositionSet()),
		  mset(*st.getMentionSet()),
		  eset(*dt.getEntitySet()),
		  tseq(*st.getTokenSequence()),
		  _cMent(0),_cProp(0),_init_done(0)
{ }

PropFactory::PropFactory(const DocTheory * doc_theory, const Mention * m)
		: sent_no(m->getSentenceNumber()),
		  dt(*doc_theory),
		  st(*dt.getSentenceTheory(sent_no)),
		  pset(*st.getPropositionSet()),
		  mset(*st.getMentionSet()),
		  eset(*dt.getEntitySet()),
		  tseq(*st.getTokenSequence()),
		  _cMent(m),_cProp(0),_init_done(0)
{ }
	
PropFactory::PropFactory(const DocTheory * doc_theory, const Proposition * p, int sent_no)
		: sent_no(sent_no),
		  dt(*doc_theory),
		  st(*dt.getSentenceTheory(sent_no)),
		  pset(*st.getPropositionSet()),
		  mset(*st.getMentionSet()),
		  eset(*dt.getEntitySet()),
		  tseq(*st.getTokenSequence()),
		  _cMent(0),_cProp(p),_init_done(0)
{ }


PropNodes & PropFactory::getForestRoots(){
	if( !_init_done ) _init();
	return _forest_roots;
}


PropNodes & PropFactory::getAllNodes(){
	if( !_init_done ) _init();
	return _forest_nodes;
}


void PropFactory::_init(){
	// virtual methods can't be called within a constructor,
	// so we defer virtual constructors to argument access
	if( _cMent )
		constructFromMention(&dt, _cMent);
	else if( _cProp )
		constructFromProposition(&dt, _cProp);
	else
		constructFromSentence(&dt);
	
	_init_done = true;
}	


std::wstring PropFactory::synnodeString(const TokenSequence * tseq, const SynNode * node, bool lowercase)
{
	std::wostringstream pred_stream;
	for(int t = node->getStartToken(); t != node->getEndToken(); t++)
		pred_stream << tseq->getToken(t)->getSymbol().to_string() << L" ";
	pred_stream << tseq->getToken( node->getEndToken() )->getSymbol().to_string();
	
	std::wstring pred_str = pred_stream.str();
	
	if (lowercase) {
		transform(pred_str.begin(), pred_str.end(), pred_str.begin(), (int (*)(int)) tolower);
	}

	return pred_str;
}


void PropFactory::extractMentionPredicates(const Mention * ment, 
													 PropNode::WeightedPredicates & preds) const
{
	// First, build a predicate for this mention's head
	Symbol m_pred = mentionPredicateString(&dt, ment).c_str();
	if (ment->getMentionType() == Mention::NAME || Predicate::validPredicate(m_pred))
		preds[Predicate(Predicate::mentionPredicateType(ment), m_pred)] = 1.0f;
}


std::wstring PropFactory::mentionPredicateString(const DocTheory * dt, const Mention * m, bool lowercase)
{
	int c_sent_no = m->getUID().sentno();
	const TokenSequence * c_tseq = dt->getSentenceTheory(c_sent_no)->getTokenSequence();
	
	const SynNode * head = m->getNode()->getHeadPreterm();
	if(m->getMentionType() == Mention::NAME)
		head = head->getParent();
	
	return synnodeString(c_tseq, head, lowercase);
}

