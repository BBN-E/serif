#ifndef _ACE_EVENT_H_
#define _ACE_EVENT_H_

#include <vector>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/BoostUtil.h"
#include "../GraphicalModels/Graph.h"
//#include "../GraphicalModels/VariablePriorFactor.h"
#include "../GraphicalModels/learning/EM.h"
#include "../GraphicalModels/distributions/Counts.h"
#include "../GraphicalModels/distributions/MultinomialDistribution.h"
#include "../GraphicalModels/pr/Constraint.h"
#include "ACEEntityFactorGroup.h"
#include "ACEEntityVariable.h"
#include "SentenceRange.h"
//#include "variables/RelationTypeVar.h"

class DocTheory;
class Entity;
class Symbol;
class DocPropForest;
class ProblemDefinition;
namespace GraphicalModel {
	class Alphabet;
	template<class GraphType> class DataSet;
};

class NoEventFound {};

BSP_DECLARE(ACEPassageDescription)
BSP_DECLARE(ACEEvent)
class ACEEvent 
: public GraphicalModel::Graph<ACEEvent>, public GraphicalModel::PRModifiableGraph<ACEEvent>,
public GraphicalModel::SupportsEM<ACEEvent>
{
	public:
		typedef std::vector<ACEEntityVariable_ptr> Variables;
		typedef std::vector<ACEEntityFactorGroup_ptr> Factors;
		typedef GraphicalModel::Constraint<ACEEvent> ACEEventConstraint;
		typedef boost::shared_ptr<ACEEventConstraint> ACEEventConstraint_ptr;
		typedef std::vector<ACEEventConstraint_ptr> ACEEventConstraints;

		static std::vector<ACEEvent_ptr> createFromDocument(const Symbol& docid, 
				const DocTheory* dt, const ProblemDefinition& problem, 
				bool keep_passage = true);
		static ACEEvent_ptr createFromPassage(const Symbol& docid,
				const DocTheory* dt, const ProblemDefinition& problem,
				const SentenceRange& passage);
		static void finishedLoading(const ProblemDefinition& problem);
		//const std::vector<unsigned int>& words() const { return _words; }
		//const std::vector<double>& relationProbs() const { return _relTypeVar->marginals(); }

		void initializeRandomly();
		void observeImpl();
		static void clearCounts();
		template <typename Dumper>
		static void updateParametersFromCounts(Dumper& dumper) {
			ACEEntityFactorGroup::updateParametersFromCounts(dumper);
		}

		const std::vector<double>& probsOfEntity(unsigned int i) const {
			return _entityVariables[i]->marginals();
		}

		unsigned int nEntities() const {
			return _entityVariables.size();
		}

		const ACEPassageDescription& passage() const {return *_passage;};

		const ACEEntityVariable& argumentTypeVar(unsigned int idx) const {
			return *_entityVariables[idx];
		}

		const ACEEntityFactorGroup& factors(unsigned int i) const {
			return *_entityFactors[i];
		}

		ACEEntityFactorGroup& factors(unsigned int i) {
			return *_entityFactors[i];
		}

		void debugInfo(unsigned int idx, const ProblemDefinition& problem,
				std::wostream& out) const; 

		double logZ(const ACEEventConstraints& constraints,
				const ACEEventConstraints& instanceConstraints) const;

		void instanceDebugInfo(const ProblemDefinition& problem,
				std::wostream& out) const;

		int gold(unsigned int entity_idx) const;

		static void loadFromDocTable(const std::string& docTable,
				const ProblemDefinition& problem, 
				GraphicalModel::DataSet<ACEEvent>& events,
				int max_docs = -1);
		static bool foundInSentenceRange(const Entity* e, SentenceRange passage);

		void findConflictingConstraints(
				std::vector<GraphicalModel::Constraint<ACEEvent>* >& conflictingConstraints) const;

		void dumpGraph() const;

		std::vector<unsigned int> orderByMention;
	private:
		ACEEvent(unsigned int idx);
		BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(ACEEvent, unsigned int);

		static unsigned int _next_index;

		Variables _entityVariables;
		Factors _entityFactors;
		ACEPassageDescription_ptr _passage;
		unsigned int _idx;

		friend class GraphicalModel::Graph<ACEEvent>;
		friend class GraphicalModel::PRModifiableGraph<ACEEvent>;

		unsigned int nFactorsImpl() const { 
			return ACEEntityFactorGroup::size() * _entityFactors.size(); 
		}
		unsigned int nVariablesImpl() const { return _entityVariables.size(); }

		void numberFactorsImpl(unsigned int& next_id);

		double inferenceImpl();

		void clearFactorModificationsImpl();

		static SentenceRange getBestSentenceRange(const DocTheory* dt,
				const ProblemDefinition& problem);

		static std::vector<SentenceRange> sentencesWithEvents(const DocTheory* dt,
			const Symbol& eventType);
		void addEntity(const DocTheory* dt, const DocPropForest& forest,
				SentenceRange passage, const Entity* e, const ProblemDefinition& problem);
		void calculateOrderByMention(const DocTheory* dt, const ACEPassageDescription& range);

		//void gatherWords(const SynNode* node, GraphicalModel::Alphabet& alphabet);
};

