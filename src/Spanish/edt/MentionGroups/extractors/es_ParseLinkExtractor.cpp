// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/edt/MentionGroups/extractors/es_ParseLinkExtractor.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/AttributeValuePair.h"
#include "Spanish/common/es_WordConstants.h"
#include "Spanish/parse/es_STags.h"
#include <boost/regex.hpp>

namespace {
	const bool DEBUG=true;
}

const Symbol SpanishParseLinkExtractor::PSEUDO_APPOSITIVE(L"pseudo-appositive");
const Symbol SpanishParseLinkExtractor::X_AND_HIS_Y(L"x-and-his-y");
const Symbol SpanishParseLinkExtractor::X_IS_Y(L"x-is-y");
const Symbol SpanishParseLinkExtractor::PAREN_DEF(L"paren-def");

SpanishParseLinkExtractor::SpanishParseLinkExtractor() 
	: AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"es-parse-link"))
{
}

namespace {
	std::wstring getChildTags(const SynNode* node) {
		std::wstringstream out;
		for (int i=0;i<node->getNChildren(); ++i) {
			out << L"<" << node->getChild(i)->getTag() << L">";
		}
		return out.str();
	}

}

std::vector<AttributeValuePair_ptr> SpanishParseLinkExtractor::extractFeatures(const Mention& context,
																					LinkInfoCache& cache,
																					const DocTheory *docTheory) 
{
	std::vector<AttributeValuePair_ptr> results;

	if (const Mention *m1 = findPseudoAppositive(context, cache, docTheory)) {
		coerceOtherOrUndet(context, m1);
		results.push_back(AttributeValuePair<const Mention*>::create(PSEUDO_APPOSITIVE, m1, getFullName()));
	}

	if (const Mention *m2 = findXandHisY(context, cache, docTheory)) {
		coerceOtherOrUndet(context, m2);
		results.push_back(AttributeValuePair<const Mention*>::create(X_AND_HIS_Y, m2, getFullName()));
	}

	if (const Mention *m3 = findXisY(context, cache, docTheory)) {
		coerceOtherOrUndet(context, m3);
		results.push_back(AttributeValuePair<const Mention*>::create(X_IS_Y, m3, getFullName()));
	}

	if (const Mention *m4 = findParenDef(context, cache, docTheory)) {
		coerceOtherOrUndet(context, m4);
		results.push_back(AttributeValuePair<const Mention*>::create(PAREN_DEF, m4, getFullName()));
	}

	return results;
}

// Looking for: (**SN* SNP (SN (GRUP.NOM -lrb word -rrb)))
const Mention *SpanishParseLinkExtractor::findParenDef(const Mention& context,
														  LinkInfoCache& cache,
														  const DocTheory *docTheory) 
{
	if (context.getMentionType() != Mention::NAME || 
		context.getEntityType() == EntityType::getPERType() ||
		context.getEntityType() == EntityType::getGPEType())
		return 0;

	const SynNode *node = context.node;

	int parenDefPos = node->getNChildren()-1;
	if (node->getChild(parenDefPos)->getTag()==SpanishSTags::POS_FC)
		--parenDefPos;
	if (parenDefPos<0)
		return 0;
	const SynNode *parenDef = node->getChild(parenDefPos);
	Symbol t1 = parenDef->getFirstTerminal()->getTag();
	Symbol t2 = parenDef->getLastTerminal()->getTag();
	if (!((SpanishWordConstants::isOpenBracket(t1) && SpanishWordConstants::isClosedBracket(t2)) ||
		  (SpanishWordConstants::isOpenDoubleBracket(t1) && SpanishWordConstants::isClosedDoubleBracket(t2))))
		return 0;
		
	return const_cast<Mention*>(&context)->getMentionSet()->getMentionByNode(parenDef);
}

// Looking for: (S (**SN**) (GRUP.VERB (vai (es))) (**SN**) fp?)
const Mention *SpanishParseLinkExtractor::findXisY(const Mention& context,
														  LinkInfoCache& cache,
														  const DocTheory *docTheory) 
{
	if (context.getEntityType() == EntityType::getGPEType())
		return 0;

	const SynNode *node = context.node;

	// Check the parent's children.
	const SynNode *parent = node->getParent();
	if (parent==0)
		return 0;
	Symbol pTag = parent->getTag();
	if ((pTag!=SpanishSTags::SENTENCE) &&
		(pTag!=SpanishSTags::S) && pTag!= SpanishSTags::S_DP_SBJ && pTag != SpanishSTags::SENTENCE_DP_SBJ)
		//(pTag!=SpanishSTags::S_A) &&
		//(pTag!=SpanishSTags::S_F_A) &&
		//(pTag!=SpanishSTags::S_F_C) &&
		//(pTag!=SpanishSTags::S_F_R) &&
		//(pTag!=SpanishSTags::S_NF_P))
		return 0;
	if ((parent->getNChildren()<3) ||
		(parent->getChild(0) != node) ||
		(parent->getChild(1)->getTag() != SpanishSTags::GRUP_VERB) ||
		(!parent->getChild(2)->hasMention()) ||
		(parent->getNChildren()>4) ||
		((parent->getNChildren()==4) && (parent->getChild(3)->getTag()!=SpanishSTags::POS_FP)))
		return 0;

	// Check the GRUP.VERB's children.
	const SynNode *verb = parent->getChild(1);
	if (verb->getEndToken() != verb->getStartToken())
		return 0; // must be a one-word verb.
	if (verb->getHeadPreterm()->getTag() != SpanishSTags::POS_VAI)
		return 0; // must be a form of "is" -- should we only accept a limited set of words here?

	const SynNode *targetSyn = parent->getChild(2);
	return const_cast<Mention*>(&context)->getMentionSet()->getMentionByNode(targetSyn);
}

