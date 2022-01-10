// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
// For documentation of this class, please see http://speechweb/text_group/default.aspx

#include "Generic/common/leak_detection.h"

#include "Generic/common/HeapChecker.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/WordConstants.h"
#include "Generic/propositions/PropositionFinder.h"
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
#include "Spanish/common/es_WordConstants.h"
#include "Spanish/parse/es_STags.h"
#include "Spanish/propositions/es_SemTreeBuilder.h"
#include "Spanish/propositions/es_SemTreeUtils.h"


#include <boost/foreach.hpp>


using namespace std;


void SpanishSemTreeBuilder::initialize() {
	_treat_nominal_premods_like_names = ParamReader::getRequiredTrueFalseParam(L"treat_nominal_premods_like_names_in_props");
}

SemNode *SpanishSemTreeBuilder::buildSemTree(const SynNode *synNode,
									  const MentionSet *mentionSet) {
	return _new SemBranch(buildSemSubtree(synNode, mentionSet), synNode);
}


bool SpanishSemTreeBuilder::isModalVerb(const SynNode &node) {
	// Spanish builds the "modals" with morphology or nesting plain verbs
	//return false; // node.getTag() == EnglishSTags::MD;

	const SynNode* parent = node.getParent();
	return parent->getNChildren() > 1 && parent->getHead() != &node;
}

bool SpanishSemTreeBuilder::canBeAuxillaryVerb(const SynNode &node) {
	if (isModalVerb(node))
		return true;

	if (isCopula(node))
		return true;

	Symbol word = node.getHeadWord();
	if (SpanishSemTreeUtils::isHaber(word,false)) {
		return true;
	}

	return false;
}

bool SpanishSemTreeBuilder::isCopula(const SynNode &node) {
	Symbol headSym = node.getHeadWord();
	return WordConstants::isCopula(headSym);
}

bool SpanishSemTreeBuilder::isNegativeAdverb(const SynNode &node) {
	// can be called at three diff levels ....
	if (node.isTerminal()){
		return false;
	}
	const SynNode *foo = &node;

	if (foo->getTag() == SpanishSTags::SADV ) { //EnglishSTags::ADVP
		foo = foo->getHead();
	} 
	if (foo->getTag() == SpanishSTags::GRUP_ADV ) { 
		foo = foo->getHead();
	}
	if (foo->getTag() == SpanishSTags::NEG ) { 
		foo = foo->getHead();
	}
	if (foo->getTag() == SpanishSTags::POS_RN) { //EnglishSTags::RB) {
		// may not need this second test since "pos_rn" is a negative adv?
		//Symbol headword = foo->getHeadWord();
		//if (SpanishSemTreeUtils::isNegativeAdverb(headword)){
		//SessionLogger::dbg("prop_gen") << "TRUE isNegAdv" <<"  " << foo->toString(4).c_str() << "\n";
		return true;
		//}
	}
	return false;
}




