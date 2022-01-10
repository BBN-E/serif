// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <boost/foreach.hpp>

#include "Generic/common/SessionLogger.h"
#include "Generic/theories/MentionConfidence.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"

/**********************************************************
 * The is English-specific, copied from
 * DistillUtilities in distill-generic.  We should eventually
 * resolve this somehow...
 *********************************************************/

Symbol EN_CC = Symbol(L"CC");
Symbol EN_COMMA = Symbol(L",");
Symbol EN_WHNP = Symbol(L"WHNP");
Symbol EN_SBAR = Symbol(L"SBAR");
Symbol EN_NP = Symbol(L"NP");
Symbol EN_WPDOLLAR = Symbol(L"WP$");
Symbol EN_WHADVP = Symbol(L"WHADVP");
Symbol EN_NN = Symbol(L"NN");
Symbol EN_NNS = Symbol(L"NNS");
Symbol EN_NNP = Symbol(L"NNP");
Symbol EN_NNPS = Symbol(L"NNPS");
Symbol EN_NPP = Symbol(L"NPP");

/* MRF this is the same as enNodeInfo::isNominalPremod except it excludes Adjective premods */
bool MentionConfidence::enIsNominalPremod(const SynNode *node) {
	Symbol tag = node->getTag();
	if (( tag == EN_NN ||
		tag == EN_NNS ||
		tag == EN_NNP ||
		tag == EN_NNPS)	&&
		node->getParent() != 0 &&
		node->getParent()->getTag() != EN_NPP &&
		node->getParent()->getHead() != node)
	{
		const SynNode *parent = node->getParent();
		for (int i = parent->getHeadIndex();
			 i > 0 && parent->getChild(i) != node; i--)
		{
			if (parent->getChild(i)->getTag() == EN_COMMA)
				return false;
		}

		return true;
	}
	else {
		return false;
	}
}
/*********************************************************************/

/* This is a modified version to return the name mention portion of the mention if there is one */
const Mention* MentionConfidence::nameFromTitleDescriptor(const Mention* desc_mention, const Entity *ent, const MentionSet* mentionSet)
{
	if (desc_mention->getMentionType() != Mention::DESC){
		return 0;
	}
	//find the immediately following name 
	const Mention* parentMention = 0;
	const SynNode *dnode = desc_mention->getNode();
	if (!enIsNominalPremod(dnode))	//must be nominal premod descriptor
		return 0;

	while(dnode->getParent()){
		if (dnode->getParent()->hasMention() && 
			mentionSet->getMention(dnode->getParent()->getMentionIndex())->getMentionType() == Mention::NAME){
			parentMention =  mentionSet->getMention(dnode->getParent()->getMentionIndex());
			break;
		}
		else{
			dnode = dnode->getParent();
		}
	}
	if (parentMention == 0)
		return 0;
		
	if (!ent->containsMention(parentMention))
		return 0;

	if (parentMention->getMentionType() != Mention::NAME)
		return 0;

	const SynNode *node = parentMention->node;
	const SynNode *NPPnode = parentMention->node->getHeadPreterm()->getParent();
	if (node == NPPnode)
		return 0;
	node = NPPnode->getParent();
	if (node == 0)
		return 0;

	// node must have at least two children
	int nkids = node->getNChildren();
	if (nkids < 2)
		return 0;

	// no commas or CCs allowed
	for (int j = 0; j < node->getNChildren(); j++) {
		const SynNode *child = node->getChild(j);
		if (child->getTag() == EN_CC || child->getTag() == EN_COMMA) {
			return 0;
		}
	}

	// rightmost child must be a name mention
	const SynNode *child = node->getChild(nkids-1);
	if (!child->hasMention())
		return 0;
	const Mention *rightmost_ment = mentionSet->getMentionByNode(child);
	if (rightmost_ment->getMentionType() == Mention::NAME) {
		// keep it
	}
	else if (rightmost_ment->getMentionType() == Mention::NONE) {
		// make sure this is the name child
		if (rightmost_ment->getParent() != parentMention)
			return 0;
	}
	else {
		// who knows what kind of mention *this* is
		return 0;
	}

	if (!rightmost_ment->isOfRecognizedType())
		return 0;

	// second-next-rightmost child must be a nominal premod descriptor
	child = node->getChild(nkids-2);
	if (!child->hasMention() || !enIsNominalPremod(child))
		return 0;

	const Mention *next_ment = mentionSet->getMentionByNode(child);
	if (next_ment->getMentionType() != Mention::DESC)
	{
		return 0;
	}
	// make sure that the descriptor mention and the name mention link to the same entity
	if(next_ment->getUID() == desc_mention->getUID()){
		// I doubt this  -- try the right-msot-mention instead
		// return next_ment;
		return rightmost_ment;
	}
	else{
		return 0;
	}	
}

