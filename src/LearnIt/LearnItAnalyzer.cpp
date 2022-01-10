/* If someone wishes to use this program, they need to upgrade it for n-ary LearnIt ~ RMG*/

//#include "Generic/common/leak_detection.h"
//#include "Generic/common/SessionLogger.h"
//#include <fstream>
//#include <iostream>
//#include <map>
//#include <string>
//#include <vector>
//#include <stdexcept>
//#include "Generic/common/ParamReader.h"
//#include "Generic/theories/DocTheory.h"
//#include "distill-generic/features/SnippetFeatureSet.h"
//#include "distill-generic/patterns/DocumentInfo.h"
//#include "distill-generic/patterns/MentionPattern.h"
//#include "distill-generic/patterns/ValueMentionPattern.h"
//#include "distill-generic/common/DistillUtilities.h"
//#include "LearnIt/LearnItAnalyzer.h"
//#include "LearnIt/db/LearnItDB.h" 
//#include "LearnIt/MainUtilities.h"
//#include "LearnIt/Pattern.h"
//#include "LearnIt/Seed.h"
//#include "LearnIt/Target.h"
//#include "LearnIt/SlotFiller.h"
//#include "LearnIt/MatchInfo.h"
//#include "boost/shared_ptr.hpp"
//#include "boost/foreach.hpp"
//#include "boost/algorithm/string/find.hpp"
//#include "boost/make_shared.hpp"
//
//LearnItAnalyzer::LearnItAnalyzer(std::vector<Pattern_ptr> patterns){
//
//	BOOST_FOREACH(Pattern_ptr p, patterns){
//		_pattern_match_counts.push_back(std::pair<int,int>(0, 0));
//	}
//	for(int i =0; i< N_COUNT_TYPES; i++){
//		//_matched_counts.push_back(0);
//		//_unmatched_counts.push_back(0);
//		_all_counts.push_back(0);
//	}		
//
//}
//
///* Generic functions that look for possible connections between two mentions */
//std::wstring LearnItAnalyzer::getPropPrefix(const Proposition *prop) 
//{	
//	std::wstring return_value = L"";
//	if (prop->getPredType() == Proposition::NOUN_PRED)
//		return_value += L"(nprop ";
//	else if (prop->getPredType() == Proposition::COPULA_PRED || prop->getPredType() == Proposition::VERB_PRED)
//		return_value += L"(vprop ";
//	else if (prop->getPredType() == Proposition::MODIFIER_PRED)
//		return_value += L"(mprop ";
//	else return L"";
//
//	if (prop->getPredHead() != 0) {
//		return_value += L"(predicate ";
//		return_value += prop->getPredHead()->getHeadWord().to_string();
//		return_value += L")";
//	}
//
//	return return_value;
//}
//
//
//std::wstring LearnItAnalyzer::findConnection(PropositionSet *ps, MentionSet *ms, const Proposition *prop, 
//							const Mention *m, Symbol label, int indent, int& depth, bool pretty_print) 
//{	
//	if (depth > 100)
//		return L"";
//
//	int working_depth = depth + 1;
//	int best_depth = 10000;
//	std::wstring best_arg = L"";
//
//	// we assume no double-nested sets, and we don't process them at the top-level
//	if (prop->getPredType() == Proposition::SET_PRED) {
//		return L"";
//	}
//
//	for (int a = 0; a < prop->getNArgs(); a++) {
//		Argument *arg = prop->getArg(a);
//		
//		const Proposition *propToProcess = 0;
//		if (arg->getType() == Argument::MENTION_ARG) {
//			const Mention *argMent = arg->getMention(ms);
//			if (argMent == m) {
//				best_depth = working_depth;
//				best_arg = L"(argument (role ";
//				best_arg += arg->getRoleSym().to_string();
//				best_arg += L") ";
//				best_arg += label.to_string();
//				best_arg += L")";
//			} else {
//				propToProcess = ps->getDefinition(arg->getMentionIndex());
//			}
//		} else if (arg->getType() == Argument::PROPOSITION_ARG) {
//			propToProcess = arg->getProposition();
//		} 
//		if (propToProcess == 0 || propToProcess == prop)
//			continue;
//		std::vector<const Proposition *> propsToProcess;
//		if (propToProcess->getPredType() == Proposition::SET_PRED) {			
//			for (int a_set = 1; a_set < propToProcess->getNArgs(); a_set++) {
//				Argument *setArg = propToProcess->getArg(a_set);
//				if (setArg->getType() == Argument::MENTION_ARG) {
//					const Mention *setArgMent = setArg->getMention(ms);
//					if (setArgMent == m) {
//						best_depth = working_depth;
//						best_arg = L"(argument (role ";
//						best_arg += arg->getRoleSym().to_string();
//						best_arg += L") ";
//						best_arg += label.to_string();
//						best_arg += L")";
//					} else {
//						const Proposition* memberProp = ps->getDefinition(setArg->getMentionIndex());
//						propsToProcess.push_back(memberProp);
//					}						
//				}
//			}
//		} else {
//			propsToProcess.push_back(propToProcess);
//		}
//		if (best_depth > working_depth) { // you won't beat that...
//			BOOST_FOREACH(const Proposition *child_prop, propsToProcess) {
//				if (child_prop == prop || child_prop == 0)
//					continue;
//				int temp_depth = working_depth;
//				std::wstring arg_result = findConnection(ps, ms, child_prop, m, label, indent+23, temp_depth, pretty_print);
//				if (arg_result.length() > 0 && temp_depth < best_depth) {
//					best_depth = temp_depth;
//					best_arg = L"(argument (role ";
//					best_arg += arg->getRoleSym().to_string();
//					best_arg += L") ";
//					if (pretty_print) {
//						best_arg += L"\n";
//						for (int in = 0; in < indent + 10; in++)
//							best_arg += L" ";
//					}
//					best_arg += getPropPrefix(child_prop);
//					if (pretty_print) {
//						best_arg += L"\n";
//						for (int in = 0; in < indent + 10 + 7; in++)
//							best_arg += L" ";
//					}
//					best_arg += L"(args ";
//					best_arg += arg_result + L")))";
//				}
//			}
//		}
//	}
//	if (best_depth == 10000)
//		return L"";
//	else {
//		depth = best_depth;
//		return best_arg;
//	}
//}
//
//
//
//int LearnItAnalyzer::getTokenConnection(SentenceTheory *st, const Mention *m1, Symbol label1, const Mention *m2, Symbol label2, std::wstringstream& tokenStream) {
//	const SynNode *n1 = m1->getNode();
//	const SynNode *n2 = m2->getNode();
//
//	int start1 = n1->getStartToken();
//	int end1 = n1->getEndToken();
//	int start2 = n2->getStartToken();
//	int end2 = n2->getEndToken();
//
//	int start_token, end_token;
//	Symbol startLabel, endLabel;
//	if (end1 < start2) {
//		startLabel = label1;
//		endLabel = label2;
//		start_token = end1 + 1;
//		end_token = start2 - 1;
//	} else if (end2 < start1) {
//		startLabel = label2;
//		endLabel = label1;
//		start_token = end2 + 1;
//		end_token = start1 - 1;
//	} else {
//		return false;
//	}
//	int n_tokens = 0;
//	tokenStream << startLabel.to_string() << L" ";
//	TokenSequence *ts = st->getTokenSequence();
//	for (int i = start_token; i <= end_token; i++) {
//		n_tokens++;
//		tokenStream << ts->getToken(i)->getSymbol().to_string() << L" ";
//	}
//	tokenStream << endLabel.to_string() << L"\n";
//	return n_tokens;
//}
//
//
//std::wstring LearnItAnalyzer::getBestPropConnection(SentenceTheory *st, const Mention *m1, Symbol label1, const Mention *m2, Symbol label2, bool pretty_print, int& depth) {
//	PropositionSet *ps = st->getPropositionSet();
//	ps->fillDefinitionsArray();
//	MentionSet *ms = st->getMentionSet();
//	
//	int best_depth = 10000;
//	std::wstring best_connection = L"";
//
//	for (int p = 0; p < ps->getNPropositions(); p++) {
//		Proposition *prop = ps->getProposition(p);
//		int depth1 = 0;
//		int start_indent = 13;
//		std::wstring ment1_connection = findConnection(ps, ms, prop, m1, label1, start_indent, depth1, pretty_print);
//		if (ment1_connection.length() == 0)
//			continue;		
//		int depth2 = 0;
//		std::wstring ment2_connection = findConnection(ps, ms, prop, m2, label2, start_indent, depth2, pretty_print);
//		if (ment2_connection.length() == 0)
//			continue;
//		int depth = depth1 + depth2;
//		if (depth < best_depth) {
//			best_depth = depth;
//			if (pretty_print) { 
//				best_connection = getPropPrefix(prop) + L"\n";
//				best_connection += L"       (args " + ment1_connection + L"\n";
//				best_connection += L"             " + ment2_connection + L"\n";
//				best_connection += L"        )\n";
//				best_connection += L")\n";
//			} else {				
//				best_connection = getPropPrefix(prop) + L" ";
//				best_connection += L"(args " + ment1_connection + L" ";
//				best_connection += ment2_connection + L"))";
//			}
//		}	
//	}	
//	depth = best_depth;
//	return best_connection;
//}
//
//
//
//
///* Functions that check recall examples */
//MatchInfo::PatternMatches LearnItAnalyzer::getMatchesForPatternAndSlots(DocumentInfo *doc_info, SentenceTheory* sent_theory, Pattern_ptr pattern,  
//										   std::vector<SlotFiller_ptr> slotx_fillers,
//										   std::vector<SlotFiller_ptr> sloty_fillers)
//{
//	MatchInfo::PatternMatches good_pattern_matches;	//only those matches for which X and Y match a slot filler
//	MatchInfo::PatternMatches matches = MatchInfo::findPatternInSentence(doc_info, sent_theory, pattern);
//
//	BOOST_FOREACH(MatchInfo::PatternMatch match, matches){
//		bool found_match_for_pattern = false;
//		BOOST_FOREACH(SlotFiller_ptr slotx_filler, slotx_fillers) {
//			if(match.slots[0]->getStartToken() == slotx_filler->getStartToken() &&
//				match.slots[0]->getEndToken() == slotx_filler->getEndToken()){
//				BOOST_FOREACH(SlotFiller_ptr sloty_filler, sloty_fillers) { 
//					if(match.slots[1]->getStartToken() == sloty_filler->getStartToken() &&
//						match.slots[1]->getEndToken() == sloty_filler->getEndToken()){
//						found_match_for_pattern = true;
//						break;
//					}
//				}
//			}
//			if(found_match_for_pattern)
//				break;
//		}
//		if(found_match_for_pattern)
//			good_pattern_matches.push_back(match);
//	}
//	return good_pattern_matches;
//}
//
//
//
//int LearnItAnalyzer::generatePatternMatchDebugString(DocumentInfo *doc_info, SentenceTheory* sent_theory,
//									std::vector<SlotFiller_ptr> slotx_fillers, std::vector<SlotFiller_ptr> sloty_fillers, 
//									std::vector<Pattern_ptr> patterns, bool active, bool correct,
//									std::wstringstream& string_stream)
//{
//	int n_matched = 0;
//	int pattern_num = 0;
//	
//	TokenSequence *tok_seq = sent_theory->getTokenSequence();
//	BOOST_FOREACH(Pattern_ptr pattern, patterns){
//		if(pattern->active() == active){
//			MatchInfo::PatternMatches pattern_matches = getMatchesForPatternAndSlots(doc_info, sent_theory, pattern, slotx_fillers, sloty_fillers);
//
//			if(pattern_matches.size() != 0){	
//				string_stream << L"Pattern "<< pattern_num <<L" (NMatches= "<<pattern_matches.size()<<L")\n   "
//					<<pattern->getPatternString()<<L"\n";
//				BOOST_FOREACH(MatchInfo::PatternMatch pm, pattern_matches){
//					string_stream <<"   Matched: "<<DistillUtilities::getTextString(tok_seq, pm.start_token, pm.end_token)<<"   |||"
//						<<"    SLOT_X: "<<DistillUtilities::getTextString(tok_seq, pm.slots[0]->getStartToken(), pm.slots[0]->getEndToken())
//						<<"    SLOT_Y: "<<DistillUtilities::getTextString(tok_seq, pm.slots[1]->getStartToken(), pm.slots[1]->getEndToken())<<"\n";
//				}
//				n_matched++;
//				//std::pair<int, int> counts = _pattern_match_counts[pattern_num];
//				if(correct){
//					_pattern_match_counts[pattern_num].first++;
//					//_pattern_match_counts[pattern_num] = std::pair(counts.first+1, counts.second);
//				}
//				else{
//					_pattern_match_counts[pattern_num].second++;
//					//_pattern_match_counts[pattern_num] = std::pair(counts.first, counts.second+1);
//				}
//
//			}
//
//		}				
//		pattern_num++;	
//	}
//	return n_matched;
//}
//std::vector<int> LearnItAnalyzer::getMentionsForSlotFillers(SentenceTheory* sent_theory, std::vector<SlotFiller_ptr> slot_fillers){
//	const MentionSet* ments = sent_theory->getMentionSet();
//
//	std::vector<int> mentionid_vector;
//	for(size_t i = 0; i < slot_fillers.size(); i++){
//		SlotFiller_ptr slot_filler =  slot_fillers[i];
//		std::wstring slotfiller_text  = slot_filler->getText();
//	}
//	BOOST_FOREACH(SlotFiller_ptr slot_filler, slot_fillers){
//		std::wstring slotfiller_text  = slot_filler->getText();
//		if(slot_filler->getType() == SlotConstraints::MENTION){
//			for(int i = 0; i < ments->getNMentions(); i++){
//				if(ments->getMention(i)->getNode()->getStartToken() == slot_filler->getStartToken() &&
//					ments->getMention(i)->getNode()->getEndToken() == slot_filler->getEndToken() ){
//						mentionid_vector.push_back(i);
//						break;
//						
//				}
//			}			
//		}
//		else if(slot_filler->getType() == SlotConstraints::VALUE){
//			//try to match an exact mention extent first
//			for(int i = 0; i < ments->getNMentions(); i++){
//				if(ments->getMention(i)->getNode()->getStartToken() == slot_filler->getStartToken() &&
//					ments->getMention(i)->getNode()->getEndToken() == slot_filler->getEndToken() ){
//						mentionid_vector.push_back(i);
//						break;
//						
//				}
//			}
//			//could use prop temp args to do more extensive matching			
//		}
//	}
//	return mentionid_vector;
//}
//
//bool LearnItAnalyzer::printAnalysisForRelationSeedsAndSentence(DocumentInfo *doc_info, Seed_ptr seed, int sent_index, bool correct,
//													    std::vector<Pattern_ptr> patterns, Target_ptr target, std::vector<SlotFiller_ptr> slotx_fillers,
//													   std::vector<SlotFiller_ptr> sloty_fillers, UTF8OutputStream &miss_out, UTF8OutputStream &fa_out, UTF8OutputStream& correct_out)
//{
//	int max_depth = 15;
//	int best_depth = 15;
//
//	SentenceTheory *sent_theory = doc_info->getDocTheory()->getSentenceTheory(sent_index);
//	const TokenSequence *tok_seq = sent_theory->getTokenSequence();
//	const MentionSet *ments = sent_theory->getMentionSet();
//
//	//Get Slot Fillers and align them to mentions
//	std::vector<int> sloty_mentionids = getMentionsForSlotFillers(sent_theory, sloty_fillers);
//	std::vector<int> slotx_mentionids = getMentionsForSlotFillers(sent_theory, slotx_fillers);
//
//	
//	std::wstringstream matched_active_pattern_stream;
//	std::wstringstream matched_inactive_pattern_stream;
//	std::wstringstream prop_connection_stream;		
//	std::wstringstream token_connection_stream;	
//
//	int n_ok_prop_connections = 0;
//	int min_tokens_between = 1000;
//
//	
//	int n_active_pattern_matches =	generatePatternMatchDebugString(doc_info, sent_theory, slotx_fillers, sloty_fillers, patterns, true, correct, matched_active_pattern_stream);
//	int n_inactive_pattern_matches = generatePatternMatchDebugString(doc_info, sent_theory, slotx_fillers, sloty_fillers, patterns, false, correct, matched_inactive_pattern_stream);
//
//
//	BOOST_FOREACH(int mentx_id, slotx_mentionids){
//		BOOST_FOREACH(int menty_id, sloty_mentionids){
//			int depth = 0;
//			//Get Prop Connections -- this is done in a slighlty different manner than what MatchInfo::Pr
//			std::wstring prop_connection =  getBestPropConnection(sent_theory, ments->getMention(mentx_id), Symbol(L"SLOT_X"), ments->getMention(menty_id), Symbol(L"SLOT_Y"), true, depth);
//			if(depth < max_depth){
//				prop_connection_stream << "PropConnection: SLOT_X: "<<ments->getMention(mentx_id)->getNode()->toTextString()
//					<<"  SLOT_Y: "<<ments->getMention(menty_id)->getNode()->toTextString()<<"   DEPTH: "<<depth<<"\n"
//					<<prop_connection<<"\n--------------------\n";
//				n_ok_prop_connections++;
//				if(depth < best_depth){
//					best_depth = depth;
//				}
//			}
//
//
//			token_connection_stream <<  "TokenConnection: Slot_X: "<<ments->getMention(mentx_id)->getNode()->toTextString()
//				<<"  SLOT_Y: "<<ments->getMention(menty_id)->getNode()->toTextString()<<"\n   ";
//			int n_tokens = getTokenConnection(sent_theory, ments->getMention(mentx_id), Symbol(L"SLOT_X"), ments->getMention(menty_id), Symbol(L"SLOT_Y"), token_connection_stream);				
//			token_connection_stream<<"   DISTANCE: "<<n_tokens<<"\n";
//
//			if(n_tokens < min_tokens_between){
//				min_tokens_between = n_tokens;
//			}
//		}
//	}
//	//now, print information
//	bool matched_sentence = false;
//	if(correct)
//		_all_counts[N_VALID]++;
//	else
//		_all_counts[N_INVALID]++;
//	if(correct && n_active_pattern_matches > 0){ //correct answer
//		updateMatchCounts(n_active_pattern_matches, n_inactive_pattern_matches,  best_depth, min_tokens_between, 
//			_all_counts[CORR_ACTIVE_NPAT], _all_counts[CORR_INACTIVE_NPAT],  _all_counts[N_CORR_TOTAL], _all_counts[N_CORR_MATCH_INACTIVE],
//			_all_counts[BEST_DEPTH_CORR], _all_counts[N_PROP_CORR], _all_counts[MIN_DIST_CORR]
//		);
//
//
//		correct_out<<"**************** LearnIt Correctly Found:" <<doc_info->getDocID()<<" Sent: "<< sent_index<<": ******************\n";
//		correct_out<<DistillUtilities::getTextString(sent_theory->getTokenSequence())<<"\n";
//		correct_out<<"SLOT_X Matches:  "<<slotx_fillers.size()<<"(ments: "<<slotx_mentionids.size()
//			<<")\n";
//		BOOST_FOREACH(int m, slotx_mentionids){
//			correct_out<<"   "<<ments->getMention(m)->getNode()->toTextString()<<"\n";
//		}
//		correct_out<<"SLOT_Y Matches: "<<sloty_fillers.size()<<"(ments: "<<sloty_mentionids.size()<<")\n";
//		BOOST_FOREACH(int m, sloty_mentionids){
//			correct_out<<"   "<<ments->getMention(m)->getNode()->toTextString()<<"\n";
//		}
//		correct_out<<"# Active Patterns Matched: "<<n_active_pattern_matches<<"\n";
//		correct_out<<"# Prop connections depth< "<<max_depth<<":  "<<n_ok_prop_connections<<"\n";
//		correct_out<<"Minimum token distance: "<<min_tokens_between<<"\n";
//		correct_out<<"=========== Active Patterns =========\n"<<matched_active_pattern_stream.str().c_str()<<"\n";
//		
//	} 
//	else if (correct && n_active_pattern_matches == 0){ //miss
//		updateMatchCounts(n_active_pattern_matches, n_inactive_pattern_matches,  best_depth, min_tokens_between, 
//			_all_counts[MISS_ACTIVE_NPAT], _all_counts[MISS_INACTIVE_NPAT],  _all_counts[N_MISS_TOTAL], _all_counts[N_MISS_MATCH_INACTIVE],
//			_all_counts[BEST_DEPTH_MISS], _all_counts[N_PROP_MISS], _all_counts[MIN_DIST_MISS]
//		);
//
//
//		//Print to File
//		miss_out<<"**************** Recall Miss:" <<doc_info->getDocID()<<" Sent: "<< sent_index<<": ******************\n";
//		miss_out<<DistillUtilities::getTextString(sent_theory->getTokenSequence())<<"\n";
//		miss_out << "SLOT_X: "<<seed->getSlot(0) <<"      SLOT_Y: "<<seed->getSlot(1)<<"\n";
//		miss_out<<"SLOT_X Matches:  "<<slotx_fillers.size()<<"(ments: "<<slotx_mentionids.size()
//			<<")\n";
//
//		BOOST_FOREACH(int m, slotx_mentionids){
//			miss_out<<"   "<<ments->getMention(m)->getNode()->toTextString()<<"\n";
//		}
//		miss_out<<"SLOT_Y Matches: "<<sloty_fillers.size()<<"(ments: "<<sloty_mentionids.size()<<")\n";
//		BOOST_FOREACH(int m, sloty_mentionids){
//			miss_out<<"   "<<ments->getMention(m)->getNode()->toTextString()<<"\n";
//		}
//		miss_out<<"# Inactive Patterns Matched: "<<n_inactive_pattern_matches<<"\n";
//		miss_out<<"# prop connections depth< "<<max_depth<<":  "<<n_ok_prop_connections<<"\n";
//		miss_out<<"Minimum token distance: "<<min_tokens_between<<"\n";
//		if(n_inactive_pattern_matches > 0){
//			miss_out<<"=========== Inactive Patterns =========\n"<<matched_inactive_pattern_stream.str().c_str()<<"\n";
//		}
//		if(prop_connection_stream.str().length() > 0){
//			miss_out<<"=========== Prop Connections =========\n"<<prop_connection_stream.str().c_str()<<"\n";
//		}
//		if(token_connection_stream.str().length() > 0){
//			miss_out<<"=========== Token Connections =========\n"<<token_connection_stream.str().c_str()<<"\n";
//		}
//
//	} 
//	else if (!correct && n_active_pattern_matches > 0){ //false alarm
//
//		updateMatchCounts(n_active_pattern_matches, n_inactive_pattern_matches,  best_depth, min_tokens_between,
//			_all_counts[FA_ACTIVE_NPAT], _all_counts[FA_INACTIVE_NPAT],  _all_counts[N_FA_TOTAL], _all_counts[N_FA_MATCH_INACTIVE],
//			_all_counts[BEST_DEPTH_FA], _all_counts[N_PROP_FA], _all_counts[MIN_DIST_FA]);
//		//Print to File
//		fa_out<<"**************** False Alarm: " <<doc_info->getDocID()<<" Sent: "<< sent_index<<": ******************\n";
//		fa_out<<DistillUtilities::getTextString(sent_theory->getTokenSequence())<<"\n";
//		fa_out<<"SLOT_X Matches:  "<<slotx_fillers.size()<<"(ments: "<<slotx_mentionids.size()
//			<<")\n";
//		BOOST_FOREACH(int m, slotx_mentionids){
//			fa_out<<"   "<<ments->getMention(m)->getNode()->toTextString()<<"\n";
//		}
//		fa_out<<"SLOT_Y Matches: "<<sloty_fillers.size()<<"(ments: "<<sloty_mentionids.size()<<")\n";
//		BOOST_FOREACH(int m, sloty_mentionids){
//			fa_out<<"   "<<ments->getMention(m)->getNode()->toTextString()<<"\n";
//		}
//		fa_out<<"# Inactive Patterns Matched: "<<n_inactive_pattern_matches<<"\n";
//		fa_out<<"# prop connections depth< "<<max_depth<<":  "<<n_ok_prop_connections<<"\n";
//		fa_out<<"Minimum token distance: "<<min_tokens_between<<"\n";
//		if(n_inactive_pattern_matches > 0){
//			fa_out<<"=========== Inactive Patterns =========\n"<<matched_inactive_pattern_stream.str().c_str()<<"\n";
//		}
//
//	} 
//	else if (!correct && n_active_pattern_matches == 0){ //correct rejection
//		SessionLogger::info("LEARNIT")<<"Correct Ignore sentence:" <<std::endl;
//
//	}
//	else{
//		SessionLogger::info("LEARNIT") <<"Warning uncategorized: Correct(true/false)"<<correct<<"    NActive Patterns: "<<n_active_pattern_matches<<std::endl;
//	}
//
//
//
//
///*	
//	if (n_active_pattern_matches == 0){	
//		matched_sentence = false;
//		//increment statistics
//		if(n_inactive_pattern_matches > 0){
//			_unmatched_counts[N_MATCH_INACTIVE]++;
//		}
//		_unmatched_counts[N_SENT]++; 
//		_unmatched_counts[INACTIVE_MATCH] += n_inactive_pattern_matches;
//		_unmatched_counts[MIN_DIST] += min_tokens_between;
//		if(best_depth < 15){
//			_unmatched_counts[BEST_DEPTH] += best_depth;
//			_unmatched_counts[N_DEPTH]++;
//		}
//		//Print to File
//		miss_out<<"**************** Recall Miss:" <<doc_info->getDocID()<<": ******************\n";
//		miss_out<<DistillUtilities::getTextString(sent_theory->getTokenSequence())<<"\n";
//		miss_out<<"SLOT_X Matches:  "<<slotx_fillers.size()<<"(ments: "<<slotx_mentionids.size()
//			<<")\n";
//		BOOST_FOREACH(int m, slotx_mentionids){
//			miss_out<<"   "<<ments->getMention(m)->getNode()->toTextString()<<"\n";
//		}
//		miss_out<<"SLOT_Y Matches: "<<sloty_fillers.size()<<"(ments: "<<sloty_mentionids.size()<<")\n";
//		BOOST_FOREACH(int m, sloty_mentionids){
//			miss_out<<"   "<<ments->getMention(m)->getNode()->toTextString()<<"\n";
//		}
//		miss_out<<"# Inactive Patterns Matched: "<<n_inactive_pattern_matches<<"\n";
//		miss_out<<"# prop connections depth< "<<max_depth<<":  "<<n_ok_prop_connections<<"\n";
//		miss_out<<"Minimum token distance: "<<min_tokens_between<<"\n";
//		if(n_inactive_pattern_matches > 0){
//			miss_out<<"=========== Inactive Patterns =========\n"<<matched_inactive_pattern_stream.str().c_str()<<"\n";
//		}
//		if(prop_connection_stream.str().length() > 0){
//			miss_out<<"=========== Prop Connections =========\n"<<prop_connection_stream.str().c_str()<<"\n";
//		}
//		if(token_connection_stream.str().length() > 0){
//			miss_out<<"=========== Token Connections =========\n"<<token_connection_stream.str().c_str()<<"\n";
//		}
//	}	
//	else{
//		//increment statistics
//		matched_sentence = true;
//		_matched_counts[N_SENT]++; 
//		_matched_counts[ACTIVE_MATCH]+= n_active_pattern_matches;
//		if(n_inactive_pattern_matches > 0){
//			_matched_counts[N_MATCH_INACTIVE]++;
//		}
//		_matched_counts[INACTIVE_MATCH] += n_inactive_pattern_matches;
//		_matched_counts[MIN_DIST] += min_tokens_between;
//		if(best_depth < 15){
//			_matched_counts[BEST_DEPTH] += best_depth;
//			_matched_counts[N_DEPTH]++;
//		}
//
//		correct_out<<"**************** LearnIt Found:" <<doc_info->getDocID()<<": ******************\n";
//		correct_out<<DistillUtilities::getTextString(sent_theory->getTokenSequence())<<"\n";
//		correct_out<<"SLOT_X Matches:  "<<slotx_fillers.size()<<"(ments: "<<slotx_mentionids.size()
//			<<")\n";
//		BOOST_FOREACH(int m, slotx_mentionids){
//			correct_out<<"   "<<ments->getMention(m)->getNode()->toTextString()<<"\n";
//		}
//		correct_out<<"SLOT_Y Matches: "<<sloty_fillers.size()<<"(ments: "<<sloty_mentionids.size()<<")\n";
//		BOOST_FOREACH(int m, sloty_mentionids){
//			correct_out<<"   "<<ments->getMention(m)->getNode()->toTextString()<<"\n";
//		}
//		correct_out<<"# Active Patterns Matched: "<<n_active_pattern_matches<<"\n";
//		correct_out<<"# Prop connections depth< "<<max_depth<<":  "<<n_ok_prop_connections<<"\n";
//		correct_out<<"Minimum token distance: "<<min_tokens_between<<"\n";
//		correct_out<<"=========== Active Patterns =========\n"<<matched_active_pattern_stream.str().c_str()<<"\n";
//		
//
//
//		
//	}
//	*/
//
//	return matched_sentence;
//}
//void LearnItAnalyzer::updateMatchCounts(int n_active, int n_inactive, int this_depth, int this_dist, int& active_npat, 
//		int&  inactive_npat, int& count, int& inactive_count, int& depth, int& ndepth, int& dist){
//		active_npat+=n_active;
//		inactive_npat+=n_inactive;
//		count++;
//		if(n_inactive > 0){
//			inactive_count++;
//		}
//		if(this_depth < 15){
//			depth+= this_depth;
//			ndepth++;
//		}
//		dist+= this_dist;
//
//	}
//
//
//
//
//
//
//
//
//
///*
//
//bool LearnItAnalyzer::printRecallMissAnalysisForRelationSeedsAndSentence(DocumentInfo *doc_info, int sent_index, 
//													    std::vector<Pattern_ptr> patterns, Target_ptr target, std::vector<SlotFiller_ptr> slotx_fillers,
//													   std::vector<SlotFiller_ptr> sloty_fillers, UTF8OutputStream &miss_out, UTF8OutputStream& correct_out)
//{
//	int max_depth = 15;
//	int best_depth = 15;
//
//	SentenceTheory *sent_theory = doc_info->getDocTheory()->getSentenceTheory(sent_index);
//	const TokenSequence *tok_seq = sent_theory->getTokenSequence();
//	const MentionSet *ments = sent_theory->getMentionSet();
//
//	//Get Slot Fillers and align them to mentions
//	std::vector<int> sloty_mentionids = getMentionsForSlotFillers(sent_theory, sloty_fillers);
//	std::vector<int> slotx_mentionids = getMentionsForSlotFillers(sent_theory, slotx_fillers);
//
//	
//	std::wstringstream matched_active_pattern_stream;
//	std::wstringstream matched_inactive_pattern_stream;
//	std::wstringstream prop_connection_stream;		
//	std::wstringstream token_connection_stream;	
//
//	int n_ok_prop_connections = 0;
//	int min_tokens_between = 1000;
//
//
//	int n_active_pattern_matches =	generatePatternMatchDebugString(doc_info, sent_theory, slotx_fillers, sloty_fillers, patterns, true, true, matched_active_pattern_stream);
//	int n_inactive_pattern_matches = generatePatternMatchDebugString(doc_info, sent_theory, slotx_fillers, sloty_fillers, patterns, false, true, matched_inactive_pattern_stream);
//
//	BOOST_FOREACH(int mentx_id, slotx_mentionids){
//		BOOST_FOREACH(int menty_id, sloty_mentionids){
//			int depth = 0;
//			//Get Prop Connections -- this is done in a slighlty different manner than what MatchInfo::Pr
//			std::wstring prop_connection =  getBestPropConnection(sent_theory, ments->getMention(mentx_id), Symbol(L"SLOT_X"), ments->getMention(menty_id), Symbol(L"SLOT_Y"), true, depth);
//			if(depth < max_depth){
//				prop_connection_stream << "PropConnection: SLOT_X: "<<ments->getMention(mentx_id)->getNode()->toTextString()
//					<<"  SLOT_Y: "<<ments->getMention(menty_id)->getNode()->toTextString()<<"   DEPTH: "<<depth<<"\n"
//					<<prop_connection<<"\n--------------------\n";
//				n_ok_prop_connections++;
//				if(depth < best_depth){
//					best_depth = depth;
//				}
//			}
//
//
//			token_connection_stream <<  "TokenConnection: Slot_X: "<<ments->getMention(mentx_id)->getNode()->toTextString()
//				<<"  SLOT_Y: "<<ments->getMention(menty_id)->getNode()->toTextString()<<"\n   ";
//			int n_tokens = getTokenConnection(sent_theory, ments->getMention(mentx_id), Symbol(L"SLOT_X"), ments->getMention(menty_id), Symbol(L"SLOT_Y"), token_connection_stream);				
//			token_connection_stream<<"   DISTANCE: "<<n_tokens<<"\n";
//
//			if(n_tokens < min_tokens_between){
//				min_tokens_between = n_tokens;
//			}
//		}
//	}
//
//		//now, print information
//	bool matched_sentence = false;
//	if (n_active_pattern_matches == 0){	
//		matched_sentence = false;
//		//increment statistics
//		if(n_inactive_pattern_matches > 0){
//			_unmatched_counts[N_MATCH_INACTIVE]++;
//		}
//		_unmatched_counts[N_SENT]++; 
//		_unmatched_counts[INACTIVE_MATCH] += n_inactive_pattern_matches;
//		_unmatched_counts[MIN_DIST] += min_tokens_between;
//		if(best_depth < 15){
//			_unmatched_counts[BEST_DEPTH] += best_depth;
//			_unmatched_counts[N_DEPTH]++;
//		}
//		//Print to File
//		miss_out<<"**************** Recall Miss:" <<doc_info->getDocID()<<": ******************\n";
//		miss_out<<DistillUtilities::getTextString(sent_theory->getTokenSequence())<<"\n";
//		miss_out<<"SLOT_X Matches:  "<<slotx_fillers.size()<<"(ments: "<<slotx_mentionids.size()
//			<<")\n";
//		BOOST_FOREACH(int m, slotx_mentionids){
//			miss_out<<"   "<<ments->getMention(m)->getNode()->toTextString()<<"\n";
//		}
//		miss_out<<"SLOT_Y Matches: "<<sloty_fillers.size()<<"(ments: "<<sloty_mentionids.size()<<")\n";
//		BOOST_FOREACH(int m, sloty_mentionids){
//			miss_out<<"   "<<ments->getMention(m)->getNode()->toTextString()<<"\n";
//		}
//		miss_out<<"# Inactive Patterns Matched: "<<n_inactive_pattern_matches<<"\n";
//		miss_out<<"# prop connections depth< "<<max_depth<<":  "<<n_ok_prop_connections<<"\n";
//		miss_out<<"Minimum token distance: "<<min_tokens_between<<"\n";
//		if(n_inactive_pattern_matches > 0){
//			miss_out<<"=========== Inactive Patterns =========\n"<<matched_inactive_pattern_stream.str().c_str()<<"\n";
//		}
//		if(prop_connection_stream.str().length() > 0){
//			miss_out<<"=========== Prop Connections =========\n"<<prop_connection_stream.str().c_str()<<"\n";
//		}
//		if(token_connection_stream.str().length() > 0){
//			miss_out<<"=========== Token Connections =========\n"<<token_connection_stream.str().c_str()<<"\n";
//		}
//	}	
//	else{
//		//increment statistics
//		matched_sentence = true;
//		_matched_counts[N_SENT]++; 
//		_matched_counts[ACTIVE_MATCH]+= n_active_pattern_matches;
//		if(n_inactive_pattern_matches > 0){
//			_matched_counts[N_MATCH_INACTIVE]++;
//		}
//		_matched_counts[INACTIVE_MATCH] += n_inactive_pattern_matches;
//		_matched_counts[MIN_DIST] += min_tokens_between;
//		if(best_depth < 15){
//			_matched_counts[BEST_DEPTH] += best_depth;
//			_matched_counts[N_DEPTH]++;
//		}
//
//		correct_out<<"**************** LearnIt Found:" <<doc_info->getDocID()<<": ******************\n";
//		correct_out<<DistillUtilities::getTextString(sent_theory->getTokenSequence())<<"\n";
//		correct_out<<"SLOT_X Matches:  "<<slotx_fillers.size()<<"(ments: "<<slotx_mentionids.size()
//			<<")\n";
//		BOOST_FOREACH(int m, slotx_mentionids){
//			correct_out<<"   "<<ments->getMention(m)->getNode()->toTextString()<<"\n";
//		}
//		correct_out<<"SLOT_Y Matches: "<<sloty_fillers.size()<<"(ments: "<<sloty_mentionids.size()<<")\n";
//		BOOST_FOREACH(int m, sloty_mentionids){
//			correct_out<<"   "<<ments->getMention(m)->getNode()->toTextString()<<"\n";
//		}
//		correct_out<<"# Active Patterns Matched: "<<n_active_pattern_matches<<"\n";
//		correct_out<<"# Prop connections depth< "<<max_depth<<":  "<<n_ok_prop_connections<<"\n";
//		correct_out<<"Minimum token distance: "<<min_tokens_between<<"\n";
//		correct_out<<"=========== Active Patterns =========\n"<<matched_active_pattern_stream.str().c_str()<<"\n";
//		
//
//
//		
//	}
//
//	return matched_sentence;
//}
//
//*/
//
//
//
//
//
//void LearnItAnalyzer::dumpSummaryStatistics(){
//
//	int total_sentences = _all_counts[N_VALID] + _all_counts[N_INVALID];
//	SessionLogger::info("LEARNIT")<<"N Correct: "<<_all_counts[N_CORR_TOTAL]<<"("<<(float)_all_counts[N_CORR_TOTAL]/total_sentences<<")"<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    N Matched With Inactive Patterns: "<<_all_counts[N_CORR_MATCH_INACTIVE]<<
//		" ("<<(float)_all_counts[N_CORR_MATCH_INACTIVE]/_all_counts[N_CORR_TOTAL]<<" of correct)"<<
//		"    ("<<(float)_all_counts[N_CORR_MATCH_INACTIVE]/total_sentences<<" of total)"<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgDepth: "<<((float)_all_counts[BEST_DEPTH_CORR]/_all_counts[N_PROP_CORR])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgDist: "<<((float)_all_counts[MIN_DIST_CORR]/_all_counts[N_CORR_TOTAL])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgAct: "<<((float)_all_counts[CORR_ACTIVE_NPAT]/_all_counts[N_CORR_TOTAL])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgInact: "<<((float)_all_counts[CORR_INACTIVE_NPAT]/_all_counts[N_CORR_TOTAL])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"========================"<<std::endl;
//
//	SessionLogger::info("LEARNIT")<<"N Miss: "<<_all_counts[N_MISS_TOTAL]<<"("<<(float)_all_counts[N_MISS_TOTAL]/total_sentences<<")"<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    N Matched With Inactive Patterns: "<<_all_counts[N_MISS_MATCH_INACTIVE]<<
//		" ("<<(float)_all_counts[N_MISS_MATCH_INACTIVE]/_all_counts[N_MISS_TOTAL]<<" of misses)"
//		"    ("<<(float)_all_counts[N_MISS_MATCH_INACTIVE]/total_sentences<<" of total)"<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgDepth: "<<((float)_all_counts[BEST_DEPTH_MISS]/_all_counts[N_PROP_MISS])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgDist: "<<((float)_all_counts[MIN_DIST_MISS]/_all_counts[N_MISS_TOTAL])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgAct: "<<((float)_all_counts[MISS_ACTIVE_NPAT]/_all_counts[N_MISS_TOTAL])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgInact: "<<((float)_all_counts[MISS_INACTIVE_NPAT]/_all_counts[N_MISS_TOTAL])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"========================"<<std::endl;
//
//
//	SessionLogger::info("LEARNIT")<<"N False Alarms: "<<_all_counts[N_FA_TOTAL]<<"("<<(float)_all_counts[N_FA_TOTAL]/total_sentences<<")"<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    N Matched With Inactive Patterns: "<<_all_counts[N_FA_MATCH_INACTIVE]<<
//		" ("<<(float)_all_counts[N_FA_MATCH_INACTIVE]/_all_counts[N_FA_TOTAL]<<" of FAs)"
//		"    ("<<(float)_all_counts[N_FA_MATCH_INACTIVE]/total_sentences<<" of total)"<<std::endl; 
//	SessionLogger::info("LEARNIT")<<"    AvgDepth: "<<((float)_all_counts[BEST_DEPTH_FA]/_all_counts[N_PROP_FA])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgDist: "<<((float)_all_counts[MIN_DIST_FA]/_all_counts[N_FA_TOTAL])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgAct: "<<((float)_all_counts[FA_ACTIVE_NPAT]/_all_counts[N_FA_TOTAL])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgInact: "<<((float)_all_counts[FA_INACTIVE_NPAT]/_all_counts[N_FA_TOTAL])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"========================"<<std::endl;
//
//
//
//	/*
//	SessionLogger::info("LEARNIT")<<"N Matched: "<<_matched_counts[N_SENT]<<"("<<(float)_matched_counts[N_SENT]/(_matched_counts[N_SENT]+_unmatched_counts[N_SENT])<<")"<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    N Matched With Inactive Patterns: "<<_matched_counts[N_MATCH_INACTIVE]<<" ("<<(float)_matched_counts[N_MATCH_INACTIVE]/(_matched_counts[N_SENT]+_unmatched_counts[N_SENT])<<")"<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgDepth: "<<((float)_matched_counts[BEST_DEPTH]/_matched_counts[N_DEPTH])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgDist: "<<((float)_matched_counts[MIN_DIST]/_matched_counts[N_SENT])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgAct: "<<((float)_matched_counts[ACTIVE_MATCH]/_matched_counts[N_SENT])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgInact: "<<((float)_matched_counts[INACTIVE_MATCH]/_matched_counts[N_SENT])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"========================"<<std::endl;
//	SessionLogger::info("LEARNIT")<<"N UnMatched: "<<_unmatched_counts[N_SENT]<<"("<<(float)_unmatched_counts[N_SENT]/(_matched_counts[N_SENT]+_unmatched_counts[N_SENT])<<")"<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    N Matched With Inactive Patterns: "<<_unmatched_counts[N_MATCH_INACTIVE]<<" ("<<(float)_unmatched_counts[N_MATCH_INACTIVE]/(_matched_counts[N_SENT]+_unmatched_counts[N_SENT])<<")"<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgDepth: "<<((float)_unmatched_counts[BEST_DEPTH]/_unmatched_counts[N_DEPTH])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgDist: "<<((float)_unmatched_counts[MIN_DIST]/_unmatched_counts[N_SENT])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgAct: "<<((float)_unmatched_counts[ACTIVE_MATCH]/_unmatched_counts[N_SENT])<<std::endl;
//	SessionLogger::info("LEARNIT")<<"    AvgInact: "<<((float)_unmatched_counts[INACTIVE_MATCH]/_unmatched_counts[N_SENT])<<std::endl;
//	*/
//	
//}
//
//void LearnItAnalyzer::updatePatternCounts(int curr_count, int& max_count, int& tot_count, int& n_match){
//	if(curr_count > max_count)
//		max_count = curr_count;
//	tot_count+= curr_count;
//	if(curr_count > 0)
//		n_match++;
//}
//
//
//
//
//void LearnItAnalyzer::printPatternSummary(UTF8OutputStream& active, UTF8OutputStream& inactive, std::vector<Pattern_ptr> patterns){
//	int max_active_correct_match = 0;
//	int max_active_wrong_match = 0;
//	int max_inactive_correct_match = 0;
//	int max_inactive_wrong_match = 0;
//
//	int tot_active_correct_match = 0;
//	int tot_active_wrong_match = 0;
//	int tot_inactive_correct_match = 0;
//	int tot_inactive_wrong_match = 0;
//
//	int n_matched_active_correct_patterns = 0;
//	int n_matched_active_wrong_patterns = 0;
//	int n_matched_inactive_correct_patterns = 0;
//	int n_matched_inactive_wrong_patterns = 0;
//
//	int n_active = 0;
//
//	for(size_t p_no= 0; p_no < _pattern_match_counts.size(); p_no++){
//		if(patterns[p_no]->active())
//			n_active++;
//		int nc = _pattern_match_counts[p_no].first;
//		int nw = _pattern_match_counts[p_no].second;
//		if(_pattern_match_counts[p_no].first > 0 || _pattern_match_counts[p_no].second > 0  ){
//			if(patterns[p_no]->active()){
//				active<<"Pattern: "<<p_no<<" N_Matched: "<< nc + nw <<"\tCorrect: "<<nc<<" ("<<(float)nc/(nc+nw)
//					<<") \t"<<"Wrong: ("<< (float)nw/(nc+nw)<<")\n\t"<<patterns[p_no]->getPatternString()<<"\n";
//				updatePatternCounts(nc, max_active_correct_match, tot_active_correct_match, n_matched_active_correct_patterns);
//				updatePatternCounts(nw, max_active_wrong_match, tot_active_wrong_match, n_matched_active_wrong_patterns);
//			}
//			else{
//
//				inactive<<"Pattern: "<<p_no<<" N_Matched: "<< nc + nw <<"\tCorrect: "<<nc<<" ("<<(float)nc/(nc+nw)
//					<<") \t"<<"Wrong: ("<< (float)nw/(nc+nw)<<")\n\t"<<patterns[p_no]->getPatternString()<<"\n";
//				updatePatternCounts(nc, max_inactive_correct_match, tot_inactive_correct_match, n_matched_inactive_correct_patterns);
//				updatePatternCounts(nw, max_inactive_wrong_match, tot_inactive_wrong_match, n_matched_inactive_wrong_patterns);
//			}
//		}
//	}
//	SessionLogger::info("LEARNIT")<< "======= Pattern Statistics ======="<<std::endl;
//	SessionLogger::info("LEARNIT")<< "........ Active Patterns "<<n_active<<" ........."<<std::endl;
//	SessionLogger::info("LEARNIT")<< "MaxMatchedSentences: Correct: "<<max_active_correct_match<<" \tWrong: "<<max_active_wrong_match<<std::endl;
//	SessionLogger::info("LEARNIT")<< "# Patterns Used: Correct: "<<n_matched_active_correct_patterns<<"  ("<<(float)n_matched_active_correct_patterns/n_active<<") \tWrong: "
//		<< n_matched_active_wrong_patterns<<"  ("<<(float)n_matched_active_wrong_patterns/n_active<<")"
//		<<std::endl;
//	SessionLogger::info("LEARNIT")<< "AvgMatched Sentences Per Active Pattern: Correct: "<<(float)tot_active_correct_match/n_active<<" \tWrong: "
//		<<(float)tot_active_wrong_match/n_active<<std::endl;
//	int n_inactive = _pattern_match_counts.size()-n_active;
//	SessionLogger::info("LEARNIT")<< "........ Inactive Patterns "<<n_inactive<<" ........."<<std::endl;
//	SessionLogger::info("LEARNIT")<< "MaxMatchedSentences: Correct: "<<max_inactive_correct_match<<" \tWrong: "<<max_inactive_wrong_match<<std::endl;
//	SessionLogger::info("LEARNIT")<< "# Patterns Used: Correct: "
//		<<n_matched_inactive_correct_patterns<<"  ("<<(float)n_matched_inactive_correct_patterns/n_inactive
//		<<") \tWrong:"<<n_matched_inactive_wrong_patterns<<"  ("<<(float)n_matched_inactive_wrong_patterns/n_inactive<<")"<<std::endl;
//	SessionLogger::info("LEARNIT")<< "AvgMatched Sentences Per Active Pattern: Correct: "<<(float)tot_inactive_correct_match/n_inactive
//		<<" \tWrong:"<<(float)tot_inactive_wrong_match/n_inactive<<std::endl;
//	SessionLogger::info("LEARNIT")<< "\n"<<std::endl;
//
//
//
//}
