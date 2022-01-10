#ifndef XX_GROUP_FN_GUESSER_H
#define XX_GROUP_FN_GUESSER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/eeml/GroupFnGuesser.h"

class GenericGroupFnGuesser : public GroupFnGuesser {
private:
	friend class GenericGroupFnGuesserFactory;

public:

	static int isPlural(const Mention *mention) 
	{
		return UNKNOWN;
	}


};

class GenericGroupFnGuesserFactory: public GroupFnGuesser::Factory {
	virtual int isPlural(const Mention *mention) {  return GenericGroupFnGuesser::isPlural(mention); }
};



#endif
