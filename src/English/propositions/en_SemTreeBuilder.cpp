// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
// For documentation of this class, please see http://speechweb/text_group/default.aspx

#include "Generic/common/leak_detection.h"

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/WordConstants.h"
#include "Generic/propositions/PropositionFinder.h"
#include "English/propositions/en_SemTreeBuilder.h"
#include "Generic/propositions/sem_tree/SemBranch.h"
#include "Generic/propositions/sem_tree/SemReference.h"
#include "Generic/propositions/sem_tree/SemMention.h"
#include "Generic/propositions/sem_tree/SemTrace.h"
#include "Generic/propositions/sem_tree/SemOPP.h"
#include "Generic/propositions/sem_tree/SemLink.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/EntityType.h"
#include "English/parse/en_STags.h"
#include "English/descriptors/en_TemporalIdentifier.h"
#include <boost/foreach.hpp>


using namespace std;


static Symbol sym_be(L"be");
static Symbol sym_is(L"is");
static Symbol sym__s(L"'s");
static Symbol sym_are(L"are");
static Symbol sym__re(L"'re");
static Symbol sym_was(L"was");
static Symbol sym_were(L"were");
static Symbol sym_been(L"been");
static Symbol sym_being(L"being");

static Symbol sym_to(L"to");

static Symbol sym_not(L"not");
static Symbol sym_n_t(L"n't");
static Symbol sym_never(L"never");

static Symbol sym_would(L"would");
static Symbol sym_will(L"will");
static Symbol sym_wo(L"wo");
static Symbol sym_could(L"could");
static Symbol sym_should(L"should");
static Symbol sym_can(L"can");
static Symbol sym_ca(L"ca");
static Symbol sym_might(L"might");
static Symbol sym_may(L"may");
static Symbol sym_shall(L"shall");

static Symbol sym_has(L"has");
static Symbol sym_have(L"have");
static Symbol sym_had(L"had");

static Symbol sym_that(L"that");

static Symbol sym_here(L"here");
static Symbol sym_there(L"there");
static Symbol sym_abroad(L"abroad");
static Symbol sym_overseas(L"overseas");
static Symbol sym_home(L"home");

static Symbol sym_by(L"by");
static Symbol sym_of(L"of");

static Symbol sym_beginning(L"beginning");
static Symbol sym_start(L"start");
static Symbol sym_end(L"end");
static Symbol sym_close(L"close");
static Symbol sym_middle(L"middle");

static Symbol sym_rid(L"rid");
static Symbol sym_rid_of(L"rid_of");

// known-to-be transitive verb past participles
static int n_xitive_verbs;
static Symbol xitiveVerbs[1000];


void EnglishSemTreeBuilder::initializeSymbols() {
	// this really ought to be a hash table instead of a list
	// ...and it should be read in from a file, too...
	n_xitive_verbs = 0;
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"found");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"killed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"murdered");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"assassinated");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"slain");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"executed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"eliminated");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"bumped");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"taken");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"blown");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"shot");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"stabbed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"gunned");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"attacked");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"assaulted");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"knifed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"bayonetted");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"stabbed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"hit");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"beaten");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"battered");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"mutilated");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"trampled");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"tortured");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"martyred");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"maimed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"bitten");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"poisoned");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"abused");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"injured");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"harmed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"wounded");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"damaged");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"gouged");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"crippled");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"lamed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"disabled");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"incapacitated");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"bruised");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"contused");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"crushed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"lacerated");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"scalded");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"disfigured");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"scarred");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"smashed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"sliced");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"raped");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"grazed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"given");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"kidnapped");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"abducted");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"snatched");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"siezed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"captured");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"accused");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"indicted");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"blamed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"charged");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"incriminated");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"arrested");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"detained");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"convicted");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"apprehended");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"collared");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"caught");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"pinched");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"nailed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"nabbed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"grabbed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"brought");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"conveyed");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"transported");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"displaced");
	xitiveVerbs[n_xitive_verbs++] = Symbol(L"founded");
}


void EnglishSemTreeBuilder::initialize() {
	TemporalIdentifier::loadTemporalHeadwordList();
	initializeSymbols();
	_treat_nominal_premods_like_names = ParamReader::getRequiredTrueFalseParam(L"treat_nominal_premods_like_names_in_props");
}


SemNode *EnglishSemTreeBuilder::buildSemTree(const SynNode *synNode,
									  const MentionSet *mentionSet) {
	return _new SemBranch(buildSemSubtree(synNode, mentionSet), synNode);
}
bool EnglishSemTreeBuilder::non1stPerCopula(Symbol head){
    return (head == sym_be ||
		head == sym_is ||
		head == sym__s ||
		head == sym_are ||
		head == sym__re ||
		head == sym_was ||
		head == sym_were ||
		head == sym_been ||
		head == sym_being);
}
////////////////////////////////
// Note that until the pronoun linker is re-trained, this method needs to pretend that "am" is not a copula
// else the score for events on adj-5 will drop 0.4 (as of 1 July 2013)
// If you are retraining, please use WordConstants::isCopula(Symbol)
///////////////////////////////
bool EnglishSemTreeBuilder::isCopula(const SynNode &node) {
	Symbol head = node.getHeadWord();
	return non1stPerCopula(head);
}

bool EnglishSemTreeBuilder::isNegativeAdverb(const SynNode &node) {
	const SynNode *foo;
	if (node.getTag() == EnglishSTags::ADVP) {
		foo = node.getHead();
	} else {
		foo = &node;
	}

	if (foo->getTag() == EnglishSTags::RB) {
		Symbol head = foo->getHeadWord();
		if (head == sym_not ||
			head == sym_n_t ||
			head == sym_never) {
			return true;
		}
	}
	return false;
}

bool EnglishSemTreeBuilder::isParticle(const SynNode &node) {
	if (node.getNChildren() == 1 &&
		(node.getTag() == EnglishSTags::PRT ||
		 node.getTag() == EnglishSTags::RP))
	{
		return true;
	}
	return false;
}

bool EnglishSemTreeBuilder::isAdverb(const SynNode &node) {
	const SynNode *foo;
	if (node.getTag() == EnglishSTags::ADVP) {
		if (node.getNChildren() != 1)
			return false;
		foo = node.getHead();
	}
	else {
		foo = &node;
	}

	if (foo->getNChildren() == 1 &&
		foo->getTag() == EnglishSTags::RB)
	{
		return true;
	}
	return false;
}

