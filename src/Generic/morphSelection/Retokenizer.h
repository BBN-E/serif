// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RETOKENIZER_H
#define RETOKENIZER_H

#include <boost/shared_ptr.hpp>
class TokenSequence;
class Token;

class Retokenizer {
public:
	/** Hook for setting the Retokenizer implementation. */
	struct Factory { virtual Retokenizer *build() = 0; };
    template<typename T> static void setImplementation() {
		_factory() = boost::shared_ptr<Factory>(_new FactoryFor<T>());
	}

	/** Return the singleton Retokenizer.  If the retokenizer does not
	 * exist (i.e., if the first time this method is called, or the
	 * first time it is called after a call to destroy(), then a new
	 * retokenizer is created using the retokenizer factory. */
	static Retokenizer& getInstance();

	/** Destroy the singleton retokenizer. */
	static void destroy();

	virtual void Retokenize(TokenSequence* origTS, Token** newTokens, int n_new) = 0;
	virtual void RetokenizeForNames(TokenSequence *origTS, Token** newTokens, int n_new) = 0;
	virtual void reset() = 0;

protected:
	/** This gets called immediately after getInstance().  It is used 
	 * by the ArabicRetokenzier subclass to do check to make sure that
	 * the session lexicon didn't get changed without our knowledge. 
	 * If it returns false, then the retokenizer will be destroyed and
	 * re-constructed. */
	virtual bool getInstanceIntegrityCheck() = 0;

	// Template factory subclass used by setImplementation:
	template<typename T> struct FactoryFor: public Factory {
		Retokenizer* build() { return _new T(); }
	};
    
    virtual ~Retokenizer() { }
private:
	static boost::shared_ptr<Factory> &_factory(); 	
	static Retokenizer * _instance;
};

// #if defined(ARABIC_LANGUAGE)
// 	#include "Arabic/BuckWalter/ar_Retokenizer.h"
// #elif defined(KOREAN_LANGUAGE)
// 	#include "Korean/morphology/kr_Retokenizer.h"
// #else
// 	#include "Generic/morphSelection/xx_Retokenizer.h"
// #endif

#endif