/* The logic of this is copied almost entirely  from en_PreLinker::getTitle() */
bool MentionConfidence::isTitleDescriptor(const Mention* desc_mention, const Entity *ent, const MentionSet* mentionSet)
{
	const Mention * nameMention = nameFromTitleDescriptor(desc_mention, ent, mentionSet);
	if (nameMention == 0) return false;
	else return true;
}

/* returns the mention that is a name if there is one that meets all criteria  */
const Mention* MentionConfidence::nameFromCopulaDescriptor(const Mention* ment, const Entity* ent, const PropositionSet* prop_set, const MentionSet* mentionSet)
{
	if(ment->getMentionType() != Mention::DESC){
		return 0;
	}
	std::vector<Proposition*> copulas;
	for(int p_no = 0; p_no < prop_set->getNPropositions(); p_no++){
		if(prop_set->getProposition(p_no)->getPredType() == Proposition::COPULA_PRED){
			copulas.push_back(prop_set->getProposition(p_no));
		}
	}
	if(copulas.size() == 0){
		return 0;
	}
	for(std::vector<Proposition*>::iterator iter = copulas.begin(); iter != copulas.end(); iter++){
		Proposition* prop = (*iter);
		if(prop->getArg(0)->getType() != Argument::MENTION_ARG || prop->getArg(1)->getType() != Argument::MENTION_ARG){
			continue;
		}
		const Mention *lhs = prop->getArg(0)->getMention(mentionSet);

		const Mention *rhs = prop->getArg(1)->getMention(mentionSet);
		if(!lhs || !rhs){
			std::ostringstream ostr;
			ostr<<"MentionConfidence::isCopulaDescriptor(), no lhs/rhs: ";
			ment->dump(ostr);
			prop->dump(ostr);
			ostr<<std::endl;
			ent->dump(ostr);
			ostr<<std::endl;
			SessionLogger::dbg("mention_confidence") << ostr.str();
		}
		else if(lhs->getUID() == ment->getUID()){
			if((rhs->getMentionType() == Mention::NAME) 
				&& (ent->containsMention(rhs)))
			{
				return rhs;
			}
		}
		else if(rhs->getUID() == ment->getUID()) {
			if((lhs->getMentionType() == Mention::NAME)
				&& (ent->containsMention(lhs)))
			{
				return lhs;
			}
		}
	}
	return 0;
}

/* The technique for doing this specified in the PreLinker is more efficient, but this fits into the framework better */
bool MentionConfidence::isCopulaDescriptor(const Mention* ment, const Entity* ent, const PropositionSet* prop_set, const MentionSet* mentionSet)
{
	const Mention * nameMention = nameFromCopulaDescriptor(ment, ent, prop_set, mentionSet);
	if (nameMention == 0) return false;
	else return true;
}

