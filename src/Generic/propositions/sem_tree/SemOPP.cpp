// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/propositions/SemTreeBuilder.h"
#include "Generic/propositions/sem_tree/SemOPP.h"
#include "Generic/propositions/sem_tree/SemBranch.h"
#include "Generic/propositions/sem_tree/SemReference.h"
#include "Generic/propositions/sem_tree/SemMention.h"
#include "Generic/propositions/sem_tree/SemTrace.h"
#include "Generic/propositions/sem_tree/SemLink.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/IDGenerator.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"

Symbol SemOPP::BETWEEN_SYMBOL = Symbol(L"between");

SemOPP::SemOPP(SemNode *children, const SynNode *synNode,
			   Proposition::PredType predType, const SynNode *head)
	: SemNode(children, synNode), _predType(predType), _head(head),
	  _headSym(head ? head->getHeadWord() : Symbol()),
	  _modal(0), _negative(0), _particle(0), _adverb(0), _proposition(0), 
	  _arg1(0), _arg2(0), _n_links(0)
{}

SemOPP::SemOPP(SemNode *children, const SynNode *synNode,
			   Proposition::PredType predType, const SynNode *head,
			   Symbol headSym)
	: SemNode(children, synNode), _predType(predType), _head(head),
	  _headSym(headSym),
	  _modal(0), _negative(0), _particle(0), _adverb(0), _proposition(0), 
	  _arg1(0), _arg2(0), _n_links(0)
{}

SemOPP::~SemOPP() {
	delete _proposition;
}


void SemOPP::addNegative(const SynNode *negative) {
	if (negative)
		_negative = negative;
}

void SemOPP::addParticle(const SynNode *particle) {
	if (particle)
		_particle = particle;
}

void SemOPP::addAdverb(const SynNode *adverb) {
	if (adverb)
		_adverb = adverb;
}

void SemOPP::addModal(const SynNode *modal) {
	if (modal)
		_modal = modal;
}


void SemOPP::simplify() {
	if (_predType == Proposition::COPULA_PRED) {
		SemNode::simplify();

		// we get rid of copulas if there is no mention in the VP for the
		// <sub> to, eh-em, copulate with.

		// look for mention in VP
		for (SemNode *c = _firstChild; c; c = c->getNext()) {
			if (c->isReference()) {
				return;
			}
		}

		// none found, so this copula kicks the bucket

		// but first, transfer any modals or negatives to possible child OPP
		// locate SemOPP among children
		SemOPP *opp = 0;
		for (SemNode *child = _firstChild; child; child = child->getNext()) {
			if (child->getSemNodeType() == OPP_TYPE) {
				// require the other opp to be a verb or compound pred
				if (child->asOPP().getPredicateType() == Proposition::VERB_PRED ||
					child->asOPP().getPredicateType() == Proposition::COMP_PRED)
				{
					opp = &child->asOPP();
					break;
				}
			}
		}
		// add modal, negative, particle, and adverb
		if (opp != 0) {
			opp->addParticle(_particle);
			opp->addAdverb(_adverb);
			opp->addNegative(_negative);
			opp->addModal(_modal);
		}

		// if any children are links, turn them into OPPs
		for (SiblingIterator iter(_firstChild); iter.more(); ++iter) {
			if ((*iter).getSemNodeType() == LINK_TYPE) {
				SemLink &link = (*iter).asLink();

				const SynNode *head = link.getHead();
				if (head == 0)
					head = link.getSynNode();

				// create the node
				SemOPP *opp;
				if (link.getSymbol() == Argument::POSS_ROLE) {
					opp = _new SemOPP(0, link.getSynNode(),
									  Proposition::POSS_PRED, head);
				}
				else if (link.getSymbol() == Argument::LOC_ROLE) {
					opp = _new SemOPP(0, link.getSynNode(),
									  Proposition::LOC_PRED, head);
				}
				else {
					opp = _new SemOPP(0, link.getSynNode(),
									  Proposition::MODIFIER_PRED, head);
				}

				// replace this link with the opp
				link.replaceWithNode(opp);

				// now put this link under the opp
				link.setNext(0);
				opp->appendChild(&link);
			}
		}

		replaceWithChildren();
	} else if (_predType == Proposition::VERB_PRED) {
		// simplify verb cluster if there is a nested VP

		// note that we do not yet call simplify on children, because we
		// don't want a copula to remove itself yet

		// locate SemOPP among children
		SemOPP *opp = 0;
		for (SemNode *child = _firstChild; child; child = child->getNext()) {
			if (child->getSemNodeType() == OPP_TYPE) {
				// require the other opp to be a verb or compound pred
				if (child->asOPP().getPredicateType() == Proposition::VERB_PRED ||
					child->asOPP().getPredicateType() == Proposition::COMP_PRED)
				{
					opp = &child->asOPP();
					break;
				}

				// alternatively, the other opp can be a copula, if this one
				// is headed by an appropriate auxillary
				if (child->asOPP().getPredicateType() ==
												Proposition::COPULA_PRED &&
					SemTreeBuilder::get()->canBeAuxillaryVerb(*_head))
				{
					opp = &child->asOPP();
					break;
				}
			}
		}

		if (opp != 0) {
			// this opp doesn't seem to correspond to an actual predicate
			// (rather, it's an inflection verb, modal verb, or copula), so
			// get rid of it.

			opp->addParticle(_particle);
			opp->addAdverb(_adverb);
			opp->addNegative(_negative);
			opp->addModal(_modal);

			if (_head != 0 && SemTreeBuilder::get()->isModalVerb(*_head)) {
				opp->addModal(_head);
			}else{
				for (int i=0; i<_synNode->getNChildren(); i++) {
					if (SemTreeBuilder::get()->isModalVerb(*_synNode->getChild(i))) {
						opp->addModal(_synNode->getChild(i));
					}
				}
			}

			bool opp_node_passed = false;
			for (SiblingIterator iter(_firstChild); iter.more(); ++iter) {
				if (&*iter == opp) {
					opp_node_passed = true;
					continue;
				}

				SemNode &child = *iter;
				if (child.getSemNodeType() == LINK_TYPE) {
					child.pruneOut();
					if (opp_node_passed) {
						opp->appendChild(&child);
					} else {
						opp->prependChild(&child);
					}
				}
			}

			SemNode::simplify();

			replaceWithChildren();
		} else {
			SemNode::simplify();
		}
	} else {
		SemNode::simplify();
	}
}

