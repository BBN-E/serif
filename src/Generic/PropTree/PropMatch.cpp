// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/PropTree/PropMatch.h"
#include "common/UnexpectedInputException.h"

PropMatch::ConfusionProbMatrix PropMatch::_baseRoleConfusionProbs;
PropMatch::ConfusionProbMatrix PropMatch::_baseTypeConfusionProbs;

//PropMatch::ConfusionProbMatrix PropMatch::_roleConfusionProbs;
//PropMatch::ConfusionProbMatrix PropMatch::_typeConfusionProbs;

using namespace std;

PropMatch::PropMatch(
	const PropNodes & nodes,
	const std::vector<float> & weights /*= std::vector<float>()*/)
 :
	_covered(nodes.size()),
	_covering(nodes.size()),
	_weights(weights.empty() ? vector<float>(nodes.size(), 1) : weights)
{
	if(nodes.size() != _weights.size())
		throw UnexpectedInputException("PropMatch::PropMatch()",
		"Input nodes and weights must have same size");
	
	PropNodes::const_iterator cur = nodes.begin(), nxt = nodes.begin();
	for(; nxt != nodes.end(); cur = nxt, ++nxt)
	{
		if(!PropNode::propnode_id_cmp()(*nxt, *cur)) continue;
		throw UnexpectedInputException("PropMatch::PropMatch()",
			"Input nodes must be sorted on PropNode::getPropNodeID()");
	}		

	if (_baseRoleConfusionProbs.size()==0) {
		_populateBaseRoleConfusionProbs();
	}
	_roleConfusionProbs=&_baseRoleConfusionProbs;


	if (_baseTypeConfusionProbs.size()==0) {
		_populateBaseTypeConfusionProbs();
	}
	_typeConfusionProbs=&_baseTypeConfusionProbs;

	_valid.resize(nodes.size());
	fill(_valid.begin(),  _valid.end(),  true);
}

// clears _covered & _covering, invokes computeCoverage(),
// and computes an overall coverage score in range [0,1].
// If multiplicative is true, it returns simply the product of
// all the node matches, ignoring weights and groups and so on
float PropMatch::compareToTarget(const PropNodes & nodes,
										   bool multiplicative) {
	if( !_covered.size())
		return 0.0f;
	
	// clear statistics
	fill(_covered.begin(),  _covered.end(),  0.0f);
	fill(_covering.begin(), _covering.end(), PropNode_ptr());

	// virtual call
	computeCoverage(nodes);
	return computeScoreFromCoverage(multiplicative);
}
// defines P( target pred type | source pred type )
float PropMatch::predicateTypeConfusionProb(const Predicate & src_pred, 
													  const Predicate & trg_pred) const {
	if( trg_pred.type() == src_pred.type() ) return 1.0f;
	
	ConfusionProbMatrix::const_iterator probEntryForward=
		_typeConfusionProbs->find(std::make_pair(src_pred.type(), trg_pred.type()));
	ConfusionProbMatrix::const_iterator probEntryBackward=
		_typeConfusionProbs->find(std::make_pair(trg_pred.type(), src_pred.type()));

	if (probEntryForward!=_typeConfusionProbs->end()) {
		return probEntryForward->second;
	} else if (probEntryBackward!=_typeConfusionProbs->end()) {
		return probEntryBackward->second;
	} else {
		return 0.10f;
	}
}

// defines P( target role | source role )
float PropMatch::roleConfusionProb(const Symbol & src_role, const Symbol & trg_role) const {
	
	if(src_role == Symbol(L"")) return 1;

	if(trg_role == src_role) return 1;
	
	ConfusionProbMatrix::const_iterator probEntryForward=
		_roleConfusionProbs->find(std::make_pair(src_role, trg_role));
	ConfusionProbMatrix::const_iterator probEntryBackward=
		_roleConfusionProbs->find(std::make_pair(trg_role, src_role));

	if (probEntryForward!=_roleConfusionProbs->end()) {
		return probEntryForward->second;
	} else if (probEntryBackward!=_roleConfusionProbs->end()) {
		return probEntryBackward->second;
	} else {
		return 0.60f;
	}
}

float PropMatch::computeScoreFromCoverage(bool multiplicative) const {

	// ignores groups, weights, and so on...
	if (multiplicative) {
		float ret=1.0f;
		for (size_t i=0; i!=_covered.size(); ++i) {
			if(_valid[i]) ret*=_covered[i];
		}
		return ret;
	} else {
		if(_covered.size() == 0)
			return 0.0f;
		// accumulate statistics
		float score = 0.0f;
		float total_weight = 0.0f;
		for(size_t i = 0; i != _covered.size(); i++)
		{
			if(_valid[i]){
				score += _covered[i] * _weights[i];
				total_weight += _weights[i];
			}

		}
		float ret = score/total_weight;
		return ret;
	}
}

void PropMatch::_populateBaseRoleConfusionProbs() {
		_baseRoleConfusionProbs[make_pair(L"of",	L"by")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"of",	L"<poss>")] = 0.99f;
		_baseRoleConfusionProbs[make_pair(L"of",	L"<unknown>")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"by",	L"<poss>")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"by",	L"<unknown>")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"<poss>",L"<unknown>")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"in",	L"at")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"in",	L"on")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"in",	L"<unknown>")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"at",	L"on")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"at",	L"<unknown>")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"on",	L"<unknown>")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"<member>",	L"<unknown>")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"to",	L"<iobj>")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"<obj>",L"<iobj>")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"<obj>",L"of")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"<sub>",L"of")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"<sub>",L"<poss>")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"<sub>",L"by")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"from",	L"in")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"to",	L"in")] = 0.8f;
		_baseRoleConfusionProbs[make_pair(L"<mod>", L"<temp>")]=0.8f;	
}

void PropMatch::_populateBaseTypeConfusionProbs()  {
	_baseTypeConfusionProbs[make_pair(Predicate::NAME_TYPE, Predicate::DESC_TYPE)] = 0.7f;
	_baseTypeConfusionProbs[make_pair(Predicate::NAME_TYPE, Predicate::PRON_TYPE)] = 0.8f;
	_baseTypeConfusionProbs[make_pair(Predicate::DESC_TYPE, Predicate::PRON_TYPE)] = 0.4f;
	// nominalizations
	_baseTypeConfusionProbs[make_pair(Predicate::DESC_TYPE, Predicate::VERB_TYPE)] = 0.8f; 		

	_baseTypeConfusionProbs[make_pair(Predicate::MOD_TYPE, Predicate::DESC_TYPE)] = 0.8f; 		
	_baseTypeConfusionProbs[make_pair(Predicate::DESC_TYPE, Predicate::MOD_TYPE)] = 0.8f; 		

	// Marjorie says not to penalize NONE_TYPE matches for names and descriptors
	// If Serif finds a nested structure where all levels might be considered a valid name/desc,
	// one level gets picked and the others are set to NONE.  We don't want to penalize this.
	_baseTypeConfusionProbs[make_pair(Predicate::NAME_TYPE, Predicate::NONE_TYPE)] = 1.0f;
	_baseTypeConfusionProbs[make_pair(Predicate::DESC_TYPE, Predicate::NONE_TYPE)] = 1.0f;
}
