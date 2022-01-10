// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/discourseRel/CrossSentRelation.h"


CrossSentRelation::CrossSentRelation(std::string Id, std::string relType, int sent1, int sent2){
	relId=Id;
	this->relType=relType;
	this->sent1=sent1;
	this->sent2=sent2;
}


void CrossSentRelation::setConnective(string conn){
	this->conn = conn;
}

void CrossSentRelation::setRelationPos(int relPos){
	this->relPos = relPos;
}

void CrossSentRelation::setSemType(std::string semType){
	this->semType = semType;
}

