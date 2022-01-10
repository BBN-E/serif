// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/rawrelations/RawRelationFinder.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/EntityType.h"


// please forgive me
static Symbol sym_of(L"of");


RelMention *RawRelationFinder::getRawRelMention(Proposition *prop,
												const MentionSet *mentionSet,
												int sent_no, int rel_no)
{
	if (prop->getPredType() == Proposition::VERB_PRED) {
		const Mention *sub = prop->getMentionOfRole(Argument::SUB_ROLE,
													mentionSet);
		const Mention *obj = prop->getMentionOfRole(Argument::OBJ_ROLE,
													mentionSet);
		if (isWorthwhileMention(sub) && isWorthwhileMention(obj)) {
			return _new RelMention(sub, obj, prop->getPredSymbol(),
								   sent_no, rel_no);
		}
		else {
			return 0;
		}
	}
	else if (prop->getPredType() == Proposition::NOUN_PRED) {
		// leaving these out for 11/6 factbrowser delivery -- SRS
		return 0;
	}
	else if (prop->getPredType() == Proposition::MODIFIER_PRED) {
		const Mention *ref = prop->getMentionOfRole(Argument::REF_ROLE,
													mentionSet);
		if (!isWorthwhileMention(ref))
			return 0;
		if (prop->getNArgs() > 1) {
			Argument *arg = prop->getArg(1);
			if (arg->getType() == Argument::MENTION_ARG &&
				arg->getRoleSym().to_string()[0] != L'<' &&
				isWorthwhileMention(arg->getMention(mentionSet)))
			{
				if (arg->getRoleSym() == sym_of)
					return 0;

				wchar_t predicate[250];
				wcscpy(predicate, L"is ");
				wcsncat(predicate, prop->getPredSymbol().to_string(), 100);

				if (prop->getPredSymbol() != arg->getRoleSym()) {
					wcscat(predicate, L" ");
					wcsncat(predicate, arg->getRoleSym().to_string(), 100);
				}
				
				return _new RelMention(ref, arg->getMention(mentionSet),
										   Symbol(predicate), sent_no, rel_no);
			}
		}

		return 0;
	}

	return 0;
}

bool RawRelationFinder::isWorthwhileMention(const Mention *mention) {
	if (mention == 0)
		return false;

	EntityType type = mention->getEntityType();
	return (type.matchesPER() ||
			type.matchesORG() ||
			type.matchesGPE() ||
			type.matchesFAC() ||
			type.matchesLOC());
}

