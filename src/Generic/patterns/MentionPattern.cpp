// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/MentionPattern.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/features/MentionPFeature.h"
#include "Generic/patterns/ShortcutPattern.h"
#include "Generic/patterns/EntityLabelPattern.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/RegexPattern.h"
#include "Generic/patterns/ScoringFactory.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/common/Sexp.h"
#include <sstream>


// Private symbols
namespace {
	Symbol blockSym(L"block");
	Symbol focusSym(L"FOCUS");
	Symbol acetypeSym(L"acetype");
	Symbol acesubtypeSym(L"acesubtype");
	Symbol mentiontypeSym(L"mentiontype");
	Symbol minEntityLevelSym(L"min-entitylevel");
	Symbol entityLabelSym(L"entitylabel");
	Symbol mentionLabelSym(L"mentionlabel");
	Symbol headwordSym(L"headword");
	Symbol blockAcetypeSym(L"block_acetype");
	Symbol blockHeadwordSym(L"block_headword");
	Symbol regexSym(L"regex");
	Symbol headRegexSym(L"head-regex");
	Symbol specificSym(L"SPECIFIC");
	Symbol genericSym(L"GENERIC");
	Symbol appositiveSym(L"APPOSITIVE");
	Symbol appositiveChildSym(L"APPOSITIVE_CHILD");
	Symbol namedAppositiveChildSym(L"NAMED_APPOSITIVE_CHILD");
	Symbol blockFTSym(L"BLOCK_FALL_THROUGH");
	Symbol docETypeFreqSym(L"doc_etype_freq");
	Symbol sentETypeFreqSym(L"sent_etype_freq");
	Symbol brownClusterSym(L"brown_cluster");
	Symbol equalSym(L"==");
	Symbol notEqualSym(L"!=");
	Symbol lessThanSym(L"<");
	Symbol lessThanOrEqualSym(L"<=");
	Symbol greaterThanSym(L">");
	Symbol greaterThanOrEqualSym(L">=");
	Symbol propDefSym(L"prop-def");
	Symbol argOfPropSym(L"arg-of-prop");
	Symbol roleSym(L"role");
}

MentionPattern::MentionPattern(Sexp *sexp, const Symbol::HashSet &entityLabels,
							   const PatternWordSetMap& wordSets) :
_is_focus(false), _requires_name(false), _requires_name_or_desc(false),
_is_specific(false), _is_generic(false), _is_appositive(false),
_is_appositive_child(false), _is_named_appositive_child(false),
_head_regex(false), _block_fall_through(false)
{
	initializeFromSexp(sexp, entityLabels, wordSets);
	if (_is_generic && _is_specific)
		throwError(sexp, "GENERIC and SPECIFIC cannot both be set in MentionPattern");
}

