// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SPAN_H
#define SPAN_H

#include "Generic/theories/Theory.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/InternalInconsistencyException.h"
#include <map>
#include <string>
#include <boost/noncopyable.hpp>

/** Spans should always be passed by pointer; they are not copyable.  
  * Typically, Span objects are owned by a document's Metadata object.
  * Spans are not usually created directly; instead, use the method
  * Metadata::newSpan() to create a new span that is owned by the
  * Metadata object. */
class Span : public Theory, private boost::noncopyable
{

public:
	Span(){};
	Span(const EDTOffset start_offset, const EDTOffset end_offset, const bool maySentenceBreak, const bool mustBreakAround)
		: _start_offset(start_offset), _end_offset(end_offset),
		_maySentenceBreak(maySentenceBreak), _mustBreakAround(mustBreakAround) {}
	virtual ~Span() {}

	virtual EDTOffset getStartOffset() const { return _start_offset; }
	virtual EDTOffset getEndOffset() const { return _end_offset; }
	virtual bool restrictSentenceBreak() const { return false; }
	virtual bool enforceTokenBreak() const { return false; }
	bool covers(const EDTOffset offset) const { return (offset >= _start_offset) && (offset <= _end_offset); }
	int length() const { return (getEndOffset().value() - getStartOffset().value()) + 1; }
	virtual Symbol getSpanType() const = 0;

	void updateObjectIDTable() const {
		throw InternalInconsistencyException("Span::updateObjectIDTable()",
											"Using unimplemented method.");
	}

	void saveState(StateSaver *stateSaver) const {
		throw InternalInconsistencyException("Span::saveState()",
											"Using unimplemented method.");

	}

	void resolvePointers(StateLoader * stateLoader) {
		throw InternalInconsistencyException("Span::resolvePointers()",
											"Using unimplemented method.");

	}
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual void loadXML(SerifXML::XMLTheoryElement elem);

	const wchar_t* XMLIdentifierPrefix() const;
	// This returns a pointer to an instance of a subclass of Span:
	static Span* loadSpanFromXML(SerifXML::XMLTheoryElement spanElem);


protected:
	EDTOffset _start_offset, _end_offset;
	bool _maySentenceBreak, _mustBreakAround;

public:
	// An XMLSpanLoader is a function that takes an XML element, and returns
	// a Span object.  Each span subclass will need to register its own 
	// XMLSpanLoader.
	typedef Span* XMLSpanLoader(SerifXML::XMLTheoryElement);
	static int registerSpanLoader(Symbol id, XMLSpanLoader loader);

};

// This fairly complex macro makes it easier to register a new span
// class, to allow it to be deserialized properly.  Just call this 
// macro once in the .cpp file corresponding to the span class.  It 
// only needs to be called for instantiable span classes.
#define REGISTER_SPAN_CLASS(NAMESPACE, CLASS) \
	Span* load##CLASS##XML(SerifXML::XMLTheoryElement e) { \
		Span *span = new NAMESPACE::CLASS(); \
		span->loadXML(e); \
		return span; \
	} \
	static int x = Span::registerSpanLoader(NAMESPACE::CLASS().getSpanType(), load##CLASS##XML); \
	NAMESPACE::CLASS::~CLASS() { }

#endif
