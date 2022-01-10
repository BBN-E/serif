// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/patterns/CombinationPattern.h"
#include "Generic/patterns/ParseNodePattern.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/MentionPattern.h"
#include "Generic/patterns/ValueMentionPattern.h"
#include "Generic/patterns/RegexPattern.h"
#include "Generic/patterns/ShortcutPattern.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/TokenSpanPFeature.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/Sexp.h"

// Private symbols
namespace {
	Symbol parseNodeSym(L"parse-node");
	Symbol tagSym(L"tag");
	Symbol blockTagSym(L"block_tag");
	Symbol headwordSym(L"headword");
	Symbol blockHeadwordSym(L"block_headword");
	Symbol premodSym(L"premod");
	Symbol optionalPremodSym(L"opt_premod");
	Symbol headSym(L"head");
	Symbol postmodSym(L"postmod");
	Symbol optionalPostmodSym(L"opt_postmod");
	Symbol nodeMentionSym(L"node_mention");
	Symbol regexSym(L"regex");
}

ParseNodePattern::ParseNodePattern(Sexp *sexp, const Symbol::HashSet &entityLabels,
							   const PatternWordSetMap& wordSets)
{
	int nkids = sexp->getNumChildren();
	if (nkids < 2)
		throwError(sexp, "too few children in ParseNodePattern");
	Symbol patternTypeSym = sexp->getFirstChild()->getValue();
	if (patternTypeSym != parseNodeSym)
		throwError(sexp, "pattern type must be parse-node in ParseNodePattern");
	initializeFromSexp(sexp, entityLabels, wordSets);
}

bool ParseNodePattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (Pattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (constraintType == nodeMentionSym) {
		_mentionPattern = parseSexp(childSexp->getSecondChild(), entityLabels, wordSets);
	} else if (constraintType == tagSym) {
		for (int j = 1; j < childSexp->getNumChildren(); j++) {
			Sexp *tagSexp = childSexp->getNthChild(j);
			if (!tagSexp->isAtom()) throwError(childSexp, "tag must be atomic");
			addToVector(_tags, tagSexp->getValue(), wordSets);
		}
	} else if (constraintType == blockTagSym) {
		for (int j = 1; j < childSexp->getNumChildren(); j++) {
			Sexp *tagSexp = childSexp->getNthChild(j);
			if (!tagSexp->isAtom()) throwError(childSexp, "block_tag must be atomic");
			addToVector(_blockTags, tagSexp->getValue(), wordSets);
		}
	} else if (constraintType == headwordSym) {
		if ((childSexp->getNumChildren() - 1) > 0) {
			fillListOfWordsWithWildcards(childSexp, wordSets, _headwords, _headwordPrefixes);
		}
	} else if (constraintType == blockHeadwordSym) {
		if ((childSexp->getNumChildren() - 1) > 0) {
			fillListOfWordsWithWildcards(childSexp, wordSets, _blockedHeadwords, _blockedHeadwordPrefixes);
		}
	} else if (constraintType == premodSym) {
		for (int j = 1; j < childSexp->getNumChildren(); j++) {
			_premods.push_back(parseSexp(childSexp->getNthChild(j), entityLabels, wordSets));
		}
	} else if (constraintType == optionalPremodSym) {
		for (int j = 1; j < childSexp->getNumChildren(); j++) {
			_optionalPremods.push_back(parseSexp(childSexp->getNthChild(j), entityLabels, wordSets));
		}
	} else if (constraintType == headSym) {
		if (childSexp->getNumChildren() != 2) throwError(childSexp, "only one head pattern is allowed");
		_head = parseSexp(childSexp->getNthChild(1), entityLabels, wordSets);
	} else if (constraintType == postmodSym) {
		for (int j = 1; j < childSexp->getNumChildren(); j++) {
			_postmods.push_back(parseSexp(childSexp->getNthChild(j), entityLabels, wordSets));
		} 
	} else if (constraintType == optionalPostmodSym) {
		for (int j = 1; j < childSexp->getNumChildren(); j++) {
			_optionalPostmods.push_back(parseSexp(childSexp->getNthChild(j), entityLabels, wordSets));
		} 
	} else if (constraintType == regexSym) {
		if (_regexPattern)
			throwError(childSexp, "more than one regex in ParseNodePattern");
		_regexPattern = parseSexp(childSexp->getSecondChild(), entityLabels, wordSets);
		if (!(boost::dynamic_pointer_cast<RegexPattern>(_regexPattern) ||
			  boost::dynamic_pointer_cast<ShortcutPattern>(_regexPattern)))
			  throwError(childSexp, "Expected a regex pattern");
	} else {
		logFailureToInitializeFromChildSexp(childSexp);		
		return false;
	}
	return true;
}


