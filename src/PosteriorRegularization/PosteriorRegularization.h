#ifndef _POSTERIOR_REGULARIZATION_H_
#define _POSTERIOR_REGULARIZATION_H_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/BoostUtil.h"
#include "../GraphicalModels/GraphicalModelTypes.h"
#include "../GraphicalModels/Variable.h"
#include "../GraphicalModels/Factor.h"
#include "../GraphicalModels/Graph.h"
#include "../GraphicalModels/DataSet.h"
#include "../GraphicalModels/BagOfWordsFactor.h"
#include "../GraphicalModels/VariablePriorFactor.h"
#include "../GraphicalModels/learning/EM.h"
#include "../GraphicalModels/distributions/BernoulliDistribution.h"
#include "../GraphicalModels/distributions/MultinomialDistribution.h"

class BadLineException {
public:
	BadLineException(const std::wstring& msg) : msg(msg) { }
	std::wstring msg;
};


// flight number, starting point, ending point + garbage
static const unsigned int NUM_CLASSES = 7; 
static const unsigned int AIRLINE = 0;
static const unsigned int FLIGHT_NUMBER = 1;
static const unsigned int AIRCRAFT_MODEL = 2;
static const unsigned int LOCATION = 3;
static const unsigned int COUNTRY = 4;
static const unsigned int GARBAGE = 5;
static const unsigned int GAR_LOC = 6;

typedef std::vector<std::wstring> FV;
typedef boost::shared_ptr<FV> FV_ptr;

BSP_DECLARE(SlotVariable)

class SlotVariable : public GraphicalModel::Variable {
public:
	SlotVariable() : Variable(NUM_CLASSES) {}
	//FV_ptr features;
	bool annotated;
	unsigned int gold_label;
	unsigned int tok_idx;

	static unsigned int classIndex(const std::wstring& className) {
		if (className == L"link") {
			return GARBAGE;
		} else if (className == L"AL") {
			return AIRLINE;
		} else if (className == L"FN") {
			return FLIGHT_NUMBER;
		} else if (className == L"AC") {
			return AIRCRAFT_MODEL;
		} else if (className == L"LO") {
			return LOCATION;
		} else if (className == L"CO") {
			return COUNTRY;
		} else {
			throw BadLineException(L"class index");
		}
	}

	size_t bestLabel() const {
		size_t best_label = -1;
		double best_score = -1;

		for (size_t i=0; i<NUM_CLASSES; ++i) {
			if (marginals()[i] > best_score) {
				best_label = i;
				best_score = marginals()[i];
			}
		}

		return best_label;
	}

	void dump() const;
};

class ArgTypePriorFactor;
typedef boost::shared_ptr<ArgTypePriorFactor> ArgTypePriorFactor_ptr;
typedef GraphicalModel::MultinomialDistribution<GraphicalModel::SymmetricDirichlet>
	ATPFDist;
typedef GraphicalModel::VariablePriorFactor<ArgTypePriorFactor,SlotVariable,ATPFDist> ATPFParent;
	
class ArgTypePriorFactor : public ATPFParent {
public:
	ArgTypePriorFactor() : ATPFParent(NUM_CLASSES)  { }
	/*friend class GraphicalModel::Factor<ArgTypePriorFactor>;
	friend class GraphicalModel::ModifiableUnaryFactor<ArgTypePriorFactor>;*/
	friend class GraphicalModel::VariablePriorFactor<ArgTypePriorFactor,SlotVariable,ATPFDist>;
	static void finishedLoading(unsigned int n_classes);
private:
	static const double SMOOTHING;
};

class BMMFeatureFactor;
typedef boost::shared_ptr<BMMFeatureFactor> BMMFeatureFactor_ptr;
typedef GraphicalModel::BagOfWordsFactor<BMMFeatureFactor,SlotVariable, 
		GraphicalModel::MultinomialDistribution<GraphicalModel::SymmetricDirichlet> > BMMFeatureFactorParent;

class BMMFeatureFactor  : public BMMFeatureFactorParent {
public:
	BMMFeatureFactor(const SlotVariable_ptr& slotVar) 
		: _slotVar(slotVar), BMMFeatureFactorParent(NUM_CLASSES) {}
	const SlotVariable& variable() const { return *_slotVar;}
	static void finishedLoading(unsigned int n_classes);
private:
	friend class GraphicalModel::BagOfWordsFactor<BMMFeatureFactor,SlotVariable, 
			GraphicalModel::MultinomialDistribution<GraphicalModel::SymmetricDirichlet> >;
	SlotVariable_ptr _slotVar;
	static const double SMOOTHING;
};

class Document 
	: public GraphicalModel::Graph<Document>, public GraphicalModel::PRModifiableGraph<Document> ,
	public GraphicalModel::SupportsEM<Document>
{
public:
	/*GraphicalModel::ModifiableUnaryFactor& factor(unsigned int i) const;
	SlotVariable& variable(unsigned int i) const;
	GraphicalModel::ModifiableUnaryFactor& rootFactor(unsigned int i) const;*/
	//unsigned int nRootFactors() const;

	std::vector<SlotVariable_ptr> slots;
	std::vector<BMMFeatureFactor_ptr> featureFactors;
	std::vector<ArgTypePriorFactor_ptr> priorFactors;
	std::vector<std::wstring> words;

	double inferenceImpl();
	void finalize(const std::vector<FV_ptr>& featureVectors);
	static void finishedLoading();
private:
	friend class GraphicalModel::Graph<Document>;
	friend class GraphicalModel::SupportsEM<Document>;
	friend class GraphicalModel::PRModifiableGraph<Document>;
	void clearFactorModificationsImpl();
	void numberFactorsImpl(unsigned int& next_id);
	unsigned int nFactorsImpl() const;
	unsigned int nVariablesImpl() const;

	void initializeRandomlyImpl();
	void observeImpl();
	static void clearCountsImpl();
	template <typename Dumper>
	static void updateParametersFromCounts(const Dumper& dump);
};

typedef boost::shared_ptr<Document> Document_ptr;

class BMMFactorConstraintView {
public:
	typedef BMMFeatureFactor FactorType;
	typedef Document GraphType;

	size_t nFactors(const Document& d) const {
		return d.featureFactors.size();
	}

	const FactorType& factor(const Document& d, size_t i) const {
		assert(i>=0 && i<d.featureFactors.size());
		return *d.featureFactors[i];
	}

	FactorType& factor(Document& d, size_t i) const {
		assert(i>=0 && i<d.featureFactors.size());
		return *d.featureFactors[i];
	}

	const std::vector<double>& marginals(const Document& d,
			size_t factor_idx) const
	{
		return d.slots[factor_idx]->marginals();
	}
};

#endif