void SemOPP::regularize() {
	SemNode::regularize();

	if (_predType == Proposition::COMP_PRED) {
		// for compound propositions, add link for each prop
		for (SemNode *child = _firstChild; child; child = child->getNext()) {
			if (child->getSemNodeType() == LINK_TYPE) {
				if (_n_links < MAX_OPP_LINKS) {
					child->asLink().setTangential(false);
					_links[_n_links++] = &child->asLink();
				}
			}
		}
	} else {
		
		// for everything else, fill arg slots with mentions or props, and
		// add all links to the link list
		for (SemNode *child = _firstChild; child; child = child->getNext()) {
			if (child->getSemNodeType() == LINK_TYPE) {
				if (_n_links < MAX_OPP_LINKS) {
					child->setTangential(false);
					_links[_n_links++] = &child->asLink();
				}
			}

			// Only mentions or valid branches are considered as possible arguments.
			// First mention or valid branch becomes arg1.
			// If we are not a verb or arg1 is a branch, then the next available child becomes arg2.
			// Otherwise, arg1 becomes arg2, and the next available child becomes arg1.
			if (child->getSemNodeType() == MENTION_TYPE ||
				(child->getSemNodeType() == BRANCH_TYPE && SemTreeBuilder::get()->uglyBranchArgHeuristic(*this, child->asBranch()))) {
				if (_arg1 == 0) {
					child->setTangential(false);
					_arg1 = child;
				} else if (_arg2 == 0) {
					if (_predType != Proposition::VERB_PRED ||
						_arg1->getSemNodeType() == BRANCH_TYPE) {
						child->setTangential(false);
						_arg2 = child;
					} else {
						_arg2 = _arg1;
						child->setTangential(false);
						_arg1 = child;
					}
				}
			}
		}
	}
}

