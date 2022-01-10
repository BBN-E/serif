// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/patterns/PropPattern.h"
#include "Generic/patterns/RegexPattern.h"
#include "Generic/patterns/ShortcutPattern.h"
#include "Generic/patterns/ArgumentPattern.h"
#include "Generic/patterns/CombinationPattern.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/features/PropPFeature.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/ParamReader.h"
#include <boost/foreach.hpp>
#include "Generic/common/Sexp.h"
#include "Generic/wordnet/xx_WordNet.h"

// Private symbols
namespace {
	Symbol nounPropSym(L"nprop");
	Symbol verbPropSym(L"vprop");
	Symbol modifierPropSym(L"mprop");
	Symbol setPropSym(L"sprop");
	Symbol compoundPropSym(L"cprop");
	Symbol anyPropSym(L"anyprop");
	Symbol blockSym(L"block_args");
	Symbol argsSym(L"args");
	Symbol optArgSym(L"opt_args");
	Symbol adjectiveSym(L"adj");
	Symbol blockAdjectiveSym(L"block_adj");
	Symbol adverbParticleSym(L"adverb_or_particle");
	Symbol blockAdverbParticleSym(L"block_adv_part");
	Symbol predicateSym(L"predicate");
	Symbol alignedPredicateSym(L"aligned_predicate");
	Symbol blockPredicateSym(L"block_predicate");
	Symbol predTypeSym(L"predicate_type");
	Symbol definiteSym(L"DEFINITE");
	Symbol negativeSym(L"NEGATIVE");
	Symbol matchAllArgsSym(L"MATCH_ALL_ARGS");
	Symbol stemPredicateSym(L"STEM_PREDICATE");
	Symbol blockFTSym(L"BLOCK_FALL_THROUGH");
	Symbol particleSym(L"particle");
	Symbol propModifierSym(L"propmod");
	Symbol regexSym(L"regex");
	Symbol oneToOneSym(L"ONE_TO_ONE");
	Symbol manyToManySym(L"MANY_TO_MANY");
	Symbol negationSym(L"negation");
	Symbol blockNegationSym(L"block_negation");
	Symbol modalSym(L"modal");
	Symbol blockModalSym(L"block_modal");
}

PropPattern::PropPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets)
: _require_one_to_one_argument_mapping(false), 
  _allow_many_to_many_mapping(false), 
  _require_all_arguments_to_match_some_pattern(false),
  _propStatusManager(boost::make_shared<PropStatusManager>()),
  _psm_manually_initialized(false),
  _stem_predicate(false),
  _block_fall_through(false)
{
	// Initialize our predicate type.
	Symbol pred = sexp->getFirstChild()->getValue();
	if (pred == nounPropSym) {
		_predicateType = Proposition::NOUN_PRED;
	} else if (pred == verbPropSym) {
		_predicateType = Proposition::VERB_PRED;
	} else if (pred == modifierPropSym) {
		_predicateType = Proposition::MODIFIER_PRED;
	} else if (pred == setPropSym) {
		_predicateType = Proposition::SET_PRED;
	} else if (pred == compoundPropSym) {
		_predicateType = Proposition::COMP_PRED;
	} else if (pred == anyPropSym) {
		_predicateType = Proposition::ANY_PRED;
	} else throwError(sexp, "unrecognized PropPattern type");

	// Parse the s-expression
	initializeFromSexp(sexp, entityLabels, wordSets);
}