bool MentionPattern::initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol atom = childSexp->getValue();
	if (LanguageVariantSwitchingPattern::initializeFromAtom(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (atom == focusSym) {
		_is_focus = true;
	} else if (atom == specificSym) {
		_is_specific = true;
	} else if (atom == appositiveSym) {
		_is_appositive = true;
	} else if (atom == appositiveChildSym) {
		_is_appositive_child = true;
	} else if (atom == namedAppositiveChildSym) {
		_is_named_appositive_child = true;
	} else if (atom == genericSym) {
		_is_generic = true;
	} else if (atom == blockFTSym) {
		_block_fall_through = true;
	} else {
		logFailureToInitializeFromAtom(childSexp);
		return false;
	}
	return true;
}

bool MentionPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (LanguageVariantSwitchingPattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (constraintType == acetypeSym) {
		int n_ace_types = childSexp->getNumChildren() - 1;
		if (n_ace_types != 0) {
			for (int j = 0; j < n_ace_types; j++) {
				_aceTypes.push_back(EntityType(childSexp->getNthChild(j+1)->getValue()));
			}
		}
	} else if (constraintType == acesubtypeSym) {
		int n_ace_subtypes = childSexp->getNumChildren() - 1;
		if (n_ace_subtypes != 0) {
			for (int j = 0; j < n_ace_subtypes; j++) {
				_aceSubtypes.push_back(EntitySubtype(childSexp->getNthChild(j+1)->getValue()));
			}
		}
	} else if (constraintType == mentiontypeSym) {
		int n_mention_types = childSexp->getNumChildren() - 1;
		if (n_mention_types != 0) {
			for (int j = 0; j < n_mention_types; j++) {
				_mentionTypes.insert(Mention::getTypeFromString(childSexp->getNthChild(j+1)->getValue().to_debug_string()));
			}
		}
	} else if (constraintType == docETypeFreqSym || constraintType == sentETypeFreqSym) {
		if (childSexp->getNumChildren() != 3 || !childSexp->getSecondChild()->isAtom() || !childSexp->getThirdChild()->isAtom())
			throwError(childSexp, "comparison constraints must have three atomic children");
		ComparisonConstraint cc;
		cc.constraint_type = constraintType;
		cc.comparison_operator = childSexp->getSecondChild()->getValue();
		if (cc.comparison_operator != equalSym &&
			cc.comparison_operator != notEqualSym &&
			cc.comparison_operator != lessThanSym &&
			cc.comparison_operator != lessThanOrEqualSym &&
			cc.comparison_operator != greaterThanSym &&
			cc.comparison_operator != greaterThanOrEqualSym)
			throwError(childSexp, "comparison operator must be ==, !=, <, >, <=, or >=");
		cc.value = boost::lexical_cast<int>(childSexp->getThirdChild()->getValue().to_string());
		_comparisonConstraints.push_back(cc);
	} else if (constraintType == minEntityLevelSym) {
		if (childSexp->getSecondChild()->getValue() == Symbol(L"NAME"))
			_requires_name = true;
		else if (childSexp->getSecondChild()->getValue() == Symbol(L"DESC"))
			_requires_name_or_desc = true;
		else throwError(childSexp, "minmentionlevel must be NAME or DESC");
	} else if (constraintType == entityLabelSym) {
		int n_entity_labels = childSexp->getNumChildren() - 1;
		if (n_entity_labels != 0) {
			for (int j = 0; j < n_entity_labels; j++) {
				Symbol label = childSexp->getNthChild(j+1)->getValue();
				_entityLabels.push_back(label);
				if (entityLabels.find(label)==entityLabels.end())
					throwError(childSexp, "unrecognized entity label");
			}
		}
	} else if (constraintType == blockSym) {
		int n_blocking_entity_labels = childSexp->getNumChildren() - 1;
		if (n_blocking_entity_labels != 0) {
			for (int j = 0; j < n_blocking_entity_labels; j++) {
				Symbol label = childSexp->getNthChild(j+1)->getValue();
				_blockingEntityLabels.push_back(label);
				if (entityLabels.find(label)==entityLabels.end())
					throwError(childSexp, "unrecognized entity label");
			}
		}
	} else if (constraintType == headwordSym) {
		if ((childSexp->getNumChildren() - 1) > 0) {
			fillListOfWordsWithWildcards(childSexp, wordSets, _headwords, _headwordPrefixes);
		}
	} else if (constraintType == blockAcetypeSym) {
		int n_ace_types = childSexp->getNumChildren() - 1;
		if (n_ace_types != 0) {
			for (int j = 0; j < n_ace_types; j++) {
				_blockedAceTypes.push_back(EntityType(childSexp->getNthChild(j+1)->getValue()));
			}
		}
	} else if (constraintType == blockHeadwordSym) {
		if ((childSexp->getNumChildren() - 1) > 0) {
			fillListOfWordsWithWildcards(childSexp, wordSets, _blockedHeadwords, _blockedHeadwordPrefixes);
		}
	} else if (constraintType == regexSym || constraintType == headRegexSym) {
		if (_regexPattern)
			throwError(childSexp, "more than one regex in MentionPattern");
		_regexPattern = parseSexp(childSexp->getSecondChild(), entityLabels, wordSets);
		if (!(boost::dynamic_pointer_cast<RegexPattern>(_regexPattern) ||
			  boost::dynamic_pointer_cast<ShortcutPattern>(_regexPattern)))
			  throwError(childSexp, "Expected a regexp pattern");
		_head_regex = (constraintType == headRegexSym);
	} else if (constraintType == propDefSym) {
		if (_propDefPattern)
			throwError(childSexp, "more than one prop-def in MentionPattern");
		_propDefPattern = parseSexp(childSexp->getSecondChild(), entityLabels, wordSets);
	} else if (constraintType == argOfPropSym) {
		if ((childSexp->getNumChildren() < 2) || (childSexp->getNumChildren() > 3))
			throwError(childSexp, "arg-of-prop syntax: (arg-of-prop [(role ...)] (prop ...))");
		ArgOfPropConstraint_ptr aop = boost::make_shared<ArgOfPropConstraint>();
		if (childSexp->getNumChildren() == 2) {
			aop->propPattern = parseSexp(childSexp->getSecondChild(), entityLabels, wordSets);
		} else {
			Sexp *roleSexp = childSexp->getSecondChild();
			if (!(roleSexp->isList() && roleSexp->getFirstChild()->isAtom() && (roleSexp->getFirstChild()->getValue()==roleSym)))
				throwError(childSexp, "arg-of-prop syntax: (arg-of-prop [(role ...)] (prop ...))");
			aop->propPattern = parseSexp(childSexp->getThirdChild(), entityLabels, wordSets);
			fillListOfWords(roleSexp, wordSets, aop->roles);
		}
		_argOfPropConstraints.push_back(aop);
	} else if (constraintType == brownClusterSym) {
		WordClusterTable::ensureInitializedFromParamFile();
		_brownClusterConstraint = childSexp->getSecondChild()->getValue();
		//if it ends with a *, then it is a prefix bitstring
		int len = static_cast<int>(wcslen(_brownClusterConstraint.to_string()));
		if (_brownClusterConstraint.to_string()[len-1] == L'*') {
			_brown_cluster_prefix = len-1;
		} else {
			_brown_cluster_prefix = 0;
		}
		_brownCluster = wcstol(_brownClusterConstraint.to_string(), 0, 2);
	} else {
		logFailureToInitializeFromChildSexp(childSexp);		
		return false;
	}
	return true;
}

PatternFeatureSet_ptr MentionPattern::matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) {
	if (_languageVariant && !patternMatcher->getActiveLanguageVariant()->matchesConstraint(*_languageVariant)) {
		return matchesAlignedSentence(patternMatcher, sTheory, _languageVariant); 
	}
	
	MentionSet *mSet = sTheory->getMentionSet();
	PatternFeatureSet_ptr allSentenceMentions = boost::make_shared<PatternFeatureSet>();
	std::vector<float> scores;
	bool matched = false;
	for (int i = 0; i < mSet->getNMentions(); i++) {		
		PatternFeatureSet_ptr sfs = matchesMention(patternMatcher, mSet->getMention(i), false);
		if (sfs != 0) {
			allSentenceMentions->addFeatures(sfs);
			scores.push_back(sfs->getScore());
			matched = true;
		}
	}
	if (matched) {
		// just want the best one from shared_from_this() sentence, no fancy combination [currently will always =_score anyway]
		allSentenceMentions->setScore(ScoringFactory::scoreMax(scores, Pattern::UNSPECIFIED_SCORE));
		return allSentenceMentions;
	} else {
		return PatternFeatureSet_ptr();
	}
}

