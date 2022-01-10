// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/expanders/WordnetExpander.h"
#include "Generic/wordnet/xx_WordNet.h"
#include <boost/foreach.hpp>
#include "common/ParamReader.h"
#include "common/SymbolUtilities.h"
#include "common/InputUtil.h"


void WordnetExpander::expand( const PropNodes & pnodes) const {
	using namespace boost;

	BOOST_FOREACH(PropNode_ptr nodep, pnodes) {
		PropNode & pnode( *nodep );
		WordNet * wn = WordNet::getInstance();
		PropNode::WeightedPredicates toAdd;

		BOOST_FOREACH(PropNode::WeightedPredicate wpred, pnode.getPredicates()) {

			// We don't do expansion for names
			int pos;				
			if (wpred.first.type() == Predicate::VERB_TYPE)
				pos = VERB;
			else if (wpred.first.type() == Predicate::DESC_TYPE)
				pos = NOUN;
			else if (wpred.first.type() == Predicate::MOD_TYPE)
				pos = ADJ;
			else continue;

			// Optional; externally specified list (block_wordnet_expansion_list: foo)
			if (!allowExpansion(wpred.first.pred()))
				continue;

			int pred_sense_count = wn->getSenseCount(wpred.first.pred(), pos);
			if (pred_sense_count > _sense_count_maximum) {
				continue;
			}

			for (int i = 0; i < 6; i++) {
				WordNet::CollectedSynonyms candidates;
				int type;
				float type_prob;

				if (i == 0) {
					type = SYNS;
					type_prob = WN_SYNONYM_PROB;
				} else if (i == 1) {
					type = HYPERPTR;
					type_prob = WN_HYPERNYM_PROB;
				} else if (i == 2) {
					type = HYPOPTR;
					type_prob = WN_HYPONYM_PROB;
				} else if (i == 3) {
					// only do meronyms for descs
					if (wpred.first.type() != Predicate::DESC_TYPE)
						continue;
					type = HASPARTPTR;
					type_prob = WN_MERONYM_PROB;
				} else if (i == 4) {
					// only do similar for adjs
					if (wpred.first.type() != Predicate::MOD_TYPE)
						continue;
					type = SIMPTR;
					type_prob = WN_SIMILAR_PROB;
				} else if (i == 5) {
					// only do antonyms for adjs
					if (wpred.first.type() != Predicate::MOD_TYPE)
						continue;
					type = ANTPTR;
					type_prob = WN_ANTONYM_PROB;
				} else continue;

				if (type_prob == 0)
					continue;

				wn->getSynonyms( wpred.first.pred(), candidates, pos, type, 1 );

				// add all one-word all-lowercase wordnet entries (uppercase are usually proper names)
				for( WordNet::CollectedSynonyms::iterator wni = candidates.begin(); wni != candidates.end(); ++wni ){
					std::wstring synonym = wni->first.to_string();
					if (synonym.size() != 0 && iswupper(synonym.at(0))) {
						// special cases
						if (synonym != L"Jan" && synonym != L"Feb" &&
							synonym != L"Mar" && synonym != L"Apr" &&
							synonym != L"Jun" && synonym != L"Jul" &&
							synonym != L"Aug" && synonym != L"Sep" && synonym != L"Sept" &&
							synonym != L"Oct" && synonym != L"Nov" &&
							synonym != L"Dec" && synonym != L"PM")
							continue;
					}
					const wchar_t * c_syn_b = wni->first.to_string();
					const wchar_t * c_syn_e = c_syn_b + wcslen( c_syn_b );
					if( std::find(c_syn_b, c_syn_e, L' ') != c_syn_e ) continue;					

					/*if (wpred.first.type() == Predicate::VERB_TYPE) {
						std::wcout << "VERB ";
					} else if ( wpred.first.type() == Predicate::DESC_TYPE ){
						std::wcout << "NOUN ";
					} else if ( wpred.first.type() == Predicate::MOD_TYPE ){
						std::wcout << "MOD ";
					}
					std::wcout << L"synonym (" << i << "): " << wpred.first.pred() << L" ~ " << synonym << L" " << wn->getSenseCount(synonym, pos);
					std::wcout << L"\n";*/
					
					int sense_count = wn->getSenseCount(synonym, pos);
					if (sense_count > _sense_count_maximum) {
						continue;
					}
					
					// Always take the max score.  
					Predicate sp(wpred.first.type(), wni->first, wpred.first.negative());

					float & v = toAdd[ sp ];
					v = std::max( v, type_prob * wpred.second );
				}

			}
		}

		BOOST_FOREACH(PropNode::WeightedPredicate addPred, toAdd) {
			pnode.addPredicate(addPred.first, addPred.second);
		}

	}

	return;
}

bool WordnetExpander::allowExpansion( Symbol p ){
	
	static bool init = false;
	static std::set<Symbol> blockedSymbols;
	if (!init) {
		std::string input_file = ParamReader::getParam("block_wordnet_expansion_list");
		if (!input_file.empty()) {
			blockedSymbols = InputUtil::readFileIntoSymbolSet(input_file, false, true);
		}
		init = true;
	}

	return (blockedSymbols.size() == 0 || blockedSymbols.find(SymbolUtilities::lowercaseSymbol(p)) == blockedSymbols.end());
}
