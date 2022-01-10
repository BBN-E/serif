// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef IDF_WORD_FEATURES_H
#define IDF_WORD_FEATURES_H

#include <string>
#include <cstdio>
#include "Generic/common/Symbol.h"
#include <boost/shared_ptr.hpp>
class IdFListSet;

class IdFWordFeatures {
public:
	/** Create and return a new IdFWordFeatures. */
	static IdFWordFeatures *build(int mode=DEFAULT, IdFListSet *listSet=0) { return _factory()->build(mode, listSet); }
	/** Hook for setting the IdFWordFeatures implementation. */
	struct Factory { 
		virtual IdFWordFeatures *build(int mode, IdFListSet *listSet) = 0; 
	};
    template<typename T> static void setImplementation() {
		_factory() = boost::shared_ptr<Factory>(_new FactoryFor<T>());
	}

    virtual Symbol features(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab) const = 0;
	virtual int getAllFeatures(Symbol wordSym, bool first_word, 
		bool normalized_word_is_in_vocab, Symbol *featureArray,
		int max_features) const { return 0; }
	virtual bool containsDigit(Symbol wordSym) const = 0;
	virtual Symbol normalizedWord(Symbol wordSym) const = 0;

	virtual IdFListSet *getListSet() const { return _listSet; }

	enum { DEFAULT, TIMEX, OTHER_VALUE };
	virtual void setMode(int mode) {}
    
    virtual ~IdFWordFeatures();

protected:
	IdFListSet *_listSet;
	IdFWordFeatures(): _listSet(0) {}

	// Template factory subclass used by setImplementation:
	template<typename T> struct FactoryFor: public Factory {
		IdFWordFeatures *build(int mode, IdFListSet *listSet) {
			return _new T(mode, listSet);
		}
	};
private:
	static boost::shared_ptr<Factory> &_factory(); 	
};

#endif
