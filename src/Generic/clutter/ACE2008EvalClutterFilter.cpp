// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/GrowableArray.h"
#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/WordConstants.h"
#include "Generic/clutter/ClutterFilter.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/RelationSet.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/clutter/ACE2008EvalClutterFilter.h"
#include "Generic/common/version.h"


Symbol PHYS_Located(L"PHYS.Located");
Symbol PHYS_Near(L"PHYS.Near");
Symbol PART_WHOLE_Geographical(L"PART-WHOLE.Geographical");
Symbol PART_WHOLE_Subsidiary(L"PART-WHOLE.Subsidiary");
Symbol PART_WHOLE_Artifact(L"PART-WHOLE.Artifact");
Symbol PER_SOC_Business(L"PER-SOC.Business");
Symbol PER_SOC_Family(L"PER-SOC.Family");
Symbol PER_SOC_Lasting_Personal(L"PER-SOC.Lasting-Personal");
Symbol ORG_AFF_Employment(L"ORG-AFF.Employment");
Symbol ORG_AFF_Ownership(L"ORG-AFF.Ownership");
Symbol ORG_AFF_Founder(L"ORG-AFF.Founder");
Symbol ORG_AFF_Student_Alum(L"ORG-AFF.Student-Alum");
Symbol ORG_AFF_Sports_Affiliation(L"ORG-AFF.Sports-Affiliation");
Symbol ORG_AFF_Investor_Shareholder(L"ORG-AFF.Investor-Shareholder");
Symbol ORG_AFF_Membership(L"ORG-AFF.Membership");
Symbol ART_User_Owner_Inventor_Manufacturer(L"ART.User-Owner-Inventor-Manufacturer");
Symbol GEN_AFF_Citizen_Resident_Religion_Ethnicity(L"GEN-AFF.Citizen-Resident-Religion-Ethnicity");
Symbol GEN_AFF_Org_Location(L"GEN-AFF.Org-Location");


std::string ACE2008EvalClutterFilter::filterName = "ACE2008EvalClutterFilter";

