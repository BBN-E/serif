// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/ParamReader.h"
#include "Generic/wordnet/xx_WordNet.h"
#include "Spanish/relations/es_RelationUtilities.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/EntityType.h"
#include "Spanish/common/es_WordConstants.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Spanish/parse/es_STags.h"

//SymbolHash * SpanishRelationUtilities::_unreliabilityIndicators;
static Symbol transDeviceSym(L"TransportationDevice");
static Symbol vehicleSym(L"VEH");
static Symbol allegedly_sym(L"allegedly_sym");
static Symbol oblast_sym(L"oblast");
static Symbol one_sym(L"one");
static Symbol noone_sym(L"noone");
static Symbol nobody_sym(L"nobody");
static Symbol neither_sym(L"neither");
static Symbol no_one_sym(L"no-one");
static Symbol no_sym(L"no");
static Symbol apos_s_sym(L"'s");
static Symbol military_sym(L"Military");
static Symbol government_sym(L"Government");
static Symbol politicalParty_sym(L"Political-Party");
static Symbol nonGovernmental_sym(L"Non-Governmental");

static Symbol ofSym(L"of");
static Symbol inSym(L"in");
static Symbol atSym(L"at");
static Symbol nearSym(L"near");
static Symbol onSym(L"on");

Symbol SpanishRelationUtilities::WEA = Symbol(L"WEA");
Symbol SpanishRelationUtilities::VEH = Symbol(L"VEH");
Symbol SpanishRelationUtilities::PER_SOC = Symbol(L"PER-SOC");
Symbol SpanishRelationUtilities::PHYS_LOCATED = Symbol(L"PHYS.Located");
Symbol SpanishRelationUtilities::PHYS_NEAR = Symbol(L"PHYS.Near");
Symbol SpanishRelationUtilities::PART_WHOLE_GEOGRAPHICAL = Symbol(L"PART-WHOLE.Geographical");
Symbol SpanishRelationUtilities::PART_WHOLE_SUBSIDIARY = Symbol(L"PART-WHOLE.Subsidiary");
Symbol SpanishRelationUtilities::PART_WHOLE_ARTIFACT = Symbol(L"PART-WHOLE.Artifact");
Symbol SpanishRelationUtilities::ORG_AFF_EMPLOYMENT = Symbol(L"ORG-AFF.Employment");
Symbol SpanishRelationUtilities::ORG_AFF_OWNERSHIP = Symbol(L"ORG-AFF.Ownership");
Symbol SpanishRelationUtilities::ORG_AFF_FOUNDER = Symbol(L"ORG-AFF.Founder");
Symbol SpanishRelationUtilities::ORG_AFF_STUDENT_ALUM = Symbol(L"ORG-AFF.Student-Alum");
Symbol SpanishRelationUtilities::EDUCATIONAL = Symbol(L"Educational");
Symbol SpanishRelationUtilities::ORG_AFF_SPORTS_AFFILIATION = Symbol(L"ORG-AFF.Sports-Affiliation");
Symbol SpanishRelationUtilities::SPORTS = Symbol(L"Sports");
Symbol SpanishRelationUtilities::ORG_AFF_INVESTOR_SHAREHOLDER = Symbol(L"ORG-AFF.Investor-Shareholder");
Symbol SpanishRelationUtilities::ORG_AFF_MEMBERSHIP = Symbol(L"ORG-AFF.Membership");
Symbol SpanishRelationUtilities::ART_USER_OWNER_INVESTOR_MANUFACTURER = Symbol(L"ART.User-Owner-Inventor-Manufacturer");
Symbol SpanishRelationUtilities::Ges_AFF_CITIZes_RESIDENT_RELIGION_ETHNICITY = Symbol(L"GEN-AFF.Citizen-Resident-Religion-Ethnicity");
Symbol SpanishRelationUtilities::Ges_AFF_ORG_LOCATION = Symbol(L"GEN-AFF.Org-Location");
Symbol SpanishRelationUtilities::PER_SOC_SUBORDINATE = Symbol(L"PER-SOC.Subordinate");

