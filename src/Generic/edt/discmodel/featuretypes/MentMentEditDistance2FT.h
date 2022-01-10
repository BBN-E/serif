// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTMENT_EDITDISTANCE2_FT_H
#define MENTMENT_EDITDISTANCE2_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTQuadgram2IntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"

#define QUAD

class MentMentEditDistance2FT : public DTCorefFeatureType {
public:
	MentMentEditDistance2FT() : DTCorefFeatureType(Symbol(L"ment-ment-edit-dist2")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgram2IntFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 0, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		static bool pos_found[MAX_SENTENCE_TOKENS];

		DTCorefObservation *o = static_cast<DTCorefObservation*>(
				state.getObservation(0));
		Entity *entity = o->getEntity();
		const Mention *ment = o->getMention();
		const EntitySet *eset = o->getEntitySet();
		const MentionSymArrayMap *HWMap = o->getHWMentionMapper();

		EntityType mentEntType = ment->getEntityType();
		Symbol mentEntTypeSymbol = (mentEntType.isDetermined()) ? mentEntType.getName() : NO_ENTITY_TYPE ;


		int n_feat = 0;
		size_t size1, size2, len_diff;
		float average_dist;
		int dist;
		SymbolArray **mentArray = HWMap->get(ment->getUID());
		SerifAssert (mentArray != NULL);
		SerifAssert (*mentArray != NULL);
		const Symbol *mentHeadWords = (*mentArray)->getArray();
		int n_ment_words = (*mentArray)->getLength();
		SerifAssert (n_ment_words <= MAX_SENTENCE_TOKENS);

		Symbol entityMentionLevel = o->getEntityMentionLevel();

		for (int i=0; i< n_ment_words; i++)
			pos_found[i] = false;
		for (int m = 0; m < entity->getNMentions(); m++) {
			Mention* entMent = o->getEntitySet()->getMention(entity->getMention(m)); 
			SymbolArray **entityArray = HWMap->get(entity->getMention(m));
			SerifAssert (entityArray != NULL);
			SerifAssert (*entityArray != NULL);
			const Symbol *entHeadWords = (*entityArray)->getArray();
			int n_ent_words = (*entityArray)->getLength();
			for (int i=0; i< n_ment_words; i++) {
				for (int j = 0; j < n_ent_words; j++) {
					if (n_feat >= MAX_FEATURES_PER_EXTRACTION-10)
						return n_feat;

					const wchar_t *s1 = mentHeadWords[i].to_string();
					const wchar_t *s2 = entHeadWords[j].to_string();
					size1 = wcslen(s1);
					size2 = wcslen(s2);
					dist = editDistance(s1, size1, s2, size2);
					if (dist>0) {
						size_t min_len = min(size1, size2);
						len_diff = (size1>size2) ? (size1-size2) : (size2-size1);
						average_dist = (float)dist/min_len;
						if (average_dist<0.2) {
							if (n_feat >= MAX_FEATURES_PER_EXTRACTION-3)
								return n_feat;
							//else
							resultArray[n_feat++] = _new DTQuadgram2IntFeature(this, state.getTag(), PER_MENTION, ZERO_DOT_2, entityMentionLevel, ((i<4) ? i : 4) , ((j<4) ? j : 4));
							if(entMent->getMentionType() == Mention::NAME) {
								resultArray[n_feat++] = _new DTQuadgram2IntFeature(this, state.getTag(), MENTION, NAME, entityMentionLevel, ((i<4) ? i : 4), ((j<4) ? j : 4));
								if (j==n_ent_words-1 && i==n_ment_words-1)
									resultArray[n_feat++] = _new DTQuadgram2IntFeature(this, state.getTag(), MENTION_LAST_WORD, mentEntTypeSymbol, entityMentionLevel, i, -1);
							}
							if(!pos_found[i]) {
								pos_found[i] = true;
//								resultArray[n_feat++] = _new DTTrigram2IntFeature(this, state.getTag(), UNIQUE, ZERO_DOT_3, ((i<4) ? i : 4), ((j<4) ? j : 4));
							}
 						}else if (average_dist<0.3) {
							if (n_feat >= MAX_FEATURES_PER_EXTRACTION-3)
								return n_feat;
							//else
							resultArray[n_feat++] = _new DTQuadgram2IntFeature(this, state.getTag(), PER_MENTION, ZERO_DOT_3, entityMentionLevel, ((i<4) ? i : 4), ((j<4) ? j : 4));
							if(entMent->getMentionType() == Mention::NAME) {
								resultArray[n_feat++] = _new DTQuadgram2IntFeature(this, state.getTag(), MENTION, NAME, entityMentionLevel, ((i<4) ? i : 4), ((j<4) ? j : 4));
								if (j==n_ent_words-1 && i==n_ment_words-1)
									resultArray[n_feat++] = _new DTQuadgram2IntFeature(this, state.getTag(), MENTION_LAST_WORD, mentEntTypeSymbol, entityMentionLevel, i, -1);
							}
							if(!pos_found[i]) {
								pos_found[i] = true;
//								resultArray[n_feat++] = _new DTTrigram2IntFeature(this, state.getTag(), UNIQUE, ZERO_DOT_3, ((i<4) ? i : 4), ((j<4) ? j : 4));
							}
 						}else if (min_len<=2 &&len_diff<1 && dist<=1) {
							if (n_feat >= MAX_FEATURES_PER_EXTRACTION-3)
								return n_feat;
							//else
							resultArray[n_feat++] = _new DTQuadgram2IntFeature(this, state.getTag(), PER_MENTION, SHORT_WORD, entityMentionLevel, ((i<4) ? i : 4), ((j<4) ? j : 4));
							if(entMent->getMentionType() == Mention::NAME) {
								resultArray[n_feat++] = _new DTQuadgram2IntFeature(this, state.getTag(), MENTION, NAME, entityMentionLevel, ((i<4) ? i : 4), ((j<4) ? j : 4));
								if (j==n_ent_words-1 && i==n_ment_words-1)
									resultArray[n_feat++] = _new DTQuadgram2IntFeature(this, state.getTag(), MENTION_LAST_WORD, mentEntTypeSymbol, entityMentionLevel, i, -1);
							}
							if(!pos_found[i]) {
								pos_found[i] = true;
//								resultArray[n_feat++] = _new DTTrigram2IntFeature(this, state.getTag(), UNIQUE, SHORT_WORD, ((i<4) ? i : 4), ((j<4) ? j : 4));
							}
 						}
					}
				}//for
			}
		}

		int n_positions_matched = 0;
		for (int i=0; i< n_ment_words; i++) {
			if(pos_found[i]) {
				n_positions_matched++;
			}
		}
		if (n_positions_matched >0) {
			if (n_feat >= MAX_FEATURES_PER_EXTRACTION-1)
				return n_feat;
			resultArray[n_feat++] = _new DTQuadgram2IntFeature(this, state.getTag(), PER_ENTITY, mentEntTypeSymbol, entityMentionLevel, n_positions_matched, (n_ment_words<5)?n_ment_words:5);

			for (int i=0; i< n_ment_words; i++) {
				if (n_feat >= MAX_FEATURES_PER_EXTRACTION-2)
						return n_feat;
				if(pos_found[i]) {
					resultArray[n_feat++] = _new DTQuadgram2IntFeature(this, state.getTag(), POS_MATCH, SymbolConstants::nullSymbol, entityMentionLevel, i, -1);
					resultArray[n_feat++] = _new DTQuadgram2IntFeature(this, state.getTag(), POS_MATCH, mentEntTypeSymbol, entityMentionLevel, i, -1);
				}else {
					resultArray[n_feat++] = _new DTQuadgram2IntFeature(this, state.getTag(), POS_NOT_MATCH, SymbolConstants::nullSymbol, entityMentionLevel, i, -1);
					resultArray[n_feat++] = _new DTQuadgram2IntFeature(this, state.getTag(), POS_NOT_MATCH, mentEntTypeSymbol, entityMentionLevel, i, -1);
				}
			}
		}
		return n_feat;
	}



int min(int a, int b) const {
	return ((a < b) ? a : b);
}
size_t min(size_t a, size_t b) const {
	return ((a < b) ? a : b);
}


int editDistance(const wchar_t *s1, size_t n1, const wchar_t *s2, size_t n2) const {
	int cost_del = 2;
	int cost_ins = 1;
	int cost_sub = 1;
	static const int cost_switch = 1; // switching 2 characters (e.g. ba insteadof ab)
	size_t i,j;



	dist[0] = 0;

	for (i = 1; i <= n1; i++) 
		dist[i*(n2+1)] = dist[(i-1)*(n2+1)] + cost_del;

	for (j = 1; j <= n2; j++) 
		dist[j] = dist[j-1] + cost_ins;

	// for i=1;
	for (j = 1; j <= n2; j++) {
		int dist_del = dist[j] + cost_del;
		int dist_ins = dist[n2+1+j-1] + cost_ins;
		int dist_sub = dist[j-1] + 
						(s1[0] == s2[j-1] ? 0 : cost_sub);
		dist[n2+1+j] = min(min(dist_del, dist_ins), dist_sub);
	}

	//for j=1;
	for (i = 1; i <= n1; i++) {
		int dist_del = dist[(i-1)*(n2+1)+1] + cost_del;
		int dist_ins = dist[i*(n2+1)] + cost_ins;
		int dist_sub = dist[(i-1)*(n2+1)] + 
						(s1[i-1] == s2[0] ? 0 : cost_sub);
		dist[i*(n2+1)+1] = min(min(dist_del, dist_ins), dist_sub);
	}

	for (i = 2; i <= n1; i++) {
		for (j = 2; j <= n2; j++) {
			int dist_del = dist[(i-1)*(n2+1)+j] + cost_del;
			int dist_ins = dist[i*(n2+1)+(j-1)] + cost_ins;
			int dist_sub = dist[(i-1)*(n2+1)+(j-1)] + 
							(s1[i-1] == s2[j-1] ? 0 : cost_sub);
			int dist_switch = dist[(i-2)*(n2+1)+(j-2)] + cost_switch;
			dist[i*(n2+1)+j] = min(min(dist_del, dist_ins), dist_sub);
			if(s1[i-1] == s2[j-2] && s1[i-2] == s2[j-1])
				dist[i*(n2+1)+j] = min(dist[i*(n2+1)+j], dist_switch);
		}
	}

	int tmp = dist[n1*(n2+1)+n2];

	return tmp;
}
private:
mutable int dist[(MAXSTRINGLEN+1)*(MAXSTRINGLEN+1)];

};
#endif
