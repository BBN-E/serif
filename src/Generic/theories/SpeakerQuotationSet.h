// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SPEAKER_QUOTATION_SET_H
#define SPEAKER_QUOTATION_SET_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/SpeakerQuotation.h"

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;
class MentionSet;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED SpeakerQuotationSet : public Theory {
public:
	SpeakerQuotationSet() {}
	SpeakerQuotationSet(std::vector<SpeakerQuotationSet*> splitSpeakerQuotationSets, std::vector<int> sentenceOffsets, std::vector<MentionSet*> mergedMentionSets);
	~SpeakerQuotationSet();

	int getNSpeakerQuotations() const { return static_cast<int>(_quotations.size()); }
	const SpeakerQuotation *getSpeakerQuotation(int i) const;

	/** Add a speaker quotation to this set.  The SpeakerQuotationSet takes
	  * ownership of the given speakerQuotation, and will delete it when the
	  * SpeakerQuotationSet is destroyed. */
	void takeQuotation(const SpeakerQuotation* speakerQuotation);

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	SpeakerQuotationSet(StateLoader *stateLoader);
	void resolvePointers(StateLoader * stateLoader);
	void loadMentionSets();
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit SpeakerQuotationSet(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;

private:
	std::vector<const SpeakerQuotation*> _quotations;
};



#endif
