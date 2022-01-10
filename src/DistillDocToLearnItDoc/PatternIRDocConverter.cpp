#include "common/leak_detection.h" // this must be the first #include

#include "PatternIRDocConverter.h"

#include <vector>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <string.h>
#include "common/ParamReader.h"
#include "theories/DocTheory.h"
#include "theories/Value.h"
#include "theories/TokenSequence.h"
#include "theories/SynNode.h"
#include "Generic/PropTree/PropNode.h"
#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/TokenSpanPFeature.h"
#include "Generic/theories/ValueMentionSet.h"
#include "LearnIt/Target.h"
#include "LearnIt/LearnItPattern.h"
#include "Annotation.h"

using namespace std;
using namespace boost;

vector<PatternIRDocConverter::AnnotationRegex> PatternIRDocConverter::REGEXES;

wstring PatternIRDocConverter::FULL_TEXT=L"<f>"; 
wstring PatternIRDocConverter::_FULL_TEXT=L"</f>";
wstring PatternIRDocConverter::MENTION_ANN=L"m";
wstring PatternIRDocConverter::MIN_ENT_DESC_ANN=L"ed";
wstring PatternIRDocConverter::MIN_ENT_NAME_ANN=L"en";
wstring PatternIRDocConverter::MENTION_NAME=L"n";
wstring PatternIRDocConverter::MENTION_DESC=L"d";

void PatternIRDocConverter::processSentence(const DocTheory* dt, 
											int sn, wostream& out) 
{
	SentenceTheory* st=dt->getSentenceTheory(sn);
	const EntitySet* es=dt->getEntitySet();

	printSentenceFullText(dt, st, es, out);
}

void PatternIRDocConverter::printSentenceFullText(const DocTheory* dt,
												SentenceTheory* st,
												const EntitySet* es,
												wostream& out) 
{
	LearnItDocConverter::indent(3, out);
	out << FULL_TEXT;
	
	vector<Annotation> annotationsByStart, annotationsByEnd;
	addValueAnnotations(st, annotationsByStart);
	addMentionAnnotations(st, es, annotationsByStart);
	addRegexAnnotations(dt, st, annotationsByStart);

	// sort annotations by printing orders of their start and
	// end tags
	// note: assumes no crossing annotations (e.g. <a><b></a></b>)
	annotationsByEnd.insert(annotationsByEnd.begin(), annotationsByStart.begin(),
		annotationsByStart.end());
	sort(annotationsByStart.begin(), annotationsByStart.end(), 
		annotation_start_compare());
	sort(annotationsByEnd.begin(), annotationsByEnd.end(), 
		annotation_end_compare());
	
	vector<Annotation>::const_iterator curAnnStart=annotationsByStart.begin();
	vector<Annotation>::const_iterator curAnnEnd=annotationsByEnd.begin();
	
	const TokenSequence* ts=st->getTokenSequence();
	
	for (int t_idx=0; t_idx<ts->getNTokens(); ++t_idx) {
		const Token* tok=ts->getToken(t_idx);
		// opening tags
		for (;curAnnStart!=annotationsByStart.end() && curAnnStart->start_token==t_idx; 
			++curAnnStart) 
		{
			out << L"<" << curAnnStart->annotationString << L">";	
		}
	
		out << tok->getSymbol().to_string();
	
		// ending tags
		for (;curAnnEnd!=annotationsByEnd.end() && curAnnEnd->end_token==t_idx; 
			++curAnnEnd) 
		{
			out << L"</" << curAnnEnd->annotationString << L">";	
		}

		out << L" ";
	}

	out << _FULL_TEXT << L"\n";
}


void PatternIRDocConverter::initAnnotationRegexes() {
	// This list needs to be kept in sync with patterns/brandy_regexp.py
	// We prefix the names with "R" to differentiate them from slot annotations
	// which may share the same name (e.g. DATE)
	if (REGEXES.empty()) {
		REGEXES.push_back(make_pair(L"R_DATE", wstring(L"(?i)\\d\\d[/-]\\d\\d([/-]\\d\\d(\\d\\d)?)?|((jan(uary)?|feb(ruary)?|mar(ch)|apr(il)?|may|jun(e)?|jul(y)?|"
	      L"aug(ust)?|sept?(ember)?|oct(ober)?|nov(ember)?|dec(ember)?).?) \\d\\d(\\d\\d)?(( ,)? \\d\\d(\\d\\d)?)?")));
		REGEXES.push_back(make_pair(L"R_X_YEAR_OLD", L"\\d+-year -?old"));
		REGEXES.push_back(make_pair(L"R_YEAR", L"[12]\\d\\d\\d"));
		REGEXES.push_back(make_pair(L"R_INT", L"\\d+"));
		REGEXES.push_back(make_pair(L"R_NUMBER", L"\\d+(\\.\\d+)?"));
	}
}


