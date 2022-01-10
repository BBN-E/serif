// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HOBBSDISTANCE_H
#define HOBBSDISTANCE_H

#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"
#include "Generic/common/DebugStream.h"

#include <vector>

class HobbsDistance {
public:

	struct SearchResult {
		const SynNode *node;
		int sentence_number;
	};
	
	static void initialize();
	static int getCandidates(const SynNode *pronNode, std::vector<const Parse *> &prevSentences, 
							SearchResult results[], int max_results);

protected:

	static const int BEAM_WIDTH;
	static const int MAX_PATH;
	// count NPs under (and including) contents of initial_beam by BFS, stop if 
	// goal found.
	static int hobbsBFS(const SynNode **initial_beam, int initial_beam_size, SearchResult results[], 
		int nResults, int max_results, int sentence_num, bool limited = false /*don't go past NP's or S's*/);

	// count NPs of sentence under given root, stop if goal found
	static int hobbsBFS(const SynNode *root, SearchResult results[], int nResults, int max_results, int sentence_num);

	enum search_direction_t {SEARCH_TO_LEFT, SEARCH_TO_RIGHT};

	// count NPs to a particular side of the path from start to X, stop if goal
	// found.
	static int hobbsBFSAlongPath(const SynNode *start, const SynNode *X, search_direction_t direction, 
		SearchResult results[], int nResults, int max_results, int sentence_num, bool limited = false /* don't go past NP's or S's */);

	static void listChildrenToLeftOf(const SynNode **buf, int *n_listed, 
		int buf_size, const SynNode *subtree, const SynNode *child);
	static void listChildrenToRightOf(const SynNode **buf, int *n_listed, 
		int buf_size, const SynNode *subtree, const SynNode *child);

	// list premods
//	static void listPremods(const SynNode **buf, int *n_listed, int buf_size,
//	                        const SynNode *node);


	// count NPs in a sentence starting at root
	/*
	static int countNPs(const SynNode *root);
	static int countNPsRecursively(const SynNode *node);
	//static int countNPsOfType(const SynNode *root, entity_type_t etype);
	//static int countNPsOfTypeRecursively(const SynNode *node, entity_type_t etype);
	*/

	// some quick functions for abstracting relevant constituent classes from
	// their labels
	static bool hobbsIsSentence(const SynNode *node);
	static bool hobbsIsNounPhrase(const SynNode *node);

	static bool hobbsIsAtTop(const SynNode *node);
	
	static bool _do_step_8;
	static DebugStream _debugOut;
};


#endif
