// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_DESCCLASSMODIFIERS_H
#define xx_DESCCLASSMODIFIERS_H

#include "Generic/descriptors/DescClassModifiers.h"

class DefaultDescClassModifiers : public DescClassModifiers {
private:
	friend class DefaultDescClassModifiersFactory;
	static void defaultMsg(){
		std::cerr << "<<<<<WARNING: Using unimplemented generic DescClassModifiers!>>>>>\n";
	};

	DefaultDescClassModifiers() {
		defaultMsg();	
	}

};

class DefaultDescClassModifiersFactory: public DescClassModifiers::Factory {
	virtual DescClassModifiers *build() { return _new DefaultDescClassModifiers(); }
};


#endif
