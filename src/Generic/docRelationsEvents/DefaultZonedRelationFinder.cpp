// Copyright 2015 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/docRelationsEvents/DefaultZonedRelationFinder.h"
#include "Generic/common/InputUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/RelMentionSet.h"

#include <boost/make_shared.hpp>
#include <boost/unordered_map.hpp>

DefaultZonedRelationFinder::DefaultZonedRelationFinder() : _maxMentionSpacing(0) {
	// Read the region tag file, if specified
	std::string pattern_file = ParamReader::getParam("zoned_relation_finder_patterns");
	if (!pattern_file.empty()) {
		std::set<std::vector<std::wstring> > patterns = InputUtil::readColumnFileIntoSet(pattern_file, false, L"\t");
		for (std::set<std::vector<std::wstring> >::const_iterator pattern_i = patterns.begin(); pattern_i != patterns.end(); ++pattern_i) {
			// Check the file format
			if (pattern_i->size() != 7) {
				std::stringstream err;
				err << "Expected 7 columns definining relation pattern, found " << pattern_i->size();
				throw UnexpectedInputException("DefaultZonedRelationFinder::DefaultZonedRelationFinder()", err.str().c_str());
			}

			// Read and store each mention matcher
			ZonedMentionMatcher_ptr leftMatcher = boost::make_shared<ZonedMentionMatcher>();
			leftMatcher->regionTag = Symbol(pattern_i->at(1));
			leftMatcher->entityType = EntityType(Symbol(pattern_i->at(2)));
			leftMatcher->mentionType = Mention::getTypeFromString(pattern_i->at(3).c_str());
			_mentionMatchers.insert(leftMatcher);
			ZonedMentionMatcher_ptr rightMatcher = boost::make_shared<ZonedMentionMatcher>();
			rightMatcher->regionTag = Symbol(pattern_i->at(4));
			rightMatcher->entityType = EntityType(Symbol(pattern_i->at(5)));
			rightMatcher->mentionType = Mention::getTypeFromString(pattern_i->at(6).c_str());
			_mentionMatchers.insert(rightMatcher);

			// Read and store the relation matcher
			ZonedRelationMatcher_ptr relationMatcher = boost::make_shared<ZonedRelationMatcher>();
			relationMatcher->relationType = Symbol(pattern_i->at(0));
			relationMatcher->leftMention = leftMatcher;
			relationMatcher->rightMention = rightMatcher;
			_relationMatchers.insert(relationMatcher);
		}
		SessionLogger::dbg("zoned-relations") << "Loaded " << patterns.size() << " zoned relation finder patterns into " << _mentionMatchers.size() << " mention matchers and " << _relationMatchers.size() << " relation matchers\n";
	}

	// Read the spacing threshold
	_maxMentionSpacing = ParamReader::getOptionalIntParamWithDefaultValue("zoned_relation_finder_max_mention_spacing", 0);
}

