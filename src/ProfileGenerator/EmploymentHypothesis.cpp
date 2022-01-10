// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnicodeUtil.h"

#include "ProfileGenerator/EmploymentHypothesis.h"
#include "ProfileGenerator/PGFact.h"

#include "boost/foreach.hpp"
#include "boost/algorithm/string.hpp"
#include <set>
#include <string>
#include <boost/scoped_ptr.hpp>

std::map<std::wstring, int> EmploymentHypothesis::_rankedJobTitles;

const std::string EmploymentHypothesis::TITLE_ROLE = "Title";
const std::string EmploymentHypothesis::STATUS_ROLE = "Status";

EmploymentHypothesis::EmploymentHypothesis(PGFact_ptr fact, employment_argument_type_t arg_type, PGDatabaseManager_ptr pgdm) : 
_job_title(L""), _job_title_up_to_date(false), _namedArgument(NameHypothesis_ptr()), _is_top_brass(false), _reliable_from_employee_pov(false), _is_spokesman(false), 
_employment_argument_type(arg_type), _num_reliable_titles_seen(0), _pgdm(pgdm)
{
	addFact(fact);
	updateJobTitleInfo();
}

void EmploymentHypothesis::addHypothesisSpecificFact(PGFact_ptr fact) {
	// Called by addFact() in GenericHypothesis

	// We want to add the employee/employer name into the hypothesis for our named argument
	// That way we get the benefit of the display value selection on the name etc.
	// We assume there is only one of these for this answer type	
	// However, there can be none, if the employee/employer didn't map to an actor in the DB
	// If so, we return without setting _namedArgument
	PGFactArgument_ptr answerArg = fact->getAnswerArgument();	
	if (answerArg == PGFactArgument_ptr())
		return;

	NameHypothesis_ptr newNameHypo = boost::make_shared<NameHypothesis>(answerArg->getActorId(), answerArg->getResolvedStringValue(), 
		_employment_argument_type == EMPLOYEE, _pgdm);
	if (_namedArgument == NameHypothesis_ptr())
		_namedArgument = newNameHypo;
	else _namedArgument->addSupportingHypothesis(newNameHypo);

	// Note hard-coded-ness
	std::vector<PGFactArgument_ptr> jobTitleArgs = fact->getArgumentsForRole(TITLE_ROLE);
	BOOST_FOREACH(PGFactArgument_ptr jobTitle, jobTitleArgs) {
		std::wstring job_title = jobTitle->getLiteralStringValue();
		if (job_title == L"")
			continue;
		std::transform(job_title.begin(), job_title.end(), job_title.begin(), towlower);
		if (job_title == L"sen.") job_title = L"senator";
		else if (job_title == L"rep.") job_title = L"representative";
		else if (job_title == L"gov.") job_title = L"governor";
		_jobTitleCounts[job_title]++;
		if (fact->hasReliableAgentMentionConfidence())
			_num_reliable_titles_seen++;
	
	}
	_job_title_up_to_date = false;

	// Note hard-coded-ness
	// Only keep statuses for reliable agent1s
	if (fact->hasReliableAgentMentionConfidence()) {	
		std::vector<PGFactArgument_ptr> statusArgs = fact->getArgumentsForRole(STATUS_ROLE);
		BOOST_FOREACH(PGFactArgument_ptr statusArg, statusArgs) {
			std::wstring status = statusArg->getLiteralStringValue();
			if (status == L"")
				continue;
			std::transform(status.begin(), status.end(), status.begin(), towlower);
			_statusCounts[status]++;
		}
	}
}

EmploymentHypothesis::status_tense_t EmploymentHypothesis::getTense(bool print_debug) {	

	typedef std::pair<std::wstring, int> wstr_int_pair_t;	
	int future = 0;
	int past = 0;
	BOOST_FOREACH(wstr_int_pair_t my_pair, _statusCounts) {
		if (isPastStatusWord(my_pair.first))
			past += my_pair.second;
		if (isFutureStatusWord(my_pair.first))
			future += my_pair.second;
	}

	double past_ratio = (double) past / _num_reliable_titles_seen;

	// To be removed later
	if (print_debug && past > 0) {
		std::wstring result = _namedArgument->getDisplayValue() + L" (" + getJobTitle() + L")";
		SessionLogger::info("PG") << result << ": " << "TOTAL--" << _num_reliable_titles_seen << ", PAST--" << past << ", ratio: " << past_ratio << "\n";
	}

	if (past_ratio > .19)	
		return PAST;

	// no future for now, should really compare to total count
	return CURRENT;
}