ACE2008EvalClutterFilter::ACE2008EvalClutterFilter () :entitySet(0), relationSet(0), numMentionSets(-1)
{

	_keepRelationsWithMultipleRelMentions = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_keep_relation_with_multiple_relmentions", true);

	// Relation Level Options
	_filterRelationsWithNonConfidentDescriptors = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_non_conf_mentions", false);
	if(_filterRelationsWithNonConfidentDescriptors)
		_filterRelationsWithNonConfidentDescriptorsRequiredConfidence = ParamReader::getRequiredFloatParam("ACE2008_clutter_remove_relations_non_conf_mentions_req_conf");

	_filterRelationsWithNonConfidentPronouns = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_non_conf_pronouns", false);
	if(_filterRelationsWithNonConfidentPronouns)
		_filterRelationsWithNonConfidentPronounsRequiredConfidence = ParamReader::getRequiredFloatParam("ACE2008_clutter_remove_relations_non_conf_pronouns_req_conf");

	_filter_PHYS_Located_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_PHYS_Located_Relations", false);
	_filter_PHYS_Near_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_PHYS_Near_Relations", false);
	_filter_PART_WHOLE_Geographical_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_PART_WHOLE_Geographical_Relations", false);
	_filter_PART_WHOLE_Subsidiary_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_PART_WHOLE_Subsidiary_Relations", false);
	_filter_PART_WHOLE_Artifact_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_PART_WHOLE_Artifact_Relations", false);
	_filter_PER_SOC_Business_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_PER_SOC_Business_Relations", false);
	_filter_PER_SOC_Family_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_PER_SOC_Family_Relations", false);
	_filter_PER_SOC_Lasting_Personal_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_PER_SOC_Lasting_Personal_Relations", false);
	_filter_ORG_AFF_Employment_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_ORG_AFF_Employment_Relations", false);
	_filter_ORG_AFF_Ownership_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_ORG_AFF_Ownership_Relations", false);
	_filter_ORG_AFF_Founder_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_ORG_AFF_Founder_Relations", false);
	_filter_ORG_AFF_Student_Alum_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_ORG_AFF_Student_Alum_Relations", false);
	_filter_ORG_AFF_Sports_Affiliation_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_ORG_AFF_Sports_Affiliation_Relations", false);
	_filter_ORG_AFF_Investor_Shareholder_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_ORG_AFF_Investor_Shareholder_Relations", false);
	_filter_ORG_AFF_Membership_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_ORG_AFF_Membership_Relations", false);
	_filter_ART_User_Owner_Inventor_Manufacturer_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_ART_User_Owner_Inventor_Manufacturer_Relations", false);
	_filter_GEN_AFF_Citizen_Resident_Religion_Ethnicity_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_GEN_AFF_Citizen_Resident_Religion_Ethnicity_Relations", false);
	_filter_GEN_AFF_Org_Location_Relations = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_filter_GEN_AFF_Org_Location_Relations", false);

	_filterRelationsWithPronLevelEntity = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_pron_lvl_entity", false);
	_filterRelationsWithPronLevelEntity2 = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_pron_lvl_entity_ver2", false);
	_filterRelationsWithPronLevelEntity3 = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_pron_lvl_entity_ver2", false);
	_filterRelationsWithPronLevelEntities = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_2_pron_lvl_entities", false);
	_filterRelationsWithPronoun = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_pronoun", false);
	_filterRelationsWithUnsureNominal = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_unsure_nominal", false);
	_filterRelationsWithPronounNameLevelEntity = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_name_level_pronoun", false);
	_filterRelationsWithPronounDescLevelEntity = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_desc_level_pronoun", false);
	_filterRelationsWith1PPronoun = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_1P_pronoun", false);
	_filterRelationsWith2PPronoun = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_2P_pronoun", false);
	_filterRelationsWith3PPronoun = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_3P_pronoun", false);
	_filterRelationsWithDescLevelEntity = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_desc_level_entity", false);
	_filterRelationsWith2DescLevelEntities = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_2_desc_level_entities", false);
	_filterRelationsWithPERPronoun = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_PER_pronoun", false);
	_filterRelationsWithPERDesc= ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_PER_descriptor", false);
	_filterRelationsWithLOCPronoun = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_LOC_pronoun", false);
	_filterRelationsWithLOCDesc = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_LOC_descriptor", false);
	_filterRelationsWithGPEPronoun = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_GPE_pronoun", false);
	_filterRelationsWithGPEDesc = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_GPE_descriptor", false);
	_filterRelationsWithFACPronoun = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_FAC_pronoun", false);
	_filterRelationsWithFACDesc = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_FAC_descriptor", false);
	_filterRelationsWithORGPronoun = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_ORG_pronoun", false);
	_filterRelationsWithORGDesc = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_ORG_descriptor", false);
	_filterRelationsWithLocTypePronoun = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_LocTypePronoun", false);
	_filterRelationsWithWHQPronoun = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_WHQ_Pronoun", false);
	_filterRelationsWithPronAndDesc = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_pron_and_desc", false);
	_filterRelationsWithGPEdescAndORGDesc = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_relations_with_desc_GPE_and_ORG", false);

	// Entity level options
	_filter1MentionEntities            = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_1_mention_entities", false);
	_filterPronounLevelEntities        = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_pronoun_lvl_entities", false);
	_filter1MentionDescLevelEntities   = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_1_desc_lvl_entities", false);

	// Mention level options
	_filterPronounMentions             = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_pronoun_mentions", false);
	_filterMentionsWithUnsureNominal   = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_mentions_with_unsure_nominal", false);
	_filter1PPronounMentions           = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_1P_pronoun_mentions", false);
	_filter2PPronounMentions           = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_2P_pronoun_mentions", false);
	_filter3PPronounMentions           = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_3P_pronoun_mentions", false);
	_filterPERPronounMentions          = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_PER_pronoun_mentions", false);
	_filterPERDescMentions             = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_PER_descriptor_mentions", false);
	_filterLOCPronounMentions          = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_LOC_pronoun_mentions", false);
	_filterLOCDescMentions             = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_LOC_descriptor_mentions", false);
	_filterGPEPronounMentions          = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_GPE_pronoun_mentions", false);
	_filterGPEDescMentions             = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_GPE_descriptor_mentions", false);
	_filterFACPronounMentions          = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_FAC_pronoun_mentions", false);
	_filterFACDescMentions             = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_FAC_descriptor_mentions", false);
	_filterORGPronounMentions          = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_ORG_pronoun_mentions", false);
	_filterORGDescMentions             = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_ORG_descriptor_mentions", false);
	_filterLocTypePronounMentions      = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_LocTypePronoun_mentions", false);
	_filterWHQPronounMentions          = ParamReader::getOptionalTrueFalseParamWithDefaultVal("ACE2008_clutter_remove_WHQ_Pronoun_mentions", false);

}

