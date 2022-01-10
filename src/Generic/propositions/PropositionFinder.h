// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROPOSITION_FINDER_H
#define PROPOSITION_FINDER_H


class PropositionSet;
class Parse;
class MentionSet;
class LinearPropositionFinder;
class SemTreeBuilder;
class SynNode;


class PropositionFinder {
public:
	PropositionFinder();

	virtual void resetForNewSentence(int sentence_number);

	virtual PropositionSet *getPropositionTheory(
									const Parse *parse,
									const MentionSet *mentionSet);
	virtual PropositionSet *getPropositionTheory(
									const SynNode *root,
									const MentionSet *mentionSet);

	static bool getUseNominalPremods() { return _use_nominal_premods; }
	static bool getUnifyAppositives() { return _unify_appositives; }
	static bool getUse2009Props() {return _use_2009_props; }

private:
	int _sentence_number;
	LinearPropositionFinder* _linearPropositionFinder;
	SemTreeBuilder* _semTreeBuilder;

	static bool _use_nominal_premods;
	static bool _unify_appositives;
	static bool _use_2009_props;
};

#endif
