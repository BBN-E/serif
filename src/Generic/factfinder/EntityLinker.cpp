// Copyright (c) 2014 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include <string>
#include "Generic/factfinder/EntityLinker.h"

#include "Generic/theories/ActorMention.h"
#include "Generic/theories/ActorMentionSet.h"
#include "Generic/theories/ActorEntity.h"
#include "Generic/theories/ActorEntitySet.h"

#include <boost/foreach.hpp>


EntityLinker::EntityLinker(bool _use_actor_id) {
	gid = 0;
	_use_actor_id_for_entity_linker = _use_actor_id;
}

Symbol EntityLinker::getEntityID(int id) {
//	std::string gid_str = std::to_string(gid++);
//	std::string id_str = std::to_string(id);

//	Symbol entity_id(("e"+gid_str+"-"+id_str).c_str());


	wchar_t  buff[100];
	swprintf(buff, 100, L"e-%d-%d", gid++, id);
	std::wstring str_entity_id(buff);

//	std::wstring str_entity_id = L"e-" + std::to_wstring(gid++) + L"-" + std::to_wstring(id);

	Symbol entity_id(str_entity_id.c_str());
	return entity_id;
}

Symbol EntityLinker::getMentionActorID(Symbol type, const Mention *m, const DocTheory* docTheory, double minActorPatternConf, double minEditDistance, double minPatternMatchScorePlusAssociationScore, std::map<int, double>& actorId2actorEntityConf) {
        std::wstring entity_type(type.to_string());

	ActorMentionSet* ams = docTheory->getSentenceTheory(0)->getActorMentionSet();
	if(m->getMentionType() == Mention::NAME) {
		std::vector<ActorMention_ptr> actorMentions = ams->findAll(m->getUID());

		// number of ActorMention this mention matched
		int numActorMentions=static_cast<int>(actorMentions.size());

		int numActorMentions_ActorPattern=0;
		double maxPatternConf=0.0;
		double maxPatternMatchScorePlusAssociationScore=0.0;
		Symbol actorIdBestByActorPattern=NULL;

		int numActorMentions_EditDistance=0;
		double maxEditDistance=0.0;
		Symbol actorIdBestByEditDistance=NULL;

		// actorMention -> actorEntity -> conf
		double maxActorEntityConf=0.0;
		Symbol actorIdBestByActorEntityConf=NULL;

		for(std::vector<ActorMention_ptr>::iterator it=actorMentions.begin(); it!=actorMentions.end(); it++) {
			ActorMention_ptr actorMention = *it;
			if(actorMention==ActorMention_ptr())
				return NULL;

			ProperNounActorMention_ptr properNounActorMention = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention);
			if(properNounActorMention==ProperNounActorMention_ptr())
				return NULL;

			int actorIdInt = properNounActorMention->getActorId().getId();
			Symbol sourceNote = properNounActorMention->getSourceNote().to_string();
			double patternMatchScore = properNounActorMention->getPatternMatchScore();
			double patternConfidenceScore = properNounActorMention->getPatternConfidenceScore();
			double associationScore = properNounActorMention->getAssociationScore();
			double editDistanceScore = properNounActorMention->getEditDistanceScore();

			wchar_t  buff[100];
			swprintf(buff, 100, L"%d", actorIdInt);
			std::wstring str_entity_id(buff);
			Symbol actorId(L":" + entity_type + L"_Actor_" + str_entity_id);

			if(sourceNote==Symbol(L"ACTOR_PATTERN")) {
				numActorMentions_ActorPattern++;
/*
                                if(patternConfidenceScore>maxPatternConf) {
					actorIdBestByActorPattern=actorId;
                                }
*/
				if(patternMatchScore + associationScore > maxPatternMatchScorePlusAssociationScore) {
					maxPatternMatchScorePlusAssociationScore = patternMatchScore + associationScore;
					actorIdBestByActorPattern=actorId;
				}
                        }
			if(sourceNote==Symbol(L"EDIT_DISTANCE")) {
				numActorMentions_EditDistance++;
				if(editDistanceScore>maxEditDistance) {
					actorIdBestByEditDistance=actorId;
				}
			}

			std::map<int, double>::iterator iter = actorId2actorEntityConf.find(actorIdInt);
			if(iter!=actorId2actorEntityConf.end()) {
				if(iter->second>maxActorEntityConf) {
					actorIdBestByActorEntityConf=actorId;
					maxActorEntityConf=iter->second;
				}
			}
		}

		// multiple actorMention but none is matched by actor pattern
		if(numActorMentions>1 && numActorMentions_ActorPattern==0)
			return NULL;

		//
                return actorIdBestByActorEntityConf;

		if(maxPatternMatchScorePlusAssociationScore > minPatternMatchScorePlusAssociationScore)
			return actorIdBestByActorPattern;

/*
		// if there is a good actorID matched by ActorPattern
		if(maxPatternConf>=minActorPatternConf && actorIdBestByActorPattern!=NULL)
			return actorIdBestByActorPattern;
		// otherwise if there is a good match by edit distance
		else if(maxEditDistance>=minEditDistance && actorIdBestByEditDistance!=NULL)
			return actorIdBestByEditDistance;
*/
	}

	return NULL;
}


