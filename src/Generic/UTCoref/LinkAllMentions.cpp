// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <string>
#include <iostream>

#include <boost/foreach.hpp>

#include "Generic/theories/UTCoref.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/EntityType.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/edt/Guesser.h"

#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"

#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/xx_LinkAllMentions.h"
#include "Generic/UTCoref/UTObservation.h"
#include "Generic/UTCoref/AbstractClassifier.h"
#include "Generic/UTCoref/SVMClassifier.h"
#include "Generic/UTCoref/MaxentClassifier.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"


#include "Generic/UTCoref/UTWordLists.h"


Symbol LinkAllMentions::am(L"am");
Symbol LinkAllMentions::a_m_(L"a.m.");
Symbol LinkAllMentions::pm(L"pm");
Symbol LinkAllMentions::p_m_(L"p.m.");
Symbol LinkAllMentions::perc(L"%");
Symbol LinkAllMentions::CD(L"CD");
Symbol LinkAllMentions::female(L"Female");
Symbol LinkAllMentions::male(L"Male");
Symbol LinkAllMentions::neutral(L"Neutral");
Symbol LinkAllMentions::singular(L"Singular");
Symbol LinkAllMentions::plural(L"Plural");
Symbol LinkAllMentions::person(L"Person");
Symbol LinkAllMentions::organization(L"Organization");
Symbol LinkAllMentions::location(L"Location");
Symbol LinkAllMentions::date(L"Date");
Symbol LinkAllMentions::time(L"Time");
Symbol LinkAllMentions::percent(L"Percent");
Symbol LinkAllMentions::money(L"Money");
Symbol LinkAllMentions::object(L"Object");
Symbol LinkAllMentions::unknown(L"Unknown");
Symbol LinkAllMentions::org(L"ORG");
Symbol LinkAllMentions::per(L"PER");
Symbol LinkAllMentions::gpe(L"GPE");
Symbol LinkAllMentions::loc(L"LOC");
Symbol LinkAllMentions::dte(L"DATE");
Symbol LinkAllMentions::veh(L"VEH");
Symbol LinkAllMentions::fac(L"FAC");

// if semclass, gender, number, and name are too slow we could cache
// like we do in python land
LinkAllMentions::LinkAllMentions(const GrowableArray <Parse *> *parses,
											   const GrowableArray <NameTheory *> *names,
											   const GrowableArray <TokenSequence *> *tokens) :
   parses(parses), names(names), tokens(tokens), classifier(0)
{
   /* make the features go register themselves */
   UTFeatureType::initFeatures();

   utwl.load();
   Guesser::initialize();

   // tag set
   std::string tag_set_file = ParamReader::getRequiredParam("utcoref_tag_set_file");
   DTTagSet* tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);

   // features
   std::string features_file = ParamReader::getRequiredParam("utcoref_features_file");
   DTFeatureTypeSet* featureTypes = _new DTFeatureTypeSet(features_file.c_str(), UTFeatureType::modeltype);

   obs = _new UTObservation(static_cast<LinkAllMentions*>(this));

   if (ParamReader::getRequiredTrueFalseParam("utcoref_use_maxent")) {
	  classifier = _new MaxentClassifier(obs, tagSet, featureTypes);
   }
   else {
	  classifier = _new SVMClassifier(obs,tagSet,featureTypes);
   }
}

LinkAllMentions::~LinkAllMentions() {
   if (classifier) {
	  delete classifier;
   }
}

bool LinkAllMentions::could_be_date(const SynNode &node) {
   /* only call the one right above the bottom a date */
   if (!(node.getNChildren() == 1 && node.getChild(0)->isTerminal())) {
	  return false;
   }

   if (node.getParent() && could_be_date(*node.getParent())) {
	  return false;
   }
   wchar_t c;
   const wchar_t* nodeStr = node.getSingleWord().to_string();

   if (!nodeStr) { /* not single token */
	  return false;
   }

   /* a string could be a date if:
	*  - every character is in [-/0-9]
	*  - somwhere we have [0-9][-/][0-9]
	*/

   /* make sure all chars are in [-/0-9] */
   for (int i = 0; nodeStr[i] != L'\0' ; i++) {
	  c = nodeStr[i];
	  if (c != L'-' && c != L'/' && (c < L'0' || c > L'9')) {
		 return false;
	  }
   }

   /* find [0-9][-/][0-9] */
   if (nodeStr[0] == L'\0' || nodeStr[1] == L'\0') {
	  return false;
   }
   wchar_t c_a, c_b, c_c;
   for (int i = 2; nodeStr[i] != L'\0' ; i++) {
	  c_a = nodeStr[i-2]; c_b = nodeStr[i-1]; c_c = nodeStr[i];
	  if ((c_a >= L'0' && c_a <= L'9') &&
		  (c_b == L'-' || c_b == L'/') &&
		  (c_c >= L'0' && c_c <= L'9'))
	  {
		 return true;
	  }
   }

   return false;
}