int SpanishRelationUtilities::getRelationCutoff() {
	static bool init = false;
	static int relation_cutoff;
	if (!init) {
		relation_cutoff = ParamReader::getOptionalIntParamWithDefaultValue("relation_mention_dist_cutoff", 0);
		init = true;
		if (relation_cutoff < 0) {
			throw UnexpectedInputException("SpanishRelationUtilities::getRelationCutoff()",
				"Parameter 'relation_mention_dist_cutoff' must be greater than or equal to 0");
		}
	}
	return relation_cutoff;
}

std::vector<bool> SpanishRelationUtilities::identifyFalseOrHypotheticalProps(const PropositionSet *propSet,
                                                                      const MentionSet *mentionSet)
{
	std::vector<bool> isBad(propSet->getNPropositions(), false);

	static bool init = false;
	if (!init) {
		init = true;
		std::string filename = ParamReader::getParam("relation_unreliability_indicator_words");
		if (!filename.empty()) {
			_unreliabilityIndicators = _new SymbolHash(filename.c_str());
		} else _unreliabilityIndicators = _new SymbolHash(5);
	}

	// typical counter-example:
	// "if he had not worked for IBM, he wouldn't have paid her"
	// can we do anything with this?

	for (int k = 0; k < propSet->getNPropositions(); k++) {
		Proposition *prop = propSet->getProposition(k);

		if (prop->getNegation() != 0) {
			// "not just" == true
			/* DK todo
			if (prop->getAdverb() == 0 ||
				prop->getAdverb()->getHeadWord() != SpanishWordConstants::JUST)
			{
				isBad[prop->getIndex()] = true;
			}
			*/
		}

		if (prop->getAdverb() && prop->getAdverb()->getHeadWord() == allegedly_sym)
			isBad[prop->getIndex()] = true;

		const SynNode *modal = prop->getModal();
		if (modal != 0) {
			Symbol modalWord = modal->getHeadWord();
			/* DK todo
			
			if (modalWord == SpanishWordConstants::SHOULD ||
				modalWord == SpanishWordConstants::COULD ||
				modalWord == SpanishWordConstants::MIGHT ||
				modalWord == SpanishWordConstants::MAY)
			{
				isBad[prop->getIndex()] = true;
			}
			*/
		}

		// if/whether: p#
		// not ALWAYS accurate, but close enough
		for (int j = 0; j < prop->getNArgs(); j++) {
			Argument *arg = prop->getArg(j);
			/* DK todo
			
			if (arg->getType() == Argument::PROPOSITION_ARG &&
				(arg->getRoleSym() == SpanishWordConstants::IF ||
				arg->getRoleSym() == SpanishWordConstants::WHETHER))
			{
				isBad[arg->getProposition()->getIndex()] = true;
			}
			*/
		}

		if (prop->getPredType() == Proposition::VERB_PRED) {
			// get "if..." unrepresented in propositions
			const SynNode* node = prop->getPredHead();
			while (node != 0) {
				/* DK todo
			
				if (node->getTag() != SpanishSTags::VP &&
					node->getTag() != SpanishSTags::S &&
					node->getTag() != SpanishSTags::SBAR &&
					!node->isPreterminal())
					break;
				if (node->getTag() == SpanishSTags::SBAR &&
					(node->getHeadWord() == SpanishWordConstants::IF ||
					(node->getHeadIndex() == 0 &&
					node->getNChildren() > 1 &&
					node->getChild(1)->getTag() == SpanishSTags::IN &&
					node->getChild(1)->getHeadWord() == SpanishWordConstants::IF)))
				{
					isBad[prop->getIndex()] = true;
					break;
				}
				*/
				node = node->getParent();
			}
		}
	}

	for (int l = 0; l < propSet->getNPropositions(); l++) {
		Proposition *prop = propSet->getProposition(l);

		Symbol predicate = prop->getPredSymbol();
		if (predicate.is_null())
			continue;
		Symbol stemmed_predicate = WordNet::getInstance()->stem_verb(predicate);

		// he did not suspect that...
		// he did not know that...
		// ... maybe these should be exceptions? as in:
		// "mr. bakinyan did not suspect that he had been watched for a long time",
		//   where the implication is that, in fact, he had been watched...

		// hmm... all false props? not sure... maybe just negative ones?

		// [false prop] that: p# -- e.g. he didn't believe that X OR he didn't do X
		// ... but only allow verbs, no modifiers, e.g. he isn't happy that...
		// OR
		// [unreliable word] that: p# -- he denied that X OR he denied X

		if (_unreliabilityIndicators->lookup(stemmed_predicate) ||
			(isBad[prop->getIndex()] && prop->getPredType() == Proposition::VERB_PRED))
		{
			for (int j = 0; j < prop->getNArgs(); j++) {
				Argument *arg = prop->getArg(j);
				if (arg->getType() == Argument::PROPOSITION_ARG &&
					(arg->getRoleSym() == Argument::OBJ_ROLE ||
					 /* DK todo arg->getRoleSym() == SpanishWordConstants::THAT || */
					 arg->getRoleSym() == Argument::IOBJ_ROLE))
				{
					isBad[arg->getProposition()->getIndex()] = true;
				}
			}
		}
	}

	// non-propagating bad props
	for (int m = 0; m < propSet->getNPropositions(); m++) {
		Proposition *prop = propSet->getProposition(m);

		for (int j = 0; j < prop->getNArgs(); j++) {
			Argument *arg = prop->getArg(j);
			if (arg->getType() == Argument::MENTION_ARG &&
				(arg->getRoleSym() == Argument::OBJ_ROLE ||
				arg->getRoleSym() == Argument::SUB_ROLE ||
				arg->getRoleSym() == Argument::REF_ROLE ||
				arg->getRoleSym() == Argument::IOBJ_ROLE))
			{
				const SynNode *node = arg->getMention(mentionSet)->getNode();
				if (node->getHeadWord() == noone_sym ||
					node->getHeadWord() == no_one_sym ||
					node->getHeadWord() == nobody_sym ||
					node->getHeadWord() == apos_s_sym ||
					node->getChild(0)->getHeadWord() == neither_sym ||
					node->getChild(0)->getHeadWord() == no_sym)
				{
					isBad[prop->getIndex()] = true;
				}
			}
		}
	}

	return isBad;
}