PatternFeatureSet_ptr ParseNodePattern::matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) 
{ 
	Parse *parse = sTheory->getFullParse();
	const SynNode *top = parse->getRoot();

	return matchesSentenceRecursive(patternMatcher, sTheory->getTokenSequence()->getSentenceNumber(), top);
}

PatternFeatureSet_ptr ParseNodePattern::matchesSentenceRecursive(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node)
{
	PatternFeatureSet_ptr rv = matchesParseNode(patternMatcher, sent_no, node);
	if (rv != 0) 
		return rv;

	const SynNode* const* children = node->getChildren();
	for (int i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = children[i];
		PatternFeatureSet_ptr childRv = matchesSentenceRecursive(patternMatcher, sent_no, child);
		if (childRv != 0) 
			return childRv;
	}

	return PatternFeatureSet_ptr();
}

PatternFeatureSet_ptr ParseNodePattern::matchesParseNode(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node)
{
	if (node->isTerminal())
		return PatternFeatureSet_ptr();

	// Check for tags, blocked tags, head words, blocked head words
	BOOST_FOREACH(Symbol tag, _blockTags) {
		if (tag == node->getTag())	
			return PatternFeatureSet_ptr();
	}

	Symbol headword = node->getHeadWord();
	if (!wordMatchesWithPrefix(headword, _headwords, _headwordPrefixes, true))
		return PatternFeatureSet_ptr();	

	if (wordMatchesWithPrefix(headword, _blockedHeadwords, _blockedHeadwordPrefixes, false))
		return PatternFeatureSet_ptr();
	
	bool found_tag = (_tags.size() == 0);
	BOOST_FOREACH(Symbol tag, _tags) {
		if (tag == node->getTag())	
			found_tag = true;
	}
	if (!found_tag) 
		return PatternFeatureSet_ptr();

	// Create PatternFeatureSet for this node because we need 
	// a place to store the submatches. We will delete if 
	// the whole pattern doesn't match.
	PatternFeatureSet_ptr returnSet = boost::make_shared<PatternFeatureSet>();

	// MentionPattern match
	if (_mentionPattern) { 
		ParseNodeMatchingPattern_ptr mentionPattern = _mentionPattern->castTo<ParseNodeMatchingPattern>();
		PatternFeatureSet_ptr mentionFeatureSet = mentionPattern->matchesParseNode(patternMatcher, sent_no, node);
		if (mentionFeatureSet != 0) {
			returnSet->addFeatures(mentionFeatureSet);
		} else {
			return PatternFeatureSet_ptr();
		}
	}

	// Check to see if child patterns match

	// Child premod patterns
	bool found_all_premods = true;
	for (size_t h = 0; h < _premods.size(); h++) {
		Pattern_ptr pattern = _premods[h];
		ParseNodeMatchingPattern_ptr premodPattern = pattern->castTo<ParseNodeMatchingPattern>();

		// Walk over premods of node to see if we can match premodPattern
		bool found_premod = false;
		for (int i = 0; i < node->getHeadIndex(); i++) {
			const SynNode *premodNode = node->getChild(i);

			PatternFeatureSet_ptr premodSet =  premodPattern->matchesParseNode(patternMatcher, sent_no, premodNode);
			if (premodSet != 0) {
				found_premod = true;
				returnSet->addFeatures(premodSet);
				break;
			}
		}
		if (!found_premod) {
			found_all_premods = false;
			break;
		}
	}
	if (!found_all_premods) {
		return PatternFeatureSet_ptr();
	}

	for (size_t h = 0; h < _optionalPremods.size(); h++) {
		Pattern_ptr pattern = _optionalPremods[h];
		ParseNodeMatchingPattern_ptr premodPattern = pattern->castTo<ParseNodeMatchingPattern>();

		// Walk over premods of node to see if we can match premodPattern
		for (int i = 0; i < node->getHeadIndex(); i++) {
			const SynNode *premodNode = node->getChild(i);
			PatternFeatureSet_ptr premodSet =  premodPattern->matchesParseNode(patternMatcher, sent_no, premodNode);
			if (premodSet != 0) {
				returnSet->addFeatures(premodSet);
				break;
			}
		}
	}

	// Head pattern
	if (_head != 0) {
		ParseNodeMatchingPattern_ptr headPattern = _head->castTo<ParseNodeMatchingPattern>();
		const SynNode *headNode = node->getHead();
		PatternFeatureSet_ptr headSet = headPattern->matchesParseNode(patternMatcher, sent_no, headNode);

		if (headSet != 0) {
			returnSet->addFeatures(headSet);
		} else {
			return PatternFeatureSet_ptr();
		}
	}

	// Child postmod patterns
	bool found_all_postmods = true;
	for (size_t h = 0; h < _postmods.size(); h++) {
		Pattern_ptr pattern = _postmods[h];
		ParseNodeMatchingPattern_ptr postmodPattern = pattern->castTo<ParseNodeMatchingPattern>();

		// Walk over postmods of node to see if we can match postmodPattern
		bool found_postmod = false;
		for (int i = node->getHeadIndex() + 1; i < node->getNChildren(); i++) {
			const SynNode *postmodNode = node->getChild(i);
			PatternFeatureSet_ptr postmodSet = postmodPattern->matchesParseNode(patternMatcher, sent_no, postmodNode);
			if (postmodSet != 0) {
				found_postmod = true;
				returnSet->addFeatures(postmodSet);
				break;
			}
		}
		if (!found_postmod) {
			found_all_postmods = false;
			break;
		}
	}
	if (!found_all_postmods) {
		return PatternFeatureSet_ptr();
	}
	for (size_t h = 0; h < _optionalPostmods.size(); h++) {
		Pattern_ptr pattern = _optionalPostmods[h];
		ParseNodeMatchingPattern_ptr postmodPattern = pattern->castTo<ParseNodeMatchingPattern>();

		// Walk over postmods of node to see if we can match postmodPattern
		for (int i = node->getHeadIndex() + 1; i < node->getNChildren(); i++) {
			const SynNode *postmodNode = node->getChild(i);
			PatternFeatureSet_ptr postmodSet = postmodPattern->matchesParseNode(patternMatcher, sent_no, postmodNode);
			if (postmodSet != 0) {
				returnSet->addFeatures(postmodSet);
				break;
			}
		}
	}

	// Regex constraint
	if (_regexPattern) {
		RegexPattern_ptr regexPattern = _regexPattern->castTo<RegexPattern>();
		PatternFeatureSet_ptr regexSfs = regexPattern->matchesParseNode(patternMatcher, sent_no, node);
		if (regexSfs)
			returnSet->addFeatures(regexSfs);
		else
			return PatternFeatureSet_ptr();
	}
	
	const SentenceTheory *st = patternMatcher->getDocTheory()->getSentenceTheory(sent_no);
	TokenSequence *ts = st->getTokenSequence();
	int start_token = node->getStartToken();
	int end_token = node->getEndToken();
	// Add a feature for the return value (if we have one)
	if (getReturn()) {
		returnSet->addFeature(boost::make_shared<TokenSpanReturnPFeature>(
			shared_from_this(), sent_no, start_token, end_token, patternMatcher->getActiveLanguageVariant()));
		returnSet->addFeature(boost::make_shared<ParseNodeReturnPFeature>(
			shared_from_this(), node, sent_no, patternMatcher->getActiveLanguageVariant()));
	}
	// Add a feature for the pattern itself.
	returnSet->addFeature(boost::make_shared<TokenSpanPFeature>(sent_no, start_token, end_token, patternMatcher->getActiveLanguageVariant()));
	// Add a feature for the ID (if we have one)
	addID(returnSet);
	// Set the feature set's scores.
	returnSet->setScore(getScore());

	return returnSet;
}

