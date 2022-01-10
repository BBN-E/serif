// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_PROP_LINK_H
#define RELATION_PROP_LINK_H

#include "Generic/common/Symbol.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/wordClustering/WordClusterClass.h"

#define MAX_WN_OFFSETS 20

/** 
*/

class TreeNodeChain;
struct TreeNodeElement;

class RelationPropLink {
public:
	RelationPropLink() :
	  _topProposition(0), _intermediateProposition(0), _arg1(0),
		  _arg2(0), _intermediate_arg(0), _nest_direction(NONE),
		  _wcTop(WordClusterClass::nullCluster()), 
		  _wcArg1(WordClusterClass::nullCluster()),
		  _wcArg2(WordClusterClass::nullCluster()),
		  _n_offsets(0), _firstEl(0), _secondEl(0)
	  {}

	  enum { NONE, LEFT, RIGHT };

	  void populate(Proposition *prop, Argument *a1, Argument *a2, const MentionSet *mentionSet);
	  void populate(Proposition *prop, Proposition *intermedProp, 
		  Argument *a1, Argument *intermed, Argument *a2, const MentionSet *mentionSet, int nest);
	  void populate(const TreeNodeChain* tnChain);

	  void reset();
	  bool isEmpty() { return _topProposition == 0; }
	  bool isNegative();

	  Proposition *getTopProposition() const { return _topProposition; }
	  Symbol getArg1Role() const;
	  Symbol getArg2Role() const;
	  Symbol getIntermedArgRole() const;
	  const Mention* getArg1Ment(const MentionSet* mentionSet) const;
	  const Mention* getArg2Ment(const MentionSet* mentionSet) const;
	  
	  Symbol getTopStemmedPred() const { return _topStemmedPred; }

	  bool isNested() const { return _nest_direction != NONE; }
	  bool isRightNested() const { return _nest_direction == RIGHT; }
	  WordClusterClass getWCTop() const { return _wcTop; }
	  WordClusterClass getWCArg1() const { return _wcArg1; }
	  WordClusterClass getWCArg2() const { return _wcArg2; }
	  int getNOffsets() const { return _n_offsets; }
	  int getNthOffset(int n) const { return _wordnetOffsets[n]; }

private:
	Proposition *_topProposition;
	Argument *_arg1;
	Argument *_arg2;
	Proposition *_intermediateProposition;
	Argument *_intermediate_arg;

	WordClusterClass _wcTop;
	WordClusterClass _wcArg1;
	WordClusterClass _wcArg2;

	// only used in English
	Symbol _topStemmedPred;
	int _wordnetOffsets[MAX_WN_OFFSETS];
	int _n_offsets;

	int _nest_direction;

	//only used when populated from TreeNodeChain
	const TreeNodeElement *_firstEl, *_secondEl, *_topEl;
	std::wstring _altRole1, _altRole2;
};

#endif