std::vector<PatternFeatureSet_ptr> MentionPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) {
	std::vector<PatternFeatureSet_ptr> return_vector;

	if (_languageVariant && !patternMatcher->getActiveLanguageVariant()->matchesConstraint(*_languageVariant)) {
		return multiMatchesAlignedSentence(patternMatcher, sTheory, _languageVariant); 
	}

	if (_force_single_match_sentence) {
		PatternFeatureSet_ptr result = matchesSentence(patternMatcher, sTheory, debug);
		if (result.get() != 0) {
			return_vector.push_back(result);
		}
		return return_vector;
	}

	MentionSet *mSet = sTheory->getMentionSet();
	for (int i = 0; i < mSet->getNMentions(); i++) {		
		PatternFeatureSet_ptr sfs = matchesMention(patternMatcher, mSet->getMention(i), false);
		if (sfs != 0)
			return_vector.push_back(sfs);
	}
	return return_vector;
}


PatternFeatureSet_ptr MentionPattern::matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides) {
	if (arg->getType() == Argument::MENTION_ARG) {
		MentionSet *mSet = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getMentionSet();
		const Mention *ment = arg->getMention(mSet);
		// Don't fall through if this is a <ref> argument (this is an implicit all-of with a proposition, typically)
		if (arg->getRoleSym() == Argument::REF_ROLE)
			return matchesMention(patternMatcher, ment, false);
		else return matchesMention(patternMatcher, ment, fall_through_children);
	} else {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because argument type is " << arg->getType() << ", not Mention\n";
		return PatternFeatureSet_ptr();
	}
}

