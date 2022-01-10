// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_PARSE_SEEDER_H
#define xx_PARSE_SEEDER_H

#include "Generic/morphSelection/ParseSeeder.h"
#include "Generic/common/UTF8OutputStream.h"
#include <iostream>

class Token;

class GenericParseSeeder : public ParseSeeder {
private:
	friend class GenericParseSeederFactory;


public:
	
	virtual void processToken(const LocatedString& sentenceString, const Token* t) {};

	virtual void dumpBothCharts(UTF8OutputStream &out) {};
	virtual void dumpBothCharts(std::ostream &out) {};

};

class GenericParseSeederFactory: public ParseSeeder::Factory {
	virtual ParseSeeder *build() { return _new GenericParseSeeder(); }
};


#endif