/* is this the highest node eligible for coreference */
bool LinkAllMentions::is_linkable(const SynNode &node) const {
   if (node.getParent() &&
	   node.getParent()->getNChildren() == 1 /*unit production*/&&
	   simple_linkable(*node.getParent())) {
	  return false;
   }
   return simple_linkable(node);
}


void LinkAllMentions::init_linkable_nodes_helper(const SynNode &node) {

   if(is_linkable(node)) {
	  linkable_nodes.add(&node);
	  cache[&node] = UtcorefCache(_getName(node), _gender(node), _number(node), _semclass(node));
   }

   for (int i = 0 ; i < node.getNChildren() ; i++) {
	  init_linkable_nodes_helper(*node.getChild(i));
   }
}

/* precompute the list of linkable nodes */
void LinkAllMentions::init_linkable_nodes() {
   for (int i = 0 ; i < parses->length() ; i++) {
	  const Parse &parse = *(*parses)[i];
	  init_linkable_nodes_helper(*parse.getRoot());
   }
}


/* This is a debug function.  It just goes through all pairs of nodes
   and links them if they are an exact string match */
void LinkAllMentions::link_strmatch(UTCoref &corefs) const {
   for (int i = 0 ; i < linkable_nodes.length() ; i++) {
	  const SynNode &left_node = *linkable_nodes[i];

	  std::wstring left_node_str = left_node.toTextString();
	  for (int j = i+1 ; j < linkable_nodes.length() ; j++) {
		 const SynNode &right_node = *linkable_nodes[j];

		 std::wstring right_node_str = right_node.toTextString();
		 if (left_node_str == right_node_str) {
			corefs.addLink(left_node, right_node);
		 }
		 right_node_str.clear();
	  }
	  left_node_str.clear();
   }
}

/* return the lowest non-leaf node that has the same token span as node */
const SynNode *LinkAllMentions::move_down(const SynNode &node) {
   const SynNode *n = &node;
   while (n->getNChildren() == 1 && !n->getChild(0)->isTerminal()) {
	  n = n->getChild(0);
   }
   return n;
}

/* return the highest non-leaf node that has the same token span as node */
const SynNode *LinkAllMentions::move_up(const SynNode &node) {
   const SynNode *n = &node;
   while (n->getParent() != NULL && n->getParent()->getNChildren() == 1) {
	  n = n->getParent();
   }
   return n;
}



/*
 * if this node is not linkable but the parent would be, and this node
 * only has one sibling, and that sibling precedes it, then we might
 * give the parent node instead if:
 *  - the preceding node is THE
 *  - the preceding node is an honorific and we're tagged PER
 * if we do make this adjustment, we also adjust the name theory.
*/
const SynNode *LinkAllMentions::adjustForLinkability(const SynNode &node) const {
   if (!is_linkable(node)) {

	  if (node.getParent() && meetsMovementCriterion(*node.getParent())) {
		 std::ostringstream ostr;
		 ostr << "CorefReplacement: treating this as the corefed item\n"
				   << *node.getParent() << "\ninstead of the original\n"
				   << node << std::endl;
		 SessionLogger::info("SERIF") << ostr.str();
		 return node.getParent();
	  }
	  else {
		 std::ostringstream ostr;
		 ostr << "Corefed unlinkable node:\n" << node << std::endl;
		 if (node.getParent()) {
			ostr << "Parent:\n" << *node.getParent() << std::endl;
		 }
		 SessionLogger::info("SERIF") << ostr.str();
	  }
   }
   return &node;
}


