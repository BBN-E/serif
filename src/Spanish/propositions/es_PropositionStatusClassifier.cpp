// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

// This code originated from en_RelationUtilities; it then moved to
// Brandy/distill_generic/patterns/DocumentInfo.cpp, where it was 
// expanded in various ways.  A simple mapping to SPanish followed.  It's possible 
// that the code in en_RelationUtilities could be simplified and/or
// eliminated by just checking Proposition::getStatus(), but we would
// need to go through and figure out whether any of the changes that
// were made when the code lived in Brandy would cause problems before
// we made such a simplification.

#include "Generic/common/leak_detection.h"

#include "Spanish/propositions/es_PropositionStatusClassifier.h"

#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

#include "English/parse/en_STags.h"   // remnove when possible
#include "Spanish/parse/es_STags.h"
#include "Generic/wordnet/xx_WordNet.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnrecoverableException.h"
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include "Generic/patterns/PatternSet.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/TopLevelPFeature.h"
#include "Generic/patterns/features/ReturnPFeature.h"

namespace {
	const bool DEBUG_PATTERNS = true;
}

SpanishPropositionStatusClassifier::SpanishPropositionStatusClassifier() : _debugPatterns(false)
{
	_debugPatterns = ParamReader::isParamTrue("debug_proposition_status_patterns");
	// Load proposition status patterns.
	std::string propStatusPatternFile = ParamReader::getParam("proposition_status_patterns");
	if (!propStatusPatternFile.empty())
		_propStatusPatterns = boost::make_shared<PatternSet>(propStatusPatternFile.c_str());
}


namespace { // Private symbols
	Symbol justSym(L"just");
	Symbol allegedlySym(L"allegedly");
	Symbol shouldSym(L"should");
	Symbol couldSym(L"could");
	Symbol mightSym(L"might");
	Symbol maySym(L"may");
	Symbol mustSym(L"must");
	Symbol canSym(L"can");
	Symbol wouldSym(L"would");
	Symbol willSym(L"will");
	Symbol shallSym(L"shall");
	Symbol ifSym(L"if");
	Symbol whetherSym(L"whether");
	Symbol onWhetherSym(L"on_whether");
	Symbol thatSym(L"that");
	Symbol neitherSym(L"neither");
	Symbol aposSym(L"'s");
	Symbol nooneSym(L"noone");
	Symbol anyoneSym(L"anyone");
	Symbol nobodySym(L"nobody");
	Symbol anybodySym(L"anybody");
	Symbol noSym(L"no");
	Symbol anySym(L"any");
	Symbol OTH_SYMBOL(L"OTH");
	Symbol nearlySym(L"nearly");
	Symbol potentiallySym(L"potentially");
	Symbol virtuallySym(L"virtually");
	Symbol probablySym(L"probably");
	Symbol possiblySym(L"possibly");
	Symbol practicallySym(L"practically");
	Symbol almostSym(L"almost");
	Symbol seeSym(L"see");
	Symbol forSym(L"for");
	Symbol fromSym(L"from");
	Symbol howSym(L"how");
	Symbol on_howSym(L"on_how");
	Symbol ofSym(L"of");
	Symbol toSym(L"to");
	Symbol UNLABELED_PATTERN_SYM(L"unlabeled-pattern");
}

void SpanishPropositionStatusClassifier::augmentPropositionTheory(DocTheory *docTheory)
{
	PatternMatcher_ptr patternMatcher;
	
	if (_propStatusPatterns)
		patternMatcher = PatternMatcher::makePatternMatcher(docTheory, _propStatusPatterns);
	for (int i=0; i<docTheory->getNSentences(); i++) {
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(i);
		augmentPropositionTheoryWithPatterns(docTheory, sentTheory, patternMatcher);
		augmentPropositionTheoryExtras(docTheory, sentTheory);
		propagatePropStatuses(docTheory, sentTheory);
	}
}

/** Use the given pattern matcher to label the status of propositions.  In particular, run each 
  * sentence-level pattern, and any proposition return feature that returns a status name will
  * cause that proposition to get tagged with that status. */
