// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"
#include "RelationInstance.h"

void RelationInstance::populate(int index1, int index2, DTRelSentenceInfo *sentenceInfo, Symbol docID) {
	if (sentenceInfo == NULL) {
		throw UnexpectedInputException("RelationInstance::populate()", "NULL sentenceInfo not allowed");
	}
	_index1 = index1;
	_index2 = index2;
	_sentenceInfo = sentenceInfo;
	_docID = docID;
	_invalid = false;

	// make a unique name; we hash by this
	int sentenceNum = _sentenceInfo->mentionSets[0]->getSentenceNumber();
	const int length = 100;

	// this gross workaround is because wsprintf didn't work right in DEBUG mode
	char name[length+1];
	sprintf(name, "%s.%02d.%d-%d", docID.to_debug_string(), sentenceNum, _index1, _index2);
	wchar_t wideName[length+1];
	mbstowcs(wideName, name, length);

	_uid = Symbol(wideName);
}
