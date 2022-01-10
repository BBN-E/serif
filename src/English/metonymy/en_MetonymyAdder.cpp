// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/Sentence.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/UnexpectedInputException.h"
#include "English/metonymy/en_MetonymyAdder.h"
#include "English/parse/en_STags.h"

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>


static Symbol sym_university(L"university");
static Symbol sym_olympics(L"olympics");
static Symbol sym_in(L"in");
static Symbol sym_within(L"within");
static Symbol sym_to(L"to");
static Symbol sym_into(L"into");
static Symbol sym_at(L"at");
static Symbol sym_near(L"near");
static Symbol sym_with(L"with");
static Symbol sym_towards(L"towards");
static Symbol sym_against(L"against");
static Symbol sym_across(L"across");
static Symbol sym_over(L"over");
static Symbol sym_around(L"around");
static Symbol sym_throughout(L"throughout");
static Symbol sym_from(L"from");
static Symbol sym_through(L"through");
static Symbol sym_on(L"on");
static Symbol sym_of(L"of");
static Symbol sym_government(L"government");
static Symbol sym_governments(L"governments");
static Symbol sym_news(L"news");
static Symbol sym_for(L"for");
static Symbol sym_capital(L"capital");
static Symbol sym_settlement(L"settlement");
static Symbol sym_province(L"province");
static Symbol sym_poss(L"'s");
static Symbol sym_the(L"the");

static Symbol sym_us1(L"us");
static Symbol sym_us2(L"u.s");
static Symbol sym_us3(L"u.s");
static Symbol sym_uk1(L"uk");
static Symbol sym_uk2(L"u.k.");
static Symbol sym_uk3(L"u.k");

static Symbol sym_korea(L"korea");
static Symbol sym_timor(L"timor");
static Symbol sym_east(L"east");

using namespace std;

EnglishMetonymyAdder::EnglishMetonymyAdder() : _total_words(0), _sports_words(0), _is_sports_story(false),
								_saw_university(false), _saw_olympics(false), _use_metonymy(false)
{
	_debug.init(Symbol(L"metonymy_debug"));
	_num_sports_phrases = 0;
	_sportsTeamSubtype = EntitySubtype::getDefaultSubtype(EntityType::getORGType());

	_use_metonymy = ParamReader::isParamTrue("use_metonymy");
	_use_gpe_roles = ParamReader::isParamTrue("use_gpe_roles");
	
	_do_metonymy_for_tac = ParamReader::isParamTrue("do_metonymy_for_tac"); // TAC

	if (_use_metonymy) {
		std::string metonymyDir = ParamReader::getRequiredParam("metonymy_dir");
		std::string file;

		_orgVerbs = _new SymbolHash(100);
		file = metonymyDir + "/org_verbs";
		loadSymbolHash(_orgVerbs, file.c_str());

		_gpeVerbs = _new SymbolHash(100);
		file = metonymyDir + "/gpe_verbs";
		loadSymbolHash(_gpeVerbs, file.c_str());

		_peopleVerbs = _new SymbolHash(100);
		file = metonymyDir + "/person_verbs";
		loadSymbolHash(_peopleVerbs, file.c_str());

		_meaninglessVerbs = _new SymbolHash(100);
		file = metonymyDir + "/meaningless_verbs";
		loadSymbolHash(_meaninglessVerbs, file.c_str());

		_adjectivalGPEs = _new SymbolHash(100);
		file = metonymyDir + "/adjectival_gsps";
		loadSymbolHash(_adjectivalGPEs, file.c_str());

		_sportsWords = _new SymbolHash(100);
		file = metonymyDir + "/sports_words";
		loadSymbolHash(_sportsWords, file.c_str());

		_capitalCities = _new SymbolHash(100);
		file = metonymyDir + "/capital_cities";
		loadSymbolHash(_capitalCities, file.c_str());

		_countries = _new SymbolHash(100);
		file = metonymyDir + "/countries";
		loadSymbolHash(_countries, file.c_str());

		_peopleWords = _new SymbolHash(100);
		file = metonymyDir + "/people_words";
		loadSymbolHash(_peopleWords, file.c_str());

		_governmentOrgs = _new SymbolHash(100);
		file = metonymyDir + "/government_orgs";
		loadSymbolHash(_governmentOrgs, file.c_str());

		_directionWords = _new SymbolHash(100);
		file = metonymyDir + "/direction_words";
		loadSymbolHash(_directionWords, file.c_str());

		_locativeReferenceWords = _new SymbolHash(100);
		file = metonymyDir + "/locative_reference_words";
		loadSymbolHash(_locativeReferenceWords, file.c_str());

		_travelWords = _new SymbolHash(100);
		file = metonymyDir + "/travel_words";
		loadSymbolHash(_travelWords, file.c_str());

		_teamVerbs = _new SymbolHash(100);
		file = metonymyDir + "/team_verblist";
		loadSymbolHash(_teamVerbs, file.c_str());

		_governmentNouns = _new SymbolHash(100);
		file = metonymyDir + "/government_nouns";
		loadSymbolHash(_governmentNouns, file.c_str());

		_states = _new SymbolHash(100);
		file = metonymyDir + "/states";
		loadSymbolHash(_states, file.c_str());

		_functioningAsFacVerbs = _new SymbolHash(100);
		file = metonymyDir + "/functioning_as_facility_verblist";
		loadSymbolHash(_functioningAsFacVerbs, file.c_str());

		_functioningAsOrgVerbs = _new SymbolHash(100);
		file = metonymyDir + "/functioning_as_organization_verblist";
		loadSymbolHash(_functioningAsOrgVerbs, file.c_str());

		file = metonymyDir + "/certain_sports_phrases";
		boost::scoped_ptr<UTF8InputStream> sportsStream_scoped_ptr(UTF8InputStream::build(file.c_str()));
		UTF8InputStream& sportsStream(*sportsStream_scoped_ptr);
		_loadSportsPhrasesList(sportsStream);
		if (!sportsStream.eof() && sportsStream.fail()) {
			std::string error = "Error reading file: " + file;
			error += " pointed to by parameter 'metonymy_dir'";
			throw UnexpectedInputException("MetonomyAdder::MetonomyAdder()", error.c_str());
		}

	}
}

EnglishMetonymyAdder::~EnglishMetonymyAdder() {
	for (int i = 0; i < _num_sports_phrases; i++) {
		delete _sportsPhrases[i];
	}
	if (_use_metonymy) {
		delete _orgVerbs;
		delete _gpeVerbs;
		delete _peopleVerbs;
		delete _meaninglessVerbs;
		delete _adjectivalGPEs;
		delete _sportsWords;
		delete _capitalCities;
		delete _countries;
		delete _peopleWords;
		delete _governmentOrgs;
		delete _directionWords;
		delete _locativeReferenceWords;
		delete _travelWords;
		delete _teamVerbs;
		delete _governmentNouns;
		delete _states;
		delete _functioningAsFacVerbs;
		delete _functioningAsOrgVerbs;
	}
}

