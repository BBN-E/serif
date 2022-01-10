// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOCUMENT_REL_MENTION_SET_H
#define DOCUMENT_REL_MENTION_SET_H

#include "Generic/common/GrowableArray.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Theory.h"
#include "Generic/theories/Mention.h"

class DocTheory;
class RelMention;
class Mention;
class MentionSet;
class RelMentionSet;

/* This class provides a convenient way to access the relation mentions
 * in a sentence.  It is *not* a theory class -- i.e., it is not used to
 * *store* the annotations as part of the DocTheory.  Instead, it is built
 * based on a DocTheory, and is used to provide access into that DocTheory.
 *
 * In particular, note that a DocTheory's "documentRelMentionSet" attribute
 * does *NOT* contain a DocumentRelMentionSet!  Instead, it contains a 
 * RelMentionSet!  This nomenclature mismatch should probably be fixed --
 * in particular, this class (DocRelMentionSet) should probably be renamed.
 */
class DocumentRelMentionSet {
public:


	DocumentRelMentionSet(DocTheory *docTheory);

	int getNSentences() const{ return _n_sentences; }
	int getNRelMentions() const{ return _n_relMentions; }
	RelMention* getRelMention(int indx);

	int getNMentions() const { return _n_mentions; }
	Mention* getMentionByIndex(int indx);
	Mention* getMentionByUID(MentionUID uid);
	MentionSet* getSentenceMentionSet(int sent);
	RelMentionSet* getSentenceRelMentionSet(int sent);
protected:

	DocTheory *_docTheory;

	void addSentence(MentionSet *mentSet, RelMentionSet *relMentionSet);
//	RelMentionUIDPair* createMentionUIDPair(RelMention *relMention, MentionSet *mentSet);

	//hash to keep track of coref ids
	struct HashKey {
        size_t operator()(const MentionUID a) const {
			return a.toInt();
        }
	};

    struct EqualKey {
        bool operator()(const MentionUID &a, const MentionUID &b) const {
			return (a == b);
		}
	}; 
	typedef serif::hash_map<MentionUID, Mention *, HashKey, EqualKey> MentionMap;
	MentionMap _mentMap;
	
//	GrowableArray<RelMentionUIDPair *> _relMentions;
	GrowableArray<RelMention *> _relMentions;
	GrowableArray<Mention *> _mentions;

	int _n_relMentions;
	int _n_mentions;
	int _n_sentences;
};

#endif
