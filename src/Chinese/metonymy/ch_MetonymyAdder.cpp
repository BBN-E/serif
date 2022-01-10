// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Chinese/metonymy/ch_MetonymyAdder.h"
#include "Chinese/parse/ch_STags.h"

//verbs with loc subject
static Symbol sym_hold_meeting = Symbol(L"\x4e3e\x884c"); // \344\270\276\350\241\214 /to hold (a meeting, ceremony)/
static Symbol sym_convene = Symbol(L"\x53ec\x5f00"); //345\217\254\345\274\200 /convene (a conference or meeting)/to convoke/to call together/

// verbs with loc object
static Symbol sym_go_to = Symbol(L"\x8d74"); // \350\265\264
static Symbol sym_come = Symbol(L"\x6765");  // \346\235\245
static Symbol sym_come_from = Symbol(L"\x6765\x81ea"); // \346\235\245\350\207\252
static Symbol sym_is_situated = Symbol(L"\x4f49\x4e8e"); // \344\275\215\344\272\216 (leave out?)
static Symbol sym_to_a_place = Symbol(L"\x5230"); // \345\210\260 /to (a place)/until (a time)/
static Symbol sym_invest = Symbol(L"\x6295\x8d44"); // \346\212\225\350\265\204 /investment/to invest/
static Symbol sym_arrive = Symbol(L"\x81f3"); // \350\207\263 /arrive/to/until/
static Symbol sym_stationed_in = Symbol(L"\x9a7b"); // \351\251\273 /resident in/located in/stationed in/to station (troops)/
static Symbol sym_all_over = Symbol(L"\x904d\x5e01"); // \351\201\215\345\270\203 /be found everywhere/(spread) all over/suffuse/
static Symbol sym_debouchment = Symbol(L"\x8fdb\x51fa"); // \350\277\233\345\207\272 /debouchment/
static Symbol sym_located_at2 = Symbol(L"\x8bbe\x5728"); // \350\256\276\345\234\250 
static Symbol sym_depart = Symbol(L"\x79bb"); // \347\246\273 /(distant) from/to leave/to depart/to go away/trample/
static Symbol sym_reach = Symbol(L"\x62b5\x8fbd"); // \346\212\265\350\276\276
static Symbol sym_visited = Symbol(L"\x8bbf\x95ed"); // \350\256\277\351\227\256

// prepositions with loc object
static Symbol sym_located_at = Symbol(L"\x5728"); // \345\234\250
static Symbol sym_from1 = Symbol(L"\x81ea"); // \350\207\252 /from/self/oneself/since/
static Symbol sym_from2 = Symbol(L"\x4ece"); // \344\273\216  /lax/yielding/from/

// verbs with org subject
static Symbol sym_carry_out = Symbol(L"\x91c7\x53d6"); // \351\207\207\345\217\226 
static Symbol sym_implement = Symbol(L"\x5b9e\x65bd"); // \345\256\236\346\226\275 (1 exception)
static Symbol sym_ratify = Symbol(L"\x6279\x51c6"); // \346\211\271\345\207\206 (2 exceptions)
static Symbol sym_agree_and_sign = Symbol(L"\x7b7e\x8ba2"); // \347\255\276\350\256\242 (occurs only once)
static Symbol sym_power_verb = Symbol(L"\x529b\x4e89"); // \345\212\233\344\272\211
static Symbol sym_employ = Symbol(L"\x4f7f");  // \344\275\277 /to make/to cause/to enable/to employ/
static Symbol sym_flourish_verb = Symbol(L"\x5174\x529e"); // \345\205\264\345\212\236 
static Symbol sym_plan = Symbol(L"\x8ba\x5212"); // \350\256\241\345\210\222 /to plan/to map out/
static Symbol sym_oppose = Symbol(L"\x53cd\x5bf9"); // \345\217\215\345\257\271 /to fight against/to oppose/
static Symbol sym_hold = Symbol(L"\x628a"); // \346\212\212 /to hold/to contain/to grasp/ - BA
static Symbol sym_uphold = Symbol(L"\x575a\x6301"); // \345\235\232\346\214\201 /to continue upholding/to remain committed to/persistence/to persist/to uphold/to insist on/holdout/
static Symbol sym_propose = Symbol(L"\x63d0\x51fa"); // \346\217\220\345\207\272 /to raise (an issue)/to propose/to put forward/to post (Usenet)/Presentation/
static Symbol sym_support = Symbol(L"\x650f\x6301"); // \346\224\257\346\214\201 /to be in favor of/to support/to back/support/backing/to stand by/Back/adhesion/advocacy/bolster/bolster up/buttress/buttress up/carries/endorsement/holdout/plonk/plunk/supported/underpin/
static Symbol sym_ask = Symbol(L"\x8981\x6c42"); // \350\246\201\346\261\202 /request/require/stake a claim (to something)/to ask/to demand/Apply/postulation/pretence/requested/requisition/
static Symbol sym_encourage = Symbol(L"\x9f13\x52b1"); // \351\274\223\345\212\261 /to encourage/encouragement/pep/reanimate/reanimation/