const Mention * MentionConfidence::nameFromApposDescriptor(const Mention* ment, const Entity* ent)
{
	if(ment->getMentionType() != Mention::DESC){
		return 0;
	}
	const Mention* parent = ment->getParent();
	if(!parent){
		return 0;
	}
	if(parent->mentionType != Mention::APPO){
		return 0;
	}
	const Mention* child1 = parent->getChild();
	const Mention* child2 = child1->getNext();
	if(!child1){
		std::ostringstream ostr;
		ostr<<"MentionConfidence::nameFromApposDescriptor(), no Child1: ";
		ment->dump(ostr);
		parent->dump(ostr);
		ostr<<std::endl;
		SessionLogger::dbg("mention_confidence") << ostr.str();
		return 0;
	}
	if(!child2){
		std::ostringstream ostr;
		ostr<<"MentionConfidence::nameFromApposDescriptor(), no Child2: ";
		ment->dump(ostr);
		parent->dump(ostr);
		ostr<<std::endl;
		SessionLogger::dbg("mention_confidence") << ostr.str();
		return 0;
	}
	if((child1->getUID() == ment->getUID()) 
		&& (child2->getMentionType() == Mention::NAME) 
		&& ent->containsMention(child2))
	{
		return child2;
	}
	if((child2->getUID() == ment->getUID())
		&& (child1->getMentionType() == Mention::NAME)
		&& ent->containsMention(child1))
	{
		return child1;
	}
	return 0;		
}

bool MentionConfidence::isApposDescriptor(const Mention* ment, const Entity* ent) {
	const Mention * nameMention = nameFromApposDescriptor(ment, ent);
	if (nameMention == 0) return false;
	else return true;
}

const Mention* MentionConfidence::getWHQLink(const Mention *currMention, const MentionSet *mentionSet) 
{
	const SynNode *pronNode = currMention->node;
	const SynNode *parent = pronNode->getParent();
	if (parent == 0)
		return NULL;

	if (pronNode->getTag() == EN_WHNP || pronNode->getTag() == EN_WHADVP) {
		if (parent->getTag() == EN_SBAR) {
			parent = parent->getParent();
			if (parent == 0) return NULL;
			if (parent->getTag() == EN_SBAR) {
				parent = parent->getParent();
				if (parent == 0) return NULL;
			}
			if (parent->getTag() == EN_NP) {
				return mentionSet->getMentionByNode(parent);
			} else return NULL;
		} else return NULL;
	}

	if (pronNode->getTag() == EN_WPDOLLAR) {
		if (parent->getTag() == EN_WHNP) {
			parent = parent->getParent();
			if (parent == 0) return NULL;
			if (parent->getTag() == EN_SBAR) {
				parent = parent->getParent();
				if (parent == 0) return NULL;
				if (parent->getTag() == EN_SBAR) {
					parent = parent->getParent();
					if (parent == 0) return NULL;
				}
				if (parent->getTag() == EN_NP) {
					return mentionSet->getMentionByNode(parent);
				} else return NULL;
			} else return NULL;
		} else return NULL;
	}

	return NULL;
}

const Mention* MentionConfidence::nameFromPronNameandPos(const Mention *ment, const Entity *ent, const MentionSet * mentionSet, const EntitySet* ent_set, const TokenSequence *toks )
{
	if(ment->getHead() == 0){
		SessionLogger::dbg("mention_confidence")<<"MentionConfidence::nameFromPronNameandPos() returning false for a headless mention"<<std::endl;
		return 0;
	}
	std::wstring ment_str = ment->getHead()->toTextString();
	std::transform(ment_str.begin(), ment_str.end(), ment_str.begin(), towlower);
	if(ment_str == L"his " || ment_str == L"her "  || ment_str == L"its "){	//note: toTextString() adds a space at the end
		//check if the word before the poss. pron is 'and' or ','
		int ment_start_tok = ment->getHead()->getStartToken();
		
		if(ment_start_tok <2 )
			return 0;
		Symbol prev_word = toks->getToken(ment_start_tok - 1)->getSymbol();
		if(prev_word != Symbol(L"and")){
			return 0;
		}
		for(int om_no = 0; om_no <mentionSet->getNMentions(); om_no++){
			const Mention* oth_ment = mentionSet->getMention(om_no);
			if(oth_ment->getMentionType() != Mention::NAME){ //must be a name
				continue;
			}
			if(oth_ment->getHead()->getEndToken() + 2 != ment_start_tok){ //must be exactly two words apart			
				continue;
			}
			const Entity* oth_ent = ent_set->getEntityByMention(oth_ment->getUID());
			if(oth_ent == 0){
				continue;
			}
			if(oth_ent->getID() == ent->getID()){ //must be the same entity
				return oth_ment;
			}

		}
	}
	return 0;
}

