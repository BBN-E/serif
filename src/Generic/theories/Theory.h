// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.
//
// Theory.h -- abstract base class for all theory/annotation objects.

#ifndef THEORY_H
#define THEORY_H

#include <xercesc/util/XercesDefs.hpp>
#include "Generic/state/XMLTheoryElement.h"

// Forward declarations
class StateSaver;
class StateLoader;

class Theory {
public:
	virtual ~Theory() {}

	// For saving state:
	virtual void updateObjectIDTable() const = 0;
	virtual void saveState(StateSaver *stateSaver) const = 0;
	// For loading state:
	virtual void resolvePointers(StateLoader * stateLoader) = 0;

	/** Serialize this theory object by writing information about it
	  * to the given XMLTheoryElement object.  The context argument is used
	  * when extra information is necessary in order to serialize an
	  * object.  Each theory type's saveXML() method will generally 
	  * have specific expectations about whether a context object should 
	  * be passed in, and if so, what it should be. */
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const = 0;

	/** Return the prefix string that should be used to construct XML
	  * identifiers for objects of this type.  E.g., for the Document 
	  * theory type, it might return "doc". */
	virtual const wchar_t* XMLIdentifierPrefix() const = 0;

	/** Return true if objects of this theory class should be assigned
	  * identifiers when serializing. */
	virtual bool hasXMLId() const { return true; }
};

#endif