static Symbol sym_with = Symbol(L"\x540c"); // \345\220\214 (leave out?)
static Symbol sym_government = Symbol(L"\x653f\x5e9c"); // \346\224\277\345\272\234
static Symbol sym_company = Symbol(L"\x516c\x53f8"); // \345\205\254\345\217\270

using namespace std;

ChineseMetonymyAdder::ChineseMetonymyAdder() : _debug(L"metonymy_debug")  {

	if (ParamReader::isParamTrue("use_metonymy"))
		_use_metonymy = true;
	else
		_use_metonymy = false;	

	if (ParamReader::isParamTrue("use_gpe_roles"))
		_use_gpe_roles = true;
	else
		_use_gpe_roles = false;
	
	_is_sports_story = false;

}

void ChineseMetonymyAdder::addMetonymyTheory(const MentionSet *mentionSet,
						              const PropositionSet *propSet)
{
	if (!_use_metonymy)
		return;

	for (int i = 0; i < propSet->getNPropositions(); i++) {
		Proposition *prop = propSet->getProposition(i);

		if (prop->getPredType() == Proposition::NOUN_PRED || 
			prop->getPredType() == Proposition::NAME_PRED) 
			processNounOrNamePredicate(prop, mentionSet);
		else if (prop->getPredType() == Proposition::VERB_PRED) 
			processVerbPredicate(prop, mentionSet);
		else if (prop->getPredType() == Proposition::SET_PRED) 
			processSetPredicate(prop, mentionSet);
		else if (prop->getPredType() == Proposition::LOC_PRED)
			processLocPredicate(prop, mentionSet);
		else if (prop->getPredType() == Proposition::MODIFIER_PRED)
			processModifierPredicate(prop, mentionSet);
		else if (prop->getPredType() == Proposition::POSS_PRED)
			processPossessivePredicate(prop, mentionSet);
	}
}

void ChineseMetonymyAdder::processNounOrNamePredicate(Proposition *prop, const MentionSet *mentionSet) {
	const Mention *ref = prop->getMentionOfRole(Argument::REF_ROLE, mentionSet);

	if (ref == NULL) {
		SessionLogger::warn("metonymy") << "ChineseMetonymyAdder::processNounOrNamePredicate(): "
							   << "Found noun or name predicate without a mention reference - ignoring.\n\n";
		return;
	}


	for (int j = 0; j < prop->getNArgs(); j++) {
		Argument *arg = prop->getArg(j);
		if (arg->getType() == Argument::MENTION_ARG) {
			Mention *ment = mentionSet->getMention(arg->getMentionIndex());
			
			if (ment->getEntityType().matchesGPE())
				processGPENounArgument(arg, ref, mentionSet);
		}
	}
}

void ChineseMetonymyAdder::processVerbPredicate(Proposition *prop, const MentionSet *mentionSet) {
	const SynNode *verbNode = prop->getPredHead();
	const Mention *subject = prop->getMentionOfRole(Argument::SUB_ROLE, mentionSet);

	for (int j = 0; j < prop->getNArgs(); j++) {
		Argument *arg = prop->getArg(j);
	
		if (arg->getType() == Argument::MENTION_ARG) {
			Mention *ment = mentionSet->getMention(arg->getMentionIndex());

			if (ment->getEntityType().matchesGPE()) 
				processGPEVerbArgument(arg, verbNode, subject, mentionSet);
		}
	}
}

