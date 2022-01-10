
#ifndef DISTRIBUTIONAL_KNOWLEDGE_CLASS_H
#define DISTRIBUTIONAL_KNOWLEDGE_CLASS_H

#include <string>
#include <set>

class Symbol;

#include "DistributionalClusterClass.h"
#include "DistributionalAssocPropClass.h"
#include "DistributionalCausalClass.h"
#include "DistributionalAnchorArgClass.h"
#include <boost/shared_ptr.hpp>

/*
  10/15/2013 : Yee Seng Chan
  The *Fea variables are temporarily there for an alternative way of representing scores, as a set of inequalities,
  instead of as bins as currently represented by the *SB variables.

  For a particular instance of EventAAObservation, i.e. a particular instance of (anchor, candidate argument) observation,
  this is the main interface to extracting/representing the various types of scores we have gathered between the (anchor, arg) pair.
  This class invokes the following classes to extract the different scores:
    - DistributionalClusterClass
    - DistributionalAssocPropClass
    - DistributionalCausalClass
    - DistributionalAnchorArgClass
*/
class DistributionalKnowledgeClass {
private:
        typedef std::pair<Symbol, int> SymIntPair;
	typedef std::pair<Symbol, Symbol> SymbolPair;

public:
	// this is invoked when the default constructor of EventAAObservation is invoked. Look in EventAAObservation.h
	static DistributionalKnowledgeClass nullDistributionalKnowledge();

	// constructor
	DistributionalKnowledgeClass(const Symbol& anchor, const Symbol& anchorPosTag, const Symbol& arg, const Symbol& argPosTag, const Symbol& eventType);

	// the following two functions are invoked in: EventAAObservation::setAADistributionalKnowledge()
	// we need to wait until EventAAObservation has done its gathering of propositions for the (anchor, arg)
	// the following two functions define features based on those propositions. 
	void assignCausalFeatures(const std::set<SymbolPair>& feas);
	void assignAssocPropFeatures(const std::set<SymbolPair>& feas);

	bool useFeatureVariant(const Symbol& fv);


	// ==== start of accessors ====
	Symbol anchor() const { return _anchor; }
	Symbol arg() const { return _arg; }
	std::set<Symbol> entityMentionsNominalHw() const { return _entityMentionsNominalHw; }

	// DistributionalClusterClass
	std::set<int> anchorAllSubCids() const { return _dCluster->anchorAllSubCids(); }
	std::set<int> anchorAllObjCids() const { return _dCluster->anchorAllObjCids(); }

	// DistributionalAnchorArgClass
	int subScoreSB() const { return _dAnchorArg->subScoreSB(); }
	int objScoreSB() const { return _dAnchorArg->objScoreSB(); }
	int avgSubObjScoreSB() const { return _dAnchorArg->avgSubObjScoreSB(); }
	int maxSubObjScoreSB() const { return _dAnchorArg->maxSubObjScoreSB(); }
	//std::vector<Symbol> subScoreFea() const { return _dAnchorArg->subScoreFea(); }
	//std::vector<Symbol> objScoreFea() const { return _dAnchorArg->objScoreFea(); }
	//std::vector<Symbol> avgSubObjScoreFea() const { return _dAnchorArg->avgSubObjScoreFea(); }
	//std::vector<Symbol> maxSubObjScoreFea() const { return _dAnchorArg->maxSubObjScoreFea(); }

	float anchorArgSim() const { return _dAnchorArg->anchorArgSim(); }
	int anchorArgSimSB() const { return _dAnchorArg->anchorArgSimSB(); }
	//std::vector<Symbol> anchorArgSimFea() const { return _dAnchorArg->anchorArgSimFea(); }

	float anchorArgPmi() const { return _dAnchorArg->anchorArgPmi(); }
	int anchorArgPmiSB() const { return _dAnchorArg->anchorArgPmiSB(); }
	//std::vector<Symbol> anchorArgPmiFea() const { return _dAnchorArg->anchorArgPmiFea(); }

	// DistributionalAssocPropClass
	float assocPropSim() const { return _dAssocProp->assocPropSim(); }	
	int assocPropSimSB() const { return _dAssocProp->assocPropSimSB(); }
	//std::vector<Symbol> assocPropSimFea() const { return _dAssocProp->assocPropSimFea(); }

	float assocPropPmi() const { return _dAssocProp->assocPropPmi(); }	
	int assocPropPmiSB() const { return _dAssocProp->assocPropPmiSB(); }
	//std::vector<Symbol> assocPropPmiFea() const { return _dAssocProp->assocPropPmiFea(); }

	// DistributionalCausalClass
	float causalScore() const { return _dCausal->causalScore(); }
	int causalScoreSB() const { return _dCausal->causalScoreSB(); }
	//std::vector<Symbol> causalScoreFea() const { return _dCausal->causalScoreFea(); }
	// ==== end of accessors ====

private:
	// (_anchor, _arg) : (anchor, arg) for this particular event-aa mention
	// _anchorPosTag, _argPosTag : their part-of-speech tags
	// _mappedAnchor : right now, it is always set equal to _anchor. We also use _mappedAnchor (instead of _anchor) to generate features.
	//                 if you want to do something fancy in future, e.g. further normalize _anchor, 
	// 		   this allows you to do it for feature generation while keeping _anchor intact.
	Symbol _anchor, _mappedAnchor, _anchorPosTag;	
	Symbol _arg, _argPosTag;

	// (ACE) event type represented by the _anchor
	Symbol _eventType;

	// 10/16/2013 : Yee Seng Chan.
	// _entityMentionsNominalHw is currently not used. If the candidate argument (_arg) is a proper noun, then this is meant to hold 
	// the nominals in its coreference chain. We might want to experiment with this in future for additional features.
	std::set<Symbol> _entityMentionsNominalHw;

	// if you have a pmi score of say 0.623 between (anchor, arg). How do you want to represent it? Two plausible alternatives follow:
	// - as a single bin, e.g. map 0.623 to bin '6'
	// - as a set of inequalities, e.g.: { '>=0.5', '>=0.6', '<0.7' }
	// right now, we set this to "anchor SB" in this class's constructor, to signify representing as single bin.
	// This variable is then checked in the features file source code (e.g. AADKAnchorArgPmiAndPropV20RTFeatureType.h) to decide how to generate features.
	Symbol _useFeatureVariant;

	// we produce different types of scores for a pair of (anchor, arg). For more information, look at the following particular classes' source code.
	// the following classes take care of representing their own scores.
	boost::shared_ptr<DistributionalClusterClass> _dCluster;
	boost::shared_ptr<DistributionalAssocPropClass> _dAssocProp;
	boost::shared_ptr<DistributionalCausalClass> _dCausal;
	boost::shared_ptr<DistributionalAnchorArgClass> _dAnchorArg;

	// default constructor needed because we had defined a non default constructor for this class
	DistributionalKnowledgeClass();

	// as mention above, currently _mappedAnchor is just set to _anchor. But you could do something more fancy.
	Symbol assignMappedAnchor(const Symbol& anchor, const Symbol& posType);
};

#endif