void EmploymentHypothesis::updateJobTitleInfo() {
	typedef std::pair<std::wstring, int> wstr_int_pair_t;
	std::wstring best_job_title = L"";
	int best_count = 0;

	BOOST_FOREACH(wstr_int_pair_t my_pair, _jobTitleCounts) {
		// ignore null titles
		if (my_pair.first == L"")
			continue;

		// tie-breaking is just based on the order of the facts, for lack of a better method
		if (my_pair.second > best_count) {
			best_count = my_pair.second;
			best_job_title = my_pair.first;
		}
	}
	_job_title = best_job_title;

	if (_job_title == L"members")
		_job_title = L"member";
	if (_job_title == L"leaders")
		_job_title = L"leader";
	if (_job_title == L"heads")
		_job_title = L"head";

	int jt_rank = rankOfJobTitle(_job_title);
	if (jt_rank == -1){
		// try with space instead of dashes
		std::wstring white_jt = UnicodeUtil::whiteOutDashes(_job_title);
		jt_rank = rankOfJobTitle(white_jt);
	}
	_is_top_brass = false;
	_is_spokesman = false;
	if (jt_rank != -1) {
		if (jt_rank <= rankOfJobTitle(L"top brass"))
			_is_top_brass = true;
		if (jt_rank <= rankOfJobTitle(L"spokesman"))
			_is_spokesman = true;
	}

	_job_title_up_to_date = true;
}

std::wstring& EmploymentHypothesis::getJobTitle() {
	if (!_job_title_up_to_date)
		updateJobTitleInfo();
	return _job_title;
}
bool EmploymentHypothesis::isTopBrass() {
	if (!_job_title_up_to_date)
		updateJobTitleInfo();
	return _is_top_brass;
}
bool EmploymentHypothesis::isSpokesman() {
	if (!_job_title_up_to_date)
		updateJobTitleInfo();
	return _is_spokesman;
}

bool EmploymentHypothesis::hasSecondaryTitleWord() {
	return (_job_title.find(L"member") != _job_title.npos ||
		_job_title.find(L"candidate") != _job_title.npos ||
		_job_title.find(L"adviser") != _job_title.npos ||
		_job_title.find(L"advisor") != _job_title.npos ||
		_job_title.find(L"flag bearer") != _job_title.npos ||
		_job_title.find(L"standard bearer") != _job_title.npos ||
		_job_title.find(L"nominee") != _job_title.npos ||
		_job_title.find(L"commander") != _job_title.npos ||
		_job_title.find(L"leader") != _job_title.npos ||
		_job_title.find(L"head") != _job_title.npos);
}

bool EmploymentHypothesis::employerIsPoliticalParty() {
	if (!isEmployerHypothesis()) 
		return false;

	std::wstring displayValue = getDisplayValue();
	return (displayValue.find(L"party") != displayValue.npos || 
		    displayValue.find(L"Party") != displayValue.npos);
}

void EmploymentHypothesis::addSupportingHypothesis(GenericHypothesis_ptr hypo) {
	BOOST_FOREACH(PGFact_ptr fact, hypo->getSupportingFacts()) {
		addFact(fact);
	}
}

