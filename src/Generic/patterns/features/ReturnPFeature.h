// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef RETURN_PFEATURE_H
#define RETURN_PFEATURE_H

#include "Generic/common/Symbol.h"
#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/common/BoostUtil.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/patterns/PatternReturn.h"
#include <boost/make_shared.hpp>

class EventMention;
class Proposition;
class SynNode;
class RelMention;
typedef boost::shared_ptr<PatternReturn> PatternReturn_ptr;

/** Abstract base class for "return features," which are used to store the
  * value (or values) returned by a pattern that defines a return label.
  * A separate subclass is defined for each type of value that can be 
  * returned (eg MentionReturnPFeature for Mentions).  These subclasses
  * are defined below (in this file). 
  */
class ReturnPatternFeature : public PatternFeature {
public:
	// Return value accessors.
	Symbol getReturnLabel() const;
	std::wstring getReturnValue(const std::wstring & key) const;
	bool hasReturnValue(const std::wstring & key) const;
	PatternReturn_ptr getPatternReturn() { return _return; }
	// Return value modifiers.
	void setReturnValue(const std::wstring & key, const std::wstring & value);
    void setPatternReturn(const PatternReturn_ptr patternReturn); // makes a copy

	//iteration
	std::map<std::wstring, std::wstring>::const_iterator begin(void) const;
	std::map<std::wstring, std::wstring>::const_iterator end(void) const;

	// Top-Level accessors & modifiers.
	bool isTopLevel(void) const { return _toplevel; }
	void setTopLevel(bool topLevel) { _toplevel = topLevel; }

	// startToken/endToken always give the entire sentence, unless it's a token span
	virtual int getStartToken() const { return 0; }
	virtual int getEndToken() const { return _end_token; }
	virtual void setCoverage(const DocTheory * docTheory);
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher);

	virtual bool equals(PatternFeature_ptr other) { 
		boost::shared_ptr<ReturnPatternFeature> f = boost::dynamic_pointer_cast<ReturnPatternFeature>(other);
		return f && getPatternReturn()->equals(f->getPatternReturn()) && returnValueIsEqual(other); 
	}
	virtual bool returnValueIsEqual(PatternFeature_ptr other) const = 0;

	/** Return a string name for the return type.  This should be used for display/serialization
	  * only; to check the type of a pattern, use boost::dynamic_pointer_cast. */
	virtual const wchar_t* getReturnTypeName() = 0;

	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	ReturnPatternFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
protected:
	/** Create a new ReturnPatternFeature.  The PatternReturn value for this new
	  * ReturnPatternFeature is a *copy* of the pattern's PatternReturn -- i.e.,
	  * calls to setReturnValue will affect only this feature's PatternReturn, and
	  * not the one owned by the pattern that generated this feature. */
	ReturnPatternFeature(Pattern_ptr pattern, const LanguageVariant_ptr& languageVariant, float confidence=1);

	/** Create a new ReturnPatternFeature with the given return label. (Used by 
	  * SignleSourceAnswer::createNewFeature() for deserialization.) */
	ReturnPatternFeature(Symbol returnLabel, const LanguageVariant_ptr& languageVariant, float confidence=1);
	
	/** Create a new ReturnPatternFeature.  The PatternReturn value for this new
	  * ReturnPatternFeature is a *copy* of the pattern's PatternReturn -- i.e.,
	  * calls to setReturnValue will affect only this feature's PatternReturn, and
	  * not the one owned by the pattern that generated this feature. */
	ReturnPatternFeature(Pattern_ptr pattern, float confidence=1);

	/** Create a new ReturnPatternFeature with the given return label. (Used by 
	  * SignleSourceAnswer::createNewFeature() for deserialization.) */
	ReturnPatternFeature(Symbol returnLabel, float confidence=1);

	// Helper method for subclass printFeatureFocus methods: display this feature
	// focus with the given symbol as val_extra_2 and the given int as val_extra_3.
	void printFeatureFocusHelper(PatternMatcher_ptr patternMatcher, UTF8OutputStream &out, const Symbol& val_extra_2_value) const;
	void printFeatureFocusHelper(PatternMatcher_ptr patternMatcher, UTF8OutputStream &out, const Symbol& val_extra_2_value, 
		int val_extra_3_value) const;
	void printFeatureFocusHelper(PatternMatcher_ptr patternMatcher, UTF8OutputStream &out, const Symbol& val_extra_2_value, 
		float val_extra_3_value) const;

	/** The PatternReturn object for this return feature.  This stores either a label
	  * for the return value, or a set of key/value pairs. */
	PatternReturn_ptr _return;

	/** The end token.  For all ReturnPatternFeatures except for 
	  * TokenSpanReturnPFeature, this is set to the end of the selected
	  * sentence. */
	int _end_token;

	/** True if this is a top-level return feature */
	bool _toplevel;

	/** Print the beginning part of the feature focus for return features, which is shared
	  * by all the return features. */
	void printFeatureFocusHeader(PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const;
};