bool MentionConfidence::isPronNameandPos(const Mention *ment, const Entity *ent, 
	const MentionSet * mentionSet, const EntitySet* ent_set, const TokenSequence *toks )
{
	const Mention* nm = nameFromPronNameandPos(ment, ent, 
		mentionSet, ent_set, toks);
	if (nm == 0) return false;
	else return true;
}

const Mention* MentionConfidence::nameFromDoubleSubjectPersonPron(const Mention *ment, const Entity *ent, const MentionSet * mentionSet, const EntitySet* ent_set, const PropositionSet * propSet)
{	
	if (ment->getMentionType() != Mention::PRON || !ment->getEntityType().matchesPER())
		return 0;
	int ment_token = ment->getNode()->getStartToken();
	const Mention *namePrecedingMention = 0;
	for (int m = 0; m < mentionSet->getNMentions(); m++) {
		const Mention *other_ment = mentionSet->getMention(m);
		if (other_ment->getMentionType() != Mention::NAME || !other_ment->getEntityType().matchesPER())
			continue;
		if (other_ment->getNode()->getEndToken() < ment_token) {
			if (namePrecedingMention != 0)
				return 0;
			namePrecedingMention = other_ment;
		}
	}
	if (namePrecedingMention == 0)
		return 0;
	if (ent_set->getEntityByMention(namePrecedingMention->getUID()) != ent)
		return 0;
	bool ment_is_subject = false;
	bool name_is_subject = false;
	for (int p = 0; p < propSet->getNPropositions(); p++) {
		const Proposition* prop = propSet->getProposition(p);
		for (int a = 0; a < prop->getNArgs(); a++) {
			if (prop->getArg(a)->getRoleSym() == Argument::SUB_ROLE) {
				const Argument *arg = prop->getArg(a);
				if (arg->getType() != Argument::MENTION_ARG)
					continue;
				const Mention *argMent = arg->getMention(mentionSet);
				if (argMent->getUID() == ment->getUID()) {
					ment_is_subject = true;
				}
				if (argMent->getUID() == namePrecedingMention->getUID()) {
					name_is_subject = true;
				}
			}
		}
	}

	if (ment_is_subject && name_is_subject){
		return namePrecedingMention;
	}else{
		return 0;
	}
}

std::string MentionConfidence::getDebugSentenceString(const DocTheory *doc_theory, int sent_no){
	TokenSequence *tseq = doc_theory->getSentenceTheory(sent_no)->getTokenSequence();
	return tseq->toDebugString(0, tseq->getNTokens()-1);
}
std::string MentionConfidence::getDebugTextFromMention(const DocTheory *docTheory, const Mention *ment){
	TokenSequence* tokenSequence = docTheory->getSentenceTheory(ment->getSentenceNumber())->getTokenSequence();	
	const SynNode* mentNode = ment->getNode();
	return tokenSequence->toDebugString(mentNode->getStartToken(), mentNode->getEndToken());
}
/* returns all mentions in sentence that are of requested mention type and which are linked
 * to an entity of the requested Entity type.
 */
