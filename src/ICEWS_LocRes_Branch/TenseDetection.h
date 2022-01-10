// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef TENSE_DETECTION_H
#define TENSE_DETECTION_H

#include <boost/regex.hpp>

#include "ICEWS/EventMentionFinder.h"

class SentenceTheory;
class ValueMentionSet;
class MentionSet;
class ValueMention;
class Argument;
class Proposition;
class Mention;
class SynNode;
class PatternFeatureSet;
class EventTriple;

namespace ICEWS {

class TenseDetection {

public:

	static void setTense(PatternFeatureSet_ptr match, ICEWSEventMentionFinder::MatchData& matchData, const DocTheory *docTheory, SentenceTheory *sentTheory);

private:

	static Symbol getTense(PatternFeatureSet_ptr match, ICEWSEventMentionFinder::MatchData& matchData, const DocTheory *docTheory, SentenceTheory *sentTheory);
	static Symbol getTense(SentenceTheory *sentTheory, const Mention* mention);
	static Symbol getTense(const SynNode *node);
	static int getHistoricity(const DocTheory *docTheory, SentenceTheory *st, const Proposition *prop, int depth);
	static int getHistoricity(const DocTheory *docTheory, SentenceTheory *st, ValueMention *val);
	static bool isSinceValue(SentenceTheory *st, ValueMention *val);
	static ValueMention* getValueFromArgument(MentionSet *ms, ValueMentionSet *vms, Argument *arg, bool is_known_temp = false);
	static ValueMention* getValueFromMention(ValueMentionSet *vms, const Mention *ment, bool is_known_temp);
	
	static const boost::wregex _timex_regex_ymd;
	static const boost::wregex _timex_regex_ym;
	static const boost::wregex _timex_regex_yw;
	static const boost::wregex _timex_regex_y;
	static const boost::wregex _timex_clock_time;
	static const boost::wregex _timex_regex_decade;
	static const boost::wregex _timex_regex_past_my;

	static const boost::wregex _superlative;
	static bool hasSuperlative(SentenceTheory *st);

	enum { OLDER_THAN_ONE_MONTH, OLDER_THAN_ONE_MONTH_ONGOING, WITHIN_ONE_MONTH, UNKNOWN };

};

}

#endif
