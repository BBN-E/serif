// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RARE_HW_LIST_FT_H
#define RARE_HW_LIST_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"
#include "Generic/common/ParamReader.h"
#include <boost/scoped_ptr.hpp>


class RareHWListFT : public P1DescFeatureType {
private:
	struct HashKey {
		size_t operator()(const Symbol& s) const {
			return s.hash_code();
		}
	};
    struct EqualKey {
        bool operator()(const Symbol& s1, const Symbol& s2) const {
            return s1 == s2;
        }
    };
	typedef serif::hash_map<Symbol, Symbol, HashKey, EqualKey> Table;
	static Table* _hwList;
	static bool _initialized; 
	/*
//mrf
The rare HW list has the format WORD TAG
The tags do not need to be entity types
For English, I created a list of all words with ACE types that occured 5x or less in the old descriptor training
and occured only with one type.  I also excluded words that occured both with a type and un-annotated

*/

static void initializeLists(){
	std::string result = ParamReader::getRequiredParam("pdesc_rare_hw_list");
	_hwList = _new Table(5000);
	boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& uis(*uis_scoped_ptr);
	uis.open(result.c_str());
	UTF8Token word;
	UTF8Token label;
	while(!uis.eof()){
		uis >> word;
		uis >> label;
		(*_hwList)[word.symValue()] = label.symValue();
	}
	_initialized = true;
}
public:
	RareHWListFT() : P1DescFeatureType(Symbol(L"rare-hw-list")) {
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if(!_initialized){
			initializeLists();
			_initialized = true;
		}
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(state.getIndex()));

		
		RareHWListFT::Table::iterator iter = _hwList->find(o->getNode()->getHeadWord());
		if (iter == _hwList->end())
			return 0;
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), (*iter).second);
		return 1;
	}


	


};
#endif
