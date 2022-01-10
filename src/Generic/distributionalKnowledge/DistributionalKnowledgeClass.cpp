
#include "Generic/common/leak_detection.h"

#include <string>
#include <set>

#include "Generic/common/Symbol.h"
#include "DistributionalKnowledgeTable.h"
#include "DistributionalKnowledgeClass.h"
#include <boost/shared_ptr.hpp>

#include "DistributionalUtil.h"
#include "DistributionalClusterClass.h"
#include "DistributionalAssocPropClass.h"
#include "DistributionalCausalClass.h"
#include "DistributionalAnchorArgClass.h"

/*
  10/15/2013 : Yee Seng Chan
  The *Fea variables are temporarily there for an alternative way of representing scores, as a set of inequalities,
  instead of as bins as currently represented by the *SB variables.
*/

// distributional knowledge is not yet populated. This is invoked from the default constructor of EventAAObservation 
DistributionalKnowledgeClass DistributionalKnowledgeClass::nullDistributionalKnowledge() {
	DistributionalKnowledgeClass c;

	c._anchor = Symbol();
	c._mappedAnchor = Symbol();
	c._anchorPosTag = Symbol();
	c._arg = Symbol();
	c._argPosTag = Symbol();
	c._eventType = Symbol();
	c._useFeatureVariant = Symbol();

	return c;
}

// default constructor
DistributionalKnowledgeClass::DistributionalKnowledgeClass() {}

// populates the following variables:
// _useFeatureVariant
// _anchor	_anchorPosTag	_arg	_argPosTag	_eventType
// _mappedAnchor
// _subScore , _objScore , _avgSubObjScore , _maxSubObjScore
// _subScoreFea , _subScoreSB , _objScoreFea , _objScoreSB , _avgSubObjScoreFea , _avgSubObjScoreSB , _maxSubObjScoreFea , _maxSubObjScoreSB
// _anchorArgSim , _anchorArgSimFea , _anchorArgSimSB
// _anchorArgPmi , _anchorArgPmiFea , _anchorArgPmiSB
// _anchorSubCid , _anchorObjCid , _anchorAllSubCids , _anchorAllObjCids
DistributionalKnowledgeClass::DistributionalKnowledgeClass(const Symbol& anchor, const Symbol& anchorPosTag, const Symbol& arg, const Symbol& argPosTag, const Symbol& eventType) {
	_useFeatureVariant = Symbol(L"anchor SB");		// plausible values: 'anchor SB' , 'anchor bins'

	_anchor = anchor;
	_anchorPosTag = anchorPosTag;	// pos-tag of anchor
	_arg = arg;
	_argPosTag = argPosTag;		// pos-tag of candidate argument
	_eventType = eventType;

	// is the anchor a noun, verb, adjective, or others
	Symbol anchorPosType = DistributionalUtil::determinePosType(_anchorPosTag);

	_mappedAnchor = assignMappedAnchor(_anchor, anchorPosType);


	// let's now set the different types of scores
	_dCluster = boost::shared_ptr<DistributionalClusterClass>(_new DistributionalClusterClass(_mappedAnchor, _arg));

	_dAssocProp = boost::shared_ptr<DistributionalAssocPropClass>(_new DistributionalAssocPropClass(_mappedAnchor, _arg));

	_dCausal = boost::shared_ptr<DistributionalCausalClass>(_new DistributionalCausalClass(_mappedAnchor, _arg));

	_dAnchorArg = boost::shared_ptr<DistributionalAnchorArgClass>(_new DistributionalAnchorArgClass(_mappedAnchor, anchorPosTag, _arg, argPosTag));
}


// populates: _causalScore , _causalScoreFea , _causalScoreSB
void DistributionalKnowledgeClass::assignCausalFeatures(const std::set<SymbolPair>& feas) {
	_dCausal->assignCausalFeatures(feas);
}
	
// populates: _assocPropSim , _assocPropSimFea , _assocPropSimSB , _assocPropPmi , _assocPropPmiFea , _assocPropPmiSB
void DistributionalKnowledgeClass::assignAssocPropFeatures(const std::set<SymbolPair>& feas) {
	_dAssocProp->assignAssocPropFeatures(feas);
}

bool DistributionalKnowledgeClass::useFeatureVariant(const Symbol& fv) {
	if(fv == _useFeatureVariant)
		return true;
	else
		return false;
}

// as mentioned in the header file, we currently always set _mappedAnchor to _anchor
Symbol DistributionalKnowledgeClass::assignMappedAnchor(const Symbol& anchor, const Symbol& posType) {
	return anchor;	
}