bool ACE2008EvalClutterFilter::filtered (const Mention *ment, double *score) const {
	double s (0.);
	MentionToDoubleHashMap::const_iterator p(mentionScore.find(ment->getUID()));
	if (p != mentionScore.end()) {
		s = (*p).second;
	}
	else {
		s = checkClutter(ment);
	}
	if (score != 0) *score = s;
	return s > 0.;
}

bool ACE2008EvalClutterFilter::filtered (const Entity *ent, double *score) const {
	double s (0.);
    serif::hash_map<int, double, serif::IntegerHashKey, serif::IntegerEqualKey>::const_iterator p(entityScore.find(ent->getID()));
	if (p != entityScore.end()) {
		s = (*p).second;
	}
	else {
		s = checkClutter(ent);
	}
	if (score != 0) *score = s;
	return s > 0.;
}

bool ACE2008EvalClutterFilter::filtered (const Relation *rel, double *score) const {
	double s (0.);
    serif::hash_map<int, double, serif::IntegerHashKey, serif::IntegerEqualKey>::const_iterator p(relationScore.find(rel->getID()));
	if (p != relationScore.end()) {
		s = (*p).second;
	}
	else {
		s = checkClutter (rel);
	}
	if (score != 0) *score = s;
	return s > 0.;
}

void ACE2008EvalClutterFilter::filterClutter (DocTheory *theory) {
	entitySet = theory->getEntitySet();
	relationSet = theory->getRelationSet();

	entityScore.clear();
	relationScore.clear();
	mentionScore.clear();

	mentionHandler();
	entityHandler ();
	relationHandler ();

	applyFilter ();
}

void ACE2008EvalClutterFilter::applyFilter () {
	int n_ents = entitySet->getNEntities();
	for (int i = 0; i < n_ents; ++i) {
		Entity *ent (entitySet->getEntity(i));
		/*for (int m=0; m<ent->getNMentions(); m++) {
			Mention *mention = entitySet->getMention(ent->getMention(m));
			mention->applyFilter(filterName, this);
		}
		*/
	}

	for (int i = 0; i < n_ents; ++i) {
		Entity *ent (entitySet->getEntity(i));
		ent->applyFilter(filterName, this);
	}

	if (relationSet != NULL) { // NULL happens where there are no relations
		int n_rels = relationSet->getNRelations();
		for (int i = 0; i < n_rels; i++) {
			Relation *rel = relationSet->getRelation(i);
			rel->applyFilter(filterName, this);
#if 0
		cout << "rel " << rel->getID() << " clutter? " 
			<< rel->isFiltered(ClutterFilter::filterName) << endl;
#endif
		}
	}
}

bool ACE2008EvalClutterFilter::isLOCTypePronoun (const Mention *m) const {
	return WordConstants::isLOCTypePronoun(m->getHead()->getHeadWord());
}

//#ifdef ENGLISH_LANGUAGE
bool ACE2008EvalClutterFilter::isWHQTypePronoun (const Mention *m) const {
	if (SerifVersion::isEnglish()) {
		return WordConstants::isWHQPronoun(m->getHead()->getHeadWord());
	} else {
		return false;
	}
}
//#endif

bool ACE2008EvalClutterFilter::is_matched (const Entity *e, const wchar_t *s) const {
	return ::wcscmp(e->getType().getName().to_string(), s) == 0;
}

bool ACE2008EvalClutterFilter::isDesc (const Mention *m) const {
	switch (m->getMentionType()) {
	case Mention::DESC:
		return true;
	case Mention::LIST:
		return isDesc (m->getChild());
    default:
        return false;
	}
}

bool ACE2008EvalClutterFilter::isNam (const Mention *m) const {
	switch (m->getMentionType()) {
	case Mention::NAME:
		return true;
	case Mention::LIST:
		return isNam (m->getChild());
    default:
            return false;
	}
}

