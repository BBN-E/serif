// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <string.h>
#include <fstream>

#include "Generic/common/UnexpectedInputException.h"
#include "CorefReader.h"
#include "Generic/theories/UTCoref.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "English/common/en_WordConstants.h"
#define COREF_MAX_READ 1024


/* Given a token in the form <sentence_number>:<node_number>, parse
   out those two numbers. */
void CorefReader::interpretNumbers(int &sentence, int &node_number, char* token) {
   char *strtok_saveptr;
   char *sentence_str = strtok_r(token, ":", &strtok_saveptr);

   if (sentence_str == NULL) {
	  throw UnexpectedInputException("CorefReader.cpp::interpretMention",
                                     "invalid coref token");
   }
   sentence = atoi(sentence_str);

   char *node_number_str = strtok_r(NULL, ":", &strtok_saveptr);
   if (node_number_str == NULL or strtok_r(NULL, ":", &strtok_saveptr) != NULL) {
	  throw UnexpectedInputException("CorefReader.cpp::interpretNumbers",
                                     "invalid coref token postcolon part");
   }
   node_number = atoi(node_number_str);
}



/* Given a token in the form <sentence_number>:<node_number>, look up
 * in parses which SynNode is being referenced
 */
const SynNode* CorefReader::interpretMention(char* token, const GrowableArray <Parse *> & parses) {
   int sentence, node_number, working_node_number;

   interpretNumbers(sentence, node_number, token);

   if (sentence < 0 or sentence > parses.length()) {
	  throw UnexpectedInputException("CorefReader.cpp::interpretMention",
									 "invalid reference into parse sentence");
   }

   working_node_number = node_number;
   const SynNode* node = LinkAllMentions::lookupNode(*parses[sentence]->getRoot(), working_node_number);
   if (node == NULL) {
	  throw UnexpectedInputException("CorefReader.cpp::interpretMention",
                                     "invalid reference into parse nodes");
   }

   return node;
}



/* Given a line of a c_flat file, create an appropriate coref entity
   and add it to corefs */
void CorefReader::processLine(char* lineptr, LinkAllMentions &lam, UTCoref & corefs) {
   char *strtok_saveptr;

   char *firstToken = strtok_r(lineptr, " ", &strtok_saveptr);

   if (firstToken == NULL) {
	  return; // blank line
   }
   const SynNode &firstMention = *lam.adjustForLinkability(*interpretMention(firstToken, *lam.parses));

   char *subsequentToken;
   while ((subsequentToken=strtok_r(NULL, " ", &strtok_saveptr)) != NULL) {
	  const SynNode &subsequentMention = *lam.adjustForLinkability(*interpretMention(subsequentToken, *lam.parses));
	  corefs.addLink(firstMention, subsequentMention);
   }
}

/* Given a file in c_flat format, load it into corefs */
void CorefReader::readAllCorefs(const char *file_name, LinkAllMentions &lam, UTCoref & corefs) {
   std::ifstream inf;
   char line[COREF_MAX_READ];
   inf.open(file_name);

   if (!inf.is_open()) {
	  throw UnexpectedInputException("CorefReader.cpp::readAllCorefs",
                                     "coref file won't open");
   }

   for (int lineno = 0; inf.getline(line, COREF_MAX_READ) ; lineno++) {
	  processLine(line, lam, corefs);
   }

   inf.close();
}

/* write the contents of corefs out in c_flat format */
void CorefReader::writeAllCorefs(const char *file_name, const GrowableArray <Parse *> & parses, UTCoref & corefs) {
   std::ofstream outf;
   outf.open(file_name);
   if (!outf.is_open()) {
	  throw UnexpectedInputException("CorefReader.cpp::writeAllCorefs",
                                     "coref file won't open");
   }

   const GrowableArray< GrowableArray<const SynNode*> * >* entities = corefs.getEntities();
   const GrowableArray< const SynNode* > *entity;
   for (int i = 0 ; i < entities->length() ; i++) {
	  entity = (*entities)[i];

	  for (int j = 0 ; j < entity->length() ; j++) {
		 if (j != 0) {
			outf << " ";
		 }

		 outf << LinkAllMentions::lookupParseNumber(*(*entity)[j], parses) << ":";

		 /* calculate which node in that parse it is */
		 outf << LinkAllMentions::lookupNodeNumber(*(*entity)[j]);
	  }
	  outf << std::endl;
   }
}