// Symbol EntityLinker::getEntityID(Symbol type, std::wstring canonicalNameStr, const Mention* m, const DocTheory* docTheory) {

Symbol EntityLinker::getEntityID(Symbol type, std::wstring canonicalNameStr, const Entity *e, const DocTheory* docTheory, double min_actor_match_conf) {
        std::wstring entity_type(type.to_string());

	if(_use_actor_id_for_entity_linker) {
		ActorEntitySet *aes = docTheory->getActorEntitySet();
		std::vector<ActorEntity_ptr> actorEntities = aes->find(e);

		int actorId = -10001;
		double actor_match_conf = 0;

		BOOST_FOREACH(ActorEntity_ptr actorEntity, actorEntities) {
			// Find highest scored actorEntity
			// if the score is above some threshold and !actorEntity->isUnmatchedActor()
			// then we have a good match that we should trust

			if(actorEntity->getConfidence()>=min_actor_match_conf && actorEntity->getConfidence()>actor_match_conf && !actorEntity->isUnmatchedActor()) {
				actor_match_conf = actorEntity->getConfidence();
				actorId = actorEntity->getActorId().getId();
			}
		}

		if(actorId!=-10001 && actor_match_conf>=min_actor_match_conf) {
			wchar_t  buff[100];
			swprintf(buff, 100, L"%d", actorId);
			std::wstring str_entity_id(buff);

			return Symbol(L":" + entity_type + L"_Actor_" + str_entity_id);
		}
	}

//      std::string gid_str = std::to_string(gid++);
//      std::string id_str = std::to_string(id);

//      Symbol entity_id(("e"+gid_str+"-"+id_str).c_str());
//	std::wstring entity_type(type.to_string());

	std::wstring lower_canonical_name = canonicalNameStr;
	boost::to_lower(lower_canonical_name);
	boost::replace_all(lower_canonical_name,L",",L" ");
	boost::replace_all(lower_canonical_name,L";",L" ");
	boost::replace_all(lower_canonical_name,L".",L" ");

        boost::replace_all(lower_canonical_name,L"&",L" ");
        boost::replace_all(lower_canonical_name,L"$",L" ");
        boost::replace_all(lower_canonical_name,L"=",L" ");
        boost::replace_all(lower_canonical_name,L"/",L" ");
        boost::replace_all(lower_canonical_name,L"\\",L" ");
        boost::replace_all(lower_canonical_name,L":",L" ");
        boost::replace_all(lower_canonical_name,L"#",L" ");
        boost::replace_all(lower_canonical_name,L"@",L" ");
        boost::replace_all(lower_canonical_name,L"!",L" ");
        boost::replace_all(lower_canonical_name,L"%",L" ");
        boost::replace_all(lower_canonical_name,L"^",L" ");
        boost::replace_all(lower_canonical_name,L"*",L" ");
        boost::replace_all(lower_canonical_name,L"(",L" ");
        boost::replace_all(lower_canonical_name,L")",L" ");
        boost::replace_all(lower_canonical_name,L"=",L" ");
        boost::replace_all(lower_canonical_name,L"+",L" ");
        boost::replace_all(lower_canonical_name,L"{",L" ");
        boost::replace_all(lower_canonical_name,L"}",L" ");
        boost::replace_all(lower_canonical_name,L"`",L" ");
        boost::replace_all(lower_canonical_name,L"\"",L" ");
        boost::replace_all(lower_canonical_name,L"'",L" ");
        boost::replace_all(lower_canonical_name,L"-",L" ");
        boost::replace_all(lower_canonical_name,L"~",L" ");
        boost::replace_all(lower_canonical_name,L"[",L" ");
        boost::replace_all(lower_canonical_name,L"]",L" ");
        boost::replace_all(lower_canonical_name,L"|",L" ");
        boost::replace_all(lower_canonical_name,L"?",L" ");
        boost::replace_all(lower_canonical_name,L"<",L" ");
        boost::replace_all(lower_canonical_name,L">",L" ");

	boost::replace_all(lower_canonical_name,L" ",L"_");

	Symbol entity_id(L":" + entity_type + L"_" + lower_canonical_name);

	if(entity_id == Symbol(L":ORG_university_of_pennsylvania") || entity_id == Symbol(L":ORG_the_university_of_pennsylvania"))
		entity_id = Symbol(L":ORG_penn");
	if(entity_id == Symbol(L":ORG_harvard_university"))
		entity_id = Symbol(L":ORG_harvard");
	if(entity_id == Symbol(L":ORG_cornell_university"))
		entity_id = Symbol(L":ORG_cornell");

	return entity_id;
}

EntityLinker::~EntityLinker() {
}