bool ACE2008EvalClutterFilter::isDescLevelEntity (const Entity *ent) const {
	for (int i (0); i < ent->mentions.length(); ++i) {
		Mention *m (entitySet->getMention(ent->mentions[i]));
		if (m->getMentionType() == Mention::NAME) {
			return false;
		}
	}
	return true;
}

bool ACE2008EvalClutterFilter::isPronLevelEntity (const Entity *ent) const {
	for (int i (0); i < ent->mentions.length(); ++i) {
		Mention *m (entitySet->getMention(ent->mentions[i]));
		if (m->getMentionType() == Mention::NAME || m->getMentionType() == Mention::DESC) {
			return false;
		}
	}
	return true;
}

bool ACE2008EvalClutterFilter::isPronLevelEntity3 (const Entity *ent) const {
	for (int i (0); i < ent->mentions.length(); ++i) {
		Mention *m (entitySet->getMention(ent->mentions[i]));
		if (m->getMentionType() == Mention::NAME || m->getMentionType() == Mention::DESC || m->getMentionType() == Mention::LIST) {
			return false;
		}
	}
	return true;
}

bool ACE2008EvalClutterFilter::isPronLevelEntity2 (const Entity *ent) const {
	for (int i (0); i < ent->mentions.length(); ++i) {
		Mention *m (entitySet->getMention(ent->mentions[i]));
		if (m->getMentionType() != Mention::PRON) {
			return false;
		}
	}
	return true;
}

bool ACE2008EvalClutterFilter::isNameLevelEntity(const Entity *ent) const {
	return isNam(ent);
}

bool ACE2008EvalClutterFilter::isNam (const Entity *ent) const {
	for (int i (0); i < ent->mentions.length(); ++i) {
		Mention *m (entitySet->getMention(ent->mentions[i]));
		if (isNam (m)) {
			return true;
		}
	}
	return false;
}

bool ACE2008EvalClutterFilter::isRef (const Entity *ent) const {
	int n_rels = relationSet->getNRelations();
	int nrefs (0), clutters (0);
	for (int i = 0; i < n_rels; i++) {
		Relation *rel = relationSet->getRelation(i);		
		if ((ent->getID() == rel->getLeftEntityID()
			|| ent->getID() == rel->getRightEntityID())) {
			if (checkClutter (rel) > 0.) {
				++clutters;
			}
			++nrefs;
		}
	}
#if 0
	if (ent->getID() == 2) {
		cout << "nrefs = " << nrefs << " clutters = " << clutters << endl;
	}
#endif
	return clutters < nrefs;
}


