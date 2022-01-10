// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

/* -----------
 * CorefReader
 * -----------
 *
 * This class is for loading UTCoref theories from simple text files.
 * The file format is:
 *
 *    4:35 6:11 7:52 9:29 14:41 15:21 16:34
 *    4:47 7:25
 *    ...
 *
 * Each line is a coref entity
 * Each token is a reference to a parse node in the format
 *   <sentence>:<nodenumber>
 * where the root is node 0 and each subsequent node in a preorder
 * traversal is one more than the previous:
 *
 *   (0 (1 (2) (3 4 5)) (6 7))
 *
 */

#ifndef COREF_READER_H
#define COREF_READER_H

#include "Generic/theories/Parse.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/theories/UTCoref.h"
#include "Generic/theories/SynNode.h"

class LinkAllMentions;

class CorefReader {
public:
   static void readAllCorefs(const char *file_name, LinkAllMentions &lam, UTCoref & corefs);
   static void writeAllCorefs(const char *file_name, const GrowableArray <Parse *> & parses, UTCoref & corefs);
private:
   static void interpretNumbers(int &sentence, int &node_number, char* token);
   static const SynNode *interpretMention(char* token, const GrowableArray <Parse *> & parses);
   static void processLine(char* lineptr, LinkAllMentions &lam, UTCoref & corefs);
};

#endif
