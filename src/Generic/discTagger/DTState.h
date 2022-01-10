// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_STATE_H
#define D_T_STATE_H

#include "Generic/common/limits.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/Symbol.h"

#include "Generic/discTagger/DTObservation.h"


/** DTState represents the state of the tagger to the feature extractors.
  */

class DTState {
public:
	// the old constructor, with no nested names (no semiReducedTag):
	DTState(const Symbol &tag, const Symbol &reducedTag, const Symbol &prevTag,
			int index, std::vector<DTObservation*> observations)
		: _tag(tag), _reducedTag(reducedTag), _semiReducedTag(reducedTag), _prevTag(prevTag),
		  _index(index)
	{
		//std::copy(observations, observations+n_observations, _observations);
		_observations = observations;
		_n_observations = static_cast<int>(observations.size());
    }

	// for nested names:
	DTState(const Symbol &tag, const Symbol &reducedTag, const Symbol &semiReducedTag, const Symbol &prevTag,
			int index, std::vector<DTObservation*> observations)
		: _tag(tag), _reducedTag(reducedTag), _semiReducedTag(semiReducedTag), _prevTag(prevTag),
		  _index(index)
	{
		//std::copy(observations, observations+n_observations, _observations);
		_observations = observations;
		_n_observations = static_cast<int>(observations.size());
			
	}

	// The references returned by these methods are invalidated when the
	// DTState object is destroyed.  If you need them longer than that,
	// then copy them.
	const Symbol &getTag() const { return _tag; } 
	const Symbol &getReducedTag() const { return _reducedTag; }
	const Symbol &getSemiReducedTag() const { return _semiReducedTag; }
	const Symbol &getPrevTag() const { return _prevTag; }
	
	int getIndex() const { return _index; }
	int getNObservations() const { return _n_observations; } 
	std::vector<DTObservation *> getObservations() const {
		return _observations;
	}
	DTObservation *getObservation(int i) const {
		if ((unsigned) i < (unsigned) _n_observations) {
			return _observations[i];
		}
		else {
			throw InternalInconsistencyException::arrayIndexException(
				"DTState::getObservation()", _n_observations, i);
		}
	}
	void dump(){
		std::cout<<"Tag: "<<_tag.to_debug_string()
			<<" prevTag: "<<_prevTag.to_debug_string();
		if(_index<_n_observations)
			_observations[_index]->dump();
		std::cout<<std::endl;
	}

private:
    Symbol _tag;
    Symbol _reducedTag;
    Symbol _semiReducedTag;
    Symbol _prevTag;
    int _index;
	int _n_observations;
	std::vector<DTObservation*> _observations;
};

#endif
