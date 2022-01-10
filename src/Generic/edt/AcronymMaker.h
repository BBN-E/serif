// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACRONYMMAKER_H
#define ACRONYMMAKER_H

#include <vector>
#include <boost/shared_ptr.hpp>

#include "Generic/common/Symbol.h"

class AcronymMaker {
public:
	static AcronymMaker& getSingleton();
	AcronymMaker() {}
	virtual ~AcronymMaker() {}

	/** Given a name, return a list of possible acronyms for that name. */
	virtual std::vector<Symbol> generateAcronyms(const std::vector<Symbol>& words) = 0;

	/** Old interface: included for backwards compatibility; delegates
	 * to the vector version.  Note: this is not virtual; do not override it.
	 * It is retained for backwards compatibility only! */
	int generateAcronyms(const Symbol symArray[], int nSymArray, Symbol results[], int max_results);

	template<typename LanguageAcronymMaker>
	static void setImplementation() {
		_factory() = boost::shared_ptr<AcronymMakerFactory>
			(_new(AcronymMakerFactoryFor<LanguageAcronymMaker>));
	}
private:
	struct AcronymMakerFactory {
		virtual boost::shared_ptr<AcronymMaker> build() = 0;
	};
	template<typename LanguageAcronymMaker>
	struct AcronymMakerFactoryFor: public AcronymMakerFactory {
		boost::shared_ptr<AcronymMaker> build() {
			return boost::shared_ptr<AcronymMaker>(_new LanguageAcronymMaker()); }
	};
	static boost::shared_ptr<AcronymMakerFactory> &_factory();
};

#endif
