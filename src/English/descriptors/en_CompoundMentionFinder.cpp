// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/descriptors/en_CompoundMentionFinder.h"
#include "Generic/CASerif/correctanswers/CorrectAnswers.h"
#include "Generic/CASerif/correctanswers/CorrectDocument.h"
#include "Generic/CASerif/correctanswers/CorrectEntity.h"
#include "Generic/CASerif/correctanswers/CorrectMention.h"
#include "Generic/common/SymbolListMap.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/common/ParamReader.h"
#include "English/parse/en_STags.h"
#include "Generic/theories/EntityType.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/WordConstants.h"
#include "English/common/en_WordConstants.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/common/SymbolListMap.h"
#include <string.h>
#include <boost/scoped_ptr.hpp>


using namespace std;

EnglishCompoundMentionFinder::EnglishCompoundMentionFinder() {
	initializeSymbols();
	_loadDescWordMap();
	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");
}

EnglishCompoundMentionFinder::~EnglishCompoundMentionFinder() {
	delete _descWordMap;
}

Mention *EnglishCompoundMentionFinder::findPartitiveWholeMention(
	MentionSet *mentionSet, Mention *baseMention)
{
	const SynNode *node = baseMention->node;

	// see if head node is considered a partitive headword
	if (!isPartitiveHeadWord(node->getHeadWord()))
		return 0;

	// find a PP child
	const SynNode *ppNode = 0;
	for (int i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = node->getChild(i);
		if (child->getTag() == EnglishSTags::PP) {
			if (ppNode != 0) {
				// no good -- only one pp allowed
				return 0;
			}
			else {
				ppNode = child;
			}
		}
	}
	if (ppNode == 0)
		return 0;

	if (ppNode->getHeadWord() != _SYM_of)
		return 0;

	const SynNode *lastChild = ppNode->getChild(ppNode->getNChildren() - 1);
	if (!lastChild->hasMention())
		return 0;

	return mentionSet->getMentionByNode(lastChild);
}

Mention **EnglishCompoundMentionFinder::findAppositiveMemberMentions(
	MentionSet *mentionSet, Mention *baseMention)
{
	const SynNode *node = baseMention->node;

	int n_ccs = 0, n_commas = 0, n_nps = 0, n_names = 0;
	int i;
	EntityType etype = EntityType::getUndetType();
	Symbol entID = Symbol();
	int num_typed_children = 0;
	for (i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = node->getChild(i);
		if (child->getTag() == EnglishSTags::CC)
			n_ccs++;
		else if (child->getTag() == EnglishSTags::COMMA)
			n_commas++;
		else if (child->hasMention()) {
			n_nps++;

			if (_use_correct_answers) {
				setCorrectMentionForNode(child);
				if (_correctMention != 0) {
					if (etype == EntityType::getUndetType())
						etype = *_correctEntity->getEntityType();
					else if (etype != *_correctEntity->getEntityType())
						return 0;
					if (_correctAnswers->usingCorrectCoref()) {
						if (entID.is_null())
							entID = _correctEntity->getAnnotationEntityID();
						else if (entID != _correctEntity->getAnnotationEntityID())
							return 0;
					}
					num_typed_children++;
				}
			}
			// find how many names. There may be more than one if this is a
			// list.
			n_names += _countNamesInMention(mentionSet->getMentionByNode(child));
		}
	}

	// correct answer games
	if (num_typed_children > 0 && num_typed_children != 2)
		return 0;
	else if (num_typed_children > 0)
		n_nps = num_typed_children;

	if (n_nps != 2 || n_commas == 0 || n_ccs > 0 || n_names > 1)
		return 0;

	int j = 0;
	for (i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = node->getChild(i);
		if (child->hasMention()) {
			if (_use_correct_answers) {
				if (num_typed_children > 0) {
					setCorrectMentionForNode(child);
					if (_correctMention == 0)
						continue;
				}
			}
			_mentionBuf[j++] = mentionSet->getMentionByNode(child);
			if (j == 2)
				break;
		}
	}
	_mentionBuf[j] = 0;

	if (j != 2)
		return 0;

	// Try to avoid "FOO, as well as BAR" as an appositive
	if ( _mentionBuf[1]->getNode()->getNChildren() > 0) {
		const SynNode *firstChildOfSecondNode = _mentionBuf[1]->getNode()->getChild(0);
		if (firstChildOfSecondNode && firstChildOfSecondNode->getTag() == EnglishSTags::CONJP && 
			firstChildOfSecondNode->getNChildren() == 3 && firstChildOfSecondNode->getChild(0)->getHeadWord() == EnglishWordConstants::AS &&
			firstChildOfSecondNode->getChild(1)->getHeadWord() == EnglishWordConstants::WELL)
		{	
			return 0;
		}
	}

	if (_use_correct_answers) {
		// we've already done our type checking
		return _mentionBuf;
	}

	// if there's a type clash,
	// if one mention is a pron, let it slide
	// if it's name-desc, let it slide (attempt to coerce later)
	// otherwise reject
	if (_mentionBuf[0]->getEntityType() != _mentionBuf[1]->getEntityType()) {
		if (_mentionBuf[0]->mentionType == Mention::PRON ||
			_mentionBuf[1]->mentionType == Mention::PRON)
			return _mentionBuf;
		if ((_mentionBuf[0]->mentionType == Mention::NAME &&
			_mentionBuf[1]->mentionType == Mention::DESC) ||
			(_mentionBuf[1]->mentionType == Mention::NAME &&
			_mentionBuf[0]->mentionType == Mention::DESC)) {
				return _mentionBuf;
			}

		else
			return 0;
	}
	else {
		return _mentionBuf;
	}
}

