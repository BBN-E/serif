// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KESTIMATOR_H
#define KESTIMATOR_H

#include <cstddef>
#include "Generic/common/NgramScoreTable.h"
#include "Generic/parse/HeadProbs.h"
#include "Generic/parse/ModifierProbs.h"
#include "Generic/parse/LexicalProbs.h"
#include "Generic/parse/HeadProbDeriver.h"
#include "Generic/parse/ModifierProbDeriver.h"
#include "Generic/parse/LexicalProbDeriver.h"
#include "Generic/common/Symbol.h"



class K_Estimator {
public:
	K_Estimator(char *head_counts_file, char *pre_counts_file, char *post_counts_file, 
						 char *left_counts_file, char *right_counts_file,
						 char *headProbFile, char *premodProbFile, char *postmodProbFile, 
						 char *leftLexicalProbFile, char *rightLexicalProbFile, 
						 char *outfile,
						 int mhc,
						 double inc, double min, double max);
	~K_Estimator();
	void estimate();
	
private:

	char* _outfile;

	enum{HEAD, PRE, POST, LEFT, RIGHT};

	HeadProbDeriver* _headDeriver;
    LexicalProbDeriver* _leftLexicalDeriver;
    LexicalProbDeriver* _rightLexicalDeriver;
    ModifierProbDeriver* _premodDeriver;
    ModifierProbDeriver* _postmodDeriver;

	HeadProbs* _headProbs;
    ModifierProbs* _premodProbs;
    ModifierProbs* _postmodProbs;
    LexicalProbs* _leftLexicalProbs;
    LexicalProbs* _rightLexicalProbs;

	string* _input_filenames;
	NgramScoreTable* _current_table;

	float _min;
	float _max;
	float _increment;
	
};



#endif
