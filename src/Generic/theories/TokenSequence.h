// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TOKEN_SEQUENCE_H
#define TOKEN_SEQUENCE_H

#include "Generic/common/Symbol.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/common/limits.h"
#include "Generic/theories/SentenceTheory.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;


// TokenSequence represents a tokenization theory, and consists of
// a sequence of tokens, a sentence number, and a score.
class SERIF_EXPORTED TokenSequence : public SentenceSubtheory {

protected:
	int _sent_no;
	int _n_tokens;
	Token **_tokens;
	float _score;
	mutable std::wstring _stringStore;

public:
	/** Construct a new TokenSequence with no tokens, whose sent_no
	 * is zero and whose score is zero. */
	TokenSequence()
		: _sent_no(0), _n_tokens(0), _tokens(0), _score(0) {}

	/** Construct a new token sequence containing the given tokens,
	 * with the specified sentence number and score.  This constructor
	 * takes ownership of the given tokens.  */
	TokenSequence(int sent_no, int n_tokens, 
	              Token** tokens, float score=0);

	TokenSequence(const TokenSequence &other, int sent_offset = 0);

	virtual ~TokenSequence();

	virtual SentenceTheory::SubtheoryType getSubtheoryType() const
	{ return SentenceTheory::TOKEN_SUBTHEORY; }

	/** Return the sentence number of the sentence whose tokens are
		contained in this token sequence. */
	int getSentenceNumber() const { return _sent_no; }

	/** Return the score of this token sequence. */
	float getScore() const { return _score; }

	/** Modify the score of this token sequence. */
	void setScore(float score) { _score = score; }

	/** Return the number of tokens in this token sequence. */
	int getNTokens() const { return _n_tokens; }

	/** Return the i-th token in this token sequence.  A bounds check
		is performed on `i`, and if it is not within the appropriate
		bounds, then an InternalInconsistencyException is raised. */
	const Token *getToken(int i) const;
	const Token *getToken(size_t i) const { 
		return getToken(static_cast<int>(i)); }

	/** Destroy the old tokenization and replace it with a new
	 * tokenization.  The TokenSequence takes ownership of the given
	 * tokens.  Warning: If you use this, you are playing with
	 * fire. Make sure any references to token numbers in other
	 * subtheories are consistent with the new tokenization. And you
	 * probably don't want to use this with any diversity turned on. */
	virtual void retokenize(int n_tokens, Token* tokens[]);

	/** Return a string representation of this token sequence.
		NOTE: NOT exactly equivalent to toString() from 0 to end. 
		For whatever reasons, this returns the toString() value 
		surrounded by parens, e.g. "(I am a sentence .)" */
	virtual std::wstring toString() const;

	/** Return a string representation of the given range of tokens in
		this token sequence */
	virtual std::wstring toString(int start_token_inclusive, int end_token_inclusive) const;

	/** Return a narrow-string representation of the given range of
		tokens in this token sequence. */
	virtual std::string toDebugString(int start_token_inclusive, int end_token_inclusive) const;

	/** Return a narrow-string representation of this token sequence. 
		Exactly equivalent to toDebugString() from 0 to end. */
	virtual std::string toDebugString() const;

	/** Return a string representation of this token sequence that
		includes information about the EDT offset of each token. */
	virtual std::wstring toStringWithOffsets() const;

	/** Return a string representation of this token sequence formed
		by concatenating the token symbols without inserting any
		spaces between them. */
	virtual std::wstring toStringNoSpaces() const;

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out, const TokenSequence &it)
	{ it.dump(out, 0); return out; }

	typedef enum {TOKEN_SEQUENCE, LEXICAL_TOKEN_SEQUENCE} TokenSequenceType;

	/** Set the "default token sequence type".  This determines what kind
	  * of object is created during state-file deserialization. */
	static void setTokenSequenceTypeForStateLoading(TokenSequenceType type);
	static TokenSequenceType getTokenSequenceTypeForStateLoading();

	// For saving state:
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	explicit TokenSequence(StateLoader* stateLoader);
	// For loading state:
	virtual void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	explicit TokenSequence(SerifXML::XMLTheoryElement elem, int sent_no);
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual const wchar_t* XMLIdentifierPrefix() const;

};

#endif
