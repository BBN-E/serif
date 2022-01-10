// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef FEATUREVALUESTRUCTURE_H
#define FEATUREVALUESTRUCTURE_H

#include <boost/shared_ptr.hpp>

#include "Generic/theories/Theory.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/InternalInconsistencyException.h"


class FeatureValueStructure : public Theory {
public:
	/** Create and return a new FeatureValueStructure. */
	static FeatureValueStructure *build() { return _factory()->build(); }
	static FeatureValueStructure *build(SerifXML::XMLTheoryElement fvsElem) { return _factory()->build(fvsElem); }
	static FeatureValueStructure *build(Symbol category, Symbol voweledString, Symbol PartOfSpeech, Symbol gloss) { return _factory()->build(category, voweledString, PartOfSpeech, gloss); }
	static FeatureValueStructure *build(Symbol category, Symbol voweledString, Symbol PartOfSpeech, Symbol gloss, bool analyzed) { return _factory()->build(category, voweledString, PartOfSpeech, gloss, analyzed); }
	/** Hook for registering new FeatureValueStructure factories */
	struct Factory { 
		virtual FeatureValueStructure *build() = 0; 
		virtual FeatureValueStructure *build(SerifXML::XMLTheoryElement fvsElem) = 0; 
		virtual FeatureValueStructure *build(Symbol category, Symbol voweledString, Symbol PartOfSpeech, Symbol gloss) = 0; 
		virtual FeatureValueStructure *build(Symbol category, Symbol voweledString, Symbol PartOfSpeech, Symbol gloss, bool analyzed) = 0; 
	};
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~FeatureValueStructure(){};

	virtual bool operator==(const FeatureValueStructure &other) const = 0;
	virtual bool operator!=(const FeatureValueStructure &other) const = 0;
	virtual void dump(UTF8OutputStream &uos) = 0;
	virtual Symbol getPartOfSpeech() = 0;
	virtual Symbol getCategory() = 0;
	virtual Symbol getVoweledString() = 0;

	void updateObjectIDTable() const {
		throw InternalInconsistencyException("FeatureValueStructure::updateObjectIDTable()",
											"Using unimplemented method.");
	}

	void saveState(StateSaver *stateSaver) const {
		throw InternalInconsistencyException("FeatureValueStructure::saveState()",
											"Using unimplemented method.");
	}

	void resolvePointers(StateLoader * stateLoader) {
		throw InternalInconsistencyException("FeatureValueStructure::resolvePointers()",
											"Using unimplemented method.");
	}

	// For XML serialization:
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const = 0;
	const wchar_t* XMLIdentifierPrefix() const { return L"fvs"; }
	virtual bool hasXMLId() const { return false; }

protected:
	FeatureValueStructure(){};

private:
	static boost::shared_ptr<Factory> &_factory();
};
//#if defined(ARABIC_LANGUAGE)
//	#include "Arabic/BuckWalter/ar_FeatureValueStructure.h"
//#elif defined(KOREAN_LANGUAGE)
//	#include "Korean/morphology/kr_FeatureValueStructure.h"
//#else
//	#include "Generic/theories/xx_FeatureValueStructure.h"
//#endif

#endif
