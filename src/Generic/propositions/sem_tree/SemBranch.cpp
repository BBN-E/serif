// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/propositions/sem_tree/SemBranch.h"
#include "Generic/propositions/sem_tree/SemOPP.h"
#include "Generic/propositions/sem_tree/SemLink.h"
#include "Generic/propositions/sem_tree/SemReference.h"
#include "Generic/propositions/sem_tree/SemTrace.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/common/OutputUtil.h"

using namespace std;


void SemBranch::simplify() {
	SemNode::simplify();

	// The root branch must not be disturbed
	if (getParent() == 0) {
		return;
	}

	// if this node just has one branch as a child, then it
	// can be removed
	if (getFirstChild() != 0 && getFirstChild()->getNext() == 0 &&
		getFirstChild()->getSemNodeType() == BRANCH_TYPE) {
		replaceWithChildren();
		//delete this;
	}
}

void SemBranch::fixLinks() {
	SemNode::fixLinks();

	// if there is a verb opp, then move any links to it
	SemNode *opp = 0;
	int opp_index = 0;
	for (SemNode *child = _firstChild; child != 0; child = child->getNext()) {
		if (child->getSemNodeType() == OPP_TYPE &&
			(child->asOPP().getPredicateType() == Proposition::VERB_PRED ||
			 child->asOPP().getPredicateType() == Proposition::COMP_PRED)) {
			opp = child;
			break;
		}
		opp_index++;
	}

	if (opp != 0) {
		int i = 0;
		for (SiblingIterator iter(_firstChild); iter.more(); ++iter) {
			if ((*iter).getSemNodeType() == LINK_TYPE) {
				SemNode &link = (*iter).asLink();
				link.pruneOut();
				if (i < opp_index) {
					opp->prependChild(&link);
				} else {
					opp->appendChild(&link);
				}
			}
			i++;
		}
	}
}

void SemBranch::regularize() {
	SemNode::regularize();

	for (SemNode *child = _firstChild; child != 0; child = child->getNext()) {
		if (child->isReference()) {
			if (_opp == 0) { // definitely English-specific :(
				_ref = &child->asReference();
			}
		} else if (child->getSemNodeType() == OPP_TYPE) {
			_opp = &child->asOPP();
		}
	}

	if (_ref != 0) {
		_ref->setTangential(false);
	}
	if (_opp != 0) {
		_opp->setTangential(false);
	}
}

bool SemBranch::createTraces(SemReference *awaiting_ref) {
	bool result = false;

	// if there's a verb there without a subject, then put a trace
	// in the subject's position
	if (_ref == 0 && _opp != 0) {
		_ref = _new SemTrace();
		prependChild(_ref);
		_ref->setTangential(false);
		result = _ref->asTrace().resolve(awaiting_ref);
	}

	if (result) {
		awaiting_ref = 0;
	}

	// if there's a ref awaiting a trace and we didn't just find a trace
	// for it, then recurse to children
	SemNode::createTraces(awaiting_ref);

	return result;
}


Proposition *SemBranch::getAssociatedProposition() {
	if (_opp == 0) {
		return 0;
	} else {
		return _opp->getProposition();
	}
}


void SemBranch::dump(ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	if (_tangential) {
		out << "(Branch) {";
	} else {
		out << "Branch {";
	}

	if (_ref != 0) {
		const Mention* ment = _ref->asReference().getMention();
		int index = -1;
		if (ment) {
			index = ment->getIndex();
		}
		std::string s = "<no_syn_node>";
		if (_ref->getSynNode()) {
			s = _ref->getSynNode()->toDebugTextString();
		}
		out << newline << "  - ref(" << index << "): " << s;
	}

	if (_opp != 0) {
		std::string s = "<no_syn_node>";
		if (_opp->getSynNode()) {
			s = _opp->getSynNode()->toDebugTextString();
		}
		out << newline << "  - opp(" << Proposition::getPredTypeString(_opp->getPredicateType()) << "): " << s;
	}

	for (SemNode *child = _firstChild; child; child = child->getNext()) {
		out << newline << "  ";
		child->dump(out, indent + 2);
	}

	out << "}";

	delete[] newline;
}