std::vector<const Mention *> MentionConfidence::mentionsMTypeEType(const DocTheory *docTheory, const Mention::Type mType, EntityType entType, int sentNo){
	const SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentNo);
	const MentionSet *mentionSet = sentTheory->getMentionSet();
	std::vector<const Mention *> winners;
	for (int m = 0; m < mentionSet->getNMentions(); m++) {
		const Mention *other_ment = mentionSet->getMention(m);
		if ( mType != other_ment->getMentionType())
			continue;
		// we treat ORG and GPE as equivalent entity types for detecting reference conflicts
		EntityType om_etype = other_ment->getEntityType();
		if ((entType != om_etype) &&
			!(entType.matchesORG() && om_etype.matchesGPE()) &&
			!(entType.matchesGPE() && om_etype.matchesORG()))
				continue;
		winners.push_back(other_ment);
	}
	return winners;
}
/* true if mention appears in the sentence and has the role that matches the requested role
*/
bool MentionConfidence::isMentInSentWithArgRole(const Mention *ment, const DocTheory *docTheory, Symbol role, int sent_no){
	const SentenceTheory *sentTheory = docTheory->getSentenceTheory(sent_no);
	const MentionSet *mentionSet = sentTheory->getMentionSet();
	const PropositionSet *propSet = sentTheory->getPropositionSet();
	for (int p = 0; p < propSet->getNPropositions(); p++) {
		const Proposition* prop = propSet->getProposition(p);
		for (int a = 0; a < prop->getNArgs(); a++) {
			if (prop->getArg(a)->getRoleSym() == role) {
				const Argument *arg = prop->getArg(a);
				if (arg->getType() != Argument::MENTION_ARG)
					continue;
				const Mention *arg_ment = arg->getMention(mentionSet);
				if (arg_ment->getUID() == ment->getUID())
					return true;
			}
		}
	}
	return false;
}