double ACE2008EvalClutterFilter::checkClutter (const Relation *rel) const {

	Entity *left (entitySet->getEntity(rel->getLeftEntityID()));
	Entity *right (entitySet->getEntity(rel->getRightEntityID()));

	if (left->isGeneric() || right->isGeneric())
		return 1.;
	// if one of its entities has been filtered
	if ((entityScore.get(left->getID())!=NULL && *entityScore.get(left->getID())>0.0)
			|| (entityScore.get(right->getID())!=NULL && *entityScore.get(right->getID())>0.0))
		return 1.;

	// this doesn't work
	if (_filterRelationsWithPronLevelEntities && (isPronLevelEntity(left) && isPronLevelEntity(right)))
		return 1.;

	if (_filterRelationsWithPronLevelEntity && (isPronLevelEntity(left) || isPronLevelEntity(right)))
		return 1.;
	if (_filterRelationsWithPronLevelEntity2 && (isPronLevelEntity2(left) || isPronLevelEntity2(right)))
		return 1.;
	if (_filterRelationsWithPronLevelEntity3 && (isPronLevelEntity3(left) || isPronLevelEntity3(right)))
		return 1.;

	if (_filterRelationsWithDescLevelEntity && (isDescLevelEntity(left) || isDescLevelEntity(right)))
		return 1.;
	if (_filterRelationsWith2DescLevelEntities && (isDescLevelEntity(left) || isDescLevelEntity(right)))
		return 1.;


	// Checking for different Relation types
	Symbol type = rel->getType();
	if (_filter_PHYS_Located_Relations && type == PHYS_Located)
		return 1.;
	if (_filter_PHYS_Near_Relations && type == PHYS_Near)
		return 1.;
	if (_filter_PART_WHOLE_Geographical_Relations && type == PART_WHOLE_Geographical)
		return 1.;
	if (_filter_PART_WHOLE_Subsidiary_Relations && type == PART_WHOLE_Subsidiary)
		return 1.;
	if (_filter_PART_WHOLE_Artifact_Relations && type == PART_WHOLE_Artifact)
		return 1.;
	if (_filter_PER_SOC_Business_Relations && type == PER_SOC_Business)
		return 1.;
	if (_filter_PER_SOC_Family_Relations && type == PER_SOC_Family)
		return 1.;
	if (_filter_PER_SOC_Lasting_Personal_Relations && type == PER_SOC_Lasting_Personal)
		return 1.;
	if (_filter_ORG_AFF_Employment_Relations && type == ORG_AFF_Employment)
		return 1.;
	if (_filter_ORG_AFF_Ownership_Relations && type == ORG_AFF_Ownership)
		return 1.;
	if (_filter_ORG_AFF_Founder_Relations && type == ORG_AFF_Founder)
		return 1.;
	if (_filter_ORG_AFF_Student_Alum_Relations && type == ORG_AFF_Student_Alum)
		return 1.;
	if (_filter_ORG_AFF_Sports_Affiliation_Relations && type == ORG_AFF_Sports_Affiliation)
		return 1.;
	if (_filter_ORG_AFF_Investor_Shareholder_Relations && type == ORG_AFF_Investor_Shareholder)
		return 1.;
	if (_filter_ORG_AFF_Membership_Relations && type == ORG_AFF_Membership)
		return 1.;
	if (_filter_ART_User_Owner_Inventor_Manufacturer_Relations && type == ART_User_Owner_Inventor_Manufacturer)
		return 1.;
	if (_filter_GEN_AFF_Citizen_Resident_Religion_Ethnicity_Relations && type == GEN_AFF_Citizen_Resident_Religion_Ethnicity)
		return 1.;
	if (_filter_GEN_AFF_Org_Location_Relations && type == GEN_AFF_Org_Location)
		return 1.;



	bool good_mention_found = false;
	const Relation::LinkedRelMention *relMentions = rel->getMentions();
	bool hasMultipleRelMention = false;
	bool firstRelMention = true;
	while (relMentions != 0) {
		bool good_mention = true;
		RelMention *relMention = relMentions->relMention;
		const Mention *leftMention = relMention->getLeftMention();
		const Mention *rightMention = relMention->getRightMention();


		if (_filterRelationsWithNonConfidentDescriptors &&
			((isDesc(leftMention) && leftMention->getLinkConfidence()<_filterRelationsWithNonConfidentDescriptorsRequiredConfidence)
			|| (isDesc(rightMention) && rightMention->getLinkConfidence()<_filterRelationsWithNonConfidentDescriptorsRequiredConfidence)))
		{
				good_mention = false;
		}
		if (_filterRelationsWithNonConfidentPronouns &&
			((isPron(leftMention) && leftMention->getLinkConfidence()<_filterRelationsWithNonConfidentPronounsRequiredConfidence)
			|| (isPron(rightMention) && rightMention->getLinkConfidence()<_filterRelationsWithNonConfidentPronounsRequiredConfidence)))
		{
				good_mention = false;
		}

		// if one of its mentions has been filtered
		if ((mentionScore.get(leftMention->getUID())!=NULL && *mentionScore.get(leftMention->getUID())>0.0)
			|| (mentionScore.get(rightMention->getUID())!=NULL && *mentionScore.get(rightMention->getUID())>0.0)){
			good_mention = false;
		}

		if (_filterRelationsWithPronoun && (isPron(leftMention) || isPron(rightMention))){
			good_mention = false;
		}
		if (_filterRelationsWithPronounNameLevelEntity && ((isPron(leftMention) && isNameLevelEntity(left)) || (isPron(rightMention) && isNameLevelEntity(right)))) {
			good_mention = false;
		}
		if (_filterRelationsWithPronounDescLevelEntity && ((isPron(leftMention) && isDescLevelEntity(left)) || (isPron(rightMention) && isDescLevelEntity(right)))) {
			good_mention = false;
		}
		if (_filterRelationsWith1PPronoun && (leftMention->is1pPronoun() || rightMention->is1pPronoun())) {
			good_mention = false;
		}
		if (_filterRelationsWith2PPronoun && (leftMention->is2pPronoun() || rightMention->is2pPronoun())) {
			good_mention = false;
		}
		if (_filterRelationsWith3PPronoun && (leftMention->is3pPronoun() || rightMention->is3pPronoun())) {
			good_mention = false;
		}
		if (_filterRelationsWithPERPronoun && ((isPron(leftMention) && isPER(left)) || (isPron(rightMention) && isPER(right)))) {
			good_mention = false;
		}
		if (_filterRelationsWithPERDesc && ((isDesc(leftMention) && isPER(left)) || (isDesc(rightMention) && isPER(right)))) {
			good_mention = false;
		}
		if (_filterRelationsWithLOCPronoun && ((isPron(leftMention) && isLOC(left)) || (isPron(rightMention) && isLOC(right)))) {
			good_mention = false;
		}
		if (_filterRelationsWithLOCDesc && ((isDesc(leftMention) && isLOC(left)) || (isDesc(rightMention) && isLOC(right)))) {
			good_mention = false;
		}
		if (_filterRelationsWithGPEPronoun && ((isPron(leftMention) && isGPE(left)) || (isPron(rightMention) && isGPE(right)))) {
			good_mention = false;
		}
		if (_filterRelationsWithGPEDesc && ((isDesc(leftMention) && isGPE(left)) || (isDesc(rightMention) && isGPE(right)))) {
			good_mention = false;
		}
		if (_filterRelationsWithFACPronoun && ((isPron(leftMention) && isFAC(left)) || (isPron(rightMention) && isFAC(right)))) {
			good_mention = false;
		}
		if (_filterRelationsWithFACDesc && ((isDesc(leftMention) && isFAC(left)) || (isDesc(rightMention) && isFAC(right)))) {
			good_mention = false;
		}
		if (_filterRelationsWithORGPronoun && ((isPron(leftMention) && isORG(left)) || (isPron(rightMention) && isORG(right)))) {
			good_mention = false;
		}
		if (_filterRelationsWithORGDesc && ((isDesc(leftMention) && isORG(left)) || (isDesc(rightMention) && isORG(right)))) {
			good_mention = false;
		}
		if (_filterRelationsWithLocTypePronoun && (isLOCTypePronoun(leftMention) || isLOCTypePronoun(rightMention))) {
			good_mention = false;
		}
//#ifdef ENGLISH_LANGUAGE
		if (SerifVersion::isEnglish()) {
			if (_filterRelationsWithWHQPronoun && (isWHQTypePronoun(leftMention) || isWHQTypePronoun(rightMention))) {
				good_mention = false;
			}
		}
//#endif
		if (_filterRelationsWithPronAndDesc && ((isPron(leftMention) && isDesc(rightMention)) || (isPron(rightMention) && isDesc(leftMention)))) {
			good_mention = false;
		}
		if (_filterRelationsWithGPEdescAndORGDesc && (((isDesc(leftMention) || isDesc(rightMention)) && isGPE(left) && isORG(right)))) {
			good_mention = false;
		}
		
		if (!firstRelMention)
			hasMultipleRelMention = true;
		else
			firstRelMention = false;
		if (good_mention)
			good_mention_found = true;

		relMentions = relMentions->next;
	}

	return (good_mention_found 
		|| (_keepRelationsWithMultipleRelMentions && hasMultipleRelMention)) 
		?  0. : 1.;
}




