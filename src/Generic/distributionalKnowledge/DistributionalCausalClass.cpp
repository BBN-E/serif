
#include "Generic/common/leak_detection.h"

#include <string>
#include <vector>
#include <set>

#include "Generic/common/Symbol.h"

#include "DistributionalKnowledgeTable.h"
#include "DistributionalUtil.h"
#include "DistributionalCausalClass.h"

/*
  10/15/2013 : Yee Seng Chan
  The *Fea variables are temporarily there for an alternative way of representing scores, as a set of inequalities,
  instead of as bins as currently represented by the *SB variables.
*/

// we can't set the causality scores yet. We need to wait until EventAAObservation has gathered the propositions for (anchor, arg)
DistributionalCausalClass::DistributionalCausalClass(const Symbol& anchor, const Symbol& arg) : _anchor(anchor), _arg(arg) {
	_causalScore = -1;
	_causalScoreSB = 0;
}

// based on the propositions gathered by EventAAObservation for (anchor, arg), we can now set our causality features
// populates: _causalScore , _causalScoreFea , _causalScoreSB
//
// to assign causality scores features, we do the following:
// - take the candidate argument, and gather the set of its associated SERIF propositions 
//     (i.e. propositions which have the candidate arg as one of their prop arguments)
// - for each of these propositions, take note of the proposition's predicate, and the proposition-role between (predicate, candidate argument)
//     (in our current implementation, we actually don't use the proposition role, but it's nice to have if in future you want to do something special for <sub>, <obj>
// - for each predicate, measure the causality score between (predicate, anchor)
// - take the max score and assign to _causalScore
void DistributionalCausalClass::assignCausalFeatures(const std::set<SymbolPair>& feas) {
	float maxS=-1, s=-1;
	//_causalScoreFea.clear();

	// right now, keywords actually just consist of the anchor. 
	// in future, you might want to experiment with adding .. say a list of keywords (based on the event type represented by the anchor)
	std::set<Symbol> keywords;
	keywords.insert(_anchor);

	// 'feas' is a set of (proposition-role, predicate) as described above
	for(std::set<SymbolPair>::const_iterator it=feas.begin(); it!=feas.end(); it++) {
		SymbolPair fea = *it;
		Symbol w = fea.second;		// grab the predicate

		for(std::set<Symbol>::iterator kwIt=keywords.begin(); kwIt!=keywords.end(); kwIt++) {
			Symbol kw = *kwIt;
			// if the predicate == anchor, then it is pointless to measure causality, since they are the same word
			if(w != kw) {
				s = DistributionalKnowledgeTable::getCausalScore(w, kw);
				if(s > maxS) 
					maxS = s;
			}
		}
	}
	_causalScore = maxS;

	// remap score as bin
	_causalScoreSB = DistributionalUtil::scoreToBin(maxS);

	//std::vector<std::wstring> bins = DistributionalUtil::scoreToBins(maxS);
	//for(unsigned i=0; i<bins.size(); i++) {		
	//	_causalScoreFea.push_back( Symbol(L"causalscore_"+bins[i]) );
	//}
}