/*************************************************************************
 * Return Pattern Feature: Mention
 *************************************************************************/
class MentionReturnPFeature: public ReturnPatternFeature {
private:
	MentionReturnPFeature(Pattern_ptr pattern, const Mention* mention, const Symbol &matchSym, 
		const LanguageVariant_ptr& languageVariant, float confidence, bool is_focus = false)
		: ReturnPatternFeature(pattern, languageVariant, confidence), _mention(mention), _matchSym(matchSym), _is_focus(is_focus) {}
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(MentionReturnPFeature, Pattern_ptr, const Mention*, const Symbol&, 
		 const LanguageVariant_ptr&, float);
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(MentionReturnPFeature, Pattern_ptr, const Mention*, const Symbol&, 
		 const LanguageVariant_ptr&, float, bool);
	MentionReturnPFeature(Symbol returnLabel, const Mention* mention, const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(returnLabel, languageVariant), _mention(mention), _matchSym(), _is_focus(false) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(MentionReturnPFeature, Symbol, const Mention*, 
		 const LanguageVariant_ptr&);
		 
	MentionReturnPFeature(Pattern_ptr pattern, const Mention* mention, const Symbol &matchSym, 
		float confidence, bool is_focus = false)
		: ReturnPatternFeature(pattern, confidence), _mention(mention), _matchSym(matchSym), _is_focus(is_focus) {}
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(MentionReturnPFeature, Pattern_ptr, const Mention*, const Symbol&, float);
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(MentionReturnPFeature, Pattern_ptr, const Mention*, const Symbol&, float, bool);
	MentionReturnPFeature(Symbol returnLabel, const Mention* mention)
		: ReturnPatternFeature(returnLabel), _mention(mention), _matchSym(), _is_focus(false) {}
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(MentionReturnPFeature, Symbol, const Mention*);
public:
	const Mention* getMention() const { return _mention; }
	Symbol getMatchSym(void) const {return _matchSym; };
	bool isFocus(void) const { return _is_focus; }
	int getSentenceNumber() const { return _mention->getSentenceNumber(); }
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
		printFeatureFocusHelper(patternMatcher, out, L"mention", _mention->getUID().toInt()); }
	virtual bool returnValueIsEqual(PatternFeature_ptr other_feature) const {
		boost::shared_ptr<MentionReturnPFeature> other = boost::dynamic_pointer_cast<MentionReturnPFeature>(other_feature);
		return (other && other->getMention()==getMention()); }
	virtual const wchar_t* getReturnTypeName() { return L"mention"; }
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	MentionReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
private:
	const Mention *_mention;
	Symbol _matchSym;
	bool _is_focus;
};

/*************************************************************************
 * Return Pattern Feature: EventMention
 *************************************************************************/
