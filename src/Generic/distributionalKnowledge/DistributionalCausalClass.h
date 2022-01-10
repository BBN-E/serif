
#ifndef DISTRIBUTIONAL_CAUSAL_CLASS_H
#define DISTRIBUTIONAL_CAUSAL_CLASS_H

#include <set>

class Symbol;

/*
  10/15/2013 : Yee Seng Chan
  The *Fea variables are temporarily there for an alternative way of representing scores, as a set of inequalities,
  instead of as bins as currently represented by the *SB variables.
*/

class DistributionalCausalClass {
private:
	typedef std::pair<Symbol, Symbol> SymbolPair;

public:
	// constructor
	DistributionalCausalClass(const Symbol& anchor, const Symbol& arg);

	// invoked in: EventAAObservation::setAADistributionalKnowledge()
	void assignCausalFeatures(const std::set<SymbolPair>& feas);

	// ==== start of accessors ====
	// returns the actual causality score
	float causalScore() const { return _causalScore; }

	// returns causality score mapped/normalized as a bin
	int causalScoreSB() const { return _causalScoreSB; }

	//std::vector<Symbol> causalScoreFea() const { return _causalScoreFea; }
	// ==== end of accessors ====

private:
	Symbol _anchor, _arg;	// (anchor, arg) for this particular event-aa mention

	float _causalScore;		// actual causality score
	int _causalScoreSB;		// causality score mapped/normalized as a bin
	//std::vector<Symbol> _causalScoreFea;
};

#endif