void EnglishMetonymyAdder::resetForNewDocument(DocTheory *docTheory) {
	if (_debug.isActive()) {
		_debug << "DOCUMENT: " << docTheory->getDocument()->getName().to_debug_string() << "\n";
	}

	_total_words = 0;
	_sports_words = 0;
	_is_sports_story = false;
	_saw_university = false;
	_saw_olympics = false;

	if (_use_metonymy && docTheory != 0) {
		int i, j;
		for (i = 0; i < docTheory->getNSentences(); i++) {
			if (_is_sports_story) break;
			const Sentence *sent = docTheory->getSentence(i);
			const LocatedString *string = sent->getString();

			LocatedString stringCopy(*string);
			stringCopy.toLowerCase();

			stringCopy.insert(L" ", 0);
			stringCopy.replace(L"\n\n", L" ");
			stringCopy.replace(L"\n", L" ");
			stringCopy.replace(L".", L" ");
			stringCopy.replace(L"  ", L" ");

			for (j = 0; j < _num_sports_phrases; j++) {
				wstring *phrase = _sportsPhrases[j];

				if (stringCopy.indexOf(phrase->data()) > -1) {
					_is_sports_story = true;
					if (_debug.isActive()) {
						_debug << L"SPORTS STORY (" << phrase->data() << ")\n";
					}
					break;
				}
			}
		}

	}
}


/*
 * Bonan: cherrypick code for resolving metony for TAC; dealing with GPE-ORG/sportsTeam and ORG-FAC metonymy
 */
void EnglishMetonymyAdder::resolveMetonymyForTAC(const MentionSet *mentionSet,
											 const PropositionSet *propSet) {
 	for (int i = 0; i < propSet->getNPropositions(); i++) {
		Proposition *prop = propSet->getProposition(i);

		if (prop->getPredType() == Proposition::NOUN_PRED ||
			prop->getPredType() == Proposition::NAME_PRED) {

			const Mention *ref = prop->getMentionOfRole(Argument::REF_ROLE, mentionSet);

			for (int j = 0; j < prop->getNArgs(); j++) {
				Argument *arg = prop->getArg(j);
				if (arg->getType() == Argument::MENTION_ARG) {
					Mention *ment = mentionSet->getMention(arg->getMentionIndex());

					// GPE
					if (ment->getEntityType().matchesGPE()) {
						if (ment == 0 || !ment->getEntityType().matchesGPE())
							continue;

						if (arg->getRoleSym() == Argument::POSS_ROLE) {
							if (_is_sports_story && ment->mentionType == Mention::NAME) {
								// Boston's struggling bullpen
								if (!ref->getEntityType().matchesORG()) {
									_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
									_debug << "\n possessive GPE in sports story -> sports metonymy\n";
									changeGPEintoSportsTeam(ment); // Bonan: this should be kept
								}
							}
						}
						else if (arg->getRoleSym() == Argument::REF_ROLE) {
							if (isSportsTeam(ment)) // Bonan: should be kept									
								continue;
						}
					}

					// ORG
					else if (ment->getEntityType().matchesORG()) {
						if (ment == 0 || !ment->getEntityType().matchesORG())
							continue;

						if (arg->getRoleSym() == Argument::REF_ROLE && isOrgFacSpecialCase(ment)) {
							_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
							_debug << "\n ORG as FAC special case\n";
							// addMetonymyToMention(ment, EntityType::getFACType());
							changeTypeORGintoFAC(ment);
						}
					}

					// FAC
					else if (ment->getEntityType().matchesFAC()) {

						if (ment == 0 || !ment->getEntityType().matchesFAC())
							continue;

						if (arg->getRoleSym() == Argument::REF_ROLE && isFacOrgSpecialCase(ment)) {
							_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
							_debug << "\n FAC as ORG special case\n";
							// addMetonymyToMention(ment, EntityType::getORGType());
							changeTypeFACintoORG(ment);
						}
					}
				}
			}
		}
		else if (prop->getPredType() == Proposition::VERB_PRED || prop->getPredType() == Proposition::COPULA_PRED) {
			const SynNode *verbNode = prop->getPredHead();

			for (int j = 0; j < prop->getNArgs(); j++) {
				Argument *arg = prop->getArg(j);

				if (arg->getType() == Argument::MENTION_ARG) {
					Mention *ment = mentionSet->getMention(arg->getMentionIndex());

					// GPE
					if (ment->getEntityType().matchesGPE()) {
						const Symbol verb = verbNode->getHeadWord();

						if (ment == 0 || !ment->getEntityType().matchesGPE())
							continue;

						if (arg->getRoleSym() == Argument::SUB_ROLE) {
							// *Boston* celebrated in their dugout after beating New York
							if (_is_sports_story && ment->getMentionType() == Mention::NAME) {
								_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
								_debug << "\n GPE SUBJ in sports story -> sports metonymy\n";
								changeGPEintoSportsTeam(ment); // Bonan: This should be kept
							}
						}
					}	

					// FAC
					else if (ment->getEntityType().matchesFAC()) {
						// EMB: This was wrong as of 9/15, should be looking at ORGs here to see if they are
						//   acting like FACs
						// JSM:  I think this is backwards.  According to the calling function it should be
						//   looking for FACs.  1/05/07
						const Symbol verb = verbNode->getHeadWord();

						if (ment == 0 || !ment->getEntityType().matchesFAC())
							continue;

						if (arg->getRoleSym() == Argument::OBJ_ROLE || arg->getRoleSym() == Argument::SUB_ROLE) {
							if (_functioningAsOrgVerbs->lookup(verb)) {
								_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
								_debug << "\n functioning_as_org_verb FAC -> FAC.ORG\n";
								addMetonymyToMention(ment, EntityType::getORGType());
							}
						}
						else if (arg->getRoleSym() == sym_with || arg->getRoleSym() == sym_against) {
							_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
							_debug << "\n verb with/against FAC -> FAC.ORG\n";
							// addMetonymyToMention(ment, EntityType::getORGType());
							changeTypeFACintoORG(ment);
						}
					}

					// ORG
					else if (ment->getEntityType().matchesORG()) {
						// EMB: This was wrong as of 9/15, should be looking at FACs here to see if they are
						//   acting like ORGs
						// JSM:  I think this is backwards.  According to the calling function it should be
						//   looking for ORGs.  1/05/07
						Mention *ment = mentionSet->getMention(arg->getMentionIndex());
						const Symbol verb = verbNode->getHeadWord();

						if (ment == 0 || !ment->getEntityType().matchesORG())
							continue;

						if (arg->getRoleSym() == Argument::OBJ_ROLE) {
							if (_functioningAsFacVerbs->lookup(verb)) {
								_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
								_debug << "\n functioning_as_fac_verb ORG -> ORG.FAC\n";
								// addMetonymyToMention(ment, EntityType::getFACType());
								changeTypeORGintoFAC(ment);
							}
						}
					}
				}
			}
		}
	}
}

void EnglishMetonymyAdder::changeTypeORGintoFAC(Mention *mention) {

	mention->setEntityType(EntityType::getFACType());
	mention->setEntitySubtype(EntitySubtype::getDefaultSubtype(EntityType::getFACType()));
	mention->setMetonymyMention();
	mention->setRoleType(EntityType::getUndetType());

	addMetonymyToMention(mention, EntityType::getFACType());
}