bool EnglishSemTreeBuilder::isModalVerb(const SynNode &node) {
	return node.getTag() == EnglishSTags::MD;
}

bool EnglishSemTreeBuilder::canBeAuxillaryVerb(const SynNode &node) {
	if (isModalVerb(node))
		return true;

	if (isCopula(node))
		return true;

	Symbol word = node.getHeadWord();
	if (word == sym_has ||
		word == sym_have ||
		word == sym_had)
	{
		return true;
	}

	return false;
}


bool EnglishSemTreeBuilder::isTemporalNP(const SynNode &node, const MentionSet *mentionSet) {
	// in 2009 version, temporals within temporals are okay
	if (PropositionFinder::getUse2009Props()) {
		return TemporalIdentifier::looksLikeTemporal(&node, mentionSet, false);
	} else {
		return TemporalIdentifier::looksLikeTemporal(&node, mentionSet);
	}
}

SemOPP *EnglishSemTreeBuilder::findAssociatedPredicateInNP(SemLink &link) {
	if (link.getParent() != 0 &&
		link.getParent()->getSemNodeType() == SemNode::MENTION_TYPE) {
		SemMention &parent = link.getParent()->asMention();

		SemOPP *noun = 0;
		if (link.getSymbol() == Argument::POSS_ROLE ||
			link.getSymbol() == Argument::UNKNOWN_ROLE) {
			// for possessives and premod/unknown's, search forwards
			for (SemNode *sib = &link; sib; sib = sib->getNext()) {
				if (sib->getSemNodeType() == SemNode::OPP_TYPE &&
					sib->asOPP().getPredicateType() == Proposition::NOUN_PRED) {
					noun = &sib->asOPP();
					break;
				}
			}
		} else if (link.getSymbol() == Argument::TEMP_ROLE) {
			// for temporals, search among all nodes and keep last one
			for (SemNode *sib = link.getParent()->getFirstChild();
				 sib != 0;
				 sib = sib->getNext()) {
				if (sib->getSemNodeType() == SemNode::OPP_TYPE &&
					sib->asOPP().getPredicateType() == Proposition::NOUN_PRED) {
					noun = &sib->asOPP();
				}
			}
		} else {
			// for other links, search among previous sibs and keep last one
			for (SemNode *sib = link.getParent()->getFirstChild();
				 sib != &link;
				 sib = sib->getNext()) {
				if (sib->getSemNodeType() == SemNode::OPP_TYPE &&
					sib->asOPP().getPredicateType() == Proposition::NOUN_PRED) {
					noun = &sib->asOPP();
				}
			}
		}

		return noun;
	}

	return 0;
}

bool EnglishSemTreeBuilder::uglyBranchArgHeuristic(SemOPP &parent, SemBranch &child) {
	const SynNode *parse = parent.getSynNode();

	if (parse == 0) {
		return true;
	}

	for (int i = 0; i < parse->getNChildren(); i++) {
		if (parse->getChild(i)->getTag() == EnglishSTags::COMMA) {
			return false;
		}
	}
	return true;
}

int EnglishSemTreeBuilder::mapSArgsToLArgs(
	SemNode **largs, SemOPP &opp, SemNode *ssubj,
	SemNode *sarg1, SemNode *sarg2, int n_links, SemLink **links) {
	if (opp.getPredicateType() == Proposition::VERB_PRED &&
		(isPassiveVerb(*opp.getHead()) ||
		 ((sarg1 == 0 && sarg2 == 0) &&
		  isKnownTransitiveVerb(*opp.getHead())))) {
		// passive verb

		SemNode *by_obj = 0;
		int n_other_links = 0; // ie, other than a link headed by "by"
		SemNode **other_links = _new SemNode*[n_links];
		for (int i = 0; i < n_links; i++) {
			if (by_obj == 0 &&
				links[i]->getSymbol() == sym_by) {
				by_obj = links[i]->getObject();
			} else {
				other_links[n_other_links++] = links[i];
			}
		}

		largs[0] = by_obj;
		largs[1] = ssubj;
		largs[2] = sarg1;
		for (int j = 0; j < n_other_links; j++) {
			largs[3+j] = other_links[j];
		}

		delete[] other_links;

		return 3 + n_other_links;
	} else {
		// active verb, or not a verb at all

		largs[0] = ssubj;
		largs[1] = sarg1;
		largs[2] = sarg2;
		for (int i = 0; i < n_links; i++) {
			largs[3+i] = links[i];
		}

		return 3+n_links;
	}
}