PatternFeatureSet_ptr MentionPattern::matchesMention(PatternMatcher_ptr patternMatcher, const Mention *ment, bool fall_through_children) {
	if (_languageVariant && !patternMatcher->getActiveLanguageVariant()->matchesConstraint(*_languageVariant)) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning the result of matchesAlignedMention(patternMatcher, " << ment->getUID() << ", " << fall_through_children << ", _languageVariant)\n";
		return matchesAlignedMention(patternMatcher, ment, fall_through_children, _languageVariant); 
	}
	
	if (!_block_fall_through && fall_through_children && ment->getMentionType() == Mention::LIST) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Attempting LIST Mention match\n";

		// First, see if this matches as-is (no fall-through necessary)
		PatternFeatureSet_ptr sfs = matchesMention(patternMatcher, ment, false);
		if (sfs) {
			SessionLogger::dbg("BRANDY") << getDebugID() << " Returning the result of matchesMention()\n";
			return sfs;
		}
		
		// Now, see if any of its children matches
		// Just pile up matching features into a single set and return, if so
		const Mention *memberMent = ment->getChild();
		std::vector<float> scores;
		sfs = boost::make_shared<PatternFeatureSet>();
		bool matched = false;
		while (memberMent != 0) {
			PatternFeatureSet_ptr member_sfs( matchesMention(patternMatcher, memberMent, fall_through_children) );
			if (member_sfs) {
				SessionLogger::dbg("BRANDY") << getDebugID() << " Matched LIST Mention member " << memberMent->getUID() << "\n";
				sfs->addFeatures(member_sfs);
				scores.push_back(member_sfs->getScore());
				matched = true;
			}
			memberMent = memberMent->getNext();
		}
		if (matched) {
			// just want the best one from shared_from_this() list, no fancy combination [currently will always =_score anyway]
			sfs->setScore(ScoringFactory::scoreMax(scores, Pattern::UNSPECIFIED_SCORE));
			SessionLogger::dbg("BRANDY") << getDebugID() << " Returning " << sfs->getNFeatures() << " LIST Mention match(es)\n";
			return sfs;
		} else {
			SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because no LIST member Mentions matched\n";
			return PatternFeatureSet_ptr();
		}
	} 

	if (!_block_fall_through && fall_through_children && ment->getMentionType() == Mention::PART) {

		// First see if this matches as-is
		PatternFeatureSet_ptr sfs = matchesMention(patternMatcher, ment, false);
		if (sfs)
			return sfs;

		if (ment->getChild() != 0)
			return matchesMention(patternMatcher, ment->getChild(), fall_through_children);
	}

	if (_is_appositive && ment->getMentionType() != Mention::APPO)
		return PatternFeatureSet_ptr();

	if (_is_appositive_child && (ment->getParent() == 0 || ment->getParent()->getMentionType() != Mention::APPO))
		return PatternFeatureSet_ptr();

	if (_is_named_appositive_child) {
		const Mention *appoMent = ment->getParent();
		if (appoMent == 0 || appoMent->getMentionType() != Mention::APPO)
			return PatternFeatureSet_ptr();
		const Mention *iter = appoMent->getChild();
		bool found_name = false;
		while (iter != 0) {
			if (iter->getMentionType() == Mention::NAME) {
				found_name = true;
				break;
			} else iter = iter->getNext();
		}
		if (!found_name)
			return PatternFeatureSet_ptr();
	}

	for (size_t i = 0; i < _blockedAceTypes.size(); i++) { // Check for blocked ace types
		if (ment->getEntityType() == _blockedAceTypes[i]) {
			return PatternFeatureSet_ptr();
		}
	}	

	if (_mentionTypes.size() > 0 && 
		_mentionTypes.find(ment->getMentionType()) == _mentionTypes.end())
		return PatternFeatureSet_ptr();

	// Let's try to keep stuff that depends on the existance of an entity below here
	EntitySet *entitySet = patternMatcher->getDocTheory()->getEntitySet();
	const Entity *ent = 0;
	
	if (entitySet)
		ent = entitySet->lookUpEntityForMention(ment->getUID());

	//
	// make sure you always check that ent != 0
	//

	for (size_t i = 0; i < _blockingEntityLabels.size(); i++) {
		if (ent != 0 && patternMatcher->getEntityLabelMatch(_blockingEntityLabels[i], ent->getID()) != 0)
			return PatternFeatureSet_ptr();
	}

	if (_requires_name || _requires_name_or_desc || _is_specific || _is_generic) {
		if (ent == 0) 
			return PatternFeatureSet_ptr();
		if (_requires_name) {
			if (!ent->hasNameMention(entitySet))
				return PatternFeatureSet_ptr();
		} else if (_requires_name_or_desc) {
			if (!ent->hasNameOrDescMention(entitySet))
				return PatternFeatureSet_ptr();
		}
		if (_is_specific && isGeneric(ent, patternMatcher))
			return PatternFeatureSet_ptr();
		if (_is_generic && !isGeneric(ent, patternMatcher))
			return PatternFeatureSet_ptr();
	}
	
	Symbol headword = ment->getNode()->getHeadWord();
	if (!wordMatchesWithPrefix(headword, _headwords, _headwordPrefixes, true))
		return PatternFeatureSet_ptr();	

	if (wordMatchesWithPrefix(headword, _blockedHeadwords, _blockedHeadwordPrefixes, false))
		return PatternFeatureSet_ptr();

	BOOST_FOREACH(ComparisonConstraint cc, _comparisonConstraints) {
		if (entitySet == 0 || ent == 0)
			return PatternFeatureSet_ptr();
		int actual_value = 0;
		if (cc.constraint_type == docETypeFreqSym) {
			actual_value = entitySet->getNEntitiesByType(ent->getType());
		} else if (cc.constraint_type == sentETypeFreqSym) {
			// This should probably be cached somewhere
			const MentionSet *mSet = patternMatcher->getDocTheory()->getSentenceTheory(ment->getSentenceNumber())->getMentionSet();
			std::set<const Entity*> entityIDs;
			for (int m = 0; m < mSet->getNMentions(); m++) {
				const Mention *sentMent = mSet->getMention(m);
				if (sentMent->getEntityType() == ment->getEntityType()) {
					const Entity* sentEnt = entitySet->lookUpEntityForMention(sentMent->getUID());
					if (sentEnt)
						entityIDs.insert(sentEnt);
				}
			}
			actual_value = static_cast<int>(entityIDs.size());
		}
		if ((cc.comparison_operator == equalSym && actual_value != cc.value) ||
			(cc.comparison_operator == notEqualSym && actual_value == cc.value) ||
			(cc.comparison_operator == lessThanSym && actual_value >= cc.value) ||
			(cc.comparison_operator == lessThanOrEqualSym && actual_value > cc.value) ||
			(cc.comparison_operator == greaterThanSym && actual_value <= cc.value) ||
			(cc.comparison_operator == greaterThanOrEqualSym && actual_value < cc.value))
		{
			return PatternFeatureSet_ptr();
		}
	}

	if (!_brownClusterConstraint.is_null()) {
		int* wordCluster = WordClusterTable::domainGet(ment->getNode()->getHeadWord());
		if (wordCluster) {
			if (_brown_cluster_prefix) {
				int tempCluster = *wordCluster;
				while (tempCluster != _brownCluster) {
					tempCluster >>= 1;
					if (tempCluster == 0) {
						return PatternFeatureSet_ptr();
					}
				}
			} else {
				if (*wordCluster != _brownCluster) {
					return PatternFeatureSet_ptr();
				}
			}
		} else {
			return PatternFeatureSet_ptr(); 
		}
	}

	if (_aceTypes.size() == 0 && _aceSubtypes.size() == 0 && _entityLabels.size() == 0) {
		return makePatternFeatureSet(ment, patternMatcher);
	}

	if (ParamReader::isParamTrue("force_entitylabels") && _entityLabels.size() > 0) {
		if (ent == 0)
			return PatternFeatureSet_ptr();

		for (size_t i = 0; i < _entityLabels.size(); i++) {
			PatternFeatureSet_ptr entityLabelMatch = patternMatcher->getEntityLabelMatch(_entityLabels[i], ent->getID());
			if (entityLabelMatch == 0) {
				return PatternFeatureSet_ptr(); 
			}
		}
	}

	if (ParamReader::isParamTrue("force_entity_subtypes") && _aceSubtypes.size() > 0) {
		if (ent == 0)
			return PatternFeatureSet_ptr();

		EntitySubtype est = entitySet->guessEntitySubtype(ent);

		bool bMatch = false;
		for (size_t i = 0; i < _aceSubtypes.size(); i++) {
			if (est == _aceSubtypes[i]) {
				bMatch = true;
				break;
			}
		}

		if(!bMatch)
			return PatternFeatureSet_ptr(); 
	}

	if (ParamReader::isParamTrue("force_entity_types") && _aceTypes.size() > 0) {
		if (ent == 0)
			return PatternFeatureSet_ptr();

		EntityType ent_type = ment->getEntityType();

		bool bMatch = false;
		for (size_t i = 0; i < _aceTypes.size(); i++) {
			if (ent_type == _aceTypes[i]) {
				bMatch = true;
				break;
			}
		}

		if(!bMatch)
			return PatternFeatureSet_ptr(); 
	}

	for (size_t i = 0; i < _aceTypes.size(); i++) {
		if (ment->getEntityType() == _aceTypes[i]) {
			return makePatternFeatureSet(ment, patternMatcher, ment->getEntityType().getName());
		}
	}

	if (ent == 0)
		return PatternFeatureSet_ptr();

	for (size_t i = 0; i < _entityLabels.size(); i++) {
		PatternFeatureSet_ptr entityLabelMatch = patternMatcher->getEntityLabelMatch(_entityLabels[i], ent->getID());
		if (entityLabelMatch != 0) {
			PatternFeatureSet_ptr sfs = makePatternFeatureSet(ment, patternMatcher, _entityLabels[i], 1.0F);
			if (!sfs)
				return sfs;
			sfs->addFeatures(entityLabelMatch);
			return sfs;
		}
	}


	if (_aceSubtypes.size() != 0) {
		// this will only match determined subtypes, not default ones!
		EntitySubtype est = entitySet->guessEntitySubtype(ent);
		for (size_t i = 0; i < _aceSubtypes.size(); i++) {
			if (est == _aceSubtypes[i]) {
				return makePatternFeatureSet(ment, patternMatcher, est.getName());
			}
		}
	}

	return PatternFeatureSet_ptr();
}