void EnglishMetonymyAdder::changeTypeFACintoORG(Mention *mention) {

	mention->setEntityType(EntityType::getORGType());
	mention->setEntitySubtype(EntitySubtype::getDefaultSubtype(EntityType::getORGType()));
	mention->setMetonymyMention();
	mention->setRoleType(EntityType::getUndetType());

	addMetonymyToMention(mention, EntityType::getORGType());
}

void EnglishMetonymyAdder::addMetonymyTheory(const MentionSet *mentionSet,
						              const PropositionSet *propSet)
{

	if (!_use_metonymy)
		return;

	// TAC
	if(_do_metonymy_for_tac)
		resolveMetonymyForTAC(mentionSet, propSet);
	// original (for ACE?)
	else { 
		for (int i = 0; i < propSet->getNPropositions(); i++) {
			Proposition *prop = propSet->getProposition(i);

			if (prop->getPredType() == Proposition::NOUN_PRED ||
				prop->getPredType() == Proposition::NAME_PRED)
				processNounOrNamePredicate(prop, mentionSet);
			else if (prop->getPredType() == Proposition::VERB_PRED || prop->getPredType() == Proposition::COPULA_PRED)
				processVerbPredicate(prop, mentionSet);
			else if (prop->getPredType() == Proposition::SET_PRED)
				processSetPredicate(prop, mentionSet);
			else if (prop->getPredType() == Proposition::LOC_PRED)
				processLocPredicate(prop, mentionSet);
			else if (prop->getPredType() == Proposition::MODIFIER_PRED)
				processModifierPredicate(prop, mentionSet);
		}
	}
}

void EnglishMetonymyAdder::processNounOrNamePredicate(Proposition *prop, const MentionSet *mentionSet) {
	const Mention *ref = prop->getMentionOfRole(Argument::REF_ROLE, mentionSet);

	for (int j = 0; j < prop->getNArgs(); j++) {
		Argument *arg = prop->getArg(j);
		if (arg->getType() == Argument::MENTION_ARG) {
			Mention *ment = mentionSet->getMention(arg->getMentionIndex());

			if (ment->getEntityType().matchesGPE())
				processGPENounArgument(arg, ref, mentionSet);

			else if (ment->getEntityType().matchesORG())
				processOrgNounArgument(arg, ref, mentionSet);

			else if (ment->getEntityType().matchesFAC())
				processFacNounArgument(arg, ref, mentionSet);
		}
	}
}

void EnglishMetonymyAdder::processVerbPredicate(Proposition *prop, const MentionSet *mentionSet) {
	const SynNode *verbNode = prop->getPredHead();

	for (int j = 0; j < prop->getNArgs(); j++) {
		Argument *arg = prop->getArg(j);

		if (arg->getType() == Argument::MENTION_ARG) {
			Mention *ment = mentionSet->getMention(arg->getMentionIndex());

			if (ment->getEntityType().matchesGPE())
				processGPEVerbArgument(arg, verbNode, mentionSet);
			else if (ment->getEntityType().matchesFAC())
				processFacVerbArgument(arg, verbNode, mentionSet);
			else if (ment->getEntityType().matchesORG())
				processOrgVerbArgument(arg, verbNode, mentionSet);
		}
	}
}


void EnglishMetonymyAdder::processSetPredicate(Proposition *prop, const MentionSet *mentionSet) {
	const Mention *ref = prop->getMentionOfRole(Argument::REF_ROLE, mentionSet);

	if (ref->getEntityType().isRecognized() && (ref->hasIntendedType() || ref->hasRoleType())) {
		EntityType intendedType = ref->hasIntendedType() ? ref->getIntendedType() : ref->getRoleType();

		// Unnecessary because addMetonymyToMention already looks at children of mention?
		// EMB: I think so...
		for (int j = 0; j < prop->getNArgs(); j++) {
			Argument *arg = prop->getArg(j);
			if (arg->getRoleSym() == Argument::MEMBER_ROLE && arg->getType() == Argument::MENTION_ARG)
			{
				Mention *ment = mentionSet->getMention(arg->getMentionIndex());
				if (ment->getEntityType().isRecognized()) {
					if (ref->hasIntendedType()) {
						_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
						_debug << "\n member of set with intended type " << intendedType.getName().to_string() << "\n";
						addMetonymyToMention(ment, intendedType);
					}
					else { // ref->hasRoleType()
						_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
						_debug << "\n member of set with role type " << intendedType.getName().to_string() << "\n";
						addRoleToMention(ment, intendedType);
					}
				}
			}
		}
	}else {
		// TB: not sure what the previous part is meant to do...
		// This part takes care of GPE mentions in sentences such as "NBC news, Washington"
		// and adds a LOC role to it.
		// It also deals with list of capital cities such as "the tension between Washington and Tehran".
		// or 'North korea and Washington'
		// For some reason the dateline sentence is also reaching here (e.g. 'WASHINGTON , Dec. 20')
		bool all_gpes = true;
		bool all_capital_cities_or_countries = true;
		for (int j = 0; j < prop->getNArgs(); j++) {
			Argument *arg = prop->getArg(j);
			if (arg->getRoleSym() == Argument::MEMBER_ROLE && arg->getType() == Argument::MENTION_ARG){
				Mention *ment = mentionSet->getMention(arg->getMentionIndex());
				if (ment->getEntityType() == EntityType::getGPEType()){
					if(!isCapitalCity(ment) 
						/*&& !isCountry(ment)*/)
						all_capital_cities_or_countries = false;
				} else {
					all_gpes = false;
				}
			}
		}
		if (!all_gpes) {
			for (int j = 0; j < prop->getNArgs(); j++) {
				Argument *arg = prop->getArg(j);
				if (arg->getRoleSym() == Argument::MEMBER_ROLE && arg->getType() == Argument::MENTION_ARG)
				{
					Mention *ment = mentionSet->getMention(arg->getMentionIndex());
					if (ment->getEntityType().isRecognized() && ment->getEntityType() == EntityType::getGPEType()) {
							_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
							_debug << "\n GPE member of a set with other members not GPEs role is set to  " << EntityType::getLOCType().getName().to_string() << "\n";
							addRoleToMention(ment, EntityType::getLOCType());
					}
				}
			}
		}
		else if (all_capital_cities_or_countries) {
			for (int j = 0; j < prop->getNArgs(); j++) {
				Argument *arg = prop->getArg(j);
				if (arg->getRoleSym() == Argument::MEMBER_ROLE && arg->getType() == Argument::MENTION_ARG)
				{
					Mention *ment = mentionSet->getMention(arg->getMentionIndex());
					if (ment->getEntityType().isRecognized() && ment->getEntityType() == EntityType::getGPEType()
						 && isCapitalCity(ment)) {
							_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
							_debug << "\n capital GPE member of a set with other members GPE capitals role is set to  " << EntityType::getORGType().getName().to_string() << "\n";
							addRoleToMention(ment, EntityType::getORGType());
							ment->setMetonymyMention();
					}
				}
			}
		}
	}
}

