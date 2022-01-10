
#ifndef DISTRIBUTIONAL_ANCHORARG_CLASS_H
#define DISTRIBUTIONAL_ANCHORARG_CLASS_H

#include <string>
#include <vector>

class Symbol;

/*
  10/15/2013 : Yee Seng Chan
  The *Fea variables are temporarily there for an alternative way of representing scores, as a set of inequalities,
  instead of as bins as currently represented by the *SB variables.

  This class handles four types of scores. Given a (anchor, arg):
  - in general, what is the pmi(anchor, arg)
  - limited to just <sub> proposition role, what is pmi_<sub>(anchor, arg)
  - limited to just <obj> proposition role, what is pmi_<obj>(anchor, arg)
  - what is the cosine similarity sim(anchor, arg)
  These scores have all been pre-normalized to range between 0 to 1.0
*/
class DistributionalAnchorArgClass {
public:
	DistributionalAnchorArgClass(const Symbol& anchor, const Symbol& anchorPosTag, const Symbol& arg, const Symbol& argPosTag);

	// ==== start of accessors ====
	// <sub> score remapped to bin
	int subScoreSB() const { return _subScoreSB; }
	//std::vector<Symbol> subScoreFea() const { return _subScoreFea; }

	// <obj> score remapped to bin
	int objScoreSB() const { return _objScoreSB; }
	//std::vector<Symbol> objScoreFea() const { return _objScoreFea; }

	// average of the <sub> and <obj> score, then remapped to bin
	int avgSubObjScoreSB() const { return _avgSubObjScoreSB; }
	//std::vector<Symbol> avgSubObjScoreFea() const { return _avgSubObjScoreFea; }

	// bin representation of the max of the <sub> and <obj> score
	int maxSubObjScoreSB() const { return _maxSubObjScoreSB; }
	//std::vector<Symbol> maxSubObjScoreFea() const { return _maxSubObjScoreFea; }

	// cosine similiarity score between anchor and arg
	float anchorArgSim() const { return _anchorArgSim; }
	// cosine similarity remapped to bin
	int anchorArgSimSB() const { return _anchorArgSimSB; }
	//std::vector<Symbol> anchorArgSimFea() const { return _anchorArgSimFea; }

	// pmi score between anchor and arg
	float anchorArgPmi() const { return _anchorArgPmi; }
	// pmi remapped to bin
	int anchorArgPmiSB() const { return _anchorArgPmiSB; }
	//std::vector<Symbol> anchorArgPmiFea() const { return _anchorArgPmiFea; }
	// ==== end of accessors ====

private:
	Symbol _anchor, _arg;	// (anchor, arg) for this particular event-aa mention

	float _subScore, _objScore;		// <sub> and <obj> score
	int _subScoreSB, _objScoreSB;	// the above scores but represented as bin
	//std::vector<Symbol> _subScoreFea, _objScoreFea;

	float _avgSubObjScore, _maxSubObjScore;
	int _avgSubObjScoreSB, _maxSubObjScoreSB;
	//std::vector<Symbol> _avgSubObjScoreFea, _maxSubObjScoreFea;

	float _anchorArgSim;			// cosine similarity between anchor and arg
	int _anchorArgSimSB;
	//std::vector<Symbol> _anchorArgSimFea;

	float _anchorArgPmi;			// pmi between anchor and arg
	int _anchorArgPmiSB;
	//std::vector<Symbol> _anchorArgPmiFea;

	// populate the different types of scores. These functions are invoked by the constructor
	void assignSubObjScores(const Symbol& anchor, const Symbol& anchorPosTag, const Symbol& arg, const Symbol& argPosTag);
	void assignPmiScores(const Symbol& anchor, const Symbol& anchorPosType, const Symbol& arg, const Symbol& argPosType);	
	void assignSimScores(const Symbol& anchor, const Symbol& anchorPosType, const Symbol& arg, const Symbol& argPosType);
};

#endif