void PatternIRDocConverter::addRegexAnnotations(const DocTheory* dt, 
		SentenceTheory* st, vector<Annotation>& annotations)
{
	if (REGEXES.empty()) {initAnnotationRegexes(); } 
	BOOST_FOREACH(const AnnotationRegex& regex, REGEXES) {
		// create a fake Learnit pattern for the refex
		const wstring full_pattern=
			L"(regex (wordsets) (entitylabels) (reference) (toplevel (regex(re (text (string \"" 
			+ regex.second + L"\"))))))";
		LearnItPattern_ptr p=LearnItPattern::create(make_shared<Target>(L"fake target", 
			vector<SlotConstraints_ptr>(), vector<SlotPairConstraints_ptr>(),
			L"fake", vector<vector<int> >()), 
			L"fake", full_pattern, false, false, 0.0f, 0.0f);

		BOOST_FOREACH(PatternFeatureSet_ptr sfs, p->applyToSentence(dt, *st)) {
			for (size_t feat_index =0; feat_index < sfs->getNFeatures(); ++feat_index) {
				PatternFeature_ptr sf = sfs->getFeature(feat_index);
				if (TokenSpanPFeature_ptr tsf = dynamic_pointer_cast<TokenSpanPFeature>(sf)) {
					annotations.push_back(Annotation(regex.first, 
						tsf->getStartToken(), tsf->getEndToken()));
				}
			}
		}
	}
}

void PatternIRDocConverter::addValueAnnotations(const SentenceTheory* st, 
							vector<Annotation>& annotations) 
{
	const ValueMentionSet* vms=st->getValueMentionSet();
	for (int vm_idx=0; vm_idx<vms->getNValueMentions(); ++vm_idx) {
		const ValueMention* vm=vms->getValueMention(vm_idx);
		if (vm->getSentenceNumber()==st->getSentNumber()) {
			const wstring annotationString=vm->isTimexValue()?
				vm->getSubtype().to_string():vm->getType().to_string();
			annotations.push_back(Annotation(annotationString, 
				vm->getStartToken(), vm->getEndToken()));
		}
	}
}


void PatternIRDocConverter::addMentionAnnotations(const SentenceTheory* st, 
								const EntitySet* es, 
								vector<Annotation>& annotations)
{
	const MentionSet* ms=st->getMentionSet();
	for (int m_idx=0; m_idx<ms->getNMentions(); ++m_idx) {
		const Mention* m=ms->getMention(m_idx);
		const SynNode* syn=m->getNode();
		const Entity* entity=es->getEntityByMentionWithoutType(m->getUID());

		annotations.push_back(Annotation(MENTION_ANN, 
			m->getNode()->getStartToken(), m->getNode()->getEndToken()));
		if (m->mentionType==Mention::NAME) {
			annotations.push_back(Annotation(MENTION_NAME, 
				m->getNode()->getStartToken(), m->getNode()->getEndToken()));
		} else if (m->mentionType==Mention::DESC) {
			annotations.push_back(Annotation(MENTION_DESC, 
				m->getNode()->getStartToken(), m->getNode()->getEndToken()));
		}
		if (entity && syn) {
			int start_token=syn->getStartToken();
			int end_token=syn->getEndToken();

			// entity type annotation (e.g. ORG, PER, etc.)
			// we let this through even if no_coref is on
			const wstring typeString=entity->getType().getName().to_string();
			
			const wstring subtypeString=es->guessEntitySubtype(entity).getName().to_string();
			if (!typeString.empty()) {
				annotations.push_back(Annotation(typeString, start_token, end_token));
			}
			if (!subtypeString.empty() && (subtypeString != L"UNDET")) {
				annotations.push_back(Annotation(subtypeString, start_token, end_token));
			}

			if (!ParamReader::isParamTrue("no_coref")) {
				// min-entitylevel annotation
				// min-entitylevel can require a NAME mention in the entity (MEL_NAME)
				// or either a NAME or a DESC mention (MEL_DESC)
				pair<bool, bool> hasNameDesc=entityHasNameDescMention(entity, es);
				bool has_name=hasNameDesc.first, has_desc=hasNameDesc.second;

				if (has_desc || has_name) {
					annotations.push_back(Annotation(MIN_ENT_DESC_ANN, start_token,
						end_token));	
					if (has_name) {
						annotations.push_back(Annotation(MIN_ENT_NAME_ANN, 
							start_token, end_token));
					}
				}
			}
		} else if (syn) {
			int start_token=syn->getStartToken();
			int end_token=syn->getEndToken();
			const wstring typeString=m->getEntityType().getName().to_string();
			if (!typeString.empty()) {
				annotations.push_back(Annotation(typeString, start_token, end_token));
			}
		}
	}
}

pair<bool, bool> PatternIRDocConverter::entityHasNameDescMention(
	const Entity* entity, const EntitySet* es) 
{
	bool has_name=false, has_desc=false;
	for (int i=0; i<entity->getNMentions(); ++i) {
		MentionUID m_id=entity->getMention(i);
		const Mention* m=es->getMention(m_id);
		if (m->getMentionType()==Mention::DESC) {
			has_desc=true;
		} else if (m->getMentionType()==Mention::NAME) {
			has_name=true;
		}
	}
	return make_pair(has_name, has_desc);
}

