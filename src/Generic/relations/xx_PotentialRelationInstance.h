#ifndef XX_POTENTIAL_RELATION_INSTANCE_H
#define XX_POTENTIAL_RELATION_INSTANCE_H

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#define POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE 13

class GenericPotentialRelationInstance: public PotentialRelationInstance {
public:
	GenericPotentialRelationInstance() {}

	void setStandardInstance(Argument *first, 
		Argument *second, Proposition *prop) {}
	void setStandardNestedInstance(Argument *first, Argument *intermediate, 
		Argument *second, Proposition *outer_prop, Proposition *inner_prop) {}
	
};



#endif