void ChineseMetonymyAdder::processSetPredicate(Proposition *prop, const MentionSet *mentionSet) {
	const Mention *ref = prop->getMentionOfRole(Argument::REF_ROLE, mentionSet);

	if (ref == NULL) {
		SessionLogger::warn("metonymy") << "ChineseMetonymyAdder::processSetPredicate(): "
							   << "Found set predicate without a mention reference - ignoring.\n\n";
		return;
	}

	if (ref->getEntityType().isDetermined() &&
		(ref->hasIntendedType() || ref->hasRoleType()))
	{
		EntityType intendedType = ref->hasIntendedType() ? ref->getIntendedType() : ref->getRoleType();

		// Unnecessary because addMetonymyToMention already looks at children of mention?
		for (int j = 0; j < prop->getNArgs(); j++) {
			Argument *arg = prop->getArg(j);
			if (arg->getRoleSym() == Argument::MEMBER_ROLE && arg->getType() == Argument::MENTION_ARG)
			{
				Mention *ment = mentionSet->getMention(arg->getMentionIndex());
				if (ment->getEntityType().isRecognized()) {
					_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
					_debug << "\n member of set with intended type " << intendedType.getName().to_string() << "\n";
					addMetonymyToMention(ment, intendedType);
				}
			}
		}
	}
}

void ChineseMetonymyAdder::processLocPredicate(Proposition *prop, const MentionSet *mentionSet) {
	for (int j = 0; j < prop->getNArgs(); j++) {
		Argument *arg = prop->getArg(j);
		if (arg->getRoleSym() == Argument::REF_ROLE || arg->getRoleSym() == Argument::LOC_ROLE) {
			Mention *ment = mentionSet->getMention(arg->getMentionIndex());
			if (ment->getEntityType().matchesGPE()) {
				_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
				_debug << "\n GPE part of LOC predicate\n";
				addMetonymyToMention(ment, EntityType::getLOCType());
			}
		}
	}
}

void ChineseMetonymyAdder::processModifierPredicate(Proposition *prop, const MentionSet *mentionSet) {
	const Mention *ref = prop->getMentionOfRole(Argument::REF_ROLE, mentionSet);

	if (ref == NULL) {
		SessionLogger::warn("metonymy") << "ChineseMetonymyAdder::processModifierPredicate(): "
							   << "Found modifier predicate without a mention reference - ignoring.\n\n";
		return;
	}

	for (int j = 0; j < prop->getNArgs(); j++) {
		Argument *arg = prop->getArg(j);
		if (arg->getType() == Argument::MENTION_ARG) {
			Mention *ment = mentionSet->getMention(arg->getMentionIndex());
			
			if (ment->getEntityType().matchesGPE()) 
				processGPENounArgument(arg, ref, mentionSet);
		}
	}
}

void ChineseMetonymyAdder::processPossessivePredicate(Proposition *prop, const MentionSet *mentionSet) {
	const Mention *ref = prop->getMentionOfRole(Argument::REF_ROLE, mentionSet);

	if (ref == NULL) {
		SessionLogger::warn("metonymy") << "ChineseMetonymyAdder::processPossessivePredicate(): "
							   << "Found possessive predicate without a mention reference - ignoring.\n\n";
		return;
	}

	for (int j = 0; j < prop->getNArgs(); j++) {
		Argument *arg = prop->getArg(j);
		if (arg->getType() == Argument::MENTION_ARG) {
			Mention *ment = mentionSet->getMention(arg->getMentionIndex());
			
			if (arg->getRoleSym() == Argument::POSS_ROLE && 
				ment->getEntityType().matchesGPE() &&
				ref->getEntityType().matchesLOC()) 
			{
				_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
				_debug << "\n GPE possessor of LOC -> GPE.LOC\n";
				addMetonymyToMention(ment, EntityType::getLOCType());
			}
		}
	}
}