// Looking for: (**SN** SN CONJ (SN (SPEC (**dp**)) GRUP.NOM))
const Mention *SpanishParseLinkExtractor::findXandHisY(const Mention& context,
															  LinkInfoCache& cache,
															  const DocTheory *docTheory) 
{
	if (context.getEntityType() == EntityType::getGPEType())
		return 0;

	const SynNode *node = context.node;

	// Check top level.
	if ((node->getTag()!=SpanishSTags::SN) || node->getNChildren()!=3 ||
		(!node->getChild(0)->hasMention()) ||
		(node->getChild(1)->getTag()!=SpanishSTags::CONJ) ||
		(!node->getChild(2)->hasMention()))
		return 0;

	// Check spec
	const SynNode *hisY = node->getChild(2);
	if ((hisY->getNChildren()<2) ||
		(hisY->getChild(0)->getTag() != SpanishSTags::SPEC) ||
		(hisY->getChild(0)->getChild(0)->getTag() != SpanishSTags::POS_DP))
		return 0;

	const SynNode *his = hisY->getChild(0)->getChild(0);
	return const_cast<Mention*>(&context)->getMentionSet()->getMentionByNode(his);
}

// Looking for: (**SN** (SPEC (da el)) (GRUP.NOM XXX ,? (**SN** YYY))).
const Mention* SpanishParseLinkExtractor::findPseudoAppositive(const Mention& context,
																	  LinkInfoCache& cache,
																	  const DocTheory *docTheory) 
{
	if (context.getEntityType() == EntityType::getGPEType())
		return 0;

	static const boost::wregex TOP_PATTERN
		(L"(<SPEC>)?<GRUP.NOM>(<fc>)?");
	static const boost::wregex GRUP_NOM_PATTERN
		(// GRUP.NOM begins with nominal stuff (XXX)
		 //L"((<SNP>(<fc>)?)|(<np>|<nc>|<S.A>|<SP>|<fc>|<S.NF.P>|<SADV>)+)(<fg>)?"
		 L"((<SNP>|<SN>|<np>|<nc>|<S.A>|<SP>|<fc>|<S.NF.P>|<SADV>)+)(<fg>)?"
		 // Followed by the phrase we want to link (YYY)
		 L"(<SN>|<SNP>)"
		 // Optionally followed by some modifiers
		 L"(<fc>|<SP>|<S.F.R>|<S.A>|<S.NF.P>)*");

	const SynNode *node = context.node;

	if ((node->getTag()!=SpanishSTags::SN) || node->getNChildren()>3)
		return 0; // root node is not SN
	
	//std::cerr << "A1 " << getChildTags(node) << std::endl;
	if (!boost::regex_match(getChildTags(node), TOP_PATTERN))
		return 0; // root node has inappropriate children.

	// Find the GRUP.NOM node.
	const SynNode *grup_nom = 0;
	for (int i=0;i<node->getNChildren(); ++i) {
		const SynNode *child = node->getChild(i);
		if (child->getTag()==SpanishSTags::GRUP_NOM)
			grup_nom = child;
	}
	if (grup_nom == 0)
		throw InternalInconsistencyException("SpanishParseLinkExtractor::extractFeatures", 
											 "No GRUP.NOM found, despite regex match");
	
	//std::cerr << "A2 " << getChildTags(grup_nom) << std::endl;
	if (!boost::regex_match(getChildTags(grup_nom), GRUP_NOM_PATTERN))
		return 0; // grup.nom node has inappropriate children.

	// Find the target SN node.
	const SynNode *target = 0;
	for (int i=0;i<grup_nom->getNChildren(); ++i) {
		const SynNode *child = grup_nom->getChild(i);
		if ((child->getTag()==SpanishSTags::SN) || (child->getTag()==SpanishSTags::SNP))
			target = child;
	}
	if (target == 0)
		throw InternalInconsistencyException("SpanishParseLinkExtractor::extractFeatures", 
											 "No SN(P) found, despite regex match");
	
	Mention *m1 = const_cast<Mention*>(&context);
	Mention *m2 = m1->getMentionSet()->getMentionByNode(target);
	if (!m2) return 0;

	Mention::Type m2MentType = m2->getMentionType();
	if (!(m2 && (m2MentType == Mention::NAME || m2MentType == Mention::DESC || 
				 m2MentType == Mention::PART || m2MentType == Mention::PRON))) 
		return 0;

	return m2;
}

void SpanishParseLinkExtractor::coerceOtherOrUndet(const Mention& context, const Mention* target) {
	Mention *m1 = const_cast<Mention*>(&context);
	Mention *m2 = const_cast<Mention*>(target);
	// This is a little evil: if one mention is UNDET or OTH, then
	// coerce it to the type we're linking it to.  If we don't do
	// this, then the mergers will never consider the pair, since
	// mentions with type other/undet do not get added as mention
	// groups (except for pronouns).
	bool m1_is_other = ((m1->getEntityType() == EntityType::getOtherType()) || 
						(m1->getEntityType() == EntityType::getUndetType()));
	bool m2_is_other = ((m2->getEntityType() == EntityType::getOtherType()) || 
						(m2->getEntityType() == EntityType::getUndetType()));
	if (m1_is_other && !m2_is_other)
		m1->setEntityType(m2->getEntityType());
	else if (m2_is_other && !m1_is_other)
		m2->setEntityType(m1->getEntityType());
}


