// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_MENTION_H
#define EVENT_MENTION_H

#include "Generic/theories/Theory.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/Attribute.h"
#include "Generic/events/EventUtilities.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EventSet.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include <iostream>
#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/theories/SentenceItemIdentifier.h"
#include <vector>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED EventMention : public Theory {
protected:
	EventMentionUID _uid;

	Symbol _eventType;
	Symbol _patternId;
	Symbol _gainLoss;
	Symbol _indicator;

	int _event_id;
	int _sent_no;
	
	ModalityAttribute _modality;
	TenseAttribute _tense;
	PolarityAttribute _polarity;
	GenericityAttribute _genericity;

	Symbol _annotationID;

	const Proposition *_anchorProp;
	const SynNode *_anchorNode;

	typedef struct {
		Symbol role;
		const Mention *mention;
		float score;
	} EventArgument;

	typedef struct {
		Symbol role;
		const ValueMention *valueMention;
		float score;
	} EventValueArgument;

	
	typedef struct {
		Symbol role;
		int start_token;
		int end_token;
		float score;
		
	} EventSpanArgument; 

	std::vector<EventArgument> _arguments;

	std::vector<EventValueArgument> _valueArguments;

	std::vector<EventSpanArgument> _spanArguments;

	float _score;	

	// For ModalityClassifier - 2008/01/17
	//Symbol _modalityType;

public:
	EventMention(int sentno, int relno);
	EventMention(EventMention &other);	
	EventMention(EventMention &other, int index);
	EventMention(const EventMention &other, int sent_offset, int event_offset, const Parse* parse, const MentionSet* mentionSet, const ValueMentionSet* valueMentionSet, const PropositionSet* propSet, const ValueMentionSet* documentVaueMentionSet, ValueMentionSet::ValueMentionMap &documentValueMentionMap);

	EventMentionUID getUID() const { return _uid; }
	void setUID(int sentno, int relno) { _uid = EventMentionUID(sentno, relno); }
	int getEventID() const { return _event_id;}
	void setEventID(int event_id) { _event_id = event_id; }
	Symbol getAnnotationID() const { return _annotationID; }
	void setAnnotationID(Symbol sym) { _annotationID = sym; }

	int getSentenceNumber() const { return _sent_no; }
	const SynNode *getAnchorNode() const { return _anchorNode; }
	void setEventType(Symbol type) { _eventType = type; }
	void setPatternID(Symbol patternId) {_patternId = patternId; }
	void setGainLoss(Symbol gainLoss) {_gainLoss = gainLoss; }
	void setIndicator(Symbol indicator) {_indicator = indicator; }

	void setAnchor(const Proposition* prop);
	void setAnchor(const SynNode *node, const PropositionSet *propSet);

	void addArgument(Symbol role, const Mention *mention, float score = 0);
	void addValueArgument(Symbol role, const ValueMention *valueMention, float score = 0);
	void addSpanArgument(Symbol role, int startToken, int endToken, float score = 0);

	void resetArguments() { _arguments.clear(); _valueArguments.clear(); _spanArguments.clear();}

	// will just return the first one found: used in EELD where there is only one per role type
	const Mention *getFirstMentionForSlot(Symbol slotName) const;
	// will just return the first one found
	const ValueMention *getFirstValueForSlot(Symbol slotName) const;

	const Proposition *getAnchorProp() const { return _anchorProp; }

	int getNArgs() const { return (int) _arguments.size(); }
	int getNValueArgs() const { return (int) _valueArguments.size(); }
	int getNSpanArgs() const { return (int) _spanArguments.size(); }

	Symbol getNthArgRole(int n) const { return _arguments[n].role; }
	float getNthArgScore(int n) const { return _arguments[n].score; }
	void changeNthRole(int n, Symbol name) { if (n < (int) _arguments.size()) _arguments[n].role = name; }
	const Mention *getNthArgMention(int n) const { return _arguments[n].mention; }

	Symbol getNthArgValueRole(int n) const { return _valueArguments[n].role; }
	float getNthArgValueScore(int n) const { return _valueArguments[n].score; }
	void changeNthValueRole(int n, Symbol name) { if (n < (int) _valueArguments.size()) _valueArguments[n].role = name; }
	const ValueMention *getNthArgValueMention(int n) const { return _valueArguments[n].valueMention; }

	Symbol getNthArgSpanRole(int n) const { return _spanArguments[n].role; }
	float getNthArgSpanScore(int n) const { return _spanArguments[n].score; }
	void changeNthSpanRole(int n, Symbol name) { if (n < (int) _spanArguments.size()) _spanArguments[n].role = name; }
	int getNthArgSpanStartToken(int n) const { return _spanArguments[n].start_token; }
	int getNthArgSpanEndToken(int n) const { return _spanArguments[n].end_token; }

	Symbol getEventType() const { return _eventType; }
	Symbol getPatternId() const { return _patternId; }
	Symbol getGainLoss() const { return _gainLoss; }
	Symbol getIndicator() const { return _indicator; }
	Symbol getReducedEventType() const { return EventUtilities::getReduced2005EventType(_eventType); }
	float getScore() const { return _score; }
	void addToScore(float score) { _score += score; }
	void setScore(float score) { _score = score; }
	void setTense(TenseAttribute t) { _tense = t; }
	TenseAttribute getTense() const { return _tense; }
	void setModality(ModalityAttribute m) { _modality = m; }
	ModalityAttribute getModality() const { return _modality; }
	void setPolarity(PolarityAttribute p) { _polarity = p; }
	PolarityAttribute getPolarity() const { return _polarity; }
	void setGenericity(GenericityAttribute g) { _genericity = g; }
	GenericityAttribute getGenericity() const { return _genericity; }

	Symbol getRoleForMention(const Mention *mention) const;
	Symbol getRoleForValueMention(const ValueMention *valueMention) const;

	Event* getEvent(const DocTheory* docTheory) const;

	std::wstring toString(const TokenSequence *tokens = 0) const;
	std::string toDebugString(const TokenSequence *tokens = 0) const;

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
	explicit EventMention(SerifXML::XMLTheoryElement elem, int sentno, int relno);
	void resolvePointers(SerifXML::XMLTheoryElement elem, const Parse* parse);
	const wchar_t* XMLIdentifierPrefix() const;


	// For ModalityClassifier - 2008/01/17
	Symbol getModalityType() const;
	void setModalityType(Symbol mtype);
	
};

#endif
