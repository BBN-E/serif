#include "Generic/common/leak_detection.h"
#include "ACEEntityFactorGroup.h"
#include <string>
#include <sstream>
#include <set>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/Symbol.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/SynNode.h"
#include "Generic/PropTree/PropNode.h"
#include "Generic/PropTree/DocPropForest.h"
#include "Generic/PropTree/MentionToPropNodeMap.h"
#include "GraphicalModels/Message.h"
#include "GraphicalModels/pr/Constraint.h"

using std::wstring;
using std::set;
using std::wstringstream;
using boost::lexical_cast;
using boost::make_shared;

const double ACEArgumentTypePrior::SMOOTHING = 10;


ACEEntityTypeFactor::ACEEntityTypeFactor(unsigned int n_arg_types,
		const Entity* e) : ACEETFParent(n_arg_types, toFeature(e))
{ }

wstring ACEEntityTypeFactor::toFeature(const Entity* e) {
	assert(e);
	Symbol type = e->getType().getName().to_string();
	return type.to_string();
}


ACEEntitySizeFactor::ACEEntitySizeFactor(unsigned int n_arg_types,
		const Entity* e) : ACEESFParent(n_arg_types, toFeature(e))
{ }

wstring ACEEntitySizeFactor::toFeature(const Entity* e) {
	assert(e);
	unsigned int n_mentions = e->getNMentions();
	if (n_mentions > 6) {
		n_mentions = 6;
	}

	return lexical_cast<wstring>(n_mentions);
}

ACETopLevelMentionFactor::ACETopLevelMentionFactor(unsigned int n_arg_types,
		const DocTheory* dt, const Entity* e, SentenceRange passage) 
: ACETLMFParent(n_arg_types, toFeature(dt, e, passage))
{}

wstring ACETopLevelMentionFactor::toFeature(const DocTheory* dt, const Entity* e,
		SentenceRange passage)
{
	assert(e);
	bool top = false;
	for (int i=0; !top && i<e->getNMentions(); ++i) {
		MentionUID m_id = e->getMention(i);
		int sn = Mention::getSentenceNumberFromUID(m_id);
		if (sn>=passage.first && sn <= passage.second) {
			bool ancestor_mention = false;
			const MentionSet* ms = dt->getSentenceTheory(sn)->getMentionSet();
			const Mention* m = ms->getMention(m_id);
			const SynNode* s = m->getNode()->getParent();

			while (s && !ancestor_mention) {
				if (s->getMentionIndex() != -1) {
					const Mention* m = ms->getMentionByNode(s);
					if (m) {
						const Entity* e = m->getEntity(dt);
						if (e) {
							// only mentions associated with entities count
							// as ancestors for this feature
							ancestor_mention = true;
						}
					}
				}
				s = s->getParent();
			}

			if (!ancestor_mention) {
				top = true;
			}
		}
	}
	
	if (top) {
		return L"top";
	} else {
		return L"non-top";
	}
}

ACEEntityFactorGroup::ACEEntityFactorGroup(const DocTheory* dt, 
		const DocPropForest& forest, SentenceRange passage,
		const Entity* e, const ACEEntityVariable_ptr& ev)   : 
	_sizeFactor(ev->nValues(), e), _topLevelFactor(ev->nValues(), dt, e, passage),
	_typeFactor(ev->nValues(), e), _priorFactor(ev->nValues()), 
	_headWordFactor(ev->nValues(), dt, e, passage),
	_propFactor(ev->nValues(), dt, forest, passage, e), _dummyFactor(ev->nValues())
{ }


ACEHeadWordFactor::ACEHeadWordFactor(unsigned int n_argument_types, 
		const DocTheory* dt, const Entity* e, SentenceRange passage) : ACEHWPar(n_argument_types)
{
	static const Symbol NULL_SYM;

	// use a set to ensure each feature is only added once
	set<wstring> feats;
	for (int i=0; i<e->getNMentions(); ++i) {
		MentionUID m_id = e->getMention(i);
		int sn = Mention::getSentenceNumberFromUID(m_id);
		if (sn >= passage.first && sn <= passage.second) {
			const Mention* m = dt->getSentenceTheory(sn)->getMentionSet()->getMention(m_id);
			const SynNode* s = m->getNode();

			Symbol hw = s->getHeadWord();

			if (hw != NULL_SYM) {
				wstring s = hw.to_string();
				std::transform(s.begin(), s.end(), s.begin(), ::tolower);
				feats.insert(s);
			}
		}
	}

	BOOST_FOREACH(const wstring& f, feats) {
		addFeature(f);
	}
}