void EnglishMetonymyAdder::processLocPredicate(Proposition *prop, const MentionSet *mentionSet) {
	for (int j = 0; j < prop->getNArgs(); j++) {
		Argument *arg = prop->getArg(j);
		// only the <loc> entity (e.g. Egypt in "Cairo, Egypt" gets the automatic LOC
		// if it's a dateline, that will get handled in isDateline
		if (arg->getRoleSym() == Argument::LOC_ROLE && arg->getType() == Argument::MENTION_ARG) {
			Mention *ment = mentionSet->getMention(arg->getMentionIndex());
			if (ment->getEntityType().matchesGPE()) {
				_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
				_debug << "\n city, *state* -> GPE.LOC\n";
				addRoleToMention(ment, EntityType::getLOCType());
			}
		}
	}
}

void EnglishMetonymyAdder::processModifierPredicate(Proposition *prop, const MentionSet *mentionSet) {
	if (isLocationPreposition(prop->getPredSymbol())) {
		for (int i = 0; i < prop->getNArgs(); i++) {
			Argument *arg = prop->getArg(i);
			// "the Kremlin in *Moscow*"
			if (isLocationPreposition(arg->getRoleSym()) && arg->getType() == Argument::MENTION_ARG) {
				Mention *ment = mentionSet->getMention(arg->getMentionIndex());
				if (ment->getEntityType().matchesGPE()) {
					_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
					_debug << "\n ... loc_prep GPE -> GPE.LOC\n";
					addRoleToMention(ment, EntityType::getLOCType());
				}
			}
			// "central *Iraq*"
			else if (arg->getRoleSym() == Argument::REF_ROLE && arg->getType() == Argument::MENTION_ARG
				&& _directionWords->lookup(prop->getPredSymbol()) )
			{
				Mention *ment = mentionSet->getMention(arg->getMentionIndex());
				Symbol mentSym  = ment->getHead()->getHeadWord();
				if (ment->getEntityType().matchesGPE() && mentSym != sym_korea && mentSym != sym_timor && mentSym != sym_east) {
					_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
					_debug << "\n ... direction GPE -> GPE.LOC\n";
					addRoleToMention(ment, EntityType::getLOCType());
				}
			}
		}
	}
}

void EnglishMetonymyAdder::processGPENounArgument(Argument *arg, const Mention *ref, const MentionSet *mentionSet) {
	Mention *ment = mentionSet->getMention(arg->getMentionIndex());

	if (ment == 0 || !ment->getEntityType().matchesGPE())
		return;

	// "ambassador to London"
	if (arg->getRoleSym() == sym_to && ref->getEntityType().matchesPER()) {
		_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
		_debug << "\n PER to GPE -> GPE.GPE\n";
		addRoleToMention(ment, EntityType::getGPEType());
	}
	// "hostels in France", "earthquake within Iran"
	else if (isLocationPreposition(arg->getRoleSym())) {
		_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
		_debug << "\n ... loc_prep GPE -> GPE.LOC\n";
		addRoleToMention(ment, EntityType::getLOCType());
		// TODO: nested location preposition with "of" - see is_object_of_location_preposition from old ACE
	}
	// GPE as unknown modifier
	else if (arg->getRoleSym() == Argument::UNKNOWN_ROLE && ref != 0) {

		Symbol refWord = ref->getNode()->getHeadWord();
		// "Chechnya-based muslim fighters"
		if (somethingIsBasedThere(ment))
			return; // GPE.LOC

		// EMB: for Ace2004, all premods should be role GPE
		_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
		_debug << "\n ... premod_prep GPE -> GPE.GPE\n";
		addRoleToMention(ment, EntityType::getGPEType());

/*
		// "Guangdong province"
		else if (refWord == sym_capital || refWord == sym_settlement || refWord == sym_province) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE political_word -> GPE.GPE\n";
			addMetonymyToMention(ment, EntityType::getGPEType());
		}
		// "United States Supreme Court"
		else if (_governmentOrgs->lookup(refWord) != NULL) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE government_org -> GPE.GPE\n";
			addMetonymyToMention(ment, EntityType::getGPEType());
		}
		// "New York policeman"
		else if (ref->getEntityType().matchesPER()) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE PER -> GPE.GPE\n";
			//addMetonymyToMention(ment, EntityType::getPERType()); - new guidelines disagree
			addMetonymyToMention(ment, EntityType::getGPEType());
		}
		// "California company", "Venezuelan river", "Pennsylvania freeway"
		else if (ref->getEntityType().matchesORG() ||
				 ref->getEntityType().matchesLOC() ||
				 ref->getEntityType().matchesFAC()) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE ORG/LOC/FAC -> GPE.LOC\n";
			addMetonymyToMention(ment, EntityType::getLOCType());
		}
		else if (ref->getEntityType().matchesGPE() &&
				(ref->getRoleType().matchesLOC() ||
				ref->getIntendedType().matchesLOC()))
		{
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE GPE.LOC -> GPE.LOC\n";
			addMetonymyToMention(ment, EntityType::getLOCType());
		}
		// "Russian diplomacy"
		else if (!ref->getEntityType().isRecognized() && _governmentNouns->lookup(refWord) != NULL) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE government_noun -> GPE.ORG\n";
			addMetonymyToMention(ment, EntityType::getORGType());
		}
		// "Korean american"
		else if (!ref->getEntityType().isRecognized() && _adjectivalGPEs->lookup(refWord) != NULL) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE adjectival_GPE -> GPE.GPE\n";
			addMetonymyToMention(ment, EntityType::getGPEType());
		}*/
	}
	// ... of GPE
	else if (arg->getRoleSym() == sym_of && ref != NULL) {
		Symbol refWord = ref->getNode()->getHeadWord();
		// person of GPE
		if (ref->getEntityType().matchesPER()) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n PER of GPE -> GPE.GPE\n";
			addRoleToMention(ment, EntityType::getGPEType());
		}
		// center/east/etc of GPE
		else if (_directionWords->lookup(refWord)) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n dir_word of GPE -> GPE.LOC\n";
			addRoleToMention(ment, EntityType::getLOCType());
		}
		else {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n default of GPE -> GPE.GPE\n";
			addRoleToMention(ment, EntityType::getGPEType());
		}
	}
	// ... on/against/with GPE
	else if (arg->getRoleSym() == sym_on
				|| arg->getRoleSym() == sym_against
				|| arg->getRoleSym() == sym_with 
/*				|| arg->getRoleSym() == sym_towards*/)
	{
/*		if (isCapitalCity(ment)) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n ... on/against/with/towards capital - GPE -> GPE.ORG\n";
			addRoleToMention(ment, EntityType::getORGType());
			ment->setMetonymyMention();
		}else {
*/			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n ... on/against/with GPE -> GPE.GPE\n";
			addRoleToMention(ment, EntityType::getGPEType());
//		}
	}
	else if (arg->getRoleSym() == Argument::POSS_ROLE) {
		// EMB: not convinced on this one
		if (ref->getEntityType().matchesLOC()) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE's LOC -> GPE.LOC\n";
			addRoleToMention(ment, EntityType::getLOCType());
		}
		else if (_is_sports_story && ment->mentionType == Mention::NAME) {
			// "Boston's baseball team"
			if (ref->getEntityType().matchesORG()) {
				_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
				_debug << "\n GPE possessor of ORG in sports story  -> GPE.GPE\n";
				addRoleToMention(ment, EntityType::getGPEType());
			}
			else {
				// Boston's struggling bullpen
				_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
				_debug << "\n possessive GPE in sports story -> sports metonymy\n";
				changeGPEintoSportsTeam(ment);
			}
		}
		// TB: If it is a capital and possesive I set the default role to ORG
		else if (isCapitalCity(ment)) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n is a capital. Adding GPE.ORG role.\n";
			addRoleToMention(ment, EntityType::getORGType());
			ment->setMetonymyMention();
		}
		// TB: I set the default role here to ORG
		//else {
		//	_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
		//	_debug << "\n By default adding GPE.ORG role.\n";
		//	addRoleToMention(ment, EntityType::getORGType());
		//}
	}
	else if (arg->getRoleSym() == Argument::REF_ROLE) {
		if (getRoleFromParent(ment) ||
			hasSpecialHeadWord(ment) ||
			isDateline(ment, mentionSet) ||
			somethingIsBasedThere(ment) ||
			isSignoff(ment, mentionSet) ||
			isLocativeReference(ment) |
			isGPEPerson(ment) ||
			isSportsTeam(ment) ||
			isCapitalCity(ment))
			return;
	}
}

