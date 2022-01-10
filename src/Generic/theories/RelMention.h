// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELMENTION_H
#define RELMENTION_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/Attribute.h"
#include "Generic/common/limits.h"
#include "Generic/common/UTF8OutputStream.h"
#include <iostream>
#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED RelMention : public Theory {
protected:
	RelMentionUID _uid;
	Symbol _type;
	Symbol _raw_type; // for _type == RelationConstants::RAW(_REVERSED)
	ModalityAttribute _modality;
	TenseAttribute _tense;

	// Argument mentions:
	const Mention *_lhs, *_rhs;

	const ValueMention *_timeArg;
	Symbol _timeRole;
	float _time_arg_score;

	float _score;
	float _confidence;
	
public:
	RelMention(const Mention *lhs, const Mention *rhs, Symbol type, int sentno, int relno, float score);
	RelMention(const Mention *lhs, const Mention *rhs, Symbol raw_type, int sentno, int relno);
	RelMention(RelMention &other);
	RelMention(RelMention &other, int sent_offset, const MentionSet* mentionSet, const ValueMentionSet* valueMentionSet);

	const Mention *getLeftMention() const { return _lhs; }
	const Mention *getRightMention() const { return _rhs; }
	const ValueMention *getTimeArgument() const { return _timeArg; }
	Symbol getTimeRole() const { return _timeRole; }
	float getTimeArgumentScore() const { return _time_arg_score; }
	Symbol getType() const { return _type; }
	Symbol getRawType() const { return _raw_type; }
	float getScore() const { return _score; }
	void setConfidence(float confidence) { _confidence = confidence; }
	float getConfidence() const { return _confidence; }
	RelMentionUID getUID() const { return _uid; }
	void setUID(int sentno, int relno) { _uid = RelMentionUID(sentno, relno); }
	void setTense(TenseAttribute t) { _tense = t; }
	TenseAttribute getTense() { return _tense; }
	void setModality(ModalityAttribute m) { _modality = m; }
	ModalityAttribute getModality() const { return _modality; }
	void setType(Symbol type) { _type = type; }

	Symbol getRoleForMention(const Mention *mention) const;
	Symbol getRoleForValueMention(const ValueMention *value_mention) const;

	void setTimeArgument(Symbol role, const ValueMention *time, float score = 0);
	void resetTimeArgument();

	std::wstring toString() const;
	std::string toDebugString() const;

	void dump(UTF8OutputStream &out, int indent) const;
	void dump(std::ostream &out, int indent) const;

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	void loadState(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit RelMention(SerifXML::XMLTheoryElement elem, int sentno, int relno);
	const wchar_t* XMLIdentifierPrefix() const;
};

#endif
