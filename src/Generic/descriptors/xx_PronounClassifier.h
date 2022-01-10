// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_PRONOUNCLASSIFIER_H
#define xx_PRONOUNCLASSIFIER_H

#include "Generic/descriptors/PronounClassifier.h"

class DefaultPronounClassifier : public PronounClassifier {
private:
	friend class DefaultPronounClassifierFactory;

	static void defaultMsg(){
		std::cerr << "<<<<<WARNING: Using unimplemented generic pronoun classifier!>>>>>\n";
	};

public:

	virtual int classifyMention (MentionSet *currSolution, Mention *currMention, 
		MentionSet *results[], int max_results, bool isBranching){
			defaultMsg();		
			return 0;
		}

private:
	DefaultPronounClassifier()
	{
		defaultMsg();	
	}

};

class DefaultPronounClassifierFactory: public PronounClassifier::Factory {
	virtual PronounClassifier *build() { return _new DefaultPronounClassifier(); }
};


#endif