double ACE2008EvalClutterFilter::checkClutter (const Mention *ment) const {
	//// Mention level options
	if (_filterPronounMentions && isPron(ment))
		return 1.0;
	if (_filter1PPronounMentions && ment->is1pPronoun())
		return 1.0;
	if (_filter2PPronounMentions && ment->is2pPronoun())
		return 1.0;
	if (_filter3PPronounMentions && ment->is3pPronoun())
		return 1.0;

	if (_filterPERPronounMentions && isPron(ment) && isPER(ment))
		return 1.0;
	if (_filterPERDescMentions && isDesc(ment) && isPER(ment))
		return 1.0;

	if (_filterLOCPronounMentions && isPron(ment) && isLOC(ment))
		return 1.0;
	if (_filterLOCDescMentions && isDesc(ment) && isLOC(ment))
		return 1.0;

	if (_filterGPEPronounMentions && isPron(ment) && isGPE(ment))
		return 1.0;
	if (_filterGPEDescMentions && isDesc(ment) && isGPE(ment))
		return 1.0;

	if (_filterFACPronounMentions && isPron(ment) && isFAC(ment))
		return 1.0;
	if (_filterFACDescMentions && isDesc(ment) && isFAC(ment))
		return 1.0;

	if (_filterORGPronounMentions && isPron(ment) && isORG(ment))
		return 1.0;
	if (_filterORGDescMentions && isDesc(ment) && isORG(ment))
		return 1.0;

	if (_filterLocTypePronounMentions && isLOCTypePronoun(ment) )
		return 1.0;
//#ifdef ENGLISH_LANGUAGE
	if (SerifVersion::isEnglish()) {
		if (_filterWHQPronounMentions && isWHQTypePronoun(ment))
			return 1.0;
	}
//#endif
	return 0;
}