inline double ACEEvent::inferenceImpl() {
	double LL = 0.0;
	for (ACEEvent::Variables::iterator vIt=_entityVariables.begin();
			vIt!=_entityVariables.end(); ++vIt)
	{
		(*vIt)->clearMessages();
	}
	for (ACEEvent::Factors::iterator fIt=_entityFactors.begin();
			fIt!=_entityFactors.end(); ++fIt)
	{
		(*fIt)->clearMessages();
	}
	for (ACEEvent::Factors::iterator fIt=_entityFactors.begin();
			fIt!=_entityFactors.end(); ++fIt)
	{
		(*fIt)->sendMessages();
	}
	for (ACEEvent::Variables::iterator vIt=_entityVariables.begin();
			vIt!=_entityVariables.end(); ++vIt)
	{
		(*vIt)->receiveMessage();
	}

	// calculate LL
	for (ACEEvent::Variables::iterator vIt=_entityVariables.begin();
			vIt!=_entityVariables.end(); ++vIt)
	{
		std::vector<double> messageProducts((*vIt)->nValues());
		(*vIt)->productOfIncomingMessages(messageProducts);
		LL += logSumExp(messageProducts);
	}
	return LL;
}

inline void ACEEvent::numberFactorsImpl(unsigned int& next_id) {
	for (ACEEvent::Factors::iterator fIt=_entityFactors.begin();
			fIt!=_entityFactors.end(); ++fIt)
	{
		(*fIt)->setID(next_id);
	}
}

inline void ACEEvent::clearFactorModificationsImpl() {
	for (ACEEvent::Factors::iterator fIt=_entityFactors.begin();
			fIt!=_entityFactors.end(); ++fIt)
	{
		(*fIt)->clearModifiers();
	}
}

class ACEDummyFactorView {
	public:
		typedef ACEDummyFactor FactorType;
		typedef ACEEvent GraphType;

		size_t nFactors(const ACEEvent& event) const {
			return event.nEntities();
		}

		const FactorType& factor(const ACEEvent& e, size_t i) const {
			return e.factors(i).dummyFactor();
		}

		const FactorType& factorForVar(const ACEEvent& e, size_t i) const {
			return factor(e, i);
		}

		FactorType& factor(ACEEvent& e, size_t i) const {
			return e.factors(i).dummyFactor();
		}

		const std::vector<double>& marginals(const ACEEvent& e,
				size_t factor_idx) const 
		{
			return e.argumentTypeVar(factor_idx).marginals();
		}
};



class HeadWordConstraintView {
	public:
		typedef ACEHeadWordFactor FactorType;
		typedef ACEEvent GraphType;

		size_t nFactors(const ACEEvent& event) const {
			return event.nEntities();
		}

		const FactorType& factor(const ACEEvent& e, size_t i) const {
			return e.factors(i).headWordFactor();
		}

		const FactorType& factorForVar(const ACEEvent& e, size_t i) const {
			return factor(e, i);
		}

		FactorType& factor(ACEEvent& e, size_t i) const {
			return e.factors(i).headWordFactor();
		}

		const std::vector<double>& marginals(const ACEEvent& e,
				size_t factor_idx) const 
		{
			return e.argumentTypeVar(factor_idx).marginals();
		}
};

class PropDepConstraintView {
	public:
		typedef ACEPropDependencyFactor FactorType;
		typedef ACEEvent GraphType;

		size_t nFactors(const ACEEvent& event) const {
			return event.nEntities();
		}

		const FactorType& factor(const ACEEvent& e, size_t i) const {
			return e.factors(i).propDependencyFactor();
		}

		const FactorType& factorForVar(const ACEEvent& e, size_t i) const {
			return factor(e, i);
		}

		FactorType& factor(ACEEvent& e, size_t i) const {
			return e.factors(i).propDependencyFactor();
		}

		const std::vector<double>& marginals(const ACEEvent& e,
				size_t factor_idx) const 
		{
			return e.argumentTypeVar(factor_idx).marginals();
		}
};

class EntityTypeConstraintView {
	public:
		typedef ACEEntityTypeFactor FactorType;
		typedef ACEEvent GraphType;

		size_t nFactors(const ACEEvent& event) const {
			return event.nEntities();
		}

		const FactorType& factor(const ACEEvent& e, size_t i) const {
			return e.factors(i).entityTypeFactor();
		}

		const FactorType& factorForVar(const ACEEvent& e, size_t i) const {
			return factor(e, i);
		}

		FactorType& factor(ACEEvent& e, size_t i) const {
			return e.factors(i).entityTypeFactor();
		}

		const std::vector<double>& marginals(const ACEEvent& e,
				size_t factor_idx) const 
		{
			return e.argumentTypeVar(factor_idx).marginals();
		}
};

#endif