void EnglishCompoundMentionFinder::setCorrectMentionForNode(const SynNode *node) {
	_correctMention = 0;
	_correctEntity = 0;
	for (int i = 0; i < _correctDocument->getNEntities(); i++) {
		CorrectEntity *correctEntity = _correctDocument->getEntity(i);
		for (int j = 0; j < correctEntity->getNMentions(); j++) {
			CorrectMention *correctMention = correctEntity->getMention(j);
			if (correctMention->getSentenceNumber() != _sentence_number)
				continue;
			const SynNode *iter  = node;
			while (!iter->isTerminal()) {
				if (correctMention->getBestHeadSynNode() == iter) {
					_correctMention = correctMention;
					_correctEntity = correctEntity;
					return;
				}
				iter = iter->getHead();
			}
		}
	}
	return;
}

// NOTE: mentions can be modified here. It's one of the only places in this class!
void EnglishCompoundMentionFinder::coerceAppositiveMemberMentions(Mention** mentions)
{

	if (_use_correct_answers) {
		if (mentions[1]->mentionType == Mention::NAME && mentions[0]->mentionType == Mention::DESC) {
			mentions[0]->setEntityType(mentions[1]->getEntityType());
			mentions[0]->setEntitySubtype(mentions[1]->getEntitySubtype());
		}
		else if (mentions[0]->mentionType == Mention::NAME && mentions[1]->mentionType == Mention::DESC) {
			mentions[1]->setEntityType(mentions[0]->getEntityType());
			mentions[1]->setEntitySubtype(mentions[0]->getEntitySubtype());
		}
		return;
	}

	if (mentions[0]->getEntityType() == mentions[1]->getEntityType())
		return;
	// if one mention is a pron, coerce the other into its type
	// assume appositive sizes of 2
	if (mentions[0]->mentionType == Mention::PRON) {
		mentions[0]->setEntityType(mentions[1]->getEntityType());
		mentions[0]->setEntitySubtype(mentions[1]->getEntitySubtype());
	}
	else if (mentions[1]->mentionType == Mention::PRON) {
		mentions[1]->setEntityType(mentions[0]->getEntityType());
		mentions[1]->setEntitySubtype(mentions[0]->getEntitySubtype());
	}

	// if we have a name, desc pair,
	// if the type of the name is valid for the head of the desc, coerce
	// else nothing to do
	if (mentions[0]->mentionType == Mention::NAME && mentions[1]->mentionType == Mention::DESC) {
		int numTypes = 0;
		const Symbol* types = _descWordMap->lookup(mentions[1]->node->getHeadWord(), numTypes);
		int i;
		for (i = 0; i < numTypes; i++) {
			if (types[i] == mentions[0]->getEntityType().getName()) {
				mentions[1]->setEntityType(mentions[0]->getEntityType());
				mentions[1]->setEntitySubtype(mentions[0]->getEntitySubtype());
				return;
			}
		}
	}
	else if (mentions[1]->mentionType == Mention::NAME && mentions[0]->mentionType == Mention::DESC) {
		int numTypes = 0;
		const Symbol* types = _descWordMap->lookup(mentions[0]->node->getHeadWord(), numTypes);
		int i;
		for (i = 0; i < numTypes; i++) {
			if (types[i] == mentions[1]->getEntityType().getName()) {
				mentions[0]->setEntityType(mentions[1]->getEntityType());
				mentions[0]->setEntitySubtype(mentions[1]->getEntitySubtype());
				return;
			}
		}
	}
}


