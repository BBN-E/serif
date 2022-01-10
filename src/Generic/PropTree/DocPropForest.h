// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOCPROPFOREST_H
#define DOCPROPFOREST_H


#include <vector>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

#include "theories/DocTheory.h"
#include "Generic/PropTree/PropNode.h"

/** A container class holding a collection of PropTrees for each sentence
  * in a document, along with a pointer to that document.  Iterating over
  * the DocPropForest will yield a sequence of PropNodes_ptr, which 
  * correspond one-to-one to the sentences in the document.  Each of these
  * PropNodes_ptrs points to a vector that contains pointers to the 
  * PropNodes for that sentence. */
class DocPropForest {
public:
	// Constructor
	DocPropForest(const DocTheory* dt);

	// Type definitions
	typedef boost::shared_ptr<DocPropForest> ptr;
	typedef std::vector<PropNodes_ptr>::const_iterator const_iterator;
	typedef std::vector<PropNodes_ptr>::iterator iterator;

	// Iterators
	iterator begin() {return _nodesBySentence.begin(); }
	iterator end() {return _nodesBySentence.end(); }
	const_iterator begin() const {return _nodesBySentence.begin(); }
	const_iterator end() const {return _nodesBySentence.end(); }

	// Size
	size_t size() const {return _nodesBySentence.size();}

	// Indexing
	PropNodes_ptr& operator[](size_t idx) {return _nodesBySentence[idx];}
	const PropNodes_ptr& operator[](size_t idx) const {return _nodesBySentence[idx];}

	// DocTheory access
	const DocTheory* getDocTheory() const {return _docTheory; }

	/** Call "clearExpansions" on every (root) PropNode in this forest. */
	void clearExpansions();

private:
	std::vector<PropNodes_ptr> _nodesBySentence;
	const DocTheory* _docTheory;
};


#endif
