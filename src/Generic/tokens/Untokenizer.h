// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef UNTOKENIZER_H
#define UNTOKENIZER_H

#include <boost/shared_ptr.hpp>

#include "Generic/common/LocatedString.h"
#include "Generic/common/Symbol.h"


class Untokenizer {
public:
	/** Create and return a new Untokenizer. */
	static Untokenizer *build() { return _factory()->build(); }
	/** Hook for registering new Untokenizer factories */
	struct Factory { virtual Untokenizer *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

public:
	virtual ~Untokenizer() {}

	virtual void untokenize(LocatedString& source) const = 0;

protected:
	Untokenizer() {}

private:
	static boost::shared_ptr<Factory> &_factory();
};

// language-specific includes determine which implementation is used
//#ifdef ENGLISH_LANGUAGE
//	#include "English/tokens/en_Untokenizer.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/tokens/ch_Untokenizer.h"
//#elif defined(KOREAN_LANGUAGE)
//	#include "Korean/tokens/kr_Untokenizer.h"
//#else
//	#include "Generic/tokens/xx_Untokenizer.h"
//#endif

#endif
