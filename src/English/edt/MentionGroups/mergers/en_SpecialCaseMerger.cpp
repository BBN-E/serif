// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/edt/MentionGroups/mergers/en_SpecialCaseMerger.h"
#include "Generic/edt/MentionGroups/mergers/CompositeMentionGroupMerger.h"

#include "English/edt/MentionGroups/mergers/en_AliasMerger.h"
#include "English/edt/MentionGroups/mergers/en_BodyMerger.h"
#include "English/edt/MentionGroups/mergers/en_GovernmentMerger.h"
#include "English/edt/MentionGroups/mergers/en_GPELocDescToNameMerger.h"
#include "English/edt/MentionGroups/mergers/en_NationalityPeopleMerger.h"
#include "English/edt/MentionGroups/mergers/en_OrgNameDescMerger.h"
#include "English/edt/MentionGroups/mergers/en_RelativePronounMerger.h"
#include "English/edt/MentionGroups/mergers/en_WHQCopulaMerger.h"

EnglishSpecialCaseMerger::EnglishSpecialCaseMerger(MentionGroupConstraint_ptr constraints) : 
	CompositeMentionGroupMerger(Symbol(L"EnglishSpecialCase"))
{
	this->add(MentionGroupMerger_ptr(_new EnglishAliasMerger(constraints)));
	this->add(MentionGroupMerger_ptr(_new EnglishRelativePronounMerger(constraints)));
	this->add(MentionGroupMerger_ptr(_new EnglishBodyMerger(constraints)));
	this->add(MentionGroupMerger_ptr(_new EnglishGPELocDescToNameMerger(constraints)));
	this->add(MentionGroupMerger_ptr(_new EnglishGovernmentMerger(constraints)));
	this->add(MentionGroupMerger_ptr(_new EnglishOrgNameDescMerger(constraints)));
	this->add(MentionGroupMerger_ptr(_new EnglishWHQCopulaMerger(constraints)));
	this->add(MentionGroupMerger_ptr(_new EnglishNationalityPeopleMerger(constraints)));
}