void EnglishMetonymyAdder::processOrgNounArgument(Argument *arg, const Mention *ref, const MentionSet *mentionSet) {
	Mention *ment = mentionSet->getMention(arg->getMentionIndex());

	if (ment == 0 || !ment->getEntityType().matchesORG())
		return;

	if (arg->getRoleSym() == Argument::REF_ROLE && isOrgFacSpecialCase(ment)) {
		_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
		_debug << "\n ORG as FAC special case\n";
		addMetonymyToMention(ment, EntityType::getFACType());
	}
}

void EnglishMetonymyAdder::processFacNounArgument(Argument *arg, const Mention *ref, const MentionSet *mentionSet) {
	Mention *ment = mentionSet->getMention(arg->getMentionIndex());

	if (ment == 0 || !ment->getEntityType().matchesFAC())
		return;

	if (arg->getRoleSym() == Argument::REF_ROLE && isFacOrgSpecialCase(ment)) {
		_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
		_debug << "\n FAC as ORG special case\n";
		addMetonymyToMention(ment, EntityType::getORGType());
	}
}

void EnglishMetonymyAdder::processGPEVerbArgument(Argument *arg, const SynNode *verbNode, const MentionSet *mentionSet) {
	Mention *ment = mentionSet->getMention(arg->getMentionIndex());
	const Symbol verb = verbNode->getHeadWord();

	if (ment == 0 || !ment->getEntityType().matchesGPE())
		return;

	// GPE mention after locative preposition that modifies a verb
	if (isLocationPreposition(arg->getRoleSym())) {
		// TODO: check to see if ref is governmental
		_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
		_debug << "\n VERB in/from/to GPE -> GPE.LOC\n";
		addRoleToMention(ment, EntityType::getLOCType());
	}
	else if (arg->getRoleSym() == Argument::OBJ_ROLE) {
		// "we bombed *Iraq*"
		if (_gpeVerbs->lookup(verb)) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE OBJ of gpe_verb\n";
			addRoleToMention(ment, EntityType::getGPEType());
		}
		// this is stuff like "elect" -- not quite sure how we could be an <obj>, but OK
		else if (_peopleVerbs->lookup(verb)) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE OBJ of people_verb\n";
			addRoleToMention(ment, EntityType::getPERType());
		}
		// "Bob voyaged to *Spain*"
		else if (_travelWords->lookup(verb)) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE OBJ of travel_word\n";
			addRoleToMention(ment, EntityType::getLOCType());
		}
	}
	else if (arg->getRoleSym() == Argument::SUB_ROLE) {
		// *Boston* celebrated in their dugout after beating New York
		if (_is_sports_story && ment->getMentionType() == Mention::NAME) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE SUBJ in sports story -> sports metonymy\n";
			changeGPEintoSportsTeam(ment);
		}
		// "*Iraq* bombed something"
		else if (_gpeVerbs->lookup(verb)) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE SUBJ of gpe_verb\n";
			addRoleToMention(ment, EntityType::getGPEType());
		}
		// "*America* elected Reagan"
		else if (_peopleVerbs->lookup(verb)) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE SUBJ of people_verb\n";
			addRoleToMention(ment, EntityType::getPERType());
		}
		// ummmm... subject? doesn't really make sense
		else if (_travelWords->lookup(verb)) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE SUBJ of travel_word\n";
			addRoleToMention(ment, EntityType::getLOCType());
		}
		// "*America* insisted/declared/negotiated, etc..."
		else if (_orgVerbs->lookup(verb)) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE SUBJ of org_verb\n";
			addRoleToMention(ment, EntityType::getORGType());
		}
		// TB: "*Paris* signed the treaty" - deal with capital city metonymy
		else if (isCapitalCity(ment)) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n GPE SUBJ of gpe_verb and a capital city\n";
			ment->setMetonymyMention();
			addRoleToMention(ment, EntityType::getORGType());
		}
	}
}

void EnglishMetonymyAdder::processFacVerbArgument(Argument *arg, const SynNode *verbNode, const MentionSet *mentionSet)
{
	// EMB: This was wrong as of 9/15, should be looking at ORGs here to see if they are
	//   acting like FACs
	// JSM:  I think this is backwards.  According to the calling function it should be
	//   looking for FACs.  1/05/07
	Mention *ment = mentionSet->getMention(arg->getMentionIndex());
	const Symbol verb = verbNode->getHeadWord();

	if (ment == 0 || !ment->getEntityType().matchesFAC())
		return;

	if (arg->getRoleSym() == Argument::OBJ_ROLE || arg->getRoleSym() == Argument::SUB_ROLE) {
		if (_functioningAsOrgVerbs->lookup(verb)) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n functioning_as_org_verb FAC -> FAC.ORG\n";
			addMetonymyToMention(ment, EntityType::getORGType());
		}
	}
	else if (arg->getRoleSym() == sym_with || arg->getRoleSym() == sym_against) {
		_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
		_debug << "\n verb with/against FAC -> FAC.ORG\n";
		addMetonymyToMention(ment, EntityType::getORGType());
	}
}

void EnglishMetonymyAdder::processOrgVerbArgument(Argument *arg, const SynNode *verbNode, const MentionSet *mentionSet)
{
	// EMB: This was wrong as of 9/15, should be looking at FACs here to see if they are
	//   acting like ORGs
	// JSM:  I think this is backwards.  According to the calling function it should be
	//   looking for ORGs.  1/05/07
	Mention *ment = mentionSet->getMention(arg->getMentionIndex());
	const Symbol verb = verbNode->getHeadWord();

	if (ment == 0 || !ment->getEntityType().matchesORG())
		return;

	if (arg->getRoleSym() == Argument::OBJ_ROLE) {
		if (_functioningAsFacVerbs->lookup(verb)) {
			_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
			_debug << "\n functioning_as_fac_verb ORG -> ORG.FAC\n";
			addMetonymyToMention(ment, EntityType::getFACType());
		}
	}
}

