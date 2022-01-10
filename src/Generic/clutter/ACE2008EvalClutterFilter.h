// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACE2008EVALCLUTTERFILTER
#define ACE2008EVALCLUTTERFILTER

#include "Generic/clutter/ClutterFilter.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/hash_map.h"

class MentionSet;
class RelationSet;
class EntitySet;
class DocTheory;

/* this class was copied from en_ATEAInternamClutterFilter
 * in ACE2008 eval we want to filter some relations because the scorer
 * now penelizes for Relations where one of the mentions is not correctly
 * linked to the correct entity (according to the global alignment)
*/
class ACE2008EvalClutterFilter: public ClutterFilter
	, public EntityClutterFilter, public RelationClutterFilter {
public:
	static std::string filterName;

	ACE2008EvalClutterFilter ();
	~ACE2008EvalClutterFilter () {};

	bool filtered (const Mention *ment, double *score = 0) const;
	bool filtered (const Entity *ent, double *score = 0) const;
	bool filtered (const Relation *rel, double *score = 0) const;

private:
    typedef serif::hash_map<MentionUID, double, MentionUID::HashOp, MentionUID::EqualOp> MentionToDoubleHashMap;
	MentionToDoubleHashMap mentionScore;
	serif::hash_map<int, double, serif::IntegerHashKey, serif::IntegerEqualKey> entityScore;
	serif::hash_map<int, double, serif::IntegerHashKey, serif::IntegerEqualKey> relationScore;

	EntitySet *entitySet;
	RelationSet *relationSet;
	int numMentionSets;
	MentionSet* mentionSets[MAX_DOCUMENT_SENTENCES];


	// Entity options
	bool _filter1MentionEntities;
	bool _filterPronounLevelEntities;
	bool _filter1MentionDescLevelEntities;

	// Mention options
	bool _filterPronounMentions;
	bool _filterMentionsWithUnsureNominal;
	bool _filter1PPronounMentions;
	bool _filter2PPronounMentions;
	bool _filter3PPronounMentions;
	bool _filterPERPronounMentions;
	bool _filterPERDescMentions;
	bool _filterLOCPronounMentions;
	bool _filterLOCDescMentions;
	bool _filterGPEPronounMentions;
	bool _filterGPEDescMentions;
	bool _filterFACPronounMentions;
	bool _filterFACDescMentions;
	bool _filterORGPronounMentions;
	bool _filterORGDescMentions;
	bool _filterLocTypePronounMentions;
	bool _filterWHQPronounMentions;

	//Relation options
	bool _keepRelationsWithMultipleRelMentions;

	bool _filterRelationsWithNonConfidentDescriptors;
	bool _filterRelationsWithNonConfidentPronouns;
	double _filterRelationsWithNonConfidentDescriptorsRequiredConfidence;
	double _filterRelationsWithNonConfidentPronounsRequiredConfidence;

	bool _filter_PHYS_Located_Relations;
	bool _filter_PHYS_Near_Relations;
	bool _filter_PART_WHOLE_Geographical_Relations;
	bool _filter_PART_WHOLE_Subsidiary_Relations;
	bool _filter_PART_WHOLE_Artifact_Relations;
	bool _filter_PER_SOC_Business_Relations;
	bool _filter_PER_SOC_Family_Relations;
	bool _filter_PER_SOC_Lasting_Personal_Relations;
	bool _filter_ORG_AFF_Employment_Relations;
	bool _filter_ORG_AFF_Ownership_Relations;
	bool _filter_ORG_AFF_Founder_Relations;
	bool _filter_ORG_AFF_Student_Alum_Relations;
	bool _filter_ORG_AFF_Sports_Affiliation_Relations;
	bool _filter_ORG_AFF_Investor_Shareholder_Relations;
	bool _filter_ORG_AFF_Membership_Relations;
	bool _filter_ART_User_Owner_Inventor_Manufacturer_Relations;
	bool _filter_GEN_AFF_Citizen_Resident_Religion_Ethnicity_Relations;
	bool _filter_GEN_AFF_Org_Location_Relations;

	bool _filterRelationsWithPronLevelEntity;
	bool _filterRelationsWithPronLevelEntity2;
	bool _filterRelationsWithPronLevelEntity3;
	bool _filterRelationsWithPronLevelEntities;
	bool _filterRelationsWithPronoun;
	bool _filterRelationsWithUnsureNominal;
	bool _filterRelationsWithPronounNameLevelEntity;
	bool _filterRelationsWithPronounDescLevelEntity;
	bool _filterRelationsWith1PPronoun;
	bool _filterRelationsWith2PPronoun;
	bool _filterRelationsWith3PPronoun;
	bool _filterRelationsWithDescLevelEntity;
	bool _filterRelationsWith2DescLevelEntities;
	bool _filterRelationsWithPERPronoun;
	bool _filterRelationsWithPERDesc;
	bool _filterRelationsWithLOCPronoun;
	bool _filterRelationsWithLOCDesc;
	bool _filterRelationsWithGPEPronoun;
	bool _filterRelationsWithGPEDesc;
	bool _filterRelationsWithORGPronoun;
	bool _filterRelationsWithORGDesc;
	bool _filterRelationsWithFACPronoun;
	bool _filterRelationsWithFACDesc;
	bool _filterRelationsWithLocTypePronoun;
	bool _filterRelationsWithPronAndDesc;
	bool _filterRelationsWithWHQPronoun;
	bool _filterRelationsWithGPEdescAndORGDesc;

	void filterClutter (DocTheory *theory);
	void applyFilter ();
	bool is_matched (const Entity *e, const wchar_t *s) const;
	bool isPER (const Entity *e) const { return is_matched (e, L"PER"); }
	bool isGPE (const Entity *e) const { return is_matched (e, L"GPE"); }
	bool isLOC (const Entity *e) const { return is_matched (e, L"LOC"); }
	bool isFAC (const Entity *e) const { return is_matched (e, L"FAC"); }
	bool isORG (const Entity *e) const { return is_matched (e, L"ORG"); }
	bool isWEA (const Entity *e) const { return is_matched (e, L"WEA"); }
	bool isVEH (const Entity *e) const { return is_matched (e, L"VEH"); }
	inline bool isPron (const Mention *m) const { return (m->getMentionType() == Mention::PRON); };
	inline bool isDesc (const Mention *m) const;
	inline bool isName (const Mention *m) const { return isNam(m); };
	bool isLOCTypePronoun (const Mention *m) const;
//#ifdef ENGLISH_LANGUAGE
	bool isWHQTypePronoun (const Mention *m) const;
//#endif
	bool isNam (const Mention *m) const;
	bool isPronLevelEntity (const Entity *ent) const;
	bool isPronLevelEntity2 (const Entity *ent) const;
	bool isPronLevelEntity3 (const Entity *ent) const;
	bool isDescLevelEntity (const Entity *ent) const;
	bool isNameLevelEntity (const Entity *ent) const;
	bool isNam (const Entity *ent) const;
	bool isRef (const Entity *ent) const;
	double checkClutter (const Relation *rel) const;
	double checkClutter (const Mention *ment) const;
	double checkClutter (const Entity *ent) const;
	void relationHandler ();
	void entityHandler ();
	void mentionHandler ();
//	void getMentions(DocTheory *docTheory);

	bool isPER(const Mention *ment) const;
	bool isLOC(const Mention *ment) const;
	bool isGPE(const Mention *ment) const;
	bool isORG(const Mention *ment) const;
	bool isFAC(const Mention *ment) const;
	bool isWEA(const Mention *ment) const;
	bool isVEH(const Mention *ment) const;
};

#endif

