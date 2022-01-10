// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/edt/ch_Guesser.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Chinese/common/ch_WordConstants.h"
#include "Chinese/parse/ch_STags.h"
#include "Generic/common/DebugStream.h"
#include "Generic/theories/EntityType.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/SimpleQueue.h"
#include "Generic/common/hash_set.h"
#include "Generic/common/ParamReader.h"

DebugStream & ChineseGuesser::_debugOut = DebugStream::referenceResolverStream;

Symbol ChineseGuesser::guessGender(const SynNode *node, const Mention *mention, std::set<Symbol> suspectedSurnames) {
	if(mention == NULL || mention->mentionType == Mention::LIST)
		return Guesser::UNKNOWN;
	else if(! NodeInfo::isOfNPKind(node))
		return Guesser::UNKNOWN;
	else if(mention->getEntityType().matchesORG())
		return Guesser::NEUTRAL;
	else if(mention->getEntityType().matchesPER())
		return guessPersonGender(node, mention);
	else if(mention->mentionType == Mention::PRON) {
		Symbol word = mention->getHead()->getSingleWord();
		if (word == ChineseWordConstants::HE ||
			word == ChineseWordConstants::HIS ||
			word == ChineseWordConstants::THEY_MASC ||
			word == ChineseWordConstants::THEIR_MASC)
			return Guesser::MASCULINE;
		else if(word == ChineseWordConstants::SHE ||
				word == ChineseWordConstants::HER ||
				word == ChineseWordConstants::THEY_FEM ||
				word == ChineseWordConstants::THEIR_FEM)
			return Guesser::FEMININE;
		else if(word == ChineseWordConstants::IT ||
				word == ChineseWordConstants::ITS ||
				word == ChineseWordConstants::THEY_INANIMATE ||
				word == ChineseWordConstants::THEIR_INANIMATE)
			return Guesser::NEUTRAL;
		else
			return Guesser::UNKNOWN;
	}
	else return Guesser::UNKNOWN;
}

Symbol ChineseGuesser::guessPersonGender(const SynNode *node, const Mention *mention) {
	SimpleQueue <const SynNode *> q;
	int nChildren = node->getNChildren();
	int i;
	for(i = 0; i < nChildren; i++) 
		//add to queue for breadth first search
		q.add(node->getChild(i));

	//now perform breadth first search
	while(!q.isEmpty()) {
		const SynNode *thisNode = q.remove();
		//if(thisNode->getTag() == ChineseSTags::NPPOS)
		//	continue;
		Symbol word = thisNode->getHeadWord();
		if (word == ChineseWordConstants::MASC_TITLE_1 ||
			word == ChineseWordConstants::MASC_TITLE_2 ||
			word == ChineseWordConstants::MASC_TITLE_3)
			return Guesser::MASCULINE;
		else if(word == ChineseWordConstants::FEM_TITLE_1 ||
				word == ChineseWordConstants::FEM_TITLE_2 ||
				word == ChineseWordConstants::FEM_TITLE_3 ||
				word == ChineseWordConstants::FEM_TITLE_4 ||
				word == ChineseWordConstants::FEM_TITLE_5)
			return Guesser::FEMININE;

		//visit all children
		if(!thisNode->isPreterminal()) {
			nChildren = thisNode->getNChildren();
			for(i=0; i<nChildren; i++)
				q.add(thisNode->getChild(i));
		}
	}//end while
	//if nothing specific found, return unknown
	return Guesser::UNKNOWN;
}


Symbol ChineseGuesser::guessType(const SynNode *node, const Mention *mention) {
	if(mention == NULL)
		return EntityType::getUndetType().getName();
	else if(mention->getEntityType().isRecognized())
		return mention->getEntityType().getName();
	else if(mention->mentionType == Mention::PRON) {
		Symbol word = mention->getHead()->getSingleWord();
		if (word == ChineseWordConstants::HE ||
			word == ChineseWordConstants::HIS ||
			word == ChineseWordConstants::SHE ||
			word == ChineseWordConstants::HER)
		{
			return EntityType::getPERType().getName();
		}
	}
	return EntityType::getUndetType().getName();
}

Symbol ChineseGuesser::guessNumber(const SynNode *node, const Mention *mention) {
	if(mention == NULL || mention->getHead() == NULL)
		return Guesser::UNKNOWN;
	
	Symbol tag = mention->getHead()->getHeadPreterm()->getTag();

	if(mention->mentionType == Mention::PRON) {
		Symbol word = mention->getHead()->getHeadWord();
		if (word == ChineseWordConstants::HE ||
			word == ChineseWordConstants::HIS ||
			word == ChineseWordConstants::SHE ||
			word == ChineseWordConstants::HER ||
			word == ChineseWordConstants::IT  ||
			word == ChineseWordConstants::ITS)
			return Guesser::SINGULAR;
		else if(word == ChineseWordConstants::THEY_INANIMATE ||
				word == ChineseWordConstants::THEY_MASC ||
				word == ChineseWordConstants::THEY_FEM ||
				word == ChineseWordConstants::THEIR_INANIMATE ||
				word == ChineseWordConstants::THEIR_MASC ||
				word == ChineseWordConstants::THEIR_FEM)
			return Guesser::PLURAL;
		else {
			_debugOut << "WARNING: Unforeseen pronoun: " << word.to_debug_string() << "\n";
			return Guesser::UNKNOWN; 
		}
	}
	else if(!NodeInfo::isOfNPKind(node)) {
		_debugOut << "WARNING: Node not of NP type\n";
		return Guesser::UNKNOWN;
	}
	else if(mention->mentionType == Mention::LIST)
		return Guesser::PLURAL;
	else return Guesser::UNKNOWN;
}