void addIfNotPresent(std::vector<PropNode::ChildRole>& vec, PropNode::ChildRole item) {
	std::vector<PropNode::ChildRole>::const_iterator it = vec.begin();
	for (; it!=vec.end(); ++it) {
		const PropNode::ChildRole& cr = *it;
		if (cr.get<0>() == item.get<0>() &&
				cr.get<1>() == item.get<1>())
		{
			return;
		}
	}

	vec.push_back(item);
}

void ACEPropDependencyFactor::findRealParents(const PropNode_ptr& node, 
		std::vector<PropNode::ChildRole>& ret)
{
	if (node->hasParent()) {
		for (size_t i=0; i<node->getNParents(); ++i) {
			PropNode_ptr par = node->getParent(i);
			Symbol role;
			try {
				role = par->getRoleForChild(node);
				if (role == Argument::MEMBER_ROLE) {
					findRealParents(par, ret);
				} else {
					PropNode::ChildRole parRole = PropNode::ChildRole(par, role);
					addIfNotPresent(ret, parRole);
				}
			} catch (UnrecoverableException&) {
				SessionLogger::warn("swallowed_prop_exception")
					<< L"ACEPropDependencyFactor::findRealParents "
					<< L" swallowed exception from PropNode::getRoleForChild";
			}
		}
	} else {
		addIfNotPresent(ret, PropNode::ChildRole(PropNode_ptr(), Symbol()));
	}
}

std::vector<PropNode::ChildRole> ACEPropDependencyFactor::findRealParents(
		const PropNode_ptr& node)
{
	std::vector<PropNode::ChildRole> ret;
	findRealParents(node, ret);
	return ret;
}

ACEPropDependencyFactor::ACEPropDependencyFactor(unsigned int n_argument_types, 
		const DocTheory* dt, const DocPropForest& forest, SentenceRange passage,
		const Entity* e) : ACEPDPar(n_argument_types)
{
	static Symbol NULL_SYM;

	// use a set to ensure each feature is only added once
	set<wstring> feats;

	MentionToPropNodeMap_ptr mToP = MentionToPropNode::buildMap(forest,
			passage.first, passage.second);
	PropNodes_ptr nodes = MentionToPropNode::propNodesForEntity(e, dt, *mToP);

	BOOST_FOREACH(const PropNode_ptr& node, *nodes) {
		std::vector<PropNode::ChildRole> parentAndRoles = findRealParents(node);
		BOOST_FOREACH(PropNode::ChildRole parentAndRole, parentAndRoles) {
			const PropNode_ptr& par = parentAndRole.get<0>();
			if (par) {
				const Predicate* pred = par->getRepresentativePredicate();
				if (pred) {
					Symbol role = parentAndRole.get<1>();
					Symbol predSym = pred->pred();

					if (role != NULL_SYM && predSym != NULL_SYM) {
						wstring roleStr = role.to_string();

						wstringstream str;
						str << predSym.to_string() << L"_" << roleStr;
						feats.insert(str.str());

						if (!roleStr.empty() && roleStr[0] != '<') {
							wstringstream roleF;
							roleF << L"_" << roleStr;
							feats.insert(roleF.str());
						}
					} 
				}
			}
		}

		for (size_t i = 0; i<node->getNChildren(); ++i) {
			const PropNode_ptr& kid = node->getChildren()[i];
			const Predicate* pred = kid->getRepresentativePredicate();
			if (pred) {
				Symbol role = node->getRoles()[i];
				Symbol predSym = pred->pred();

				if (role != NULL_SYM && predSym != NULL_SYM) {
					wstringstream str;
					str << role.to_string() << L"_" << predSym.to_string();
					feats.insert(str.str());
				}
			}
		}
	}

	BOOST_FOREACH(const wstring& f, feats) {
		addFeature(f);
	}
}

