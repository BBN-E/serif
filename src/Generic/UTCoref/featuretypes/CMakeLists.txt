####################################################################
# Copyright (c) 2010 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/UTCoref/featuretypes                                     #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(featuretypes
  SOURCE_FILES
    UTFeatureType.h
    UTFeatureType.cpp

    UTFeatureNameTypeMatch.h
    UTFeatureHeadwordMatch.h
    UTFeatureHeadwordMatchLower.h
    UTFeatureNameTypesLiteral.h
    UTFeatureStrictStringMatch.h
    UTFeatureAOfBIsBA.h
    UTFeatureAliasDate.h
    UTFeatureHeadwordMatchNoPro.h
    UTFeatureHeadwordLeft.h
    UTFeatureHeadwordLeftLower.h
    UTFeatureHeadwordRight.h
    UTFeatureHeadwordRightLower.h
    UTFeatureSomeSubstring.h
    UTFeatureLeftIsPro.h
    UTFeatureRightIsPro.h
    UTFeatureSoonStrMatch.h
    UTFeatureStrMatchNoDtNoPro.h
    UTFeatureStrMatchNoDtNoProFixedDt.h
    UTFeatureLeftSubstrRight.h
    UTFeatureRightSubstrLeftNoAncestor.h
    UTFeatureRightSubstrLeft.h
    UTFeatureRightSubstrLeftNoPro.h
    UTFeatureLeftSubstrRightNoPro.h
    UTFeatureExactModifierHead.h
    UTFeatureAliasSuper.h
    UTFeatureAliasSuperNoStrMatch.h
    UTFeatureAliasSoon.h
    UTFeatureAliasIgnoreLower.h
    UTFeatureAliasOf.h
    UTFeatureAliasOrgFirstWord.h
    UTFeatureAliasGpe.h
    UTFeatureAliasOrg.h
    UTFeatureAliasPerson.h
    UTFeatureApposMuc6NextTo.h
    UTFeatureApposMuc6IndefNextTo.h
    UTFeatureApposMuc7NextTo.h
    UTFeatureSemclassLeft.h
    UTFeatureSemclassRight.h
    UTFeatureWordnetLca.h
    UTFeatureProperName.h
    UTFeatureSemclass.h
    UTFeatureGender.h
    UTFeatureNumber.h
    UTFeatureResolvePronouns.h
    UTFeatureTreeMuc7.h
    UTFeatureTreeMuc6.h
    UTFeatureDifferentSentences.h
)