void EnglishMetonymyAdder::countSportsWords(const SynNode *node) {
	if (node->isPreterminal()) {
		_total_words++;
		if (_sportsWords->lookup(node->getHeadWord()))
			_sports_words++;
		if (node->getHeadWord() == sym_olympics) {
			_saw_olympics = true;
			_sports_words++;
		}
		if (node->getHeadWord() == sym_university)
			_saw_university = true;
	}
	// TODO: look for phrases that begin with "team" for olympic teams
	else {
		for (int i = 0; i < node->getNChildren(); i++)
			countSportsWords(node->getChild(i));
	}
}

bool EnglishMetonymyAdder::isGPEPerson(Mention *ment) {

	// EMB: This whole function was totally wrong as of 9/15/05
	// It should be looking for GPEs that refer to the general population

	const SynNode *node = ment->getNode();
	Symbol headword = node->getHeadWord();

	if (!ment->getEntityType().matchesGPE())
		return false;

	// "The Iraqis blame ..."
	if (isNationalityWord(node->getHeadWord()) && theIsOnlyPremod(node) &&
		headword != sym_us1 && headword != sym_us2 && headword != sym_us3 &&
		headword != sym_uk1 && headword != sym_uk2 && headword != sym_uk3)
	{
		_debug << "Mention: " << ment->getUID() << "\n" << ment->getNode()->toPrettyParse(2);
		_debug << "\n GPE person -> GPE.PER\n";
		addRoleToMention(ment, EntityType::getPERType());
		return true;
	}
	// TODO: "America watched...."
	return false;
}

bool EnglishMetonymyAdder::isNationalityWord(Symbol word) {
	if (_adjectivalGPEs->lookup(word))
		return true;
	else {
		wstring str(word.to_string());
		wstring::size_type len = str.length();
		if (len > 1 && (str[len-1] == L's') && _adjectivalGPEs->lookup(Symbol(str.substr(0,len-1).data())))
			return true;
		if (len > 2 && (str.substr(len-2,2) == wstring(L"es") && _adjectivalGPEs->lookup(Symbol(str.substr(0,len-2).data()))))
			return true;
	}
	return false;
}

bool EnglishMetonymyAdder::isOrgFacSpecialCase(Mention *mention) {
	if (!mention->getEntityType().matchesORG())
		return false;

	const SynNode *node = mention->getNode();
	const SynNode *parent = node->getParent();

	if (node->toTextString().find(L"white house") == 0 ||
		node->toTextString().find(L"the white house") == 0) {

		if (parent != NULL &&
			parent->toTextString().find(L"the white house") == 0)
		{
			parent = parent->getParent();
		}

		if (parent != NULL && parent->getTag() == EnglishSTags::PP &&
			(parent->getHeadWord() == sym_to || parent->getHeadWord() == sym_into ||
			 parent->getHeadWord() == sym_at || parent->getHeadWord() == sym_near))
			 return true;
		// TODO: check for ", the white house." and return true
		else if (parent != NULL && parent->getTag() == EnglishSTags::VP &&
				 _travelWords->lookup(parent->getHeadWord()))
			 return true;
		else
			return false;
	}

	return false;
}

bool EnglishMetonymyAdder::isFacOrgSpecialCase(Mention *mention) {
	if (!mention->getEntityType().matchesFAC())
		return false;

	const SynNode *node = mention->getNode();

	if (node->toTextString().find(L"white house") == 0  ||
		node->toTextString().find(L"the white house") == 0 ||
		node->toTextString().find(L"the pentagon") == 0 ||
		node->toTextString().find(L"the kremlin") == 0)
	{
		return true;
	}
	return false;
}

bool EnglishMetonymyAdder::theIsOnlyPremod(const SynNode *node) {
	if (node->getHeadIndex() > 0) {
		for (int i = 0; i < node->getHeadIndex(); i++) {
			const SynNode *child = node->getChild(i);
			if (child->getHeadWord() != sym_the)
				return false;
		}
		return true;
	}
	return false;
}

Symbol EnglishMetonymyAdder::getSmartVerbHead(const SynNode *node) {
	const SynNode* head = node;
	if (head->isPreterminal())
		return head->getHeadWord();
	while (head->getHead()->getTag() == EnglishSTags::TO ||
		   head->getHead()->getTag() == EnglishSTags::VBD ||
		   head->getHead()->getTag() == EnglishSTags::VBN ||
		   head->getHead()->getTag() == EnglishSTags::MD ||
		   head->getHead()->getTag() == EnglishSTags::VBZ ||
		   head->getHead()->getTag() == EnglishSTags::VB ||
		   head->getHead()->getTag() == EnglishSTags::VBG ||
		   head->getHead()->getTag() == EnglishSTags::VBP)
	{
		int i;
		for (i = head->getHeadIndex()+1; i < head->getNChildren(); i++) {
			const SynNode *child = head->getChild(i);
			if (child->getTag() == EnglishSTags::VP) {
				head = child;
				break;
			}
		}
		if ((head->getHeadIndex() == head->getNChildren()-1) || i == head->getNChildren())
			break;
	}
	return head->getHeadWord();
}

bool EnglishMetonymyAdder::getRoleFromParent(Mention *mention) {
	if (!mention->getEntityType().matchesGPE())
		return false;

	Mention *parent = mention->getParent();
	if (parent != NULL &&
		(parent->getNode()->getHead() == mention->getNode() ||
	     parent->getNode()->getTag() == EnglishSTags::NPPOS))
	{
		if (parent->hasRoleType()) {
			_debug << "Mention: " << mention->getUID() << "\n" << mention->getNode()->toPrettyParse(2);
			_debug << "\n got ROLE from parent\n";
			mention->setRoleType(parent->getRoleType());
		}
		else if (parent->hasIntendedType()) {
			_debug << "Mention: " << mention->getUID() << "\n" << mention->getNode()->toPrettyParse(2);
			_debug << "\n got intended type from parent\n";
			mention->setIntendedType(parent->getIntendedType());
		}
		return true;
	}
	return false;
}

bool EnglishMetonymyAdder::hasSpecialHeadWord(Mention *mention) {
	if (mention->getNode()->getHeadWord() == sym_government ||
		mention->getNode()->getHeadWord() == sym_governments)
	{
		_debug << "Mention: " << mention->getUID() << "\n" << mention->getNode()->toPrettyParse(2);
		_debug << "\n has gov't head word -> GPE.ORG\n";
		addRoleToMention(mention, EntityType::getORGType());
		return true;
	}
	return false;
}

bool EnglishMetonymyAdder::isDateline(Mention *mention, const MentionSet *mentionSet) {
	const SynNode *node = mention->getNode();
	const SynNode *parent = node->getParent();

	if (parent == NULL) {
		_debug << "Mention: " << mention->getUID() << "\n" << mention->getNode()->toPrettyParse(2);
		_debug << "\n is_dateline -> GPE.LOC\n";
		addRoleToMention(mention, EntityType::getLOCType());
		return true;
	}
	else if (parent->hasMention() &&
			 mentionSet->getMention(parent->getMentionIndex())->mentionType == Mention::LIST &&
			 parent->getParent() == NULL &&
			 mentionSet->getSentenceNumber() == 0)
	{
		Mention *list = mentionSet->getMention(parent->getMentionIndex());
		Mention *child = list->getChild();
		if (!child->getEntityType().matchesGPE())
			return false;
		_debug << "Mention: " << mention->getUID() << "\n" << node->toPrettyParse(2);
		_debug << "\n is_dateline -> GPE.LOC\n";
		addRoleToMention(mention, EntityType::getLOCType());
		return true;
	}
	return false;
}