/** [XX] In the Brandy implementation, this called patternMatcher->isGenericEntity(),
  * which has its own notion of what is Generic and what is not.  Here we are
  * using ent->isGeneric() instead.  It is not clear how the notions of 
  * "generic entity" differ in these two contexts.  If the old behavior is
  * required, then we should set the genericity value appropriately on the 
  * entities, rather than having this strange alternate value in what used
  * to be called DocumentInfo (and is now PatternMatcher) */
bool MentionPattern::isGeneric(const Entity* ent, PatternMatcher_ptr patternMatcher) const {
	return ent->isGeneric();
	// [XX] THE OLD IMPLEMENTATION
	//return patternMatcher->isGenericEntity(ent->getID());
}

PatternFeatureSet_ptr MentionPattern::makePatternFeatureSet(const Mention* mention, PatternMatcher_ptr patternMatcher, 
															const Symbol matchSym, float confidence) { 
	PatternFeatureSet_ptr sfs = boost::make_shared<PatternFeatureSet>();
	// Add a feature for the return value (if we have one)
	if (getReturn())
		sfs->addFeature(boost::make_shared<MentionReturnPFeature>(shared_from_this(), mention, matchSym, patternMatcher->getActiveLanguageVariant(), confidence));
	// Add a feature for the pattern itself
	sfs->addFeature(boost::make_shared<MentionPFeature>(shared_from_this(), mention, matchSym, patternMatcher->getActiveLanguageVariant(), confidence));
	// Add a feature for the ID (if we have one)
	addID(sfs);
	// Add in any features from the regex sub-pattern.
	if (_regexPattern) {
		RegexPattern_ptr regexPattern = _regexPattern->castTo<RegexPattern>();
		PatternFeatureSet_ptr regexSfs;
		if (_head_regex) {
			regexSfs = regexPattern->matchesMentionHead(patternMatcher, mention);
		} else { 
			regexSfs = regexPattern->matchesMention(patternMatcher, mention, false); // fall_through_children not relevant here
		}
		if (regexSfs) {
			sfs->addFeatures(regexSfs);
		} else {
			return PatternFeatureSet_ptr();
		}
	}
	// Add in any features from the prop-def subpattern (and check that it matches)
	if (_propDefPattern) {
		PropMatchingPattern_ptr propDefPattern = _propDefPattern->castTo<PropMatchingPattern>();
		int sent_no = mention->getSentenceNumber();
		PropositionSet *propSet = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getPropositionSet();
		if (!propSet)
			return PatternFeatureSet_ptr();
		Proposition *defProp = propSet->getDefinition(mention->getIndex());
		if (!defProp)
			return PatternFeatureSet_ptr();
		// are these values for fall_through_children and statusOverrides ok?
		PatternFeatureSet_ptr pdSfs = propDefPattern->matchesProp(patternMatcher, sent_no, defProp, false, PropStatusManager_ptr());
		if (!pdSfs)
			return PatternFeatureSet_ptr();
		sfs->addFeatures(pdSfs);
	}
	// Add in any features from arg-of-prop subpatterns (and check that they match)
	BOOST_FOREACH(ArgOfPropConstraint_ptr aop, _argOfPropConstraints) {
		PropMatchingPattern_ptr aopPattern = aop->propPattern->castTo<PropMatchingPattern>();
		int sent_no = mention->getSentenceNumber();
		PropositionSet *propSet = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getPropositionSet();
		MentionSet *mentionSet = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getMentionSet();
		bool found_match = false;
		for (int p=0; p<propSet->getNPropositions(); ++p) {
			Proposition *prop = propSet->getProposition(p);
			for (int a=0; a<prop->getNArgs(); ++a) {
				Argument *arg = prop->getArg(a);
				if (arg->getType() == Argument::MENTION_ARG) {
					if (arg->getMentionIndex() == mention->getIndex()) {
						if (aop->roles.find(arg->getRoleSym()) != aop->roles.end()) {
							// are these values for fall_through_children and statusOverrides ok?
							PatternFeatureSet_ptr aopSfs = aopPattern->matchesProp(patternMatcher, sent_no, prop, false, PropStatusManager_ptr());
							if (aopSfs) {
								found_match = true;
								sfs->addFeatures(aopSfs);
							}
						}
					}
				}
			}
		}
		if (!found_match)
			return PatternFeatureSet_ptr();
	}
	// Initialize our score.
	sfs->setScore(this->getScore());

	return sfs;
}

