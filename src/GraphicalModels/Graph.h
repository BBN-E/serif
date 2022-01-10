#ifndef _GRAPH_H_
#define _GRAPH_H_

#include "Generic/common/bsp_declare.h"

namespace GraphicalModel {
	class Alphabet;

// uses Curiously Recurring Template Pattern for compile-time
// polymorphism for a profiler-verified significant performance
// gain
template <typename GraphType>
class Graph {
public:
	unsigned int nFactors() const {
		return static_cast<const GraphType*>(this)->nFactorsImpl();
	}

	unsigned int nVariables() const {
		return static_cast<const GraphType*>(this)->nVariablesImpl();
	}

	unsigned int id;
	void setID(unsigned int ID) { id = ID; }

	void numberFactors(unsigned int& next_id) {
		static_cast<GraphType*>(this)->numberFactorsImpl(next_id);
	}

	//void clearFactorModifications();

	double inference() {
		return static_cast<GraphType*>(this)->inferenceImpl();
	}

	virtual void dumpGraph() const =0;
};

// interface for factor graphs where factor distributions can
// be altered by posterior regularization
template <typename GraphType>
class PRModifiableGraph {
public:
	// should be implemented by subclasses by simply iterating
	// over all factors and calling clearModifiers on them
	void clearFactorModifications()  {
		return static_cast<GraphType*>(this)->clearFactorModificationsImpl();
	}
};

};

#endif

