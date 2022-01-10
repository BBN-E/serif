// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/HobbsDistance.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/common/ParamReader.h"

const int HobbsDistance::BEAM_WIDTH = 100;
const int HobbsDistance::MAX_PATH = 20;
DebugStream HobbsDistance::_debugOut;
bool HobbsDistance::_do_step_8;

inline int min(int a, int b) {return a>b ? b : a;}

void HobbsDistance::initialize() {
	_do_step_8 = true;

	std::string hobbs8 = ParamReader::getParam("hobbs_do_step_8");
	if (hobbs8 == "false")
		_do_step_8 = false;

	_debugOut.init(Symbol(L"hobbs_distance_debug_file"), false);
}

// Note: For info about slight differences from Hobbs' original algorithm, 
// read comments about each step in getCandidates().
int HobbsDistance::getCandidates(const SynNode *pronNode, 
								 std::vector<const Parse *> &prevSentences, 
								 SearchResult results[], 
								 int max_results)
{
	// nodeX is where p ends, like in the paper. nodeY is where p starts.
	const SynNode *nodeX, *nodeY;
	int nResults = 0;

	/*
	if(hobbsIsNounPhrase(pronNode))
		_debugOut << "YES: " << pronNode->toFlatString();
	else
		_debugOut << "  NO: " << pronNode->toFlatString() << " *** " << pronNode->getParent()->toFlatString();
	_debugOut << "\n";
	*/
	// step (1): go to immediately dominating NP
	nodeY = pronNode; // for us, that's the starting point (pronNode)

	if (_debugOut.isActive())
		_debugOut << "\n\nEND STEP 1:  START = " << nodeY->toFlatString() << "\n";
	/*
	while (!hobbsIsNounPhrase(nodeY)) {
		nodeY = nodeY->getParent();
		if (nodeY == 0)
			return 0;
	}
	*/

	// step (2): find node X
	// Once in a while we can have the NP (pronNode) be at the very top of a
	// sentence. In that case, we just set nodeX to nodeY, so that effectively
	// our path has length 0, so no more NP nodes are found in this sentence,
	// and the search continues to previous sentences.
	if (hobbsIsAtTop(nodeY))
		nodeX = nodeY;
	else
		nodeX = nodeY->getParent();
	while (!hobbsIsSentence(nodeX) && !hobbsIsNounPhrase(nodeX) && !hobbsIsAtTop(nodeX)) {
		nodeX = nodeX->getParent();
		if (nodeX == 0)
			return 0;
	}

	if (_debugOut.isActive())
		_debugOut << "END STEP 2:  NODEX = " << nodeX->toFlatString() << "\n";
	// step (3): look to left of p
	// note: currently this over-counts NP's by failing to discard those for
	// which there is no intervening NP or S node. Lance and I (Sam), in a
	// brief discussion, were not able to interpret this part of the step
	// in a way that did not appear to be the Wrong Thing, so I'm leaving
	// it like this for now.
	nResults = hobbsBFSAlongPath(nodeY, nodeX, SEARCH_TO_LEFT, results, nResults, max_results, 
		static_cast<int>(prevSentences.size()));
	
	_debugOut << "END STEP 3.\n";
	while (nResults < max_results) {
		// step (4): look to prev sentences if necessary
		if (hobbsIsAtTop(nodeX)) {
			for(int i=static_cast<int>(prevSentences.size())-1; i>=0 && nResults < max_results; i--) 
				nResults = hobbsBFS(prevSentences[i]->getRoot(), results, nResults, max_results, i);
			_debugOut << "END STEP 4.\n";
			return nResults;
		}

		_debugOut << "SKIPPED STEP 4.\n";
		// step (5) -- find next NP or S node
		nodeY = nodeX;
		nodeX = nodeY->getParent();
		while (!hobbsIsSentence(nodeX) && !hobbsIsNounPhrase(nodeX) && !hobbsIsAtTop(nodeX)) {
			nodeX = nodeX->getParent();
			if (nodeX == 0)
				return 0;
		}
		if (_debugOut.isActive())
			_debugOut << "END STEP 5:  NEW NODEX = " << nodeX->toFlatString() << "\n";

		// step (6) -- propose X; note that we don't do Hobb's N-bar test
		/**/
		if (hobbsIsNounPhrase(nodeX)) {
			//SHOULD NOT CAUSE ERROR
			results[nResults].node = nodeX;
			results[nResults].sentence_number = static_cast<int>(prevSentences.size());
			if (_debugOut.isActive())
				_debugOut << "   ADDED " << nodeX->toFlatString() << "\n";
			if(++nResults == max_results) break;
		}
		/**/
		
		_debugOut << "END STEP 6.\n";
		// step (7) -- look to left of path from Y to X
		nResults = hobbsBFSAlongPath(nodeY, nodeX, SEARCH_TO_LEFT, results, nResults, max_results,
			static_cast<int>(prevSentences.size()));
		if(nResults == max_results) break;
		
		_debugOut << "END STEP 7.\n";
		// step (8) -- if X is an S, then look to right of path
		if (_do_step_8) {
			nResults = hobbsBFSAlongPath(nodeY, nodeX, SEARCH_TO_RIGHT, results, nResults, max_results, 
				static_cast<int>(prevSentences.size()), true);
			if(nResults == max_results) break;
			_debugOut << "END STEP 8.\n";
		}
	}
	return nResults;
}


