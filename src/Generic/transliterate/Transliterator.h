// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TRANSLITERATOR_H
#define TRANSLITERATOR_H

#include <string>
#include <boost/shared_ptr.hpp>
class Symbol;
class TokenSequence;
class Mention;
class Token;

// Currently, the default Transliterator factory raises an exception
// saying that the "Transliterator" feature module is required.  Should
// we instead return a Transliterator that always returns empty strings?

/** Interface for transliterators, which transliterate (or translate)
  * languages from their native language into English strings. */
class Transliterator {
public:
	/** Create and return a new Transliterator. */
	static Transliterator *build();

	/** Return a transliteration for the string spanning from token
		`start` to token `end` (inclusive) in `tokenSequence`. */
	virtual std::wstring getTransliteration(const TokenSequence* tokenSequence, int start, int end) = 0;

	/** Return a transliteration for the tokens covered by the given
		mention. */
	virtual std::wstring getTransliteration(const Mention *mention);
    
    virtual ~Transliterator() { }

	/** Hook for registering new Transliterator factories */
	struct Factory { virtual Transliterator *build(); };
	static void setFactory(boost::shared_ptr<Factory> factory);
private:
	static boost::shared_ptr<Factory> &_factory();
};

#endif