/* true if mention is a PRON or DESC and 
*  mention is the SUBJ of a prop in sentence N and
*  mention shares an entity with a named mention in sentence N-1 and 
*  the named mention is the SUBJ of a prop in sentence N-1.
*  and there is no other named SUBJ mention of the same entity type 
*     between these two mentions nor earlier on sentence N
*     that is not tied to the same entity
*/
bool MentionConfidence::isMentPrevSentDoubleSubj(const Mention *ment, const Entity *ent, const EntitySet *entitySet, const DocTheory *docTheory){
	bool verbose = false;

	int ment_sent_no = ment->getSentenceNumber();
	if (ment_sent_no < 1) 
		return false;
	int prev_sent_no = ment_sent_no - 1;
	const Mention::Type mentType = ment->getMentionType();
	const SentenceTheory *prevSentTheory = docTheory->getSentenceTheory(prev_sent_no);
	const SentenceTheory *mentSentTheory = docTheory->getSentenceTheory(ment_sent_no);
	EntityType entType = ent->getType();
	if (mentType!= Mention::PRON && mentType != Mention::DESC)
		return false;

	int ment_token = ment->getNode()->getStartToken();
	std::string mentStr = getDebugTextFromMention(docTheory, ment);

	if (verbose){
		std::cerr << "\nMentPrevSentDouble called for ment =>"<< mentStr << "<= in sentence " << ment_sent_no <<std::endl;
		std::cerr << getDebugSentenceString(docTheory, ment_sent_no-1) << std::endl;
		std::cerr << getDebugSentenceString(docTheory, ment_sent_no) << std::endl;
	}
	if (mentStr == "who" || mentStr == "which"){
		if (verbose)
			std::cerr << "mentPrevSentDouble failed because ment was WH pronoun" << std::endl;
		return false;
	}
	
	bool ment_is_subject = isMentInSentWithArgRole(ment, docTheory, Argument::SUB_ROLE, ment_sent_no);	
	if (!ment_is_subject){
		if (verbose) 
			std::cerr << "mentPrevSentDouble failed because ment is not SUBJ" << std::endl;
		return false;
	}

	std::vector<const Mention *> prev_sentence_names = mentionsMTypeEType(docTheory, Mention::NAME, entType, prev_sent_no);
	if (prev_sentence_names.size() == 0){
		if (verbose)
			std::cerr << "mentPrevSentDouble failed for lack of names in previous sentence" << std::endl;
		return false;
	}
	std::vector<const Mention *> same_ent_prev_sent_name_ments;
	std::vector<const Mention *> wrong_ent_names;
	BOOST_FOREACH(const Mention *prev_name, prev_sentence_names) {
		if (entitySet->getEntityByMention(prev_name->getUID()) == ent)
			same_ent_prev_sent_name_ments.push_back(prev_name);
		else 
			wrong_ent_names.push_back(prev_name);
	}

	int early_name_token = ment_token;  // initially set to an untrue upper limit
	int late_name_token = -1;  // initially set to an untrue lower limit
	int first_name_token = ment_token; //intially set to untrue upper limit
	bool name_is_subject = false;
	const Mention *same_ent_prev_sent_name_ment = 0;
	BOOST_FOREACH(const Mention *prev_ent_name, same_ent_prev_sent_name_ments) {
		if (isMentInSentWithArgRole(prev_ent_name, docTheory, Argument::SUB_ROLE, prev_sent_no)){
			name_is_subject = true;
			same_ent_prev_sent_name_ment = prev_ent_name;
			int nt = prev_ent_name->getNode()->getStartToken();
			if (nt < early_name_token)
				early_name_token = nt;
			if (nt > late_name_token)
				late_name_token = nt;
		}
	}
	const Mention *bad_subject = 0;
	const Mention *early_subject = 0;
	const Mention *intruding_subject = 0;

	//std::cerr << "checking ment sentence for preceding subj NAME contenders" <<std::endl;
	std::vector<const Mention *> ment_sent_names = mentionsMTypeEType(docTheory, Mention::NAME, entType, ment_sent_no);
	BOOST_FOREACH(const Mention *ment_sent_name, ment_sent_names) {
		if (isMentInSentWithArgRole(ment_sent_name, docTheory, Argument::SUB_ROLE, ment_sent_no)){
			int nt = ment_sent_name->getNode()->getStartToken();
			if (nt < ment_token && entitySet->getEntityByMention(ment_sent_name->getUID()) != ent){
				first_name_token = nt;
				intruding_subject = ment_sent_name;
			}
		}
	}

	if (!name_is_subject){
		if (verbose)
			std::cerr << "\nbasic MentPrevSentDouble loses since name is not  SUBJ " <<std::endl;
		return false;
	}
	if (verbose){
		std::string entStr = getDebugTextFromMention(docTheory, same_ent_prev_sent_name_ment);
		std::cerr << "basic MentPrevSentDouble wins linking " << mentStr << " to name " << entStr << std::endl;
		//std::cerr << getDebugSentenceString(docTheory, ment_sent_no-1) << std::endl;
		//std::cerr << getDebugSentenceString(docTheory, ment_sent_no) << std::endl;
	}


	BOOST_FOREACH(const Mention *wrong_name, wrong_ent_names) {
		if (isMentInSentWithArgRole(wrong_name, docTheory, Argument::SUB_ROLE, prev_sent_no)){
			int nt = wrong_name->getNode()->getStartToken();
			if (nt < early_name_token)
				early_subject = wrong_name;
			else if (nt > late_name_token)
				bad_subject = wrong_name;
		}
	}
	if (bad_subject != 0){
		if (verbose){
			std::string badMentStr = getDebugTextFromMention(docTheory, bad_subject);
			std::cerr << "***********MentPrevSentDouble loses for bad_subj_name " << badMentStr << std::endl;
		}
		return false;
	}
	if (early_subject != 0){
		if (verbose){
			std::string earlyMentStr = getDebugTextFromMention(docTheory, early_subject);
			std::cerr << "***********MentPrevSentDouble loses for earlier subj in prev sentence " << earlyMentStr << std::endl;
		}
		return false;
	}
	if (intruding_subject != 0){
		if (verbose){
			std::string intruderMentStr = getDebugTextFromMention(docTheory, intruding_subject);
			std::cerr << "***********MentPrevSentDouble loses for same sent early subj_name " << intruderMentStr << std::endl;
		}
		return false;
	}
	return true;
}