bool SemOPP::createTracesEarnestly(SemReference *awaiting_ref) {
	if (_arg1 != 0 && _arg1->getSemNodeType() == BRANCH_TYPE) {
		return _arg1->createTracesEarnestly(awaiting_ref);
	} else if (_arg2 != 0 && _arg2->getSemNodeType() == BRANCH_TYPE) {
		return _arg2->createTracesEarnestly(awaiting_ref);
	} else if (_predType == Proposition::VERB_PRED ||
			 _predType == Proposition::COPULA_PRED) {
		if (_arg1 == 0) {
			_arg1 = _new SemTrace();
			prependChild(_arg1);
			_arg1->setTangential(false);
			_arg1->asTrace().setSource(awaiting_ref);
			return true;
		} else if (_arg2 == 0) {
			_arg2 = _new SemTrace();
			prependChild(_arg2);
			_arg2->setTangential(false);
			_arg2->asTrace().setSource(awaiting_ref);
			return true;
		} else {
			return SemNode::createTracesEarnestly(awaiting_ref);
		}
	} else if (_predType == Proposition::COMP_PRED) {
		bool result = false;
		for (int i = 0; i < _n_links; i++) {
			if (_links[i]->getObject() != 0) {
				result |= _links[i]->getObject()->
					createTracesEarnestly(awaiting_ref);
			}
		}
		return result;
	} else {
		return SemNode::createTracesEarnestly(awaiting_ref);
	}
}