bool EnglishMetonymyAdder::somethingIsBasedThere(Mention *mention) {
	const SynNode *node = mention->getNode();
	const SynNode *parent = node->getParent();

	if (parent == NULL) return false;

	for (int i = 0; i < parent->getHeadIndex() - 1; i++) {
		const SynNode *child = parent->getChild(i);
		if (child == node) {
			const SynNode *next = parent->getChild(i+1);
			if (next->isPreterminal() &&
				next->toTextString().find(L"based") != std::wstring::npos)
			{
				_debug << "Mention: " << mention->getUID() << "\n" << mention->getNode()->toPrettyParse(2);
				_debug << "\n something is based there -> GPE.LOC " << next->toTextString() << "\n";
				addRoleToMention(mention, EntityType::getLOCType());
				return true;
			}
		}
	}
	return false;
}

bool EnglishMetonymyAdder::isSignoff(Mention *mention, const MentionSet *mentionSet) {
	const SynNode *node = mention->getNode();
	const SynNode *parent = node->getParent();

	if (parent == NULL)
		return false;

	if (parent->hasMention() &&
		mentionSet->getMention(parent->getMentionIndex())->mentionType == Mention::LIST)
	{
		Mention *list = mentionSet->getMention(parent->getMentionIndex());
		Mention *child = list->getChild();
		if (child == NULL || !child->getEntityType().matchesPER())
			return false;
		child = child->getNext();
		if (child == NULL || (!child->getEntityType().matchesORG() && child->getNode()->getHeadWord() != sym_news))
			return false;
		child = child->getNext();
		while (child != NULL && child->getEntityType().matchesGPE()) {
			if (child == mention) {
				_debug << "Mention: " << mention->getUID() << "\n" << mention->getNode()->toPrettyParse(2);
				_debug << "\n in sign off list -> GPE.LOC\n";
				addRoleToMention(mention, EntityType::getLOCType());
				return true;
			}
			child = child->getNext();
		}
	}
	else {
		// must have at least 2 postmods
		if (parent->getHeadIndex() > parent->getNChildren() - 2)
			return false;
		if (parent->getChild(parent->getHeadIndex()+1)->getTag() != EnglishSTags::COMMA)
			return false;

		// make sure node is in postmods
		int i;
		for (i = parent->getHeadIndex()+2; i < parent->getNChildren(); i++) {
			if (parent->getChild(i) == node)
				break;
		}
		if (i >= parent->getNChildren())
			return false;

		const SynNode *person = parent;
		while (person != NULL && person->hasMention() &&
			mentionSet->getMention(person->getMentionIndex()) != 0 &&
			!mentionSet->getMention(person->getMentionIndex())->getEntityType().matchesPER() &&
			person->getTag() != EnglishSTags::PP &&
			person->getHeadWord() != sym_for &&
			mentionSet->getMention(person->getMentionIndex())->isOfRecognizedType())
		{
			person = person->getParent();
		}
		if (parent->getHeadWord() == sym_news) {
			_debug << "Mention: " << mention->getUID() << "\n" << mention->getNode()->toPrettyParse(2);
			_debug << "\n is sign off 1 -> GPE.LOC\n";
			addRoleToMention(mention, EntityType::getLOCType());
			return true;
		}
		if (person == NULL || !person->hasMention() || 
			mentionSet->getMention(parent->getMentionIndex()) == NULL ||
			!mentionSet->getMention(person->getMentionIndex())->getEntityType().matchesPER())
			return false;
		if (person->getParent() != NULL && person->getParent()->getTag() != EnglishSTags::S)
			return false;
		if (parent->hasMention() &&
			mentionSet->getMention(parent->getMentionIndex()) != NULL &&
			(mentionSet->getMention(parent->getMentionIndex())->getEntityType().matchesPER() ||
			 mentionSet->getMention(parent->getMentionIndex())->getEntityType().matchesLOC() ||
			 mentionSet->getMention(parent->getMentionIndex())->getEntityType().matchesORG()))
		{
			_debug << "Mention: " << mention->getUID() << "\n" << mention->getNode()->toPrettyParse(2);
			_debug << "\n is sign off 2 -> GPE.LOC\n";
			addRoleToMention(mention, EntityType::getLOCType());
			return true;
		}
	}

	return false;
}

bool EnglishMetonymyAdder::isLocativeReference(Mention *mention) {
	if (_locativeReferenceWords->lookup(mention->getNode()->getHeadWord())) {
		_debug << "Mention: " << mention->getUID() << "\n" << mention->getNode()->toPrettyParse(2);
		_debug << "\n is locative reference (e.g. here) -> GPE.LOC\n";
		addRoleToMention(mention, EntityType::getLOCType());
		return true;
	}
	return false;
}

bool EnglishMetonymyAdder::isLocationPreposition(Symbol preposition) {

	if (preposition == sym_in ||
		preposition == sym_within ||
		preposition == sym_to ||
		preposition == sym_into ||
		preposition == sym_across ||
		preposition == sym_at ||
		preposition == sym_over ||
		preposition == sym_near ||
		preposition == sym_around ||
		preposition == sym_throughout ||
		preposition == sym_from ||
		preposition == sym_through)
		return true;
	return false;
}

bool EnglishMetonymyAdder::isSportsTeam(Mention *mention) {
	// EMB: I don't know how this works in Ace2004 -- fix me!
	if (!_is_sports_story)
		return false;

	if (mention->getMentionType() != Mention::NAME)
		return false;

	// get rid of GPEs with an already assigned (i.e. better) role
	if (mention->hasRoleType() && !mention->getRoleType().matchesGPE())
		return false;

	const SynNode *node = mention->getNode();
	const SynNode *parent = mention->getNode();

	// "Picabo Street of the United States" -> no metonymy
	if (parent != NULL && parent->getTag() == EnglishSTags::PP &&
	   (parent->getHeadWord() == sym_of || parent->getHeadWord() == sym_from))
	   return false;

	Mention *name = mention;
	while (name->getChild() != NULL)
		name = name->getChild();
	Symbol nameSymbol = Symbol(name->getNode()->toTextString().data());

	if (node->getTag() == EnglishSTags::NPPOS || !isNationalityWord(nameSymbol)) {
		if (_saw_university) {
			if (_states->lookup(nameSymbol)) {
				_debug << "Mention: " << mention->getUID() << "\n" << mention->getNode()->toPrettyParse(2);
				_debug << "\n is university sports team -> sports metonymy\n";
				changeGPEintoSportsTeam(mention);
				return true;
			}
			else
				return false;
		}
		else if (_saw_olympics) {
			if (! _countries->lookup(nameSymbol))
				return false;
			// TODO: check for name on list of olympic teams from document?
			// EMB: can't figure out what this does, so I'm turning it off
			/*if (node->getParent() != NULL && node->getParent()->getTag() == EnglishSTags::VP) {
				Symbol verb = getSmartVerbHead(node->getParent());
				if (_teamVerbs->lookup(verb) != NULL) {
					_debug << "Mention: " << mention->getUID() << "\n" << mention->getNode()->toPrettyParse(2);
					_debug << "\n is sports team, saw olypics -> GPE.ORG\n";
					addMetonymyToMention(mention, EntityType::getORGType());
					return true;
				}
			}*/
		}
		else {
			_debug << "Mention: " << mention->getUID() << "\n" << mention->getNode()->toPrettyParse(2);
			_debug << "\n is sports team -> sports metonymy\n";
			changeGPEintoSportsTeam(mention);
			return true;
		}
	}
	return false;
}

