#ifndef _ACE_ENTITY_FACTOR_GROUP_H_
#define _ACE_ENTITY_FACTOR_GROUP_H_
#include <iostream>
#include "Generic/common/bsp_declare.h"
#include "Generic/PropTree/PropNode.h"
#include "../GraphicalModels/SingleFeatureDistributionFactor.h"
#include "../GraphicalModels/VariablePriorFactor.h"
#include "../GraphicalModels/distributions/Counts.h"
#include "../GraphicalModels/distributions/MultinomialDistribution.h"
#include "../GraphicalModels/BagOfWordsFactor.h"
#include "../GraphicalModels/DummyFactor.h"
#include "ACEEntityVariable.h"
#include "ProblemDefinition.h"
#include "SentenceRange.h"

BSP_DECLARE(ACEEntityFactorGroup)
class Entity;
class DocTheory;
class DocPropForest;
class ACEEvent;

BSP_DECLARE(ACEArgumentTypePrior)
typedef GraphicalModel::MultinomialDistribution<GraphicalModel::AsymmetricDirichlet>
	ATPDist;
typedef GraphicalModel::VariablePriorFactor<ACEEvent, ACEArgumentTypePrior,
		ACEEntityVariable,ATPDist> ATPParent;
class ACEArgumentTypePrior : public ATPParent {
	public:
	ACEArgumentTypePrior(unsigned int n_arg_types) : ATPParent(n_arg_types) {}
	friend class GraphicalModel::VariablePriorFactor<ACEEvent, ACEArgumentTypePrior,
		   ACEEntityVariable,ATPDist>;
	static void finishedLoading(unsigned int n_classes, 
			const boost::shared_ptr<GraphicalModel::AsymmetricDirichlet>& prior)
	{
		ATPParent::finishedLoading(n_classes, prior);
	}
private:
	static const double SMOOTHING;
};

BSP_DECLARE(ACEEntityTypeFactor)
typedef GraphicalModel::SingleFeatureDistributionFactorSymmetricPrior<
	ACEEvent, ACEEntityTypeFactor,ACEEntityVariable> ACEETFParent;
class ACEEntityTypeFactor : public ACEETFParent {
	public:
		ACEEntityTypeFactor(unsigned int n_arg_types, const Entity* e);
		static std::wstring toFeature(const Entity* e);
};

BSP_DECLARE(ACETopLevelMentionFactor)
typedef GraphicalModel::SingleFeatureDistributionFactorSymmetricPrior<
	ACEEvent, ACETopLevelMentionFactor,ACEEntityVariable> ACETLMFParent;
class ACETopLevelMentionFactor : public ACETLMFParent {
	public:
		ACETopLevelMentionFactor(unsigned int n_arg_types, const DocTheory* dt,
				const Entity* e, SentenceRange passage);
		static std::wstring toFeature(const DocTheory* dt,
				const Entity* e, SentenceRange passage);
};

BSP_DECLARE(ACEEntitySizeFactor)
typedef GraphicalModel::SingleFeatureDistributionFactorSymmetricPrior<
	ACEEvent, ACEEntitySizeFactor,ACEEntityVariable> ACEESFParent;
class ACEEntitySizeFactor : public ACEESFParent {
	public:
		ACEEntitySizeFactor(unsigned int n_arg_types, const Entity* e);
	private:
		static std::wstring toFeature(const Entity* e);
};

BSP_DECLARE(ACEHeadWordFactor)
typedef GraphicalModel::BagOfWordsFactor<ACEEvent, ACEHeadWordFactor, 
		ACEEntityVariable, GraphicalModel::MultiSymPrior> ACEHWPar;
class ACEHeadWordFactor : public ACEHWPar {
	public:
		ACEHeadWordFactor(unsigned int n_argument_types, const DocTheory* dt,
				const Entity* e, SentenceRange passage);
};

BSP_DECLARE(ACEPropDependencyFactor)
typedef GraphicalModel::BagOfWordsFactor<ACEEvent, ACEPropDependencyFactor,
		ACEEntityVariable, GraphicalModel::MultiSymPrior> ACEPDPar;
class ACEPropDependencyFactor : public ACEPDPar {
	public:
		ACEPropDependencyFactor(unsigned int n_argument_types, const DocTheory* dt,
				const DocPropForest& forest, SentenceRange passage, const Entity* e);
	private:
		void findRealParents(const PropNode_ptr& node, 
				std::vector<PropNode::ChildRole>& ret);
		std::vector<PropNode::ChildRole> findRealParents(const PropNode_ptr& node);
};

BSP_DECLARE(ACEDummyFactor)
typedef GraphicalModel::DummyFactor<ACEEvent, ACEDummyFactor, ACEEntityVariable> DummyPar;
class ACEDummyFactor : public DummyPar {
public:
	ACEDummyFactor(unsigned int n_argument_types) : DummyPar(n_argument_types) {}
};

class ACEEntityFactorGroup {
public:
	ACEEntityFactorGroup(const DocTheory* dt, const DocPropForest& forest,
			SentenceRange passage, const Entity* e, 
			const ACEEntityVariable_ptr& ev);

	void observe(const ACEEntityVariable& v) {
		_sizeFactor.observe(v);
		_topLevelFactor.observe(v);
		_typeFactor.observe(v);
		_priorFactor.observe(v);
		_headWordFactor.observe(v);
		_propFactor.observe(v);
		_dummyFactor.observe(v);
	}

