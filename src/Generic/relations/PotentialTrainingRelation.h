// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef POTENTIAL_RELATION_H
#define POTENTIAL_RELATION_H

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/Offset.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/Mention.h"

#include <iostream>


class MentionSet;
class Argument;
class Proposition;
class DocTheory;
class SynNode;
class Symbol;
class LocatedString;
class TokenSequence;
class PotentialRelationInstance;
class UTF8InputStream;


class PotentialTrainingRelation {
public:
	
	PotentialTrainingRelation();
	PotentialTrainingRelation(const PotentialTrainingRelation& other);
	PotentialTrainingRelation(Argument *first, Argument *second, 
							  DocTheory *docTheory, const int sent_no,
							  bool leftMetonymy = false, bool rightMetonymy = false);
	PotentialTrainingRelation(Mention *first, Mention *second,
	 						  DocTheory *docTheory, const int sent_no,
					          bool leftMetonymy = false, bool rightMetonymy = false); 
	PotentialTrainingRelation(UTF8InputStream& in);

	~PotentialTrainingRelation() {
		if (_sentence != NULL) { delete [] _sentence; }
	}

	const PotentialTrainingRelation &operator=(const PotentialTrainingRelation &other);
	bool operator==(const PotentialTrainingRelation &other) const;
	bool operator!=(const PotentialTrainingRelation &other) const; 

	void dump(std::ostream &out, int indent = 0) const;
	void dump(UTF8OutputStream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out, const PotentialTrainingRelation &it)
		{ it.dump(out, 0); return out; }
	friend UTF8OutputStream &operator <<(UTF8OutputStream &out, const PotentialTrainingRelation &it)
		{ it.dump(out, 0); return out; }
	void read(UTF8InputStream &in);
	friend UTF8InputStream &operator >>(UTF8InputStream &in, PotentialTrainingRelation &it)
		{ it.read(in); return in; }

	size_t hash_code() const { return _edt_start1.value() + _edt_end1.value() + _edt_start2.value() + _edt_end2.value(); }

	std::wstring toString(int indent = 0) const;
	std::string toDebugString(int indent = 0) const;

	const Symbol getRelationType() { return _relationType; }

	MentionUID getLeftMention() const { return _leftMention; }
	MentionUID getRightMention() const { return _rightMention; }

	bool relationIsReversed() const { return _relation_reversed; }
	bool argsAreReversed() const { return _args_reversed; }

	static const SynNode* getEDTHead(const Mention* ment, const MentionSet *mentionSet);

	EntityType _type1;
	EntityType _type2;

private:
	Symbol _docName;
	const wchar_t *_sentence;

	MentionUID _leftMention;
	MentionUID _rightMention;

	int _start1;
	int _start2;
	int _end1;
	int _end2;

	EDTOffset _edt_start1;
	EDTOffset _edt_start2;
	EDTOffset _edt_end1;
	EDTOffset _edt_end2;

	Symbol _relationType;
	bool _relation_reversed;

	// CAUTION: _args_reversed only gets set when the system produces the relation, not when it
	// gets read in from annotation. Refers to the order of the arguments as produced by the
	// combination of propositions and relation finding.
	bool _args_reversed;     

	void populateOffsets(const LocatedString *sent, TokenSequence *tokenSequence, 
						const SynNode *head1, const SynNode *head2);

	static const wchar_t* getDisplaySentence(const wchar_t *sentence);

	static Symbol UNDET_RELATION_SYM;
	static Symbol REVERSED_SYM;
};

#endif
