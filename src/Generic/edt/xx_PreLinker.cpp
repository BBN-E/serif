// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/edt/xx_PreLinker.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Argument.h"
#include "Generic/common/version.h"


bool GenericPreLinker::LINK_SPECIAL_CASES = false;
bool GenericPreLinker::FILTER_BY_ENTITY_SUBTYPE = false;

void GenericPreLinker::setSpecialCaseLinkingSwitch(bool linking_switch) {
	LINK_SPECIAL_CASES = linking_switch;
}

void GenericPreLinker::setEntitySubtypeFilteringSwitch(bool filtering_switch) {
	FILTER_BY_ENTITY_SUBTYPE = filtering_switch;
}

void GenericPreLinker::preLinkAppositives(PreLinker::MentionMap &preLinks,
										  const MentionSet *mentionSet)
{
	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		const Mention *mention = mentionSet->getMention(i);

		if (mention->getMentionType() == Mention::APPO) {
			const Mention *child1 = mention->getChild();
			if (!child1)
				continue;
			const Mention *child2 = child1->getNext();
			if (!child2)
				continue;

			if (FILTER_BY_ENTITY_SUBTYPE && !subtypeMatch(child1, child2))
				continue;

			//#ifdef CHINESE_LANGUAGE
			if (SerifVersion::isChinese()) {
				if (child1->getMentionType() == Mention::LIST || 
					child2->getMentionType() == Mention::LIST)
					continue;
			}
			//#endif

			if (child1->isOfRecognizedType() || child2->isOfRecognizedType()) {
				if (child2->getMentionType() == Mention::NAME) {
					// have the appos and the other half of the appos pre-link
					// to child2
					preLinks[i] = child2;
					preLinks[child1->getIndex()] = child2;
				}
				else {
					// pre-link to child1
					preLinks[i] = child1;
					preLinks[child2->getIndex()] = child1;
					if (child2->getMentionType() == Mention::LIST) {
						Mention *grandchild = child2->getChild();
						while (grandchild != NULL) {
							if (grandchild->isOfRecognizedType())
								preLinks[grandchild->getIndex()] = child1;
							grandchild = grandchild->getNext();
						}
					}
				}
			}
		}
	}
}

void GenericPreLinker::preLinkCopulas(PreLinker::MentionMap &preLinks,
									  const MentionSet *mentionSet,
									  const PropositionSet *propSet)
{
	for (int i = 0; i < propSet->getNPropositions(); i++) {
		Proposition *prop = propSet->getProposition(i);

		if (prop->getPredType() == Proposition::COPULA_PRED) {
			
			if (prop->getNegation() != 0)
				continue;

			if (prop->getNArgs() < 2 ||
				prop->getArg(0)->getType() != Argument::MENTION_ARG ||
				prop->getArg(1)->getType() != Argument::MENTION_ARG)
			{
				continue;
			}

			const Mention *lhs = prop->getArg(0)->getMention(mentionSet);
			const Mention *rhs = prop->getArg(1)->getMention(mentionSet);

			if (lhs->getMentionType() == Mention::NAME && rhs->getMentionType() == Mention::NAME) {
				continue;
			}

			if (FILTER_BY_ENTITY_SUBTYPE && !subtypeMatch(lhs, rhs))
				continue;

			if (lhs->isOfRecognizedType() && rhs->isOfRecognizedType()) {

				if (rhs->getMentionType() == Mention::LIST) {
					//#ifndef CHINESE_LANGUAGE 
					if (! SerifVersion::isChinese()) {
						Mention *child = rhs->getChild();
						while (child != 0) {
							createPreLink(preLinks, lhs, child);
							child = child->getNext();
						}
					}
					//#endif
				} else {
					createPreLink(preLinks, lhs, rhs);
				}
			}
		}
	}
}

void GenericPreLinker::createPreLink(PreLinker::MentionMap &preLinks,
									 const Mention *m1, const Mention *m2)
{
	// Because partitives and pronouns are bad linkers, they want to 
	// pre-link to something else, whereas names want the other mention
	// to pre-link to them:
	if (m1->getMentionType() == Mention::PART ||
		m1->getMentionType() == Mention::PRON)
	{
		preLinks[m1->getIndex()] = m2;
	}
	else if (m2->getMentionType() == Mention::PART ||
			 m2->getMentionType() == Mention::PRON)
	{
		preLinks[m2->getIndex()] = m1;
	}
	else if (m1->getMentionType() == Mention::NAME) {
		preLinks[m2->getIndex()] = m1;
	}
	else if (m2->getMentionType() == Mention::NAME) {
		preLinks[m1->getIndex()] = m2;
	}
	else {
		// no rule applies, so just link the second thing to the first
		// by default.
		preLinks[m2->getIndex()] = m1;
	}
}

bool GenericPreLinker::subtypeMatch(const Mention *ment1, const Mention *ment2) {
	EntitySubtype subtype1 = ment1->getEntitySubtype();
	EntitySubtype subtype2 = ment2->getEntitySubtype();

	if (subtype1.isDetermined() && subtype2.isDetermined() && subtype1 != subtype2)
		return false;

	return true;
}


const int GenericPreLinker::postLinkSpecialCaseNames(const EntitySet* entitySet, 
													 int* entity_link_pairs, int maxpairs) {
	return 0;
}
void GenericPreLinker::preLinkSpecialCases(PreLinker::MentionMap &preLinks,
								const MentionSet *mentionSet,
								const PropositionSet *propSet)
{}

void GenericPreLinker::preLinkTitleAppositives(PreLinker::MentionMap &preLinks,
											   const MentionSet *mentionSet) 
{}