class EventMentionReturnPFeature: public ReturnPatternFeature {
private:
	EventMentionReturnPFeature(Pattern_ptr pattern, const EventMention* eventMention, int sent_no,
		const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(pattern,languageVariant), _eventMention(eventMention), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(EventMentionReturnPFeature, Pattern_ptr, const EventMention*, int,
		 const LanguageVariant_ptr&);
	EventMentionReturnPFeature(Symbol returnLabel, const EventMention* eventMention, int sent_no,
		const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(returnLabel,languageVariant), _eventMention(eventMention), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(EventMentionReturnPFeature, Symbol, const EventMention*, int,
		 const LanguageVariant_ptr&);
		 
	EventMentionReturnPFeature(Pattern_ptr pattern, const EventMention* eventMention, int sent_no)
		: ReturnPatternFeature(pattern), _eventMention(eventMention), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(EventMentionReturnPFeature, Pattern_ptr, const EventMention*, int);
	EventMentionReturnPFeature(Symbol returnLabel, const EventMention* eventMention, int sent_no)
		: ReturnPatternFeature(returnLabel), _eventMention(eventMention), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(EventMentionReturnPFeature, Symbol, const EventMention*, int);
public:
	const EventMention* getEventMention() const { return _eventMention; }
	int getSentenceNumber() const { return _sent_no; }
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
		printFeatureFocusHelper(patternMatcher, out, L"event", _eventMention->getUID().toInt()); }
	virtual bool returnValueIsEqual(PatternFeature_ptr other_feature) const {
		boost::shared_ptr<EventMentionReturnPFeature> other = boost::dynamic_pointer_cast<EventMentionReturnPFeature>(other_feature);
		return (other && other->getEventMention()==getEventMention()); }
	virtual const wchar_t* getReturnTypeName() { return L"event"; }
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	EventMentionReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
private:
	const EventMention *_eventMention;
	int _sent_no;
};

/*************************************************************************
 * Return Pattern Feature: RelMention
 *************************************************************************/
class RelMentionReturnPFeature: public ReturnPatternFeature {
private:
	RelMentionReturnPFeature(Pattern_ptr pattern, const RelMention* relMention, int sent_no,
		const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(pattern,languageVariant), _relMention(relMention), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(RelMentionReturnPFeature, Pattern_ptr, const RelMention*, int,
		 const LanguageVariant_ptr&);
	RelMentionReturnPFeature(Symbol returnLabel, const RelMention* relMention, int sent_no,
		const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(returnLabel,languageVariant), _relMention(relMention), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(RelMentionReturnPFeature, Symbol, const RelMention*, int,
		 const LanguageVariant_ptr&);
		 
	RelMentionReturnPFeature(Pattern_ptr pattern, const RelMention* relMention, int sent_no)
		: ReturnPatternFeature(pattern), _relMention(relMention), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(RelMentionReturnPFeature, Pattern_ptr, const RelMention*, int);
	RelMentionReturnPFeature(Symbol returnLabel, const RelMention* relMention, int sent_no)
		: ReturnPatternFeature(returnLabel), _relMention(relMention), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(RelMentionReturnPFeature, Symbol, const RelMention*, int);
public:
	const RelMention* getRelMention() const { return _relMention; }
	int getSentenceNumber() const { return _sent_no; }
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
		printFeatureFocusHelper(patternMatcher, out, L"relation", _relMention->getUID().toInt()); }
	virtual bool returnValueIsEqual(PatternFeature_ptr other_feature) const {
		boost::shared_ptr<RelMentionReturnPFeature> other = boost::dynamic_pointer_cast<RelMentionReturnPFeature>(other_feature);
		return (other && other->getRelMention()==getRelMention()); }
	virtual const wchar_t* getReturnTypeName() { return L"relation"; }
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	RelMentionReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
private:
	const RelMention *_relMention;
	int _sent_no;
};

/*************************************************************************
 * Return Pattern Feature: Proposition
 *************************************************************************/
class PropositionReturnPFeature: public ReturnPatternFeature {
private:
	PropositionReturnPFeature(Pattern_ptr pattern, const Proposition* prop, int sent_no,
		const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(pattern,languageVariant), _prop(prop), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(PropositionReturnPFeature, Pattern_ptr, const Proposition*, int,
		 const LanguageVariant_ptr&);
	PropositionReturnPFeature(Symbol returnLabel, const Proposition* prop, int sent_no,
		const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(returnLabel,languageVariant), _prop(prop), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(PropositionReturnPFeature, Symbol, const Proposition*, int,
		 const LanguageVariant_ptr&);
		 
	PropositionReturnPFeature(Pattern_ptr pattern, const Proposition* prop, int sent_no)
		: ReturnPatternFeature(pattern), _prop(prop), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(PropositionReturnPFeature, Pattern_ptr, const Proposition*, int);
	PropositionReturnPFeature(Symbol returnLabel, const Proposition* prop, int sent_no)
		: ReturnPatternFeature(returnLabel), _prop(prop), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(PropositionReturnPFeature, Symbol, const Proposition*, int);
public:
	const Proposition* getProp() const { return _prop; }
	int getSentenceNumber() const { return _sent_no; }
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
		printFeatureFocusHelper(patternMatcher, out, L"prop", _prop->getID()); }
	virtual bool returnValueIsEqual(PatternFeature_ptr other_feature) const {
		boost::shared_ptr<PropositionReturnPFeature> other = boost::dynamic_pointer_cast<PropositionReturnPFeature>(other_feature);
		return (other && other->getProp()==getProp()); }
	virtual const wchar_t* getReturnTypeName() { return L"prop"; }
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	PropositionReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
private:
	const Proposition *_prop;
	int _sent_no;
};


/*************************************************************************
 * Return Pattern Feature: Parse Node
 *************************************************************************/
class ParseNodeReturnPFeature: public ReturnPatternFeature {
private:
	ParseNodeReturnPFeature(Pattern_ptr pattern, const SynNode* node, int sent_no,
		const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(pattern,languageVariant), _node(node), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(ParseNodeReturnPFeature, Pattern_ptr, const SynNode*, int,
		 const LanguageVariant_ptr&);
	ParseNodeReturnPFeature(Symbol returnLabel, const SynNode* node, int sent_no,
		const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(returnLabel,languageVariant), _node(node), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(ParseNodeReturnPFeature, Symbol, const SynNode*, int,
		 const LanguageVariant_ptr&);
		 
	ParseNodeReturnPFeature(Pattern_ptr pattern, const SynNode* node, int sent_no)
		: ReturnPatternFeature(pattern), _node(node), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(ParseNodeReturnPFeature, Pattern_ptr, const SynNode*, int);
	ParseNodeReturnPFeature(Symbol returnLabel, const SynNode* node, int sent_no)
		: ReturnPatternFeature(returnLabel), _node(node), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(ParseNodeReturnPFeature, Symbol, const SynNode*, int);
public:
	const SynNode* getNode() const { return _node; }
	int getSentenceNumber() const { return _sent_no; }
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
		printFeatureFocusHelper(patternMatcher, out, L"parse-node", _node->getID()); }
	virtual bool returnValueIsEqual(PatternFeature_ptr other_feature) const {
		boost::shared_ptr<ParseNodeReturnPFeature> other = boost::dynamic_pointer_cast<ParseNodeReturnPFeature>(other_feature);
		return (other && other->getNode()==getNode()); }
	virtual const wchar_t* getReturnTypeName() { return L"parse-node"; }
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	ParseNodeReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
private:
	const SynNode *_node;
	int _sent_no;
};

/*************************************************************************
 * Return Pattern Feature: ValueMention
 *************************************************************************/
class ValueMentionReturnPFeature: public ReturnPatternFeature {
private:
	ValueMentionReturnPFeature(Pattern_ptr pattern, const ValueMention* valueMention,
		const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(pattern,languageVariant), _valueMention(valueMention), _sent_no(valueMention->getSentenceNumber()) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(ValueMentionReturnPFeature, Pattern_ptr, const ValueMention*,
		 const LanguageVariant_ptr&);
	ValueMentionReturnPFeature(Symbol returnLabel, const ValueMention* valueMention,
		const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(returnLabel,languageVariant), _valueMention(valueMention), _sent_no(valueMention->getSentenceNumber()) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(ValueMentionReturnPFeature, Symbol, const ValueMention*,
		 const LanguageVariant_ptr&);
		 
	ValueMentionReturnPFeature(Pattern_ptr pattern, const ValueMention* valueMention)
		: ReturnPatternFeature(pattern), _valueMention(valueMention), _sent_no(valueMention->getSentenceNumber()) {}
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(ValueMentionReturnPFeature, Pattern_ptr, const ValueMention*);
	ValueMentionReturnPFeature(Symbol returnLabel, const ValueMention* valueMention)
		: ReturnPatternFeature(returnLabel), _valueMention(valueMention), _sent_no(valueMention->getSentenceNumber()) {}
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(ValueMentionReturnPFeature, Symbol, const ValueMention*);
public:
	const ValueMention* getValueMention() const { return _valueMention; }
	int getSentenceNumber() const { return _sent_no; }
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
		printFeatureFocusHelper(patternMatcher, out, L"value", _valueMention->getUID().toInt()); }
	virtual bool returnValueIsEqual(PatternFeature_ptr other_feature) const {
		boost::shared_ptr<ValueMentionReturnPFeature> other = boost::dynamic_pointer_cast<ValueMentionReturnPFeature>(other_feature);
		return (other && other->getValueMention()==getValueMention()); }
	virtual const wchar_t* getReturnTypeName() { return L"value"; }
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	ValueMentionReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
private:
	const ValueMention *_valueMention;
	int _sent_no;
};

/*************************************************************************
 * Return Pattern Feature: DocumentDateReturnPFeature
 *************************************************************************/
class DocumentDateReturnPFeature: public ReturnPatternFeature {
private:
	DocumentDateReturnPFeature(Pattern_ptr pattern, std::wstring formattedDate, int sent_no,
		const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(pattern,languageVariant), _formattedDate(formattedDate), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(DocumentDateReturnPFeature, Pattern_ptr, std::wstring, int,
		 const LanguageVariant_ptr&);
	DocumentDateReturnPFeature(Symbol returnLabel, std::wstring formattedDate, int sent_no,
		const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(returnLabel,languageVariant),_formattedDate(formattedDate), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(DocumentDateReturnPFeature, Symbol, std::wstring, int,
		 const LanguageVariant_ptr&);
		 
	DocumentDateReturnPFeature(Pattern_ptr pattern, std::wstring formattedDate, int sent_no)
		: ReturnPatternFeature(pattern), _formattedDate(formattedDate), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(DocumentDateReturnPFeature, Pattern_ptr, std::wstring, int);
	DocumentDateReturnPFeature(Symbol returnLabel, std::wstring formattedDate, int sent_no)
		: ReturnPatternFeature(returnLabel),_formattedDate(formattedDate), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(DocumentDateReturnPFeature, Symbol, std::wstring, int);
public:
	std::wstring getFormattedDate() const { return _formattedDate; }
	int getSentenceNumber() const { return _sent_no; }
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
		printFeatureFocusHelper(patternMatcher, out, L":NULL"); }
	virtual bool returnValueIsEqual(PatternFeature_ptr other_feature) const {
		boost::shared_ptr<DocumentDateReturnPFeature> other = boost::dynamic_pointer_cast<DocumentDateReturnPFeature>(other_feature);
		return (other && other->getFormattedDate()==getFormattedDate()); }
	virtual const wchar_t* getReturnTypeName() { return L"document-date"; }
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	DocumentDateReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
private:
	std::wstring _formattedDate;
	int _sent_no;
};

/*************************************************************************
 * Return Pattern Feature: DateSpecReturnPFeature
 *************************************************************************/
class DateSpecReturnPFeature: public ReturnPatternFeature {
public:
	//	DateSpecReturnPFeature_ptr dateSpecFeature = boost::make_shared<DateSpecReturnPFeature>(Symbol(L"inferredSpecString"), specString, dateMention->getSentenceNumber(), empty , dateMention); 