bool LinkAllMentions::hasNamedAncestor(const SynNode &node) const {
   const SynNode &n = *move_up(node);

   const SynNode *parent = n.getParent();
   if (parent == NULL) {
	  return false;
   }

   if (!getName(*parent).is_null()) {
	  return true;
   }

   return hasNamedAncestor(*parent);
}

bool LinkAllMentions::isPunct(const SynNode &node) const {
   Symbol p = node.getSingleWord();

   const static Symbol hash(L"#");
   const static Symbol dollar(L"$");
   const static Symbol quote(L"\"");
   const static Symbol lsb(L"-LSB-");
   const static Symbol rsb(L"-RSB-");
   const static Symbol lrb(L"-LRB-");
   const static Symbol rrb(L"-RRB-");
   const static Symbol lcb(L"-LCB-");
   const static Symbol rcb(L"-RCB-");
   const static Symbol lsq(L"[");
   const static Symbol rsq(L"]");
   const static Symbol lpn(L"(");
   const static Symbol rpn(L")");
   const static Symbol lcr(L"{");
   const static Symbol rcr(L"}");
   const static Symbol squote(L"'");
   const static Symbol comma(L",");
   const static Symbol period(L".");
   const static Symbol colon(L":");
   const static Symbol backticks(L"``");
   const static Symbol ticks(L"''");
   const static Symbol punc(L"PUNC");
   const static Symbol numcom(L"NUMERIC_COMMA");

   return (p == hash || p == dollar || p == quote || p == lrb || p == rsb ||
		   p == lrb || p == lcb || p == rcb || p == lsq || p == rsq || p == lpn ||
		   p == lpn || p == rpn || p == lcr || p == rcr || p == squote ||
		   p == comma || p == period || p == colon || p == backticks ||
		   p == ticks || p == punc || p == numcom);
}


bool LinkAllMentions::onlyPunctBefore(const SynNode &node) const {
   const SynNode *t = node.getFirstTerminal();

   while ((t=t->getPrevTerminal()) != NULL) {
	  if (!isPunct(*t)) {
		 return false;
	  }
   }
   return true;
}

/* return the name type for node if there is one, otherwise Symbol() */
/* might pull the name type off a child if we have to
 * - that is adjustForLinkability run in reverse
 *
 * slowing this down is that we call hasNamedAncestor which calls
 *    getName and there are a lot of redundant calls back and forth.  We
 *    can add caching if this is too slow
 */

Symbol LinkAllMentions::getName(const SynNode &node) const {
   if (cache.find(&node) == cache.end()) {
	  return _getName(node);
   }
   return (*cache.find(&node)).second.name;
}

Symbol LinkAllMentions::_getName(const SynNode &node) const {
   if (could_be_date(node)) {
	  return Symbol(L"DATE");
   }

   EntityType et = getNameEntityType(node);

   if (et == EntityType::getOtherType()) {
	  if (node.getStartToken() != 0 &&
		  !onlyPunctBefore(node) &&
		  isCaps(node) &&
		  !hasNamedAncestor(node))
	  {
		 return Symbol(L"OTHER");
	  }
	  return Symbol();
   }
   return et.getName();
}

/* return otherType on failure */
EntityType LinkAllMentions::getNameEntityType(const SynNode &node) const {

   const SynNode &n = *move_down(node);

   if (meetsMovementCriterion(n)) {
	  EntityType child_name = getNameEntityType(*n.getChild(1));
	  if (child_name != EntityType::getOtherType()) {
		 return child_name;
	  }
   }

   int parse_number = lookupParseNumber(n, *parses);
   NameTheory &nt = *(*names)[parse_number];
   const SynNode *p = (*parses)[parse_number]->getRoot();

   for (int i = 0 ; i < nt.getNNameSpans() ; i++) {

	  const SynNode *name_node = p->getCoveringNodeFromTokenSpan(nt.getNameSpan(i)->start, nt.getNameSpan(i)->end);
	  if (!name_node) {
		 throw UnexpectedInputException("LinkAllMentions.cpp::getName",
										"Lookup of name entity failed");
	  }

	  /* we don't want leaves */
	  if (name_node->isTerminal()) {
		 name_node = name_node->getParent();
	  }

	  if (!name_node) {
		 throw UnexpectedInputException("LinkAllMentions.cpp::getName",
										"Terminal only tree?");
	  }

	  if (name_node == &n) {
		 return nt.getNameSpan(i)->type;
	  }
   }

   return EntityType::getOtherType();
}

