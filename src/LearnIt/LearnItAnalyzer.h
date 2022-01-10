#include "LearnIt/MatchInfo.h"
class DocumentInfo;
class SentenceTheory;
class SlotFiller;
class Pattern;
#include "LearnIt/Seed.h"


typedef boost::shared_ptr<SlotFiller> SlotFiller_ptr;
typedef boost::shared_ptr<Pattern> Pattern_ptr;

class LearnItAnalyzer: boost::noncopyable {
public:
	LearnItAnalyzer(std::vector<Pattern_ptr> patterns);
	int generatePatternMatchDebugString(DocumentInfo *doc_info, SentenceTheory* sent_theory,
									std::vector<SlotFiller_ptr> slotx_fillers, std::vector<SlotFiller_ptr> sloty_fillers, 
									std::vector<Pattern_ptr> patterns, bool active, bool correct,
									std::wstringstream& string_stream);

	std::vector<int> getMentionsForSlotFillers(SentenceTheory* sent_theory, std::vector<SlotFiller_ptr> slot_fillers);
	MatchInfo::PatternMatches getMatchesForPatternAndSlots(DocumentInfo *doc_info, SentenceTheory* sent_theory, Pattern_ptr pattern,  
										   std::vector<SlotFiller_ptr> slotx_fillers, std::vector<SlotFiller_ptr> sloty_fillers);
	
	bool LearnItAnalyzer::printRecallMissAnalysisForRelationSeedsAndSentence(DocumentInfo *doc_info, int sent_index, 
													  std::vector<Pattern_ptr> patterns, Target_ptr target, std::vector<SlotFiller_ptr> slotx_fillers,
													   std::vector<SlotFiller_ptr> sloty_fillers, UTF8OutputStream &miss_out, UTF8OutputStream& correct_out);
	bool LearnItAnalyzer::printAnalysisForRelationSeedsAndSentence(DocumentInfo *doc_info, Seed_ptr seed, int sent_index, bool correct,
													  std::vector<Pattern_ptr> patterns, Target_ptr target, std::vector<SlotFiller_ptr> slotx_fillers,
													   std::vector<SlotFiller_ptr> sloty_fillers, UTF8OutputStream &miss_out, 
													   UTF8OutputStream &fa_out, UTF8OutputStream& correct_out);
	bool LearnItAnalyzer::printPrecisionAnalysisForRelationSeedsAndSentence(DocumentInfo *doc_info, int sent_index, 
													  std::vector<Pattern_ptr> patterns, Target_ptr target, std::vector<SlotFiller_ptr> slotx_fillers,
													   std::vector<SlotFiller_ptr> sloty_fillers, UTF8OutputStream &miss_out, UTF8OutputStream &fa_out, 
													   UTF8OutputStream& correct_out);

	void dumpSummaryStatistics();
	void printPatternSummary(UTF8OutputStream& active, UTF8OutputStream& inactive, std::vector<Pattern_ptr> patterns);
private:
	//enum {N_SENT, N_MATCH_INACTIVE, ACTIVE_MATCH, INACTIVE_MATCH, BEST_DEPTH, N_DEPTH, MIN_DIST};
	static const int N_COUNT_TYPES = 23;
	typedef enum {N_VALID, N_INVALID,
		FA_ACTIVE_NPAT, FA_INACTIVE_NPAT, N_FA_TOTAL,  N_FA_MATCH_INACTIVE,
		MISS_ACTIVE_NPAT, MISS_INACTIVE_NPAT, N_MISS_TOTAL, N_MISS_MATCH_INACTIVE,
		CORR_ACTIVE_NPAT, CORR_INACTIVE_NPAT, N_CORR_TOTAL, N_CORR_MATCH_INACTIVE,
		BEST_DEPTH_FA,  N_PROP_FA, MIN_DIST_FA, 
		BEST_DEPTH_MISS, N_PROP_MISS, MIN_DIST_MISS,
		BEST_DEPTH_CORR, N_PROP_CORR, MIN_DIST_CORR} COUNT_TYPES;


	std::vector<int> _all_counts;
	//std::vector<int> _unmatched_counts;
	std::vector<std::pair<int, int>> _pattern_match_counts;


	/* Generic functions that look for possible connections between two mentions */
	std::wstring getPropPrefix(const Proposition *prop);
	std::wstring findConnection(PropositionSet *ps, MentionSet *ms, const Proposition *prop, const Mention *m, Symbol label, int indent, int& depth, bool pretty_print);
	int getTokenConnection(SentenceTheory *st, const Mention *m1, Symbol label1, const Mention *m2, Symbol label2, std::wstringstream& tokenStream);
	std::wstring getBestPropConnection(SentenceTheory *st, const Mention *m1, Symbol label1, const Mention *m2, Symbol label2, bool pretty_print, int& depth);

	void updatePatternCounts(int curr_count, int& max_count, int& tot_count, int& n_match); 
	void updateMatchCounts(int n_active, int n_inactive, int this_depth, int this_dist, int& active_npat, 
		int&  inactive_npat, int& count, int& inactive_count, int& depth, int& ndepth, int& dist);


};