bool PropPattern::initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) 
{
	Symbol atom = childSexp->getValue();
	if (LanguageVariantSwitchingPattern::initializeFromAtom(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (atom == negativeSym) {
		// This is to maintain backwards compatibility		
		if (_psm_manually_initialized)
			throwError(childSexp, "cannot initialize PropStatusManager twice; note that 'NEGATIVE' counts as a psm");
		_propStatusManager = boost::make_shared<PropStatusManager>(true);
		_psm_manually_initialized = true;
		return true;
	} else if (childSexp->getValue() == blockFTSym) {
		_block_fall_through = true;
		return true;
	} else if (childSexp->getValue() == oneToOneSym) {
		_require_one_to_one_argument_mapping = true;
		return true;
	} else if (childSexp->getValue() == manyToManySym) {
		_allow_many_to_many_mapping = true;
		return true;
	} else if (childSexp->getValue() == matchAllArgsSym) {
		_require_all_arguments_to_match_some_pattern = true;
		return true;
	} else if (childSexp->getValue() == stemPredicateSym) {
		_stem_predicate = true;
		return true;
	} else {
		logFailureToInitializeFromAtom(childSexp);
		return false;
	}
}

bool PropPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets)
{
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (LanguageVariantSwitchingPattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (constraintType == predicateSym) {
		fillListOfWordsWithWildcards(childSexp, wordSets, _predicates, _predicatePrefixes);
	} else if (constraintType == alignedPredicateSym) {
		fillListOfWords(childSexp, wordSets, _alignedPredicates);
	} else if (constraintType == blockPredicateSym) {
		fillListOfWordsWithWildcards(childSexp, wordSets, _blockedPredicates, _blockedPredicatePrefixes);
	} else if (constraintType == particleSym) {
		fillListOfWords(childSexp, wordSets, _particles);
	} else if (constraintType == adjectiveSym) {			
		fillListOfWords(childSexp, wordSets, _adjectives);
	} else if (constraintType == blockAdjectiveSym) {			
		fillListOfWords(childSexp, wordSets, _blockedAdjectives);
	} else if (constraintType == blockAdverbParticleSym) {			
		fillListOfWords(childSexp, wordSets, _blockedAdverbsOrParticles);
	} else if (constraintType == adverbParticleSym) {			
		fillListOfWords(childSexp, wordSets, _adverbOrParticles);
	} else if (constraintType == negationSym) {
		fillListOfWordsWithWildcards(childSexp, wordSets, _negations, _negationPrefixes);
	} else if (constraintType == blockNegationSym) {
		fillListOfWordsWithWildcards(childSexp, wordSets, _blockedNegations, _blockedNegationPrefixes);
	} else if (constraintType == modalSym) {
		fillListOfWordsWithWildcards(childSexp, wordSets, _modals, _modalPrefixes);
	} else if (constraintType == blockModalSym) {
		fillListOfWordsWithWildcards(childSexp, wordSets, _blockedModals, _blockedModalPrefixes);
	} else if (constraintType == regexSym) {
		if (_regexPattern != 0)
			throwError(childSexp, "more than one regex in PropPattern");
		_regexPattern = parseSexp(childSexp->getSecondChild(), entityLabels, wordSets);
	} else if (constraintType == blockSym) {
		int n_blocked_args = childSexp->getNumChildren() - 1;
		for (int j = 0; j < n_blocked_args; j++) {
			_blockedArgs.push_back(parseSexp(childSexp->getNthChild(j+1), entityLabels, wordSets));
		}
	} else if (constraintType == argsSym) {
		int n_args = childSexp->getNumChildren() - 1;
		for (int j = 0; j < n_args; j++) {
			_args.push_back(parseSexp(childSexp->getNthChild(j+1), entityLabels, wordSets));
		}
	} else if (constraintType == optArgSym) {
		int n_opt_args = childSexp->getNumChildren() -1;
		for(int j = 0; j < n_opt_args; j++) {
			_optArgs.push_back(parseSexp(childSexp->getNthChild(j+1), entityLabels, wordSets));
		}
	} else if (PropStatusManager::isPropStatusManagerSym(constraintType)) {
		if (_psm_manually_initialized)
			throwError(childSexp, "cannot initialize PropStatusManager twice; note that 'NEGATIVE' counts as a psm");
		_propStatusManager = boost::make_shared<PropStatusManager>(childSexp);
		_psm_manually_initialized = true;
	} else if (constraintType == propModifierSym) {
		if (_propModifierPattern != 0)
			throwError(childSexp, "only one propmod is allowed per pattern");
		if (childSexp->getNumChildren() < 3)
			throwError(childSexp, "propmod format invalid... must be (propMod (role ...) PATTERN)");
		Sexp *roles = childSexp->getSecondChild();
		if (!roles->isList()) throwError(childSexp, "propmod format invalid... must be (propmod (role ...) PATTERN)");
		for (int r = 1; r < roles->getNumChildren(); r++) {
			Sexp *role = roles->getNthChild(r);
			if (!role->isAtom()) throwError(childSexp, "propmod format invalid... must be (propmod (role ...) PATTERN)");
			_propModifierRoles.insert(roles->getNthChild(r)->getValue());
		}
		_propModifierPattern = parseSexp(childSexp->getThirdChild(), entityLabels, wordSets);
	} else {
		logFailureToInitializeFromChildSexp(childSexp);		
		return false;
	}

	return true;
}

Pattern_ptr PropPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {
	replaceShortcut<RegexPattern>(_regexPattern, refPatterns);
	replaceShortcut<Pattern>(_propModifierPattern, refPatterns);
	for (size_t i=0; i<_blockedArgs.size(); ++i) {
		replaceShortcut<ArgumentMatchingPattern>(_blockedArgs[i], refPatterns);
		if (ArgumentPattern_ptr pat = boost::dynamic_pointer_cast<ArgumentPattern>(_blockedArgs[i]))
			pat->disallowCapitalizedRoles();
	}
	for (size_t i=0; i<_args.size(); ++i) {
		replaceShortcut<ArgumentMatchingPattern>(_args[i], refPatterns);
		if (ArgumentPattern_ptr pat = boost::dynamic_pointer_cast<ArgumentPattern>(_args[i]))
			pat->disallowCapitalizedRoles();
	}
	for(size_t i=0; i<_optArgs.size(); ++i) {
		replaceShortcut<ArgumentMatchingPattern>(_optArgs[i], refPatterns);
		if (ArgumentPattern_ptr pat = boost::dynamic_pointer_cast<ArgumentPattern>(_optArgs[i]))
			pat->disallowCapitalizedRoles();
	}

	if (_allow_many_to_many_mapping && _require_one_to_one_argument_mapping)
		throw UnexpectedInputException("PropPattern::replaceShortcuts()", "ONE_TO_ONE and MANY_TO_MANY defined for same PropPattern");

	//Now that shortcuts have been replaced,
	//separate optional and required arguments into their respective vectors.
	std::vector<Pattern_ptr> tempArgs;
	for(size_t i = 0; i < _args.size(); ++i) {
		ArgumentPattern_ptr pat = boost::dynamic_pointer_cast<ArgumentPattern>(_args[i]);
		if (!_allow_many_to_many_mapping) {
			if (pat && pat->hasFallThroughRoles())
				throw UnexpectedInputException("PropPattern::replaceShortcuts()", "argument with allow_fall_through defined for PropPattern that isn't MANY_TO_MANY");
			CombinationPattern_ptr pat2 = boost::dynamic_pointer_cast<CombinationPattern>(_args[i]);
			if (pat2 && pat2->hasFallThroughRoles())
				throw UnexpectedInputException("PropPattern::replaceShortcuts()", "argument with allow_fall_through defined for PropPattern that isn't MANY_TO_MANY");
		}		

		if (pat && pat->isOptional()) {
			_optArgs.push_back(_args[i]);
		} else {
			tempArgs.push_back(_args[i]);
		}
	}
	_args.resize(0);
	std::copy(tempArgs.begin(), tempArgs.end(), std::back_inserter(_args));
	return shared_from_this();
}

// returns all proposition matches in the sentence
PatternFeatureSet_ptr PropPattern::matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) {
	if (_languageVariant && !patternMatcher->getActiveLanguageVariant()->matchesConstraint(*_languageVariant)) {
		return matchesAlignedSentence(patternMatcher, sTheory, _languageVariant); 
	}
	
	PatternFeatureSet_ptr allSentenceProps = boost::make_shared<PatternFeatureSet>();
	bool matched = false;
	
	PropositionSet *propSet = sTheory->getPropositionSet();
	std::vector<float> scores;
	for (int i = 0; i < propSet->getNPropositions(); i++) {
		//if (i != 4) { continue; }
		if (debug != 0) {
			//*debug << "      prop " << (i + 1) << "\n";
			//*debug << propSet->getProposition(i)->toString() << "\n";
		}
		PatternFeatureSet_ptr sfs = matchesProp(patternMatcher, 
			sTheory->getTokenSequence()->getSentenceNumber(), 
			propSet->getProposition(i), false, PropStatusManager_ptr());
		if (sfs != 0) {
			SessionLogger::dbg("BRANDY") << getDebugID() << " PropPattern matches prop# " << i << "\n";
			if (debug != 0) {
				//*debug << "        match!\n";
			}
			allSentenceProps->addFeatures(sfs);
			scores.push_back(sfs->getScore());
			matched = true;
		}
	}
	if (matched) {
		// just want the best one from this sentence, no fancy combination
		allSentenceProps->setScore(ScoringFactory::scoreMax(scores, Pattern::UNSPECIFIED_SCORE));		
		return allSentenceProps;
	} else {
		return PatternFeatureSet_ptr();
	}
}

std::vector<PatternFeatureSet_ptr> PropPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) {
	std::vector<PatternFeatureSet_ptr> return_vector;

	if (_languageVariant && !patternMatcher->getActiveLanguageVariant()->matchesConstraint(*_languageVariant)) {
		return multiMatchesAlignedSentence(patternMatcher, sTheory, _languageVariant); 
	}

	//TODO: parameterize this check
	//if (!patternMatcher->patternQuickCheck(toIRQuery())) {
	//	return return_vector;
	//}

	if (_force_single_match_sentence) {
		PatternFeatureSet_ptr result = matchesSentence(patternMatcher, sTheory, debug);
		if (result.get() != 0) {
			return_vector.push_back(result);
		}
		return return_vector;
	} 

	PropositionSet *propSet = sTheory->getPropositionSet();
	for (int i = 0; i < propSet->getNPropositions(); i++) {
		//if (i != 6) { continue; }
		if (debug != 0) {
			//*debug << "      prop " << (i + 1) << "\n";
			//*debug << propSet->getProposition(i)->toString() << "\n";
		}

		PatternFeatureSet_ptr sfs = matchesProp(patternMatcher, 
			sTheory->getTokenSequence()->getSentenceNumber(), 
			propSet->getProposition(i), false, PropStatusManager_ptr());
		if (sfs != 0) {
			SessionLogger::dbg("BRANDY") << getDebugID() << " PropPattern multi-matches prop# " << i << "\n";
			if (debug != 0) {
				//*debug << "        match!\n";
			}
			return_vector.push_back(sfs);
		}
	}
	return return_vector;
}

