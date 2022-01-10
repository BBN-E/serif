// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/extractors/GenderFeatureExtractor.h"
#include "Generic/edt/Guesser.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SynNode.h"

#include "Generic/common/SessionLogger.h"

GenderFeatureExtractor::GenderFeatureExtractor() : 
	AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"gender")) 
{
	validateRequiredParameters();
}

void GenderFeatureExtractor::validateRequiredParameters() {
	Guesser::initialize();
}

std::vector<AttributeValuePair_ptr> GenderFeatureExtractor::extractFeatures(const Mention& context,
                                                                            LinkInfoCache& cache,
                                                                            const DocTheory *docTheory)
{
	std::vector<AttributeValuePair_ptr> results;

	Symbol gender = Guesser::guessGender(context.getNode(), &context, _suspectedSurnames);
	if (SessionLogger::dbg_or_msg_enabled("MentionGroups_extractFeatures")) {
		SessionLogger::dbg("MentionGroups_extractFeatures") << context.getUID().toInt() << " (" << context.toCasedTextString() << ") has gender " << gender;
	}
	results.push_back(AttributeValuePair<Symbol>::create(Symbol(L"gender"), gender, getFullName()));
	return results;
}

void GenderFeatureExtractor::resetForNewDocument(const DocTheory *docTheory) {
	// Gather suspected last names, this will inform whether we assign
	// a gender to a single name
	_suspectedSurnames.clear();

	for (int i = 0; i < docTheory->getNSentences(); i++) {
		SentenceTheory *st = docTheory->getSentenceTheory(i);
		MentionSet *ms = st->getMentionSet();
		for (int j = 0; j < ms->getNMentions(); j++) {
			const Mention *mention = ms->getMention(j);
			if (!mention->getEntityType().matchesPER())
				continue;
			const SynNode *nameNode = mention->getEDTHead();
			std::vector<Symbol> words = nameNode->getTerminalSymbols();
			if (words.size() < 2)
				continue;
			Symbol lastName = words[words.size() - 1];
			_suspectedSurnames.insert(lastName);
		}
	}
}
