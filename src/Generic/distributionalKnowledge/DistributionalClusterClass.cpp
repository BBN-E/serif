
#include "Generic/common/leak_detection.h"

#include <string>

#include "Generic/common/Symbol.h"
#include "DistributionalKnowledgeTable.h"
#include "DistributionalClusterClass.h"


DistributionalClusterClass::DistributionalClusterClass(const Symbol& anchor, const Symbol& arg) : _anchor(anchor), _arg(arg) {
	assignAnchorAllClusterIds(anchor, arg);		// populates: _anchorAllSubCids , _anchorAllObjCids
}

/*
// 24/10/2013. Yee Seng Chan. 
// Old code to get the cluster id for a specific pair of (verb, noun).
// Perhaps good to keep around for the moment for experimental purposes.
void DistributionalClusterClass::assignAnchorSpecificClusterIds(const Symbol& anchor, const Symbol& arg) {
	int pSubCid = DistributionalKnowledgeTable::getPredicateSubCid(anchor, arg);
	if(pSubCid == -1)	// arg wasn't a subject of anchor
		_anchorSubCid = std::pair<Symbol, int>(Symbol(), -1);
	else
		_anchorSubCid = std::pair<Symbol, int>(anchor, pSubCid);

	int pObjCid = DistributionalKnowledgeTable::getPredicateObjCid(anchor, arg);
	if(pObjCid == -1)	// arg wasn't an object of anchor
		_anchorObjCid = std::pair<Symbol, int>(Symbol(), -1);
	else
		_anchorObjCid = std::pair<Symbol, int>(anchor, pObjCid);
}
*/

void DistributionalClusterClass::assignAnchorAllClusterIds(const Symbol& anchor, const Symbol& arg) {
	_anchorAllSubCids = DistributionalKnowledgeTable::getSubClusterIdsForArg(arg);
	_anchorAllObjCids = DistributionalKnowledgeTable::getObjClusterIdsForArg(arg);
}