// multiMatchesSentence is untested on ParseNodePattern!
std::vector<PatternFeatureSet_ptr> ParseNodePattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) 
{ 
	_multiMatchReturnVector.clear();

	Parse *parse = sTheory->getFullParse();
	const SynNode *top = parse->getRoot();

	// This fills up _multiMatchReturnVector
	multiMatchesSentenceRecursive(patternMatcher, sTheory->getTokenSequence()->getSentenceNumber(), top);

	return _multiMatchReturnVector;
}
void ParseNodePattern::multiMatchesSentenceRecursive(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node)
{
	PatternFeatureSet_ptr rv = matchesParseNode(patternMatcher, sent_no, node);
	if (rv != 0) 
		_multiMatchReturnVector.push_back(rv);

	const SynNode* const* children = node->getChildren();
	for (int i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = children[i];
		multiMatchesSentenceRecursive(patternMatcher, sent_no, child);
	}
}

Pattern_ptr ParseNodePattern::replaceShortcuts(const SymbolToPatternMap &refPatterns)
{
	for (size_t i = 0; i < _premods.size(); i++) {
		replaceShortcut<Pattern>(_premods[i], refPatterns);
		checkPatternType(_premods[i]);
	}

	for (size_t i = 0; i < _optionalPremods.size(); i++) {
		replaceShortcut<Pattern>(_optionalPremods[i], refPatterns);
		checkPatternType(_optionalPremods[i]);
	}

	if (_head) {
		replaceShortcut<Pattern>(_head, refPatterns);
		checkPatternType(_head);
	}

	for (size_t i = 0; i < _postmods.size(); i++) {
		replaceShortcut<Pattern>(_postmods[i], refPatterns);
		checkPatternType(_postmods[i]);
	}
	
	for (size_t i = 0; i < _optionalPostmods.size(); i++) {
		replaceShortcut<Pattern>(_optionalPostmods[i], refPatterns);
		checkPatternType(_optionalPostmods[i]);
	}

	replaceShortcut<MentionMatchingPattern>(_mentionPattern, refPatterns);

	replaceShortcut<RegexPattern>(_regexPattern, refPatterns);

	return shared_from_this();
}

