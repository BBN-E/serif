// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/expanders/PredicateStemmer.h"

#include "Generic/PropTree/Predicate.h"
#include "Generic/PropTree/PropNode.h"
#include "Generic/PropTree/expanders/PropTreeExpander.h"
#include "Generic/porterStemmer/Porter.h"
#include "Generic/wordnet/xx_WordNet.h"
#include "../Predicate.h"
#include <boost/foreach.hpp>

void PredicateStemmer::expand( const PropNodes & pnodes) const 
{
	using namespace boost;

	const static Symbol us_sym(L"us");

	BOOST_FOREACH(PropNode_ptr nodep, pnodes) {
		PropNode & pnode( *nodep );
		PropNode::WeightedPredicates orig_preds(pnode.getPredicates().begin(), pnode.getPredicates().end());
		WordNet * wn = WordNet::getInstance();

		BOOST_FOREACH(PropNode::WeightedPredicate wpred, orig_preds) {
			bool is_noun_type=wpred.first.type() == Predicate::DESC_TYPE 
				|| wpred.first.type() == Predicate::PART_TYPE;
			bool is_verb_type=wpred.first.type() == Predicate::VERB_TYPE;

			// WN-stem descriptors and verbs
			if( !_skip_nouns && (WN_STEM_PROB && is_noun_type)) {		
				pnode.addPredicate(Predicate( wpred.first.type(), 
						wn->stem_noun(wpred.first.pred()), wpred.first.negative() ), WN_STEM_PROB*wpred.second);

				// Stem gerunds as verbs, too
				if ( !_skip_verbs) {
					std::wstring pred_str = wpred.first.pred().to_string();
					size_t index = pred_str.find(L"ing");
					if (pred_str.size() > 5 && index == pred_str.size() - 3) {
						Symbol stem = wn->stem_verb(wpred.first.pred());
						if (stem != wpred.first.pred()) {
							pnode.addPredicate(Predicate( Predicate::VERB_TYPE, 
								stem, wpred.first.negative() ), WN_STEM_PROB*wpred.second);		
						}
					}
				}
			}

			if( !_skip_verbs && (WN_STEM_PROB && is_verb_type)){
				pnode.addPredicate(Predicate( wpred.first.type(), 
						wn->stem_verb(wpred.first.pred()), wpred.first.negative() ), WN_STEM_PROB*wpred.second);			}

			// Porter-stem anything but names & pronouns
			if( PORTER_STEM_PROB && wpred.first.type() != Predicate::NAME_TYPE &&
				wpred.first.type() != Predicate::PRON_TYPE && 				
				!(_skip_nouns && is_noun_type) && !(_skip_verbs && is_verb_type))
			{				
				Symbol stem = Porter::stem(wpred.first.pred());
				// Special case: "use" gets stemmed to "us" which makes us match against the country US
				if (stem != us_sym)
					pnode.addPredicate(Predicate( wpred.first.type(), 
						Porter::stem(wpred.first.pred()), wpred.first.negative() ), PORTER_STEM_PROB * wpred.second );
			}
		}
	}

	return;
}