/* i: right node's index into linkable_nodes
 * j: left node's index into linkable_node
 * returns: either j or some k less than j which refers to a node that
 * would be easier to learn from than j's
 * Details at create_examples */
int LinkAllMentions::smartgen(int i, int j, const UTCoref &corefs) const {

   if (isPronoun(*(linkable_nodes[i]))) { /* pronouns link to the most recent antecedent */
	  return j;
   }

   if (!getName(*linkable_nodes[i]).is_null()) {
	  for (int k = j ; k >= 0 ; k--) { /* consider previous nodes */
		 if (corefs.areLinked(*linkable_nodes[k], *linkable_nodes[j])) { /* consider nodes in this chain */
			if (!getName(*linkable_nodes[k]).is_null()) {
			   return k;
			}
		 }
	  }
   }

   for (int k = j ; k >= 0 ; k--) { /* consider previous nodes */
	  if (corefs.areLinked(*linkable_nodes[k], *linkable_nodes[j])) { /* consider nodes in this chain */
		 if (!isPronoun(*(linkable_nodes[k]))) { /* no pronouns */
			return k;
		 }
	  }
   }

   return j; /* no node is better than the j we started with */
}


/* Given gold corefs, either:
 *  - write to out_file in a format suitable for yamcha to train an svm
 *  - give the maxentClassifier stuff to train on
 *
 * For each linkable node right_node we consider in reverse order all
 * previous linkable nodes left_node. Create negative examples until
 * we get to a pair that should be linked, then we create a single
 * positive example and stop. We attempt to generate a positive
 * example for the pairing that we think is most informative or
 * easiest to learn from:
 *  - pronouns link to their immediate (rightmost) antecedent
 *  - if the right node has a name and some antecedent also has a
 *     name, link to the rightmost named antecedent
 *  - otherwise link to the rightmost non-pronoun antecedent
 */
void LinkAllMentions::create_examples(const UTCoref &corefs) {
   std::wstring feature_vec;
   bool are_linked;

   startTraining();

   for (int i = 0 ; i < linkable_nodes.length() ; i++) { /*consider each node in the document */
	  const SynNode &right_node = *linkable_nodes[i];
	  for (int j = i-1 ; j >= 0 ; j--) { /* consider each previous node */
		 are_linked = corefs.areLinked(*linkable_nodes[j], right_node);
		 if (are_linked) {
			j = smartgen(i, j, corefs);
		 }

		 const SynNode &left_node = *linkable_nodes[j];
		 trainOnExample(left_node, right_node, are_linked);

		 if(are_linked) {
			break; /* go on to the next rightNode */
		 }
	  }
   }

   classifier->finishTraining();
}


void LinkAllMentions::trainOnExample(const SynNode &leftNode, const SynNode &rightNode, bool are_linked) {
   obs->populate(leftNode, rightNode);
   classifier->trainOnExample(are_linked);
}

void LinkAllMentions::startTraining() {
	std::string model_file_name = ParamReader::getRequiredParam("utcoref_model_file");
	classifier->startTraining(model_file_name.c_str());
}

void LinkAllMentions::startClassifying() {
	std::string model_file_name = ParamReader::getRequiredParam("utcoref_model_file");
   classifier->startClassifying(model_file_name.c_str(), UTFeatureType::modeltype);
}

double LinkAllMentions::classifyExample(const SynNode &leftNode, const SynNode &rightNode) {
   obs->populate(leftNode, rightNode);
   return classifier->classifyExample();
}

/* For each linkable node right_node we consider in reverse order all
 * possible antecedents left_node.  We ask the classifier how to judge
 * each possible pairing, and if any rate above 0 we take the pairing
 * that gets the highest rating.
 */
void LinkAllMentions::decode(UTCoref &corefs) {
   startClassifying();
   for (int i = 0 ; i < linkable_nodes.length() ; i++) {
	  const SynNode &right_node = *linkable_nodes[i];
	  double best_score = -1.0;
	  double cur_score;
	  const SynNode* best_j = NULL;

	  for (int j = i-1 ; j >= 0 ; j--) {
		 const SynNode &left_node = *linkable_nodes[j];
		 if ((cur_score = classifyExample(left_node, right_node)) > best_score) {
			best_score = cur_score;
			best_j = &left_node;
		 }
	  }

	  if (best_score > 0) {
		 std::ostringstream ostr;
		 ostr << "Linked:\n  " << *best_j << "\n  " << right_node << std::endl;
		 SessionLogger::info("SERIF") << ostr.str();
		 corefs.addLink(*best_j, right_node);
	  }
   }
}

