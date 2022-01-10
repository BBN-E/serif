// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/Attribute.h"

const wchar_t *Polarity::STRINGS[] = { L"Positive", 
                                       L"Negative" };

const PolarityAttribute Polarity::POSITIVE(Polarity::POSITIVE_);
const PolarityAttribute Polarity::NEGATIVE(Polarity::NEGATIVE_);

const wchar_t *Tense::STRINGS[] = { L"Unspecified", 
                                    L"Past", 
									L"Present", 
									L"Future" };

const TenseAttribute Tense::UNSPECIFIED(Tense::UNSPECIFIED_);
const TenseAttribute Tense::PAST(Tense::PAST_);
const TenseAttribute Tense::PRESENT(Tense::PRESENT_);
const TenseAttribute Tense::FUTURE(Tense::PAST_);

const wchar_t *Modality::STRINGS[] = { L"Asserted", 
                                       L"Other" };

const ModalityAttribute Modality::ASSERTED(Modality::ASSERTED_);
const ModalityAttribute Modality::OTHER(Modality::OTHER_);

const wchar_t *Genericity::STRINGS[] = { L"Specific", 
                                         L"Generic" };

const GenericityAttribute Genericity::SPECIFIC(Genericity::SPECIFIC_);
const GenericityAttribute Genericity::GENERIC(Genericity::GENERIC_);

const wchar_t *PropositionStatus::STRINGS[] = {
	L"Default", 
	L"If",
	L"Future",
	L"Negative",
	L"Alleged",
	L"Modal",
	L"Unreliable"};
const PropositionStatusAttribute PropositionStatus::DEFAULT(PropositionStatus::DEFAULT_);
const PropositionStatusAttribute PropositionStatus::IF(PropositionStatus::IF_);
const PropositionStatusAttribute PropositionStatus::FUTURE(PropositionStatus::FUTURE_);
const PropositionStatusAttribute PropositionStatus::NEGATIVE(PropositionStatus::NEGATIVE_);
const PropositionStatusAttribute PropositionStatus::ALLEGED(PropositionStatus::ALLEGED_);
const PropositionStatusAttribute PropositionStatus::MODAL(PropositionStatus::MODAL_);
const PropositionStatusAttribute PropositionStatus::UNRELIABLE(PropositionStatus::UNRELIABLE_);

const wchar_t *MentionConfidenceStatus::STRINGS[] = {
	L"UnknownConfidence",
	L"AnyName",
	L"AmbiguousName",
	L"TitleDesc",
	L"CopulaDesc",
	L"ApposDesc",
	L"OnlyOneCandidateDesc",
	L"PrevSentDoubleSubjectDesc",
	L"OtherDesc",
	L"WhqLinkPron",
	L"NameAndPossPron",
	L"DoubleSubjectPersonPron",
	L"OnlyOneCandidatePron",
	L"PrevSentDoubleSubjectPron",
	L"OtherPron",
	L"NoEntity"
};
const MentionConfidenceAttribute MentionConfidenceStatus::UNKNOWN_CONFIDENCE(MentionConfidenceStatus::UNKNOWN_CONFIDENCE_);
const MentionConfidenceAttribute MentionConfidenceStatus::ANY_NAME(MentionConfidenceStatus::ANY_NAME_);
const MentionConfidenceAttribute MentionConfidenceStatus::AMBIGUOUS_NAME(MentionConfidenceStatus::AMBIGUOUS_NAME_);
const MentionConfidenceAttribute MentionConfidenceStatus::TITLE_DESC(MentionConfidenceStatus::TITLE_DESC_);
const MentionConfidenceAttribute MentionConfidenceStatus::COPULA_DESC(MentionConfidenceStatus::COPULA_DESC_);
const MentionConfidenceAttribute MentionConfidenceStatus::APPOS_DESC(MentionConfidenceStatus::APPOS_DESC_);
const MentionConfidenceAttribute MentionConfidenceStatus::ONLY_ONE_CANDIDATE_DESC(MentionConfidenceStatus::ONLY_ONE_CANDIDATE_DESC_);
const MentionConfidenceAttribute MentionConfidenceStatus::PREV_SENT_DOUBLE_SUBJECT_DESC(MentionConfidenceStatus::PREV_SENT_DOUBLE_SUBJECT_DESC_);
const MentionConfidenceAttribute MentionConfidenceStatus::OTHER_DESC(MentionConfidenceStatus::OTHER_DESC_);
const MentionConfidenceAttribute MentionConfidenceStatus::WHQ_LINK_PRON(MentionConfidenceStatus::WHQ_LINK_PRON_);
const MentionConfidenceAttribute MentionConfidenceStatus::NAME_AND_POSS_PRON(MentionConfidenceStatus::NAME_AND_POSS_PRON_);
const MentionConfidenceAttribute MentionConfidenceStatus::DOUBLE_SUBJECT_PERSON_PRON(MentionConfidenceStatus::DOUBLE_SUBJECT_PERSON_PRON_);
const MentionConfidenceAttribute MentionConfidenceStatus::ONLY_ONE_CANDIDATE_PRON(MentionConfidenceStatus::ONLY_ONE_CANDIDATE_PRON_);
const MentionConfidenceAttribute MentionConfidenceStatus::PREV_SENT_DOUBLE_SUBJECT_PRON(MentionConfidenceStatus::PREV_SENT_DOUBLE_SUBJECT_PRON_);
const MentionConfidenceAttribute MentionConfidenceStatus::OTHER_PRON(MentionConfidenceStatus::OTHER_PRON_);
const MentionConfidenceAttribute MentionConfidenceStatus::NO_ENTITY(MentionConfidenceStatus::NO_ENTITY_);

const wchar_t *Language::STRINGS[] = {
	L"Unspecified",
	L"English",
	L"Arabic",
	L"Chinese",
	L"Spanish",
	L"Farsi",
	L"Hindi",
	L"Bengali",
	L"Thai",
	L"Korean",
	L"Urdu"};
const wchar_t *Language::SHORT_STRINGS[] = {
	L"unspecified",
	L"en",
	L"ar",
	L"ch",
	L"es",
	L"fa",
	L"hi",
	L"bn",
	L"th",
	L"ko",
	L"ur"};
const LanguageAttribute Language::UNSPECIFIED(Language::UNSPECIFIED_);
const LanguageAttribute Language::ENGLISH(Language::ENGLISH_);
const LanguageAttribute Language::ARABIC(Language::ARABIC_);
const LanguageAttribute Language::CHINESE(Language::CHINESE_);
const LanguageAttribute Language::SPANISH(Language::SPANISH_);
const LanguageAttribute Language::FARSI(Language::FARSI_);
const LanguageAttribute Language::HINDI(Language::HINDI_);
const LanguageAttribute Language::BENGALI(Language::BENGALI_);
const LanguageAttribute Language::THAI(Language::THAI_);
const LanguageAttribute Language::KOREAN(Language::KOREAN_);
const LanguageAttribute Language::URDU(Language::URDU_);