PatternFeatureSet_ptr PropPattern::matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides) {
	if (arg->getType() == Argument::PROPOSITION_ARG) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " matchesArgumentValue() matching against a proposition arg.\n";
		return matchesProp(patternMatcher, sent_no, arg->getProposition(), fall_through_children, statusOverrides);
	} else if (arg->getType() == Argument::MENTION_ARG) {
		if (SessionLogger::dbg_or_msg_enabled("BRANDY"))
			SessionLogger::dbg("BRANDY") << getDebugID() << " matchesArgumentValue() trying to find a definitional prop for a mention argument: " << arg->toDebugString() << ".\n";
		Proposition *defProp = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getPropositionSet()->getDefinition(arg->getMentionIndex());
		if (defProp != 0) {
			if (SessionLogger::dbg_or_msg_enabled("BRANDY"))
				SessionLogger::dbg("BRANDY") << getDebugID() << " matchesArgumentValue() found a definitional prop: '" << defProp->toDebugString() << "'\n";
			// Don't fall through if this is a <ref> argument (this is an implicit all-of with a proposition, typically)
			if (arg->getRoleSym() == Argument::REF_ROLE)
				return matchesProp(patternMatcher, sent_no, defProp, false, statusOverrides);
			else return matchesProp(patternMatcher, sent_no, defProp, fall_through_children, statusOverrides);
		} else {
			SessionLogger::dbg("BRANDY") << getDebugID() << " matchesArgumentValue() failed to find a definitional prop for a mention argument with index " << arg->getMentionIndex() << "!\n";
		}
	}
	SessionLogger::dbg("BRANDY") << getDebugID() << " matchesArgumentValue() returning an empty feature set.\n";
	return PatternFeatureSet_ptr();
}

