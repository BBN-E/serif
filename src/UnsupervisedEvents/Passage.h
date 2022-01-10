/*#ifndef _PASSAGE_H_
#define _PASSAGE_H_
#include <vector>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/BoostUtil.h"
#include "../GraphicalModels/Graph.h"
#include "../GraphicalModels/VariablePriorFactor.h"
#include "../GraphicalModels/learning/EM.h"
#include "../GraphicalModels/distributions/Counts.h"
#include "../GraphicalModels/distributions/MultinomialDistribution.h"
#include "variables/RelationTypeVar.h"
#include "../GraphicalModels/pr/Constraint.h"

class DocTheory;
class SynNode;
class Symbol;

BSP_DECLARE(Passage)
BSP_DECLARE(PassageDescription)
BSP_DECLARE(RelationTypeBagOfWordsFactor)

BSP_DECLARE(RelationTypePrior)
typedef GraphicalModel::MultinomialDistribution<GraphicalModel::SymmetricDirichlet>
	RTPDist;
typedef GraphicalModel::VariablePriorFactor<RelationTypePrior,RelationTypeVariable,RTPDist>
	RTPParent;
class RelationTypePrior : public RTPParent {
public:
	RelationTypePrior(unsigned int n_classes) : RTPParent(n_classes) {}
	friend class GraphicalModel::VariablePriorFactor<RelationTypePrior,RelationTypeVariable,RTPDist>;
	static void finishedLoading(unsigned int n_classes) {
		RTPParent::finishedLoading(n_classes, 
				boost::make_shared<GraphicalModel::SymmetricDirichlet>(SMOOTHING));
	}
private:
	static const double SMOOTHING;
};

class Passage 
: public GraphicalModel::Graph<Passage>, public GraphicalModel::PRModifiableGraph<Passage>,
public GraphicalModel::SupportsEM<Passage>
{
	public:
		static Passage_ptr create(const PassageDescription_ptr& pd, const DocTheory* dt, 
				unsigned int n_relation_types, bool keep_passage = true);
		static void finishedLoading(unsigned int n_relation_classes,
				const GraphicalModel::SymmetricDirichlet_ptr& asymDir);

		const PassageDescription_ptr& description() const { return _description; }
		void setDescription(const PassageDescription_ptr& pd) { _description = pd; }
		const std::vector<double>& relationProbs() const { return _relTypeVar->marginals(); }

		void initializeRandomly();
		void observeImpl();
		static void clearCounts();
		template <typename Dumper>
		static void updateParametersFromCounts(const Dumper& dumper);
	
		const std::vector<unsigned int>& words() const;

		RelationTypeVariable& relationVar() {return *_relTypeVar;}
		const RelationTypeVariable& relationVar() const { return *_relTypeVar;}
		RelationTypeBagOfWordsFactor& documentFactor() { return *_relTypeBOWFact;}
		const RelationTypeBagOfWordsFactor& documentFactor() const {
			return *_relTypeBOWFact;
		}

		double logZ(const std::vector<boost::shared_ptr<GraphicalModel::Constraint<Passage> > >& constraints,
				const std::vector<boost::shared_ptr<GraphicalModel::Constraint<Passage> > >& instanceConstraints) const {
			throw UnrecoverableException("Passage::logZ()",
					"Function not yet implementated");
		}
	private:
		RelationTypeVariable_ptr _relTypeVar;
		RelationTypeBagOfWordsFactor_ptr _relTypeBOWFact;
		RelationTypePrior_ptr _relTypePrior;
		PassageDescription_ptr _description;

		friend class RelationDocWordsConstraintAdaptor;

		Passage() {}
		BOOST_MAKE_SHARED_0ARG_CONSTRUCTOR(Passage);

		friend class GraphicalModel::Graph<Passage>;
		friend class GraphicalModel::PRModifiableGraph<Passage>;

		unsigned int nFactorsImpl() const { return 2; }
		unsigned int nVariablesImpl() const { return 1; }

		void numberFactorsImpl(unsigned int& next_id);

		double inferenceImpl();

		void clearFactorModificationsImpl();
};

typedef boost::shared_ptr<Passage> Passage_ptr;


#include "factors/RelationTypeBagOfWordsFactor.h"

inline double Passage::inferenceImpl() {
	double LL = 0.0;
	_relTypeVar->clearMessages();
	_relTypeBOWFact->clearMessages();
	_relTypePrior->clearMessages();
	_relTypeBOWFact->sendMessages();
	_relTypePrior->sendMessages();
	_relTypeVar->receiveMessage();

	// calculate LL
	std::vector<double> messageProducts(_relTypeVar->nValues());
	_relTypeVar->productOfIncomingMessages(messageProducts);
	LL += logSumExp(messageProducts);
	return LL;
}

inline void Passage::numberFactorsImpl(unsigned int& next_id) {
	_relTypeBOWFact->setID(next_id++);
	_relTypePrior->setID(next_id++);
}

inline void Passage::clearFactorModificationsImpl() {
	_relTypeBOWFact->clearModifiers();
	_relTypePrior->clearModifiers();
}
		
inline const std::vector<unsigned int>& Passage::words() const { 
	return _relTypeBOWFact->features();
}

template <typename Dumper>
void Passage::updateParametersFromCounts(const Dumper& dumper) {
	RelationTypeBagOfWordsFactor::updateParametersFromCounts(dumper);
	RelationTypePrior::updateParametersFromCounts(dumper);
}

#endif
*/