bool SpanishRelationUtilities::coercibleToType(const Mention *ment, Symbol type) {
	static int init = 0;
	if (init == 0) {
		if (ParamReader::isParamTrue("coerce_types_for_events_and_relations"))	{
			init = 1;
		} else init = -1;
	}
	if (init < 0)
		return false;

	Symbol headword = ment->getNode()->getHeadWord();
	/* DK todo
	if (type == EntityType::getPERType().getName()) {
		
			
		if (headword == SpanishWordConstants::HE ||
			headword == SpanishWordConstants::HIM ||
			headword == SpanishWordConstants::HIS ||
			headword == SpanishWordConstants::SHE ||
			headword == SpanishWordConstants::HER ||
			headword == SpanishWordConstants::THEY ||
			headword == SpanishWordConstants::THEM ||
			headword == SpanishWordConstants::US ||
			headword == SpanishWordConstants::WE ||
			headword == SpanishWordConstants::OUR ||
			headword == SpanishWordConstants::I ||
			headword == SpanishWordConstants::ME ||
			headword == SpanishWordConstants::MY ||
			headword == SpanishWordConstants::YOU ||
			headword == SpanishWordConstants::YOUR)
			return true;
		else return WordNet::getInstance()->isPerson(headword);
	} else if (type == EntityType::getLOCType().getName()) {
		if (headword == SpanishWordConstants::HOME ||
			headword == SpanishWordConstants::ABROAD ||
			headword == SpanishWordConstants::OVERSEAS ||
			headword == SpanishWordConstants::HERE ||
			headword == SpanishWordConstants::THERE ||
			headword == oblast_sym)
			return true;
		else return WordNet::getInstance()->isLocation(headword);
	} else if (type == EntityType::getORGType().getName()) {
		return WordNet::getInstance()->isHyponymOf(headword,"organization");
	} else if (type == EntityType::getFACType().getName()) {
		return (WordNet::getInstance()->isHyponymOf(headword,"facility") ||
				WordNet::getInstance()->isHyponymOf(headword,"office") ||
				WordNet::getInstance()->isHyponymOf(headword,"stairs") ||
				WordNet::getInstance()->isHyponymOf(headword,"dwelling") ||
				WordNet::getInstance()->isHyponymOf(headword,"entryway") ||
				WordNet::getInstance()->isHyponymOf(headword,"building"));
	} else if (type == transDeviceSym || type == vehicleSym) {
		return (WordNet::getInstance()->isHyponymOf(headword,"vehicle"));
	} else */return false;
}


