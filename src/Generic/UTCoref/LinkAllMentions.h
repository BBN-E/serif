// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

/* ---------------
 * LinkAllMentions
 * ---------------
 *
 * This code has two uses, training and decoding.  In training we
 * generate examples for use by yamcha in creating an svm model.  In
 * decoding we create a UTCoref theory from parses, names, and the
 * model.
 *
 *
 * The code only depends on getting three lists, one of Parse theories
 * one of NameTheories, and one of TokenSequences.  For the
 * convienience of calling from within running serif, though, there is
 * prepareAndDecode which takes a DocTheory and extracts the parse,
 * name, and token info.
 */

#ifndef LINK_ALL_MENTIONS_H
#define LINK_ALL_MENTIONS_H

#include <boost/shared_ptr.hpp>

#include <iostream>
#include <string>
#include "boost/unordered_map.hpp"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/NameSpan.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/UTCoref.h"
#include "Generic/UTCoref/UTWordLists.h"
#include "Generic/discTagger/DTFeature.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/maxent/MaxEntModel.h"

class DocTheory;
class LinkAllMentions;
class AbstractClassifier;
class EntityType;
class UTObservation;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

struct UtcorefCache {
	  Symbol name;
	  Symbol gender;
	  Symbol number;
	  Symbol semclass;

	  UtcorefCache() : name(Symbol()), gender(Symbol()), number(Symbol()), semclass(Symbol()) {}

	  UtcorefCache(Symbol name, Symbol gender, Symbol number, Symbol semclass) : 
		 name(name), gender(gender), number(number), semclass(semclass) {}		 
};

typedef boost::unordered_map< const SynNode *, UtcorefCache > UtcorefCacheMap;

class SERIF_EXPORTED LinkAllMentions {
public:
	/** Create and return a new LinkAllMentions. */
	static LinkAllMentions *build(const GrowableArray <Parse *> *parses,
			const GrowableArray <NameTheory *> *names,
			const GrowableArray <TokenSequence *> *tokens) { 
				return _factory()->build(parses,names,tokens); 
	}
	/** Hook for registering new LinkAllMentions factories */
	struct Factory { 
		virtual LinkAllMentions *build(const GrowableArray <Parse *> *parses,
			const GrowableArray <NameTheory *> *names,
			const GrowableArray <TokenSequence *> *tokens) = 0; 
	};
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

protected:
   /* after construction, call init_linkable_nodes */
   LinkAllMentions(const GrowableArray <Parse *> *parses,
						  const GrowableArray <NameTheory *> *names,
						  const GrowableArray <TokenSequence *> *tokens);
   GrowableArray <const SynNode *> linkable_nodes;
   UtcorefCacheMap cache;
   UTObservation* obs;
   AbstractClassifier *classifier;
public:
   virtual ~LinkAllMentions();
   const GrowableArray <Parse *> *parses;
   const GrowableArray <NameTheory *> *names;
   const GrowableArray <TokenSequence *> *tokens;
   UTWordLists utwl;
   void link_strmatch(UTCoref &corefs) const;
   void create_examples(const UTCoref &corefs);
   void decode(UTCoref &corefs);
   static const SynNode *lookupNode(const SynNode &node, int &node_number);
   static int lookupNodeNumber(const SynNode &targetNode);
   static bool lookupNodeNumberHelper(const SynNode &node, const SynNode &targetNode, int &node_number);
   static int lookupParseNumber(const SynNode &node, const GrowableArray <Parse *> & parses);
   static const SynNode *getRoot(const SynNode &node);
   bool is_linkable(const SynNode &node) const;
   const SynNode *adjustForLinkability(const SynNode &node) const;
   static void prepareAndDecode(DocTheory *docTheory);
   virtual bool meetsMovementCriterion(const SynNode &parent) const = 0;
   virtual bool simple_linkable(const SynNode &node) const = 0;
   void init_linkable_nodes();
   bool hasNamedAncestor(const SynNode &node) const;
   bool isCaps(const SynNode &node) const {
	  if (node.getStartToken() != node.getEndToken()) {
		 return false;
	  }
	  int parse_no = lookupParseNumber(node, *parses);

	  const wchar_t* tokstr = (*tokens)[parse_no]->getToken(node.getStartToken())->getSymbol().to_string();

	  return tokstr[0] >= L'A' && tokstr[0] <= L'Z';
   }
   std::wstring casedString(const SynNode &node) const {
	  int parse_no = lookupParseNumber(node, *parses);
	  if (parse_no != -1) {
		 return node.toCasedTextString((*tokens)[parse_no]);
	  }
	  return std::wstring();
   }
   Symbol semclass(const SynNode &node) const;
   Symbol gender(const SynNode &node) const;
   Symbol number(const SynNode &node) const;
   Symbol getName(const SynNode &node) const;

   bool isPronoun(const SynNode &node) const;
   bool isDeterminer(const SynNode &node) const;

   bool is_premodifier(Symbol s) const;
   bool isProper(const SynNode &node) const ;
   bool isIndefinite(const SynNode &node) const;
   bool isDefinite(const SynNode &node) const;
   static const SynNode *move_down(const SynNode &node);
   static const SynNode *move_up(const SynNode &node);

   static Symbol am;
   static Symbol a_m_;
   static Symbol pm;
   static Symbol p_m_;
   static Symbol CD;
   static Symbol perc;
   static Symbol female;
   static Symbol male;
   static Symbol person;
   static Symbol organization;
   static Symbol location;
   static Symbol date;
   static Symbol time;
   static Symbol percent;
   static Symbol money;
   static Symbol object;
   static Symbol unknown;
   static Symbol singular;
   static Symbol plural;
   static Symbol neutral;
   static Symbol org;
   static Symbol per;
   static Symbol gpe;
   static Symbol loc;
   static Symbol dte;
   static Symbol veh;
   static Symbol fac;

protected:
   void init_linkable_nodes_helper(const SynNode &node);
   static bool could_be_date(const SynNode &node);
   bool isPunct(const SynNode &node) const;
   bool onlyPunctBefore(const SynNode &node) const;
   int smartgen(int i, int j, const UTCoref &corefs) const;
   bool isMoney(const SynNode &node) const;
   bool isTime(const SynNode &node) const;
   bool isPercent(const SynNode &node) const;
   Symbol simpleGender(const SynNode &node) const;
   bool allChildrenArePreterminals(const SynNode &node) const;
   void mockupMention(const SynNode &node, Mention &mockMention) const;
   EntityType getNameEntityType(const SynNode &node) const;
   Symbol _semclass_helper(const SynNode &node) const;

   void trainOnExample(const SynNode &leftNode, const SynNode &rightNode, bool are_linked);
   void startTraining();
   void startClassifying();
   double classifyExample(const SynNode &leftNode, const SynNode &rightNode);

   Symbol _semclass(const SynNode &node) const;
   Symbol _gender(const SynNode &node) const;
   Symbol _number(const SynNode &node) const;
   Symbol _getName(const SynNode &node) const;
private:
	static boost::shared_ptr<Factory> &_factory();
};

//#ifdef ENGLISH_LANGUAGE
//    #include "English/UTCoref/en_LinkAllMentions.h"
//#else
//    #include "Generic/UTCoref/xx_LinkAllMentions.h"
//#endif

#endif
