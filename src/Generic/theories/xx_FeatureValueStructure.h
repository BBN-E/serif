// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_FEATUREVALUESTRUCTURE_H
#define XX_FEATUREVALUESTRUCTURE_H
#include "Generic/theories/FeatureValueStructure.h"
#include <iostream>

class GenericFeatureValueStructure : public FeatureValueStructure {
private:
	friend class GenericFeatureValueStructureFactory;

	static void defaultMsg() {
		std::cerr<<"<<<<<<<<<WARNING: Using unimplemented theories/GenericFeatureValueStructure!>>>>>\n";
	};

public:
	~GenericFeatureValueStructure(){};
	virtual bool operator==(const FeatureValueStructure &other) const { return false; }
	virtual bool operator!=(const FeatureValueStructure &other) const { return false; }
	void dump(UTF8OutputStream &uos){}
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const {}
	Symbol getPartOfSpeech(){ Symbol null(L":NULL"); return null; }
	Symbol getCategory(){ Symbol null(L":NULL"); return null; }
	Symbol getVoweledString(){ Symbol null(L":NULL"); return null; }
private:
	GenericFeatureValueStructure(){
		defaultMsg();
	}
    explicit GenericFeatureValueStructure(SerifXML::XMLTheoryElement fvsElem) {
		defaultMsg();
	}

};

class GenericFeatureValueStructureFactory: public FeatureValueStructure::Factory {
	virtual FeatureValueStructure *build() { return _new GenericFeatureValueStructure(); }
	virtual FeatureValueStructure *build(SerifXML::XMLTheoryElement fvsElem) { return _new GenericFeatureValueStructure(fvsElem); }
	virtual FeatureValueStructure *build(Symbol category, Symbol voweledString, Symbol PartOfSpeech, Symbol gloss) { return _new GenericFeatureValueStructure(); }
	virtual FeatureValueStructure *build(Symbol category, Symbol voweledString, Symbol PartOfSpeech, Symbol gloss, bool analyzed) { return _new GenericFeatureValueStructure(); }
};

#endif