bool EmploymentHypothesis::isIllegalHypothesis(ProfileSlot_ptr slot, std::string& rationale) {

	if (!_job_title_up_to_date)
		updateJobTitleInfo();

	// We only trust reliable answers and agents
	if (nDoublyReliableFacts() == 0) {
		rationale = "not doubly-reliable";
		return true;
	}

	if (_namedArgument == NameHypothesis_ptr()){
		rationale = "no name for named argument (employer/employee)";
		return true;
	} else if (_namedArgument->getDisplayValue() == L"") {
		rationale = "no name for named argument (employer/employee)";
		return true;
	} else if (slot->getUniqueId() == ProfileSlot::LEADER_UNIQUE_ID && !_is_top_brass){
		rationale = "top brass needs a top ranking job title";
		return true;
	} else if (slot->getUniqueId() == ProfileSlot::EMPLOYEE_UNIQUE_ID && _is_top_brass){
		rationale = "employee needs a low-ranking or non-existent job title";
		return true;
	} else if (_job_title == L"sect") {
		rationale = "bad job title";
		return true;
	}
	
	if (_employment_argument_type == EMPLOYEE) {
		if (!_reliable_from_employee_pov) {
			rationale = "not marked reliable from employee POV";
			return true;
		} else if (_namedArgument->getNormalizedValue().length() < 2){
			rationale = " name too small without its honorifics";
			return true;
		}
	}
	
	return false;
}

bool EmploymentHypothesis::isEquiv(GenericHypothesis_ptr hypoth) { 
	// Two employee/employer hypotheses are equivalent if they point to the same named entity, regardless of job title
	EmploymentHypothesis_ptr empHypoth = boost::dynamic_pointer_cast<EmploymentHypothesis>(hypoth);
	if (empHypoth == EmploymentHypothesis_ptr())
		return false;
	NameHypothesis_ptr empNameHyp = boost::dynamic_pointer_cast<NameHypothesis>(empHypoth->getNamedArgument());
	return isEquivNamedArgument(empNameHyp);
}

bool EmploymentHypothesis::isSimilar(GenericHypothesis_ptr hypoth) { 
	EmploymentHypothesis_ptr empHypoth = boost::dynamic_pointer_cast<EmploymentHypothesis>(hypoth);
	if (empHypoth == EmploymentHypothesis_ptr())
		return false;
	NameHypothesis_ptr empNameHyp = boost::dynamic_pointer_cast<NameHypothesis>(empHypoth->getNamedArgument());
	if (isSimilarNamedArgument(empNameHyp))
		return true; 

	if (!_job_title_up_to_date)
		updateJobTitleInfo();
	
	if (_employment_argument_type == EMPLOYEE) {
		// two employees are similar if they have the same "top brass" title, e.g. we assume
		//  that if there are two CEOs, they are likely to be the same person. This is obviously
		//  not true for titles like 'spokesman'.
		if (_is_top_brass && _job_title != L"" && getTense() == empHypoth->getTense())
			return isEquivJobTitle(empHypoth->getJobTitle());
	}
	if (_employment_argument_type == EMPLOYER) {
		// two employers are similar if they have the same title, e.g. we assume
		//  that if someone is the CEO of two companies, those are probably the same company
		if (isEquivJobTitle(empHypoth->getJobTitle()))
			return true;
	}

	return false;
}

bool EmploymentHypothesis::isEquivJobTitle(std::wstring &otherJobTitle) { 
	// make sure we call getJobTitle() so we know it's up to date
	return jobTitlesEquiv(getJobTitle(), otherJobTitle);
}

bool EmploymentHypothesis::isEquivNamedArgument(NameHypothesis_ptr& otherNamedArgument) {
	if (_namedArgument == NameHypothesis_ptr() && otherNamedArgument == NameHypothesis_ptr())
		return true;
	else if (_namedArgument == NameHypothesis_ptr() || otherNamedArgument == NameHypothesis_ptr())
		return false;
	// Not sure if this is reciprocal, so let's be safe
	return _namedArgument->isEquiv(otherNamedArgument) || otherNamedArgument->isEquiv(_namedArgument);
}

bool EmploymentHypothesis::isSimilarNamedArgument(NameHypothesis_ptr& otherNamedArgument) {
	if (_namedArgument == NameHypothesis_ptr() && otherNamedArgument == NameHypothesis_ptr())
		return true;
	else if (_namedArgument == NameHypothesis_ptr() || otherNamedArgument == NameHypothesis_ptr())
		return false;
	return _namedArgument->isSimilar(otherNamedArgument) || otherNamedArgument->isSimilar(_namedArgument);
}