SemNode *EnglishSemTreeBuilder::buildSemSubtree(const SynNode *synNode,
										 const MentionSet *mentionSet) {
	static bool init = false;
	static bool make_partitive_props = false;
	if (!init) {
		make_partitive_props = ParamReader::getRequiredTrueFalseParam("make_partitive_props");
		init = true;
	}

#if defined(_WIN32)
//	HeapChecker::checkHeap("EnglishSemTreeBuilder::buildSemTree()");
#endif

/*	std::cout << "/----\n" << *synNode << "\n";
	std::cout.flush();
*/

	if (synNode->isTerminal()) {
		return 0;
	}

	// branch out according to constituent type:
	Symbol synNodeTag = synNode->getTag();

	if (synNodeTag == EnglishSTags::TOP) {
		return analyzeChildren(synNode, mentionSet);
	} else if (synNodeTag == EnglishSTags::S ||
			 synNodeTag == EnglishSTags::SBAR ||
			 synNodeTag == EnglishSTags::SINV ||
			 synNodeTag == EnglishSTags::FRAG ||
			 synNodeTag == EnglishSTags::PRN) {
		// for sentence, make a branch node ordinarily; but if there's a
		// preposition or WH-word linking it back to it's parent, make
		// a link
		Symbol first_child_sym = Symbol();
		if (synNode->getNChildren() > 0) {
			first_child_sym = synNode->getChild(0)->getTag();
		}

		if (first_child_sym == EnglishSTags::IN) {
			return _new SemLink(analyzeChildren(synNode, mentionSet),
								synNode,
								synNode->getChild(0)->getHeadWord(),
								synNode->getChild(0));
		} else if (first_child_sym == EnglishSTags::WHADVP &&
				 synNode->getChild(0)->getNChildren() == 1) {
			return _new SemLink(analyzeChildren(synNode, mentionSet),
								synNode,
								synNode->getChild(0)->getHeadWord(),
								synNode->getChild(0)->getChild(0));
		} else {

			// no linker word -- just a regular branch
			SemNode *semChildren = analyzeChildren(synNode, mentionSet);

			// if there's a negative here, move it to any opp we find
			const SynNode *negative = 0;
			for (int i = 0; i < synNode->getNChildren(); i++) {
				if (isNegativeAdverb(*synNode->getChild(i))) {
					negative = synNode->getChild(i);
					break;
				}
			}
			if (negative != 0) {
				for (SemNode *semChild = semChildren; semChild; semChild = semChild->getNext()) {
					if (semChild->getSemNodeType() == SemNode::OPP_TYPE) {
						semChild->asOPP().addNegative(negative);
					}
				}
			}

			return _new SemBranch(semChildren, synNode);
		}
	// PRN now handled as S (above)
/*	} else if (synNodeTag == EnglishSTags::PRN) {
		// a parenthetical -- pass up the children
		return analyzeChildren(synNode, mentionSet);
	*/
	} else if (synNode->hasMention() &&
			 (synNode->getTag() != EnglishSTags::NN &&
			  synNode->getTag() != EnglishSTags::NNS &&
			  synNode->getTag() != EnglishSTags::NNP &&
			  synNode->getTag() != EnglishSTags::NNPS)) {
		const Mention *mention = mentionSet->getMentionByNode(synNode);

		if (mention == 0) {
			throw InternalInconsistencyException(
				"EnglishSemTreeBuilder::buildSemTree()",
				"Given SynNode thinks it has a mention but no mention found");
		}

		// because of an unfortunate idiosyncracy in the mention finder,
		// there are some mentions which have a single LIST child, and are
		// coreferent with that LIST, and should therefore be discarded as
		// redundant
		const SynNode *singleListChild = 0;
		for (int i = 0; i < synNode->getNChildren(); i++) {
			if (synNode->getChild(i)->hasMention()) {
				if (singleListChild != 0) {
					// the list child we found is not the only one so it
					// doesn't count
					singleListChild = 0;
					break;
				} else {
					Mention *childMention =
						mentionSet->getMentionByNode(synNode->getChild(i));
					if (childMention->getMentionType() == Mention::LIST) {
						singleListChild = synNode->getChild(i);
					} else {
						// again, found a non-list child, so there can be no
						// "single" list child
						singleListChild = 0;
						break;
					}
				}
			}
		}
		if (singleListChild != 0) {
			SemNode *result = buildSemSubtree(singleListChild, mentionSet);

			SemNode *otherChildren = analyzeOtherChildren(
									synNode, singleListChild, mentionSet);
			for (SemNode::SiblingIterator iter(otherChildren); iter.more(); ++iter) {
				result->appendChild(&*iter);
			}

			return result;
		}

		if (mention->getMentionType() == Mention::NONE) {
			// this may be the head of a name
			if (mention->getParent() &&
				mention->getParent()->getMentionType() == Mention::NAME) {
				// The name OPP is generated when the parent name is
				// processed, so don't analyze any further
				return 0;
			} else if (synNode->getNChildren() == 1 &&
					 synNode->getHeadWord() == sym_that) {
				// "... including that of ..." construction
				return _new SemOPP(0, synNode, Proposition::NOUN_PRED, synNode);
			} else {
				// This is an empty mention, so just go through it
				return analyzeChildren(synNode, mentionSet);
			}
		} else if (mention->getMentionType() == Mention::PART && !make_partitive_props) {
			// for partitives, make sure we don't create a link between
			// the part and the whole
			SemNode *children = analyzeChildren(synNode, mentionSet);
			SemMention *result = _new SemMention(children, synNode, mention, true);

			// remove that link
			for (SemNode::SiblingIterator iter(children); iter.more(); ++iter) {
				if ((*iter).getSemNodeType() == SemNode::LINK_TYPE &&
					(*iter).asLink().getSymbol() == sym_of) {
					(*iter).replaceWithChildren();
				}
			}

			return result;
		} else if (mention->getMentionType() == Mention::LIST) {
			// looking at list of conjoined mentions

			SemNode *semChildren = analyzeChildren(synNode, mentionSet);

			// This OPP will be the "set" defining the mention
			SemOPP *opp = _new SemOPP(semChildren, synNode,
									  Proposition::SET_PRED, 0);

			SemOPP *noun = 0;

			// replace all mentions with a "member" link to that mention
			for (SemNode::SiblingIterator iter(semChildren); iter.more(); ++iter) {
				SemNode &child = *iter;

				if (child.getSemNodeType() == SemNode::MENTION_TYPE) {
					SemLink *newLink = _new SemLink(0, child.getSynNode(),
													Argument::MEMBER_ROLE,
													child.getSynNode());
					child.replaceWithNode(newLink);
					newLink->appendChild(&child);
				} else if (noun == 0 &&
						 child.getSemNodeType() == SemNode::OPP_TYPE &&
						 child.asOPP().getPredicateType() == Proposition::NOUN_PRED) {
					child.pruneOut();
					child.setNext(0);
					noun = &child.asOPP();
				}
			}

			if (noun != 0) {
				opp->setNext(noun);
			}

			return _new SemMention(opp, synNode, mention, false);
		} else if (mention->getEntityType().isTemp()) {
			// If the mention type is any of those that we haven't tested for
			// yet, then we want to see if this is actually a temporal
			// mention.
			SemMention *semMention =
				_new SemMention(analyzeChildren(synNode, mentionSet),
								synNode, mention, false);

			return _new SemLink(semMention, synNode, Argument::TEMP_ROLE,
														synNode, false);
		
/*		} else if (mention->getMentionType() == Mention::PART) {

			// THIS WAS NEVER GOING TO FIRE, SO I COMMENTED IT OUT

			// for partitives, we have to decide whether to create a
			// separate mention or not

			// get partitive head
			SynNode *headNode = mention->getChild()->getNode();

			SemNode *object = buildSemTree(headNode, mentionSet);
			// this is only a single-mention partitive if the object
			// is an indefinite NP

			// otherwise, create a separate mention with a partitive predicate
			SemLink *of_link = _new SemLink(object, partitive_head,
											Argument::poss(), synNode);
			SemOPP partitive_opp = _new SemOPP(_new SemUnit[] {of_link}, synNode, synNode.getEnglishNPHeadPreterm(),
				PredTypes.PARTITIVE_PREDICATION);
			result = _new SemMention(_new SemUnit[] {partitive_opp}, synNode, is_definite);

			// Start by creating a SemMention for this node
			SemMention *result = _new SemMention(
				analyzeChildren(synNode, mentionSet), synNode, mention, false);

			return result;
		*/
		} else if (mention->getMentionType() == Mention::APPO) {
			SemMention *result =
				_new SemMention(analyzeChildren(synNode, mentionSet),
								synNode, mention, false);

			return result;
		} else if (mention->getMentionType() == Mention::NAME ||
		           mention->getMentionType() == Mention::NEST) 
		{

			// locate the node which we want to associate with the name
			const SynNode *nameNode = synNode;
			if (mention->getChild()) {
				nameNode = mention->getChild()->getNode();
			}

			SemOPP *nameOPP =
				_new SemOPP(0, nameNode, Proposition::NAME_PRED, 0);

			if (nameNode == synNode) {
				SemMention *result =
					_new SemMention(nameOPP, synNode, mention, true);

				// put together list of any nested names
				SemNode *nested = analyzeNestedNames(synNode, mentionSet); 
				if (nested != 0)
					result->appendChild(nested);

				return result;
			} else {
				// this name has other stuff in it, so use that big
				// name-noun-name heuristic

				SemMention *result =
					_new SemMention(0, synNode, mention, true);

				// check for city-state construction
				const SynNode *state = getStateOfCity(synNode, mentionSet);
				if (state) {
					SemMention *stateMention = _new SemMention(0, state,
						mentionSet->getMentionByNode(state), true);
					SemLink *stateLink = _new SemLink(stateMention, state,
						Argument::LOC_ROLE, state);
					SemOPP *stateOPP = _new SemOPP(stateLink, state,
						Proposition::LOC_PRED, 0);
					result->appendChild(nameOPP);
					result->appendChild(stateOPP);
					return result;
				}

				// put together list of non-name children
				SemNode *children = analyzeChildren(synNode, mentionSet);

				applyNameNounNameHeuristic(*result, children, true, mentionSet);

				result->appendChild(nameOPP);

				return result;
			}
		} else if (synNodeTag == EnglishSTags::PRPS ||
				 (synNodeTag == EnglishSTags::NPPRO &&
				  synNode->getNChildren() == 1 &&
				  synNode->getHead()->getTag() == EnglishSTags::PRPS)) {
			// if we have a possessive pronoun, make a special link

			SemOPP *pronOPP = _new SemOPP(0, synNode,
										  Proposition::PRONOUN_PRED, synNode);
			SemMention *pronoun = _new SemMention(pronOPP, synNode,
												  mention, true);
			return _new SemLink(pronoun, synNode, Argument::POSS_ROLE, synNode);
		} else if (mention->getMentionType() == Mention::PRON) {
			// if we have a regular pronoun, just make a pronoun pred and
			// SemMention

			SemOPP *pronOPP = _new SemOPP(0, synNode,
										  Proposition::PRONOUN_PRED, synNode);
			SemMention *pronoun = _new SemMention(pronOPP, synNode,
												  mention, true);
			return pronoun;
		} else if (isTemporalNP(*synNode, mentionSet)) {
			// make a link for the temporal np

			SemMention *semMention =
				_new SemMention(analyzeChildren(synNode, mentionSet),
								synNode, mention, false);

			return _new SemLink(semMention, synNode, Argument::TEMP_ROLE,
														synNode, false);
		} else {
			SemNode *children = analyzeChildren(synNode, mentionSet);
			children = applyBasedHeuristic(children);

			SemMention *result = _new SemMention(0, synNode, mention, false);
			applyNameNounNameHeuristic(*result, children, false, mentionSet);

			return result;
		}
	} else if (synNodeTag == EnglishSTags::VP && isVPList(*synNode)) {
		// looking at conjoined list of predicates

		// try to find head, which is a CC
		const SynNode *head = 0;
		for (int i = 0; i < synNode->getNChildren(); i++) {
			if (synNode->getChild(i)->getTag() == EnglishSTags::CC) {
				head = synNode->getChild(i);
				break;
			}
		}

		SemOPP *result = _new SemOPP(0, synNode, Proposition::COMP_PRED,
									 head);

		SemNode *lastPred = 0;
		for (int j = 0; j < synNode->getNChildren(); j++) {
			const SynNode *synChild = synNode->getChild(j);

			// analyze children, except when we see verbs, we wnt to treat
			// them as their own predicates
			SemNode *children = 0;
			if (synChild->getTag() == EnglishSTags::VB ||
				synChild->getTag() == EnglishSTags::VBZ ||
				synChild->getTag() == EnglishSTags::VBD ||
				synChild->getTag() == EnglishSTags::VBG ||
				synChild->getTag() == EnglishSTags::VBN) {
				children = _new SemOPP(0, synChild, Proposition::VERB_PRED,
									   synChild);
			} else {
				children = buildSemSubtree(synChild, mentionSet);
			}

			for (SemNode::SiblingIterator iter(children); iter.more(); ++iter) {
				SemNode *child = &*iter;

				// this node will be moving, and we dont want the rest
				// to move with it:
				child->setNext(0);

				if (child->getSemNodeType() == SemNode::OPP_TYPE) {
					// for the OPP, create a branch for it to live in
					child = _new SemBranch(child, child->getSynNode());
				}
				if (child->getSemNodeType() == SemNode::BRANCH_TYPE) {
					// first try to locate a predicate (for future reference)
					if (child->getFirstChild() != 0 &&
						child->getFirstChild()->getSemNodeType()
													== SemNode::OPP_TYPE)
					{
						lastPred = child->getFirstChild();
					}
					if (child->getLastChild() != 0 &&
						child->getLastChild()->getSemNodeType()
													== SemNode::OPP_TYPE)
					{
						lastPred = child->getLastChild();
					}

					// make a branch child the object of a <member> link
					SemLink *newLink = _new SemLink(child, child->getSynNode(),
													Argument::MEMBER_ROLE, 0);
					result->appendChild(newLink);
				}
				else if (lastPred != 0) {
					// attach child to predicate
					lastPred->appendChild(child);
				}
				else {
					// otherwise, just leave child on the COMP_PRED node
					result->appendChild(child);
				}
			}
		}

		return result;
	} else if (synNodeTag == EnglishSTags::VP ||
			 synNodeTag == EnglishSTags::ADJP ||
			 synNodeTag == EnglishSTags::JJ ||
			 synNodeTag == EnglishSTags::JJR ||
			 synNodeTag == EnglishSTags::JJS ||
			 synNodeTag == EnglishSTags::VBG ||
			 synNodeTag == EnglishSTags::VBN ||
			 synNodeTag == EnglishSTags::CD ||
			 synNodeTag == EnglishSTags::NN ||
			 synNodeTag == EnglishSTags::NNS ||
			 synNodeTag == EnglishSTags::NNP ||
			 synNodeTag == EnglishSTags::NNPS ||
			 synNodeTag == EnglishSTags::PRP) {
		// looking at (non-list) predicate -- create SemOPP

		// depending on the constit type, we set the predicate type, head
		// node, and for verbs, negative modifier
		Proposition::PredType predType;
		const SynNode *head;
		const SynNode *negative = 0;
		const SynNode *particle = 0;
		const SynNode *adverb = 0;

		if (synNodeTag == EnglishSTags::VP) {

			// The "to" particle just bites the dust
			// However, make sure we account for negatives
			if (synNode->getHeadWord() == sym_to) {		
				SemNode *semChildren = analyzeChildren(synNode, mentionSet);

				// if there's a negative here, move it to any opp we find
				const SynNode *negative = 0;
				for (int i = 0; i < synNode->getNChildren(); i++) {
					if (isNegativeAdverb(*synNode->getChild(i))) {
						negative = synNode->getChild(i);
						break;
					}
				}
				if (negative != 0) {
					for (SemNode *semChild = semChildren; semChild; semChild = semChild->getNext()) {
						if (semChild->getSemNodeType() == SemNode::OPP_TYPE) {
							semChild->asOPP().addNegative(negative);
						}
					}
				}
				return semChildren;
			}

			if (isCopula(*synNode->getHead())) {
				predType = Proposition::COPULA_PRED;
			} else {
				predType = Proposition::VERB_PRED;
			}
			head = synNode->getHead();

			// find first negative adverb
			// last particle that is not negative adverb
			// first adverb
			for (int i = 0; i < synNode->getNChildren(); i++) {
				const SynNode *child = synNode->getChild(i);

				if (negative == 0 && isNegativeAdverb(*child)) {
					negative = child;
				} else if (adverb == 0 && isAdverb(*child)) {
					adverb = child;
				}

				if (isParticle(*child)) {
					particle = child;
				}
			}
		} else if (synNodeTag == EnglishSTags::ADJP) {
			// as a special little rule for "get rid of...", we create a
			// nested-link ("rid_of") for ADJPs headed "rid"
			SemNode *ridOf = processRidOf(synNode, mentionSet);
			if (ridOf != 0)
				return ridOf;

			predType = Proposition::MODIFIER_PRED;
			head = synNode->getHead();
		} else if (synNodeTag == EnglishSTags::JJ ||
				 synNodeTag == EnglishSTags::JJR ||
				 synNodeTag == EnglishSTags::JJS ||
				 synNodeTag == EnglishSTags::CD ||
				 synNodeTag == EnglishSTags::VBN ||
				 synNodeTag == EnglishSTags::VBG) {
			predType = Proposition::MODIFIER_PRED;
			head = synNode;
		} else if (synNodeTag == EnglishSTags::PRP) {
			// XXX
			return 0;
		} else /* only types left are noun types NN[P][S] */ {
			predType = Proposition::NOUN_PRED;
			head = synNode;
		}

		// If we are a modifier with no negative, check our previous token to see if it is a negative
		if (predType == Proposition::MODIFIER_PRED && negative == 0) {
			const SynNode* prevTerminal = synNode->getPrevTerminal();
			if (prevTerminal) {
				Symbol prevSymbol = prevTerminal->getTag();
				if (prevSymbol == sym_not || prevSymbol == sym_n_t || prevSymbol == sym_never) {
					negative = prevTerminal->getParent();
				}
			}
		}

		SemOPP *result = _new SemOPP(analyzeChildren(synNode, mentionSet),
									 synNode, predType, head);
		result->addNegative(negative);
		result->addParticle(particle);
		result->addAdverb(adverb);

	
		return result;
	} else if (synNodeTag == EnglishSTags::PP) {
		// first see if this is actually 2 or more conjoined PPs
		int n_pp_children = 0;
		for (int i = 0; i < synNode->getNChildren(); i++) {
			if (synNode->getChild(i)->getTag() == EnglishSTags::PP)
				n_pp_children++;
		}
		if (n_pp_children >= 2)
			return analyzeChildren(synNode, mentionSet);

		// looking at a non-compound PP -- create SemLink

		const SynNode *head = synNode->getHead(); // XXX will this find VBG head?
		// XXX Also, it won't be null when it's supposed to

		SemNode *children = analyzeChildren(synNode, mentionSet);

		// we also want to see if there's a nested PP
		if (children &&
			children->getNext() == 0 &&
			children->getSemNodeType() == SemNode::LINK_TYPE)
		{
			// there's a nested link, so merge the two
			if (children->asLink().getSymbol() == Argument::TEMP_ROLE) {
				// actually, temporals don't get merged -- instead, create
				// a new temporal link which contains the whole expression

				SemLink *result = _new SemLink(
					children->getFirstChild(), synNode, Argument::TEMP_ROLE,
					synNode, children->asLink().isQuote());

				return result;
			}
			else {
				// wasn't a temporal -- just go on with the
				// nested-link merging:
				SemLink &inner = children->asLink();

				inner.setSynNode(synNode);

				std::wstringstream head_string;  
				head_string << head->getHeadWord().to_string()
							<< L"_"
							<< inner.getSymbol().to_string();
				inner.setSymbol(Symbol(head_string.str()));

				return &inner;
			}
		}
		else {
			return _new SemLink(children, synNode,
								head->getHeadWord(), head);
		}
	} else if (synNodeTag == EnglishSTags::ADVP) {
		if (isLocativePronoun(*synNode)) {
			// "here", "there", "abroad"
			SemNode *children = analyzeChildren(synNode, mentionSet);
			SemLink *result = _new SemLink(children, synNode,
										   Argument::LOC_ROLE, synNode);
			return result;
		} else {
			// see if there's a more complex locative expression
			// XXX
			return analyzeChildren(synNode, mentionSet);
		}
#if 0
		if (isLocativePronoun(synNode)) {
			return 0;
		}
		else {
			// for other ADVPs, it's possible that we'll want to create a link

			// first thing we need is an adv head
			SynNode *head = findAmongChildren(synNode,
					{EnglishSTags::RB, EnglishSTags::RBR, Symbol()});

			// second thing we need is a link
			SemLink *link = 0;
			Vector other_children = _new Vector(child_array.length);
			for (int i = 0; i < child_array.length; i++) {
				if (link == null && child_array[i] instanceof SemLink)
					link = (SemLink) child_array[i];
				else
					other_children.addElement(child_array[i]);
			}

			if (link != null && head != null) {
				// if we have both, then make a new Link

				// put together list of all children of both this node and the link child
				child_array = _new SemUnit[/*other_children.size() +*/ link.getNChildren()];
				int j = 0;
				/*        for (int i = 0; i < other_children.size(); i++)
				child_array[j++] = (SemUnit) other_children.elementAt(i); */
				for (int i = 0; i < link.getNChildren(); i++)
					child_array[j++] = link.getChild(i);

				if (isLocativePronoun(head)) {
					// this is a case of "here in Bangkok"
					SemOPP pronoun_opp = _new SemOPP(_new SemUnit[] {link}, head,
						head, PredTypes.PRONOUN_PREDICATION);
					SemMention pronoun_mention = _new SemMention(_new SemUnit[] {pronoun_opp},
						synNode, true);
					result = _new SemLink(_new SemUnit[] {pronoun_mention}, synNode,
						SemLink.LOCATION_LINK, head);
				}
				else {
					// this a case of "out of ..." (double-preposition)
					// _new head symbol is combination of this head and the link's head
					Symbol head_sym = SymbolMap.add(head.getHeadWord().toString() + '_' + link.getHeadSymbol().toString());

					result = _new SemLink(child_array, synNode, SemLink.PREPOSITION_LINK, head, head_sym);
				}
			}
			else {
				result = null;

				// it's also possible that there's a name here which we should keep
				for (int i = 0; i < child_array.length; i++) {
					if (child_array[i] instanceof SemMention)
						result = child_array[i];
				}
			}
		}
#endif
	} else if (synNodeTag == EnglishSTags::NPPOS) {
		// we have a possessive, so make a special link

		return _new SemLink(analyzeChildren(synNode, mentionSet),
							synNode, Argument::POSS_ROLE, synNode);
	} else if (synNodeTag == EnglishSTags::WHNP) {
		// The propfinder doesn't (yet) know how to correctly descend
		// WH-phrases, so we avoid trouble by leaving them out. (This was
		// also the behavior of the old java propfinder.)
		return 0;
	} else {
		return analyzeChildren(synNode, mentionSet);
	}
}