Mention **EnglishCompoundMentionFinder::findListMemberMentions(
	MentionSet *mentionSet, Mention *baseMention)
{
	const SynNode *node = baseMention->node;

	int i;

/* this attempt to handle list nesting is commented out because it caused
   inconsistency exceptions and probably didn't help significantly anyway

	// first, see if this node is really just the head of some other
	// NP, which should be classified instead
	if (node->getParent() != 0 &&
		node->getParent()->hasMention() &&
		node->getParent()->getHead() == node)
	{
		return 0;
	}

	// now see if this node has an NP head which is what we should
	// actually be classifying
	const SynNode *mentionChild = 0;
	for (i = 0; i < node->getNChildren(); i++) {
		if (node->getChild(i)->hasMention()) {
			if (mentionChild == 0) {
				mentionChild = node->getChild(i);
			}
			else {
				// found more than one mention, so there is no "only" mention
				mentionChild = 0;
				break;
			}
		}
	}
	if (mentionChild != 0)
		node = mentionChild;
*/
	int n_ccs = 0, n_commas = 0, n_nps = 0, n_hyphens = 0;
	for (i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = node->getChild(i);
		if (child->getTag() == EnglishSTags::CC || child->getTag() == EnglishSTags::CONJP)
			n_ccs++;
		else if (child->getTag() == EnglishSTags::COMMA)
			n_commas++;
		else if (child->hasMention())
			n_nps++;
		else if (child->getHeadWord() == EnglishWordConstants::_HYPHEN_ && child->getStartToken() == child->getEndToken())
			n_hyphens++;
	}

	// In "British - US relations" "British - US" should be considered a list
	if (n_hyphens == 1 && node->getNChildren() == 3 && 
		node->getChild(0)->hasMention() &&
		node->getChild(1)->getHeadWord() == EnglishWordConstants::_HYPHEN_ &&
		node->getChild(2)->hasMention())
	{
		Mention *mention1 = mentionSet->getMentionByNode(node->getChild(0));
		Mention *mention2 = mentionSet->getMentionByNode(node->getChild(2));
		if (mention1->getMentionType() == Mention::NAME &&
			mention2->getMentionType() == Mention::NAME &&
			MAX_RETURNED_MENTIONS >= 2)
		{
			_mentionBuf[0] = mention1;
			_mentionBuf[1] = mention2;
			_mentionBuf[2] = 0;
			return _mentionBuf;
		}
		
	}

	if (n_nps < 2 || (n_ccs + n_commas) == 0) 
		return 0;

	int j = 0;
	for (i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = node->getChild(i);
		if (child->hasMention()) {
			_mentionBuf[j++] = mentionSet->getMentionByNode(child);
			if (j == MAX_RETURNED_MENTIONS)
				break;
		}
	}
	_mentionBuf[j] = 0;

	// This is ugly, but if this is a city-state construction, then we
	// need to make sure it is *not* reported as a list
	if (j == 2 &&
		n_ccs == 0 &&
		n_nps == 2 &&
		n_commas > 0 &&
		_mentionBuf[0]->getMentionType() == Mention::NAME &&
		_mentionBuf[0]->getNode()->getTag() == EnglishSTags::NPP &&
		_mentionBuf[0]->getEntityType().matchesGPE() &&
		_mentionBuf[1]->getMentionType() == Mention::NAME &&
		_mentionBuf[1]->getNode()->getTag() == EnglishSTags::NPP &&
		_mentionBuf[1]->getEntityType().matchesGPE())
	{
		return 0;
	}

	return _mentionBuf;
}