std::wstring EmploymentHypothesis::getDisplayValue() { 
	if (_namedArgument == NameHypothesis_ptr())
		return L""; // this is bad! fortunately we mark these as illegal, plus they should never happen
	std::wstring result = _namedArgument->getDisplayValue();

	// make sure we call getJobTitle() so we know it's up to date
	std::wstring job_title = getJobTitle();

	if (job_title.size() != 0) {
		std::wstring capitalizedTitle = EmploymentHypothesis::capitalizeTitle(job_title);
		result += L" (" + capitalizedTitle + L")";
	}

	status_tense_t tense = getTense();
	if (tense == PAST) 
		result += L" [FORMER]";
	else if (tense == FUTURE)
		result += L" [FUTURE]";	

	return result;
}

std::vector<GenericHypothesis::kb_arg_t> EmploymentHypothesis::getKBArguments(int actor_id, ProfileSlot_ptr slot) {
	std::vector<kb_arg_t> kb_args;

	kb_arg_t focus;
	focus.actor_id = actor_id;
	focus.value = L"";
	focus.role = slot->getFocusRole();
	kb_args.push_back(focus);

	NameHypothesis_ptr namedArg = getNamedArgument();
	if (namedArg) {
		kb_arg_t answer;
		answer.actor_id = namedArg->getActorId();
		answer.value = L"";
		answer.role = slot->getAnswerRole();
		kb_args.push_back(answer);
	}

	kb_arg_t title;
	title.actor_id = -1;
	title.role = TITLE_ROLE; // note hard-coded
	title.value = getJobTitle(); // make sure we call getJobTitle() so we know it's up to date
	kb_args.push_back(title);

	return kb_args;
}


int EmploymentHypothesis::rankAgainst(GenericHypothesis_ptr hypo) {
	EmploymentHypothesis_ptr empHypo = boost::dynamic_pointer_cast<EmploymentHypothesis>(hypo);
	if (empHypo == EmploymentHypothesis_ptr()) // cast failed
		return SAME;

	if (hasExternalHighConfidenceFact() && !hypo->hasExternalHighConfidenceFact())
		return BETTER;
	else if (hypo->hasExternalHighConfidenceFact() && !hasExternalHighConfidenceFact())
		return WORSE;

	if (nSupportingFacts() > hypo->nSupportingFacts())
		return BETTER;
	else if (hypo->nSupportingFacts() > nSupportingFacts())
		return WORSE;

	int jt_contest = rankJobTitles(getJobTitle(), empHypo->getJobTitle());
	if (jt_contest != SAME)
		return jt_contest;

	if (getBestFactScore() > hypo->getBestFactScore())
		return BETTER;
	else if (hypo->getBestFactScore() > getBestFactScore())
		return WORSE;

	// Break ties and show more recent epoch facts (with id as proxy) ahead of older epoch
	if (getNewestFactID() > hypo->getNewestFactID())
		return BETTER;
	else if (hypo->getNewestFactID() > getNewestFactID())
		return WORSE;

	// should never happen unless they share facts, which they shouldn't
	return SAME;
}

// static methods below here

int EmploymentHypothesis::rankJobTitles(std::wstring jt1, std::wstring jt2){
	int r1 = rankOfJobTitle(UnicodeUtil::whiteOutDashes(jt1));
	int r2 = rankOfJobTitle(UnicodeUtil::whiteOutDashes(jt2));
	if (r1 == r2) return SAME;
	if (r1 == -1) return WORSE;
	if (r2 == -1) return BETTER;
	if (r1 < r2) return BETTER;
	return WORSE;
}
void EmploymentHypothesis::loadJobTitles(){
	static bool init = false;
	if (!init) {
		boost::scoped_ptr<UTF8InputStream> ranked_jt_istream_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& ranked_jt_istream(*ranked_jt_istream_scoped_ptr);
		std::string jt_fpath = ParamReader::getRequiredParam("job_title_rank_path");
		ranked_jt_istream.open(jt_fpath.c_str());
		if (ranked_jt_istream.fail()) {
			throw UnexpectedInputException("GenerateProfiles EmploymentHypothesis loadJobTitles failed to open", jt_fpath.c_str());
		}
		std::wstring title;
		int count = 0;
		while (!ranked_jt_istream.eof()) {
			ranked_jt_istream.getLine(title);
			if (title == L"")
				break;
			_rankedJobTitles[title] = count++;
		}
		init = true;
	}
}