// count NPs to a particular side of the path from start to X, stop if goal
// found.
int HobbsDistance::hobbsBFSAlongPath(
	const SynNode *start, const SynNode *X, 
	search_direction_t direction, 
	SearchResult results[], int nResults, int max_results, 
	int sentence_number,
	bool limited /* don't go past NP's or S's */)
{
	const SynNode *path[MAX_PATH];
	const SynNode *init_beam[BEAM_WIDTH];
	int pathlen, init_beam_size;

	if (start == X)
		return nResults;

	// construct path from start to X
	int i = 0;
	const SynNode *walker = start;
	while (i < MAX_PATH) {
		path[i++] = walker;
		if (walker == X)
			break;
		walker = walker->getParent();
	}
	pathlen = i;

	int j = 0; // position in init_beam
	for (i = pathlen-1; i >= 1; i--) {
		int n;
		if (direction == SEARCH_TO_LEFT)
			listChildrenToLeftOf(init_beam+j, &n, BEAM_WIDTH - j, path[i], path[i-1]);
		else
			listChildrenToRightOf(init_beam+j, &n, BEAM_WIDTH - j, path[i], path[i-1]);
		j += n;
	}
	init_beam_size = j;

	return hobbsBFS(init_beam, init_beam_size, results, nResults, max_results, sentence_number, limited);
}

// count NPs under (and including) contents of initial_beam by BFS, stop if 
// goal found.
// this only works if no nodes in initial_beam are ancestors of any others.
int HobbsDistance::hobbsBFS(const SynNode **initial_beam,int initial_beam_size, 
	SearchResult results[], int nResults, int max_results, 
	int sentence_number,
	bool limited /*don't go past NP's or S's */)
{
	const SynNode *beam_buf1[BEAM_WIDTH], *beam_buf2[BEAM_WIDTH];
	const SynNode **beam, **new_beam;
	int beam_size;
	int i;
	// initialize beam
	beam = beam_buf1;
	for (i = 0; i < initial_beam_size; i++)
		beam[i] = initial_beam[i];
	beam_size = initial_beam_size;

	// initialize new beam
	new_beam = beam_buf2;

	while (beam_size > 0) {
		// increment score for every NP we see, until we find goal
		for (i = 0; i < beam_size; i++) {
			if (hobbsIsNounPhrase(beam[i])) {
				results[nResults].node = beam[i];
				results[nResults].sentence_number = sentence_number;
				if (_debugOut.isActive())
					_debugOut << "   ADDED " << beam[i]->toFlatString() << "\n";
				if(++nResults == max_results)
					return nResults;
				//if (beam[i] == goal) {
				//	result.status = int::GOAL_FOUND;
				//	return result;
				//}
			}
		}

		// construct new beam from old one
		int j = 0; // position in new_beam
		for (i = 0; i < beam_size && j < BEAM_WIDTH; i++) {
			const SynNode *cur_node = beam[i];
			// if this is a "limited" search, skip over S and NP nodes
			if (limited)
				if (hobbsIsNounPhrase(cur_node) || hobbsIsSentence(cur_node))
					continue;
			//simply add children, right?
			int nChildren = cur_node->getNChildren();
			for (int k=0; k<nChildren && j<BEAM_WIDTH; k++)
				new_beam[j++] = cur_node->getChild(k);
		}
		const SynNode **temp = new_beam;
		new_beam = beam;
		beam = temp;
		beam_size = j;
	}
	return nResults;
}

// count NPs of sentence under given subtree, stop if goal found
int HobbsDistance::hobbsBFS(const SynNode *root, SearchResult results[], int nResults, 
							int max_results, int sentence_number)
{
	return hobbsBFS(&root, 1, results, nResults, max_results, sentence_number);
}


void HobbsDistance::listChildrenToLeftOf(const SynNode **buf, int *n_listed, 
	int buf_size, const SynNode *subtree, const SynNode *child)
{
	int j = 0; // position in buf;

	//just traverse the children, right?
	int nChildren = subtree->getNChildren();
	for(int i=0; i<nChildren; i++) {
		if(subtree->getChild(i) == child || j == buf_size) {
			*n_listed = j;
			return;
		}
		buf[j++] = subtree->getChild(i);
	}
	// we never saw child among the children -- that shouldn't happen...
	// put *something* in n_listed
	*n_listed = 0;
}

void HobbsDistance::listChildrenToRightOf(const SynNode **buf, int *n_listed, 
	int buf_size, const SynNode *subtree, const SynNode *child)
{
	int j = 0; // position in buf;
	bool child_seen = false;

	int i;
	int nChildren = subtree->getNChildren();
	//first skip children before child
	for(i=0; i<nChildren && subtree->getChild(i) != child; i++);
	//now skip child
	i++;
	//now add remaining children
	for(; i<nChildren && j<buf_size; i++)
		buf[j++] = subtree->getChild(i);
	*n_listed = j;
	return;
}


bool HobbsDistance::hobbsIsSentence(const SynNode *node) {
	return NodeInfo::isOfHobbsSKind(node);
}

bool HobbsDistance::hobbsIsNounPhrase(const SynNode *node) {
#if 0
	// if parent is NP, then don't count this node as one
	const SynNode *parent = node->getParent();
	if (parent != 0) {
		if (NodeInfo::isOfHobbsNPKind(parent)) 
			return false;
	}
#endif
	// otherwise, all that matters is the constituent-type
	return NodeInfo::isOfHobbsNPKind(node);
}

bool HobbsDistance::hobbsIsAtTop(const SynNode *node) {
	if (node->getParent() == 0)
		return true;
	else
		return false;
}