void ACEEntityFactorGroup::debugInfo(const ProblemDefinition& problem,
		std::wostream& out) const 
{
	_sizeFactor.dumpOutgoingMessagesToHTML(L"Size", out, true);
	_topLevelFactor.dumpOutgoingMessagesToHTML(L"Top", out, true);
	_typeFactor.dumpOutgoingMessagesToHTML(L"Type", out, true);
	_priorFactor.dumpOutgoingMessagesToHTML(L"Prior", out, true);
	_headWordFactor.dumpOutgoingMessagesToHTML(L"HW", out, true);
	_propFactor.dumpOutgoingMessagesToHTML(L"Prop", out, true);
	_dummyFactor.dumpOutgoingMessagesToHTML(L"Inst", out, true);
}

void ACEEntityFactorGroup::findConflictingConstraints(
		std::vector<GraphicalModel::Constraint<ACEEvent>* >& conflictingConstraints) const
{
	posNegConflicts(conflictingConstraints);
}

void ACEEntityFactorGroup::posNegConflicts(
		std::vector<GraphicalModel::Constraint<ACEEvent>* >& conflictingConstraints) const
{
	for (unsigned int assignment = 0; assignment < _priorFactor.nModifiers();
			++assignment)
	{
		GraphicalModel::ModificationInfo<ACEEvent> mod = 
			_sizeFactor.maxModifier(assignment);
		mod.combine(_topLevelFactor.maxModifier(assignment));
		mod.combine(_priorFactor.maxModifier(assignment));
		mod.combine(_typeFactor.maxModifier(assignment));
		mod.combine(_headWordFactor.maxModifier(assignment));
		mod.combine(_propFactor.maxModifier(assignment));
		mod.combine(_dummyFactor.maxModifier(assignment));

		if (mod.conflict() > 5.0) {
			if (mod.positive.constraint) {
				conflictingConstraints.push_back(mod.positive.constraint);
			}
			if (mod.negative.constraint) {
				conflictingConstraints.push_back(mod.negative.constraint);
			}
		}
	}
}

double ACEEntityFactorGroup::minMaxNegativeConflict() const {
	double conflict = -std::numeric_limits<double>::max();
	for (unsigned int assignment = 0; assignment < _priorFactor.nModifiers();
			++assignment)
	{
		conflict = 
			(std::max)(conflict, _sizeFactor.maxModifier(assignment).negative.magnitude);
		conflict = 
			(std::max)(conflict, _topLevelFactor.maxModifier(assignment).negative.magnitude);
		conflict = 
			(std::max)(conflict, _priorFactor.maxModifier(assignment).negative.magnitude);
		conflict = 
			(std::max)(conflict, _typeFactor.maxModifier(assignment).negative.magnitude);
		conflict = 
			(std::max)(conflict, _headWordFactor.maxModifier(assignment).negative.magnitude);
		conflict = 
			(std::max)(conflict, _propFactor.maxModifier(assignment).negative.magnitude);
		conflict = 
			(std::max)(conflict, _dummyFactor.maxModifier(assignment).negative.magnitude);
	}
	return -conflict;
}

void updateNumOverThresholdAndMax(const GraphicalModel::ModificationInfo<ACEEvent>& mod,
		size_t& num_over_threshold, double& mx)
{
	static const double THRESHOLD = 10.0;
	if (mod.positive.magnitude > THRESHOLD) {
		++num_over_threshold;
	}
	if (mod.positive.magnitude > mx) {
		mx = mod.positive.magnitude;
	}
}

double ACEEntityFactorGroup::duelingPositiveConflict() const {
	size_t num_over_threshold = 0;
	double mx = 0.0;
	for (unsigned int assignment = 0; assignment<_priorFactor.nModifiers();
			++assignment) 
	{
		updateNumOverThresholdAndMax(_sizeFactor.maxModifier(assignment),
				num_over_threshold, mx);
		updateNumOverThresholdAndMax(_topLevelFactor.maxModifier(assignment),
				num_over_threshold, mx);
		updateNumOverThresholdAndMax(_priorFactor.maxModifier(assignment),
				num_over_threshold, mx);
		updateNumOverThresholdAndMax(_headWordFactor.maxModifier(assignment),
				num_over_threshold, mx);
		updateNumOverThresholdAndMax(_typeFactor.maxModifier(assignment),
				num_over_threshold, mx);
		updateNumOverThresholdAndMax(_propFactor.maxModifier(assignment),
				num_over_threshold, mx);
		updateNumOverThresholdAndMax(_dummyFactor.maxModifier(assignment),
				num_over_threshold, mx);

	}

	if (num_over_threshold > 1) {
		return mx;
	} else {
		return 0.0;
	}
}

