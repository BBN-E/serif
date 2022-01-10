// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_LINK_ALL_MENTIONS_H
#define EN_LINK_ALL_MENTIONS_H

#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/common/WordConstants.h"
#include "English/common/en_WordConstants.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "English/parse/en_STags.h"

static Symbol sPER(L"PER");

class EnglishLinkAllMentions : public LinkAllMentions {
private:
	friend class EnglishLinkAllMentionsFactory;

   bool meetsMovementCriterion(const SynNode &parent) const {
	  return (is_linkable(parent) &&
			  parent.getNChildren() == 2 &&
			  parent.getChild(0)->isPreterminal() &&
			  (parent.getChild(0)->getChild(0)->getTag() == EnglishWordConstants::THE ||
			   (getNameEntityType(*parent.getChild(1)).getName() == sPER &&
				WordConstants::isHonorificWord(parent.getChild(0)->getChild(0)->getTag()))));
   }

   bool simple_linkable(const SynNode &node) const {
	  Symbol tag = node.getTag();
	  return  (could_be_date(node) ||
			   tag == EnglishSTags::NP ||
			   tag == EnglishSTags::NPA ||
			   tag == EnglishSTags::NPP ||
			   tag == EnglishSTags::NPPRO ||
			   tag == EnglishSTags::PRPS ||
			   tag == EnglishSTags::DATE);
   }

   EnglishLinkAllMentions(const GrowableArray <Parse *> *parses,
				   const GrowableArray <NameTheory *> *names,
				   const GrowableArray <TokenSequence *> *tokens) : 
	  LinkAllMentions(parses,names,tokens) {}
};

class EnglishLinkAllMentionsFactory: public LinkAllMentions::Factory {
	virtual LinkAllMentions *build(const GrowableArray <Parse *> *parses,
			const GrowableArray <NameTheory *> *names,
			const GrowableArray <TokenSequence *> *tokens) { 
				return _new EnglishLinkAllMentions(parses,names,tokens);
	}
};


#endif
