// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/trainers/FullAnnotatedParseReader.h"
#include "ParseNameReader.h"
#include <wchar.h>



Symbol ParseNameReader::toLower(Symbol s) {
   const wchar_t *old_str = s.to_string();
   int s_len = wcslen(old_str);

   wchar_t new_str[s_len+1 /* +1 for terminator */];
   for (int i = 0 ; i < s_len + 1 /* copy the terminator */; i++) {
	  new_str[i] = towlower(old_str[i]);
   }
   //std::cout << std::wstring(old_str) << " -> " << std::wstring(new_str) << std::endl;
   return Symbol(new_str);
}

/* the tags in the parse may be in the form
 *
 *   NPP::Name_ORG
 *
 * if so, turn the tag to
 *
 *   NPP
 *
 * and turn ORG into a NameSpan that can go in nameSpans
 *
 * Also, the input parse will have normal casing on the nodes, and we
 *  want everything lowecase.  So replace the normal case symbols with
 *  lower case, and put the normal case ones into a TokenSequence
 */
SynNode* ParseNameReader::tagsToNameSpans(const SynNode* parseNode,
										  SynNode* parent,
										  GrowableArray <NameSpan *> & nameSpans,
										  Token *token_arr[]) {

   wchar_t const *tag_str = parseNode->getTag().to_string();
   wchar_t *name_str;

   Symbol new_tag = parseNode->getTag();

   if (name_str = wcsstr(tag_str, L"::Name_")) {
	  /* this is the number of wchars between tag_str and name_str,
	     the length of the real portion of the tag */
	  int real_tag_len = name_str-tag_str;

	  /* replace the old tag 'NPP::Name_FOO' with the portion before
	     the double colon, 'NPP' */
	  new_tag = Symbol(tag_str, 0, real_tag_len);

	  name_str = &(name_str[7]); /* advance by length of L"::Name_" */
	  nameSpans.add(_new NameSpan(parseNode->getStartToken(),
								  parseNode->getEndToken(),
								  EntityType(Symbol(name_str))));
   }

   if (parseNode->isTerminal()) {
	  token_arr[parseNode->getStartToken()] = _new Token(0,0, new_tag);
	  new_tag = toLower(new_tag);
   }

   SynNode *new_node = _new SynNode(parseNode->getID(), parent, new_tag, parseNode->getNChildren());
   new_node->setTokenSpan(parseNode->getStartToken(), parseNode->getEndToken());
   for (int i = 0 ; i < parseNode->getNChildren() ; i++) {
	  new_node->setChild(i, ParseNameReader::tagsToNameSpans(parseNode->getChild(i),
															 /*parent=*/new_node,
															 nameSpans,
															 token_arr));
   }
   new_node->setHeadIndex(parseNode->getHeadIndex());
   return new_node;
}

/* given a file in pn_parse format, read it into name and parse theories */
void ParseNameReader::readAllParses(const char* file_name,
									GrowableArray <Parse *> & parses,
									GrowableArray <NameTheory *> & names,
									GrowableArray <TokenSequence *> & tokens){
   FullAnnotatedParseReader reader(file_name);
   reader.readAllParses(parses);

   NameTheory *sentence_names;
   TokenSequence *sentence_tokens;
   for(int i = 0 ; i < parses.length() ; i++) {
	  GrowableArray <NameSpan *> nameSpans;
	  int n_tokens = parses[i]->getRoot()->getNTerminals();
	  Token *token_arr[n_tokens];

	  /* because parses are mostly const we have to create a new Parse
	     object instead of just modifying the tags in the old parse */
	  Parse* old_parse = parses[i];
	  parses[i] = _new Parse(tagsToNameSpans(parses[i]->getRoot(), /*parent=*/0, nameSpans, token_arr), 0.0);
	  delete old_parse;

	  sentence_names = _new NameTheory();
	  sentence_names->n_name_spans = nameSpans.length();
	  sentence_names->nameSpans = _new NameSpan*[nameSpans.length()];
	  for (int j = 0 ; j < nameSpans.length() ; j++) {
		 sentence_names->nameSpans[j] = nameSpans[j];
	  }
	  names.add(sentence_names);

	  sentence_tokens = _new TokenSequence();
	  sentence_tokens->retokenize(n_tokens, token_arr);
	  tokens.add(sentence_tokens);
   }
}