void SemOPP::createPropositions(IDGenerator &propIDGenerator) {
	SemNode::createPropositions(propIDGenerator);

	//if (_head) {
	//	std::wcout << L"Creating proposition for SemOPP with SynNode head: (" << _head->toTextString() << L")\n";
	//	if (_head->toTextString() == std::wstring(L"shooting ")) {
	//		std::wcout << "FOUND\n";
	//	}
	//} else {
	//	std::wcout << L"Creating proposition for SemOPP with SynNode head: (<no_head>)\n";
	//}

	if (propIDGenerator.getNUsedIDs() >= MAX_SENTENCE_PROPS) {
		SessionLogger::warn("sem_tree") << "SemOPP::createPropositions(): "
				<< "Number of potential propositions exceeds MAX_SENTENCE_PROPS."
				<< "Skipping creation of proposition for this SemOPP\n";
		_proposition = 0;
		return;
	}

	SemNode *ssubj = findSyntacticSubject();

	// trim down list of arguments to only those which are valid
	if (!isValidArgument(ssubj)) {
		ssubj = 0;
	}
	if (!isValidArgument(_arg1)) {
		_arg1 = 0;
	}
	if (!isValidArgument(_arg2)) {
		_arg2 = 0;
	}
	for (int i = 0; i < _n_links; i++) {
		if (!isValidArgument(_links[i])) {
			_n_links--;
			for (int j = i; j < _n_links; j++)
				_links[j] = _links[j+1];
			i--;
		}
	}

	SemNode *argNodes[3+MAX_OPP_LINKS];
	int n_arg_nodes = SemTreeBuilder::get()->mapSArgsToLArgs(argNodes, *this,
									ssubj, _arg1, _arg2, _n_links, _links);

	// role names 0..2 depend on predicate type
	Symbol roleSyms[3] = {Argument::REF_ROLE,
						  Symbol(),
						  Symbol()};
	if (_predType == Proposition::VERB_PRED || 
		_predType == Proposition::COPULA_PRED) {
		roleSyms[0] = Argument::SUB_ROLE;
		roleSyms[1] = Argument::OBJ_ROLE;
		roleSyms[2] = Argument::IOBJ_ROLE;
	}

	// we produce a predicate if we have a verb with either sub or obj role filled, or
	// a non-verb with the subject role filled (... or copula with both sub and obj)
	bool good_pred;
	if (_predType == Proposition::VERB_PRED) {
		// For verb, require subject or object
		good_pred = argNodes[0] != 0 || argNodes[1] != 0;

		// As a special case, allow the verb if it has a "between" link as in:
		// "fighting between South and North Korean forces"
		if (!good_pred) {
			for (int l = 3; l < n_arg_nodes; l++) {
				if (argNodes[l] == 0) {
					continue;
				}
				SemLink &link = argNodes[l]->asLink();
				if (link.getSymbol() == SemOPP::BETWEEN_SYMBOL) {
					good_pred = true;
					break;
				}
			}			
		}
	} else if (_predType == Proposition::COPULA_PRED) {
		// for copula, require subject *and* object
		good_pred = argNodes[0] != 0 && argNodes[1] != 0;
	} else if (_predType == Proposition::NOUN_PRED ||
			 _predType == Proposition::MODIFIER_PRED ||
			 _predType == Proposition::SET_PRED) {
		// for noun and modifier, require referent, and make sure
		// the other two slots (_arg1 & _arg2) are empty
		good_pred = argNodes[0] != 0 && argNodes[1] == 0 && argNodes[2] == 0;
		if (argNodes[0] && !good_pred) {
//			std::cout << "--\n" << *this << "\n" << *_synNode->getParent() << "\n";
//			std::cout.flush();

			// EMB: added SET_PRED to this statement 6/17/06 to try to fix mysterious bug
			//if (_predType == Proposition::SET_PRED) {
			//	SessionLogger::warn("sem_tree") <<
			//		"Set proposition had to lose non-link argument -- talk to Liz about this, please.\n";
			//}

			// In order to avoid arguments with no role symbols, we lose the
			// arguments, but keep the proposition:
			argNodes[1] = 0;
			argNodes[2] = 0;
			good_pred = true;
		}
	} else {
		// for everything else, just require *some* argument somewhere
		good_pred = argNodes[0] != 0 || n_arg_nodes > 3;
	}

	if (!good_pred) {
		_proposition = 0;
		return;
	}

	int n_arguments = 0;
	for (int j = 0; j < n_arg_nodes; j++) {
		if (argNodes[j] != 0)
			n_arguments++;
	}
	if (_predType == Proposition::NAME_PRED) {
		n_arguments++; // for the name itself, which has no sem node
	}

	int id = propIDGenerator.getID();
	//std::cout << "Creating proposition " << id << "\n";
	_proposition = _new Proposition(id, _predType, n_arguments);
	_proposition->setPredHead(_head);
	_proposition->setParticle(_particle);
	_proposition->setAdverb(_adverb);
	_proposition->setNegation(_negative);
	_proposition->setModal(_modal);

	int arg_no = 0;
	for (int k = 0; k < 3; k++) {
		if (argNodes[k] != 0) {
			Argument newArg;
			if (argNodes[k]->isReference()) {
				SemReference &argNode = argNodes[k]->asReference();
				newArg.populateWithMention(roleSyms[k],
										   argNode.getMention()->getIndex());
			} else if (argNodes[k]->getSemNodeType() == OPP_TYPE) {
				SemOPP &argNode = argNodes[k]->asOPP();
				newArg.populateWithProposition(roleSyms[k],
											   argNode.getProposition());
			} else if (argNodes[k]->getSemNodeType() == BRANCH_TYPE) {
				SemBranch &argNode = argNodes[k]->asBranch();
				newArg.populateWithProposition(roleSyms[k],
									argNode.getAssociatedProposition());
			} else {
				std::ostringstream ostr;
				ostr << "\n"
						  << *_synNode << "\n"
						  << *this << "\n";
				SessionLogger::info("SERIF") << ostr.str();
				throw InternalInconsistencyException(
					"SemOPP::createPropositions()",
					"One of the basic 3 roles not occupied by mention or prop");
			}
			_proposition->setArg(arg_no++, newArg);
		}
	}

	for (int l = 3; l < n_arg_nodes; l++) {
		
		if (argNodes[l] == 0) {
			continue;
		}

		SemLink &link = argNodes[l]->asLink();

		Argument newArg;

		if (link.isQuote()) {
			newArg.populateWithText(link.getSymbol(),
									link.getObject()->getSynNode());
		} else if (link.getObject()->isReference()) {
			const Mention *mention =
				link.getObject()->asReference().getMention();
			const SynNode *node = mention->getNode();

			newArg.populateWithMention(link.getSymbol(), mention->getIndex());
		} else if (link.getObject()->getSemNodeType() == BRANCH_TYPE) {
			SemBranch &branch = link.getObject()->asBranch();

			newArg.populateWithProposition(link.getSymbol(),
										   branch.getAssociatedProposition());
		} else if (link.getObject()->getSemNodeType() == OPP_TYPE) {
			SemOPP &opp = link.getObject()->asOPP();

			newArg.populateWithProposition(link.getSymbol(),
										   opp.getProposition());
		} else {
			throw InternalInconsistencyException(
				"SemOPP::createProposition()",
				"Semantic type of argument node is unrecognized");
		}

		_proposition->setArg(arg_no++, newArg);
	}

	// for names, quietly slip in the name argument
	if (_predType == Proposition::NAME_PRED) {
		if (arg_no != 1) {
			std::ostringstream ostr;
			ostr << "\n--\n" << *this << "\n";
			SessionLogger::info("SERIF") << ostr.str();
		}

		Argument newArg;
		newArg.populateWithText(Symbol(), _synNode);
		_proposition->setArg(arg_no++, newArg);
	}

	if (arg_no != n_arguments) {
		throw InternalInconsistencyException(
			"SemOPP::createProposition()",
			"arg_no != n_arguments");
	}
}


