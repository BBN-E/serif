// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CROSS_SENT_REL_H
#define CROSS_SENT_REL_H

#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <string>

using namespace std;

class CrossSentRelation {
public:
	CrossSentRelation(string Id, string relType, int sent1, int sent2);
	~CrossSentRelation(){};

	void setSemType(string semType);
	void setConnective(string conn);
	void setRelationPos (int relPos);
	string getSemType(){return semType;}
	string getRelType(){return relType;}
	string Connective(){return conn;}
	int firstSentNo(){return sent1;}
	int secondSentNo(){return sent2;}
	int relationPosition(){return relPos;}
	string getRelId(){return relId;}

private:
	string relId;	//relation id -- docId%relPos%relType
	string relType;  //relation types: explicit, implicit, altLex, entRel 
	string semType;   //semantic type (for implicit, altLex)
	string conn;  //connective word for explicit, implicit
	int sent1;    //first sentence involving in this relation
	int sent2;	  //second sentence involving in this relation
	int relPos;   //relation "marker" positiion (for explicit, implicit) 

};

#endif
