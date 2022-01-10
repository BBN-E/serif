// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/descriptors/xx_CompoundMentionFinder.h"

namespace { 
	void defaultMsg(){
		std::cerr<<"<<<<<<<<<WARNING: Using unimplemented compound mention finder!>>>>>\n";
	}
}

GenericCompoundMentionFinder::GenericCompoundMentionFinder() { 
	defaultMsg(); }

Mention *GenericCompoundMentionFinder::findPartitiveWholeMention(MentionSet *mentionSet,
	Mention *baseMention){ 
		defaultMsg();
		return NULL; 
}
Mention **GenericCompoundMentionFinder::findAppositiveMemberMentions(MentionSet *mentionSet,
	Mention *baseMention) { 
		defaultMsg();
		return NULL; 
}
Mention **GenericCompoundMentionFinder::findListMemberMentions(MentionSet *mentionSet,
	Mention *baseMention) { 
		defaultMsg();
		return NULL; 
}
Mention *GenericCompoundMentionFinder::findNestedMention(MentionSet *mentionSet,
	Mention *baseMention) { 
		defaultMsg();
		return NULL; 
}
void GenericCompoundMentionFinder::coerceAppositiveMemberMentions(Mention** mentions){
	defaultMsg();
}
void GenericCompoundMentionFinder::setCorrectAnswers(class CorrectAnswers *correctAnswers) {
	defaultMsg();
}
void GenericCompoundMentionFinder::setCorrectDocument(class CorrectDocument *correctDocument) {
	defaultMsg();
}
void GenericCompoundMentionFinder::setSentenceNumber(int sentno) {
	defaultMsg();
}

