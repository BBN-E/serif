// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/SPropTrees/SPropTreeInfo.h"
#include "Generic/SPropTrees/SPropTree.h"
#include "Generic/common/Sexp.h"

bool lessProp::operator()(const SPropTreeInfo& p1, const SPropTreeInfo& p2) const { 
		if ( p1.sentNumber < p2.sentNumber ) return 1.0;
		else if ( p1.sentNumber > p2.sentNumber ) return 0.0;
		else if ( p1.tree && p2.tree ) { //don't check if heads exist!!!
			return p1.tree->getHead()->getPropositionID() < p2.tree->getHead()->getPropositionID();
		} else return p1.tree < p2.tree;
}

//===========METHODS OF SPropTreeInfo======================

//read of SPropTreeInfo from UTF8 stream
bool SPropTreeInfo::resurect(UTF8InputStream& utf8, const DocTheory* dt, SPropTreeInfo& ptin) {
	ptin.allEntityMentions.clear();
	ptin.sentNumber = -1;
	ptin.tree = 0;
	Sexp* se = _new Sexp(utf8);
	if ( ! se || se->isVoid() ) return false;
	std::wstring propstr=se->to_string();

	if (se->getNumChildren() != 2) {
		SessionLogger::warn("prop_trees") << "invalid entry (lev 1): " << propstr << std::endl;
		return false;
	}
	ptin.sentNumber = _wtoi(se->getFirstChild()->to_string().c_str());
	const SentenceTheory* st=dt->getSentenceTheory(ptin.sentNumber);
	ptin.tree = _new SPropTree(dt, st, se->getSecondChild());
	if ( ! ptin.tree ) return false;
	ptin.tree->getAllEntityMentions(ptin.allEntityMentions);
	return ptin.tree->getSize() != 0;
}