void SpanishRelationUtilities::artificiallyStackPrepositions(PotentialRelationInstance *instance)
{
	instance->setPredicate(instance->getLeftHeadword());
	instance->setStemmedPredicate(stemPredicate(instance->getPredicate(), Proposition::NOUN_PRED));
	instance->setLeftRole(Argument::REF_ROLE);
}

Symbol SpanishRelationUtilities::stemPredicate(Symbol word, Proposition::PredType predType)
{

	if (predType == Proposition::NOUN_PRED) {
		return WordNet::getInstance()->stem_noun(word);
	} else if (predType == Proposition::VERB_PRED || predType == Proposition::COPULA_PRED) 	{
		return WordNet::getInstance()->stem_verb(word);
	} else if (predType == Proposition::MODIFIER_PRED || predType == Proposition::POSS_PRED) {
		return word;
	} else {
		//SessionLogger::warn("") << "unknown predicate type in PotentialRelationInstance::setStandardInstance(): "
		//	<< Proposition::getPredTypeString(predType);
		return PotentialRelationInstance::CONFUSED_SYM;
	}
}

Symbol SpanishRelationUtilities::stemWord(Symbol word, Symbol pos) {

	if (LanguageSpecificFunctions::isNPtypePOStag(pos)) {
		return WordNet::getInstance()->stem_noun(word);
	} else /* DK todo if (pos == SpanishSTags::VB ||
		pos == SpanishSTags::VBD ||
		pos == SpanishSTags::VBG ||
		pos == SpanishSTags::VBN ||
		pos == SpanishSTags::VBP ||
		pos == SpanishSTags::VBZ)
	{
		return WordNet::getInstance()->stem_verb(word);
	} else */ return word;
}

int SpanishRelationUtilities::getOrgStack(const Mention *mention, Mention **orgs, int max_orgs) {
	if (mention->getMentionType() != Mention::LIST)
		return 0;

	// valid options:
	//	SET: E1 E2
	//	SET: E1 E2 E3
	//  SET: E1 (SET: E2 E3)

	// silly, what is a stack of size 1
	if (max_orgs < 2)
		return 0;

	// test for ands
	Symbol results[100];
	mention->getNode()->getTerminalSymbols(results, 99);
	int n = mention->getNode()->getNTerminals();
	for (int i = 0; i < n; i++)
		if (results[i] == SpanishWordConstants::Y)
			return 0;

	int n_orgs = 0;

	Mention *child = mention->getChild();
	if (child->getEntityType().matchesORG() || child->getEntityType().matchesFAC())
	{
		orgs[n_orgs++] = child;
		Mention *next = child->getNext();
		if (next != 0) {
			if (next->getEntityType().matchesORG() || next->getEntityType().matchesFAC())
				orgs[n_orgs++] = next;
			else return 0;

			if (next->getNext() == 0)
				return n_orgs;
			else if ((next->getNext()->getEntityType().matchesORG() ||
				      next->getNext()->getEntityType().matchesFAC()) &&
				next->getNext()->getNext() == 0 &&
				max_orgs >= 3)
			{
				orgs[n_orgs++] = next->getNext();
				return n_orgs;
			}
		} else if (next != 0 && next->getMentionType() == Mention::LIST) {
			child = next->getChild();
			if (child != 0 &&
				(child->getEntityType().matchesORG() ||
				 child->getEntityType().matchesFAC()) &&
				child->getNext() != 0 &&
				(child->getNext()->getEntityType().matchesORG() ||
				 child->getNext()->getEntityType().matchesFAC()) &&
				child->getNext()->getNext() == 0 &&
				max_orgs >= 3)
			{
				orgs[n_orgs++] = child;
				orgs[n_orgs++] = child->getNext();
				return n_orgs;
			}
		}
	}
	return 0;
}