PatternFeatureSet_ptr  PropPattern::matchesProp(PatternMatcher_ptr patternMatcher, int sent_no, const Proposition *prop, bool fall_through_children, PropStatusManager_ptr statusOverrides) {
	if (SessionLogger::dbg_or_msg_enabled("BRANDY"))
		SessionLogger::dbg("BRANDY") << getDebugID() << " Matching against prop '" << prop->toDebugString() << "'\n";
	
	if (!_block_fall_through && fall_through_children) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " In if (fall_through_children)\n";
		
		// First, see if this proposition matches as-is (no fall-through necessary)
		PatternFeatureSet_ptr sfs = matchesProp(patternMatcher, sent_no, prop, false, statusOverrides);
		if (sfs) {
			SessionLogger::dbg("BRANDY") << getDebugID() << " Returning the result of matchesProp(patternMatcher, sent_no, prop, false, statusOverrides)\n";
			return sfs;
		}
		if (SessionLogger::dbg_or_msg_enabled("BRANDY"))
			SessionLogger::dbg("BRANDY") << getDebugID() << " Didn't match, now exploring children for " << prop->toDebugString() << "'\n";

		if (prop->getPredType() == Proposition::COMP_PRED || prop->getPredType() == Proposition::SET_PRED)	{
			// See if any of its children match
			// Just pile up matching features into a single set and return, if so
			std::vector<float> scores;
			PatternFeatureSet_ptr sfs = boost::make_shared<PatternFeatureSet>();
			bool matched = false;
			for (int i = 0; i < prop->getNArgs(); i++) {
				PatternFeatureSet_ptr member_sfs;
				if( prop->getArg(i)->getRoleSym() == Argument::MEMBER_ROLE && 
					(member_sfs = matchesArgumentValue(patternMatcher, sent_no, prop->getArg(i), fall_through_children, statusOverrides)) )
				{
					sfs->addFeatures(member_sfs);
					scores.push_back(member_sfs->getScore());
					matched = true;
				}
			}
			if (matched) {
				// just want the best one from shared_from_this() list, no fancy combination [currently will always =_score anyway]
				sfs->setScore(ScoringFactory::scoreMax(scores, Pattern::UNSPECIFIED_SCORE));
				SessionLogger::dbg("BRANDY") << getDebugID() << " Returning the result of matching a comp or set child.\n";
				return sfs;
			} else {
				SessionLogger::dbg("BRANDY") << getDebugID() << " Didn't match a comp or set child, returning the empty set.\n";
				return PatternFeatureSet_ptr();
			}
		} else {
			// Fall through to the partitive child
			if (prop->getNArgs() > 0) {
				Argument *arg = prop->getArg(0);
				if (arg->getRoleSym() == Argument::REF_ROLE && arg->getType() == Argument::MENTION_ARG) {
					const Mention *refMention = arg->getMention(patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getMentionSet());
					if (refMention->getMentionType() == Mention::PART && refMention->getChild() != 0) {
						Proposition *defProp = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getPropositionSet()->getDefinition(refMention->getChild()->getIndex());
						if (defProp != 0) {
							SessionLogger::dbg("BRANDY") << getDebugID() << " About to return the result of matching a partitive child.\n";
							return matchesProp(patternMatcher, sent_no, defProp, fall_through_children, statusOverrides);
						}
					}
				}
			}
		}
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set from falls_through_children.\n";
		return PatternFeatureSet_ptr();
	}

	if (statusOverrides) {
		if (!statusOverrides->propositionIsValid(patternMatcher, prop)) {
			SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because !statusOverrides->propositionIsValid(patternMatcher, prop)\n";
			return PatternFeatureSet_ptr();
		}
	} else if (!_propStatusManager->propositionIsValid(patternMatcher, prop)) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because !_propStatusManager->propositionIsValid(patternMatcher, prop)\n";
		return PatternFeatureSet_ptr();
	}

	if (prop->getPredType() != _predicateType) {
		if (prop->getPredType() == Proposition::COPULA_PRED && _predicateType == Proposition::VERB_PRED) {
			// then that's OK
		} else if (prop->getPredType() == Proposition::NAME_PRED && _predicateType == Proposition::NOUN_PRED) {
			// this is OK too
		} else if (prop->getPredType() == Proposition::PRONOUN_PRED && _predicateType == Proposition::NOUN_PRED) {
			// this is OK too
		} else if (prop->getPredType() == Proposition::COMP_PRED && _predicateType == Proposition::COMP_PRED) {
			// this is OK too
		} else if (_predicateType == Proposition::ANY_PRED) {
			// It is OK if this prop pattern should match "any" proposition
		} else if (prop->getPredType() == Proposition::POSS_PRED && _predicateType == Proposition::MODIFIER_PRED) {
			// this is OK too
		} else {
			SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because pred types don't match (" << Proposition::getPredTypeString(_predicateType) << " != " << Proposition::getPredTypeString(prop->getPredType()) << ")\n";
			return PatternFeatureSet_ptr();
		}
	}	

	Symbol predSym = prop->getPredSymbol();
	if (_stem_predicate && !predSym.is_null()) {
		if (prop->getPredType() == Proposition::VERB_PRED) {
			predSym = WordNet::getInstance()->stem_verb(prop->getPredSymbol());
		} else if (prop->getPredType() == Proposition::NOUN_PRED) {
			predSym = WordNet::getInstance()->stem_noun(prop->getPredSymbol());
		} else if (prop->getPredType() == Proposition::MODIFIER_PRED) {
			predSym = WordNet::getInstance()->stem_verb(prop->getPredSymbol());
		}
	}

	if (!wordMatchesWithPrefix(predSym, _predicates, _predicatePrefixes, true)) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because invalid predicate\n";
		return PatternFeatureSet_ptr();	
	}
	
	if (_alignedPredicates.size() != 0) {
		if (predSym.is_null()) {
			return PatternFeatureSet_ptr();
		}
		
		bool matchesOne = false;
		for (std::set<Symbol>::iterator it = _alignedPredicates.begin(); it != _alignedPredicates.end(); ++it) {
			
			std::vector<LanguageVariant_ptr> alignedLVs = patternMatcher->getDocumentSet()->getAlignedLanguageVariants(
					patternMatcher->getActiveLanguageVariant());
			for (std::size_t lvi=0;lvi<alignedLVs.size();lvi++) {
				LanguageVariant_ptr lv = alignedLVs[lvi];
						
				SentenceTheory* alignedSentTheory = patternMatcher->getDocumentSet()
						->getDocTheory(lv)->getSentenceTheory(sent_no);
				
				if (!prop->getPredHead()) break;
				//SessionLogger::info("BRANDY") << boost::lexical_cast<std::wstring>(prop->getPredHead()->getStartToken());
				std::vector<int> alignedTokens = patternMatcher->getDocumentSet()->getAlignedTokens(
						patternMatcher->getActiveLanguageVariant(), lv, sent_no, 
						prop->getPredHead()->getStartToken());
				for (std::size_t vectorInd = 0;vectorInd<alignedTokens.size();vectorInd++) {
					int i=alignedTokens[vectorInd];
					
					if (i >= 0 && i < alignedSentTheory->getTokenSequence()->getNTokens()) {
						Symbol alignedSymbol = alignedSentTheory->getTokenSequence()->getToken(i)->getSymbol();
						
						//SessionLogger::info("BRANDY") << L"Is: " << predSym.to_string()
						//	<< L" aligned to " << s.to_string()  << L"? "
						//	<< alignedSymbol.to_string() << L"\n";
						
						if (alignedSymbol == *it) {
							matchesOne = true;
							break;
						}
					}
				} 
				if (matchesOne) break;
			}
			if (matchesOne) break;
		}
		
		if (!matchesOne) {
			return PatternFeatureSet_ptr();
		}
	}
	/*
	SentenceTheory* sTheory = patternMatcher->getDocTheory()->getSentenceTheory(sent_no);
	SessionLogger::info("BRANDY") << sTheory->getTokenSequence()->toString() << L"\n";
	SessionLogger::info("BRANDY") << sTheory->getFullParse()->toTreebankString(true) << L"\n";
	for (int i=0;i<sTheory->getPropositionSet()->getNPropositions();i++) {
		SessionLogger::info("BRANDY") << sTheory->getPropositionSet()->getProposition(i)->toString() << L"\n";
	}
	for (int i=0;i<sTheory->getEntitySet()->getNEntities();i++) {
		Entity* ent = sTheory->getEntitySet()->getEntity(i);
		std::wstring str = L"";
		str += ent->getType().getName().to_string();
		str += L" Entity " + boost::lexical_cast<std::wstring>(ent->getID()) + L": ";
		for (int j=0;j<ent->getNMentions();j++) {
			MentionUID uid = ent->getMention(j);
			if (Mention::getSentenceNumberFromUID(uid) == sTheory->getSentNumber()) {
				str += L"(S)";
			}
			if (patternMatcher->getDocTheory()->getMention(uid)->getMentionType() == Mention::NAME) {
				str += L"(N)";
			}
			str += patternMatcher->getDocTheory()->getMention(uid)->toCasedTextString() + L", ";
		}
		SessionLogger::info("BRANDY") << str << L"\n";
	}
	*/
	
	if (_particles.size() != 0 && (prop->getParticle() == 0 || _particles.find(prop->getParticle()->getHeadWord()) == _particles.end())) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because of particles\n";
		return PatternFeatureSet_ptr();
	}
	if (_adverbOrParticles.size() != 0) {
		bool found = false;
		if (prop->getAdverb() != 0 && _adverbOrParticles.find(prop->getAdverb()->getHeadWord()) != _adverbOrParticles.end())
			found = true;
		// allow particles to count as adverbs... because who the heck really knows which is which!
		if (prop->getParticle() != 0 && _adverbOrParticles.find(prop->getParticle()->getHeadWord()) != _adverbOrParticles.end())
			found = true;
		if (!found) {
			SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because adverb or particle was not found\n";
			return PatternFeatureSet_ptr();
		}
	}
	if (_blockedAdverbsOrParticles.size() != 0) {
		if (prop->getAdverb() != 0 && _blockedAdverbsOrParticles.find(prop->getAdverb()->getHeadWord()) != _blockedAdverbsOrParticles.end()) {
			SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because of blocked adverb\n";
			return PatternFeatureSet_ptr();
		}
		if (prop->getParticle() != 0 && _blockedAdverbsOrParticles.find(prop->getParticle()->getHeadWord()) != _blockedAdverbsOrParticles.end()) {
			SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because of blocked particle\n";
			return PatternFeatureSet_ptr();
		}
	}
	if (_adjectives.size() != 0 && !matchesAdjective(_adjectives, patternMatcher, sent_no, prop)) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because of missing adj\n";
		return PatternFeatureSet_ptr();
	}
	if (_blockedAdjectives.size() != 0 && matchesAdjective(_blockedAdjectives, patternMatcher, sent_no, prop)) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because of blocked adj\n";
		return PatternFeatureSet_ptr();
	}
	if (wordMatchesWithPrefix(predSym, _blockedPredicates, _blockedPredicatePrefixes, false)) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because of blocked predicate\n";
		return PatternFeatureSet_ptr();
	}

	// Negations
	Symbol negation = (prop->getNegation() ? prop->getNegation()->getHeadWord() : Symbol());
	if (!wordMatchesWithPrefix(negation, _negations, _negationPrefixes, true)) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because !wordMatchesWithPrefix(negation, _negations, _negationPrefixes, true)\n";
		return PatternFeatureSet_ptr();
	}
	if (wordMatchesWithPrefix(negation, _blockedNegations, _blockedNegationPrefixes, false)) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because wordMatchesWithPrefix(negation, _blockedNegations, _blockedNegationPrefixes, false)\n";
		return PatternFeatureSet_ptr();
	}

	// Modals
	Symbol modal = (prop->getModal() ? prop->getModal()->getHeadWord() : Symbol());
	if (!wordMatchesWithPrefix(modal, _modals, _modalPrefixes, true)) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because !wordMatchesWithPrefix(modal, _modals, _modalPrefixes, true)\n";
		return PatternFeatureSet_ptr();
	}
	if (wordMatchesWithPrefix(modal, _blockedModals, _blockedModalPrefixes, false)) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because wordMatchesWithPrefix(modal, _blockedModals, _blockedModalPrefixes, false)\n";
		return PatternFeatureSet_ptr();
	}

	// these don't count for scoring, because if they fire, we aren't matching!
	for (size_t i = 0; i < _blockedArgs.size(); i++) {
		ArgumentMatchingPattern_ptr blockedArgPattern = _blockedArgs[i]->castTo<ArgumentMatchingPattern>();
		for (int j = 0; j < prop->getNArgs(); j++) {
			PatternFeatureSet_ptr sfs = blockedArgPattern->matchesArgument(patternMatcher, sent_no, prop->getArg(j), true);
			if (sfs) {
				SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because of blocked args\n";
				return PatternFeatureSet_ptr();
			}
		}
	}

	// Try every matching from pattern argument (i) to this prop argument (j).
	// Take the one with the highest score if it's greater than 0.
	std::vector<std::vector<PatternFeatureSet_ptr> > req_i_j_to_sfs;
	for (size_t i = 0; i < _args.size(); i++) {
		ArgumentMatchingPattern_ptr argMatchingPattern = _args[i]->castTo<ArgumentMatchingPattern>();
		std::vector<PatternFeatureSet_ptr> empty_vector;
		req_i_j_to_sfs.push_back(empty_vector);
		bool matched = false;
		for (int j = 0; j < prop->getNArgs(); j++) {
			if (SessionLogger::dbg_or_msg_enabled("BRANDY"))
				SessionLogger::dbg("BRANDY") << getDebugID() << " Trying to match pattern's arg '" << _args[i]->toDebugString(0) << "' to prop's arg '" << prop->getArg(j)->toDebugString() << "'\n";
			PatternFeatureSet_ptr sfs = argMatchingPattern->matchesArgument(patternMatcher, sent_no, prop->getArg(j), true);
			req_i_j_to_sfs[i].push_back(sfs);
			if (sfs) {
				matched = true;
			}
		}
		// if we fail at a non-optional argument, which should be any argument in this vector, might as well bail now
		if (!matched) {
			//ArgumentPattern_ptr argPattern = boost::dynamic_pointer_cast<ArgumentPattern>(_args[i]);
			//if (argPattern && !argPattern->isOptional()) {
			if (SessionLogger::dbg_or_msg_enabled("BRANDY"))
				SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because missing req arg: " << _args[i]->toDebugString(0) << "\n";
			return PatternFeatureSet_ptr();
			//}
		}
	}
	SessionLogger::dbg("BRANDY") << getDebugID() << " All " << _args.size() << " args matched\n";

	//Do the same with the optional arguments
	std::vector<std::vector<PatternFeatureSet_ptr> > opt_i_j_to_sfs;
	for(size_t i = 0; i < _optArgs.size(); ++i) {
		//SessionLogger::logger->reportInfoMessage() << "On optional argument pattern " << i << "\n";
		ArgumentMatchingPattern_ptr optArgMatchingPattern = _optArgs[i]->castTo<ArgumentMatchingPattern>();
		std::vector<PatternFeatureSet_ptr>empty_vector;
		opt_i_j_to_sfs.push_back(empty_vector);
		for(int j = 0; j < prop->getNArgs(); j++) {
			PatternFeatureSet_ptr sfs = optArgMatchingPattern->matchesArgument(patternMatcher, sent_no, prop->getArg(j), true);
			opt_i_j_to_sfs[i].push_back(sfs);
		}
	}


	PatternFeatureSet_ptr allFeatures = boost::make_shared<PatternFeatureSet>();
	// Add a feature for the return value (if we have one)
	if (getReturn())
		allFeatures->addFeature(boost::make_shared<PropositionReturnPFeature>(shared_from_this(), prop, sent_no, patternMatcher->getActiveLanguageVariant()));
	// Add a feature for the pattern itself
	allFeatures->addFeature(boost::make_shared<PropPFeature>(shared_from_this(), prop, sent_no, patternMatcher->getActiveLanguageVariant()));
	// Add a feature for the ID (if we have one)
	addID(allFeatures);
	// Initialize our score.
	allFeatures->setScore(this->getScore());

	if (_regexPattern != 0) {
		if (prop->getPredHead() == 0) {
			SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because of bad pred head\n";
			return PatternFeatureSet_ptr();
		}
		RegexPattern_ptr regexPattern = _regexPattern->castTo<RegexPattern>();
		PatternFeatureSet_ptr regexSFS = regexPattern->matchesParseNode(patternMatcher, sent_no, prop->getPredHead()->getHeadPreterm());
		if (regexSFS == 0) {
			SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because of regexSFS\n";
			return PatternFeatureSet_ptr();
		}
		allFeatures->addFeatures(regexSFS);
	}

	if (_propModifierPattern != 0) {
		PatternFeatureSet_ptr propModifierSFS = matchesPropModifier(patternMatcher, sent_no, prop);
		if (propModifierSFS == 0) {
			SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because prop modifier SFS\n";
			return PatternFeatureSet_ptr();
		}
		allFeatures->addFeatures(propModifierSFS);
	}

	// fillAllFeatures() will delete elements of i_j_to_sfs and deletes allFeatures if it fails
	allFeatures = fillAllFeatures(req_i_j_to_sfs, opt_i_j_to_sfs, allFeatures, _args, _optArgs,  
		_require_one_to_one_argument_mapping, _require_all_arguments_to_match_some_pattern, _allow_many_to_many_mapping);
	if (allFeatures) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " allFeatures score = " << allFeatures->getScore() << " features = " << allFeatures->getNFeatures() << "\n";
	} else {
		SessionLogger::dbg("BRANDY") << getDebugID() << " allFeatures is null\n";
	}
	return allFeatures;
	
}

