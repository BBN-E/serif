// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <boost/algorithm/string.hpp>

#include "Generic/common/OutputUtil.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/WordConstants.h"
#include "Generic/parse/STags.h"
#include "Generic/propositions/sem_tree/SemBranch.h"
#include "Generic/propositions/sem_tree/SemOPP.h"
#include "Generic/propositions/sem_tree/SemLink.h"
#include "Generic/propositions/sem_tree/SemMention.h"
#include "Generic/propositions/sem_tree/SemReference.h"
#include "Generic/propositions/sem_tree/SemTrace.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

// moved this to WordConstants for language independence
//std::set<Symbol> SemTrace::_temporalWords;
//Symbol SemTrace::OF_SYMBOL = Symbol(L"of");
//Symbol SemTrace::FOR_SYMBOL = Symbol(L"for");
Symbol SemTrace::OTH_SYMBOL = Symbol(L"OTH");
Symbol SemTrace::PP_TAG = STags::getPP();
Symbol SemTrace::DATE_TAG = STags::getDATE();
Symbol SemTrace::COMMA_TAG = STags::getCOMMA();

void SemTrace::setSource(SemReference *source) {
    _source = source;
	if (source == 0) {
		_synNode = 0;
	} else {
		setSynNode(source->getSynNode());
	}
}

