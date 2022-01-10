// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_OBSERVATION_H
#define D_T_OBSERVATION_H

#include "Generic/common/Symbol.h"
#include "Generic/discTagger/DTFeature.h"

/** A DTObservation is an abstract interface class. A subclass object
  * represents information about the context of a tagging
  * decision, which the FeatureType extractors extract features from.
  */

class DTObservation {
public:
	DTObservation(Symbol name) : _name(name) {}
	virtual ~DTObservation() {}
	virtual bool isValidTag(Symbol tag) { return true; }
	virtual DTObservation* makeCopy() = 0; 
	virtual void setAltDecoderPrediction(int i, Symbol prediction) {}
	virtual void dump() {
		std::cout << "Observation: " << _name.to_debug_string() << " dump unimplemented";
	}
	virtual std::string toString() const { return std::string(_name.to_debug_string()); }
private:
	Symbol _name;
};

#endif
