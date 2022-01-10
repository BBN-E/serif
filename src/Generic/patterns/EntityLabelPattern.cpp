// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include <sstream>
#include <boost/regex.hpp>
#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Sexp.h"
#include "Generic/patterns/EntityLabelPattern.h"
#include "Generic/patterns/EventPattern.h"
#include "Generic/patterns/MentionPattern.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/PropPattern.h"
#include "Generic/patterns/RelationPattern.h"
#include "Generic/patterns/features/MentionPFeature.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/RelationSet.h"

EntityLabelPattern::EntityLabelPattern(Sexp *sexp, const Symbol::HashSet &prevLabels,
                                       const PatternWordSetMap& wordSets) 
{
	int nkids = sexp->getNumChildren();
	if (nkids == 1) {
		_label = sexp->getFirstChild()->getValue();
		_pattern.reset();
	} else if (nkids == 2) {
		_label = sexp->getFirstChild()->getValue();
		_pattern = Pattern::parseSexp(sexp->getSecondChild(), prevLabels, wordSets);
		if (!(boost::dynamic_pointer_cast<RelationPattern>(_pattern) ||
		      boost::dynamic_pointer_cast<EventPattern>(_pattern) ||
		      boost::dynamic_pointer_cast<PropPattern>(_pattern) ||
		      boost::dynamic_pointer_cast<MentionPattern>(_pattern)))
			throw UnexpectedInputException("EntityLabelPattern::EntityLabelPattern()", 
				"EntityLabelPattern patterns must be relations, props, mentions, or events");
	} else {
		std::ostringstream err;
		err << "Wrong number of children in entity label pattern sexp: " << sexp->to_debug_string();
		throw UnexpectedInputException("EntityLabelPattern::EntityLabelPattern()", err.str().c_str());
	}	
}

EntityLabelPattern::EntityLabelPattern(Symbol label): _label(label) {
}

void EntityLabelPattern::labelEntities(PatternMatcher_ptr patternMatcher, EntityLabelPattern::EntityLabelFeatureMap& result) const {
	if (_pattern == 0)
		return; // For automatic (non-pattern) labels like AGENT1.

	if (MentionPattern_ptr mentionPattern = boost::dynamic_pointer_cast<MentionPattern>(_pattern)) {
		for (int sentno = 0; sentno < patternMatcher->getDocTheory()->getNSentences(); sentno++) {
			MentionSet *mentions = patternMatcher->getDocTheory()->getSentenceTheory(sentno)->getMentionSet();
			for (int i = 0; i < mentions->getNMentions(); i++) {
				PatternFeatureSet_ptr sfs = mentionPattern->matchesMention(patternMatcher, mentions->getMention(i), false); // don't allow fall-through, they'll each get their chance
				if (sfs) labelFeatureSet(sfs, patternMatcher, result);
			}
		}
	} else if (RelationPattern_ptr relationPattern = boost::dynamic_pointer_cast<RelationPattern>(_pattern)) { 
		RelationSet *relations = patternMatcher->getDocTheory()->getRelationSet();
		for (int i = 0; i < relations->getNRelations(); i++) {
			Relation *rel = relations->getRelation(i);	
			const Relation::LinkedRelMention *lrm = rel->getMentions();
			while (lrm != 0) {
				RelMention* relMention = lrm->relMention;
				const Mention* leftMention = relMention->getLeftMention();
				Entity* leftEnt = patternMatcher->getDocTheory()->getEntitySet()->getEntityByMention(leftMention->getUID());
				int sentno = leftMention->getSentenceNumber();
				PatternFeatureSet_ptr sfs = relationPattern->matchesRelMention(patternMatcher, sentno, relMention);
				if (sfs) labelFeatureSet(sfs, patternMatcher, result);
				lrm = lrm->next;
			}
		}
	} else if (EventPattern_ptr eventPattern = boost::dynamic_pointer_cast<EventPattern>(_pattern)) {
		for (int sentno = 0; sentno < patternMatcher->getDocTheory()->getNSentences(); sentno++) {
			EventMentionSet *vmSet = patternMatcher->getDocTheory()->getSentenceTheory(sentno)->getEventMentionSet();
			for (int v = 0; v < vmSet->getNEventMentions(); v++) {
				PatternFeatureSet_ptr sfs = eventPattern->matchesEventMention(patternMatcher, sentno, vmSet->getEventMention(v));
				if (sfs) labelFeatureSet(sfs, patternMatcher, result);
			}
		}	
	} else if (PropPattern_ptr propPattern = boost::dynamic_pointer_cast<PropPattern>(_pattern)) {
		for (int sentno = 0; sentno < patternMatcher->getDocTheory()->getNSentences(); sentno++) {
			//if (sentno != 37) { continue; }
			PropositionSet *propSet = patternMatcher->getDocTheory()->getSentenceTheory(sentno)->getPropositionSet();
			for (int i = 0; i < propSet->getNPropositions(); i++) {
				//if (i != 0) { continue; }
				PatternFeatureSet_ptr sfs = propPattern->matchesProp(patternMatcher, sentno, propSet->getProposition(i), false, PropStatusManager_ptr()); // don't allow fall-through, they'll each get their chance
				if (sfs) {
					SessionLogger::dbg("BRANDY") << "Labeling sent " << sentno << " prop " << i;
					std::ostringstream oss;
					propSet->getProposition(i)->dump(oss);
					SessionLogger::dbg("BRANDY") << oss.str();
					labelFeatureSet(sfs, patternMatcher, result);
				}
			}
		}
	}
}


void EntityLabelPattern::labelFeatureSet(PatternFeatureSet_ptr sfs, PatternMatcher_ptr patternMatcher,
										EntityLabelFeatureMap &entityMapping) const
{
	// IMPORTANT:
	// We were not using confidence anyway, so I have not tried to replicate it now that we are 
	// returning feature sets rather than confidences. You can check the archive
	// of this file if you want to see what it previously did.

	for (size_t i = 0; i < sfs->getNFeatures(); i++) {
		if (MentionPFeature_ptr mentionFeature = boost::dynamic_pointer_cast<MentionPFeature>(sfs->getFeature(i))) {
			if (mentionFeature->isFocusMention()) {
				Entity *ent = patternMatcher->getDocTheory()->getEntitySet()->getEntityByMention(mentionFeature->getMention()->getUID());
				if (ent != 0) {
					PatternFeatureSet_ptr existing_sfs = entityMapping[ent->getID()];
					if (!existing_sfs)
						existing_sfs = boost::make_shared<PatternFeatureSet>();
					// Keep return features only
					for (size_t j = 0; j < sfs->getNFeatures(); j++) {
						if (ReturnPatternFeature_ptr rf = boost::dynamic_pointer_cast<ReturnPatternFeature>(sfs->getFeature(j))) {
							existing_sfs->addFeature(rf);
						}					
					}
					entityMapping[ent->getID()] = existing_sfs;
				}
			}
		}
	}
}


void EntityLabelPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {
	_pattern->replaceShortcuts(refPatterns);
}
