// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Spanish/edt/MentionGroups/constraints/es_NameParseClashConstraint.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Mention.h"
#include "Spanish/edt/MentionGroups/extractors/es_PersonNameParseExtractor.h"

#include <boost/foreach.hpp>


typedef SpanishPersonNameParseExtractor::SpanishNameParse SpanishNameParse;
typedef std::vector<SpanishNameParse> SpanishNameParseList;

namespace {
	void showParse(const SpanishNameParse &parse) {
		std::wcerr << "  " << parse.toString() << std::endl;
	}
	void showParseList(SpanishNameParseList parses) {
		BOOST_FOREACH(const SpanishNameParse& parse, parses)
			showParse(parse);
	}
}

bool SpanishNameParseClashConstraint::violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const {
	std::vector<AttributeValuePair_ptr> features;
	for (MentionGroup::const_iterator it = mg1.begin(); it != mg1.end(); ++it) {
		std::vector<AttributeValuePair_ptr> f = cache.getMentionFeaturesByName(*it, Symbol(L"Mention-es-person-name-parse"), 
																			   SpanishPersonNameParseExtractor::FEATURE_NAME);
		features.insert(features.end(), f.begin(), f.end());
	}
	for (MentionGroup::const_iterator it = mg2.begin(); it != mg2.end(); ++it) {
		std::vector<AttributeValuePair_ptr> f = cache.getMentionFeaturesByName(*it, Symbol(L"Mention-es-person-name-parse"), 
																			   SpanishPersonNameParseExtractor::FEATURE_NAME);
		features.insert(features.end(), f.begin(), f.end());
	}
	if (features.empty())
		return false; // Not merging person names.

	// We should merge if there is any consistent set of parses.

	// Initialize the queue.
	SpanishNameParseList queue;
	queue.push_back(SpanishNameParse());

	BOOST_FOREACH(AttributeValuePair_ptr avp, features) {
		const SpanishNameParseList &candidates = boost::dynamic_pointer_cast<AttributeValuePair<SpanishNameParseList> >(avp)->getValue();

		SpanishNameParseList new_queue;
		BOOST_FOREACH(const SpanishNameParse &parse, queue) {
			BOOST_FOREACH(const SpanishNameParse &candidate, candidates) {
				if (parse.is_consistent(candidate))
					new_queue.push_back(parse.merge(candidate));
			}
		}
		if (new_queue.empty()) {
			if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) {
				SessionLogger::dbg("MentionGroups_violatesConstraint") 
					<< "SpanishNameParseClashConstraint blocked merge of two groups";
			}		
			return true;
		}
		std::swap(queue, new_queue);
	}
	
	if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) {
		SessionLogger::dbg("MentionGroups_violatesConstraint") 
			<< "SpanishNameParseClashConstraint allowed merge of two groups";
	}		
	//std::cerr << "SpanishNameParseClashConstraint allowed merge of two groups.  Possible parses:" << std::endl;
	//showParseList(queue);
	return false;
}