Mention *EnglishCompoundMentionFinder::findNestedMention(MentionSet *mentionSet,
												  Mention *baseMention)
{
	const SynNode *base = baseMention->node;

	const SynNode *nestedMentionNode = 0;
	if (base->getNChildren() > 0)
		nestedMentionNode = base->getHead();
	if (nestedMentionNode != 0 && nestedMentionNode->hasMention()) {
		Mention *nestedMention = mentionSet->getMention(nestedMentionNode->getMentionIndex());

		// Don't go down through a LIST, APPO, or PART looking for a NPP
		Mention::Type mentType = nestedMention->getMentionType();
		if (mentType == Mention::LIST || mentType == Mention::APPO || mentType == Mention::PART || mentType == Mention::NEST)
			return nestedMention;

		const SynNode *twiceNestedMentionNode = 0;

		// now we see if there is a further nested mention
		// this can only be a name, and may not be followed by any nouns or
		// mentions (i.e., it has to be the head, and we don't want lists or
		// appositives).

		for (int i = 0; i < nestedMentionNode->getNChildren(); i++) {
			const SynNode *child = nestedMentionNode->getChild(i);

			if (twiceNestedMentionNode != 0 &&
				(child->hasMention() ||
				 NodeInfo::canBeNPHeadPreterm(child)))
			{
				// bail out because there's more than one mention here
				twiceNestedMentionNode = 0;
				break;
			}

			if (nestedMentionNode->getChild(i)->getTag() == EnglishSTags::NPP) {
				if (nestedMentionNode->getChild(i)->hasMention() &&
					mentionSet->getMention(nestedMentionNode->getChild(i)->getMentionIndex())->getMentionType() == Mention::NEST) 
				{
					// ignore nested mentions
				} else {
					twiceNestedMentionNode = nestedMentionNode->getChild(i);
				}
			}
		}

		if (twiceNestedMentionNode != 0 && twiceNestedMentionNode->hasMention())
			return mentionSet->getMention(twiceNestedMentionNode->getMentionIndex());
		else
			return nestedMention;
	}
	else {
		return 0;
	}
}


bool EnglishCompoundMentionFinder::isPartitiveHeadWord(Symbol sym) {
	for (int i = 0; i < _n_partitive_headwords; i++) {
		if (sym == _partitiveHeadwords[i])
			return true;
	}
	return false;
}


bool EnglishCompoundMentionFinder::isNumber(const SynNode *node) {
	static Symbol CD_sym(L"CD");

	if (node->isTerminal())
		return false;
	if (node->getTag() == CD_sym)
		return true;
	return isNumber(node->getHead());
}


Symbol EnglishCompoundMentionFinder::_SYM_of;

int EnglishCompoundMentionFinder::_n_partitive_headwords;
Symbol EnglishCompoundMentionFinder::_partitiveHeadwords[1000];

