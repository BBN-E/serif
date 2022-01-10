// Copyright (c) 2018 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/ParamReader.h"
#include "Generic/edt/MentionGroups/extractors/ConfidentActorMatchExtractor.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/ActorMentionSet.h"
#include "Generic/actors/AWAKEActorInfo.h"

#include <boost/foreach.hpp>

ConfidentActorMatchExtractor::ConfidentActorMatchExtractor() :
	AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"actor-match"))
{
	_use_awake_info = ParamReader::isParamTrue("do_actor_match");
	validateRequiredParameters();
}

void ConfidentActorMatchExtractor::validateRequiredParameters() { }

std::vector<AttributeValuePair_ptr> ConfidentActorMatchExtractor::extractFeatures(const Mention& context,
                                                                                  LinkInfoCache& cache,
                                                                                  const DocTheory *docTheory) 
{
	std::vector<AttributeValuePair_ptr> results;
	if (_use_awake_info && context.getEntityType().matchesGPE()) {
		const SentenceTheory *st = docTheory->getSentenceTheory(context.getSentenceNumber());
		ActorMentionSet *ams = st->getActorMentionSet();
		std::set<ActorId> actorIds;
		ProperNounActorMention_ptr bestActorMatch = ProperNounActorMention_ptr();
		double best_georesolution_score = -1000000;
		BOOST_FOREACH(ActorMention_ptr am, ams->findAll(context.getUID())) {
			ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(am);
			if (!pnam)
				continue;
			if (pnam->getGeoresolutionScore() > 0 && pnam->getGeoresolutionScore() > best_georesolution_score) {
				bestActorMatch = pnam;
				best_georesolution_score = pnam->getGeoresolutionScore();
			}
		}
		if (bestActorMatch) {
			//std::wcout << "Best actor match for: " << context.toCasedTextString() << " | " << bestActorMatch->getActorName() << "\n";
			results.push_back(
				AttributeValuePair<int>::create(Symbol(L"confident-actor-match"), 
					                            bestActorMatch->getActorId().getId(), 
												getFullName()));
		} else {
			//std::wcout << "Didn't find best actor match for: " << context.toCasedTextString() << "\n";
		}
	}
	return results;
}
