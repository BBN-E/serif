// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CORRECT_RELATION_H
#define CORRECT_RELATION_H

#include "Generic/CASerif/correctanswers/CorrectRelMention.h"
#include "Generic/common/Sexp.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/Attribute.h"


class CorrectRelation
{
private:
	
	int _system_relation_id;
	Symbol _relationType;
	bool _is_explicit;
	Symbol _modality;
	Symbol _tense;

	int _n_mentions;
	CorrectRelMention *_mentions;

	Symbol _annotationID;
	bool _isRelationType(Symbol type);
	
public:
	CorrectRelation();
	~CorrectRelation();

	void loadFromSexp(Sexp *relationSexp);
	//void loadFromERE(ERE *relationERE);
	int getNMentions() { return _n_mentions; }
	CorrectRelMention * getMention(size_t index) { return &(_mentions[index]); }
	Symbol getRelationType() { return _relationType; }
	int getSystemRelationID() { return _system_relation_id; }
	void setSystemRelationID(int id) { _system_relation_id = id; }
	bool isExplicit() { return _is_explicit; }
	Symbol getAnnotationID() { return _annotationID; }

	ModalityAttribute getModality() {
		if (_modality == CASymbolicConstants::RDR_MOD_ASSERTED)
			return Modality::ASSERTED;
		else if (_modality == CASymbolicConstants::RDR_MOD_OTHER)
			return Modality::OTHER;
		else return Modality::ASSERTED;
	}
	TenseAttribute getTense() {
		if (_tense == CASymbolicConstants::RDR_TENSE_FUTURE)
			return Tense::FUTURE;
		else if (_tense == CASymbolicConstants::RDR_TENSE_PAST)
			return Tense::PAST;
		else if (_tense == CASymbolicConstants::RDR_TENSE_PRESENT)
			return Tense::PRESENT;
		else if (_tense == CASymbolicConstants::RDR_TENSE_UNSPECIFIED)
			return Tense::UNSPECIFIED;
		else return Tense::UNSPECIFIED;
	}

};

#endif
