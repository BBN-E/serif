// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef INSTANCESETTEST_H
#define INSTANCESETTEST_H

#include "relations/HighYield/HYRelationInstance.h"
#include "relations/HighYield/HYInstanceSet.h"

#include "Utilities.h"

class InstanceSetTester {
public:
	void runTest() {
		char tag_set_file[PARAM_LENGTH];
		getStringParam("maxent_relation_tag_set_file", tag_set_file, PARAM_LENGTH, "InstanceSetTester::runTest()");
		DTTagSet* tagset = _new DTTagSet(tag_set_file, false, false);

		HYInstanceSet set;
		set.setTagset(tagset);
		set.addDataFromSerifFileList("\\\\traid04\\u0\\users\\askotows\\MaxentAL\\FixOffsets\\instance-test.list", 20);
		printf("Basic set has %d instances.\n", static_cast<int>(set.size()));

		// make smallSet to test writing out:
		HYInstanceSet smallSet;
		smallSet.setTagset(tagset);

		for (vector<HYRelationInstance*>::const_iterator instIt = set.begin();
			(instIt != set.end()) && (smallSet.size() < 20);
			 ++instIt) {
				 smallSet.addInstance(*instIt);
				 switch (smallSet.size()) {
					 case 1: (*instIt)->setAnnotationSymbol(L"BAD EDT");
						 break;
					 case 2: (*instIt)->setAnnotationSymbol(L"BAD PRON");
						 break;
					 case 4:  (*instIt)->setAnnotationSymbol(L"randomsym");
						 break;
				 }
		}
		printf("Writing to annotation file...\n");
		smallSet.writeToAnnotationFile("instance-test.relsent");

		// make newSet to test reading in:
		HYInstanceSet newSet;
		newSet.setTagset(tagset);
		printf("Reading from annotation file...\n");
		newSet.addDataFromAnnotationFile("instance-test.relsent", set);
		printf("Printing out the %d instances read in:\n", static_cast<int>(newSet.size()));
		for (vector<HYRelationInstance*>::const_iterator instIt = newSet.begin();
			instIt != newSet.end();
			++instIt) {
		
			HYRelationInstance* instance = *instIt;
			printf("Instance %s: %s%svalid\n", 
				instance->getID().to_debug_string(), 
				instance->isReversed() ? "REVERSED, " : "",
				instance->isValid() ? "" : "NOT ");
		}
	}
};

#endif