void SpanishPropositionStatusClassifier::augmentPropositionTheoryWithPatterns(DocTheory *docTheory, SentenceTheory *sentTheory,
																  PatternMatcher_ptr propStatusPatternMatcher)
{
	// Use the pattern matcher to look for propositions that should be labeled with select statuses.
	if (!propStatusPatternMatcher)
		return;
	std::vector<PatternFeatureSet_ptr> sentMatches = propStatusPatternMatcher->getSentenceSnippets(sentTheory, 0, true);
	BOOST_FOREACH(const PatternFeatureSet_ptr &match, sentMatches) {
		Symbol patternLabel = UNLABELED_PATTERN_SYM;
		for (size_t f = 0; f < match->getNFeatures(); f++) {
			if (TopLevelPFeature_ptr tsf = boost::dynamic_pointer_cast<TopLevelPFeature>(match->getFeature(f)))
				patternLabel = tsf->getPatternLabel();
		}
		for (size_t f = 0; f < match->getNFeatures(); f++) {
			try {
				if (PropositionReturnPFeature_ptr prpf = boost::dynamic_pointer_cast<PropositionReturnPFeature>(match->getFeature(f))) {
					PropositionStatusAttribute status = PropositionStatusAttribute::getFromString(prpf->getReturnLabel().to_string());
					Proposition *prop = const_cast<Proposition*>(prpf->getProp());
					if (_debugPatterns)
						showPatternDebugMessage(patternLabel, prpf->getReturnLabel(), prop);
					prop->addStatus(status);
				}
				if (ParseNodeReturnPFeature_ptr prpf = boost::dynamic_pointer_cast<ParseNodeReturnPFeature>(match->getFeature(f))) {
					PropositionStatusAttribute status = PropositionStatusAttribute::getFromString(prpf->getReturnLabel().to_string());
					const SynNode *node = const_cast<SynNode*>(prpf->getNode());
					Proposition *prop = sentTheory->getPropositionSet()->findPropositionByNode(node);
					if (prop) {
						if (_debugPatterns)
							showPatternDebugMessage(patternLabel, prpf->getReturnLabel(), prop);
						prop->addStatus(status);
					}
				}
			} catch (UnrecoverableException &e) {
				e.prependToMessage(patternLabel.to_debug_string());
				throw;
			}
		}
	}
}

void SpanishPropositionStatusClassifier::showPatternDebugMessage(Symbol patternLabel, Symbol status, Proposition *prop) {
	std::cerr << "  [" << patternLabel << "] added status \"" << status << "\" to:\n    ";
	prop->dump(std::cerr);
	std::cerr << std::endl;
}


/** Add proposition status labels that can't (easily) be added using patterns.
*/
void SpanishPropositionStatusClassifier::augmentPropositionTheoryExtras(DocTheory *docTheory, SentenceTheory *sentTheory)
{
	PropositionSet *propSet = sentTheory->getPropositionSet();

	// typical counter-example:
	// "if he had not worked for IBM, he wouldn't have paid her"
	// can we do anything with this?
	for (int k = 0; k < propSet->getNPropositions(); k++) {
		Proposition *prop = propSet->getProposition(k);

		// get "if..." unrepresented in propositions
		if (prop->getPredType() == Proposition::VERB_PRED) {			
			const SynNode* node = prop->getPredHead();
			while (node != 0) {
				if (node->getTag() != EnglishSTags::VP &&
					node->getTag() != EnglishSTags::S &&
					node->getTag() != EnglishSTags::SBAR &&
					!node->isPreterminal()) {
					break;
				}
				if (node->getTag() == EnglishSTags::SBAR &&
					(node->getHeadWord() == ifSym ||
					(node->getHeadIndex() == 0 &&
					node->getNChildren() > 1 &&
					node->getChild(1)->getTag() == EnglishSTags::IN &&
					node->getChild(1)->getHeadWord() == ifSym))) 
				{
					prop->addStatus(PropositionStatus::IF);
					break;
				}
				node = node->getParent();
			}
		}
	}
}