int EmploymentHypothesis::rankOfJobTitle(std::wstring jt){
	loadJobTitles(); //no-op if already loaded
	int rank = -1;
	std::wstring bareJT = jt;
	// the job title already has dashes mapped to blanks
	if (boost::starts_with(bareJT, L"co "))
		bareJT = bareJT.substr(3);
	if (boost::starts_with(bareJT, L"vice "))
		bareJT = bareJT.substr(5);
	if (_rankedJobTitles.find(bareJT) != _rankedJobTitles.end())
		rank = _rankedJobTitles[bareJT];
	return rank;
}

bool EmploymentHypothesis::jobTitlesEquiv(const std::wstring job_title, const std::wstring otherJobTitle){
	if (job_title == otherJobTitle)
		return true;
	std::wstring wh_jt = UnicodeUtil::whiteOutDashes(job_title);
	std::wstring wh_other = UnicodeUtil::whiteOutDashes(otherJobTitle);
	if (wh_jt == wh_other)
		return true;
	std::wstring shorter, longer;

	if (longer.length() < shorter.length()){
		shorter = wh_other;
		longer = wh_jt;
	}else{
		longer = wh_jt;
		shorter = wh_other;
	}
	if (shorter == L"ceo")
		return (longer ==L"chief executive officer");
	else if (shorter == L"cio")
		return (longer == L"chief investment officer");
	else if (shorter == L"coo")
		return (longer == L"chief operating officer");
	else if (shorter == L"cto")
		return (longer == L"chief technology officer");
	else if (shorter == L"mp")
		return (longer == L"member of parliament");
	else if (shorter == L"mc") 
		return (longer == L"master of ceremonies");
	else if (shorter == L"syg")
		return (longer == L"secretary general");
	else return false;
}
std::wstring EmploymentHypothesis::capitalizeTitle(std::wstring title){
	std::wstring capitalizedTitle = L"";
	bool acronym = (title == L"ceo" || title == L"cio" || title == L"coo" || 
		title == L"cto" || title == L"hh" || title == L"hm" || title == L"hrm" ||
		title == L"mc" || title == L"mp" || title == L"syg");
	// Uppercase first letter of each word but if acronym do all letters
	for (size_t i = 0; i < title.size(); i++) {
		wchar_t letter = title.at(i);
		if (acronym || i == 0 || title.at(i-1) == L' ' || title.at(i-1) == L'-')
			letter = towupper(letter);
		capitalizedTitle.push_back(letter);
	}
	return capitalizedTitle;
}

bool EmploymentHypothesis::isPastStatusWord(std::wstring word) {
	return (boost::iequals(word, L"former") ||
			boost::iequals(word, L"then") ||
			boost::iequals(word, L"late") ||
			boost::iequals(word, L"slain") ||
			boost::iequals(word, L"assassinated") ||
			boost::iequals(word, L"deposed") ||
			boost::iequals(word, L"retired") ||
			boost::iequals(word, L"past") ||
			boost::iequals(word, L"onetime") ||
			boost::iequals(word, L"one-time") ||
			boost::iequals(word, L"ousted") ||
			boost::iequals(word, L"previous"));
}

bool EmploymentHypothesis::isFutureStatusWord(std::wstring word) {
	return (boost::iequals(word, L"next") || 
			boost::iequals(word, L"future"));
}

bool EmploymentHypothesis::isPresentStatusWord(std::wstring word) {
	return (boost::iequals(word, L"new") ||
			boost::iequals(word, L"current") ||
			boost::iequals(word, L"sitting") ||
			boost::iequals(word, L"active"));
}

bool EmploymentHypothesis::excludeQuestionableHoldDates() {
	return true;
}
