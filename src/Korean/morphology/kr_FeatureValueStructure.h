// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KR_FEATUREVALUESTRUCTURE_H
#define KR_FEATUREVALUESTRUCTURE_H

#include "theories/KoreanFeatureValueStructure.h"
#include "common/Symbol.h"

class KoreanFeatureValueStructure : public FeatureValueStructure {
private:
	friend class KoreanFeatureValueStructureFactory;


public:
	~KoreanFeatureValueStructure(){};

	bool operator==(const FeatureValueStructure &other) const;
	bool operator!=(const FeatureValueStructure &other) const;

	Symbol getPartOfSpeech() { return _partOfSpeech; }
	bool isAnalyzed() { return _analyzed; }

	void dump(UTF8OutputStream &uos);

	// For XML serialization:
	void saveXML(SerifXML::XMLElement elem, const Theory *context=0) const;
	explicit KoreanFeatureValueStructure(SerifXML::XMLElement fvsElem);
private:
	Symbol _partOfSpeech;
	bool _analyzed;

	KoreanFeatureValueStructure();
	KoreanFeatureValueStructure(Symbol partOfSpeech, bool analyzed = false);
};

class KoreanFeatureValueStructureFactory: public FeatureValueStructure::Factory {
	virtual FeatureValueStructure *build() { return _new KoreanFeatureValueStructure(); }
	virtual FeatureValueStructure *build(Symbol category, Symbol voweledString, Symbol PartOfSpeech, Symbol gloss) { return _new KoreanFeatureValueStructure(partOfSpeech); }
	virtual FeatureValueStructure *build(Symbol category, Symbol voweledString, Symbol PartOfSpeech, Symbol gloss, bool analyzed) { return _new KoreanFeatureValueStructure(partOfSpeech, analyzed); }
};

#endif
