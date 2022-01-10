// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef FULLMENTMENT_EDITDISTANCE2_FT_H
#define FULLMENTMENT_EDITDISTANCE2_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discTagger/DT6gramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/linuxPort/serif_port.h"

#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"

#define MAXSTRINGLEN 300

class FullMentMentEditDistance2FT : public DTCorefFeatureType {
public:
	FullMentMentEditDistance2FT() : DTCorefFeatureType(Symbol(L"full-ment-ment-edit-dist2")) {
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DT6gramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol
			, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol
			, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
				state.getObservation(0));
		Entity *entity = o->getEntity();
		const Mention *ment = o->getMention();
		HWMap = o->getHWMentionMapper();

		mentEntType = ment->getEntityType();
		Symbol mentEntTypeSymbol = (mentEntType.isDetermined()) ? mentEntType.getName() : NO_ENTITY_TYPE ;
		n_feat = 0;
		for(int i=1;i<=3;i++)
			for(int j=0;j<2;j++)
				zero[i][j] = false;


		Symbol entityMentionLevel = o->getEntityMentionLevel();

		mentArray = HWMap->get(ment->getUID());
		SerifAssert (mentArray != NULL);
		SerifAssert (*mentArray != NULL);
		const Symbol *mentHeadWords = (*mentArray)->getArray();
		n_ment_words = (*mentArray)->getLength();
		SerifAssert (n_ment_words <= MAX_SENTENCE_TOKENS);
		// append the mention tokens into one string
	    mention_str[0] = '\0';
		for (size_t j=0; j<n_ment_words; j++) {
			word = mentHeadWords[j].to_string();
			goodAppend(mention_str, word);
		}
		mentlen = wcslen(mention_str);
		for (int m = 0; m < entity->getNMentions(); m++) {
			Mention* entMent = o->getEntitySet()->getMention(entity->getMention(m)); 
			entityArray = HWMap->get(entity->getMention(m));
			SerifAssert (entityArray != NULL);
			SerifAssert (*entityArray != NULL);
			const Symbol *entHeadWords = (*entityArray)->getArray();
			n_ent_words = (*entityArray)->getLength();
			// append the entity tokens into one string
		    entity_str[0] = '\0';
			for (size_t j=0; j<n_ent_words; j++) {
				word = entHeadWords[j].to_string();
				goodAppend(entity_str, word);
			}
			entlen = wcslen(entity_str);
			feDistance = fullEditDistance();

			Mention::Type mentType = /*entMent->getMentionType();*/Mention::NONE;
			Symbol mentLevel = (mentType == Mention::NAME) ? DTCorefObservation::NAME_LVL : DTCorefObservation::NOMINAL_LVL;
			int mentLevelIndx = (mentType == Mention::NAME) ? 1 : 0;
			
			if (n_feat >= MAX_FEATURES_PER_EXTRACTION-3)
				return n_feat;
			if (feDistance>0) {
				min_len = _cpp_min(mentlen, entlen);
				average_dist = (float)feDistance/min_len;
				if (average_dist<0.3) {
					zero[3][mentLevelIndx]=true;
					resultArray[n_feat++] = _new DT6gramFeature(this, state.getTag(), mentEntTypeSymbol, ZERO_DOT_3, PER_MENTION, mentLevel, entityMentionLevel);
					if (average_dist<0.2) {
						zero[2][mentLevelIndx]=true;
						resultArray[n_feat++] = _new DT6gramFeature(this, state.getTag(), mentEntTypeSymbol, ZERO_DOT_2, PER_MENTION, mentLevel, entityMentionLevel);
						if (average_dist<0.1) {
							zero[1][mentLevelIndx]=true;
							resultArray[n_feat++] = _new DT6gramFeature(this, state.getTag(), mentEntTypeSymbol, ZERO_DOT_1, PER_MENTION, mentLevel, entityMentionLevel);
						}
					}
				}
			}
		}// for mention

		if (n_feat >= MAX_FEATURES_PER_EXTRACTION-6)
			return n_feat;

