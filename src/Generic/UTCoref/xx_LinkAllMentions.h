// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_LINK_ALL_MENTIONS_H
#define XX_LINK_ALL_MENTIONS_H

#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/common/GrowableArray.h"

class GenericLinkAllMentions : public LinkAllMentions {
private:
	friend class GenericLinkAllMentionsFactory;

   bool meetsMovementCriterion(const SynNode &parent) const { return false; }
   bool simple_linkable(const SynNode &node) const { return false; }

   GenericLinkAllMentions(const GrowableArray <Parse *> *parses,
                   const GrowableArray <NameTheory *> *names,
				   const GrowableArray <TokenSequence *> *tokens) :
      LinkAllMentions(parses,names,tokens) {}
};

class GenericLinkAllMentionsFactory: public LinkAllMentions::Factory {
	virtual LinkAllMentions *build(const GrowableArray <Parse *> *parses,
			const GrowableArray <NameTheory *> *names,
			const GrowableArray <TokenSequence *> *tokens) { 
		return _new GenericLinkAllMentions(parses,names,tokens); 
	}
};



#endif