	DateSpecReturnPFeature(Pattern_ptr pattern, std::wstring specString, int sent_no, 
		const ValueMention* valueMention1, const ValueMention* valueMention2, const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(pattern,languageVariant), _specString(specString), _sent_no(sent_no), _valueMention1(valueMention1), _valueMention2(valueMention2) {}
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(DateSpecReturnPFeature, Pattern_ptr, std::wstring, int, 
		const ValueMention*, const ValueMention*, const LanguageVariant_ptr&);


	DateSpecReturnPFeature(Symbol returnLabel, std::wstring specString, int sent_no, 
		const ValueMention* valueMention1, const ValueMention* valueMention2, const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(returnLabel,languageVariant),  _specString(specString), _sent_no(sent_no), _valueMention1(valueMention1), _valueMention2(valueMention2){}
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(ValueMentionReturnPFeature, Symbol, std::wstring, int, const ValueMention*, const ValueMention*,
		 const LanguageVariant_ptr&); 
		 
	DateSpecReturnPFeature(Pattern_ptr pattern, std::wstring specString, int sent_no, 
		const ValueMention* valueMention1, const ValueMention* valueMention2)
		: ReturnPatternFeature(pattern), _specString(specString), _sent_no(sent_no), _valueMention1(valueMention1), _valueMention2(valueMention2) {}
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(DateSpecReturnPFeature, Pattern_ptr, std::wstring, int, 
		const ValueMention*, const ValueMention*);

	DateSpecReturnPFeature(Symbol returnLabel, std::wstring specString, int sent_no, 
		const ValueMention* valueMention1, const ValueMention* valueMention2)
		: ReturnPatternFeature(returnLabel),  _specString(specString), _sent_no(sent_no), _valueMention1(valueMention1), _valueMention2(valueMention2){}
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(ValueMentionReturnPFeature, Symbol, std::wstring, int, const ValueMention*, const ValueMention*); 
public:
	const ValueMention* getValueMention1() const { return _valueMention1; }
	const ValueMention* getValueMention2() const { return _valueMention2; }
	std::wstring getDateSpecString() const {return _specString; }
	int getSentenceNumber() const { return _sent_no; }
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
		printFeatureFocusHelper(patternMatcher, out, L"value", _valueMention1->getUID().toInt()); }
	virtual bool returnValueIsEqual(PatternFeature_ptr other_feature) const {
		boost::shared_ptr<DateSpecReturnPFeature> other = boost::dynamic_pointer_cast<DateSpecReturnPFeature>(other_feature);
		return (other && other->getValueMention1()==getValueMention1() &&
			other->getValueMention2()==getValueMention2()) && _specString == other->getDateSpecString(); }
	virtual const wchar_t* getReturnTypeName() { return L"value"; }
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	DateSpecReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
private:
	const ValueMention* _valueMention1;
	const ValueMention* _valueMention2;
	std::wstring _specString;
	int _sent_no;
};

/*************************************************************************
 * Return Pattern Feature: Generic
 *************************************************************************/
class GenericReturnPFeature: public ReturnPatternFeature {
private:
	GenericReturnPFeature(Pattern_ptr pattern, int sent_no, const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(pattern,languageVariant), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(GenericReturnPFeature, Pattern_ptr, int, const LanguageVariant_ptr&);
	
	GenericReturnPFeature(Pattern_ptr pattern, int sent_no)
		: ReturnPatternFeature(pattern), _sent_no(sent_no) {}
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(GenericReturnPFeature, Pattern_ptr, int);
public:
	int getSentenceNumber() const { return _sent_no; }
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
		printFeatureFocusHelper(patternMatcher, out, L":NULL"); }
	virtual bool returnValueIsEqual(PatternFeature_ptr other_feature) const {
		boost::shared_ptr<GenericReturnPFeature> other = boost::dynamic_pointer_cast<GenericReturnPFeature>(other_feature);
		return (other && other->getSentenceNumber()==getSentenceNumber()); }
	virtual const wchar_t* getReturnTypeName() { return L"generic"; }
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	GenericReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
private:
	int _sent_no;
};

/*************************************************************************
 * Return Pattern Feature: Topic
 *************************************************************************/
class TopicReturnPFeature: public ReturnPatternFeature {
private:
	TopicReturnPFeature(Pattern_ptr pattern, int sent_no, float relevance, Symbol querySlot, const Mention* mention,
		const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(pattern,languageVariant), _sent_no(sent_no), _relevance_score(relevance), _querySlot(querySlot) {}
	BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(TopicReturnPFeature, Pattern_ptr, int, float, Symbol, const Mention*,
		 const LanguageVariant_ptr&);
		 
	TopicReturnPFeature(Pattern_ptr pattern, int sent_no, float relevance, Symbol querySlot, const Mention* mention)
		: ReturnPatternFeature(pattern), _sent_no(sent_no), _relevance_score(relevance), _querySlot(querySlot) {}
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(TopicReturnPFeature, Pattern_ptr, int, float, Symbol, const Mention*);
public:
	const Mention* getMention() const { return _mention; }
	float getRelevanceScore() const { return _relevance_score; }
	int getSentenceNumber() const { return _sent_no; }
	Symbol getQuerySlot() const { return _querySlot; }
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
		printFeatureFocusHelper(patternMatcher, out, L"topic", _relevance_score); }
	virtual bool returnValueIsEqual(PatternFeature_ptr other_feature) const {
		boost::shared_ptr<TopicReturnPFeature> other = boost::dynamic_pointer_cast<TopicReturnPFeature>(other_feature);
		return (other && other->getMention()==getMention() && 
			other->getSentenceNumber()==getSentenceNumber() &&
			other->getQuerySlot() == getQuerySlot() &&
			other->getRelevanceScore()==getRelevanceScore()); }
	virtual const wchar_t* getReturnTypeName() { return L"topic"; }
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	TopicReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
private:
	int _sent_no;
	Symbol _querySlot;
	float _relevance_score;
	const Mention* _mention;
};

/*************************************************************************
 * Return Pattern Feature: Token Span
 *************************************************************************
 * Return feature indicating a span of tokens.  Unlike all other return
 * features, this return feature's getStartToken() and getEndToken()
 * methods don't just return the entire sentence. */
class TokenSpanReturnPFeature: public ReturnPatternFeature {
private:
	TokenSpanReturnPFeature(Pattern_ptr pattern, int sent_no, int start_token, int end_token, const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(pattern,languageVariant), _sent_no(sent_no), _start_token(start_token), _end_token(end_token) {}
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(TokenSpanReturnPFeature, Pattern_ptr, int, int, int, const LanguageVariant_ptr&);
	TokenSpanReturnPFeature(Symbol returnLabel, int sent_no, int start_token, int end_token, const LanguageVariant_ptr& languageVariant)
		: ReturnPatternFeature(returnLabel,languageVariant), _sent_no(sent_no), _start_token(start_token), _end_token(end_token) {}
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(TokenSpanReturnPFeature, Symbol, int, int, int, const LanguageVariant_ptr&);
	
	TokenSpanReturnPFeature(Pattern_ptr pattern, int sent_no, int start_token, int end_token)
		: ReturnPatternFeature(pattern), _sent_no(sent_no), _start_token(start_token), _end_token(end_token) {}
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(TokenSpanReturnPFeature, Pattern_ptr, int, int, int);
	TokenSpanReturnPFeature(Symbol returnLabel, int sent_no, int start_token, int end_token)
		: ReturnPatternFeature(returnLabel), _sent_no(sent_no), _start_token(start_token), _end_token(end_token) {}
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(TokenSpanReturnPFeature, Symbol, int, int, int);
public:
	int getSentenceNumber() const { return _sent_no; }
	virtual int getStartToken() const { return _start_token; }
	virtual int getEndToken() const { return _end_token; }
	virtual void setCoverage(const DocTheory * docTheory) { /* do nothing*/ }
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher) { /* do nothing*/ }
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
		printFeatureFocusHelper(patternMatcher, out, L"text"); }
	virtual bool returnValueIsEqual(PatternFeature_ptr other_feature) const {
		boost::shared_ptr<TokenSpanReturnPFeature> other = boost::dynamic_pointer_cast<TokenSpanReturnPFeature>(other_feature);
		return (other && other->_sent_no==_sent_no && other->_start_token==_start_token &&
			other->_end_token==_end_token); }
	virtual const wchar_t* getReturnTypeName() { return L"text"; }
	virtual void saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const;
	TokenSpanReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap);
private:
	int _sent_no;
	int _start_token;
	int _end_token;
};

typedef boost::shared_ptr<ReturnPatternFeature> ReturnPatternFeature_ptr;
typedef boost::shared_ptr<MentionReturnPFeature> MentionReturnPFeature_ptr;
typedef boost::shared_ptr<ValueMentionReturnPFeature> ValueMentionReturnPFeature_ptr;
typedef boost::shared_ptr<DocumentDateReturnPFeature> DocumentDateReturnPFeature_ptr;
typedef boost::shared_ptr<EventMentionReturnPFeature> EventMentionReturnPFeature_ptr;
typedef boost::shared_ptr<RelMentionReturnPFeature> RelMentionReturnPFeature_ptr;
typedef boost::shared_ptr<GenericReturnPFeature> GenericReturnPFeature_ptr;
typedef boost::shared_ptr<TopicReturnPFeature> TopicReturnPFeature_ptr;
typedef boost::shared_ptr<TokenSpanReturnPFeature> TokenSpanReturnPFeature_ptr;
typedef boost::shared_ptr<PropositionReturnPFeature> PropositionReturnPFeature_ptr;
typedef boost::shared_ptr<ParseNodeReturnPFeature> ParseNodeReturnPFeature_ptr;
typedef boost::shared_ptr<DateSpecReturnPFeature> DateSpecReturnPFeature_ptr;

#endif

