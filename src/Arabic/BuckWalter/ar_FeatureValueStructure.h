// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_FEATUREVALUESTRUCTURE_H
#define AR_FEATUREVALUESTRUCTURE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/theories/FeatureValueStructure.h"

class ArabicFeatureValueStructure : public FeatureValueStructure{
private:
	friend class ArabicFeatureValueStructureFactory;

private:
	Symbol _category;
	Symbol _voweledString;
	Symbol _partOfSpeech;
	bool _analyzed;
	Symbol _gloss;

public:
	~ArabicFeatureValueStructure(){};

	bool operator==(const FeatureValueStructure &other) const;
	bool operator!=(const FeatureValueStructure &other) const;

	Symbol getCategory(){ return _category;}
	Symbol getVoweledString(){return _voweledString;}	
	Symbol getPartOfSpeech(){return _partOfSpeech;}
	Symbol getGloss(){return _gloss;}
	bool isAnalyzed(){return _analyzed;}

	void dump(UTF8OutputStream &uos);

	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;

private:
	ArabicFeatureValueStructure();
	explicit ArabicFeatureValueStructure(SerifXML::XMLTheoryElement fvsElem);
	ArabicFeatureValueStructure(Symbol category, Symbol voweledString, Symbol PartOfSpeech, Symbol gloss, bool analyzed = false);

};

class ArabicFeatureValueStructureFactory: public FeatureValueStructure::Factory {
	virtual FeatureValueStructure *build() { return _new ArabicFeatureValueStructure(); }
	virtual FeatureValueStructure *build(SerifXML::XMLTheoryElement fvsElem) { return _new ArabicFeatureValueStructure(fvsElem); }
	virtual FeatureValueStructure *build(Symbol category, Symbol voweledString, Symbol PartOfSpeech, Symbol gloss) { return _new ArabicFeatureValueStructure(category, voweledString, PartOfSpeech, gloss); }
	virtual FeatureValueStructure *build(Symbol category, Symbol voweledString, Symbol PartOfSpeech, Symbol gloss, bool analyzed) { return _new ArabicFeatureValueStructure(category, voweledString, PartOfSpeech, gloss, analyzed); }
};

#endif