SemNode *EnglishSemTreeBuilder::analyzeChildren(const SynNode *synNode,
										 const MentionSet *mentionSet) {
	if (synNode->isTerminal()) {
		return 0;
	}

	SemNode *firstResult = 0;
	SemNode *lastResult = 0;

	for (int i = 0; i < synNode->getNChildren(); i++) {
		// append the linked list returned by buildSemTree to our result list
		SemNode *result = buildSemSubtree(synNode->getChild(i), mentionSet);
//		std::cout << "--\n" << *result << "\n";
		if (lastResult) {
			lastResult->setNext(result);
		} else {
			firstResult = result;
			lastResult = result;
		}
		if (lastResult) {
			while (lastResult->getNext() != 0) {
				lastResult = lastResult->getNext();
			}
		}
	}

	return firstResult;
}

SemNode *EnglishSemTreeBuilder::analyzeOtherChildren(const SynNode *synNode,
	const SynNode *exception, const MentionSet *mentionSet)
{
	if (synNode->isTerminal())
		return 0;

	SemNode *firstResult = 0;
	SemNode *lastResult = 0;

	for (int i = 0; i < synNode->getNChildren(); i++) {
		if (synNode->getChild(i) == exception)
			continue;

		// append the linked list returned by buildSemTree to our result list
		SemNode *result = buildSemSubtree(synNode->getChild(i), mentionSet);
		if (lastResult) {
			lastResult->setNext(result);
		}
		else {
			firstResult = result;
			lastResult = result;
		}
		if (lastResult) {
			while (lastResult->getNext() != 0)
				lastResult = lastResult->getNext();
		}
	}

	return firstResult;
}

