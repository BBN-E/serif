// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef COREF_SPAN_H
#define COREF_SPAN_H

#include "Generic/common/Symbol.h"
#include "Generic/theories/Span.h"

namespace DataPreprocessor {

	class CorefSpan : public Span {
	public:
		CorefSpan()
			: Span() { _id = NO_ID; _parent = 0; }
		CorefSpan(EDTOffset start_offset, EDTOffset end_offset)
			: Span(start_offset, end_offset, false, true){ _id = NO_ID; _parent = 0; }
		CorefSpan(EDTOffset start_offset, EDTOffset end_offset, int id)
			: Span(start_offset, end_offset, false, true) { _id = id; _parent = 0; }
		~CorefSpan() {}

		static const int NO_ID = -1;

		int getID() { return _id; }
		void setID(int id) { _id = id; }
		
		//void setParent(CorefSpan *parent) { _parent = parent; }
		//const CorefSpan* getParent() { return _parent; }
		//bool hasParent() { return _parent != NULL; }

		virtual bool restrictSentenceBreak() const { return !_maySentenceBreak; }
		virtual bool enforceTokenBreak() const { return _mustBreakAround; }

		virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
		virtual void loadXML(SerifXML::XMLTheoryElement elem);
	private:
		int _id;
		const CorefSpan *_parent;
	};

} // namespace DataPreprocessor

#endif