bool SpanishRelationUtilities::isPrepStack(RelationObservation *o) {

	RelationPropLink *link = o->getPropLink();
	if (link->isEmpty())
		return false;

	const SynNode *firstPP = o->getMention1()->getNode()->getParent();
	const SynNode *secondPP = o->getMention2()->getNode()->getParent();
	if (firstPP == 0 || secondPP == 0 ||
		firstPP->getTag() != Symbol(L"PP") ||
		secondPP->getTag() != Symbol(L"PP"))
		return false;
	const SynNode *parent = firstPP->getParent();
	if (parent == 0 || parent != secondPP->getParent())
		return false;
	bool stacked = false;
	for (int i = 0; i < parent->getNChildren() - 1; i++) {
		if (parent->getChild(i) == firstPP) {
			if (parent->getChild(i+1) == secondPP) {
				stacked = true;
				break;
			} else return false;
		}
	}
	if (!stacked) return false;

	EntityType leftType = o->getMention1()->getEntityType();
	EntityType rightType = o->getMention2()->getEntityType();

	// let's only do this for types we understand well
	if (!leftType.matchesFAC() &&
		!leftType.matchesGPE() &&
		!leftType.matchesLOC() &&
		!leftType.matchesORG() &&
		!leftType.matchesPER())
		return false;

	// no persons on the right side
	if (!rightType.matchesFAC() &&
		!rightType.matchesGPE() &&
		!rightType.matchesLOC() &&
		!rightType.matchesORG())
		return false;

	// usually names don't get stacked under descriptors...
	//   "he was killed in St. Petersburg in his apartment" != stack
	//   exception: "he was killed on Prospect Street in the city of Moscow"...
	//     but what can you do?
	if (o->getMention1()->getMentionType() == Mention::NAME &&
		o->getMention2()->getMentionType() == Mention::DESC)
		return false;

	Symbol leftRole = link->getArg1Role();
	Symbol rightRole = link->getArg2Role();

	if (leftRole == ofSym) {
		if ((leftType.matchesGPE() || leftType.matchesLOC()) &&
			(rightType.matchesGPE() || rightType.matchesLOC()))
		{
			if (o->getMention1()->getMentionType() == Mention::DESC &&
				o->getMention2()->getMentionType() == Mention::NAME)
			{
				// get the heck out of here...
				//   this is probably something like "republic of Chechnya"
				return false;
			}
		}
	}

	if (leftRole == ofSym) {

		// of the building  in   Russia
		//        people    on   the street
		//        cafe      at   the Kremlin
		//        mountains near the lake
		//        north     of   the city
		if (rightRole == onSym ||
			rightRole == atSym ||
			rightRole == nearSym ||
			rightRole == inSym ||
			rightRole == ofSym)
		{
			//std::cerr << "Found PS: " << link->getTopStemmedPred().to_debug_string() << " ";
			//std::cerr << leftRole.to_debug_string() << " " << rightRole.to_debug_string() << "\n";
			return true;
		} else return false;

	}

	// don't use PER unless preceded by of (too dangerous)
	if (leftType.matchesPER()) {
		return false;
	}

	// "in" and "of" are usually good bets on the right-hand side
	//   (unless the lefthand side is a person, but we've already taken care of that)
	if (rightRole == inSym ||
		rightRole == ofSym)
	{
		//std::cerr << "Found PS: " << link->getTopStemmedPred().to_debug_string() << " ";
		//std::cerr << leftRole.to_debug_string() << " " << rightRole.to_debug_string() << "\n";
		return true;
	}

	// "on" and "near" are OK bets for the right-hand side, but
	//   only if the lefthand side is "in" or "at"
	if (rightRole == onSym || rightRole == nearSym)
	{
		if (leftRole == inSym || leftRole == atSym)
		{
			//std::cerr << "Found PS: " << link->getTopStemmedPred().to_debug_string() << " ";
			//std::cerr << leftRole.to_debug_string() << " " << rightRole.to_debug_string() << "\n";
			return true;
		} else return false;

	}
	return false;
}