SemNode *EnglishSemTreeBuilder::analyzeNestedNames(const SynNode *synNode,
                                                   const MentionSet *mentionSet) 
{

	SemNode *firstResult = 0;
	SemNode *lastResult = 0;

	for (int i = 0; i < synNode->getNChildren(); i++) {

		const SynNode *child = synNode->getChild(i);
		if (child->hasMention()) {
			Mention *childMent = mentionSet->getMentionByNode(child);
			if (childMent->getMentionType() == Mention::NEST) {

				// append the linked list returned by buildSemTree to our result list
				SemNode *result = buildSemSubtree(child, mentionSet);
	
				if (lastResult) {
					lastResult->setNext(result);
				} else {
					firstResult = result;
					lastResult = result;
				}
				if (lastResult) {
					while (lastResult->getNext() != 0) {
						lastResult = lastResult->getNext();
					}
				}
			}
		}
	}

	return firstResult;
}


const SynNode* EnglishSemTreeBuilder::findAmongChildren(const SynNode *node,
												 const Symbol *tags)
{
	for (int i = 0; i < node->getNChildren(); i++) {
		for (int j = 0; !tags[j].is_null(); j++) {
			if (node->getChild(i)->getTag() == tags[i])
				return node->getChild(i);
		}
	}
	return 0;
}


