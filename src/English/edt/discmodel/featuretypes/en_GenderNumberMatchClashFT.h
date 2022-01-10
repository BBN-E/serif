// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_GENDERNUMBER_CLASHMATCH_FT_H
#define EN_GENDERNUMBER_CLASHMATCH_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discTagger/DTSeptgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"

#include "English/edt/en_Guesser.h"

#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntitySet.h"


class EnglishEn_GenderNumberMatchClashFT : public DTCorefFeatureType {
	enum {FEMININE, MASCULINE, GENDER_NEUTRAL, SINGULAR, PLURAL, NUMBER_UNKNOWN} constants;

public:
	EnglishEn_GenderNumberMatchClashFT() : DTCorefFeatureType(Symbol(L"en_gender-number-match-clash")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTSeptgramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		int nFeat = 0;
		DTCorefObservation *o = static_cast<DTCorefObservation*>(state.getObservation(0));
		const MentionToSymbolMap* genderMap = o->getMentionGenderMapper();
		const MentionToSymbolMap* numberMap = o->getMentionNumberMapper();
		Symbol entityType = o->getEntity()->getType().getName();

		MentionUID mentUID = o->getMention()->getUID();
		const Symbol *linkedGender = genderMap->get(mentUID);
		const Symbol *linkedNumber = numberMap->get(mentUID);
		if(linkedGender==NULL || linkedNumber==NULL)
			return 0;

		const EntitySet *eset = o->getEntitySet();
		const Entity *entity = o->getEntity();

		const Symbol *entMentionGender, *entMentionNumber;
		MentionUID entMentionUID;
		// collect gender/number statistics on the entity
		int statistics[6] = {0,0,0,0,0,0};
		for(int m=0; m<entity->getNMentions(); m++) {
			entMentionUID = eset->getMention(entity->mentions[m])->getUID();
			
			entMentionGender = genderMap->get(entMentionUID);
			if(*entMentionGender == Guesser::FEMININE) {
				statistics[FEMININE]++;
			}else if(*entMentionGender == Guesser::MASCULINE) {
				statistics[MASCULINE]++;
			}else if(*entMentionGender == Guesser::NEUTRAL) {
				statistics[GENDER_NEUTRAL]++;
			}

			entMentionNumber = numberMap->get(entMentionUID);
			if(*entMentionNumber == Guesser::SINGULAR) {
				statistics[SINGULAR]++;
			}else if(*entMentionNumber == Guesser::PLURAL) {
				statistics[PLURAL]++;
			}else if(*entMentionNumber == Guesser::UNKNOWN) {
				statistics[NUMBER_UNKNOWN]++;
			}
		}

		Symbol hasFEMININE = (statistics[FEMININE]>0) ? HAS_FEMININE : SymbolConstants::nullSymbol;
		Symbol hasMASCULINE = (statistics[MASCULINE]>0) ? HAS_MASCULINE : SymbolConstants::nullSymbol;

		Symbol moreGender;
		if (statistics[FEMININE]>statistics[MASCULINE])
			moreGender = MORE_FEMININE;
		else if (statistics[FEMININE]<statistics[MASCULINE])
			moreGender = MORE_MASCULINE;
		else
			moreGender = GENDER_EQUAL;


		Symbol mostlyGender = MOSTLY_NEUTRAL;
		if (moreGender == MORE_FEMININE && 
			((float)statistics[FEMININE])/(statistics[FEMININE]+statistics[MASCULINE]+statistics[GENDER_NEUTRAL])>0.4)
			mostlyGender = MOSTLY_FEMININE;
		else if(moreGender == MORE_MASCULINE && 
			((float)statistics[MASCULINE])/(statistics[FEMININE]+statistics[MASCULINE]+statistics[GENDER_NEUTRAL])>0.4)
			mostlyGender = MOSTLY_MASCULINE;



		Symbol hasSINGULAR = (statistics[SINGULAR]>0) ? HAS_SINGULAR : SymbolConstants::nullSymbol;
		Symbol hasPLURAL = (statistics[PLURAL]>0) ? HAS_PLURAL : SymbolConstants::nullSymbol;

		Symbol moreNumber;
		if (statistics[SINGULAR]>statistics[PLURAL])
			moreNumber = MORE_SINGULAR;
		else if (statistics[SINGULAR]<statistics[PLURAL])
			moreNumber = MORE_PLURAL;
		else
			moreNumber = NUMBER_EQUAL;


		//gender features
		resultArray[nFeat++] = _new DTSeptgramFeature(this, state.getTag(), entityType,
			*linkedGender, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, moreGender, mostlyGender);
		resultArray[nFeat++] = _new DTSeptgramFeature(this, state.getTag(), entityType,
			*linkedGender, hasFEMININE, hasMASCULINE, moreGender, mostlyGender);

		// number features
		resultArray[nFeat++] = _new DTSeptgramFeature(this, state.getTag(), entityType,
			*linkedNumber, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, moreNumber, SymbolConstants::nullSymbol);
		resultArray[nFeat++] = _new DTSeptgramFeature(this, state.getTag(), entityType,
			*linkedNumber, hasSINGULAR, hasPLURAL, moreNumber, SymbolConstants::nullSymbol);

		//combined gender/number feature
		resultArray[nFeat++] = _new DTSeptgramFeature(this, state.getTag(), entityType,
			*linkedGender, *linkedNumber, moreNumber, moreGender, mostlyGender);

		return nFeat;
	}

};
#endif
