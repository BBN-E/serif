
#include "Generic/common/leak_detection.h"

#include <string>
#include <vector>
#include <set>

#include "Generic/common/Symbol.h"
#include "DistributionalKnowledgeTable.h"
#include "DistributionalAssocPropClass.h"
#include "DistributionalUtil.h"

/*
  10/15/2013 : Yee Seng Chan
  The *Fea variables are temporarily there for an alternative way of representing scores, as a set of inequalities,
  instead of as bins as currently represented by the *SB variables.
*/

// we can't set the assoc prop scores yet. We need to wait until EventAAObservation has gathered the propositions for (anchor, arg)
DistributionalAssocPropClass::DistributionalAssocPropClass(const Symbol& anchor, const Symbol& arg) : _anchor(anchor), _arg(arg) {
	_assocPropSim = -1;
	_assocPropSimSB = 0;
	_assocPropPmi = -1;
	_assocPropPmiSB = 0;
}

// this has close similarities to the way causality scores are assigned
// based on the propositions gathered by EventAAObservation for (anchor, arg), we can now set our assoc prop features
// populates: _assocPropSim , _assocPropSimFea , _assocPropSimSB , _assocPropPmi , _assocPropPmiFea , _assocPropPmiSB
//
// to assign assoc prop scores features, we do the following:
// - take the candidate argument, and gather the set of its associated SERIF propositions 
//     (i.e. propositions which have the candidate arg as one of their prop arguments)
// - for each of these propositions, take note of the proposition's predicate, and the proposition-role between (predicate, candidate argument)
//     (in our current implementation, we actually don't use the proposition role, but it's nice to have if in future you want to do something special for <sub>, <obj>
// - for each predicate, measure the pmi and cosine-sim score between (predicate, anchor)
//
// we take in a set of (proposition-role, proposition-predicate) associated with the candidate argument
void DistributionalAssocPropClass::assignAssocPropFeatures(const std::set<SymbolPair>& feas) {
	float maxS=-1, maxPmi=-1, s=-1;
	//_assocPropSimFea.clear();
	//_assocPropPmiFea.clear();

        // right now, keywords actually just consist of the anchor. 
        // in future, you might want to experiment with adding .. say a list of keywords (based on the event type represented by the anchor)
	std::set<Symbol> keywords;
	keywords.insert(_anchor);

	for(std::set<SymbolPair>::const_iterator it=feas.begin(); it!=feas.end(); it++) {
		SymbolPair fea = *it;
		Symbol w = fea.second;		// grab the predicate

		for(std::set<Symbol>::iterator kwIt=keywords.begin(); kwIt!=keywords.end(); kwIt++) {
			Symbol kw = *kwIt;
		
			// meaningless to assign scores if anchor and predicate are the same word string
			if(w != kw) {
				s = DistributionalKnowledgeTable::getVVSim(w, kw);
				if(s > maxS) 
					maxS = s;
				s = DistributionalKnowledgeTable::getNNSim(w, kw);
				if(s > maxS) 
					maxS = s;
				s = DistributionalKnowledgeTable::getVNSim(w, kw);
				if(s > maxS) 
					maxS = s;
				s = DistributionalKnowledgeTable::getVNSim(kw, w);
				if(s > maxS) 
					maxS = s;

				s = DistributionalKnowledgeTable::getVVPmi(w, kw);
				if(s > maxPmi) 
					maxPmi = s;
				s = DistributionalKnowledgeTable::getNNPmi(w, kw);
				if(s > maxPmi) 
					maxPmi = s;
				s = DistributionalKnowledgeTable::getVNPmi(w, kw);
				if(s > maxPmi) 
					maxPmi = s;
				s = DistributionalKnowledgeTable::getVNPmi(kw, w);
				if(s > maxPmi) 
					maxPmi = s;
				}
			}
	}
	_assocPropSim = maxS;
	_assocPropPmi = maxPmi;

	_assocPropSimSB = DistributionalUtil::scoreToBin(_assocPropSim);
        //std::vector<std::wstring> bins = DistributionalUtil::scoreToBins(maxS);
	//for(unsigned i=0; i<bins.size(); i++) {
	//	_assocPropSimFea.push_back( Symbol(L"assocpropsim_"+bins[i]) );
	//}

	_assocPropPmiSB = DistributionalUtil::scoreToBin(_assocPropPmi);
        //bins = DistributionalUtil::scoreToBins(maxPmi);
	//for(unsigned i=0; i<bins.size(); i++) {
	//	_assocPropPmiFea.push_back( Symbol(L"assocproppmi_"+bins[i]) );
	//}
}