bool EnglishSemTreeBuilder::isPassiveVerb(const SynNode &verb) {
    // sanity check -- we need to look at parent
	if (verb.getParent() == 0) {
		return false;
	}

	// find topmost VP in verb cluster
	const SynNode *top_vp = &verb;
	while (top_vp->getParent() != 0 && top_vp->getParent()->getTag() == EnglishSTags::VP) {
		top_vp = top_vp->getParent();
	}

	// figure out if we're in a reduced relative clause
	if (top_vp->getParent() != 0 &&
		top_vp->getParent()->getTag() == EnglishSTags::NP) {
		// in reduced relative, just see if verb is VBN
		if (verb.getTag() == EnglishSTags::VBN) {
			return true;
		} else {
			return false;
		}
	} else {
		// not in reduced relative, so look for copula and VBN

		if (verb.getTag() != EnglishSTags::VBN) {
			return false;
		}

		// if we find copula one level up in the parse, we're done
		const SynNode *gparent = verb.getParent()->getParent();
		if (gparent == 0) {
			return false;
		}
		if (gparent->getTag() == EnglishSTags::VP && isCopula(*gparent->getChild(0))) {
			return true;
		}

		// for conjoined VPs, the copula may be 2 levels up:
		const SynNode *ggparent = gparent->getParent();
		if (ggparent == 0) {
			return false;
		}
		if (gparent->getTag() == EnglishSTags::VP && ggparent->getTag() == EnglishSTags::VP && isCopula(*ggparent->getChild(0))) {
			return true;
		}

		// no copula found
		return false;
    }
}