void ChineseMetonymyAdder::processGPENounArgument(Argument *arg, const Mention *ref, const MentionSet *mentionSet) {
	Mention *ment = mentionSet->getMention(arg->getMentionIndex());	

	if (ment == 0 || !ment->getEntityType().matchesGPE())
		return;
	
	if (arg->getRoleSym() == sym_located_at ||
		arg->getRoleSym() == sym_from1 ||
		arg->getRoleSym() == sym_from2) {
		_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
		_debug << "\n object of locative preposition -> GPE.LOC\n";
		addMetonymyToMention(ment, EntityType::getLOCType());
	}
	else if (arg->getRoleSym() == Argument::UNKNOWN_ROLE) {
		if (ref->getNode()->getHeadWord() == sym_government && ref->getEntityType().isRecognized()) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE government -> GPE.government_type\n";
			addMetonymyToMention(ment, ref->getEntityType());
		}
		/*else if (ref->getEntityType().matchesLOC() ||
		         //ref->getEntityType().matchesORG() || - JSM removed for ACE2004 Eval
				 ref->getEntityType().matchesFAC())
		{
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n unknown GPE modifier of LOC/ORG/FAC -> GPE.LOC\n";
			addMetonymyToMention(ment, EntityType::getLOCType());
		}*/
		else if (ref->getEntityType().matchesPER()) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n unknown GPE modifier of PER -> GPE.GPE\n";
			addMetonymyToMention(ment, EntityType::getGPEType());
		}
		else if (ref->getEntityType().matchesGPE()) {
			if (ref->hasRoleType()) {
				_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
				_debug << "\n unknown GPE modifier of GPE." << ref->getRoleType().getName().to_debug_string() << "\n";
				addMetonymyToMention(ment, ref->getRoleType());
			}
			else if (ref->hasIntendedType()) {
				_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
				_debug << "\n unknown GPE modifier of GPE." << ref->getIntendedType().getName().to_debug_string() << "\n";
				addMetonymyToMention(ment, ref->getIntendedType());
			}
		}
	}
	else if (arg->getRoleSym() == Argument::POSS_ROLE) {
		if (ref->getEntityType().matchesLOC()) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE possessor of LOC -> GPE.LOC\n";
			addMetonymyToMention(ment, EntityType::getLOCType());
		}
	}
	else if (arg->getRoleSym() == Argument::REF_ROLE) {
		if (isStoryHeader(ment, mentionSet)) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE in story header -> GPE.LOC\n";
			addMetonymyToMention(ment, EntityType::getLOCType());
		}
		else if (ment->getNode()->getHeadWord() == sym_government) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n 'government' -> GPE.ORG\n";
			addMetonymyToMention(ment, EntityType::getORGType());
		}
	}
}

void ChineseMetonymyAdder::processGPEVerbArgument(Argument *arg, const SynNode *verbNode, 
										   const Mention *subj, const MentionSet *mentionSet) {
	Mention *ment = mentionSet->getMention(arg->getMentionIndex());
	const Symbol verb = verbNode->getHeadWord();

	if (ment == 0 || !ment->getEntityType().matchesGPE())
		return;

	if (arg->getRoleSym() == Argument::SUB_ROLE) {
		if (verb == sym_carry_out ||
			verb == sym_implement ||
			verb == sym_ratify ||
			verb == sym_agree_and_sign ||
			verb == sym_power_verb ||
			verb == sym_employ ||
			verb == sym_flourish_verb ||
			verb == sym_plan ||
			verb == sym_oppose ||
			verb == sym_hold ||
			verb == sym_uphold ||
			verb == sym_propose ||
			verb == sym_support ||
			verb == sym_ask ||
			verb == sym_encourage) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n subject of ORG verb -> GPE.ORG\n";
			addMetonymyToMention(ment, EntityType::getORGType());
		}
		else if (verb == sym_hold_meeting ||
				 verb == sym_convene) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n subject of LOC verb -> GPE.LOC\n";
			addMetonymyToMention(ment, EntityType::getLOCType());
		}
	}
	else if (arg->getRoleSym() == Argument::OBJ_ROLE) {
		if (//verb == sym_is_situated ||
			verb == sym_come ||
			verb == sym_come_from ||
			verb == sym_go_to ||
			verb == sym_to_a_place ||
			verb == sym_invest ||
			verb == sym_arrive ||
			verb == sym_stationed_in ||
			verb == sym_all_over ||
			verb == sym_debouchment ||
			verb == sym_located_at2 ||
			verb == sym_depart ||
			verb == sym_reach ||
			verb == sym_visited)
		{
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n object of locative verb -> GPE.LOC\n";
			addMetonymyToMention(ment, EntityType::getLOCType());
		}
	}
	else if (arg->getRoleSym() == Argument::LOC_ROLE) {
		_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
		_debug << "\n locative argument to verb -> GPE.LOC\n";
		addMetonymyToMention(ment, EntityType::getLOCType());
	}
	else if (arg->getRoleSym() == sym_located_at ||
			 arg->getRoleSym() == sym_from1 ||
			 arg->getRoleSym() == sym_from2) {
		_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
		_debug << "\n object of locative preposition -> GPE.LOC\n";
		addMetonymyToMention(ment, EntityType::getLOCType());
	}
	else if (arg->getRoleSym() == sym_with) {
		if (subj != 0) {
			if (subj->hasIntendedType()) {
				_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
				_debug << "\n object of preposition like/with\n";
				addMetonymyToMention(ment, subj->getIntendedType());
			}
			else if (subj->hasRoleType()) {
				_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
				_debug << "\n object of preposition like/with\n";
				addMetonymyToMention(ment, subj->getRoleType());
			}
		}
	}
}

