// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/Symbol.h"
#include "UTFeatureType.h"

#include "UTFeatureNameTypeMatch.h"
#include "UTFeatureHeadwordMatch.h"
#include "UTFeatureHeadwordMatchLower.h"
#include "UTFeatureNameTypesLiteral.h"
#include "UTFeatureStrictStringMatch.h"
#include "UTFeatureAOfBIsBA.h"
#include "UTFeatureAliasDate.h"
#include "UTFeatureAliasOrg.h"
#include "UTFeatureAliasPerson.h"
#include "UTFeatureSemclass.h"
#include "UTFeatureGender.h"
#include "UTFeatureNumber.h"
#include "UTFeatureSemclassLeft.h"
#include "UTFeatureSemclassRight.h"
#include "UTFeatureWordnetLca.h"
#include "UTFeatureHeadwordMatchNoPro.h"
#include "UTFeatureHeadwordLeft.h"
#include "UTFeatureHeadwordLeftLower.h"
#include "UTFeatureHeadwordRight.h"
#include "UTFeatureHeadwordRightLower.h"
#include "UTFeatureSomeSubstring.h"
#include "UTFeatureLeftIsPro.h"
#include "UTFeatureRightIsPro.h"
#include "UTFeatureSoonStrMatch.h"
#include "UTFeatureStrMatchNoDtNoPro.h"
#include "UTFeatureStrMatchNoDtNoProFixedDt.h"
#include "UTFeatureLeftSubstrRight.h"
#include "UTFeatureLeftSubstrRightNoPro.h"
#include "UTFeatureRightSubstrLeft.h"
#include "UTFeatureRightSubstrLeftNoAncestor.h"
#include "UTFeatureRightSubstrLeftNoPro.h"
#include "UTFeatureAliasSuper.h"
#include "UTFeatureAliasSuperNoStrMatch.h"
#include "UTFeatureAliasSoon.h"
#include "UTFeatureAliasIgnoreLower.h"
#include "UTFeatureAliasOf.h"
#include "UTFeatureAliasOrgFirstWord.h"
#include "UTFeatureAliasGpe.h"
#include "UTFeatureApposMuc6NextTo.h"
#include "UTFeatureApposMuc6IndefNextTo.h"
#include "UTFeatureApposMuc7NextTo.h"
#include "UTFeatureDifferentSentences.h"
#include "UTFeatureProperName.h"
#include "UTFeatureResolvePronouns.h"
#include "UTFeatureTreeMuc6.h"
#include "UTFeatureTreeMuc7.h"
#include "UTFeatureExactModifierHead.h"

Symbol UTFeatureType::modeltype = Symbol(L"utcoref");
bool UTFeatureType::registered = false;

Symbol UTFeatureType::True = Symbol(L"T");
Symbol UTFeatureType::False = Symbol(L"F");
Symbol UTFeatureType::Unknown = Symbol(L"U");
Symbol UTFeatureType::NotImplemented = Symbol(L"NI");
Symbol UTFeatureType::None = Symbol(L"None");

void UTFeatureType::initFeatures() {
   std::cout << "initFeatures" << std::endl;
   if (!registered) {
	  registered = true;

	  std::cout << "registering features" << std::endl;
		
	  // they register themselves on construction
	  _new UTFeatureNameTypeMatch();
	  _new UTFeatureHeadwordMatch();
	  _new UTFeatureHeadwordMatchLower();
	  _new UTFeatureNameTypesLiteral();
	  _new UTFeatureStrictStringMatch();
	  _new UTFeatureAliasDate();
	  _new UTFeatureAOfBIsBA();
	  _new UTFeatureAliasOrg();
	  _new UTFeatureHeadwordMatchNoPro();
	  _new UTFeatureHeadwordLeft();
	  _new UTFeatureHeadwordLeftLower();
	  _new UTFeatureHeadwordRight();
	  _new UTFeatureHeadwordRightLower();
	  _new UTFeatureSomeSubstring();
	  _new UTFeatureLeftIsPro();
	  _new UTFeatureRightIsPro();		
	  _new UTFeatureSemclass();
	  _new UTFeatureGender();
	  _new UTFeatureNumber();
	  _new UTFeatureSemclassLeft();
	  _new UTFeatureSemclassRight();
	  _new UTFeatureWordnetLca();
	  _new UTFeatureSoonStrMatch();
	  _new UTFeatureStrMatchNoDtNoPro();
	  _new UTFeatureStrMatchNoDtNoProFixedDt();
	  _new UTFeatureLeftSubstrRight();
	  _new UTFeatureRightSubstrLeftNoAncestor();
	  _new UTFeatureRightSubstrLeft();
	  _new UTFeatureRightSubstrLeftNoPro();
	  _new UTFeatureLeftSubstrRightNoPro();	  
	  _new UTFeatureAliasPerson();
	  _new UTFeatureAliasSoon();
	  _new UTFeatureAliasSuperNoStrMatch();
	  _new UTFeatureAliasSuper();
	  _new UTFeatureAliasIgnoreLower();
	  _new UTFeatureAliasOf();
	  _new UTFeatureAliasOrgFirstWord();
	  _new UTFeatureAliasGpe();
	  _new UTFeatureApposMuc6NextTo();
	  _new UTFeatureApposMuc6IndefNextTo();
	  _new UTFeatureApposMuc7NextTo();
	  _new UTFeatureDifferentSentences();
	  _new UTFeatureProperName();
	  _new UTFeatureResolvePronouns();
	  _new UTFeatureTreeMuc6();
	  _new UTFeatureTreeMuc7();
	  _new UTFeatureExactModifierHead();
   }
}