RelMentionSet* DefaultZonedRelationFinder::findRelations(DocTheory* docTheory) {
	// Return immediately if no patterns loaded
	if (_relationMatchers.size() == 0)
		return _new RelMentionSet();

	// Get the regions and set up the container for matches
	int nRegions = docTheory->getDocument()->getNRegions();
	boost::unordered_map<ZonedMentionMatcher_ptr, std::vector<std::vector<Mention*> > > mentionsByMatcher;
	for (boost::unordered_set<ZonedMentionMatcher_ptr>::const_iterator matcher_i = _mentionMatchers.begin(); matcher_i != _mentionMatchers.end(); ++matcher_i) {
		std::vector<std::vector<Mention*> > mentionsByRegion(nRegions);
		mentionsByMatcher.insert(std::make_pair(*matcher_i, mentionsByRegion));
	}
	const Region* const* regions = docTheory->getDocument()->getRegions();

	// Collect all the matching mentions by region
	for (int r = 0; r < nRegions; r++) {
		for (boost::unordered_set<ZonedMentionMatcher_ptr>::const_iterator matcher_i = _mentionMatchers.begin(); matcher_i != _mentionMatchers.end(); ++matcher_i) {
			if (regions[r]->getRegionTag() == (*matcher_i)->regionTag) {
				// Loop through the sentences but only consider the ones in this matching region
				int previousMentionEnd = 0;
				int doneWithRegion = false;
				for (int s = 0; s < docTheory->getNSentences(); s++) {
					if (docTheory->getSentence(s)->getRegion() == regions[r]) {
						// Check each mention in this sentence against this matcher
						MentionSet* mentions = docTheory->getSentenceTheory(s)->getMentionSet();
						TokenSequence* tokens = docTheory->getSentenceTheory(s)->getTokenSequence();
						for (int m = 0; m < mentions->getNMentions(); m++) {
							Mention* mention = mentions->getMention(m);
							if (mention->getMentionType() == (*matcher_i)->mentionType && mention->getEntityType() == (*matcher_i)->entityType) {
								int mentionSpacing = tokens->getToken(mention->getNode()->getStartToken())->getStartCharOffset().value() - previousMentionEnd;
								if (_maxMentionSpacing > 0 && previousMentionEnd > 0 && mentionSpacing > _maxMentionSpacing) {
									SessionLogger::dbg("zoned-relations") << "Ignored matching " << mention->getEntityType().getName() << "." << Mention::getTypeString(mention->getMentionType()) << " in "
										<< regions[r]->getRegionTag() << " region because " << mentionSpacing << " characters since previous mention exceeds max " << _maxMentionSpacing << "\n";
									doneWithRegion = true;
									break;
								} else {
									previousMentionEnd = tokens->getToken(mention->getNode()->getEndToken())->getEndCharOffset().value();
									mentionsByMatcher.find(*matcher_i)->second.at(r).push_back(mention);
									SessionLogger::dbg("zoned-relations") << "Matched " << mention->getEntityType().getName() << "." << Mention::getTypeString(mention->getMentionType()) << " in " << regions[r]->getRegionTag() << " region\n";
								}
							}
						}
						if (doneWithRegion)
							break;
					}
				}	
			}
		}
	}

	// Create relations between matching mentions
	RelMentionSet* relations = _new RelMentionSet();
	for (boost::unordered_set<ZonedRelationMatcher_ptr>::const_iterator matcher_i = _relationMatchers.begin(); matcher_i != _relationMatchers.end(); ++matcher_i) {
		// If the mention matchers specify the same region tag, only build relations within the same region
		if ((*matcher_i)->leftMention->regionTag == (*matcher_i)->rightMention->regionTag) {
			// Check each region
			for (int r = 0; r < nRegions; r++) {
				// Gather all of the previously matched left and right mentions that are contained in the same region
				std::vector<Mention*> leftMentionsByRegion = mentionsByMatcher.find((*matcher_i)->leftMention)->second.at(r);
				std::vector<Mention*> rightMentionsByRegion = mentionsByMatcher.find((*matcher_i)->rightMention)->second.at(r);
				createRelations(relations, (*matcher_i)->relationType, docTheory->getNSentences(), leftMentionsByRegion, rightMentionsByRegion);
			}
		} else {
			// Check all possible pairs of regions
			for (int leftRegion = 0; leftRegion < nRegions; leftRegion++) {
				for (int rightRegion = 0; rightRegion < nRegions; rightRegion++) {
					// Gather all of the previously matched left and right mentions by region
					std::vector<Mention*> leftMentionsByRegion = mentionsByMatcher.find((*matcher_i)->leftMention)->second.at(leftRegion);
					std::vector<Mention*> rightMentionsByRegion = mentionsByMatcher.find((*matcher_i)->rightMention)->second.at(rightRegion);
					createRelations(relations, (*matcher_i)->relationType, docTheory->getNSentences(), leftMentionsByRegion, rightMentionsByRegion);
				}
			}
		}
	}

	// Done
	return relations;
}

void DefaultZonedRelationFinder::createRelations(RelMentionSet* relations, Symbol relationType, int nSentences, std::vector<Mention*> & leftMentions, std::vector<Mention*> & rightMentions) {
	// Loop over all possible pairs between the specified mentions and create relations between them of the specified type
	for (std::vector<Mention*>::const_iterator leftMention_i = leftMentions.begin(); leftMention_i != leftMentions.end(); ++leftMention_i) {
		for (std::vector<Mention*>::const_iterator rightMention_i = rightMentions.begin(); rightMention_i != rightMentions.end(); ++rightMention_i) {
			// Don't allow reflexive relations
			if (*leftMention_i == *rightMention_i)
				continue;
			RelMention* relation = _new RelMention(*leftMention_i, *rightMention_i, relationType, nSentences, relations->getNRelMentions(), 1.0);
			if (SessionLogger::dbg_or_msg_enabled("zoned-relations")) {
				SessionLogger::dbg("zoned-relations") << "Created " << relationType << " relation\n";
			}
			relations->takeRelMention(relation);
		}
	}
}
