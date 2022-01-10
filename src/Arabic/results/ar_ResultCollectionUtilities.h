// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
#ifndef ar_RESULT_COLLECTION_UTILITIES_H
#define ar_RESULT_COLLECTION_UTILITIES_H
#include "Generic/theories/Mention.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/ParamReader.h"
#include "Arabic/parse/ar_STags.h"
#include "Generic/results/ResultCollectionUtilities.h"


class ArabicResultCollectionUtilities : public ResultCollectionUtilities {
private:
	friend class ArabicResultCollectionUtilitiesFactory;

private:
	static bool _initialized;
	static bool _doToPremod;
	static void initialize(){
		_doToPremod = ParamReader::isParamTrue("do_premod_apf");
		_initialized = true;
	};

public:
	static bool isPREmention(const EntitySet *entitySet, Mention *ment) {
		if(!_initialized){
			initialize();
		}
		if(!_doToPremod){
			return false;
		}
		//too much of a loss to allow name mentions to be pre's in ACE2004
		if(ment->getMentionType() != Mention::DESC)
			return false;
		const SynNode *thisNode = ment->getNode();
		const SynNode *parent = thisNode->getParent();
		if(parent == 0)
			return false;
		const SynNode *parentsHead = parent->getHead();
		if(parentsHead->hasMention() != true){
			return false;
		}
		if(parentsHead == thisNode){
			return false;
		}
		return true;

	}


};

class ArabicResultCollectionUtilitiesFactory: public ResultCollectionUtilities::Factory {
	virtual bool isPREmention(const EntitySet *entitySet, Mention *ment) {  return ArabicResultCollectionUtilities::isPREmention(entitySet, ment); }
};




bool ArabicResultCollectionUtilities::_initialized = false;
bool ArabicResultCollectionUtilities::_doToPremod = false;

#endif