/* decode from docTheory into docTheory->utcoref */
void LinkAllMentions::prepareAndDecode(DocTheory *docTheory) {

   UTCoref* utcoref = _new UTCoref();

   GrowableArray <Parse *> parses;
   GrowableArray <NameTheory *> names;
   GrowableArray <TokenSequence *> tokens;

   for (int i = 0 ; i < docTheory->getNSentences() ; i++) {
      SentenceTheory *sentence = docTheory->getSentenceTheory(i);
	  parses.add(sentence->getPrimaryParse());
	  names.add(sentence->getNameTheory());
	  tokens.add(sentence->getTokenSequence());
   }

   LinkAllMentions* lam = LinkAllMentions::build(&parses, &names, &tokens);
   lam->init_linkable_nodes();
   lam->decode(*utcoref);

   docTheory->setUTCoref(utcoref);
}


/* What node has number node_number? */
const SynNode* LinkAllMentions::lookupNode(const SynNode &node, int &node_number) {
   /* do a depth first traversal of the tree, subtracting 1 from
	  node_number for each node we vist.  When node number is 0 we've
	  found our target */

   const SynNode* target_node;

   if (node.isTerminal()) {
	  return NULL;
   }

   if (node_number == 0) {
	  return &node;
   }

   node_number -= 1;

   for (int i = 0 ; i < node.getNChildren() ; i++) {
	  if ((target_node = lookupNode(*node.getChild(i), node_number)) != NULL) {
		 return target_node;
	  }
   }

   return NULL;
}

/* what is the number of this node? */
int LinkAllMentions::lookupNodeNumber(const SynNode &targetNode) {
   int node_number = 0;
   if (!lookupNodeNumberHelper(*getRoot(targetNode), targetNode, node_number)) {
	  return -1;
   }
   return node_number;
}

bool LinkAllMentions::lookupNodeNumberHelper(const SynNode &node, const SynNode &targetNode, int &node_number) {
   /* do a depth first traversal of the tree, adding 1 from
	  node_number for each node we vist.  When we visit targetNode,
	  node_number has the right node number.  Return true on success,
	  false on failure*/

   if (node.isTerminal()) {
	  return false;
   }

   if (&targetNode == &node) {
	  return true;
   }

   node_number += 1;

   for (int i = 0 ; i < node.getNChildren() ; i++) {
	  if (lookupNodeNumberHelper(*node.getChild(i), targetNode, node_number)) {
		 return true;
	  }
   }

   return false;
}

const SynNode *LinkAllMentions::getRoot(const SynNode &node) {
   if (node.getParent()) {
	  return getRoot(*node.getParent());
   }
   return &node;
}

/* which parse is this node in? */
int LinkAllMentions::lookupParseNumber(const SynNode &node, const GrowableArray <Parse *> &parses) {
   const SynNode *root = getRoot(node);
   for (int parse_no = 0; parse_no < parses.length() ; parse_no++) {
	  if (root == parses[parse_no]->getRoot()) {
		 return parse_no;
	  }
   }
   return -1;
}

bool LinkAllMentions::isPronoun(const SynNode &node) const {
   return WordConstants::isPronoun(node.getSingleWord());
}

bool LinkAllMentions::isDeterminer(const SynNode &node) const {
   return WordConstants::isDeterminer(node.getSingleWord());
}

void LinkAllMentions::mockupMention(const SynNode &node, Mention &mockMention) const {
   mockMention.node = &node;
   mockMention.setEntityType(getNameEntityType(node));
   if (isPronoun(node)) {
	  mockMention.mentionType = Mention::PRON;
   }
   else if (mockMention.getEntityType() != EntityType::getOtherType()) {
	  mockMention.mentionType = Mention::NAME;
   }
   else {
	  mockMention.mentionType = Mention::DESC;
   }
}


Symbol LinkAllMentions::gender(const SynNode &node) const {
   if (cache.find(&node) == cache.end()) {
	  return _gender(node);
   }
   return (*cache.find(&node)).second.gender;
}

