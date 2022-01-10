#ifndef EN_GROUP_FN_GUESSER_H
#define EN_GROUP_FN_GUESSER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/edt/Guesser.h"
#include "Generic/eeml/GroupFnGuesser.h"

class EnglishGroupFnGuesser : public GroupFnGuesser {
private:
	friend class EnglishGroupFnGuesserFactory;

public:

	static int isPlural(const Mention *mention) 
	{
		Symbol number = Guesser::guessNumber(mention->getNode(), mention);
		if (number == Guesser::SINGULAR)
			return SINGULAR;
		else if (number == Guesser::PLURAL)
			return PLURAL;
		else return UNKNOWN;
	}


};

class EnglishGroupFnGuesserFactory: public GroupFnGuesser::Factory {
	virtual int isPlural(const Mention *mention) {  return EnglishGroupFnGuesser::isPlural(mention); }
};



#endif