bool EnglishMetonymyAdder::isCapitalCity(Mention *mention) {
	// EMB: This doesn't actually get taken into consideration in entity linking, 
	// so there's really no point in this right now

	if (_is_sports_story) // or eeld_story
		return false;

	if (mention->getMentionType() != Mention::NAME)
		return false;


	const SynNode *node = mention->getNode();
	Mention *name = mention;
	while (name->getChild() != NULL)
		name = name->getChild();
	// ack, we have a trailing space problem here (sigh)
	std::wstring nameStr = name->getNode()->toTextString();	
	Symbol nameSymbol = Symbol();
	if (nameStr.length() > 1)
		nameSymbol = Symbol(nameStr.substr(0, nameStr.length() - 1).c_str());
	else nameSymbol = Symbol(L":NULL");

	if (_capitalCities->lookup(nameSymbol) &&
		! mention->hasIntendedType() && ! mention->hasRoleType())
	{
		const SynNode *parent = node->getParent();
		if (parent == NULL)
			return false;

		bool possessive = false;
		if (node->getTag() == EnglishSTags::NPPOS || parent->getTag() == EnglishSTags::NPPOS)
			possessive = true;
		else if (parent->getHeadWord() == sym_poss)
			possessive = true;

		if (!possessive && parent->getHeadIndex() > 0) {
			for (int i = 0; i < parent->getHeadIndex(); i++) {
				if (parent->getChild(i)->getHeadWord() == sym_poss) {
					possessive = true;
					break;
				}
			}
		}
		if (!possessive && parent->getHeadIndex() < parent->getNChildren() - 1) {
			for (int j = parent->getHeadIndex()+1; j < parent->getNChildren(); j++) {
				if (parent->getChild(j)->getHeadWord() == sym_poss) {
					possessive = true;
					break;
				}
			}
		}

//		if ((parent->getTag() == EnglishSTags::VP || parent->getHead()->getTag() == EnglishSTags::VP) ||
			 // TODO: or on list of previously marked capitals or already with ORG role
//			 possessive)
		{
			return true;
		}
	}
	return false;
}


bool EnglishMetonymyAdder::isCountry(Mention *mention) {

	if (mention->getMentionType() != Mention::NAME)
		return false;


	const SynNode *node = mention->getNode();
	Mention *name = mention;
	while (name->getChild() != NULL)
		name = name->getChild();
	// ack, we have a trailing space problem here (sigh)
	std::wstring nameStr = name->getNode()->toTextString();	
	Symbol nameSymbol = Symbol();
	if (nameStr.length() > 1)
		nameSymbol = Symbol(nameStr.substr(0, nameStr.length() - 1).c_str());
	else nameSymbol = Symbol(L":NULL");

	if (_countries->lookup(nameSymbol))
	{
		return true;
	}
	return false;
}

void EnglishMetonymyAdder::addMetonymyToMention(Mention *mention, EntityType type) {

	if (mention->isMetonymyMention() || mention->hasIntendedType())	{
		SessionLogger::warn("multiple_metonymy_assignments") << "EnglishMetonymyAdder::addMetonymyToMention(): "
							<< "metonymy already assigned to mention " << mention->getUID() << ".\n\n";
		return;
	}
	
	if (mention->getEntityType() == type || !_use_metonymy)
		return;

	// first apply to children
	Mention *child = mention->getChild();
	while (child != 0) {
		addMetonymyToMention(child, type);
		child = child->getNext();
	}

	//mention->setIntendedType(type);
	mention->setEntityType(type);
	mention->setEntitySubtype(EntitySubtype::getDefaultSubtype(type));
	mention->setMetonymyMention();
}

void EnglishMetonymyAdder::addRoleToMention(Mention *mention, EntityType type) {
	if (mention->hasRoleType())	{
		return;
	}

	if (!mention->getEntityType().matchesGPE() || !_use_gpe_roles)
		return;

	// first apply to children
	Mention *child = mention->getChild();
	while (child != 0) {
		addRoleToMention(child, type);
		child = child->getNext();
	}

	mention->setRoleType(type);
}

void EnglishMetonymyAdder::_loadSportsPhrasesList(UTF8InputStream &stream)
{
	_num_sports_phrases = 0;

	wchar_t line[501];
	while (!stream.eof()) {
		stream.getLine(line, 500);
		if (line[0] == L'#')
			continue;

		if (line[0] == L'\0' || line[1] == L'\0' || line[2] == L'\0') continue;

		if (_num_sports_phrases >= MAX_SPORTS_PHRASES)
			throw UnexpectedInputException("EnglishMetonymyAdder::_loadSportsPhrasesList()",
							"too many sports phrases");

		_sportsPhrases[_num_sports_phrases++] = _new std::wstring(line);
	}
}

void EnglishMetonymyAdder::loadSymbolHash(SymbolHash *hash, const char* file) {
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(file));
	UTF8InputStream& stream(*stream_scoped_ptr);

	if (stream.fail()) {
		string err = "Problem opening ";
		err.append(file);
		throw UnexpectedInputException("EnglishMetonymyAdder::loadSymbolHash()", err.c_str());
	}

	wchar_t line[501];
	while (!stream.eof()) {
		stream.getLine(line, 500);
		if (line[0] == L'#')
			continue;
#if defined(_WIN32)
		_wcslwr(line);
#else
		boost::to_lower(line);
#endif
		Symbol lineSym(line);
		hash->add(lineSym);
	}
	if (!stream.eof() && stream.fail()) {
		string err = "Error reading file: ";
		err.append(file);
		err.append(" pointed to by parameter 'metonymy_dir'");

		throw UnexpectedInputException("EnglishMetonymyAdder::loadSymbolHash()", err.c_str());
	}
	stream.close();
}


void EnglishMetonymyAdder::changeGPEintoSportsTeam(Mention *mention) {

	mention->setEntityType(EntityType::getORGType());
	mention->setEntitySubtype(_sportsTeamSubtype);
	mention->setMetonymyMention();
	mention->setRoleType(EntityType::getUndetType());

	addMetonymyToMention(mention, EntityType::getORGType());
}

bool EnglishMetonymyAdder::_isObjectOfLocativeProposition(Mention *mention, const PropositionSet *propSet, const MentionSet *mentionSet)
{
	for (int i = 0; i < propSet->getNPropositions(); i++) {
		Proposition *prop = propSet->getProposition(i);
		for (int j = 0; j < prop->getNArgs(); j++) {
			Argument *arg = prop->getArg(j);
			if (arg->getType() == Argument::MENTION_ARG &&
				arg->getMention(mentionSet) == mention &&
				WordConstants::isLocativePreposition(arg->getRoleSym()))
				return true;
		}
	}
	return false;
}