Symbol LinkAllMentions::_gender(const SynNode &node) const {
   Symbol g;

   if ((g = simpleGender(node)) != unknown) {
	  return g;
   }

   if (!node.isPreterminal() && (g = gender(*node.getHead())) != unknown) {
	  return g;
   }

   /* try using semclass to get gender */

   Symbol s = semclass(node);

   if (s == female) {
	  return female;
   }
   if (s == male) {
	  return male;
   }
   if (s != unknown && s != person) {
	  return neutral;
   }
   return unknown;
}

Symbol LinkAllMentions::simpleGender(const SynNode &node) const {
   Mention mockMention;
   mockupMention(node, mockMention);

   Symbol g = Guesser::guessGender(&node, &mockMention);

   if (g == Symbol(L"MASCULINE")) {
	  return male;
   }
   else if (g == Symbol(L"FEMININE")) {
	  return female;
   }
   else if (g == Symbol(L"NEUTRAL")) {
	  return neutral;
   }
   else if (g == Symbol(L":UNKNOWN")) {
	  return unknown;
   }

   throw UnexpectedInputException("LinkAllMentions.cpp::simpleGender",
								  "Got an unidentified grammatical gender");
   return unknown;
}

Symbol LinkAllMentions::number(const SynNode &node) const {
   if (cache.find(&node) == cache.end()) {
	  return _number(node);
   }
   return (*cache.find(&node)).second.number;
}

Symbol LinkAllMentions::_number(const SynNode &node) const {
   Mention mockMention;
   mockupMention(node, mockMention);
   Symbol n = Guesser::guessNumber(&node, &mockMention);

   if (n == Symbol(L"SINGULAR")) {
	  return singular;
   }
   else if (n == Symbol(L"PLURAL")) {
	  return plural;
   }
   else if (n == Symbol(L":UNKNOWN")) {
	  return unknown;
   }

   throw UnexpectedInputException("LinkAllMentions.cpp::number",
                                  "Got an unidentified grammatical numberr");
   return unknown;
}

bool LinkAllMentions::allChildrenArePreterminals(const SynNode &node) const {
   if (node.isPreterminal()) {
	  return false;
   }

   for(int i = 0 ; i < node.getNChildren() ; i++) {
	  if (! node.getChild(i)->isPreterminal()) {
		 return false;
	  }
   }

   return true;
}

bool LinkAllMentions::isTime(const SynNode &node) const {

   if (!allChildrenArePreterminals(node)) {
	  return false;
   }

   Symbol last_tag = node.getLastTerminal()->getTag();
   if (last_tag == am ||
	   last_tag == a_m_ ||
	   last_tag == pm ||
	   last_tag == p_m_)
   {
	  return true;
   }

   const SynNode *head = node.getHeadPreterm();
   if (head->getTag() == CD) {
	  BOOST_FOREACH(const wchar_t &wc, head->toTextString()) {
		 if (wc == L':') {
			return true;
		 }
	  }
   }

   return false;
}

bool LinkAllMentions::isMoney(const SynNode &node) const {
   return node.isTerminal() && node.getTag() == Symbol(L"$");
}

bool LinkAllMentions::isPercent(const SynNode &node) const {

   if (!allChildrenArePreterminals(node)) {
      return false;
   }

   // skip first child
   for (int i = 1 ; i < node.getNChildren() ; i++) {
	  Symbol tag = node.getChild(i)->getTag();
	  Symbol word = node.getChild(i)->getChild(0)->getTag();
	  if (tag == CD && (word == perc || word == percent)) {
		 return true;
	  }
   }
   return false;
}

Symbol LinkAllMentions::semclass(const SynNode &node) const {
   if (cache.find(&node) == cache.end()) {
	  return _semclass(node);
   }
   return (*cache.find(&node)).second.semclass;
}

Symbol LinkAllMentions::_semclass(const SynNode &node) const {
   return _semclass_helper(*move_up(node));
}