bool SpanishRelationUtilities::isValidRelationEntityTypeCombo(Symbol validation_type, 
										   const Mention* m1, const Mention* m2, Symbol relType)
{
	if(wcscmp(validation_type.to_string(), L"NONE") == 0){
		return true;
	}
	if(wcscmp(validation_type.to_string(), L"2005") == 0){
		if(is2005ValidRelationEntityTypeCombo(m1, m2, relType)) return true;
		if(is2005ValidRelationEntityTypeCombo(m2, m1, relType)) return true;
		return false;
	}
	if(wcscmp(validation_type.to_string(), L"2005_ORDERED") == 0){
		if(is2005ValidRelationEntityTypeCombo(m1, m2, relType)) return true;
		return false;
	}
	return true;
}
bool SpanishRelationUtilities::is2005ValidRelationEntityTypeCombo(const Mention* arg1, const Mention* arg2,
														   Symbol relType )
{

	const Symbol& PER = EntityType::getPERType().getName();
	const Symbol& ORG = EntityType::getORGType().getName();
	const Symbol& LOC = EntityType::getLOCType().getName();
	const Symbol& FAC = EntityType::getFACType().getName();
	const Symbol& GPE = EntityType::getGPEType().getName();

	const Symbol& relCat = RelationConstants::getBaseTypeSymbol(relType);
	const Symbol& relSubtype = RelationConstants::getSubtypeSymbol(relType);

	const Symbol& arg1_type = arg1->getEntityType().getName();
	const Symbol& arg1_subtype = EntitySubtype::getUndetType().getName();
	if(arg1->getEntitySubtype().isDetermined()){
		const Symbol& arg1_subtype = arg1->getEntitySubtype().getName();
	}
	const Symbol& arg2_type = arg2->getEntityType().getName();
	const Symbol& arg2_subtype = EntitySubtype::getUndetType().getName();
	if(arg2->getEntitySubtype().isDetermined()){
		const Symbol& arg2_subtype = arg1->getEntitySubtype().getName();
	}
	if(relCat == PER_SOC){
		if((arg1_type ==  PER) && (arg2_type == PER)){
			return true;
		}
		return false;
	}
	if(relType == PHYS_LOCATED){
		if(arg1_type == PER){
			if((arg2_type == GPE) || (arg2_type == LOC) || (arg2_type == FAC)){
					return true;
				}
		}
		return false;
	}
	else if(relType == PHYS_NEAR){
		if((arg1_type== PER)|| (arg1_type == GPE) || (arg1_type == LOC) || 
			(arg1_type == FAC))
		{
			if((arg2_type == GPE) || (arg2_type == LOC) || (arg2_type == FAC)){
				return true;
			}
		}
		return false;
	}
	else if(relType == PART_WHOLE_GEOGRAPHICAL){
		if((arg1_type == GPE) || (arg1_type == LOC) ||(arg1_type == FAC))
		{
			if((arg2_type == GPE) || (arg2_type == LOC) || (arg2_type == FAC)){
				return true;
			}
		}
		return false;
	}
	else if(relType == PART_WHOLE_SUBSIDIARY){
		if((arg1_type == ORG) )
		{
			if((arg2_type == GPE) || (arg2_type == ORG)){
				return true;
			}
		}
		return false;
	}
	else if(relType == PART_WHOLE_ARTIFACT){
		if(arg1_type != arg2_type){
			return false;
		}
		if((arg1_type == VEH) ||(arg1_type == WEA) )
		{
			if((arg2_type == VEH) ||(arg2_type == WEA) ){
				return true;
			}
		}
		return false;
	}		
	else if(relType == ORG_AFF_EMPLOYMENT){
		if(arg1_type == PER){
			if( (arg2_type == GPE) || (arg2_type == ORG) ){
				return true;
			}
		}
		return false;
	}
	else if(relType == ORG_AFF_OWNERSHIP){
		if(arg1_type == PER){
			if( arg2_type == ORG){
				return true;
			}
		}
		return false;
	}
	else if(relType == ORG_AFF_FOUNDER){
		if((arg1_type == PER) || (arg1_type == ORG)){
			if( (arg2_type == GPE) || (arg2_type == ORG) ){
				return true;
			}
		}
		return false;
	}
	else if(relType == ORG_AFF_STUDENT_ALUM){
		if(arg1_type == PER){
			if( (arg2_type == ORG) ){
				if(arg2_subtype == EntitySubtype::getUndetType().getName()){
					//subtype hasn't been determined, so keep the relation
					return true;
				}
				if(arg2_subtype == EDUCATIONAL){
					return true;
				}
			}
		}
		return false;
	}
	else if(relType == ORG_AFF_SPORTS_AFFILIATION){
		if(arg1_type == PER){
			if( (arg2_type == ORG) ){
				if(arg2_subtype == EntitySubtype::getUndetType().getName()){
					//subtype hasn't been determined, so keep the relation
					return true;
				}
				if(arg2_subtype == SPORTS){
					return true;
				}
			}
		}
		return false;
	}

	else if(relType == ORG_AFF_INVESTOR_SHAREHOLDER){
		if((arg1_type == PER) || (arg1_type == ORG) || (arg1_type == GPE)){
			if( (arg2_type == GPE) || (arg2_type == ORG) ){
				return true;
			}
		}
		return false;
	}
	else if(relType == ORG_AFF_MEMBERSHIP){
		if((arg1_type == PER) || (arg1_type == ORG) || (arg1_type == GPE)){
			if( (arg2_type == ORG) ){
				return true;
			}
		}
		return false;
	}
	else if(relType == ART_USER_OWNER_INVESTOR_MANUFACTURER){
		if((arg1_type == PER) || (arg1_type == ORG) || (arg1_type == GPE)){
			if( (arg2_type == WEA) || (arg2_type == VEH) || (arg2_type == FAC) ){
				return true;
			}
		}
		return false;
	}
	else if(relType == Ges_AFF_CITIZes_RESIDENT_RELIGION_ETHNICITY){
		if((arg1_type == PER) ){
			if( (arg2_type == ORG)|| (arg2_type == GPE) || (arg2_type == LOC) ||
				(arg2_type == PER)){
				return true;
			}
		}
		return false;
	}
	else if(relType == Ges_AFF_ORG_LOCATION){
		if((arg1_type == ORG) ){
			if( (arg2_type == GPE) || (arg2_type == LOC) ){
				return true;
			}
		}
		return false;
	}
	// ignore metonymy for now
	
	return true;
}

