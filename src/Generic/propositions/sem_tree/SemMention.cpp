// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/propositions/PropositionFinder.h"
#include "Generic/propositions/sem_tree/SemMention.h"
#include "Generic/propositions/sem_tree/SemBranch.h"
#include "Generic/propositions/sem_tree/SemOPP.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/SessionLogger.h"


using namespace std;


SemMention::SemMention(SemNode *children, const SynNode *synNode, 
					   const Mention *mention, bool definite)
	: SemReference(children, synNode), _definite(definite), 
	  _mention(mention), _n_opps(0), _branch(0)
{}


void SemMention::simplify() {
	SemNode::simplify();

	// treatment of appositives
	if (_mention->getMentionType() == Mention::APPO) {
		if (PropositionFinder::getUnifyAppositives()) {
			// This is the old way (pre-ACE2004).
			// Turn whole appositive into a single mention by splicing out
			// the child-mentions.
			for (SiblingIterator iter(_firstChild); iter.more(); ++iter) {
				if ((*iter).getSemNodeType() == MENTION_TYPE &&
					(*iter).asMention().getMention()->getParent() ==
						getMention()) {
					(*iter).replaceWithChildren();
				}
			}
		} else {
			// This is the new way (ACE2004), and is in distill.best-english.par.
			// Only remove one of the two mentions (ideally, the non-name)

			// first collect the lhs and rhs children of the appositive
			SemMention *lhs = 0, *rhs = 0;
			for (SemNode *child = _firstChild; child != 0;
				 child = child->getNext()) {
				if (child->getSemNodeType() == MENTION_TYPE &&
					child->asMention().getMention()->getParent() ==
						getMention()) {
					if (lhs == 0) {
						lhs = &child->asMention();
					} else {
						rhs = &child->asMention();
					}
				}
			}

			if (lhs != 0 && rhs != 0) {
				SemNode *parent, *child; // child is never used?

				if ((lhs->getMention()->getMentionType() != Mention::NAME &&
					 lhs->getMention()->getMentionType() != Mention::PRON) &&
					(rhs->getMention()->getMentionType() == Mention::NAME ||
					 rhs->getMention()->getMentionType() == Mention::PRON)) {
					parent = rhs; child = lhs;
				} else {
					parent = lhs; child = rhs;
				}

				// move all children under the new parent, except itself
				for (SiblingIterator iter(_firstChild); iter.more(); ++iter) {
					if (&(*iter) == parent) {
						continue;
					}
					(*iter).pruneOut();
                    parent->appendChild(&(*iter));
				}

				// at this point, parent is this node's only child; we can
				// now replace the node with parent
				replaceWithChildren();
			}
		}
	}
}

void SemMention::regularize() {
	SemNode::regularize();

	for (SemNode *child = _firstChild; child; child = child->getNext()) {
		if (child->getSemNodeType() == OPP_TYPE) {
			if (_n_opps < MAX_SEM_MENTION_OPPS) {
				child->setTangential(false);
				_opps[_n_opps] = &child->asOPP();
			} else {
				SessionLogger::warn("sem_tree") << "SemMention::regularize(): "
					<< "MAX_MENTION_SET_MENTIONS exceeded\n";
			}
		} else if (child->getSemNodeType() == BRANCH_TYPE) {
			if (_branch == 0) {
				child->setTangential(false);
				_branch = &child->asBranch();
			}
		}
	}
}

// This appears to be the only entry point to createTracesEarnestly
bool SemMention::createTraces(SemReference *awaiting_ref) {
	if (_branch == 0) {
		return SemNode::createTraces(awaiting_ref);
	} else {
		
		// if there's a branch it's a relative clause, which means
		// we need to seek out a trace to point to this node

		// first search in the branch
		bool result = _branch->createTraces(this);
		if (result == false) {
			_branch->createTracesEarnestly(this);
		}

		// now create traces among all other children
		result = false;
		for (SemNode *child = getFirstChild();
			 child;
			 child = child->getNext()) {
			if (child != _branch) {
				if (child->isTangential()) {
					child->createTraces(0);
				} else {
					result |= child->createTraces(this);
				}
			}
		}

		return result;
	}
}


void SemMention::dump(ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	if (_tangential) {
		out << "(Mention) {";
	} else {
		out << "Mention {";
	}

	if (_mention == 0) {
		out << newline << "  - entity mention: <null>";
	} else { 
		out << newline << "  - entity mention: ";
		_mention->dump(out, indent + 4);
	}

	if (_definite) {
		out << newline << "  - determiner type: definite";
	} else {
		out << newline << "  - determiner type: indefinite";
	}

	for (SemNode *child = _firstChild; child; child = child->getNext()) {
		out << newline << "  ";
		child->dump(out, indent + 2);
	}

	out << "}";

	delete[] newline;
}