SemOPP *SpanishSemTreeBuilder::findAssociatedPredicateInNP(SemLink &link) {
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

bool SpanishSemTreeBuilder::uglyBranchArgHeuristic(SemOPP &parent, SemBranch &child) {
	const SynNode *parse = parent.getSynNode();

	if (parse == 0) {
		return true;
	}

	for (int i = 0; i < parse->getNChildren(); i++) {
		if (parse->getChild(i)->getTag() == SpanishSTags::COMMA) {
			return false;
		}
	}
	return true;
}

int SpanishSemTreeBuilder::mapSArgsToLArgs(
	SemNode **largs, SemOPP &opp, SemNode *ssubj,
	SemNode *sarg1, SemNode *sarg2, int n_links, SemLink **links) {
	if (opp.getPredicateType() == Proposition::VERB_PRED &&
		(isPassiveVerb(*opp.getHead()) ||
		 ((sarg1 == 0 && sarg2 == 0) &&
		  isKnownTransitiveVerb(*opp.getHead())))) {
		// passive verb

		SemNode *by_obj = 0;
		int n_other_links = 0; // ie, other than a link headed by the passive actor tag (eng==by, sp ==por)
		SemNode **other_links = _new SemNode*[n_links];
		for (int i = 0; i < n_links; i++) {
			if (by_obj == 0 &&
				links[i]->getSymbol() == SpanishSemTreeUtils::sym_por) {
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



SemNode *SpanishSemTreeBuilder::buildSemSubtree(const SynNode *synNode,const MentionSet *mentionSet) {
	static bool init = false;
	static bool make_partitive_props = false;
	if (!init) {
		make_partitive_props = ParamReader::getRequiredTrueFalseParam("make_partitive_props");
		init = true;
	}

#if defined(_WIN32)
	//	HeapChecker::checkHeap("SpanishSemTreeBuilder::buildSemTree()");
#endif

	/*	std::cout << "/----\n" << *synNode << "\n";
	std::cout.flush();
	*/

	if (synNode->isTerminal()) {
		return 0;
	}

	// branch out according to constituent type:
	Symbol synNodeTag = synNode->getTag();

	if (synNodeTag == SpanishSTags::SENTENCE) {  // EnglishSTags::TOP 
		return analyzeChildren(synNode, mentionSet);
	} else if (synNodeTag == SpanishSTags::S ||			  // EnglishSTags::S
		synNodeTag == SpanishSTags::S_DP_SBJ ||			  // EnglishSTags::S
		synNodeTag == SpanishSTags::S_F_A ||      // adverbial comparative clause
		synNodeTag == SpanishSTags::S_F_A_DP_SBJ ||      // adverbial comparative clause
		synNodeTag == SpanishSTags::SADV ||      // adverbial phrase
		//synNodeTag == SpanishSTags::S_F_C ||      // completive clause
		//synNodeTag == SpanishSTags::S_F_R  ||    // relative clause
		//synNodeTag == SpanishSTags::S_NF_P  ||    // participle clause
		//synNodeTag == EnglishSTags::SBAR ||  
		//synNodeTag == EnglishSTags::SINV ||
		synNodeTag == SpanishSTags::S_STAR	||	// a verb-less sentence
		synNodeTag == SpanishSTags::S_F_C_STAR		// a verb-less completive phrase
		//EnglishSTags::FRAG ||
		//synNodeTag == EnglishSTags::PRN) {
		) { 
			// for sentence, make a branch node ordinarily; but if there's a
			// preposition or WH-word linking it back to it's parent, make
			// a link
			Symbol first_child_sym = Symbol();
			if (synNode->getNChildren() > 0) {
				first_child_sym = synNode->getChild(0)->getTag();
			}

			if (first_child_sym == SpanishSTags::CONJ_SUBORD || // coord conj (bare conj.cord is POS_CS)
				first_child_sym == SpanishSTags::PARTICIPI || // past or present participle
				first_child_sym == SpanishSTags::INFINITIU || // infinitive
				first_child_sym == SpanishSTags::POS_S ||   // bare preposition //xx not gonna happen
				//EnglishSTags::IN (preposition or subordinating conj)
				first_child_sym == SpanishSTags::GRUP_ADV || // compound adverbial
				first_child_sym == SpanishSTags::PREP) { // compound preposition
					return _new SemLink(analyzeChildren(synNode, mentionSet),
						synNode,
						synNode->getChild(0)->getHeadWord(),
						synNode->getChild(0));
					//// Spanish doesn't have a special treatment for "wh" adverbs .. covered as conj above
					//} else if (first_child_sym == EnglishSTags::WHADVP &&
					//		 synNode->getChild(0)->getNChildren() == 1) {
					//	return _new SemLink(analyzeChildren(synNode, mentionSet),
					//						synNode,
					//						synNode->getChild(0)->getHeadWord(),
					//						synNode->getChild(0)->getChild(0));
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
				/* DK:  The spanish parse lacks an equivalent
				 * tag to VP. As a result, we're going to force any children of a branch
				 * that occur after a verb to be children of the
				 * verb rather than siblings.
				 */
				//std::cout << "BEFORE\n" << *_new SemBranch(semChildren, synNode) << "\n";
				moveObjectsUnderVerbs(semChildren);
				//std::cout << "AFTER\n" << *_new SemBranch(semChildren, synNode) << "\n";
				return _new SemBranch(semChildren, synNode);
			}
			// a parenthetical looks like "inclusion" in Spanish
			// (old Eng note)==> PRN now handled as S (above)
	} else if (synNodeTag == SpanishSTags::INC) { //EnglishSTags::PRN) {
		// a parenthetical -- pass up the children
		return analyzeChildren(synNode, mentionSet);

	} else if (synNode->hasMention() && // exclude common and proper nouns
		(synNodeTag != SpanishSTags::POS_NC && // EnglishSTags::NN &&
		//synNodeTag != EnglishSTags::NNS &&
		synNodeTag != SpanishSTags::POS_NP )){ //EnglishSTags::NNP &&
			//synNodeTag != EnglishSTags::NNPS)) {
			const Mention *mention = mentionSet->getMentionByNode(synNode);

			if (mention == 0) {
				throw InternalInconsistencyException(
					"SpanishSemTreeBuilder::buildSemTree()",
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
					SpanishWordConstants::isRelativePronoun(synNode->getHeadWord())) {
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
						(*iter).asLink().getSymbol() == SpanishSemTreeUtils::sym_de) {
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
				SemOPP partitive_opp = _new SemOPP(_new SemUnit[] {of_link}, synNode, synNode.getSpanishNPHeadPreterm(),
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
			} else if (mention->getMentionType() == Mention::NAME) {

				// locate the node which we want to associate with the name
				const SynNode *nameNode = synNode;
				if (mention->getChild()) {
					nameNode = mention->getChild()->getNode();
				}

				SemOPP *nameOPP =
					_new SemOPP(0, nameNode, Proposition::NAME_PRED, 0);

				if (nameNode == synNode) {
					// name is atomic, so we're done

					return _new SemMention(nameOPP, synNode, mention, true);
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
				// may need to fix this to split "su amigo"(dp)  from "suyo"(px)
			} else if (synNodeTag == SpanishSTags::POS_PX ||// possessive pronoun //EnglishSTags::PRPS ||
				synNodeTag == SpanishSTags::POS_DP || //possessive determiner 
				///// the branch below is trying for a possessive pronounn phrase -- but is ?????
				(synNodeTag == SpanishSTags::SN && //EnglishSTags::NPPRO && // can't find a Spanish NP pro
				synNode->getNChildren() == 1 &&
				synNode->getHead()->getTag() == SpanishSTags::POS_PX)){ //EnglishSTags::PRPS)) {
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
			} else if (SpanishSemTreeUtils::isTemporalNP(*synNode, mentionSet)) {
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

		// and also must handle a complex verb group outside the VP context
	/*} else if (synNodeTag == SpanishSTags::GRUP_VERB || //possibly-complex verb cluster
		synNodeTag == SpanishSTags::INFINITIU ||  //infinitive construct
		//synNodeTag == SpanishSTags::RELATIU ||
		synNodeTag == SpanishSTags::GERUNDI){	  // gerund construct 

		const SynNode *head = synNode->getHead();
		Symbol nodeSym = SpanishSemTreeUtils::primaryVerbSym(synNode);
		Proposition::PredType predType;
		if (synNodeTag == SpanishSTags::INFINITIU || synNodeTag == SpanishSTags::GERUNDI){
			predType = Proposition::MODIFIER_PRED;
		}else if (isCopula(*head)) {
			predType = Proposition::COPULA_PRED;
		} else {
			predType = Proposition::VERB_PRED;
		}

				SessionLogger::dbg("prop_gen") << "GRUP_VERB for:"
		<<  "  synNodeTagTag=" <<synNodeTag.to_debug_string() << ":   using NodeSym = " << nodeSym.to_debug_string() <<" from node " << synNode->toString(4).c_str() << "\n";


		Symbol headSymbol = SpanishSemTreeUtils::primaryVerbSym(head);
		SemOPP *result;
		if (synNode->getNChildren() > 1){
			result = _new SemOPP(analyzeChildren(synNode, mentionSet), 
			synNode, predType, head, headSymbol);
		}else{
			result = _new SemOPP(0, synNode, predType, head, headSymbol);
		}
		return result;
*/
		
	} else if (synNodeTag == SpanishSTags::GRUP_VERB || //possibly-complex verb cluster
		synNodeTag == SpanishSTags::INFINITIU ||  //infinitive construct
		synNodeTag == SpanishSTags::GERUNDI){	  // gerund construct 
		
		const SynNode *head = synNode->getHead();
		Symbol nodeSym = SpanishSemTreeUtils::primaryVerbSym(synNode);
		Proposition::PredType predType;
		/*if (synNodeTag == SpanishSTags::INFINITIU || synNodeTag == SpanishSTags::GERUNDI){
			predType = Proposition::MODIFIER_PRED;
		}else*/ if (isCopula(*head)) {
			predType = Proposition::COPULA_PRED;
		} else {
			predType = Proposition::VERB_PRED;
		}
		
		SessionLogger::dbg("prop_gen") << "GRUP_VERB for:"
		<<  "  synNodeTagTag=" <<synNodeTag.to_debug_string() << ":   using NodeSym = " << nodeSym.to_debug_string() <<" from node " << synNode->toString(4).c_str() << "\n";
		/* The *head* of the verb group, if the verb group is complex,
			is the infinitive or gerund. To comply with the English code
			and the way that the SemOPP-> Props code works,
			we want the infinitive/gerund to be a child of the helper
			verb. 
		*/

		Symbol headSymbol = SpanishSemTreeUtils::primaryVerbSym(head);
		SemOPP *result  = _new SemOPP(0, synNode, predType, head, headSymbol);
		if (synNode->getNChildren() > 1){
			for (int i = 0; i < synNode->getNChildren(); i++) {
				const SynNode* child = synNode->getChild(i);
				if (child != head){
					Proposition::PredType childPredType;
					Symbol childTag = child->getTag();
					if (childTag == SpanishSTags::INFINITIU || childTag == SpanishSTags::GERUNDI){
						childPredType = Proposition::MODIFIER_PRED;
					}else if (isCopula(*child)) {
						childPredType = Proposition::COPULA_PRED;
					} else {
						childPredType = Proposition::VERB_PRED;
					}
					result = _new SemOPP(result, child, childPredType, child->getHead(), SpanishSemTreeUtils::primaryVerbSym(child));
				}
			}
		}
		
		return result;
	
	
	}else if (SpanishSemTreeUtils::isVerbPOS(synNodeTag)){
		// Ancora Spanish parse has sufficient extra nesting to need a raw verb handler
		const SynNode *head = synNode->getHead();
		Symbol nodeSym = SpanishSemTreeUtils::primaryVerbSym(synNode);
		Proposition::PredType predType;
		if (isCopula(*head)) {
			predType = Proposition::COPULA_PRED;
		} else {
			predType = Proposition::VERB_PRED;
		}

		SemOPP *result = _new SemOPP(analyzeChildren(synNode, mentionSet), synNode, predType, synNode);
		return result;

	} else if (SpanishSemTreeUtils::isVerbPhrase(synNodeTag)){

		Proposition::PredType predType;
		const SynNode *head = synNode->getHead();
		const SynNode *negative = 0;
		const SynNode *particle = 0;
		const SynNode *adverb = 0;
		Symbol synHeadTag = head->getTag();
		Symbol headSym = head->getHeadWord();

		SessionLogger::dbg("prop_gen") << "VerbPhrase create SemNode for:"
			<<  "  headTag=" <<synHeadTag.to_debug_string() << ":   headSym = " << headSym.to_debug_string() <<"  " << synNode->toString(4).c_str() << "\n";


		if (isVPList(*synNode)) { //synNodeTag == EnglishSTags::VP && isVPList(*synNode)) {
			// looking at conjoined list of predicates

			// try to find head, which is an English CC
			const SynNode *head = 0;
			for (int i = 0; i < synNode->getNChildren(); i++) {
				if (SpanishSemTreeUtils::isConjunctionPOS(synNode->getChild(i)->getTag())) { // EnglishSTags::CC) {
					head = synNode->getChild(i);
					synHeadTag = head->getTag();
					if (synHeadTag == SpanishSTags::CONJ){
						head = head->getChild(0); // prefer a "real" conj
						break;
					}else if (synHeadTag == SpanishSTags::CONJ_SUBORD){
						head = head->getChild(0); // but take what you can get
					}
				}
			}
			if (head != 0){ // really a vp List
				synHeadTag = head->getTag();
				headSym = head->getHeadWord();
				SessionLogger::dbg("prop_gen") << "VP List create SemNode for " << synNodeTag.to_debug_string() <<
				    "  headTag=" << synHeadTag.to_debug_string() << "  headSym = " << headSym.to_debug_string() <<"  " << synNode->toString(4).c_str() << "\n";

				SemOPP *result = _new SemOPP(0, synNode, Proposition::COMP_PRED,
					head);

				SemNode *lastPred = 0;
				for (int j = 0; j < synNode->getNChildren(); j++) {
					const SynNode *synChild = synNode->getChild(j);

					// analyze children, except when we see verbs, we treat
					// them as their own predicates
					SemNode *children = 0;
					if (SpanishSemTreeUtils::isVerbPOS(synChild->getTag())) {
						// eng was VB || VBZ || VBD || VBG || VBN
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
						}else if (lastPred != 0) {
							// attach child to predicate
							lastPred->appendChild(child);
						}else {
							// otherwise, just leave child on the COMP_PRED node
							result->appendChild(child);
						}
					} // end sibling iter
				} //end n-children loop

				return result;

			} //end really vp list test
			//end VP-list processing

		} // end if lookslike VP list test  -- fall through to base VP is not returned in "real VP list"

		//EnglishSTags::VP //NOT a VP list, perhaps by second test 

		// Basic verb phrase here; reset in case false VP list mucked things
		head = synNode->getHead();
		headSym = head->getHeadWord();
		synHeadTag = head->getTag();

		if (synHeadTag == SpanishSTags::GRUP_VERB){  //possibly-complex verb cluster
			headSym = SpanishSemTreeUtils::primaryVerbSym(head);

		}else if (synHeadTag == SpanishSTags::INFINITIU ||  //infinitive construct
			//synHeadTag == SpanishSTags::RELATIU ||
			synHeadTag == SpanishSTags::GERUNDI){	  // gerund construct 

				SemNode *semChildren = analyzeChildren(synNode, mentionSet);

				// if there's a negative here, move it to any opp we find
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
				
				//return _new SemBranch(semChildren, synNode);
				//return _new SemOPP(semChildren, synNode, Proposition::VERB_PRED, head);
				moveObjectsUnderVerbs(semChildren);
				return semChildren;
				//return _new SemBranch(_new SemOPP(semChildren, synNode, Proposition::VERB_PRED, head)
		}

		if (isCopula(*head)) {
			predType = Proposition::COPULA_PRED;
		} else {
			predType = Proposition::VERB_PRED;
		}


		// no linker word -- just a regular branch
		SemNode *semChildren = analyzeChildren(synNode, mentionSet);


		// find first negative adverb
		// last particle that is not negative adverb
		// first adverb
		for (int i = 0; i < synNode->getNChildren(); i++) {
			const SynNode *child = synNode->getChild(i);

			if (negative == 0 && isNegativeAdverb(*child)) {
				negative = child;
			} else if (adverb == 0 && SpanishSemTreeUtils::isAdverb(*child)) {
				adverb = child;
			}

			if (SpanishSemTreeUtils::isParticle(*child)) {
				particle = child;
			}
		}

		
		for (SemNode *semChild = semChildren; semChild; semChild = semChild->getNext()) {
			if (semChild->getSemNodeType() == SemNode::OPP_TYPE) {
				if (negative != 0){
					semChild->asOPP().addNegative(negative);
				}
				if (particle != 0){
					semChild->asOPP().addParticle(particle);
				}
				if (adverb != 0){
					semChild->asOPP().addAdverb(adverb);
				}
			}
		}
		

		SemBranch *result = _new SemBranch(semChildren,
			synNode);

		return result;
		// end non-list isVP code

	}else if(synNodeTag == SpanishSTags::S_A ||  //EnglishSTags::ADJP // S.A is noun complement adj phrase
		synNodeTag == SpanishSTags::SA ||  // SA is argumental adjectival phrase
		synNodeTag == SpanishSTags::GRUP_A || // adj group (composed of pos_aq or pos_ao)
		synNodeTag == SpanishSTags::POS_AQ ||  //EnglishSTags::JJ || adjective
		synNodeTag == SpanishSTags::POS_AO ||  //ordinal adj
		//synNodeTag == EnglishSTags::JJR || comparative adj // composed in Sp
		//synNodeTag == EnglishSTags::JJS || superlative adj  // composed in Sp
		SpanishSemTreeUtils::isGerundPOS(synNodeTag) || // EnglishSTags::VBG // gerund or present participle
		SpanishSemTreeUtils::isPastParticiplePOS(synNodeTag) || // EnglishSTags::VBN // past participle
		synNodeTag == SpanishSTags::POS_Z || //EnglishSTags::CD || cardinal number
		synNodeTag == SpanishSTags::POS_ZC || // currency
		synNodeTag == SpanishSTags::POS_NC || //EnglishSTags::NN // commnon noun (sing amd plural)
		//synNodeTag == EnglishSTags::NNS ||
		synNodeTag == SpanishSTags::POS_NP || //EnglishSTags::NNP // proper noun (sing and plural)
		//synNodeTag == EnglishSTags::NNPS ||
		synNodeTag ==  SpanishSTags::POS_PP   //EnglishSTags::PRP // personal pronoun
		) {
			// looking at (non-list) predicate -- create SemOPP

			// depending on the constit type, we set the predicate type, head
			// node, and for verbs, negative modifier
			Proposition::PredType predType;
			const SynNode *head;
			const SynNode *negative = 0;

			if (synNodeTag == SpanishSTags::S_A || //EnglishSTags::ADJP
				synNodeTag == SpanishSTags::SA ||
				synNodeTag == SpanishSTags::GRUP_A) { // adj grouping in Sp

					predType = Proposition::MODIFIER_PRED;
					head = synNode->getHead();
			} else if (synNodeTag == SpanishSTags::POS_AQ ||  //EnglishSTags::JJ // adjective
				synNodeTag == SpanishSTags::POS_AO ||  //ordinal adj 
				//synNodeTag == EnglishSTags::JJR ||
				//synNodeTag == EnglishSTags::JJS ||
				synNodeTag == SpanishSTags::POS_Z || //EnglishSTags::CD // cardinal number
				synNodeTag == SpanishSTags::POS_ZC || // currency
				SpanishSemTreeUtils::isInfinitivePOS(synNodeTag) || // used like participle in Eng
				SpanishSemTreeUtils::isGerundPOS(synNodeTag) || // EnglishSTags::VBG // gerund or present participle
				SpanishSemTreeUtils::isPastParticiplePOS(synNodeTag)){ // EnglishSTags::VBN // past participle
					predType = Proposition::MODIFIER_PRED;
					head = synNode;
			} else if (synNodeTag == SpanishSTags::POS_PP) {   //EnglishSTags::PRP // personal pronoun
				// XXX
				return 0;
			} else { // only types left are noun types NN[P][S] 
				predType = Proposition::NOUN_PRED;
				head = synNode;
			}

			// If we are a modifier with no negative, check our previous token to see if it is a negative
			if (predType == Proposition::MODIFIER_PRED && negative == 0) {
				const SynNode* prevTerminal = synNode->getPrevTerminal();
				if (prevTerminal) {
					Symbol prevSymbol = prevTerminal->getTag();
					if (SpanishSemTreeUtils::isNegativeAdverb(prevSymbol)) {
						negative = prevTerminal->getParent();
					}
				}
			}

			SemOPP *result = _new SemOPP(analyzeChildren(synNode, mentionSet),
				synNode, predType, head);
			result->addNegative(negative);

			return result;

	} else if (synNodeTag == SpanishSTags::SP){ //EnglishSTags::PP prepositional phrase
		// first see if this is actually 2 or more conjoined PPs
		int n_pp_children = 0;
		for (int i = 0; i < synNode->getNChildren(); i++) {
			if (synNode->getChild(i)->getTag() == SpanishSTags::SP) //EnglishSTags::PP)
				n_pp_children++;
		}
		if (n_pp_children >= 2)
			return analyzeChildren(synNode, mentionSet);

		// looking at a non-compound PP -- create SemLink

		const SynNode *head = synNode->getHead(); 
		// XXX Also, it won't be null when it's supposed to

		SemNode *children = analyzeChildren(synNode, mentionSet);

		// we also want to see if there's a nested PP
		if (children &&
			children->getNext() == 0 &&
			children->getSemNodeType() == SemNode::LINK_TYPE)
		{
			// there's a nested link, so merge the two
			if (children->asLink().getSymbol() == Argument::TEMP_ROLE) {
				// Temporals don't get merged -- instead, create
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
				if (inner.getSymbol().is_null()){
					inner.setSymbol(head->getHeadWord());
				}else{
					Symbol mergedSym = head->getHeadWord() + inner.getSymbol();
					inner.setSymbol(mergedSym);
				}

				return &inner;
			}
		}
		else {  //vanilla prep phrase
			// head should be tagged PREP
			Symbol head_prep_sym;
			if (head->getNChildren() > 1){
				head_prep_sym = SpanishSemTreeUtils::composeChildSymbols(head, SpanishSTags::POS_S);
			}else{
				head_prep_sym = head->getHeadWord();
			}
			return _new SemLink(children, synNode, head_prep_sym, head);
		}
	} else if (synNodeTag == SpanishSTags::GRUP_ADV) { //EnglishSTags::ADVP
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
		////////// Spanish uses "de Jose" instead of "Jose's"
		//} else if (synNodeTag == EnglishSTags::NPPOS) {
		//	// we have a possessive, so make a special link
		//
		//	return _new SemLink(analyzeChildren(synNode, mentionSet),
		//						synNode, Argument::POSS_ROLE, synNode);
		//} else if (synNodeTag == EnglishSTags::WHNP) {
		// The propfinder doesn't (yet) know how to correctly descend
		// WH-phrases, so we avoid trouble by leaving them out. (This was
		// also the behavior of the old java propfinder.)
		//	return 0;
	}else if (synNodeTag == SpanishSTags::SPEC ){ // determiner of sorts, sometimes complex
		const SynNode * spec_head = synNode->getHead();
		Symbol synHeadTag = spec_head->getTag();
		if (synHeadTag == SpanishSTags::POS_DA ||  //"la", "el"
			synHeadTag == SpanishSTags::POS_DI){ // "una", "unos"
				// we're dropping the determiner and going home unless more complex
				return analyzeOtherChildren(synNode, spec_head, mentionSet);
		}else {
			//SessionLogger::warn("") << "SpanishSemTreeBuilder::buildSemTree::SPEC(): Could not create SemNode for:\n"
			//<< "    tag=" <<synNodeTag.to_debug_string() << "  " << synNode->toString(4).c_str() << "\n";
			return analyzeChildren(synNode, mentionSet);
		}
		// don't worry with punctuation
	} else if (SpanishSemTreeUtils::isPunctuationPOS(synNodeTag)||
		synNodeTag == SpanishSTags::S ||  //raw prep handled under PrepPhrase
		synNodeTag == SpanishSTags::S_DP_SBJ || 
		synNodeTag == SpanishSTags::PREP){ // complex of prepositions handled under PrepPhrase
			return 0;
	} else {
		SessionLogger::dbg("prop_gen") << "SpanishSemTreeBuilder::buildSemTree(): Could not create SemNode for:\n"
			<< "    tag=" <<synNodeTag.to_debug_string() << "  " << synNode->toString(4).c_str() << "\n";
		return analyzeChildren(synNode, mentionSet);
	}
}

SemNode *SpanishSemTreeBuilder::analyzeChildren(const SynNode *synNode, const MentionSet *mentionSet) {
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

SemNode *SpanishSemTreeBuilder::analyzeOtherChildren(const SynNode *synNode,
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

void SpanishSemTreeBuilder::moveObjectsUnderVerbs(SemNode* semChildren)
{
	SemNode *childNode = semChildren;
	SemNode *verbNode = 0;
	SemNode *lastVerbSibling = 0;
	while (childNode != 0) {
		if (!verbNode && childNode->getSemNodeType() == SemNode::OPP_TYPE &&
			(childNode->asOPP().getPredicateType() == Proposition::VERB_PRED ||
			childNode->asOPP().getPredicateType() == Proposition::COPULA_PRED)){
			verbNode = childNode;
			childNode = childNode->getNext();
			//Change the link to the next
			lastVerbSibling = verbNode;
			lastVerbSibling->setNext(0);
			
			for (SemNode *child = verbNode->getFirstChild(); child; child = child->getNext()) {
				if (child->getSemNodeType() == SemNode::OPP_TYPE) {
					// require the other opp to be a verb or compound pred
					if (child->asOPP().getPredicateType() == Proposition::VERB_PRED ||
						child->asOPP().getPredicateType() == Proposition::COMP_PRED)
					{
						verbNode = child;
						break;
					}

					// alternatively, the other opp can be a copula, if this one
					// is headed by an appropriate auxillary
					if (child->asOPP().getPredicateType() ==
								Proposition::COPULA_PRED &&
								canBeAuxillaryVerb(*(verbNode->asOPP().getHead())))
					{
						verbNode = child;
						break;
					}
				}
			}
		}else{
			if (verbNode){
				//Force noun phrases & completive clauses after the verb to be children of
				// the verb; otherwise, make them siblings
				if (childNode->getSynNode()->getTag() == SpanishSTags::SN 
					|| childNode->getSynNode()->getTag() == SpanishSTags::S_F_C 
					|| childNode->getSynNode()->getTag() == SpanishSTags::S_F_C_DP_SBJ){
					SemNode *childToAppend = childNode;
					childNode = childNode->getNext();
					verbNode->appendChild(childToAppend);
				}else{
					lastVerbSibling->setNext(childNode);
					lastVerbSibling = childNode;
					childNode = childNode->getNext();
					lastVerbSibling->setNext(0);
				}
			}else{
				childNode = childNode->getNext();
			}
		}
	}
}


const SynNode* SpanishSemTreeBuilder::findAmongChildren(const SynNode *node,
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


bool SpanishSemTreeBuilder::isPassiveVerb(const SynNode &verb) {
    // sanity check -- we need to look at parent
	if (verb.getParent() == 0) {
		return false;
	}

	// find topmost VP in verb cluster
	const SynNode *top_vp = &verb;
	while (top_vp->getParent() != 0 && SpanishSemTreeUtils::isVerbPhrase(top_vp->getParent()->getTag())){ //EnglishSTags::VP
		top_vp = top_vp->getParent();
	}

	// figure out if we're in a reduced relative clause
	if (top_vp->getParent() != 0 &&
		top_vp->getParent()->getTag() == SpanishSTags::GRUP_NOM) { //EnglishSTags::NP
		// in reduced relative, just see if verb is VBN
		if (SpanishSemTreeUtils::isPastParticiplePOS(verb.getTag())){ //  EnglishSTags::VBN
			return true;
		} else {
			return false;
		}
	} else {
		// not in reduced relative, so look for copula and VBN

		if (!SpanishSemTreeUtils::isPastParticiplePOS(verb.getTag())) { //verb.getTag() != EnglishSTags::VBN) {
			return false;
		}

		// if we find copula one level up in the parse, we're done
		const SynNode *gparent = verb.getParent()->getParent();
		if (gparent == 0) {
			return false;
		}
		if (SpanishSemTreeUtils::isVerbPhrase(gparent->getTag()) && // EnglishSTags::VP 
			 isCopula(*gparent->getChild(0))) {
			return true;
		}

		// for conjoined VPs, the copula may be 2 levels up:
		const SynNode *ggparent = gparent->getParent();
		if (ggparent == 0) {
			return false;
		}
		if (SpanishSemTreeUtils::isVerbPhrase(gparent->getTag()) &&  //EnglishSTags::VP 
			SpanishSemTreeUtils::isVerbPhrase(ggparent->getTag()) &&  //EnglishSTags::VP  
			isCopula(*ggparent->getChild(0))) {
			return true;
		}

		// no copula found
		return false;
    }
}

bool SpanishSemTreeBuilder::isKnownTransitiveVerb(const SynNode &verb) {
	Symbol head = verb.getHeadWord();
	return SpanishSemTreeUtils::isKnownTransitivePastParticiple(head);
}

bool SpanishSemTreeBuilder::isLocativePronoun(const SynNode &synNode) {
	const SynNode *head = &synNode;
	while (!head->isPreterminal()) {
		if (head->getNChildren() > 1)
			return false;
		head = head->getHead();
	}

	Symbol headWord = head->getHeadWord();
	return SpanishSemTreeUtils::isLocativeNominal(headWord); 
}

bool SpanishSemTreeBuilder::isVPList(const SynNode &node) {
	int vps_seen = 0;
	int separators_seen = 0;
	for (int i = 0; i < node.getNChildren(); i++) {
		Symbol tag = node.getChild(i)->getTag();
		if (SpanishSemTreeUtils::isVerbPhrase(tag) || // EnglishSTags::VP 
			tag == SpanishSTags::S_F_C_STAR ||  // reduced verb phrase of sorts
			tag == SpanishSTags::GRUP_VERB ||
			tag.isInSymbolGroup(SpanishSTags::POS_V_GROUP))
		{
			vps_seen++;
		}
		else if (tag == SpanishSTags::COMMA || // english comma
				 tag == SpanishSTags::POS_CC || //EnglishSTags::CC 
				 tag == SpanishSTags::CONJ) //EnglishSTags::CONJP
		{
			separators_seen++;
		}
	}
	if (vps_seen > 1 && separators_seen > 0)
		return true;
	else
		return false;
}

const SynNode *SpanishSemTreeBuilder::getStateOfCity(const SynNode *synNode,
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
	if (child->getTag() != SpanishSTags::SNP) //EnglishSTags::NPP)
		return 0;
	childMention = mentionSet->getMentionByNode(child);
	if (childMention->getParent() != mention)
		return 0;
	child_no++;

	// for the next child, ensure that it's a comma
	if (child_no == synNode->getNChildren())
		return 0;
	// JCS: allow PRN to intervene - "Ho Chi Minh City (Saigon), Vietnam"
	//if (synNode->getChild(child_no)->getTag() == EnglishSTags::PRN) {
	//	child_no++;
	//	if (child_no == synNode->getNChildren())
	//		return 0;
	//}
	if (synNode->getChild(child_no)->getTag() != SpanishSTags::COMMA)
		return 0;
	child_no++;

	// for the next child, ensure that it's a GPE name
	if (child_no == synNode->getNChildren())
		return 0;
	child = synNode->getChild(child_no);
	if (!child->hasMention())
		return 0;
	if (child->getTag() != SpanishSTags::SNP) //EnglishSTags::NPP)
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
/* no Spanish equiv (yet)
SemNode *SpanishSemTreeBuilder::processRidOf(const SynNode *synNode,
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
*/

// See if we have a "such-and-such -based" or "such-and-such -led" among
// the children of an NP
SemNode *SpanishSemTreeBuilder::applyBasedHeuristic(SemNode *children) {
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
void SpanishSemTreeBuilder::applyNameNounNameHeuristic(SemMention &mentionNode,
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