double ACE2008EvalClutterFilter::checkClutter (const Entity *ent) const {
	int nValidMentions = 0;
	int nValidDescMentions = 0;
	int nValidNameMentions = 0;
	for (int i=0; i<ent->getNMentions(); i++) {
		MentionUID mentUID = ent->getMention(i);
		const Mention *ment = entitySet->getMention(mentUID);
		if (mentionScore.get(mentUID) != NULL && *mentionScore.get(mentUID) > 0.0)
			continue;
		nValidMentions++;
		if (isDesc(ment))
			nValidDescMentions++;
		if (isNam(ment))
			nValidNameMentions++;
	}
	int nValidDescLevelMentions = nValidDescMentions + nValidNameMentions;
	int nValidNameLevelMentions = nValidNameMentions;


	if (_filterPronounLevelEntities && nValidDescLevelMentions ==0)
		return 1.;
	if (_filter1MentionDescLevelEntities && nValidDescLevelMentions <2)
		return 1.;
	if (_filter1MentionEntities && nValidMentions < 2)
		return 1.;
	return (nValidMentions > 0) ? 0. : 1.;
}

void ACE2008EvalClutterFilter::relationHandler () {
	if(!relationSet) // this happens when there is no text in the document or maybe no relations
		return;
	int n_rels = relationSet->getNRelations();
	for (int i = 0; i < n_rels; i++) {
		Relation *rel = relationSet->getRelation(i);
		relationScore[rel->getID()] = checkClutter (rel);
	}
}

void ACE2008EvalClutterFilter::entityHandler () {
	int n_ents = entitySet->getNEntities();
	for (int i = 0; i < n_ents; ++i) {
		Entity *ent (entitySet->getEntity(i));
		entityScore[ent->getID()] = checkClutter (ent);
	}
}

void ACE2008EvalClutterFilter::mentionHandler () {
	for (int i = 0; i < entitySet->getNEntities(); ++i) {
		Entity *ent (entitySet->getEntity(i));
		for (int m=0; m<ent->getNMentions(); m++) {
			const Mention *mention = entitySet->getMention(ent->getMention(m));
			mentionScore[mention->getUID()] = checkClutter (mention);
		}
	}
}


bool ACE2008EvalClutterFilter::isPER(const Mention *ment) const {
	return ment->getEntityType() == EntityType::getPERType();
}

bool ACE2008EvalClutterFilter::isLOC(const Mention *ment) const {
	return ment->getEntityType() == EntityType::getLOCType();
}

bool ACE2008EvalClutterFilter::isGPE(const Mention *ment) const {
	return ment->getEntityType() == EntityType::getGPEType();
}

bool ACE2008EvalClutterFilter::isORG(const Mention *ment) const {
	return ment->getEntityType() == EntityType::getORGType();
}

bool ACE2008EvalClutterFilter::isFAC(const Mention *ment) const {
	return ment->getEntityType() == EntityType::getFACType();
}

bool ACE2008EvalClutterFilter::isWEA(const Mention *ment) const {
	return ment->getEntityType().getName() == Symbol(L"WEA");
}

bool ACE2008EvalClutterFilter::isVEH(const Mention *ment) const {
	return ment->getEntityType().getName() == Symbol(L"VEH");
}