void SpanishPropositionStatusClassifier::propagatePropStatuses(DocTheory *docTheory, SentenceTheory *sentTheory) {
	PropositionSet *propSet = sentTheory->getPropositionSet();
	const MentionSet *mentionSet = sentTheory->getMentionSet();

	// EMB NEW: propagate badness down from compound predicate propositions
	// Examples: "if he shot and killed the man..." / "he could run or jump"
	for (int p = 0; p < propSet->getNPropositions(); p++) {
		Proposition *prop = propSet->getProposition(p);
		if (prop->getPredType() == Proposition::COMP_PRED) {
			for (int k = 0; k < prop->getNArgs(); k++) {
				if (prop->getArg(k)->getType() == Argument::PROPOSITION_ARG && 
					prop->getArg(k)->getRoleSym() == Argument::MEMBER_ROLE) {
					Proposition* argProp = const_cast<Proposition*>(prop->getArg(k)->getProposition());
					argProp->addStatuses(prop);
				}
			}
		}
	}

	// EMB COMMENTS:
	// Previously, we did no full propagation of bad props. It was actually almost non-deterministic,
	//   because the order of the propositions is more or less random, so the loop would sometimes propagate
	//   down and sometimes not. Here we control for that by using a separate array for each iteration.
	
	// Examples of deeper propagation:
	//   if he had been convicted of conspiring to carry out a terrorist attack in Australia
	//   what Pakistan should be doing to help fight the Taliban
	//   he denied using a MiG fighter jet to shoot down a spy plane
	// propagate propositions N levels deep
	for (int iteration = 0; iteration < 3; iteration++) {
		std::vector< std::set<PropositionStatusAttribute> > iterativeBadness(propSet->getNPropositions());
	
		for (int p = 0; p < propSet->getNPropositions(); p++) {
			Proposition *prop = propSet->getProposition(p);
			// Propagate from a bad verb prop to its verb (and currently not its mention) arguments
			if (prop->hasAnyStatus()) {
				iterativeBadness[prop->getIndex()] = prop->getStatuses();
				if (prop->getPredType() == Proposition::VERB_PRED) {
					for (int j = 0; j < prop->getNArgs(); j++) {	
						Argument *arg = prop->getArg(j);
						if (//arg->getRoleSym() == Argument::SUB_ROLE || 
							arg->getRoleSym() == Argument::OBJ_ROLE || 
							arg->getRoleSym() == Argument::IOBJ_ROLE ||
							arg->getRoleSym() == forSym ||
							arg->getRoleSym() == fromSym ||	
							arg->getRoleSym() == howSym ||	
							arg->getRoleSym() == on_howSym ||	
							arg->getRoleSym() == ofSym ||
							arg->getRoleSym() == thatSym ||
							arg->getRoleSym() == toSym) {
							if (arg->getType() == Argument::PROPOSITION_ARG) {
								iterativeBadness[arg->getProposition()->getIndex()] = prop->getStatuses();
							} /*else if (arg->getType() == Argument::MENTION_ARG) {
	
								// We only do this for non-ACE types
								if (arg->getMention(mentionSet)->getEntityType().getName() == OTH_SYMBOL) {
									const Proposition *defProp = propSet->getDefinition(arg->getMentionIndex());
									if (defProp != 0) {
										iterativeBadness[defProp->getIndex()] = isBad[prop->getIndex()];
									}
								}
							}*/
						}
					}
				}
			}
		}
		for (int p = 0; p < propSet->getNPropositions(); p++) {
			Proposition *prop = propSet->getProposition(p);
			if (iterativeBadness[prop->getIndex()].size() != 0 && prop->getPredType() == Proposition::COMP_PRED) {
				for (int k = 0; k < prop->getNArgs(); k++) {
					if (prop->getArg(k)->getType() == Argument::PROPOSITION_ARG && 
						prop->getArg(k)->getRoleSym() == Argument::MEMBER_ROLE) {
						iterativeBadness[prop->getArg(k)->getProposition()->getIndex()] = iterativeBadness[prop->getIndex()];
					}
				}
			}
		}
		for (int p = 0; p < propSet->getNPropositions(); p++) {
			Proposition *prop = propSet->getProposition(p);
			BOOST_FOREACH(PropositionStatusAttribute status, iterativeBadness[prop->getIndex()]) {
				if (status == PropositionStatus::NEGATIVE && !prop->hasStatus(PropositionStatus::NEGATIVE))
					prop->addStatus(PropositionStatus::UNRELIABLE);
				else prop->addStatus(status);
			}
		}
	}

	// non-propagating bad props
	for (int m = 0; m < propSet->getNPropositions(); m++) {
		Proposition *prop = propSet->getProposition(m);
		for (int j = 0; j < prop->getNArgs(); j++) {
			Argument *arg = prop->getArg(j);
			if (arg->getType() == Argument::MENTION_ARG &&
				(arg->getRoleSym() == Argument::OBJ_ROLE ||
				arg->getRoleSym() == Argument::SUB_ROLE ||
				arg->getRoleSym() == Argument::REF_ROLE ||
				arg->getRoleSym() == Argument::IOBJ_ROLE)) {
				const SynNode *node = arg->getMention(mentionSet)->getNode();
				if (node->getHeadWord() == nooneSym ||
					node->getHeadWord() == nobodySym ||
					node->getHeadWord() == aposSym) {					
					prop->addStatus(PropositionStatus::NEGATIVE);	
				} else if (node->getChild(0)->getHeadWord() == neitherSym || 
						   node->getChild(0)->getHeadWord() == noSym) {
					prop->addStatus(PropositionStatus::NEGATIVE);
				} else if (node->getChild(0)->getHeadWord() == anySym) {
					prop->addStatus(PropositionStatus::UNRELIABLE);
				}
			}
		}
	}
}