bool SemTrace::resolve(SemReference *awaiting_ref) {
	SemNode* walker = getParent();
	SemNode* originalWalker = walker;
	SemReference *result = 0;

	// System.out.println("Trying to resolve trace in: " + getParent()._parse_node.toString());

	if (walker == 0) {
		result = 0;
	} else if (walker->getSemNodeType() == BRANCH_TYPE) {
		// System.out.print("-> B ");
		walker = walker->getParent();
		if (walker == 0) {
			result = 0;
		} else if (walker->isReference()) {
			// System.out.print("-> R -- found: " + walker.toString());
			result = static_cast<SemReference *>(walker);
		} else if (walker->getSemNodeType() == OPP_TYPE) {
			//        System.out.print("-> O ");
			SemOPP *opp = static_cast<SemOPP *>(walker);
			SemNode *arg = opp->getArg2();
			if (arg != 0 && arg->isReference()) {
				// System.out.print("<- R -- found: " + arg.toString());
				result = static_cast<SemReference *>(arg);
			} else {
				arg = opp->getArg1();
				if (arg != 0 && arg->isReference()) {
					// System.out.print("<- R -- found: " + arg.toString());
					result = static_cast<SemReference *>(arg);
				}
			}
			
			// Check if our reference is followed by a comma.
			// Consider:
			// He threw the football winning the game.
			// He threw the football bearing his name.
			// He threw the football, winning the game.
			// He threw the football, bearing his name.
			if (result) {
				const SynNode* node = result->getSynNode();
				node = node->getNextSibling();
				int commaIndex = -1;
				while (node) {
					Symbol tag_sym = node->getTag();
					std::wstring text = node->toTextString();
					boost::trim(text);

					// He threw the football [on] Friday, winning the game.
					if ( tag_sym == PP_TAG || tag_sym == DATE_TAG || containsDailyTemporalExpression(text) ){ 
						node = node->getNextSibling();
					} else if (tag_sym == COMMA_TAG) {					
						commaIndex = node->getStartToken();
						break;
					} else {
						break;
					}
				}
				int verbIndex; // We only care about commas that come before the verb
				if (originalWalker->getSemNodeType() == SemNode::BRANCH_TYPE && originalWalker->asBranch().getOPP()) {
					verbIndex = originalWalker->asBranch().getOPP()->getSynNode()->getEndToken();
				} else {
					verbIndex = originalWalker->getSynNode()->getEndToken();
				}
				if (commaIndex >= 0 && commaIndex <= verbIndex) {
					result = 0;  // Bad candidate, set to zero so we back off up the tree.
				}
			}

			if (result == 0) {
				walker = walker->getParent();
				if (walker == 0) {
					result = 0;
				} else if (walker->isReference()) {
					// System.out.print("-> R -- found: " + walker.toString());
					result = static_cast<SemReference *>(walker);
				} else if (walker->getSemNodeType() == BRANCH_TYPE) {
					// System.out.print("-> B ");
					SemNode *ref = walker->asBranch().getReference();
					if (ref != 0 && ref->isReference()) {
						// System.out.print("<- R -- found: " + ref.toString());
						result = static_cast<SemReference *>(ref);
					}
				}
			}
		} else if (walker->getSemNodeType() == BRANCH_TYPE) {
			// System.out.print("-> B ");
			SemNode *ref = walker->asBranch().getReference();
			if (ref != 0 && ref->isReference()) {
				// System.out.print("<- R -- found: " + ref.toString());
				result = static_cast<SemReference *>(ref);
			}
		} else if (walker->getSemNodeType() == LINK_TYPE) {
				
			// Check if we are in a for/of link
			Symbol p_link = walker->asLink().getSymbol();
			bool inside_for_link = WordConstants::isForReasonPreposition(p_link);
			bool inside_of_link = WordConstants::isOfActionPreposition(p_link);

			// System.out.print("-> L ");
			walker = walker->getParent();
			if (walker == 0) {
				result = 0;
			} else if (walker->getSemNodeType() == OPP_TYPE) {
				
				// Handle being a noun OPP parent of a misparsed "for" link.
				//     Dick got (a death sentence for killing Jane).
				if (walker->asOPP().getPredicateType() == Proposition::NOUN_PRED && inside_for_link) {					
					SemNode* parent = walker->getParent();
					if (parent->getSemNodeType() == MENTION_TYPE) {
						SemNode* parentParent = parent->getParent();
						if (parentParent->getSemNodeType() == OPP_TYPE && parentParent->asOPP().getPredicateType() == Proposition::VERB_PRED) {
							SemNode* sub = parentParent->asOPP().findSyntacticSubject();
							if (sub && sub->isReference()) {
								result = &(sub->asReference());
							}
						}
					}
				}

				// Handle being a verb OPP parent of an "of" link.  The object of the outer verb is the subject of the inner verb.
				//     Bob accused Dick of killing Jane.
				if (walker->asOPP().getPredicateType() == Proposition::VERB_PRED && inside_of_link && 
					walker->asOPP().getArg1() && walker->asOPP().getArg1()->isReference()) {
					result = &(walker->asOPP().getArg1()->asReference());
				} 

				// Handle being a verb OPP parent of a "for" link.
				if (walker->asOPP().getPredicateType() == Proposition::VERB_PRED && inside_for_link &&
					walker->asOPP().getArg1() && walker->asOPP().getArg1()->isReference()) { 

					// If the object of the outer verb is an ACE type, the object of the outer verb is the subject of the inner verb.
					//     Bob arrested Dick for killing Jane.
					if (walker->asOPP().getArg1()->asReference().getMention() &&
						walker->asOPP().getArg1()->asReference().getMention()->getEntityType().getName() != OTH_SYMBOL) {
						result = &(walker->asOPP().getArg1()->asReference());
					}

					// If the object of the outer verb is not an ACE type, the subject of the outer verb is the subject of the inner verb.
					//     Dick got (a death sentence) (for killing Jane).
					// Do nothing - the default backoff behavior below handles this.
				}

				// Back off to assuming our parent's reference is our subject.
				if (result == 0) {
				
					// System.out.print("-> O ");
					walker = walker->getParent();
					if (walker == 0) {
						result = 0;
					} else if (walker->getSemNodeType() == BRANCH_TYPE) {
						// System.out.print("-> B ");
						SemNode *ref = walker->asBranch().getReference();
						if (ref != 0 && ref->isReference()) {
							// System.out.print("<- R -- found: " + ref.toString());
							result = static_cast<SemReference *>(ref);
						}
					}
				}
			} else if (walker->getSemNodeType() == BRANCH_TYPE) {
				// System.out.print("-> B ");
				SemNode *ref = walker->asBranch().getReference();
				if (ref != 0 && ref->isReference()) {
					// System.out.print("<- R -- found: " + ref.toString());
					result = static_cast<SemReference *>(ref);
				}
			}
		}
	}

	// System.out.println();

	if (awaiting_ref != 0) {
		if (result == 0) {
			result = awaiting_ref;
		}

		_source = result;

		if (result == awaiting_ref) {
			return true;
		}
	}

	_source = result;

	return false;
}


const Mention *SemTrace::getMention() const {
	if (_source == 0) {
		return 0;
	} else {
		return _source->getMention();
	}
}


void SemTrace::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	if (_tangential) {
		out << "(Trace) {";
	} else {
		out << "Trace {";
	}

	const Mention *mention = getMention();
	if (mention == 0) {
		out << newline << "  - entity mention: <unresolved>";
	} else {
		out << newline << "  - entity mention: " << *mention;
	}

	out << "}";

	delete[] newline;
}

bool SemTrace::containsDailyTemporalExpression(std::wstring text) {
	std::vector<std::wstring> words;
	boost::algorithm::split(words, text, boost::is_any_of(L" "));
	for(std::vector<std::wstring>::iterator it=words.begin();it!=words.end();++it){
		if (WordConstants::isDailyTemporalExpression(Symbol(*it))){
			return true;
		}
	}
	return false;
}