void ChineseMetonymyAdder::processORGNounArgument(Argument *arg, const Mention *ref, const MentionSet *mentionSet) {
	Mention *ment = mentionSet->getMention(arg->getMentionIndex());	

	if (ment == 0 || !ment->getEntityType().matchesORG())
		return;
	
	if (arg->getRoleSym() == sym_located_at) {
		_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
		_debug << "\n object of locative preposition -> ORG.FAC\n";
		addMetonymyToMention(ment, EntityType::getFACType());
	}
}

void ChineseMetonymyAdder::processORGVerbArgument(Argument *arg, const SynNode *verbNode, const MentionSet *mentionSet) {
	Mention *ment = mentionSet->getMention(arg->getMentionIndex());
	const Symbol verb = verbNode->getHeadWord();

	if (ment == 0 || !ment->getEntityType().matchesORG())
		return;

	if (arg->getRoleSym() == sym_located_at) {
		_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
		_debug << "\n object of locative preposition -> ORG.FAC\n";
		addMetonymyToMention(ment, EntityType::getFACType());
	}
}

bool ChineseMetonymyAdder::isStoryHeader(Mention *mention, const MentionSet *mentionSet) {
	const SynNode *node = mention->getNode();
	const SynNode *parent = node->getParent();

	if (parent != NULL && parent->getTag() == ChineseSTags::FRAG) {
		if (parent->getNChildren() < 2)
			return false;
		const SynNode *first = parent->getChild(0);
		const SynNode *second = parent->getChild(1);
		if (!first->hasMention() || 
			!mentionSet->getMention(first->getMentionIndex())->getEntityType().matchesORG())
			return false;
		if (!second->hasMention() ||
			!mentionSet->getMention(second->getMentionIndex())->getEntityType().matchesGPE())
			return false;
		return true;
	}	
	return false;
}

void ChineseMetonymyAdder::addMetonymyToMention(Mention *mention, EntityType type) {
	if (mention->hasIntendedType() || mention->hasRoleType())	{
		/*SessionLogger::logger->beginWarning();
		*SessionLogger::logger << "ChineseMetonymyAdder::addMetonymyToMention(): "
							<< "metonymy already assigned to mention " << mention->getUID() << ".\n\n";
		*/
		return;
	}
	
	// first apply to children
	Mention *child = mention->getChild();
	while (child != 0) {
		addMetonymyToMention(child, type);
		child = child->getNext();
	}

	if (_use_gpe_roles && mention->getEntityType().matchesGPE() &&
		!(_is_sports_story && type.matchesORG()))
		mention->setRoleType(type);
	else  {
		if (mention->getEntityType() == type)
			return;
		else
			mention->setIntendedType(type);
	}
}
