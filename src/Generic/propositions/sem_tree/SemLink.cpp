// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/propositions/sem_tree/SemOPP.h"
#include "Generic/propositions/sem_tree/SemBranch.h"
#include "Generic/propositions/sem_tree/SemReference.h"
#include "Generic/propositions/sem_tree/SemMention.h"
#include "Generic/propositions/sem_tree/SemTrace.h"
#include "Generic/propositions/sem_tree/SemLink.h"
#include "Generic/propositions/SemTreeBuilder.h"
#include "Generic/propositions/PropositionFinder.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/InternalInconsistencyException.h"


void SemLink::simplify() {
	SemNode::simplify();

	// XXX
	// see if this is degenerate in that it has no head symbol
	if (_symbol.is_null()) {
		// hmmm let's see if this ever actually happens by making it a
		// warning condition:
		SessionLogger::warn("sem_tree") << "SemLink has null symbol\n";

		replaceWithChildren();
		//delete this;
		return;
	}

	/*    // see if there is an object available
	SemNode object = null;
	for (int i = 0; i < _children.length; i++) {
	if (_children[i] instanceof SemMention ||
	_children[i] instanceof SemMentionSet ||
	_children[i] instanceof SemBranch)
	{
	object = _children[i];
	break;
	}
	}

	if (object == null) {
	// combine with child link (e.g. (out (of ...)) -> (out_of ...) )

	if (_children.length == 1 &&
	_children[0] instanceof SemLink)
	{
	replaceWithChildren();
	}
	}*/
}


void SemLink::fixLinks() {
	// 2009 addition.
	// promote <poss>:<temp>:X to <temp>:X
	// e.g. "Monday night's game" becomes
	// (game <temp>:"Monday night")
	// requires: we're poss, have one child, 
	// child is temp. ~ RMG
	if (PropositionFinder::getUse2009Props()) {
		if (_symbol==Argument::POSS_ROLE &&
			_firstChild && _firstChild==_lastChild &&
			_firstChild->getSemNodeType()==LINK_TYPE &&
			_firstChild->asLink().getSymbol()==Argument::TEMP_ROLE) {
			replaceWithNode(_firstChild);
			_firstChild->fixLinks();
			return;
		}
	}

	SemNode::fixLinks();

	if (_parent == 0) {
		return;
	}

//	if (_symbol == Argument::TEMP_ROLE)
//		return;

	/* if we're under a mention or we're the only child of a branch, then
	 * we try to associate with a noun (which will only work if we're under
	 * a mention), and if that fails we turn into an OPP */
	if (_parent->getSemNodeType() == MENTION_TYPE ||
		(_parent->getSemNodeType() == BRANCH_TYPE &&
		_parent->getFirstChild()->getNext() == 0)) {

		// try to associate with noun, instead of standing alone
		SemOPP *noun = SemTreeBuilder::get()->findAssociatedPredicateInNP(*this);
		if (noun != 0) {
			// move over to the noun we found
			pruneOut();
			noun->appendChild(this);
		} else {

			// in NP or branch but not associated with noun,
			// so introduce predicate
			const SynNode *head = _head;
			if (head == 0) {
				head = _synNode;
			}

			// create the node
			SemOPP *opp;
			if (_symbol == Argument::POSS_ROLE) {
				opp = _new SemOPP(0, getSynNode(),
								  Proposition::POSS_PRED, head);
			} else if (_symbol == Argument::LOC_ROLE) {
				opp = _new SemOPP(0, getSynNode(),
								  Proposition::LOC_PRED, head);
			} else {
				opp = _new SemOPP(0, getSynNode(),
								  Proposition::MODIFIER_PRED, head);
			}

			// replace this link with the opp
			replaceWithNode(opp);

			// now put this link under the opp
			this->setNext(0);
			opp->appendChild(this);
		}
	}
}

void SemLink::regularize() {
	SemNode::regularize();

	if (_quote) {
		// for quote-style links (which result in TEXT_ARG-style args),
		// the object is the same node
		_object = this;
	} else {
		for (SemNode *child = _firstChild; child; child = child->getNext()) {
			if (child->getSemNodeType() == MENTION_TYPE ||
				child->getSemNodeType() == BRANCH_TYPE) {
				_object = child;
			}
		}

		// if there were no mentions or branches, look for an opp (these
		// can be objects of links of COMP_PRED-type OPPs)
		if (_object == 0 && _symbol == Argument::MEMBER_ROLE) {
			for (SemNode *child = _firstChild; child; child = child->getNext()) {
				if (child->getSemNodeType() == OPP_TYPE) {
					_object = child;
				}
			}
		}
	}

	if (_object) {
		_object->setTangential(false);
	}
}

bool SemLink::createTraces(SemReference *awaiting_ref) {
	bool result = false;

	// if we don't have an object, then there must be a trace here
	if (_object == 0) {
		prependChild(_new SemTrace());
		_object = _firstChild;
		_object->setTangential(false);
		result = _object->asTrace().resolve(awaiting_ref);
	}

	if (result == true) {
		awaiting_ref = 0;
	}

	// if we didn't find a place for awaiting ref, keep looking
	// among children
	return result || SemNode::createTraces(awaiting_ref);
}


void SemLink::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	if (_tangential)
		out << "(Link) {";
	else
		out << "Link {";

	out << newline << "  - symbol: \"" << getSymbol().to_debug_string() << "\"";

	for (SemNode *child = _firstChild; child; child = child->getNext()) {
		out << newline << "  ";
		child->dump(out, indent + 2);
	}

	out << "}";

	delete[] newline;
}