PatternFeatureSet_ptr PropPattern::matchesPropModifier(PatternMatcher_ptr patternMatcher, int sent_no, const Proposition *prop) {

	if (_propModifierPattern == 0)
		return PatternFeatureSet_ptr();

	const PropositionSet *ps = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getPropositionSet();

	// Get the index of the Mention of this Proposition's <ref> Argument, falling up through sprops
	const Proposition* searchProp = prop;
	int reference_index = -1;
	for (int j = 0; j < searchProp->getNArgs(); j++) {
		Argument *arg = searchProp->getArg(j);
		if (arg->getRoleSym() == Argument::REF_ROLE && arg->getType() == Argument::MENTION_ARG) {
			reference_index = arg->getMentionIndex();
			break;
		}
	}
	if (reference_index < 0)
		return PatternFeatureSet_ptr();
	bool fallThrough = true;
	while (fallThrough) {
		fallThrough = false;
		for (int i = 0; i < ps->getNPropositions(); i++) {
			int new_reference_index = -1;
			const Proposition *newProp = ps->getProposition(i);
			if (newProp == searchProp || newProp == prop)
				// Avoid prop graph cycles as we follow <ref>s
				continue;
			if (newProp->getPredType() != Proposition::SET_PRED)
				// Only fall up through sprops
				continue;

			// Sweep through the arguments of this sprop, retrieving its <ref> Mention and checking
			// if one of its members is the current <ref> Mention
			for (int k = 0; k < newProp->getNArgs(); k++) {
				Argument *arg = newProp->getArg(k);
				if (arg->getRoleSym() == Argument::REF_ROLE && arg->getType() == Argument::MENTION_ARG) {
					new_reference_index = arg->getMentionIndex();
				} else if (arg->getRoleSym() == Argument::MEMBER_ROLE && arg->getType() == Argument::MENTION_ARG && arg->getMentionIndex() == reference_index) {
					searchProp = newProp;
				}
			}

			// If we found a new sprop to fall through, update our <ref> Mention and keep falling up
			if (searchProp == newProp && new_reference_index >= 0) {
				reference_index = new_reference_index;
				fallThrough = true;
				break;
			}
		}
	}

	// Now that we've fallen up to the <ref> Mention, actually run the pattern against props that contain it in an arg
	PatternFeatureSet_ptr bestSFS;
	PropMatchingPattern_ptr propModifierPattern = _propModifierPattern->castTo<PropMatchingPattern>();
	for (int i = 0; i < ps->getNPropositions(); i++) {
		const Proposition *newProp = ps->getProposition(i);
		if (newProp == prop)
			continue;
		for (int k = 0; k < newProp->getNArgs(); k++) {
			Argument *arg = newProp->getArg(k);
			if (arg->getType() == Argument::MENTION_ARG && arg->getMentionIndex() == reference_index &&
				_propModifierRoles.find(arg->getRoleSym()) != _propModifierRoles.end()) 
			{				
				PatternFeatureSet_ptr sfs = propModifierPattern->matchesProp(patternMatcher, sent_no, newProp, true, PropStatusManager_ptr());
				if (sfs) {
					if (bestSFS == 0 || sfs->getScore() > bestSFS->getScore())
						bestSFS = sfs;
				}
			}
		}
	}
	return bestSFS;
}