		if(zero[3][0]) {
			resultArray[n_feat++] = _new DT6gramFeature(this, state.getTag(), mentEntTypeSymbol, ZERO_DOT_3, PER_ENTITY, DTCorefObservation::NOMINAL_LVL, entityMentionLevel);
			if(zero[2][0]) {
				resultArray[n_feat++] = _new DT6gramFeature(this, state.getTag(), mentEntTypeSymbol, ZERO_DOT_2, PER_ENTITY, DTCorefObservation::NOMINAL_LVL, entityMentionLevel);
				if(zero[1][0]) {
					resultArray[n_feat++] = _new DT6gramFeature(this, state.getTag(), mentEntTypeSymbol, ZERO_DOT_1, PER_ENTITY, DTCorefObservation::NOMINAL_LVL, entityMentionLevel);
				}
			}
		}
		if(zero[3][1]) {
			resultArray[n_feat++] = _new DT6gramFeature(this, state.getTag(), mentEntTypeSymbol, ZERO_DOT_3, PER_ENTITY, DTCorefObservation::NAME_LVL, entityMentionLevel);
			if(zero[2][1]) {
				resultArray[n_feat++] = _new DT6gramFeature(this, state.getTag(), mentEntTypeSymbol, ZERO_DOT_2, PER_ENTITY, DTCorefObservation::NAME_LVL, entityMentionLevel);
				if(zero[1][1]) {
					resultArray[n_feat++] = _new DT6gramFeature(this, state.getTag(), mentEntTypeSymbol, ZERO_DOT_1, PER_ENTITY, DTCorefObservation::NAME_LVL, entityMentionLevel);
				}
			}
		}
		return n_feat;
	}


	int fullEditDistance() const {
		static const int cost_del = 2;
		static const int cost_ins = 1;
		static const int cost_sub = 1;
		static const int cost_switch = 1; // switching 2 characters (e.g. ba insteadof ab)
		size_t i,j;

		if(entlen>MAXSTRINGLEN || mentlen>MAXSTRINGLEN)
			return 10000;

		dist[0] = 0;

		for (i = 1; i <= mentlen; i++) 
			dist[i*(entlen+1)] = dist[(i-1)*(entlen+1)] + cost_del;

		for (j = 1; j <= entlen; j++) 
			dist[j] = dist[j-1] + cost_ins;

		i=1; // for i==1
		for (j = 1; j <= entlen; j++) {
			int dist_del = dist[j] + cost_del;
			int dist_ins = dist[entlen+j] + cost_ins;
			int dist_sub = dist[j-1] + 
							(mention_str[0] == entity_str[j-1] ? 0 : cost_sub);
			dist[(entlen+1)+j] = _cpp_min(_cpp_min(dist_del, dist_ins), dist_sub);
		}

		
		j=1; // for j==1
		for (i = 1; i <= mentlen; i++) {
			int dist_del = dist[(i-1)*(entlen+1)+1] + cost_del;
			int dist_ins = dist[i*(entlen+1)] + cost_ins;
			int dist_sub = dist[(i-1)*(entlen+1)] + 
							(mention_str[i-1] == entity_str[0] ? 0 : cost_sub);
			dist[i*(entlen+1)+1] = _cpp_min(_cpp_min(dist_del, dist_ins), dist_sub);
		}

		for (i = 2; i <= mentlen; i++) {
			for (j = 2; j <= entlen; j++) {
				int dist_del = dist[(i-1)*(entlen+1)+j] + cost_del;
				int dist_ins = dist[i*(entlen+1)+(j-1)] + cost_ins;
				int dist_sub = dist[(i-1)*(entlen+1)+(j-1)] + 
								(mention_str[i-1] == entity_str[j-1] ? 0 : cost_sub);
				int dist_switch = dist[(i-2)*(entlen+1)+(j-2)] + cost_switch;
				dist[i*(entlen+1)+j] = _cpp_min(_cpp_min(dist_del, dist_ins), dist_sub);
				if(mention_str[i-1] == entity_str[j-2] && mention_str[i-2] == entity_str[j-1])
					dist[i*(entlen+1)+j] = _cpp_min(dist[i*(entlen+1)+j], dist_switch);
			}
		}

		return dist[mentlen*(entlen+1)+entlen];
	}

	void goodAppend(wchar_t string[], const wchar_t suffix[]) const
	{
		const wchar_t *space = L" \0";
		wcsncat( string, space, _cpp_min( 1, static_cast<int>(MAXSTRINGLEN-wcslen(string))) );
		wcsncat( string, suffix, _cpp_min( static_cast<int>(wcslen(suffix)), static_cast<int>(MAXSTRINGLEN-wcslen(string))) );
	}

	private:
	mutable int feDistance;
	mutable const MentionSymArrayMap *HWMap;
	mutable SymbolArray **entityArray;
	mutable SymbolArray **mentArray;
	mutable wchar_t mention_str[MAXSTRINGLEN+1];
	mutable wchar_t entity_str[MAXSTRINGLEN+1];
	mutable const wchar_t *word;
	mutable size_t n_ment_words,n_ent_words;
	mutable size_t min_len;
	mutable size_t mentlen, entlen;
	mutable int n_feat;
	mutable float average_dist;
	mutable bool zero[4][2];
	mutable int dist[(MAXSTRINGLEN+1)*(MAXSTRINGLEN+1)];
	mutable EntityType mentEntType;
};
#endif