PatternFeatureSet_ptr MentionPattern::matchesParseNode(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node) {
	// Walk over all Mentions in sentence, and try to match any 
	// Mention that resides on the node, or whose head resides on 
	// the node
	int start = node->getStartToken();
	int end = node->getEndToken();

	SentenceTheory *st = patternMatcher->getDocTheory()->getSentenceTheory(sent_no);
	const MentionSet *ms = st->getMentionSet();
	for (int i = 0; i < ms->getNMentions(); i++) {
		const Mention *ment = ms->getMention(i);
		const SynNode *mentNode = ment->getNode();
		const SynNode *mentHead = ment->getAtomicHead();

		if (mentNode == node || mentHead == node ||
			(mentNode->getStartToken() == start && mentNode->getEndToken() == end) ||
			(mentHead->getStartToken() == start && mentHead->getEndToken() == end))
		{
			PatternFeatureSet_ptr returnSet = matchesMention(patternMatcher, ment, false);
			if (returnSet != 0) {
				return returnSet;	
			}
		}
	}
	return PatternFeatureSet_ptr();
}

void MentionPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "MentionPattern: ";
	if (!getID().is_null()) out << getID();
	out << std::endl;
		
	if (_is_generic)
		out << "GENERIC\n";
	if (_is_specific)
		out << "SPECIFIC\n";
	if (_is_appositive)
		out << "APPOSITIVE\n";
	if (_requires_name)
		out << "REQUIRES NAME\n";
	if (_requires_name_or_desc)
		out << "REQUIRES NAME/DESC\n";

	if (_aceTypes.size() != 0) {		
		for (int i = 0; i < indent; i++) out << " ";
		out << "  acetypes = {";
		for (size_t j = 0; j < _aceTypes.size(); j++)
			out << (j==0?"":" ") << _aceTypes[j].getName();
		out << "}\n";	
	}
		
	if (_aceSubtypes.size() != 0) {		
		for (int i = 0; i < indent; i++) out << " ";
		out << "  acesubtypes = {";
		for (size_t j = 0; j < _aceSubtypes.size(); j++)
			out << (j==0?"":" ") << _aceSubtypes[j].getName();
		out << "}\n";	
	}

	if (_headwords.size() != 0 || _headwordPrefixes.size() != 0) {		
		for (int i = 0; i < indent; i++) out << " ";
		out << "  headwords = {";
		BOOST_FOREACH(Symbol sym, _headwords) {
			out << sym << " ";
		}
		BOOST_FOREACH(Symbol sym, _headwordPrefixes) {
			out << sym << "* ";
		}
		out << "}\n";	
	}	

	if (!_brownClusterConstraint.is_null()) {
		out << L"  brown cluster = {" << _brownClusterConstraint.to_string() << L"}\n";
	}

	if (_blockedHeadwords.size() != 0 || _blockedHeadwordPrefixes.size() != 0) {	
		for (int i = 0; i < indent; i++) out << " ";
		out << "  blocked headwords = {";
		BOOST_FOREACH(Symbol sym, _blockedHeadwords) {
			out << sym << " ";
		}
		BOOST_FOREACH(Symbol sym, _blockedHeadwordPrefixes) {
			out << sym << "* ";
		}
		out << "}\n";	
	}	
	
	if (_blockingEntityLabels.size() != 0) {		
		for (int i = 0; i < indent; i++) out << " ";
		out << "  blocking entity labels = {";
		for (size_t j = 0; j < _blockingEntityLabels.size(); j++)
			out << (j==0?"":" ") << _blockingEntityLabels[j];
		out << "}\n";	
	}
	
	if (_entityLabels.size() != 0) {		
		for (int i = 0; i < indent; i++) out << " ";
		out << "  entity labels = {";
		for (size_t j = 0; j < _entityLabels.size(); j++)
			out << (j==0?"":" ") << _entityLabels[j];
		out << "}\n";	
	}

	if (_propDefPattern) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  prop_def = {\n";
		_propDefPattern->dump(out, indent+4);		
		for (int i = 0; i < indent; i++) out << " ";
		out << "}\n";
	}

	BOOST_FOREACH(ArgOfPropConstraint_ptr constraint, _argOfPropConstraints) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  arg-of-prop";
		if (!constraint->roles.empty()) {
			out << " (role={";
			BOOST_FOREACH(Symbol sym, constraint->roles)
				out << sym << " ";
			out << "})";
		}
		out << " = {\n";
		constraint->propPattern->dump(out, indent+4);		
		for (int i = 0; i < indent; i++) out << " ";
		out << "}\n";
	}
}


Pattern_ptr MentionPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {
	replaceShortcut<RegexPattern>(_regexPattern, refPatterns);
	replaceShortcut<PropMatchingPattern>(_propDefPattern, refPatterns);
	BOOST_FOREACH(ArgOfPropConstraint_ptr aop, _argOfPropConstraints)
		replaceShortcut<PropMatchingPattern>(aop->propPattern, refPatterns);
	return shared_from_this();
}

void MentionPattern::getReturns(PatternReturnVecSeq & output) const {
	Pattern::getReturns(output);
	if (_regexPattern)
		_regexPattern->getReturns(output);
	if (_propDefPattern)
		_propDefPattern->getReturns(output);
	BOOST_FOREACH(ArgOfPropConstraint_ptr aop, _argOfPropConstraints)
		aop->propPattern->getReturns(output);
}


