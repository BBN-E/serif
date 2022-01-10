// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/descriptors/ch_CompoundMentionFinder.h"
#include "Generic/CASerif/correctanswers/CorrectAnswers.h"
#include "Generic/CASerif/correctanswers/CorrectDocument.h"
#include "Generic/CASerif/correctanswers/CorrectEntity.h"
#include "Generic/CASerif/correctanswers/CorrectMention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/NodeInfo.h"
#include "Chinese/parse/ch_STags.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/SymbolListMap.h"
#include "Generic/common/WordConstants.h"
#include <boost/scoped_ptr.hpp>

ChineseCompoundMentionFinder::ChineseCompoundMentionFinder() {
	_loadDescWordMap();
	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");
}

ChineseCompoundMentionFinder::~ChineseCompoundMentionFinder() {
	delete _descWordMap;
}

Mention *ChineseCompoundMentionFinder::findPartitiveWholeMention(
	MentionSet *mentionSet, Mention *baseMention)
{
	const SynNode *node = baseMention->node;

	if (! WordConstants::isPartitiveWord(node->getHeadWord()))
		return 0;

	int n_nps = 0;
	const SynNode *firstMentionNode = 0;
	for (int i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = node->getChild(i);
		if (child->hasMention()) {
			if (firstMentionNode == 0)
				firstMentionNode = child;
			n_nps++;
		}
	}

	if (n_nps != 2 || firstMentionNode == 0)
		return 0;

	return mentionSet->getMentionByNode(firstMentionNode);

}

Mention **ChineseCompoundMentionFinder::findAppositiveMemberMentions(
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
		if (child->getTag() == ChineseSTags::CC)
			n_ccs++;
		else if (child->getTag() == ChineseSTags::PU && child->getSingleWord() == ChineseSTags::CH_COMMA)
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

	if (n_nps != 2 || n_ccs + n_commas > 0) //|| n_names > 1)
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

	// Rule out anything except non-NAME NAME pattern
	if (_mentionBuf[0]->mentionType == Mention::NAME ||
		_mentionBuf[1]->mentionType != Mention::NAME)
		return 0;

	if (_use_correct_answers) {
		// we've already done our type checking
		return _mentionBuf;
	}

	// if there's a type clash,
	// if it's desc-name, let it slide (attempt to coerce later)
	// otherwise reject
	if (_mentionBuf[0]->getEntityType() != _mentionBuf[1]->getEntityType()) {
 		if (_mentionBuf[1]->mentionType == Mention::NAME &&
			_mentionBuf[0]->mentionType == Mention::DESC)
			return _mentionBuf;
		else
			return 0;
	}
	else {
		return _mentionBuf;
	}

}

void ChineseCompoundMentionFinder::setCorrectMentionForNode(const SynNode *node) {
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
void ChineseCompoundMentionFinder::coerceAppositiveMemberMentions(Mention** mentions)
{

    if (_use_correct_answers) {
		if (mentions[1]->mentionType == Mention::NAME && mentions[0]->mentionType == Mention::DESC) {
			mentions[0]->setEntityType(mentions[1]->getEntityType());
			mentions[0]->setEntitySubtype(mentions[1]->getEntitySubtype());
		}
		return;
	}

	if (mentions[0]->getEntityType() == mentions[1]->getEntityType())
		return;

	// if we have a desc-name pair,
	// if the type of the name is valid for the head of the desc, coerce
	// else nothing to do
	if (mentions[1]->mentionType == Mention::NAME && mentions[0]->mentionType == Mention::DESC) {
		int numTypes = 0;
		const Symbol* types = _descWordMap->lookup(mentions[0]->node->getHeadWord(), numTypes);
		for (int i = 0; i < numTypes; i++) {
			if (types[i] == mentions[1]->getEntityType().getName()) {
				mentions[0]->setEntityType(mentions[1]->getEntityType());
				mentions[0]->setEntitySubtype(mentions[1]->getEntitySubtype());
				return;
			}
		}
	}
}


Mention **ChineseCompoundMentionFinder::findListMemberMentions(
	MentionSet *mentionSet, Mention *baseMention)
{
	const SynNode *node = baseMention->node;

	int n_ccs = 0, n_commas = 0, n_nps = 0;
	bool last_is_etc = false;
	int i;
	for (i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = node->getChild(i);
		if (child->getTag() == ChineseSTags::CC)
			n_ccs++;
		else if (child->getTag() == ChineseSTags::PU && child->getSingleWord() == ChineseSTags::CH_COMMA)
			n_commas++;
		else if (i == node->getNChildren() - 1 && child->getTag() == ChineseSTags::ETC)
			last_is_etc = true;
		else if (child->hasMention())
			n_nps++;
	}

	if (n_nps < 2 || ((n_ccs + n_commas) == 0 && !last_is_etc))
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

	return _mentionBuf;
}

Mention *ChineseCompoundMentionFinder::findNestedMention(MentionSet *mentionSet,
												  Mention *baseMention)
{
	const SynNode *base = baseMention->node;
	const SynNode *nestedMentionNode = 0;
	if (base->getNChildren() > 0)
		nestedMentionNode = base->getHead();
	if (nestedMentionNode != 0 && nestedMentionNode->hasMention())
		return mentionSet->getMention(nestedMentionNode->getMentionIndex());
	else
		return 0;
}

void ChineseCompoundMentionFinder::_loadDescWordMap() {
	std::string param_file = ParamReader::getRequiredParam("desc_types");
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& stream(*stream_scoped_ptr);
	stream.open(param_file.c_str());
	_descWordMap = _new SymbolListMap(stream);
	stream.close();
}

int ChineseCompoundMentionFinder::_countNamesInMention(Mention* ment)
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
	else return 0;
}