bool MentionConfidence::isDoubleSubjectPersonPron(const Mention *ment, const Entity *ent, const MentionSet * mentSet, const EntitySet* ent_set, const PropositionSet * propSet)
{
	const Mention *nm = nameFromDoubleSubjectPersonPron(ment, 
							ent, mentSet, ent_set, propSet);
	if (nm == 0) return false;
	else return true;
}
/* true if the name of entity precedes mention in an earlier sentence */
bool MentionConfidence::entityNamePrecedesMentSentence(const Mention* ment, const Entity *ent, const EntitySet *ent_set){
	int sent_no = ment->getSentenceNumber();	
	/// walk all mentions of this ent, looking for ones in earlier sentences
	for (int oment_no = 0; oment_no < ent->getNMentions(); oment_no++){
		MentionUID oment_uid = ent->getMention(oment_no);
		int osent_no = oment_uid.sentno();
		if (osent_no >= sent_no)
			continue;
		Mention *oth_ment = ent_set->getMention(oment_uid);
		if (oth_ment->getMentionType() == Mention::NAME) 
			return true;
	}

	return false;
}

/* true if the name of entity precedes (or is) mention in the same sentence */
bool MentionConfidence::entityNamePrecedesMentInSentence(const Mention* ment, const Entity *ent, const EntitySet *entitySet, const MentionSet* mentionSet){
	if (ment->getMentionType() == Mention::NAME) 
		return true;
	int ment_token = ment->getNode()->getStartToken();
	for (int m = 0; m < mentionSet->getNMentions(); m++) {
		const Mention *other_ment = mentionSet->getMention(m);
		const Entity *other_ent = entitySet->getEntityByMention(other_ment->getUID());
		if ((other_ment->getMentionType() != Mention::NAME) || (!other_ent) ||
			(ent->getID() != other_ent->getID()))
			continue;
		if (other_ment->getNode()->getEndToken() < ment_token) 
			return true;
	}
	return false;
}

/* true if this entity is the only one of the matching type mentioned in this mention's sentence or earlier */
bool MentionConfidence::isMentOnlyPrecedingTypeMatch(const Mention* ment, EntityType entType, const Entity *ent, const EntitySet *ent_set){
	if(!(ment->getEntityType() == entType)){
		return false;
	}
	int sent_no = ment->getSentenceNumber();
	for (int ent_no = 0; ent_no < ent_set->getNEntities(); ent_no++){
		const Entity* oth_ent = ent_set->getEntity(ent_no);
		if (oth_ent->getType() == entType && oth_ent->getID() != ent->getID()){ 
			for (int oment_no = 0; oment_no < oth_ent->getNMentions(); oment_no++){
				MentionUID oment_uid = oth_ent->getMention(oment_no);
				int osent_no = oment_uid.sentno();
				if (osent_no <= sent_no){
					return false;
				}
			}
		}
	}
	return true;
}

bool MentionConfidence::namePrecedesMentAndIsOnlyPrecedingTypeMatch(const Mention* ment, EntityType entType, const Entity *ent,const EntitySet *entitySet, const MentionSet *mentionSet, const DocTheory *docTheory){
	bool basic = isMentOnlyPrecedingTypeMatch(ment, entType, ent, entitySet);
	if (!basic) 
		return false;
	bool preSentence = entityNamePrecedesMentSentence(ment, ent, entitySet);
	bool inSent = entityNamePrecedesMentInSentence(ment, ent, entitySet, mentionSet);

	bool verbose = false;
	if (verbose){
		int sn = ment->getSentenceNumber();
		if (!preSentence)
			std::cerr << "no name referent in earlier sentence -- entityNamePrecedes Ment sentence  FALSE " << std::endl;
		if (!inSent)
			std::cerr << "no name referent preceding in sentence -- entityNamePrecedes Ment in sent false " << std::endl;
		if (!preSentence || !inSent){
			std::string mentStr = getDebugTextFromMention(docTheory, ment);
			std::string sentStr = getDebugSentenceString(docTheory, sn);
			std::cerr << "  was trying ment " << mentStr << " \nsentence " << sentStr << std::endl;
		}
	}
	return (preSentence || inSent);
}