void EnglishCompoundMentionFinder::initializeSymbols() {
	_SYM_of = Symbol(L"of");

	// now read in partitive head-words
	std::string ph_list = ParamReader::getRequiredParam("partitive_headword_list");
	boost::scoped_ptr<UTF8InputStream> wordStream_scoped_ptr(UTF8InputStream::build(ph_list.c_str()));
	UTF8InputStream& wordStream(*wordStream_scoped_ptr);

	_n_partitive_headwords = 0;
	while (!wordStream.eof() && _n_partitive_headwords < 1000) {
		wchar_t line[100];
		wordStream.getLine(line, 100);
		if (line[0] != '\0')
			_partitiveHeadwords[_n_partitive_headwords++] = Symbol(line);
	}

	wordStream.close();
}

void EnglishCompoundMentionFinder::_loadDescWordMap()
{
	std::string buffer = ParamReader::getRequiredParam("desc_types");

	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& stream(*stream_scoped_ptr);
	stream.open(buffer.c_str());
	_descWordMap = _new SymbolListMap(stream);
	stream.close();
}

int EnglishCompoundMentionFinder::_countNamesInMention(Mention* ment)
{
	if (ment->getMentionType() == Mention::LIST ||
		ment->getMentionType() == Mention::APPO)
	{
		int numNames = 0;
		Mention* child = ment->getChild();
		while (child != 0) {
			numNames += _countNamesInMention(child);
			child = child->getNext();
		}
		return numNames;
	}
	else if (ment->getMentionType()	== Mention::NAME)
		return 1;
	else if (isHackedName(ment))
		return 1;
	else return 0;
}

bool EnglishCompoundMentionFinder::isHackedName(Mention *ment) {
	static int reduction = -1;
	static SymbolHash *table = 0;
	if (reduction == 0)
		return false;
	if (reduction == -1) {
		if (ParamReader::isParamTrue("reduce_special_names")) {
			reduction = 0;
			return false;
		}

		std::string reduce_file = ParamReader::getParam("reduction_names");
		if (!reduce_file.empty()) {
			boost::scoped_ptr<UTF8InputStream> fileStream_scoped_ptr(UTF8InputStream::build(reduce_file.c_str()));
			UTF8InputStream& fileStream(*fileStream_scoped_ptr);
			UTF8Token token;
			fileStream >> token;
			fileStream.close();
			
			wstring ws = token.chars();
			string s(ws.begin(), ws.end());
			s.assign(ws.begin(), ws.end());

			s = ParamReader::expand(s);

			boost::scoped_ptr<UTF8InputStream> namesStream_scoped_ptr(UTF8InputStream::build(s.c_str()));
			UTF8InputStream& namesStream(*namesStream_scoped_ptr);
			table = _new SymbolHash(100);
			while (!namesStream.eof()) {
				std::wstring str = L"";
				namesStream >> token;
				if (token.symValue() != SymbolConstants::leftParen)
					break;
				namesStream >> token;
				while (token.symValue() != SymbolConstants::rightParen) {
					str += token.chars();
					namesStream >> token;
					if (token.symValue() != SymbolConstants::rightParen)
						str += L" ";
				}
				wstring::size_type length = str.length();
				for (size_t i = 0; i < length; ++i) {
					str[i] = towlower(str[i]);
				}
				table->add(Symbol(str.c_str()));
			}
			namesStream.close();
		} else {
			reduction = 0;
			return false;
		}

		reduction = 1;
	}

	const SynNode *node = ment->getNode();
	for (int i = 0; i < node->getNChildren(); i++) {
		if (node->getChild(i)->getTag() == EnglishSTags::NPP) {
			std::wstring str = node->getChild(i)->toTextString();
			Symbol sym = Symbol(str.substr(0, str.length() - 1).c_str());
			if (table->lookup(sym))
				return true;
		}
	}
	return false;
}