void ParseNodePattern::checkPatternType(Pattern_ptr pattern) const
{
	if ((!boost::dynamic_pointer_cast<MentionPattern>(pattern)) &&
		(!boost::dynamic_pointer_cast<ValueMentionPattern>(pattern)) &&
		(!boost::dynamic_pointer_cast<ParseNodePattern>(pattern)) &&
		(!boost::dynamic_pointer_cast<MentionCombinationPattern>(pattern)) &&
		//(!boost::dynamic_pointer_cast<ValueMentionCombinationPattern>(pattern)) &&
		(!boost::dynamic_pointer_cast<ParseNodeCombinationPattern>(pattern)))
	{
		std::ostringstream ostr;
		pattern->dump(ostr);
		SessionLogger::err("SERIF") << "Invalid ParseNode subpattern: \n" << ostr.str();
		throw UnexpectedInputException("ParseNodePattern::checkPatternType", 
			"subpatterns of ParseNodePattern must be MentionPatterns, ValueMentionPatterns, or ParseNodePatterns");
	}
}


void ParseNodePattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "ParseNodePattern:";
	if (!getID().is_null()) out << getID();
	out << std::endl;

	if (_tags.size() != 0) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  tags = {";
		for (size_t j = 0; j < _tags.size(); j++)
			out << (j==0?"":" ") << _tags[j].to_debug_string();
		out << "}\n";
	}	
	if (_blockTags.size() != 0) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  blocked tags = {";
		for (size_t j = 0; j < _blockTags.size(); j++)
			out << (j==0?"":" ") << _blockTags[j].to_debug_string();
		out << "}\n";
	}
	if (_headwords.size() != 0 || _headwords.size() != 0) {		
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
	if (_premods.size() != 0) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  premods:\n";
		for (size_t j = 0; j < _premods.size(); j++) {
			_premods[j]->dump(out, indent + 4);
		}
	}
	if (_optionalPremods.size() != 0) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  optional premods:\n";
		for (size_t j = 0; j < _optionalPremods.size(); j++) {
			_optionalPremods[j]->dump(out, indent + 4);
		}
	}

	if (_head) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  head:\n";
		_head->dump(out, indent + 4);
	}

	if (_postmods.size() != 0) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  postmods:\n";
		for (size_t j = 0; j < _postmods.size(); j++) {
			_postmods[j]->dump(out, indent + 4);
		}
	}
	
	if (_optionalPostmods.size() != 0) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  optional postmods:\n";
		for (size_t j = 0; j < _optionalPostmods.size(); j++) {
			_optionalPostmods[j]->dump(out, indent + 4);
		}
	}
}

void ParseNodePattern::addToVector(std::vector<Symbol> &wordList, Symbol wordOrWordSet, const PatternWordSetMap& wordSets)
{
	PatternWordSetMap::const_iterator it = wordSets.find(wordOrWordSet);
	if (it != wordSets.end()) {
		const PatternWordSet_ptr wordSet = (*it).second;
		for (int word=0; word<wordSet->getNWords(); ++word)
			wordList.push_back(wordSet->getNthWord(word));
	} else {
		wordList.push_back(wordOrWordSet);
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
void ParseNodePattern::getReturns(PatternReturnVecSeq & output) const {
	Pattern::getReturns(output);
	for (size_t n = 0; n < _premods.size(); ++n)
		_premods[n]->getReturns(output);	
	for (size_t n = 0; n < _optionalPremods.size(); ++n)
		_optionalPremods[n]->getReturns(output);
	for (size_t n = 0; n < _postmods.size(); ++n)
		_postmods[n]->getReturns(output);	
	for (size_t n = 0; n < _optionalPostmods.size(); ++n)
		_optionalPostmods[n]->getReturns(output);
	if (_head)
		_head->getReturns(output);
	if (_mentionPattern)
		_mentionPattern->getReturns(output);
	if (_regexPattern)
		_regexPattern->getReturns(output);
}