bool EnglishSemTreeBuilder::isKnownTransitiveVerb(const SynNode &verb) {
	Symbol head = verb.getHeadWord();
	for (int i = 0; i < n_xitive_verbs; i++) {
		if (head == xitiveVerbs[i])
			return true;
	}
	return false;
}

bool EnglishSemTreeBuilder::isLocativePronoun(const SynNode &synNode) {
	const SynNode *head = &synNode;
	while (!head->isPreterminal()) {
		if (head->getNChildren() > 1)
			return false;
		head = head->getHead();
	}

	Symbol headWord = head->getHeadWord();
	return (headWord == sym_here ||
			headWord == sym_there ||
			headWord == sym_abroad ||
			headWord == sym_overseas ||
			headWord == sym_home);
}

bool EnglishSemTreeBuilder::isVPList(const SynNode &node) {
	int vps_seen = 0;
	int separators_seen = 0;
	for (int i = 0; i < node.getNChildren(); i++) {
		Symbol tag = node.getChild(i)->getTag();
		if (tag == EnglishSTags::VP ||
			tag == EnglishSTags::VB ||
			tag == EnglishSTags::VBZ ||
			tag == EnglishSTags::VBD ||
			tag == EnglishSTags::VBG ||
			tag == EnglishSTags::VBN)
		{
			vps_seen++;
		}
		else if (tag == EnglishSTags::COMMA ||
				 tag == EnglishSTags::CC ||
				 tag == EnglishSTags::CONJP)
		{
			separators_seen++;
		}
	}
	if (vps_seen > 1 && separators_seen > 0)
		return true;
	else
		return false;
}

const SynNode *EnglishSemTreeBuilder::getStateOfCity(const SynNode *synNode,
											  const MentionSet *mentionSet)
{
	// ensure that node is a GPE name
	if (!synNode->hasMention())
		return 0;
	const Mention *mention = mentionSet->getMentionByNode(synNode);
	if (mention->getMentionType() != Mention::NAME)
		return 0;
	if (!mention->getEntityType().matchesGPE())
		return 0;

	// go thru children and perform similar tests
	int child_no = 0;
	const SynNode *child;
	const Mention *childMention;

	// for first child, ensure that it's the mention child
	if (child_no == synNode->getNChildren())
		return 0;
	child = synNode->getChild(child_no);
	if (!child->hasMention())
		return 0;
	if (child->getTag() != EnglishSTags::NPP)
		return 0;
	childMention = mentionSet->getMentionByNode(child);
	if (childMention->getParent() != mention)
		return 0;
	child_no++;

	// for the next child, ensure that it's a comma
	if (child_no == synNode->getNChildren())
		return 0;
	// JCS: allow PRN to intervene - "Ho Chi Minh City (Saigon), Vietnam"
	if (synNode->getChild(child_no)->getTag() == EnglishSTags::PRN) {
		child_no++;
		if (child_no == synNode->getNChildren())
			return 0;
	}
	if (synNode->getChild(child_no)->getTag() != EnglishSTags::COMMA)
		return 0;
	child_no++;

	// for the next child, ensure that it's a GPE name
	if (child_no == synNode->getNChildren())
		return 0;
	child = synNode->getChild(child_no);
	if (!child->hasMention())
		return 0;
	if (child->getTag() != EnglishSTags::NPP)
		return 0;
	childMention = mentionSet->getMentionByNode(child);
	if (childMention->getMentionType() != Mention::NAME)
		return 0;
	if (!childMention->getEntityType().matchesGPE())
		return 0;

	// and make sure there aren't too many more children
	if (synNode->getNChildren() - child_no > 2)
		return 0;

	// we made it -- return that GPE name as the state
	return synNode->getChild(child_no);
}

// as a special little rule for "get rid of...", we create a
// nested-link ("rid_of") for ADJPs headed "rid"
SemNode *EnglishSemTreeBuilder::processRidOf(const SynNode *synNode,
									  const MentionSet *mentionSet)
{
	if (synNode->getHeadWord() != sym_rid)
		return 0;
	SemNode *children = analyzeChildren(synNode, mentionSet);
	if (children == 0 ||
		children->getSemNodeType() != SemNode::OPP_TYPE ||
		children->asOPP().getHeadSymbol() != sym_rid)
	{
		return 0;
	}
	// skip the "rid" child
	children = children->getNext();
	if (children == 0 ||
		children->getSemNodeType() != SemNode::LINK_TYPE ||
		children->asLink().getSymbol() != sym_of)
	{
		return 0;
	}

	SemLink &link = children->asLink();

	link.setSynNode(synNode);
	link.setSymbol(sym_rid_of);

	return children;
}


