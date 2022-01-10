// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "English/edt/discmodel/en_DTCorefFeatureTypes.h"

#include "English/edt/discmodel/featuretypes/en_HWentGNT_FT.h"
#include "English/edt/discmodel/featuretypes/en_HWentGNT_Deprecated_FT.h"
#include "English/edt/discmodel/featuretypes/en_entGNT_FT.h"
#include "English/edt/discmodel/featuretypes/en_GenderFT.h"
#include "English/edt/discmodel/featuretypes/en_isGenericFT.h"
#include "English/edt/discmodel/featuretypes/en_NumberClashFT.h"
#include "English/edt/discmodel/featuretypes/en_MentPossesiveFT.h"
#include "English/edt/discmodel/featuretypes/en_NamePersonClashFT.h"
#include "English/edt/discmodel/featuretypes/en_GenderNumberMatchClashFT.h"
#include "English/edt/discmodel/featuretypes/en_CountryCapitalFT.h"

void EnglishDTCorefFeatureTypes::ensureFeatureTypesInstantiated() {

	// get the language independent featureTypes
	if (DTCorefFeatureTypes::_instantiated)
		return;
	DTCorefFeatureTypes::ensureBaseFeatureTypesInstantiated();

	_new EnglishEntGNT_FT();
	_new EnglishEn_HWentGNT_Deprecated_FT(); // for backward compatibility
	_new EnglishHWentGNT_FT();
	_new EnglishGenderFT();
	_new EnglishIsGenericFT();
	_new EnglishNumberClashFT();
	_new EnglishMentPossesiveFT();

	// For Name linking
	_new EnglishNamePersonClashFT();
	_new EnglishEn_GenderNumberMatchClashFT();
	_new EnglishEn_CountryCapitalFT();

}
