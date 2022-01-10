// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_SENTENCE_SPAN_H
#define ICEWS_SENTENCE_SPAN_H

#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"
#include "Generic/common/Offset.h"

class DocTheory;

namespace ICEWS {

	class IcewsSentenceSpan : public Span {
	public:
		IcewsSentenceSpan(): Span(), _original_sentence_index(0) {
			_maySentenceBreak = true;
			_mustBreakAround = false;
		}
		IcewsSentenceSpan(EDTOffset start_offset, EDTOffset end_offset, size_t original_sentence_index)
			: Span(start_offset, end_offset, true, false), _original_sentence_index(original_sentence_index) {}
		virtual Symbol getSpanType() const {
			return Symbol(L"ICEWS_Sentence"); }
		size_t getSentNo() const { return _original_sentence_index; }
		virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
		virtual void loadXML(SerifXML::XMLTheoryElement elem);
		static int icewsSentenceNoToSerifSentenceNo(int icews_sentence_no, const DocTheory* docTheory);
		static int serifSentenceNoToIcewsSentenceNo(int serif_sentence_no, const DocTheory* docTheory);
		static int edtOffsetToIcewsSentenceNo(EDTOffset offset, const DocTheory* docTheory);
	private:
		size_t _original_sentence_index;
	};

	class IcewsSentenceSpanCreator : public SpanCreator {
	public:
		IcewsSentenceSpanCreator() {}
		~IcewsSentenceSpanCreator() {}
		virtual Span* createSpan(EDTOffset start_offset, EDTOffset end_offset, const void *param) {
			size_t original_sentence_index = *(static_cast<const size_t*>(param));
			return _new IcewsSentenceSpan(start_offset, end_offset, original_sentence_index);
		}
		virtual Symbol getIdentifier() { return Symbol(L"ICEWS_Sentence"); }
	};

	Span* loadIcewsSentenceSpan(SerifXML::XMLTheoryElement e);

}

#endif