bool PropPattern::matchesNegation(const std::set<Symbol>& negations, const Proposition *prop) {
	Symbol head = prop->getNegation()->getHeadWord();
	return (negations.find(head) != negations.end());
}

bool PropPattern::matchesAdjective(const std::set<Symbol>& adjectives, PatternMatcher_ptr patternMatcher, int sent_no, const Proposition *prop) {

	// Adjective matching only makes sense for noun propositions
	if (prop->getPredType() != Proposition::NOUN_PRED || prop->getNArgs() == 0)
		return false;

	// The first argument should be the <ref> argument; if it's not, bail
	Argument *firstArg = prop->getArg(0);	
	if (firstArg->getRoleSym() != Argument::REF_ROLE || firstArg->getType() != Argument::MENTION_ARG)
		return false;
	int ref_ment_index = firstArg->getMentionIndex();
	
	// Find all modifier propositions in the set that modify the same mention as the noun prop
	const PropositionSet *ps = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getPropositionSet();
	for (int i = 0; i < ps->getNPropositions(); i++) {
		const Proposition *newProp = ps->getProposition(i);
		if (newProp->getPredType() == Proposition::MODIFIER_PRED && newProp->getNArgs() != 0) {
			Argument *newPropFirstArg = newProp->getArg(0);
			if (newPropFirstArg->getMentionIndex() == ref_ment_index &&
				newPropFirstArg->getRoleSym() == Argument::REF_ROLE)
			{
				Symbol prop_adj = newProp->getPredHead()->getHeadWord();
				if (adjectives.find(prop_adj) != adjectives.end())
					return true;
			}
		}
	}

	for (int i = 0; i < prop->getNArgs(); i++) {
		if (prop->getArg(i)->getRoleSym() == Argument::UNKNOWN_ROLE && prop->getArg(i)->getType() == Argument::MENTION_ARG) {
			const Mention *ment = prop->getArg(i)->getMention(patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getMentionSet());
			Symbol premod_headword = ment->getNode()->getHeadWord();
			if (adjectives.find(premod_headword) != adjectives.end())
				return true;
		}
	}
	
	return false;
}

void PropPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";	
	out << "PropPattern (" << Proposition::getPredTypeString(_predicateType) << "):";
	if (!getID().is_null()) out << getID();
	out << std::endl;
	for (int i = 0; i < indent; i++) out << " ";
	out << "  Score: " << _score << "\n";

	std::set<Symbol> emptySet;
	typedef std::pair<const std::set<Symbol>*, const std::set<Symbol>*> WordList;
	typedef std::pair<std::string, WordList> WordListPair;
	std::vector<std::pair<std::string, WordList> > wordLists;
	if (_stem_predicate) {
		wordLists.push_back(std::make_pair("stemmed predicates", std::make_pair(&_predicates, &_predicatePrefixes)));
		wordLists.push_back(std::make_pair("blocked stemmed predicates", std::make_pair(&_blockedPredicates, &_blockedPredicatePrefixes)));
	} else {
		wordLists.push_back(std::make_pair("predicates", std::make_pair(&_predicates, &_predicatePrefixes)));
		wordLists.push_back(std::make_pair("blocked predicates", std::make_pair(&_blockedPredicates, &_blockedPredicatePrefixes)));
	}
	wordLists.push_back(std::make_pair("particles", std::make_pair(&_particles, &emptySet)));
	wordLists.push_back(std::make_pair("adjectives", std::make_pair(&_adjectives, &emptySet)));
	wordLists.push_back(std::make_pair("blocked adjectives", std::make_pair(&_blockedAdjectives, &emptySet)));
	wordLists.push_back(std::make_pair("adverbs or particles", std::make_pair(&_adverbOrParticles, &emptySet)));
	wordLists.push_back(std::make_pair("blocked adverbs or particles", std::make_pair(&_blockedAdverbsOrParticles, &emptySet)));
	wordLists.push_back(std::make_pair("prop modifier roles", std::make_pair(&_propModifierRoles, &emptySet)));
	wordLists.push_back(std::make_pair("negations", std::make_pair(&_negations, &_negationPrefixes)));
	wordLists.push_back(std::make_pair("blocked neagations", std::make_pair(&_blockedNegations, &_blockedNegationPrefixes)));
	wordLists.push_back(std::make_pair("modals", std::make_pair(&_modals, &_modalPrefixes)));
	wordLists.push_back(std::make_pair("blocked modals", std::make_pair(&_blockedModals, &_blockedModalPrefixes)));
	BOOST_FOREACH(const WordListPair &wordListPair, wordLists) {
		const std::string& name = wordListPair.first;
		const std::set<Symbol>* words = wordListPair.second.first;
		const std::set<Symbol>* prefixes = wordListPair.second.second;
		if ((words->size()>0) | (prefixes->size()>0)) {
			for (int i = 0; i < indent; i++) out << " ";
			out << "  " << name << " = {";
			BOOST_FOREACH(Symbol sym, *words) {
				out << sym << " ";
			}
			BOOST_FOREACH(Symbol sym, *prefixes) {
				out << sym << "* ";
			}
			out << "}\n";	
		}
	}

	if (!_blockedArgs.empty()) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  blocked_args = {\n";
		for (size_t j = 0; j < _blockedArgs.size(); j++)
			_blockedArgs[j]->dump(out, indent+4);		
		for (int i = 0; i < indent; i++) out << " ";
		out << "}\n";
	}

	if (!_args.empty()) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  args = {\n";
		for (size_t j = 0; j < _args.size(); j++)
			_args[j]->dump(out, indent+4);		
		for (int i = 0; i < indent; i++) out << " ";
		out << "}\n";
	}

	if(!_optArgs.empty()) {
		for(int i =0; i < indent; i++) out << " ";
		out << " opt args = {\n";
		for(size_t j = 0; j < _optArgs.size(); j++)
			_optArgs[j]->dump(out, indent+4);
		for(int i = 0; i < indent; i++) out << " ";
		out << "}\n";
	}
}

/**
 * Redefinition of parent class's virtual method that returns a vector of 
 * pointers to PatternReturn objects for a given pattern.
 *
 * @param pointer to vector of pointers to PatternReturn objects for a given pattern
 *
 * @author afrankel@bbn.com
 * @date 2010.10.20
 **/
void PropPattern::getReturns(PatternReturnVecSeq & output) const {
	// Note: this currently does *not* recurse into _regexpPattern.  It is unclear
	// whether that is intentional or not.
	Pattern::getReturns(output);
	for (size_t n = 0; n < _args.size(); ++n) {
		if (_args[n]) {
			_args[n]->getReturns(output);
		}
	}
	for (size_t n = 0; n < _optArgs.size(); ++n) {
		if (_optArgs[n]) {
			_optArgs[n]->getReturns(output);
		}
	}
}

