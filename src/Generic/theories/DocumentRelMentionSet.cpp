// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "theories/DocumentRelMentionSet.h"

#include "theories/DocTheory.h"
#include "theories/RelMentionSet.h"
#include "theories/Mention.h"
#include "theories/RelMention.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/state/XMLTheoryElement.h"


/* this class is set to hold all the relMentionSet in a document
*/
DocumentRelMentionSet::DocumentRelMentionSet(DocTheory *docTheory):	
	_n_relMentions(0), _n_mentions(0),	_n_sentences(0), _docTheory(docTheory)
{
		for (int i = 0; i < docTheory->getNSentences(); i++) {
			SentenceTheory *sentTheory = docTheory->getSentenceTheory(i);
			RelMentionSet *rmSet = sentTheory->getRelMentionSet();
			MentionSet *mentSet = sentTheory->getMentionSet();
			addSentence(mentSet, rmSet);
		}
		_n_sentences = docTheory->getNSentences();;
}

//RelMentionUIDPair* DocumentRelMentionSet::getRelMention(int indx){
RelMention* DocumentRelMentionSet::getRelMention(int indx){
	if (indx < _n_relMentions)
		return _relMentions[indx];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"MentionSet::getMention()", _n_relMentions, indx);
}

Mention* DocumentRelMentionSet::getMentionByUID(MentionUID uid){
	if (_mentMap.get(uid)!=NULL)
		return _mentMap[uid];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"DocumentRelMentionSet::getMentionByUID()", -99, uid.toInt());
}

Mention* DocumentRelMentionSet::getMentionByIndex(int indx) {
	if (indx < _n_mentions)
		return _mentions[indx];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"DocumentRelMentionSet::getMentionByIndex()", -99, indx);
}

void DocumentRelMentionSet::addSentence(MentionSet *mentSet, RelMentionSet *relMentionSet)
{
	for(int i=0; i< relMentionSet->getNRelMentions(); i++){
//		RelMentionUIDPair *pair = createMentionUIDPair(relMentionSet->getRelMention(i), mentSet);
//		_relMentions.add(pair);
		_relMentions.add(relMentionSet->getRelMention(i));
		_n_relMentions++;
	}
	for(int j=0; j<mentSet->getNMentions(); j++){
		Mention * mention = mentSet->getMention(j);
		_mentMap[mention->getUID()] = mention;
		_mentions.add(mention);
		_n_mentions++;
	}
}

MentionSet* DocumentRelMentionSet::getSentenceMentionSet(int sent){
	return _docTheory->getSentenceTheory(sent)->getMentionSet();
}

RelMentionSet* DocumentRelMentionSet::getSentenceRelMentionSet(int sent){
	return _docTheory->getSentenceTheory(sent)->getRelMentionSet();
}
