// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/descriptors/es_CompoundMentionFinder.h"
#include "Generic/CASerif/correctanswers/CorrectAnswers.h"
#include "Generic/CASerif/correctanswers/CorrectDocument.h"
#include "Generic/CASerif/correctanswers/CorrectEntity.h"
#include "Generic/CASerif/correctanswers/CorrectMention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/common/ParamReader.h"
#include <boost/scoped_ptr.hpp>

#include "Spanish/parse/es_STags.h"


int SpanishCompoundMentionFinder::_n_partitive_headwords;
Symbol SpanishCompoundMentionFinder::_partitiveHeadwords[1000];

namespace {
	Symbol HYPHEN(L"-");

	// From EnglishCompoundMentionFinder:
	int _countNamesInMention(Mention* ment)
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

}

SpanishCompoundMentionFinder::SpanishCompoundMentionFinder()
{
	initializeSymbols();
	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");
}

SpanishCompoundMentionFinder::~SpanishCompoundMentionFinder() { 
}

//Not looking for compound mentions yet, so always return 0?

Mention *SpanishCompoundMentionFinder::findPartitiveWholeMention(
	MentionSet *mentionSet, Mention *baseMention) {
	return 0;
}

// This is adapted from the english version.
Mention **SpanishCompoundMentionFinder::findAppositiveMemberMentions(
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
		if (child->getTag() == SpanishSTags::POS_CC || child->getTag() == SpanishSTags::CONJ)
			n_ccs++;
		else if (child->getTag() == SpanishSTags::POS_FC)
			n_commas++;
		else if (child->hasMention()) {
			n_nps++;
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
			_mentionBuf[j++] = mentionSet->getMentionByNode(child);
			if (j == 2)
				break;
		}
	}
	_mentionBuf[j] = 0;

	if (j != 2) {
		return 0;
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

		else {
			return 0;
		}
	}
	else {
		//std::wcerr << L"  appos:" << node->toTextString() << std::endl;
		return _mentionBuf;
	}
}


// This is adapted from the english version.
Mention **SpanishCompoundMentionFinder::findListMemberMentions(
	MentionSet *mentionSet, Mention *baseMention)
{
	const SynNode *node = baseMention->node;

	// The Ancora parse contains constituents like this:
	//   (SN (GRUP.NOM (GRUP.NOM xxx) (CONJ y) (GRUP.NOM yyy)))
	// In this case, we want to mark the SN as a list because
	// otherwise it will end up classified as a DESC.
	bool child_of_unary_sn = ((node->getTag() == SpanishSTags::SN) && 
							  (node->getNChildren()==1));
	if (child_of_unary_sn) {
		node = node->getChild(0);
	}

	int n_ccs = 0, n_commas = 0, n_nps = 0, n_hyphens = 0;
	for (int i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = node->getChild(i);
		if (child->getTag() == SpanishSTags::POS_CC || child->getTag() == SpanishSTags::CONJ)
			n_ccs++;
		else if (child->getTag() == SpanishSTags::POS_FC)
			n_commas++;
		else if (child->hasMention())
			n_nps++;
		else if ((child->getHeadWord() == HYPHEN)
				 && child->getStartToken() == child->getEndToken())
			n_hyphens++;
	}

	// In "British - US relations" "British - US" should be considered a list
	if (n_hyphens == 1 && node->getNChildren() == 3 && 
		node->getChild(0)->hasMention() &&
		node->getChild(1)->getHeadWord() == HYPHEN &&
		node->getChild(2)->hasMention())
	{
		if (child_of_unary_sn) {
			_mentionBuf[0] = mentionSet->getMentionByNode(node);
			_mentionBuf[1] = 0;
			return _mentionBuf;
		}
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

	// It can't be a list unless it contains at least 2 NPs.
	if (n_nps < 2)
		return 0;
	// Require at least one conjunction, *or* at least 3 NPs.
	if ((n_ccs==0) || (n_nps<3))
		return 0;

	int j = 0;
	for (int i = 0; i < node->getNChildren(); i++) {
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
	if (j == 2 && n_ccs == 0 && n_nps == 2 && n_commas > 0 &&
		_mentionBuf[0]->getMentionType() == Mention::NAME &&
		_mentionBuf[0]->getNode()->getTag() == SpanishSTags::SNP &&
		_mentionBuf[0]->getEntityType().matchesGPE() &&
		_mentionBuf[1]->getMentionType() == Mention::NAME &&
		_mentionBuf[1]->getNode()->getTag() == SpanishSTags::SNP &&
		_mentionBuf[1]->getEntityType().matchesGPE()) {
		return 0;
	}

	if (child_of_unary_sn) {
		_mentionBuf[0] = mentionSet->getMentionByNode(node);
		_mentionBuf[1] = 0;
		return _mentionBuf;
	}

	return _mentionBuf;
}


Mention *SpanishCompoundMentionFinder::findNestedMention(
	MentionSet *mentionSet, Mention *baseMention)
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

void SpanishCompoundMentionFinder::coerceAppositiveMemberMentions(Mention **mentions){
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

	/** //We do not (yet) have _descWordMap for Spanish:
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
	*/
}

void SpanishCompoundMentionFinder::initializeSymbols(){
	_n_partitive_headwords = 0;
	if (!ParamReader::isParamTrue("find_partitive_mentions")) {
//		std::cout<<"***Don't Find Partitive Mentions"<<std::endl;
		return;	//since no partitive head words are iniailized, partitives will never be found!
	}

	// now read in partitive head-words
	std::string param_file = ParamReader::getRequiredParam("partitive_headword_list");
	boost::scoped_ptr<UTF8InputStream> wordStream_scoped_ptr(UTF8InputStream::build(param_file.c_str()));
	UTF8InputStream& wordStream(*wordStream_scoped_ptr);

	while (!wordStream.eof() && _n_partitive_headwords < 1000) {
		wchar_t line[100];
		wordStream.getLine(line, 100);
		if (line[0] != '\0'){
			//std::cout<<"Add Partitive Word: "<<Symbol(line).to_debug_string()<<std::endl;
			_partitiveHeadwords[_n_partitive_headwords++] = Symbol(line);
		}
	}

	wordStream.close();
}

bool SpanishCompoundMentionFinder::isPartitiveHeadWord(Symbol sym) {
	for (int i = 0; i < _n_partitive_headwords; i++) {
		if (sym == _partitiveHeadwords[i])
			return true;
	}
	return false;
}
bool SpanishCompoundMentionFinder::isNumber(const SynNode *node) {
	static Symbol CD_sym(L"NUMERIC");

	if (node->isTerminal())
		return false;
	if (node->getTag() == CD_sym)
		return true;
	return isNumber(node->getHead());
}

