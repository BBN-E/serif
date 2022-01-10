// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CLUTTER_FILTER_H
#define CLUTTER_FILTER_H

#include <boost/shared_ptr.hpp>


class DocTheory;
class EntityClutterFilter;
class RelationClutterFilter;
class EventClutterFilter;

// see documentation below on its usage
class ClutterFilter {
protected:
	/**
	 * Default constructor. Nothing to initialize.
	 */
	ClutterFilter() {}

public:
	/** Create and return a new ClutterFilter. */
	static ClutterFilter *build() { return _factory()->build(); }
	/** Hook for registering new ClutterFilter factories. */
	struct Factory { virtual ClutterFilter *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	/**
	 * Potentially set clutter flags of entities of docTheory
	 */
	virtual void filterClutter (DocTheory* docTheory) = 0;
	/**
	 * return the different filters
	 */
//	virtual EntityClutterFilter *getEntityFilter () const = 0;
//	virtual RelationClutterFilter *getRelationFilter () const = 0;
//	virtual EventClutterFilter *getEventFilter () const = 0;

	virtual ~ClutterFilter() {}


private:
	static boost::shared_ptr<Factory> &_factory();
};

//#if defined(ENGLISH_LANGUAGE)
//	#include "English/clutter/en_ClutterFilter.h"
////#elif defined(CHINESE_LANGUAGE)
////	#include "Chinese/clutter/ch_ClutterFilter.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/clutter/ar_ClutterFilter.h"
//#else
//	#include "Generic/clutter/xx_ClutterFilter.h"
//#endif

/*

This document briefly describes the design and usage of a new module
for Serif called ClutterFilter.  (The name is probably a misnomer
because nothing is really being filtered; "marker" or "tagger" might
be more appropriate here.)  The purpose of the module is to identify
Entity, Relation, or Event objects that might be considered as
"cluttered" according to some heuristics and/or machine learning
models.  The remainder of this document describes how to use and
extend the module.

By design, the ClutterFilter module is modeled closely after the
existing GenericsFilter module.  Under the new module generic/clutter,
there are two header files ClutterFilter.h and xx_ClutterFilter.h,
which defines a generic clutter filter interface ClutterFilter
and its default implementation, ClutterFilter, respectively.  (A
language-specific implementation is currently available for the
English language only.)  The interface for ClutterFilter is
defined as follows:

	virtual void filterClutter (DocTheory* docTheory) = 0;
	virtual EntityClutterFilter *getEntityFilter () const = 0;
	virtual RelationClutterFilter *getRelationFilter () const = 0;
	virtual EventClutterFilter *getEventFilter () const = 0;

The filterClutter() method, as is the case with GenericsFilter,
is the main entry point to the filter.  The remaining three methods
provide access to the underlying implementation for each respectively
document type.

In adding support for ClutterFilter, a new generic filter framework has
been developed for the three fundamental types under generic/theories,
namely, Entity, Relation, and Event.  In this framework, three filter
interfaces, corresponding to each respective type, are defined:

class EntityClutterFilter {
public:
	virtual bool filtered (const Entity *, double *score = 0) const = 0;
};

class RelationClutterFilter {
public:
	virtual bool filtered (const Relation *, double *score = 0) const = 0;
};

class EventClutterFilter {
public:
	virtual bool filtered (const Event *, double *score = 0) const = 0;
};


Here, a filter is defined in terms of a boolean indicating whether an
object has been "filtered" and, if true, an optional numeric score can
be assigned to the result.  For each type, three methods have been
added to support the new filter interface:

class Entity {
// ...
	void applyFilter (const std::string& filterName, EntityClutterFilter *filter);
	bool isFiltered (const std::string& filterName) const;
	double getFilterScore (const std::string& filterName) const;

// ...
};

(Similar methods are defined for Relation and Event.)  To add new
filter to an object, one simply calls applyFilter() given a unique
filter name and a corresponding implementation of the filter
interface (i.e., one of EntityClutterFilter, RelationClutterFilter, or
EventClutterFilter).  Note that there are no limits to the number of filters
that can be associated with an object.

Let's now give an example of adding a new filter using the new
filtering framework.  For the purpose of this example, the new filter
is only applicable to Entity objects and its purpose is to determine
whether an instance of Entity is generic or not.  (Note that here we
simply provide a wrapper around the existing method isGeneric() for
Entity.)

//... assuming proper include files...

class MyGenericsFilter: public ClutterFilter {
     struct MyGenericsFilter: public EntityClutterFilter {

	// implementation of EntityClutterFilter interface
	bool filtered (const Entity *ent, double *score=0) const {
	     bool isGeneric (ent->isGeneric());
	     if (isGeneric && score != 0) {
		// set the score to 1 if the given Entity is generic
		*score = 1.;
	     }
	     return isGeneric;
	}
     };

     MyGenericsFilter *_filter;

public:
  static const std::string filterName;

  MyGenericsFilter () :_filter (new MyGenericsFilter) {}
  ~MyGenericsFilter () { delete _filter; }

  // implementation of ClutterFilter interface
  void filterClutter (DocTheory *theory) {
     EntitySet *entitySet = theory->getEntitySet();
     int n_ents = entitySet->getNEntities();
     for (int i = 0; i < n_ents; ++i) {
        Entity *ent (entitySet->getEntity(i));
        ent->applyFilter(filterName, _filter);
     }
  }
  EntityClutterFilter *getEntityFilter () const { return _filter; }
  RelationClutterFilter *getRelationFilter () const { return 0; }
  EventClutterFilter *getEventFilter () const { return 0; }
};

const std::string filterName = "MyGenericsFilter";

//....

Now to use this new filter, we simply call filterClutter() on a given
instance of DocTheory, e.g.,

// assume theory is an instance of DocTheory defined elsewhere....
MyGenericsFilter *filter = new MyGenericsFilter;
filter->filterClutter(theory);

// now each instance of Entity can be queried...
EntitySet *entitySet (theory->getEntitySet());
int n_ents (entitySet->getNEntities());
for (int i = 0; i < n_ents; ++i) {
   Entity *ent (entitySet->getEntity(i));
   if (ent->isFiltered(MyGenericsFilter::filterName)) {
      cout << MyGenericsFilter::filterName 
           << " score for entity " << ent->getID() 
           << " is " << ent->getFilterScore(MyGenericsFilter::filterName)
           << endl;
   }
}

//....

It should be clear from this example that adding new filters to either
Entity, Relation, and/or Event types do not require making changes to
the respective type!

*/

#endif
