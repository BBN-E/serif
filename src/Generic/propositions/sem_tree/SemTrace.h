// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SEM_TRACE_H
#define SEM_TRACE_H

#include <set>
#include "Generic/propositions/sem_tree/SemReference.h"

class SynNode;
class Mention;


class SemTrace : public SemReference {
protected:
	SemReference *_source;


public:
	SemTrace() : SemReference(0, 0), _source(0) {}


	virtual Type getSemNodeType() const { return TRACE_TYPE; }
	virtual SemTrace &asTrace() { return *this; }


	void setSource(SemReference *source);

	// this is a good candidate for the ugliest part of the program
	bool resolve(SemReference *awaiting_ref);

	virtual const Mention *getMention() const;
	virtual void dump(std::ostream &out, int indent) const;

	static bool containsDailyTemporalExpression(std::wstring);

private:
	static Symbol OTH_SYMBOL;
	static Symbol DATE_TAG;
	static Symbol COMMA_TAG;
	static Symbol PP_TAG;
};

#endif
