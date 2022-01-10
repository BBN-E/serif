// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef VIKTRS_SENTENCE_SPAN_H
#define VIKTRS_SENTENCE_SPAN_H

#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"

class VIKTRSSentenceSpan : public Span {
public:
	VIKTRSSentenceSpan()
		: Span(), _original_sentence_index(0), _step_pos_id(0) {}
	VIKTRSSentenceSpan(EDTOffset start_offset, EDTOffset end_offset, size_t original_sentence_index, size_t step_pos_id)
		: Span(start_offset, end_offset, true, true), _original_sentence_index(original_sentence_index), _step_pos_id(step_pos_id) {}
	~VIKTRSSentenceSpan();
	virtual Symbol getSpanType() const {
		return Symbol(L"VIKTRS_Sentence"); }
	size_t getSentNo() const { return _original_sentence_index; }
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	virtual void loadXML(SerifXML::XMLTheoryElement elem);

	virtual bool restrictSentenceBreak() const { return !_maySentenceBreak; }
	virtual bool enforceTokenBreak() const { return _mustBreakAround; }

private:
	size_t _original_sentence_index;
	size_t _step_pos_id;
};

class VIKTRSSentenceSpanCreator : public SpanCreator {
public:
	VIKTRSSentenceSpanCreator() {}
	~VIKTRSSentenceSpanCreator() {}

	virtual Span* createSpan(EDTOffset start_offset, EDTOffset end_offset, const void *param) {
		const RegionSpec *sentSpec = (const RegionSpec *) param;
		size_t original_sentence_index = sentSpec->sentno;
		size_t step_pos_id = sentSpec->stepno;
		return _new VIKTRSSentenceSpan(start_offset, end_offset, original_sentence_index, step_pos_id);
	}

	virtual Symbol getIdentifier() { return Symbol(L"VIKTRS_Sentence"); }

private:
	struct RegionSpec {
		int start, end;
		Symbol tag;
		size_t sentno;
		size_t stepno;
		RegionSpec(int start, int end, Symbol tag, size_t sentno=0, size_t stepno=0):
		start(start), end(end), tag(tag), sentno(sentno), stepno(stepno) {}
	};
};

#endif