	static void finishedLoading(const ProblemDefinition& problem)
	{
		static const double HEAD_WORD_SMOOTHING = 3;
		static const double PROP_DEP_SMOOTHING = 3;
		ACEEntityTypeFactor::finishedLoading(problem.nClasses(), 10.0);
		ACEEntitySizeFactor::finishedLoading(problem.nClasses(), 10.0);
		ACETopLevelMentionFactor::finishedLoading(problem.nClasses(), 10.0);
		ACEArgumentTypePrior::finishedLoading(problem.nClasses(),
				boost::make_shared<GraphicalModel::AsymmetricDirichlet>(problem.typePriors()));
		ACEHeadWordFactor::finishedLoading(problem.nClasses(), false,
				boost::make_shared<GraphicalModel::SymmetricDirichlet>(HEAD_WORD_SMOOTHING));
		ACEPropDependencyFactor::finishedLoading(problem.nClasses(), false, 
				boost::make_shared<GraphicalModel::SymmetricDirichlet>(PROP_DEP_SMOOTHING));
		ACEDummyFactor::finishedLoading();
	}

	static void clearCounts() {
		ACEEntityTypeFactor::clearCounts();
		ACEEntitySizeFactor::clearCounts();
		ACETopLevelMentionFactor::clearCounts();
		ACEArgumentTypePrior::clearCounts();
		ACEHeadWordFactor::clearCounts();
		ACEPropDependencyFactor::clearCounts();
		ACEDummyFactor::clearCounts();
	}

	template <typename Dumper>
	static void updateParametersFromCounts(Dumper& dumper) {
		ACEEntityTypeFactor::updateParametersFromCounts(dumper);
		ACEEntitySizeFactor::updateParametersFromCounts(dumper);
		ACETopLevelMentionFactor::updateParametersFromCounts(dumper);
		ACEArgumentTypePrior::updateParametersFromCounts(dumper);
		ACEHeadWordFactor::updateParametersFromCounts(dumper);
		ACEPropDependencyFactor::updateParametersFromCounts(dumper);
		ACEDummyFactor::updateParametersFromCounts(dumper);
	}

	void clearMessages() {
		_sizeFactor.clearMessages();
		_topLevelFactor.clearMessages();
		_typeFactor.clearMessages();
		_priorFactor.clearMessages();
		_headWordFactor.clearMessages();
		_propFactor.clearMessages();
		_dummyFactor.clearMessages();
	}

	void sendMessages() {
		_sizeFactor.sendMessages();
		_topLevelFactor.sendMessages();
		_typeFactor.sendMessages();
		_priorFactor.sendMessages();
		_headWordFactor.sendMessages();
		_propFactor.sendMessages();
		_dummyFactor.sendMessages();
	}

	void clearModifiers() {
		_sizeFactor.clearModifiers();
		_topLevelFactor.clearModifiers();
		_typeFactor.clearModifiers();
		_priorFactor.clearModifiers();
		_headWordFactor.clearModifiers();
		_propFactor.clearModifiers();
		_dummyFactor.clearModifiers();
	}

	void setID(unsigned int& next_id) {
		_sizeFactor.setID(next_id++);
		_topLevelFactor.setID(next_id++);
		_typeFactor.setID(next_id++);
		_priorFactor.setID(next_id++);
		_headWordFactor.setID(next_id++);
		_propFactor.setID(next_id++);
		_dummyFactor.setID(next_id++);
	}

	void connectToVar(ACEEntityVariable& var) {
		connect(_sizeFactor, var);
		connect(_topLevelFactor, var);
		connect(_typeFactor, var);
		connect(_priorFactor, var);
		connect(_headWordFactor, var);
		connect(_propFactor, var);
		connect(_dummyFactor, var);
	}

	static unsigned int size() { return 7;}

	ACEHeadWordFactor& headWordFactor() {
		return _headWordFactor;
	}

	const ACEHeadWordFactor& headWordFactor() const {
		return _headWordFactor;
	}

	ACEPropDependencyFactor& propDependencyFactor() {
		return _propFactor;
	}

	const ACEPropDependencyFactor& propDependencyFactor() const {
		return _propFactor;
	}

	ACEEntityTypeFactor& entityTypeFactor() {
		return _typeFactor;
	}

	const ACEEntityTypeFactor& entityTypeFactor() const {
		return _typeFactor;
	}

	const ACEDummyFactor& dummyFactor() const {
		return _dummyFactor;
	}

	ACEDummyFactor& dummyFactor() {
		return _dummyFactor;
	}

	void debugInfo(const ProblemDefinition& problem, std::wostream& out) const;

	void findConflictingConstraints(std::vector<GraphicalModel::Constraint<ACEEvent>* >&
			conflictingConstraints) const;

private:
	ACEEntitySizeFactor _sizeFactor;
	ACETopLevelMentionFactor _topLevelFactor;
	ACEEntityTypeFactor _typeFactor;
	ACEArgumentTypePrior _priorFactor;
	ACEHeadWordFactor _headWordFactor;
	ACEPropDependencyFactor _propFactor;
	// this contributes nothing on its own and exists only to
	// absorb constraint weight which don't really apply to other factors
	ACEDummyFactor _dummyFactor;

	//double maxPositiveNegativeForSameAssignmentConflict() const;
	void posNegConflicts(
		std::vector<GraphicalModel::Constraint<ACEEvent>* >& conflictingConstraints) const;
	double minMaxNegativeConflict() const;
	double duelingPositiveConflict() const;
};

#endif