Symbol LinkAllMentions::_semclass_helper(const SynNode &node) const {
   Symbol gender = simpleGender(node);

   if (gender == female) {
	  return female;
   }
   if (gender == male) {
	  return male;
   }

   Symbol name = getName(node);
   if (name == org || name == gpe) {
	  return organization;
   }
   if (name == loc) {
	  return location;
   }
   if (name == dte) {
	  return date;
   }
   if (name == veh || name == fac) {
	  return object;
   }
   if (isTime(node) ) {
	  return time;
   }
   if (isPercent(node)) {
	  return percent;
   }
   if (isMoney(node)) {
	  return money;
   }

   std::wstring s = node.toTextString();
   s.resize(s.length()-1); // delete trailing space
   for (size_t i = 0 ; i < s.length() ; i++) {
	  if (s[i] == L' ') {
		 s[i] = L'_';
	  }
   }

   if (utwl.isParent(s, L"female.n.02")) {
	  return female;
   }
   if (utwl.isParent(s, L"male.n.02")) {
	  return male;
   }
   if (utwl.isParent(s, L"person.n.01")) {
	  return person;
   }
   if (utwl.isParent(s, L"organization.n.01")) {
	  return organization;
   }
   if (utwl.isParent(s, L"location.n.01")) {
	  return location;
   }
   if (utwl.isParent(s, L"clock_time.n.01") ||
	   utwl.isParent(s, L"time_period.n.01") ||
	   utwl.isParent(s, L"time_interval.n.01"))
   {
	  return time;
   }
   if (utwl.isParent(s, L"date.n.01") ||
	   utwl.isParent(s, L"date.n.04") ||
	   utwl.isParent(s, L"date.n.06") ||
	   utwl.isParent(s, L"date.n.07"))
   {
	  return date;
   }
   if (utwl.isParent(s, L"money.n.01") ||
	   utwl.isParent(s, L"money.n.02") ||
	   utwl.isParent(s, L"monetary_unit.n.01"))
   {
	  return money;
   }
   if (utwl.isParent(s, L"percentage.n.01")) {
	  return percent;
   }
   if (utwl.isParent(s, L"object.n.01")) {
	  return object;
   }


   if (!node.isPreterminal()) {
	  return _semclass_helper(*node.getHead());
   }

   return unknown;
}

// from the muc scoring config file
// very rare ones removed
bool LinkAllMentions::is_premodifier(Symbol s) const {
   static Symbol sCO(L"co");
   static Symbol sCO_(L"co.");
   static Symbol sCOS(L"cos");
   static Symbol sCOS_(L"cos.");
   static Symbol sCOMPANY(L"company");
   static Symbol sCORP(L"corp");
   static Symbol sCORP_(L"corp.");
   static Symbol sCORPORATION(L"corporation");
   static Symbol sGP(L"gp");
   static Symbol sG_P_(L"g.p.");
   static Symbol sINC(L"inc");
   static Symbol sINCORPORATED(L"incorporated");
   static Symbol sLTD(L"ltd");
   static Symbol sLTD_(L"ltd.");
   static Symbol sLIMITED(L"limited");
   static Symbol sLP(L"lp");
   static Symbol sNL(L"nl");
   static Symbol sNPL(L"npl");
   static Symbol sPLC(L"plc");


   return (s == sCO || s == sCO_ || s == sCOS || s == sCOS_ || s == sCOMPANY || s == sCORP ||
		   s == sCORP_ || s == sCORPORATION || s == sGP || s == sG_P_ || s == sINC ||
		   s == sINCORPORATED || s == sLTD || s == sLTD_ || s == sLIMITED || s == sLP ||
		   s == sNL || s == sNPL || s == sPLC);
}

bool LinkAllMentions::isProper(const SynNode &node) const {
   static Symbol npp(L"NPP");
   static Symbol nnp(L"NNP");
   static Symbol nnps(L"NNPS");

   Symbol t = node.getTag();

   return t == nnp || t == npp || t == nnps || (!node.isPreterminal() && isProper(*node.getHead()));
}

bool LinkAllMentions::isDefinite(const SynNode &node) const {
   static Symbol the(L"the");
   return node.getFirstTerminal() && node.getFirstTerminal()->getTag() == the;
}

bool LinkAllMentions::isIndefinite(const SynNode &node) const {
   static Symbol a(L"a");
   static Symbol an(L"an");

   const SynNode *ft = node.getFirstTerminal();

   return ft && (ft->getTag() == a || ft->getTag() == an);
}


boost::shared_ptr<LinkAllMentions::Factory> &LinkAllMentions::_factory() {
	static boost::shared_ptr<LinkAllMentions::Factory> factory(new GenericLinkAllMentionsFactory());
	return factory;
}