int SpanishRelationUtilities::calcMentionDist(const Mention *m1, const Mention *m2) {
	int start_ment1 = getMentionStartToken(m1);
	int end_ment1 = getMentionEndToken(m1);
	int start_ment2 = getMentionStartToken(m2);
	int end_ment2 = getMentionEndToken(m2);
	
	if ((start_ment1 < 0) || (end_ment1 < 0) || (start_ment2 < 0) || (end_ment2 < 0)) {
		return getRelationCutoff() + 50;
	}

	int dist = start_ment2 - end_ment1;
	if (dist < 0) 
		dist = start_ment1 - end_ment2;
	if (dist < 0)
		dist = 0;

	return dist;
	
}

bool SpanishRelationUtilities::distIsLessThanCutoff(const Mention *m1, const Mention *m2) {
	int dist = calcMentionDist(m1, m2);
	if (dist < getRelationCutoff()) 
		return true;
	else
		return false;
}

int SpanishRelationUtilities::getMentionStartToken(const Mention *m1) {
	int start_ment1 = -1;

	if (m1->getHead() != 0) {
		start_ment1 = m1->getHead()->getStartToken();
	}
	else if (m1->getNode() != 0) {
		const SynNode *node = m1->getNode();
		if (!node->isPreterminal()) {
			while (node->getHead() != 0 && !node->getHead()->isPreterminal())
				node = node->getHead();
		}
		start_ment1 = node->getStartToken();
	}

	return start_ment1;
}

int SpanishRelationUtilities::getMentionEndToken(const Mention *m1){
	int end_ment1 = -1;

	if (m1->getHead() != 0) {
		end_ment1 = m1->getHead()->getEndToken();
	}
	else if (m1->getNode() != 0) {
		const SynNode *node = m1->getNode();
		if (!node->isPreterminal()) {
			while (node->getHead() != 0 && !node->getHead()->isPreterminal())
				node = node->getHead();
		}
		end_ment1 = node->getEndToken();
	}

	return end_ment1;
}

// ATEA hack -- we invented new subtypes, but didn't retain relation model
Symbol SpanishRelationUtilities::mapToTrainedOnSubtype(Symbol subtype) {
	if (subtype == military_sym)
		return government_sym;
	if (subtype == politicalParty_sym)
		return nonGovernmental_sym;

	return subtype;
}

bool SpanishRelationUtilities::isATEARelationType(Symbol type) {
	return type == PER_SOC_SUBORDINATE;
}
