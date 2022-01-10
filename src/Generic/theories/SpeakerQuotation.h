// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SPEAKER_QUOTATION_H
#define SPEAKER_QUOTATION_H

#include "Generic/theories/Theory.h"

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;
class Mention;

/** A quotation that is attributed to a speaker.  Each SpeakerQuotation 
  * constist of a pointer to the speaker mention, along with a span of
  * tokens (which may span across multiple sentences) containing the 
  * quotation itself.  This cross-sentence token span is specified using
  * a start sentence number and start token within that sentence; and an
  * end sentence number and end token with that sentence. */
class SpeakerQuotation : public Theory {
public:
	SpeakerQuotation(const Mention* speakerMention, 
	                 int quote_start_sentno, int quote_start_tokno,
	                 int quote_end_sentno, int quote_end_tokno);

	/** Return the mention containing the speaker of this quotation. */
	const Mention *getSpeakerMention() const { return _speakerMention; }

	// Beginning of the quotation.
	int getQuoteStartSentence() const { return _quote_start_sentno; }
	int getQuoteStartToken() const { return _quote_start_tokno; }

	// End of the quotation
	int getQuoteEndSentence() const { return _quote_end_sentno; }
	int getQuoteEndToken() const { return _quote_end_tokno; }

	void setQuoteStartSentence(int sentno) { _quote_start_sentno = sentno; }
	void setQuoteStartToken(int tokno) { _quote_start_tokno = tokno; }
	void setQuoteEndSentence(int sentno) { _quote_end_sentno = sentno; }
	void setQuoteEndToken(int tokno) { _quote_end_tokno = tokno; }

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	SpeakerQuotation(StateLoader *stateLoader);
	void resolvePointers(StateLoader *stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit SpeakerQuotation(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;

private:
	const Mention *_speakerMention;

	int _quote_start_sentno;
	int _quote_start_tokno;
	int _quote_end_sentno;
	int _quote_end_tokno;
};

#endif