// See if we have a "such-and-such -based" or "such-and-such -led" among
// the children of an NP
SemNode *EnglishSemTreeBuilder::applyBasedHeuristic(SemNode *children) {
	SemNode *results = 0, *lastResult;

	for (SemNode::SiblingIterator iter(children); iter.more(); ++iter) {
		SemNode &child = *iter;

		SemNode *nextNode = 0;

		if (child.getSemNodeType() == SemNode::MENTION_TYPE &&
			child.getNext() != 0 &&
			child.getNext()->getSemNodeType() == SemNode::OPP_TYPE) {
			SemOPP &opp = child.getNext()->asOPP();
			if (!opp.getHeadSymbol().is_null() &&
				wcslen(opp.getHeadSymbol().to_string()) > 3 &&
				opp.getHeadSymbol().to_string()[0] == L'-') {
				child.setNext(0);
				SemLink *link = _new SemLink(&child, child.getSynNode(),
											 Argument::UNKNOWN_ROLE,
											 child.getSynNode());
				nextNode = _new SemOPP(link, opp.getSynNode(),
									   Proposition::MODIFIER_PRED,
									   opp.getSynNode());

				// we used up two of those children, not just one, so
				// increment iter an extra time
				++iter;
			} else {
				nextNode = &child;
			}
		} else {
			nextNode = &child;
		}

		if (nextNode != 0) {
			if (results == 0) {
				results = nextNode;
				lastResult = nextNode;
			} else {
				lastResult->setNext(nextNode);
				lastResult = nextNode;
			}
		}
	}

	return results;
}

// apply name/noun/name heuristic for NP's
// (For stuff like "IBM president Bob Smith" and its sundry variations)
void EnglishSemTreeBuilder::applyNameNounNameHeuristic(SemMention &mentionNode,
												SemNode *children,
												bool contains_name,
												const MentionSet *mentionSet) {
#if 0
	std::cout << "\n***\n";
	mentionNode.dump(std::cout);
	std::cout << "\n---\n";
	for (SemNode *child = children; child; child = child->getNext()) {
		std::cout << " -";
		child->dump(std::cout, 2);
		std::cout << "\n";
	}
	std::cout << "\n";
#endif

	// the bins we'll be putting nodes into:
	std::vector<SemNode *> premods;
	std::vector<SemNode *> postmods;
	std::vector<SemNode *> premodRefs;
	SemOPP *noun = 0;

	// for each node, assign it to one of the bins
	for (SemNode::SiblingIterator iter(children); iter.more(); ++iter) {

		(*iter).setNext(0); // break node off from child list

		if ((*iter).isReference()) {
			premodRefs.push_back(&(*iter).asReference());
		} else if ((*iter).getSemNodeType() == SemNode::OPP_TYPE) {
			SemOPP *opp = &(*iter).asOPP();
			if (opp->getPredicateType() == Proposition::NOUN_PRED) {

				// What we considered the noun is apparently not the final noun
				if (noun != 0) {

					Mention* mentionForNoun = 0;
					if (noun->getHead()->hasMention()) {
						mentionForNoun = mentionSet->getMentionByNode(noun->getSynNode());
					}					
					
					// If _treat_nominal_premods_like_names is TRUE: 
					//   If it is a mention and of a recognized type, turn it into a premodRef
					//   Otherwise, turn it into a regular premodifier						
					if (_treat_nominal_premods_like_names && mentionForNoun && mentionForNoun->getEntityType().isRecognized()) {
						SemMention* nounOPPAsSemMention = _new SemMention(noun, noun->getSynNode(), mentionForNoun, false);
						premodRefs.push_back(nounOPPAsSemMention);
					} else {
						noun->setPredicateType(Proposition::MODIFIER_PRED);
						premods.push_back(noun);
					}
				}
				noun = opp;
			} else if (opp->getPredicateType() == Proposition::NAME_PRED) {
				// do nothing with names
			} else {
				if (noun == 0) {
					premods.push_back(opp);
				} else {
					postmods.push_back(opp);
				}
			}
		} else {
			if (noun == 0) {
				premods.push_back(&*iter);
			} else {
				postmods.push_back(&*iter);
			}
		}
	}

	// make an unknown-role link for the premodifier reference
	std::vector<SemLink *> premodLinks;
	BOOST_FOREACH(SemNode *premodRef, premodRefs) {
		premodLinks.push_back(_new SemLink(premodRef,
								  premodRef->getSynNode(),
								  Argument::UNKNOWN_ROLE,
								  premodRef->getSynNode()));
	}

	// now add what we found to new list of children
	if (!PropositionFinder::getUseNominalPremods()) {
		// old way
		BOOST_FOREACH(SemNode *node, premods) {
			mentionNode.appendChild(node);
		}
		BOOST_FOREACH(SemLink *link, premodLinks) {
			mentionNode.appendChild(link);
		}
		if (noun) {
			mentionNode.appendChild(noun);
		}
		BOOST_FOREACH(SemNode *node, postmods) {
			mentionNode.appendChild(node);
		}
	} else {
		// new way (ACE2004)
		if (contains_name &&
			noun &&
			noun->getSynNode()->hasMention()) {
			// unlike old way, the noun is a separate mention, and the
			// premods attach to it.
			Mention *temp = mentionSet->getMentionByNode(noun->getSynNode());
			SemMention *nounMention = _new SemMention(0, noun->getSynNode(), temp, true);

			BOOST_FOREACH(SemNode *node, premods) {
				nounMention->appendChild(node);
			}
			BOOST_FOREACH(SemLink *link, premodLinks) {
				nounMention->appendChild(link);
			}
			nounMention->appendChild(noun);

			mentionNode.appendChild(nounMention);
		} else {
			BOOST_FOREACH(SemNode *node, premods) {
				mentionNode.appendChild(node);
			}			
			BOOST_FOREACH(SemLink *link, premodLinks) {
				mentionNode.appendChild(link);
			}
			if (noun) {
				mentionNode.appendChild(noun);
			}
		}
		BOOST_FOREACH(SemNode *node, postmods) {
			mentionNode.appendChild(node);
		}
	}
}


