
#ifndef DISTRIBUTIONAL_ASSOCPROP_CLASS_H
#define DISTRIBUTIONAL_ASSOCPROP_CLASS_H

#include <vector>
#include <set>

class Symbol;

/*
  10/15/2013 : Yee Seng Chan
  The *Fea variables are temporarily there for an alternative way of representing scores, as a set of inequalities,
  instead of as bins as currently represented by the *SB variables.
*/

class DistributionalAssocPropClass {
private:
	typedef std::pair<Symbol, Symbol> SymbolPair;

public:
	DistributionalAssocPropClass(const Symbol& anchor, const Symbol& arg);

	void assignAssocPropFeatures(const std::set<SymbolPair>& feas);

	// ==== start of accessors ====
	float assocPropSim() const { return _assocPropSim; }	
	int assocPropSimSB() const { return _assocPropSimSB; }
	//std::vector<Symbol> assocPropSimFea() const { return _assocPropSimFea; }

	float assocPropPmi() const { return _assocPropPmi; }	
	int assocPropPmiSB() const { return _assocPropPmiSB; }
	//std::vector<Symbol> assocPropPmiFea() const { return _assocPropPmiFea; }
	// ==== end of accessors ====

private:
	Symbol _anchor, _arg;	// (anchor, arg) for this particular event-aa mention

	float _assocPropSim;	// based on cosine similarity
	int _assocPropSimSB;
	//std::vector<Symbol> _assocPropSimFea;

	float _assocPropPmi;	// based on pmi
	int _assocPropPmiSB;
	//std::vector<Symbol> _assocPropPmiFea;
};

#endif