MentionConfidenceAttribute MentionConfidence::determineMentionConfidence(const DocTheory *docTheory, const SentenceTheory *sentTheory, const Mention *ment, const std::set<Symbol>& ambiguousLastNames) 
{
	const EntitySet *entitySet = docTheory->getEntitySet();		
	const Entity* ent = entitySet->getEntityByMention(ment->getUID());
	if (ent == 0)
		return MentionConfidenceStatus::NO_ENTITY;
	const Mention::Type mentType = ment->getMentionType();

	if (mentType == Mention::NAME && ent->getType().matchesPER() && ambiguousLastNames.size() > 0) {
		const SynNode *head = ment->getHead();
		int n_terminals = head->getNTerminals();
		if (n_terminals == 1 && ambiguousLastNames.find(head->getHeadWord()) != ambiguousLastNames.end()) {
			//std::cout << "Ambiguous last name: " << head->getHeadWord().to_debug_string() << "\n";
			return MentionConfidenceStatus::AMBIGUOUS_NAME;
		}
	}

	if (mentType == Mention::NAME)
		return MentionConfidenceStatus::ANY_NAME;

	const MentionSet *mentionSet = sentTheory->getMentionSet();
	if (mentType == Mention::APPO) {
		return MentionConfidenceStatus::APPOS_DESC;
	} 
	EntityType entType = ent->getType();
	const PropositionSet *propSet = sentTheory->getPropositionSet();
	if (mentType == Mention::DESC) {
		if (isTitleDescriptor(ment, ent, mentionSet)) {
			return MentionConfidenceStatus::TITLE_DESC;
		} else if (isCopulaDescriptor(ment, ent, propSet, mentionSet)) {
			return MentionConfidenceStatus::COPULA_DESC;
		} else if (isApposDescriptor(ment, ent)) {
			return MentionConfidenceStatus::APPOS_DESC;
		} else if (namePrecedesMentAndIsOnlyPrecedingTypeMatch(ment, entType, ent, 
												 entitySet, mentionSet, docTheory)){
			return MentionConfidenceStatus::ONLY_ONE_CANDIDATE_DESC;
		} else if (isMentPrevSentDoubleSubj(ment, ent, entitySet, docTheory)){
			return MentionConfidenceStatus::PREV_SENT_DOUBLE_SUBJECT_DESC;
		} else return MentionConfidenceStatus::OTHER_DESC;
	}
	if (mentType == Mention::PRON) {
		const Mention *whqLinkedMent = getWHQLink(ment, mentionSet);
		if (whqLinkedMent != NULL) {
			const Entity* whoLinkedEnt = entitySet->getEntityByMention(whqLinkedMent->getUID());
			if (whoLinkedEnt == ent) {
				return MentionConfidenceStatus::WHQ_LINK_PRON;
			}
		}
		if (isPronNameandPos(ment, ent, mentionSet, entitySet, sentTheory->getTokenSequence())){
			return MentionConfidenceStatus::NAME_AND_POSS_PRON;
		} else if (isDoubleSubjectPersonPron(ment, ent, mentionSet, entitySet, propSet)) {
			return MentionConfidenceStatus::DOUBLE_SUBJECT_PERSON_PRON;
		} else if (isMentOnlyPrecedingTypeMatch(ment, entType, ent, entitySet)) {
			return MentionConfidenceStatus::ONLY_ONE_CANDIDATE_PRON;
		} else if (isMentPrevSentDoubleSubj(ment, ent, entitySet, docTheory)){
			return MentionConfidenceStatus::PREV_SENT_DOUBLE_SUBJECT_PRON;
		} else return MentionConfidenceStatus::OTHER_PRON;
	}
	return MentionConfidenceStatus::UNKNOWN_CONFIDENCE;
}