SemNode *SemOPP::findSyntacticSubject() const {

	// compound props have no subject
	if (_predType == Proposition::COMP_PRED) {
		return 0;
	}

	SemNode *parent = _parent;

	if (parent == 0) {
		return 0;
	}

	if (parent->getSemNodeType() == OPP_TYPE &&
		parent->asOPP().getPredicateType() == Proposition::COMP_PRED) {
			parent = parent->getParent(); 
	}

	if (parent == 0) {
		return 0;
	}

	if (parent->isReference()) {
		return parent;
	} else if (parent->getSemNodeType() == BRANCH_TYPE) {
		return parent->asBranch().getReference();
	}

	return 0;
}

bool SemOPP::isValidArgument(SemNode *node) {
	if (node == 0)
		return false;

	if (node->isReference()) {
		return node->asReference().getMention() != 0 &&
			   node->asReference().getMention()->getMentionType()
														!= Mention::NONE;
	}
	else if (node->getSemNodeType() == SemNode::BRANCH_TYPE) {
		return node->asBranch().getAssociatedProposition() != 0;
	}
	else if (node->getSemNodeType() == SemNode::OPP_TYPE) {
		return node->asOPP().getProposition() != 0;
	}
	else if (node->getSemNodeType() == SemNode::LINK_TYPE) {
		if (node->asLink().isQuote())
			return node->asLink().getObject() != 0;
		else
			return isValidArgument(node->asLink().getObject());
	}
	else {
		return false;
	}
}


void SemOPP::listPropositions(PropositionSet &result) {
	if (_proposition != 0) {
		result.takeProposition(_proposition);
		_proposition = 0; // so it doesn't get deleted twice
	}

	SemNode::listPropositions(result);
}


void SemOPP::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	if (_tangential)
		out << "(OPP) {";
	else
		out << "OPP {";

	out << newline << "  - type: " << Proposition::getPredTypeString(_predType);

	if (!_headSym.is_null()) {
		out << newline << "  - symbol: " << _headSym.to_debug_string();
	}

	if (_predType == Proposition::NAME_PRED) {
		out << newline << "  - text: ";
		Symbol symArray[MAX_SENTENCE_TOKENS];
		int n_tokens = _synNode->getTerminalSymbols(symArray, 
													MAX_SENTENCE_TOKENS);
		out << "\"";
		if (n_tokens > 0)
			out << symArray[0].to_debug_string();
		for (int i = 1; i < n_tokens; i++)
			out << " " << symArray[i].to_debug_string();
		out << "\"";
	}

	if (_head != 0) {
		out << newline << "  - head: ";
		_head->dump(out, indent + 10);
	}
	if (_modal != 0) {
		out << newline << "  - modal: ";
		_modal->dump(out, indent + 11);
	}
	if (_negative != 0) {
		out << newline << "  - negative: ";
		_negative->dump(out, indent + 14);
	}
	SemNode* sub = findSyntacticSubject();
	if (sub != 0) {
		const Mention* ment = sub->asReference().getMention();
		int index = -1;
		if (ment) {
			index = ment->getIndex();
		}
		std::string s = "<no_syn_node>";
		if (sub->getSynNode()) {
			s = sub->getSynNode()->toDebugTextString();
		}
		out << newline << "  - sub(" << index << "): " << s;
	}
	if (_arg1 != 0) {
		out << newline << "  - arg1: " << _arg1->getSynNode()->toDebugTextString();
	}
	if (_arg2 != 0) {
		out << newline << "  - arg2: " << _arg2->getSynNode()->toDebugTextString();
	}

	for (SemNode *child = _firstChild; child; child = child->getNext()) {
		out << newline << "  ";
		child->dump(out, indent + 2);
	}

	out << "}";

	delete[] newline;
}


