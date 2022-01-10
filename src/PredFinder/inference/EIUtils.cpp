#include "Generic/common/leak_detection.h"

#pragma warning(disable: 4996)
//#include <time.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <vector>
#pragma warning(push, 0)
#include "boost/regex.hpp"
#pragma warning(pop)
#include "boost/tuple/tuple.hpp"
#include "boost/tuple/tuple_comparison.hpp"
#include "boost/algorithm/string/replace.hpp"
#include "boost/date_time/gregorian/gregorian.hpp" 
#include "PredFinder/common/ElfMultiDoc.h"
#include "PredFinder/inference/EIDocData.h"
#include "PredFinder/inference/EITbdAdapter.h"
#include "PredFinder/inference/EIUtils.h"
#include "PredFinder/inference/PlaceInfo.h"
#include "Generic/common/ParamReader.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/common/NationalityRecognizer.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/ASCIIUtil.h"
//#include "Generic/common/OutputUtil.cpp"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/Value.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/ValueSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/common/NameEquivalenceTable.h"
#include "Generic/common/TimexUtils.h"
#include "PredFinder/elf/ElfDocument.h"
#include "PredFinder/elf/ElfRelation.h"
#include "PredFinder/elf/ElfRelationArg.h"
#include "PredFinder/elf/ElfIndividual.h"
#include "PredFinder/elf/ElfIndividualFactory.h"
#include "PredFinder/elf/ElfRelationArgFactory.h"
#include "LearnIt/MainUtilities.h"
#include "boost/algorithm/string/trim.hpp"
#include "Generic/wordnet/xx_WordNet.h"
#include "Generic/edt/Guesser.h"
#include <boost/scoped_ptr.hpp>

using boost::dynamic_pointer_cast;
static const std::wstring NONLOCAL_WORDS[] = {
	L"foreign", L"international", L"global", L"other", L"opposition", L"rebel",
	L"insurgent", L"guerrilla", L"coalition", L"multi-national", L"multi - national"}; // hyphenization untested

static const size_t NONLOCAL_WORDS_COUNT = sizeof(NONLOCAL_WORDS)/sizeof(std::wstring);
Symbol EIUtils::PAST_REF = Symbol(L"PAST_REF");
Symbol EIUtils::FUTURE_REF = Symbol(L"FUTURE_REF");	
Symbol EIUtils::PRESENT_REF= Symbol(L"PRESENT_REF");

// static data members
std::vector<std::wstring> EIUtils::_nonLocalWords(NONLOCAL_WORDS_COUNT);
std::vector<EIUtils::MilPair> EIUtils::_militarySpecialCases;
std::vector<EIUtils::NatTuple> EIUtils::_nationalityPrefixedXDocFailures;
NameEquivalenceTable * EIUtils::_countryNameEquivalenceTable = NULL;

std::map<std::wstring, std::vector<std::wstring> > EIUtils::_locationExpansions;
std::set<std::wstring> EIUtils::_locationStringsInTable;
std::map<std::wstring, std::vector<std::wstring> > EIUtils::_continentExpansions;
std::set<std::wstring> EIUtils::_continentStringsInTable;
std::map<std::wstring, std::wstring> EIUtils::_individualTypesByName;
std::set<std::wstring> EIUtils::_democraticWords;
std::set<std::wstring> EIUtils::_governmentWords;
std::set<Symbol> EIUtils::_deputyWords;
std::set<Symbol> EIUtils::_partyEmployees;
std::set<Symbol> EIUtils::_partyNames;

EIUtils::RelationRoleRoleSet EIUtils::_allowedDoubleEntityRelations;

// STATIC METHODS

// Called by the ElfInference constructor.
void EIUtils::init(const std::string& failed_xdoc_path) {
	initAllowedDoubleEntityRelations();
	initRelationSpecificFilterWords();
	initNonLocalWords();
	initLocationTables();
	initCountryNameEquivalenceTable();
	initIndividualTypeNameList();
	load_xdoc_failures(failed_xdoc_path);
	xdocWarnings();
	insertDemocraticWord(L"party");
	insertGovernmentWord(L"army");
	insertGovernmentWord(L"military");
	insertGovernmentWord(L"congress");
	insertGovernmentWord(L"senate");
	insertGovernmentWord(L"embassy");
	insertGovernmentWord(L"embassies");
	insertGovernmentWord(L"ministry");
	insertGovernmentWord(L"consulate");
}

void EIUtils::initNonLocalWords() {
	_nonLocalWords.assign(NONLOCAL_WORDS, NONLOCAL_WORDS + NONLOCAL_WORDS_COUNT);
}

void EIUtils::initRelationSpecificFilterWords() {
	initLeadershipFilterWords();
	initEmploymentFilterWords();
}

void EIUtils::initLeadershipFilterWords() {
	_deputyWords.insert(L"deputy");
	_deputyWords.insert(L"deputy-");
	_deputyWords.insert(L"vice-");
	_deputyWords.insert(L"vice");
	_deputyWords.insert(L"assistant");
	_deputyWords.insert(L"assistant-");
}

void EIUtils::initEmploymentFilterWords() {
	_partyEmployees.insert(L"spokesman");
	_partyEmployees.insert(L"spokeswoman");
	_partyEmployees.insert(L"spokesperson");
	_partyEmployees.insert(L"fighter");
	_partyEmployees.insert(L"fighters");
	_partyEmployees.insert(L"employee");
	_partyEmployees.insert(L"employees");
	_partyEmployees.insert(L"soldier");
	_partyEmployees.insert(L"soldiers");
	_partyEmployees.insert(L"official");
	_partyEmployees.insert(L"officials");

	_partyNames.insert(L"democrat");
	//_partyNames.insert(L"democrat"); // TODO: What was intended here? "democratic"?
	_partyNames.insert(L"republican");
	_partyNames.insert(L"labor");
	_partyNames.insert(L"labour");
	_partyNames.insert(L"likud");
	_partyNames.insert(L"tory");
	_partyNames.insert(L"r");
	_partyNames.insert(L"d");
}

bool EIUtils::containsWord(const SynNode* s, const std::set<Symbol>& strSet)
{
	if (s) {
		int n_terminals = s->getNTerminals();
		Symbol* terminals = new Symbol[n_terminals];
		s->getTerminalSymbols(terminals, n_terminals);

		for (int i=0; i<n_terminals; ++i) {
			if (strSet.find(terminals[i]) != strSet.end()) {
				delete[] terminals;
				return true;
			}
		}
		delete[] terminals;
	}

	return false;
}


bool EIUtils::is_govt_type_symbol(const Symbol& sym) {
    //Note: the SymbolGroup is statically defined, so it's only initialized once,
	//when the function is first called.
    static Symbol::SymbolGroup symbols = Symbol::makeSymbolGroup(
		// Note: the C++ preprocessor will merge these strings, so make sure
		// to leave a space at the end of each line except the last.
		L"court courts army navy forces parliament legislature bureau agency agencies " 
		L"ministry ministries department departments cabinet administration");
	return (sym.isInSymbolGroup(symbols));
}

bool EIUtils::is_person_victim_headword(const Symbol & sym) {
	static Symbol::SymbolGroup symbols = Symbol::makeSymbolGroup(
		// Note: the C++ preprocessor will merge these strings, so make sure
		// to leave a space at the end of each line except the last.
		L"civilian civilians man men woman women child children person people "
		L"dozens hundreds thousands others injured policemen rebels insurgents "
		L"officers militants soldiers guerrillas");
	return (sym.isInSymbolGroup(symbols));
}

bool EIUtils::is_opined_headword(const Symbol & sym) {
	static Symbol::SymbolGroup symbols = Symbol::makeSymbolGroup(
		// Note: the C++ preprocessor will merge these strings, so make sure
		// to leave a space at the end of each line except the last.
		L"wanted arrested condemned denounced vilified admonished blasted assailed "
		L"castigated censured castigated chastised chided criticized criticised decried "
		L"denigrated deplored derided lambasted rebuked scolded slammed court-martialed "
		L"charged tried sentenced filed said told reported ");
	return (sym.isInSymbolGroup(symbols));
}

bool EIUtils::is_location_prep_headword(const Symbol & sym) {
	static Symbol::SymbolGroup symbols = Symbol::makeSymbolGroup(
		L"in at near outside inside on");
	return (sym.isInSymbolGroup(symbols));
}

bool EIUtils::is_continental_headword(const Symbol & sym) {
	static Symbol::SymbolGroup symbols = Symbol::makeSymbolGroup(
		L"african asian european");
	return (sym.isInSymbolGroup(symbols));
}


void EIUtils::initCountryNameEquivalenceTable() {
    string n_e_table = ParamReader::getParam("country_name_equivalencies");
	_countryNameEquivalenceTable = _new NameEquivalenceTable(n_e_table);
}

void EIUtils::destroyCountryNameEquivalenceTable() {
	delete _countryNameEquivalenceTable;
}

std::set<std::wstring> EIUtils::getEquivalentNames(std::wstring originalName, double min_score /*= 0.0*/) {
	return _countryNameEquivalenceTable->getEquivalentNames(originalName, min_score);
}

bool EIUtils::strContainsSubstrFromVector(const std::wstring & str, const std::vector<std::wstring> & substrs) {
	BOOST_FOREACH(std::wstring substr, substrs) {
		if (str.find(substr) != std::wstring::npos) {
			return true;
		}
	}
	return false;
}

/**
 * Reads same file read by ElfMultiDoc::load_mr_xdoc_output_file(), but looks at the lines
 * (i.e., the ones containing the strings "NONE" or "MILITARY") that are not read by that method.
 **/
void EIUtils::load_xdoc_failures(const std::string& failed_xdoc_path) {
	if (failed_xdoc_path.empty()) {
		return;
	}
	boost::scoped_ptr<UTF8InputStream> instream_scoped_ptr(UTF8InputStream::build(failed_xdoc_path.c_str()));
	UTF8InputStream& instream(*instream_scoped_ptr);
	std::wstring line;
	while (getline(instream, line)) {
		std::vector<std::wstring> parts;
		boost::algorithm::split(parts, line, boost::is_any_of("\t"));
		if (parts.size() == 3) {
			std::wstring boundString = parts[0];
			std::wstring xdocID = parts[1];
			std::wstring boundID = parts[2];

			if (L"NONE" == xdocID || L"MILITARY" == xdocID) {
				std::wstring nat = L"";
				if (looksLikeMilitary(boundString, nat)) {
					SessionLogger::info("mb_uri_0") << "Mapping <" << nat << "> to military bound URI <" <<
						boundID << ">" << endl;
					EIUtils::appendToMilitarySpecialCases(nat, boundID);
				}
			}
			if (xdocID == L"NONE") {
				wstring nat;
				wstring rest;
				if (extractNationalityPrefix(boundString, nat, rest)) {
					SessionLogger::info("nat_uri_0") << "Mapping <" << rest << ">, associated with <" <<
						nat << ">, to bound URI <" << boundID << ">" << endl;
					_nationalityPrefixedXDocFailures.push_back(
						boost::make_tuple(nat, rest, boundID));
				}
			} 
		} else {
			if (!parts.empty() && !line.empty() && ASCIIUtil::containsNonWhitespace(line)) {
				SessionLogger::warn("LEARNIT") << "load_xdoc_failures(): ignoring ill-formed line in XDoc failures file " <<
					"(" << failed_xdoc_path << "): " << std::endl << line << std::endl;
			}
		}
	}
	instream.close();
}

bool EIUtils::looksLikeMilitary(const std::wstring& boundString, std::wstring& nat) {
	wstring rest;
	if (extractNationalityPrefix(boundString, nat, rest)) {
		if (L"army" == rest || L"military" == rest || L"armed forces" == rest ||
			L"s army" == rest || L"s military" == rest || L"s armed forces" == rest) 
		{
			return true;
		}
	}
	return false;
}

void EIUtils::xdocWarnings() {
	if (_militarySpecialCases.empty()) {
		SessionLogger::info("LEARNIT") << "Note: Found no special military bound URIs" << endl;
	}

	if (_nationalityPrefixedXDocFailures.empty()) {
		SessionLogger::info("LEARNIT") << "Note: No XDOC bound URI failures" << endl;
	}
}

bool EIUtils::extractNationalityPrefix(const std::wstring& s,
								std::wstring& nat, std::wstring& rest) 
{
	if (_countryNameEquivalenceTable) {
		size_t space = 0;
		bool hasNat = false;
		while (!hasNat && ((size_t)(space + 1) < s.length() ) && 
			(space = s.find(L' ', space + 1 )) != wstring::npos) 
		{
			nat = s.substr(0, space);
			nat = normalizeCountryName(nat);
			
			if (_countryNameEquivalenceTable->isInTable(nat, 0)) {
				hasNat = true;
				rest = s.substr(space + 1);
				boost::to_lower(rest);
			}
		}

		// don't bother if the rest is too short
		return hasNat && (rest.length() > 3);
	} else {
		SessionLogger::warn("LEARNIT") << "Trying to extract nationality, but " <<
			" country name equivalence table is uninitialized" << endl;
	}
	return false;
}

std::wstring EIUtils::normalizeCountryName(const std::wstring& s) {
	using namespace boost;
	std::wstring ret = s;
	to_lower(ret);
	replace_all(ret, ".", " ");
	replace_all(ret, "'", " ");
	trim(ret);
	return ret;
}

// static methods
// I do not like that these are hard-coded, but I'm not sure I have a choice
bool EIUtils::isBombing(const ElfRelation_ptr relation) {
	return (relation->get_arg(EITbdAdapter::getEventRole(L"bombing")) != ElfRelationArg_ptr());
}
bool EIUtils::isAttack(const ElfRelation_ptr relation) {
	return (relation->get_arg(EITbdAdapter::getEventRole(L"attack")) != ElfRelationArg_ptr());
}
bool EIUtils::isKilling(const ElfRelation_ptr relation) {
	return (relation->get_arg(EITbdAdapter::getEventRole(L"killing")) != ElfRelationArg_ptr());
}
bool EIUtils::isInjury(const ElfRelation_ptr relation) {
	return (relation->get_arg(EITbdAdapter::getEventRole(L"injury")) != ElfRelationArg_ptr());
}
bool EIUtils::isGenericViolence(const ElfRelation_ptr relation) {
	return (relation->get_arg(EITbdAdapter::getEventRole(L"generic_violence")) != ElfRelationArg_ptr());
}

// static method
std::wstring EIUtils::getTBDEventType(const ElfRelation_ptr relation) {
	if (isKilling(relation))
		return L"killing";
	else if (isInjury(relation))
		return L"injury";
	else if (isBombing(relation))
		return L"bombing";
	else if (isAttack(relation))
		return L"attack";
	else return L"";
}

void EIUtils::addPersonOrgMentions(EIDocData_ptr docData, const ElfRelation_ptr elf_relation, 
								   const Mention *orgMention, std::set<ElfRelation_ptr> & relationsToAdd) {
	static const wstring PER_ROLE = L"eru:person";
	if (orgMention) {
		// expand offsets to cover both sentences
		EDTOffset start_offset = elf_relation->get_start();
		EDTOffset end_offset = elf_relation->get_end();
		TokenSequence *orgTS = docData->getSentenceTheory(
			orgMention->getSentenceNumber())->getTokenSequence();
		start_offset = std::min(orgTS->getToken(0)->getStartEDTOffset(), start_offset);
		end_offset = std::max(orgTS->getToken(orgTS->getNTokens()-1)->getEndEDTOffset(), end_offset);
		const ElfRelationArg_ptr perArg = elf_relation->get_arg(PER_ROLE);
		const Mention *perMention = findMentionForRelationArg(docData, perArg);
		if (perArg && perMention) {
			relationsToAdd.insert(createBinaryRelation(docData, elf_relation->get_name(), start_offset, end_offset, 
				L"eru:humanOrganization", L"ic:HumanOrganization", orgMention, L"eru:person", L"ic:Person", perMention, elf_relation->get_confidence()));
		}
	}
}


void EIUtils::cleanupDocument(EIDocData_ptr docData) {
	docData->clear();
}

/**
 *  We'd like to go through the individuals in document order, so sort them accordingly. Untested, TODO.
 */

void EIUtils::sortIndividualMapByOffsets(const ElfIndividualSet & individuals, std::list<ElfIndividual_ptr>& sortedList) {
	// Use the commented-out code to determine how long this sort takes (and hence whether it's worthwhile
	// to use a faster sorting routine).
	//clock_t start_time = clock();
	BOOST_FOREACH(ElfIndividual_ptr ind, individuals) {
		EDTOffset start, end;
		ind->get_name_or_desc()->get_offsets(start, end);
		bool inserted = false;
		for (std::list<ElfIndividual_ptr>::iterator iter = sortedList.begin(); iter != sortedList.end(); iter++) {
			EDTOffset iter_start, iter_end;
			(*iter)->get_name_or_desc()->get_offsets(iter_start, iter_end);
			if (start > iter_start) {
				iter++;
				sortedList.insert(iter, ind);
				inserted = true;
				break;
			} else if (start == iter_start && end > iter_end) {
				iter++;
				sortedList.insert(iter, ind);
				inserted = true;
				break;
			}
		}
		if (!inserted) {
			sortedList.push_front(ind);
		}
	}
	//clock_t end_time = clock();
	//clock_t diff = end_time - start_time;
	//double seconds = diff / CLOCKS_PER_SEC;
	//std::cout << "Time elapsed: " << diff << " ticks (" << seconds << " seconds)" << std::endl;
}

/** 
 *  Add a set of relations to the ElfDocument.
 **/
void EIUtils::addNewRelations(EIDocData_ptr docData, const std::set<ElfRelation_ptr> & relations,
							  const std::wstring & src_to_add /*= L""*/) {
	docData->getElfDoc()->add_relations(relations);
	BOOST_FOREACH(ElfRelation_ptr rel, relations) {
		if (!src_to_add.empty()) {
			rel->add_source(src_to_add);
		}
	}
}

/** 
 *  Create a binary ElfRelation & two new individuals.
 **/
ElfRelation_ptr EIUtils::createBinaryRelation(EIDocData_ptr docData,
	   const std::wstring & relation_name, EDTOffset start_offset, EDTOffset end_offset, 
	   const std::wstring & role1, const std::wstring & type1, const Mention *arg1, 
	   const std::wstring & role2, const std::wstring & type2, const Mention *arg2,
	   double confidence)
{
	std::vector<ElfRelationArg_ptr> arguments = createRelationArgFromMention(docData, role1, type1, arg1);
	std::vector<ElfRelationArg_ptr> args2 =  createRelationArgFromMention(docData, role2, type2, arg2);
	arguments.insert(arguments.end(), args2.begin(), args2.end());
	LocatedString* relationTextLS = MainUtilities::substringFromEdtOffsets(
		docData->getDocument()->getOriginalText(), start_offset, end_offset);
	std::wstring relation_text = relationTextLS->toString();
	delete relationTextLS;
	return boost::make_shared<ElfRelation>(relation_name, arguments, relation_text, start_offset, end_offset, confidence, Pattern::UNSPECIFIED_SCORE_GROUP);
}

ElfRelation_ptr EIUtils::createBinaryRelationWithSentences(EIDocData_ptr docData,
		const std::wstring& relation_name, const std::wstring& role1, 
		ElfIndividual_ptr indiv1, const std::wstring& role2, 
		ElfIndividual_ptr indiv2, double confidence)
{
	std::vector<ElfRelationArg_ptr> arguments; 
	ElfRelationArg_ptr arg1 = boost::make_shared<ElfRelationArg>(role1, indiv1);
	ElfRelationArg_ptr arg2 = boost::make_shared<ElfRelationArg>(role2, indiv2);
	arguments.push_back(arg1);
	arguments.push_back(arg2);
	EDTOffset start, end;
	spanOfSentences(docData, getSentenceNumberForIndividualStart(
				docData->getDocTheory(), indiv1),
			getSentenceNumberForIndividualStart(docData->getDocTheory(), indiv2), 
			start, end);
	std::wstring text = textFromIndividualSentences(docData, indiv1, indiv2);
	//SessionLogger::info("foo") << L"Text is " << text;
	return boost::make_shared<ElfRelation>(relation_name, arguments, 
			text, start, end, confidence, Pattern::UNSPECIFIED_SCORE_GROUP);
}

/** 
 *  Create a new ElfRelationArg for a SERIF mention. Technically returns a vector, but it will always have one member.
 **/
std::vector<ElfRelationArg_ptr> EIUtils::createRelationArgFromMention(EIDocData_ptr docData,
	const std::wstring & role, const std::wstring & type, const Mention *arg)
{
	MentionReturnPFeature_ptr rf = boost::make_shared<MentionReturnPFeature>(Symbol(L"ElfInference"), arg);
	rf->setReturnValue(L"role", role);
	rf->setReturnValue(L"type", type);
	rf->setCoverage(docData->getDocTheory());
	std::vector<ElfRelationArg_ptr> arguments = ElfRelationArgFactory::from_return_feature(docData->getDocTheory(), rf);
	return arguments;
}

/** 
 *  Create a new ElfRelationArg for a token span. Technically returns a vector, but it will always have one member.
 **/
std::vector<ElfRelationArg_ptr> EIUtils::createRelationArgFromTokenSpan(EIDocData_ptr docData, 
	const std::wstring & role, const std::wstring & type, int sent_no, int start, int end)
{
	TokenSpanReturnPFeature_ptr rf = boost::make_shared<TokenSpanReturnPFeature>(Symbol(L"ElfInference"), sent_no, start, end);
	rf->setReturnValue(L"role", role);
	rf->setReturnValue(L"type", type);
	rf->setCoverage(docData->getDocTheory());
	std::vector<ElfRelationArg_ptr> arguments = ElfRelationArgFactory::from_return_feature(docData->getDocTheory(), rf);
	return arguments;
}

/** 
 *  Given an ElfRelationArg, try to find the SERIF Mention that was its origin.
 *  We do this by finding a SERIF Mention that has matching offsets and is part of the same entity.
 **/
const Mention * EIUtils::findMentionForRelationArg(EIDocData_ptr docData, ElfRelationArg_ptr rel_arg) {
	// Ignore empty arg individual
	if (rel_arg->get_individual().get() == NULL)
		return NULL;

	// Make sure there's an associated entity and mention
	if (!rel_arg->get_individual()->has_entity_id())
		return NULL;
	if (!rel_arg->get_individual()->has_mention_uid())
		return NULL;

	// Look up by mention ID
	return docData->getMention(rel_arg->get_individual()->get_mention_uid());
}

/** 
 *  Given a SERIF entity, try to find an ElfIndividual that matches it. 
 *  We do this by iterating over all ElfIndividuals until we find a match.
 *  You might want to do this in order to find out whether a SERIF entity has
 *    a particular ElfType.
 **/
ElfIndividual_ptr EIUtils::findElfIndividualForEntity(EIDocData_ptr docData, int entity_id, const std::wstring & ic_type) {	
	BOOST_FOREACH(ElfIndividual_ptr ind, docData->get_individuals_by_type(ic_type)) {
		if (ind->get_entity_id() == entity_id)
			return ind;		
	}
	return ElfIndividual_ptr();
}

/** 
 *  Find the first named mention that matches one of a set of valid name strings.
 *  Please note that the valid name strings must be lowercase.
 **/
const Mention *EIUtils::findMentionByNameStrings(EIDocData_ptr docData, EntityType et, std::set<std::wstring>& validNames) {
	const DocTheory *docTheory = docData->getDocTheory();
	for (int sentno = 0; sentno < docTheory->getNSentences(); sentno++) {
		MentionSet *ms = docTheory->getSentenceTheory(sentno)->getMentionSet();
		for (int m = 0; m < ms->getNMentions(); m++) {
			const Mention *ment = ms->getMention(m);
			if (ment->getEntityType() != et || ment->getMentionType() != Mention::NAME)
				continue;
			std::wstring nameString = ment->getAtomicHead()->toTextString();
			boost::algorithm::trim(nameString); // toTextString() adds an extra space at the end
			if (validNames.find(nameString) != validNames.end())
				return ment;
		}
	}
	return 0;
}

ElfRelationMap EIUtils::findRelationsForEntity(EIDocData_ptr docData, int entity_id){
	//TODO: Check if a vector of individuals is better
	ElfIndividual_ptr individual = findElfIndividualForEntity(docData, entity_id, L"");
	ElfRelationMap rel_map = docData->get_relations_by_individual(individual);
	return rel_map;
}
ElfRelation_ptr EIUtils::copyRelationAndSubstituteIndividuals(EIDocData_ptr docData, 
	ElfRelation_ptr old_relation, const std::wstring & source, ElfIndividual_ptr individual_to_replace, 
	const Mention* ment, const std::wstring & type, EDTOffset start, EDTOffset end){
	std::vector<ElfRelationArg_ptr> args = old_relation->get_args();
	std::vector<ElfRelationArg_ptr> arg_copy;
	const DocTheory* docTheory =  docData->getDocTheory();
	BOOST_FOREACH(ElfRelationArg_ptr oth_arg, args){
		ElfIndividual_ptr oth_indiv = oth_arg->get_individual();
		if(individual_to_replace != NULL && oth_indiv != NULL && oth_indiv->get_best_uri() !=  individual_to_replace->get_best_uri()){
			arg_copy.push_back(boost::make_shared<ElfRelationArg>(ElfRelationArg(oth_arg)));
		}
		else{
			std::vector<ElfRelationArg_ptr> new_args = createRelationArgFromMention(docData, oth_arg->get_role(), type, ment);
			arg_copy.insert(arg_copy.begin(), new_args.begin(), new_args.end());
		}
	}
	//get the offsets and text for the new relation
	LocatedString* relationTextLS = MainUtilities::substringFromEdtOffsets(docData->getDocument()->getOriginalText(), start, end);
	std::wstring r_text = relationTextLS->toString();
	delete relationTextLS;

	ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(old_relation->get_name(), arg_copy, r_text, start, end, old_relation->get_confidence(), old_relation->get_score_group());
	new_relation->set_source(old_relation->get_source());
	new_relation->add_source(L"eru:copyRelationAndSubstituteIndividuals");
	return new_relation;
}

ElfRelation_ptr EIUtils::copyRelationAndSubstituteArgs(EIDocData_ptr docData, const ElfRelation_ptr old_relation, 
															const std::wstring & source, 
															ElfRelationArg_ptr arg, const Mention* ment, 
															const std::wstring & type, EDTOffset start, EDTOffset end){
	std::vector<ElfRelationArg_ptr> args = old_relation->get_args();
	std::vector<ElfRelationArg_ptr> arg_copy;
	const DocTheory* docTheory =  docData->getDocTheory();
	BOOST_FOREACH(ElfRelationArg_ptr oth_arg, args){
		if(arg != oth_arg){
			arg_copy.push_back(boost::make_shared<ElfRelationArg>(ElfRelationArg(oth_arg)));
		}
		else{
			std::vector<ElfRelationArg_ptr> new_args = createRelationArgFromMention(
				docData, arg->get_role(), type, ment);
			arg_copy.insert(arg_copy.begin(), new_args.begin(), new_args.end());
		}
	}
	//get the offsets and text for the new relation
	LocatedString* relationTextLS = MainUtilities::substringFromEdtOffsets(
		docData->getDocument()->getOriginalText(), start, end);
	std::wstring r_text = relationTextLS->toString();
	delete relationTextLS;

	ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(old_relation->get_name(), 
		arg_copy, r_text, start, end, old_relation->get_confidence(), old_relation->get_score_group());
	new_relation->set_source(old_relation->get_source());
	new_relation->add_source(L"eru:copyRelationAndSubstituteArgs");
	new_relation->add_source(source);
	return new_relation;

}

std::vector<ElfRelationArg_ptr> EIUtils::getArgsWithEntity(ElfRelation_ptr relation, const Entity* entity){
	std::vector<ElfRelationArg_ptr> all_args = relation->get_args();
	std::vector<ElfRelationArg_ptr> matching_args;
	BOOST_FOREACH(ElfRelationArg_ptr arg, all_args){
		ElfIndividual_ptr individual = arg->get_individual();
		if (individual.get() != NULL){
			if (individual->get_entity_id() == entity->getID())
				matching_args.push_back(arg);
		}
	}
	return matching_args;
}

std::vector<std::vector<ElfRelation_ptr> > EIUtils::groupEquivalentRelations(
	const std::vector<ElfRelation_ptr> & relations){
	std::vector<std::vector<ElfRelation_ptr> > relation_groups;
	BOOST_FOREACH(ElfRelation_ptr relation, relations){
		bool found_match = false;
		BOOST_FOREACH(std::vector<ElfRelation_ptr> oth_relation_vector, relation_groups){
			if(relation->offsetless_equals(oth_relation_vector[0])){
				oth_relation_vector.push_back(relation);
				found_match = true;
				break;
			}
		}
		if(!found_match){
			std::vector<ElfRelation_ptr> new_group;
			new_group.push_back(relation);
			relation_groups.push_back(new_group);
		}
	}
	return relation_groups;
}

std::pair<EDTOffset, EDTOffset> EIUtils::getEDTOffsetsForMention(EIDocData_ptr docData, 
																	  const Mention* mention, bool head_offsets){
	const SynNode* node = mention->getNode();
	const TokenSequence* toks = 
		docData->getSentenceTheory(mention->getSentenceNumber())->getTokenSequence();
	if(head_offsets && mention->getHead() != NULL){
		node = mention->getHead();
	}
	EDTOffset start = toks->getToken(node->getStartToken())->getStartEDTOffset();
	EDTOffset end = toks->getToken(node->getEndToken())->getEndEDTOffset();
	return std::pair<EDTOffset, EDTOffset>(start, end);
}

std::pair<EDTOffset, EDTOffset> EIUtils::getSentenceEDTOffsetsForMention(EIDocData_ptr docData, const Mention* mention){
	const SynNode* node = mention->getNode();
	const TokenSequence* toks = 
		docData->getSentenceTheory(mention->getSentenceNumber())->getTokenSequence();
	EDTOffset start = toks->getToken(0)->getStartEDTOffset();
	EDTOffset end = toks->getToken(toks->getNTokens()-1)->getEndEDTOffset();
	return std::pair<EDTOffset, EDTOffset>(start, end);
}

bool EIUtils::hasAttendedSchoolRelation(EIDocData_ptr docData, ElfRelationArg_ptr per, ElfRelationArg_ptr org) {
	
	static const wstring SCHOOL_RELATION_NAME1 = L"eru:attendedSchool";
	static const wstring SCHOOL_ROLE1 = L"eru:educationalInstitution";
	static const wstring STUDENT_ROLE1 = L"eru:person";

	static const wstring SCHOOL_RELATION_NAME2 = L"eru:AttendsSchool";
	static const wstring SCHOOL_ROLE2 = L"eru:subjectOfAttendsSchool";
	static const wstring STUDENT_ROLE2 = L"eru:objectOfAttendsSchool";
	wstring SCHOOL_ROLE; 
	wstring STUDENT_ROLE; 
	if (per->get_individual() == ElfIndividual_ptr() ||
		org->get_individual() == ElfIndividual_ptr())
		return false;
	int perSentNum = getSentenceNumberForArg(docData->getDocTheory(), per);
	BOOST_FOREACH(ElfRelation_ptr school_relation, docData->get_relations()) {
		if (school_relation->get_name() == SCHOOL_RELATION_NAME1){
			SCHOOL_ROLE = SCHOOL_ROLE1;
			STUDENT_ROLE = STUDENT_ROLE1;
		}
		else if(school_relation->get_name() == SCHOOL_RELATION_NAME2){
			SCHOOL_ROLE = SCHOOL_ROLE2;
			STUDENT_ROLE = STUDENT_ROLE2;		
		}
		else{
			continue;
		}
		const ElfRelationArg_ptr schoolArg = school_relation->get_arg(SCHOOL_ROLE);
		const ElfRelationArg_ptr studentArg = school_relation->get_arg(STUDENT_ROLE);
		int studentSentNum = getSentenceNumberForArg(docData->getDocTheory(), studentArg);
		if(perSentNum !=  studentSentNum)	//in biographies people end up teaching where they studied
			continue;
		if (schoolArg != ElfRelationArg_ptr() && 
			studentArg != ElfRelationArg_ptr() &&
			schoolArg->get_individual() != ElfIndividual_ptr() &&
			schoolArg->get_individual()->get_best_uri() == org->get_individual()->get_best_uri() &&
			studentArg->get_individual() != ElfIndividual_ptr() &&
			studentArg->get_individual()->get_best_uri() == per->get_individual()->get_best_uri())
			return true;
	}
	return false;
}

bool EIUtils::hasCitizenship(EIDocData_ptr docData, const ElfIndividual_ptr& indiv, 
								  const std::wstring& country) {
	const std::wstring citizenship_relationName = L"eru:hasCitizenship";
	const std::wstring personRole = L"eru:person";
	const std::wstring gpeRole = L"eru:nationState";

	if (indiv) {
		const ElfRelationMap rels = docData->get_relations_by_individual(indiv);
		ElfRelationMap::const_iterator probe = rels.find(citizenship_relationName);
		if (probe != rels.end()) {
			std::vector<ElfRelation_ptr> citizenshipRels = probe->second;
			BOOST_FOREACH(const ElfRelation_ptr& rel, citizenshipRels) {
				const ElfRelationArg_ptr person = rel->get_arg(personRole);
				const ElfRelationArg_ptr gpe = rel->get_arg(gpeRole);

				if (person && gpe) {
					const ElfIndividual_ptr personIndiv = person->get_individual();
					const ElfIndividual_ptr gpeIndiv = gpe->get_individual();
					if (gpeIndiv && personIndiv 
						&& personIndiv->get_best_uri() == indiv->get_best_uri()) 
					{
						const std::set<std::wstring> gpeSyns = 
							getCountrySynonymsForIndividual(docData, gpeIndiv);	
						if (gpeSyns.find(country) != gpeSyns.end()) {
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

bool EIUtils::isContinentConflict(EIDocData_ptr docData, ElfRelationArg_ptr super, ElfRelationArg_ptr sub){
	std::string continent_file = ParamReader::getParam("continent_expansions_table", "NONE");
	if(continent_file == "NONE")
		return false;

	if (!super->get_individual()->has_entity_id() || !sub->get_individual()->has_entity_id())
		return false;

	Entity *e1 = docData->getEntity(super->get_individual()->get_entity_id());
	Entity *e2 = docData->getEntity(sub->get_individual()->get_entity_id());
	//never let a continent be a subGPE
	if (e2 == 0)
		return false;
	const Mention* continent2 = getContinent(docData, e2);
	if (continent2 != 0){
		return true;	//never let a continent be sub location
	}
	//super must be a continent
	if (e1 == 0)
		return false;
	const Mention* continent1 = getContinent(docData, e1);
	if (continent1 == 0){
		return false;
	}

	const SynNode* head = continent1->getAtomicHead();
	std::wstring head_string = UnicodeUtil::normalizeTextString(head->toTextString());
	if(isInContinent(docData, head_string, e2))
		return false;
	const EntitySet* entities = docData->getEntitySet();
	std::set<int> super_locations = getSuperLocations(docData, e2->getID());
	BOOST_FOREACH(int super, super_locations){
		if(isInContinent(docData, head_string, entities->getEntity(super)))
			return false;
	}
	return true;
}

std::set<int> EIUtils::getSuperLocations(EIDocData_ptr docData, int entity_id){
	return docData->getDocumentPlaceInfo(entity_id).getWKSuperLocations();
}
//Warning: Untested
std::set<int> EIUtils::getSubLocations(EIDocData_ptr docData, int entity_id){
	return docData->getDocumentPlaceInfo(entity_id).getWKSubLocations();
	
}

bool EIUtils::isInContinent(EIDocData_ptr docData, const std::wstring & continent, const Entity* entity){
	std::vector<std::wstring> known_subs = _continentExpansions[continent];
	std::set<std::wstring> sub_set(known_subs.begin(), known_subs.end());
	bool has_name = false;
	const Mention* n;
	const EntitySet* entities = docData->getEntitySet();
	//check for known subs
	for(int i =0; i< entity->getNMentions(); i++){
		const Mention* m = entities->getMention(entity->getMention(i));
		if(m->getMentionType() != Mention::NAME)
			continue;
		if(m->getHead() == 0)
			continue;
		const SynNode* head = m->getAtomicHead();
		if(!head)
			continue;
		has_name = true;
		n = m;
		std::wstring head_string = UnicodeUtil::normalizeTextString(head->toTextString());
		if(sub_set.find(head_string) != sub_set.end()){
			return true;
		}
	}
	if(!has_name){
		return true;
	}
	else{
		return false;
	}
}

void EIUtils::initLocationTables() {
	static bool init = false;
	if (!init) {
		std::string continent_file = ParamReader::getParam("continent_expansions_table", "NONE");
		std::string loc_file = ParamReader::getParam("location_expansions_table");
		if(!loc_file.empty()) {
			SessionLogger::info("loc_exp_0")<<"Loading location_expansions_table " << loc_file;
			readSTOPSeparatedSubLocationFile(loc_file, _locationStringsInTable, _locationExpansions);
		} 
		else {
			SessionLogger::warn("emp_loc_exp_0") << "ELFInference: empty location expansions file\n";			
		}
		if(continent_file != "NONE"){
			SessionLogger::info("cont_exp_0")<<"Loading continent_expansions_table " << continent_file;
			readSTOPSeparatedSubLocationFile(continent_file, _continentStringsInTable, 
				_continentExpansions);
		}
		init = true;		
	}
}

bool EIUtils::extractPersonMemberishRelation(const ElfRelation_ptr& relation, 
		wstring& relName, ElfIndividual_ptr& org, ElfIndividual_ptr& person) {
	relName = relation->get_name();
	wstring orgRole=L"", personRole=L"";

	if (L"eru:isLedBy" == relName) {
		orgRole = L"eru:humanOrganization";
		personRole = L"eru:leadingPerson";
	} else if (L"eru:hasMember" == relName) {
		orgRole = L"eru:personGroup";
		personRole = L"eru:personMember";		
	} else if (L"eru:hasPersonMember" == relName) {
		orgRole = L"eru:personGroup";
		personRole = L"eru:personMember";		
	} else if (L"eru:employs" == relName) {
		orgRole = L"eru:humanAgentEmployer";
		personRole = L"eru:personEmployed";
	} else {
		return false;
	}

	ElfRelationArg_ptr org_arg = relation->get_arg(orgRole);
	ElfRelationArg_ptr person_arg = relation->get_arg(personRole);

	if (org_arg && person_arg) {
		org = org_arg->get_individual();
		person = person_arg->get_individual();
	}

	return org && person;
}

bool EIUtils::isSubOrgOfCountry(EIDocData_ptr docData, const ElfIndividual_ptr indiv, 
									 const wstring& countryName) 
{
	if (indiv) {
		const ElfRelationMap rels = docData->get_relations_by_individual(indiv);
		//SessionLogger::info("LEARNIT") << "MilOrg is in " << rels.size() << " different types of relations" << endl;
		/*BOOST_FOREACH(ElfRelationMap::value_type t, rels) {
			SessionLogger::info("LEARNIT") << "\t" << t.first << endl;
		}*/

		ElfRelationMap::const_iterator probe = rels.find(L"eru:gpeHasSubOrg");
		if (probe != rels.end()) {
			std::vector<ElfRelation_ptr> subOrgRels = probe->second;
			BOOST_FOREACH(const ElfRelation_ptr& rel, subOrgRels) {
				if (L"eru:gpeHasSubOrg" == rel->get_name()) {
					//SessionLogger::info("LEARNIT") << "Is subOrg of something!" << endl;
					const ElfRelationArg_ptr sub = rel->get_arg(L"eru:sub");
					const ElfRelationArg_ptr super = rel->get_arg(L"eru:super");

					if (sub && super) {
						const ElfIndividual_ptr subIndiv = sub->get_individual();
						const ElfIndividual_ptr superIndiv = super->get_individual();
						if (subIndiv && subIndiv->get_best_uri() == indiv->get_best_uri()) {
							if (superIndiv) {
								std::set<std::wstring> superSyns = 
									EIUtils::getCountrySynonymsForIndividual(docData, superIndiv);
								if (superSyns.find(countryName) != superSyns.end()) {
									return true;
								}
							}
						}
					}
				}
			}
		}
	}
	return false;
}

bool EIUtils::isMilitaryORG(EIDocData_ptr docData, const ElfIndividual_ptr& org) {
	if (org->has_entity_id()) {
		const Entity* entity = docData->getEntity(org->get_entity_id());

		if (entity && entity->getType().matchesORG()) {
			for (int i=0; i<entity->getNMentions(); ++i) {
				const Mention* ment = 
					docData->getEntitySet()->getMention(entity->getMention(i));
				const SynNode* node = ment->getAtomicHead();
				std::wstring name = node->toTextString();
				
				// TODO: check about casing here
				if (name.find(L"military") != wstring::npos ||
					name.find(L"troops") != wstring::npos ||
					name.find(L"army") != wstring::npos ||
					name.find(L"navy") != wstring::npos ||
					name.find(L"forces") !=wstring::npos) 
				{
					return true;	
				}
			}
		}
	}
	return false;
}


//////////////////////////////
//                          //
// LOCATION-RELATED METHODS //
//   -- for class           //
//   -- for document        //
//                          //
//////////////////////////////

void EIUtils::readSTOPSeparatedSubLocationFile(const std::string & file_name, 
		std::set<std::wstring>& strings_in_table, 
		std::map<std::wstring, std::vector<std::wstring> >& expansion_table){
	boost::scoped_ptr<UTF8InputStream> inStream_scoped_ptr(UTF8InputStream::build(file_name.c_str()));
	UTF8InputStream& inStream(*inStream_scoped_ptr);
	std::wstring line;
	while (getline(inStream, line)) {
		std::vector<std::wstring> parts;
		boost::algorithm::split_regex(parts, line, boost::basic_regex<wchar_t>(L":STOP"));
		if (parts.size() > 1) {
			std::wstring base_name = parts[0];
			boost::trim(base_name);
			strings_in_table.insert(base_name);
			std::wstring first_sub = parts[1];
			boost::trim(first_sub);
			// have to get rid of integer
			size_t space_index = first_sub.find(L" ");
			if (space_index == std::wstring::npos) {
				SessionLogger::warn("LEARNIT") << "ignoring ill-formed line in expansions file: " 
					<< file_name << ", \nline: " << line << std::endl;
				continue;
			}
			first_sub = first_sub.substr(space_index);
			boost::trim(first_sub);
			std::vector<std::wstring> subLocations;
			subLocations.push_back(first_sub);
			strings_in_table.insert(first_sub);
			for (size_t i = 2; i < parts.size(); i++) {	
				std::wstring sub = parts.at(i);
				boost::trim(sub);
				subLocations.push_back(sub);
				strings_in_table.insert(sub);
			}
			expansion_table[base_name] = subLocations;
		} else if (line.length() > 0)  {
			SessionLogger::warn("LEARNIT") << "ignoring ill-formed line in location expansions file: " << 
				file_name << ", \nline: " << line << std::endl;
		}
	}
	inStream.close();
}

void EIUtils::normalizeForTerroristOrgs(std::wstring& name) {
	boost::trim(name);
	name = UnicodeUtil::normalizeNameString(
			UnicodeUtil::normalizeTextString(name));
	boost::to_lower(name);
	boost::replace_all(name, L"'s", L" 's");
}

bool EIUtils::isTerroristOrganizationName(const std::wstring& name) {
	static std::set<std::wstring> terroristOrganizations;
	static bool init = false;

	if (!init) {
		init = true;
		loadTerroristOrganizations(terroristOrganizations);
	}

	std::wstring normName = name;
	normalizeForTerroristOrgs(normName);

	return terroristOrganizations.find(normName) != terroristOrganizations.end();
}

void EIUtils::loadTerroristOrganizations(std::set<std::wstring>& terroristOrgs) {
	std::string terroristOrganizationsFile = 
		ParamReader::getParam("terrorist_organizations");
	if (!terroristOrganizationsFile.empty()) {
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(terroristOrganizationsFile.c_str()));
		UTF8InputStream& in(*in_scoped_ptr);
		std::wstring line;
		while (getline(in, line)) {
			std::vector<std::wstring> parts;
			boost::split(parts, line, boost::is_any_of("\t"));
			if (!parts.empty()) {
				std::wstring name = parts[0];
				normalizeForTerroristOrgs(name);
				terroristOrgs.insert(name);
			}
		}
	}
	SessionLogger::info("load_terrorists") << L"Loaded "
		<< terroristOrgs.size() << " terrorist organization names";
}

////////////////////////////////
//                            //
// FUNCTIONS TO ADD TYPES     //
//                            //
////////////////////////////////

void EIUtils::initIndividualTypeNameList() {
	static bool init = false;
	if (!init) {
		std::string individual_types_file = ParamReader::getParam("individual_type_name_list");
		if (individual_types_file != string()) {
			boost::scoped_ptr<UTF8InputStream> inStream_scoped_ptr(UTF8InputStream::build(individual_types_file.c_str()));
			UTF8InputStream& inStream(*inStream_scoped_ptr);
			std::wstring line;
			while (getline(inStream, line)) {
				std::vector<std::wstring> parts;
				boost::split(parts, line, boost::is_any_of("\t"));
				if (parts.size() == 2) {
					std::wstring name = parts[0];
					std::wstring type = parts[1];
					name = UnicodeUtil::normalizeNameString(UnicodeUtil::normalizeTextString(name));
					boost::to_lower(name);
					boost::replace_all(name,L"'s",L" 's"); // SERIF always breaks these off
					if (_individualTypesByName.find(name) != _individualTypesByName.end()) {
						//SessionLogger::warn("LEARNIT") << "duplicate name in individual type name list: " << line << std::endl;
					} else {
						_individualTypesByName[name] = type;
					}
				} else if (line.length() > 0)  {
					SessionLogger::warn("LEARNIT") << "ignoring ill-formed line in individual type name list: " 
						<< std::endl << line << std::endl;
				}
			}
			inStream.close();
		} else {
			SessionLogger::warn("LEARNIT") << "no individual type name list specified\n";
		}
	}
}

void EIUtils::addIndividualTypes(EIDocData_ptr docData) {
	
	const DocTheory* docTheory = docData->getDocTheory();
	const EntitySet* entity_set = docTheory->getEntitySet();

	// Accumulate any cloned but unmerged individuals with new types
	ElfIndividualSet individuals_to_add;

	BOOST_FOREACH(ElfIndividual_ptr ind, docData->get_individuals_by_type()) {
		if (!ind->has_entity_id())
			continue;
		const Entity *entity = entity_set->getEntity(ind->get_entity_id());
		if (entity == 0)
			continue;

		const GrowableArray<MentionUID> &ments = entity->mentions;

		for (int j = 0; j < ments.length(); j++) {
			const Mention *ment = entity_set->getMention(ments[j]);	
			if (ment->getMentionType() != Mention::NAME)
				continue;
			// atomic head is probably what you want to check; but let's do both
			std::wstring full_name = ment->getNode()->toTextString();
			full_name = UnicodeUtil::normalizeNameString(UnicodeUtil::normalizeTextString(full_name));
			boost::to_lower(full_name);
			std::map<std::wstring, std::wstring>::iterator iter = _individualTypesByName.find(full_name);			
			if (iter == _individualTypesByName.end()) {
				std::wstring atomic_head_name = ment->getAtomicHead()->toTextString();
				atomic_head_name = UnicodeUtil::normalizeNameString(UnicodeUtil::normalizeTextString(atomic_head_name));
				boost::to_lower(atomic_head_name);
				iter = _individualTypesByName.find(atomic_head_name);
			}
			if (iter != _individualTypesByName.end()) {
				TokenSequence *ts = docTheory->getSentenceTheory(ment->getSentenceNumber())->getTokenSequence();
				EDTOffset start = ts->getToken(0)->getStartEDTOffset();
				EDTOffset end = ts->getToken(ts->getNTokens()-1)->getEndEDTOffset();
				LocatedString* token_span = MainUtilities::substringFromEdtOffsets(docTheory->getDocument()->getOriginalText(), start, end);
				std::wstring text = std::wstring(token_span->toString());
				delete token_span;

				// Copy the individual, but override with the new type evidence
				ElfIndividual_ptr retyped_individual = boost::make_shared<ElfIndividual>(ind);
				retyped_individual->set_type(boost::make_shared<ElfType>((*iter).second, text, start, end));
				individuals_to_add.insert(retyped_individual);
			}
			else if(ParamReader::getRequiredTrueFalseParam("coerce_bound_types") && 
				!ind->has_type(L"ic:PoliticalParty") && !ind->has_type(L"ic:TerroristOrganization")){
				const SynNode* head = ment->getAtomicHead();
				if(head != 0){
					Symbol head_words[20];
					int nwords = head->getTerminalSymbols(head_words, 19);
					bool coerceParty = false;
					for(int h = 0; h < nwords; h++){
						if(_democraticWords.find(head_words[h].to_string()) != _democraticWords.end()){
							TokenSequence *ts = docTheory->getSentenceTheory(ment->getSentenceNumber())->getTokenSequence();
							EDTOffset start = ts->getToken(0)->getStartEDTOffset();
							EDTOffset end = ts->getToken(ts->getNTokens()-1)->getEndEDTOffset();
							LocatedString* token_span = MainUtilities::substringFromEdtOffsets(docTheory->getDocument()->getOriginalText(), start, end);
							std::wstring text = std::wstring(token_span->toString());
							delete token_span;
							// get_individuals_by_type() returns copies of individuals. we want the real one.
							// Copy the individual, but override with the new type evidence
							ElfIndividual_ptr retyped_individual = boost::make_shared<ElfIndividual>(ind);
							retyped_individual->set_type(boost::make_shared<ElfType>(L"ic:PoliticalParty", text, start, end));
							individuals_to_add.insert(retyped_individual);
							coerceParty = true;
						}
					}
					if(!coerceParty){
						for(int h = 0; h < nwords; h++){
							if(_governmentWords.find(head_words[h].to_string()) != _governmentWords.end()){
								TokenSequence *ts = docTheory->getSentenceTheory(ment->getSentenceNumber())->getTokenSequence();
								EDTOffset start = ts->getToken(0)->getStartEDTOffset();
								EDTOffset end = ts->getToken(ts->getNTokens()-1)->getEndEDTOffset();
								LocatedString* token_span = MainUtilities::substringFromEdtOffsets(docTheory->getDocument()->getOriginalText(), start, end);
								std::wstring text = std::wstring(token_span->toString());
								delete token_span;
								// get_individuals_by_type() returns copies of individuals. we want the real one.
								ElfIndividual_ptr retyped_individual = boost::make_shared<ElfIndividual>(ind);
								retyped_individual->set_type(boost::make_shared<ElfType>(L"ic:GovernmentOrganization", text, start, end));
								individuals_to_add.insert(retyped_individual);
							}
						}
					}
				}
			}
		}
	}

	// Insert any generated type assertions
	BOOST_FOREACH(ElfIndividual_ptr individual, individuals_to_add) {
		docData->getElfDoc()->insert_individual(individual);
	}
}

////////////////////////////////
//                            //
// FUNCTIONS TO ADD ARGUMENTS //
//                            //
////////////////////////////////

void EIUtils::getDates(const DocTheory* docTheory, int sent_no, const Proposition *prop, std::set<const ValueMention*>& dateMentions, 
					   bool ignoreOrphanCheck, std::set<const Proposition*> seenProps) {	
	const SentenceTheory* sTheory = docTheory->getSentenceTheory(sent_no);
	const PropositionSet *pSet = sTheory->getPropositionSet();
	const MentionSet *mSet = sTheory->getMentionSet();		
	const SynNode* root = sTheory->getPrimaryParse()->getRoot();
	//std::wcout<<"Sentence : "<<root->toTextString()<<std::endl;

	Symbol predSym = prop->getPredSymbol();
	if (predSym == Symbol(L"wanted") || predSym == Symbol(L"arrested") || predSym == Symbol(L"condemned") || predSym == Symbol(L"denounced") || 
		predSym == Symbol(L"vilified") || predSym == Symbol(L"admonished") || predSym == Symbol(L"blasted") || predSym == Symbol(L"assailed") || 
		predSym == Symbol(L"castigated") || predSym == Symbol(L"censured") || predSym == Symbol(L"castigated") || predSym == Symbol(L"chastised") || 
		predSym == Symbol(L"chided") || predSym == Symbol(L"criticized") || predSym == Symbol(L"criticised") || predSym == Symbol(L"decried") || 
		predSym == Symbol(L"denigrated") || predSym == Symbol(L"deplored") || predSym == Symbol(L"derided") || predSym == Symbol(L"lambasted") || 
		predSym == Symbol(L"rebuked") || predSym == Symbol(L"scolded") || predSym == Symbol(L"slammed") || predSym == Symbol(L"court-martialed") || 
		predSym == Symbol(L"charged") || predSym == Symbol(L"tried") || predSym == Symbol(L"sentenced") ||
		predSym == Symbol(L"said") || predSym == Symbol(L"told") || predSym == Symbol(L"reported"))		
		return;

	//find unattached date propositions
	const ValueMentionSet* vset = docTheory->getSentenceTheory(sent_no)->getValueMentionSet();
	std::set<const ValueMention*> orphanedDates;
	for(int pno = 0; pno < pSet->getNPropositions(); pno++){
		if(pSet->getProposition(pno)->getPredType()== Proposition::MODIFIER_PRED){
			int start = sTheory->getTokenSequence()->getNTokens()+1;
			int end = -1;
			pSet->getProposition(pno)->getStartEndTokenProposition(mSet, start, end);
			const ValueMention* vm = 0;
			for(int vno = 0; vno < vset->getNValueMentions(); vno++){
				if((vset->getValueMention(vno)->getStartToken() == start) &&
					(vset->getValueMention(vno)->getEndToken() == end)){
						vm = vset->getValueMention(vno); ; 
				}
			}
			//check that it is really orphaned (e.g. doesn't exist as a temporal elsewhere)
			if(vm != 0){
				if (ignoreOrphanCheck) {
					orphanedDates.insert(vm);
				} else {
					const Proposition* modProp = pSet->getProposition(pno);
					const Mention* modRefMention = modProp->getMentionOfRole(Argument::REF_ROLE, mSet);
					bool found_ref = false;
					for(int opno = 0; opno < pSet->getNPropositions(); opno++){
						if(opno == pno)
							continue;
						Symbol role = pSet->getProposition(opno)->getRoleOfMention(modRefMention, mSet);
						if(role != Symbol())
							found_ref = true;
					}
					if(!found_ref){
						orphanedDates.insert(vm);
					}	
				}
			}
		}
	}
	//std::wcout<<"Orphans: "<<orphanedDates.size()<<std::endl;
	//BOOST_FOREACH(const ValueMention* vm, orphanedDates){
	//	std::wcout<<"\t"<< vm->getDocValue()->getTimexVal().to_string()<<std::endl;
	//}
	
	for (int a = 0; a < prop->getNArgs(); a++) {
		Argument *arg = prop->getArg(a);
		// Any mention arg, not just the temporal ones
		if (arg->getType() == Argument::MENTION_ARG) {
			const Mention *ment = arg->getMention(mSet);
			//if(orphanedDates.size() > 0){
			//	std::wcout<<"Sentence : "<<root->toTextString()<<std::endl;
			//	std::wcout<<"Mention: "<<ment->getAtomicHead()->toTextString()<<" -- "<<ment->getNode()->getStartToken()<<", "<<ment->getNode()->getEndToken()<<std::endl;
			//}
			// Get dates directly from this mention (i.e. if it is = "1998", no propagation)
			getDates(docTheory, sent_no, ment, arg->getRoleSym() == Argument::TEMP_ROLE, dateMentions);
			// Add orphan dates that are close to this mention
			BOOST_FOREACH(const ValueMention* orphan, orphanedDates){
				//std::wcout<<"\torphan: "<<orphan->getDocValue()->getTimexVal().to_string()<<" --"<<orphan->getStartToken()<<", "<<orphan->getEndToken()<<std::endl;
				if((orphan->getEndToken() == ment->getNode()->getEndToken()) &&
					orphan->getStartToken() == ment->getNode()->getStartToken()){
						//std::wcout<<"\t***inserted"<<std::endl;
					dateMentions.insert(orphan);
				} 
				else if((ment->getNode()->getParent() != 0) &&
					(ment->getNode()->getParent()->getTag() == Symbol(L"PP")) &&
					(orphan->getStartToken() == (ment->getNode()->getEndToken()+2))){
					dateMentions.insert(orphan);
					//std::wcout<<"\t***inserted"<<std::endl;
				} 
			}
			// Now look at things that modify this mention, by getting its definition and calling getDates on that
			//   Track seen propositions do we don't recurse until a stack overflow
			seenProps.insert(prop);
			const Proposition *defProp = pSet->getDefinition(ment->getIndex());
			if (defProp != 0 && seenProps.find(defProp) == seenProps.end())
				getDates(docTheory, sent_no, defProp, dateMentions, ignoreOrphanCheck, seenProps);
		} 
	}
}

void EIUtils::getDates(const DocTheory* docTheory, int sent_no, const Mention *mention, 
									bool is_from_temp_arg, std::set<const ValueMention*>& dateMentions) 
{	
	PropositionSet *pSet = docTheory->getSentenceTheory(sent_no)->getPropositionSet();
	MentionSet *mSet = docTheory->getSentenceTheory(sent_no)->getMentionSet();		
	// First look to see if this thing is a date itself
	int stoken = mention->getNode()->getStartToken();
	int etoken = mention->getNode()->getEndToken();
	ValueMentionSet *vmSet = docTheory->getSentenceTheory(sent_no)->getValueMentionSet();
	getDatesWithinTokenSpan(docTheory, vmSet, stoken, etoken, is_from_temp_arg, dateMentions);

	// Next, see if it is modified by a temporal argument in the PropositionSet
	// This is somewhat redundant with the check in the getDates() function for propositions,
	//   but not if we got here by some other method.
	const Proposition *defProp = pSet->getDefinition(mention->getIndex());
	if (defProp != 0) {
		for (int a = 0; a < defProp->getNArgs(); a++) {
			Argument *arg = defProp->getArg(a);
			if (arg->getRoleSym() == Argument::TEMP_ROLE && arg->getType() == Argument::MENTION_ARG) {
				getDates(docTheory, sent_no, arg->getMention(mSet), true, dateMentions);
			} 
		}
	}

	// Then desperately, look for modifiers that are dates that point at this mention
	for (int p = 0; p < pSet->getNPropositions(); p++) {
		const Proposition *prop = pSet->getProposition(p);
		if (prop->getPredType() == Proposition::MODIFIER_PRED &&
			prop->getNArgs() > 0 &&
			prop->getArg(0)->getRoleSym() == Argument::REF_ROLE &&
			prop->getArg(0)->getType() == Argument::MENTION_ARG &&
			prop->getArg(0)->getMentionIndex() == mention->getIndex())
		{
			const SynNode *node = prop->getPredHead();
			getDatesWithinTokenSpan(docTheory, vmSet, node->getStartToken(), node->getEndToken(), is_from_temp_arg, dateMentions);
		}
	}

	// Before you finish, make sure you recurse down sets and partitives and whatnot
	const Mention *iter = mention->getChild();
	while (iter != 0) {
		getDates(docTheory, sent_no, iter, is_from_temp_arg, dateMentions);
		iter = iter->getNext();
	}		
}

void EIUtils::getDatesWithinTokenSpan(const DocTheory* docTheory, ValueMentionSet *vmSet, int stoken, int etoken, bool is_from_temp_arg, std::set<const ValueMention*>& dateMentions) {
	for (int v = 0; v < vmSet->getNValueMentions(); v++) {
		ValueMention *vm = vmSet->getValueMention(v);
		if (!vm->isTimexValue())
			continue;		
		if ((vm->getStartToken() == stoken && vm->getEndToken() == etoken) ||
		    (is_from_temp_arg && vm->getStartToken() >= stoken && vm->getEndToken() <= etoken) ||
			(is_from_temp_arg && vm->getStartToken() <= stoken && vm->getEndToken() >= etoken))
		{
			// Exact match OR look for the temporal INSIDE or AROUND this mention, sine 
			// sometimes it gets hidden, as in "his appearance Tuesday", or 
			// strangely expanded, as in VM="the July 27", node="July 27"
			Value *val = docTheory->getValueSet()->getValueByValueMention(vm->getUID());
			dateMentions.insert(vm);
		}
	}


}

std::vector<const ValueMention*> EIUtils::getDatesFromSentence(EIDocData_ptr docData, int sent_no) {
	std::vector<const ValueMention*> dateMentions;
	ValueMentionSet *vmSet = docData->getSentenceTheory(sent_no)->getValueMentionSet();
	for (int v = 0; v < vmSet->getNValueMentions(); v++) {
		ValueMention *vm = vmSet->getValueMention(v);
		if (!vm->isTimexValue())
			continue;	
		Value *val = docData->getDocTheory()->getValueSet()->getValueByValueMention(vm->getUID());
		if (val->isSpecificDate())
			dateMentions.push_back(vm);
	}
	return dateMentions;
}

bool EIUtils::isViolentTBDEvent(PatternFeatureSet_ptr featureSet) {	
	for (size_t i = 0; i < featureSet->getNFeatures(); i++) {
		PatternFeature_ptr feature = featureSet->getFeature(i);
		if (ReturnPatternFeature_ptr rf = dynamic_pointer_cast<ReturnPatternFeature>(feature)) {
			//role
			std::wstring tbd = rf->getReturnValue(L"tbd");
			if (tbd.compare(L"bombing") == 0 ||
				tbd.compare(L"killing") == 0 ||
				tbd.compare(L"injury") == 0 ||
				tbd.compare(L"attack") == 0)
				return true;
		}
	}
	return false;
}

bool EIUtils::containsKBPStart(PatternFeatureSet_ptr featureSet){
	for (size_t i = 0; i < featureSet->getNFeatures(); i++) {
		PatternFeature_ptr feature = featureSet->getFeature(i);
		if (ReturnPatternFeature_ptr rf = dynamic_pointer_cast<ReturnPatternFeature>(feature)) {
			std::wstring role = rf->getReturnValue(L"role");
			if (role.compare(L"KBP:START") == 0){
				return true;
			}
		}
	}
	return false;
}



bool EIUtils::containsKBPEnd(PatternFeatureSet_ptr featureSet){
	for (size_t i = 0; i < featureSet->getNFeatures(); i++) {
		PatternFeature_ptr feature = featureSet->getFeature(i);
		if (ReturnPatternFeature_ptr rf = dynamic_pointer_cast<ReturnPatternFeature>(feature)) {
			std::wstring role = rf->getReturnValue(L"role");
			if (role.compare(L"KBP:END") == 0){
				return true;
			}
		}
	}

	return false;
}

const ValueMention* EIUtils::getBestDateMention(std::set<const ValueMention*> dateMentions, std::set<const ValueMention*> nonSpecificDateMentions,
												SentenceTheory* sent_theory, PatternFeatureSet_ptr featureSet) {
	const ValueMention* bestDate = 0;

	if(dateMentions.size() > 0) {
		bestDate = (*dateMentions.begin());
		//get the best date mention, since for start/end we only allow one
		if(dateMentions.size() > 1){
			const SynNode* root = sent_theory->getPrimaryParse()->getRoot();
			std::set<const SynNode*> nodes;
			for (size_t i = 0; i < featureSet->getNFeatures(); i++) {
				PatternFeature_ptr feature = featureSet->getFeature(i);
				if (MentionReturnPFeature_ptr mf = dynamic_pointer_cast<MentionReturnPFeature>(feature)){
					if(mf->getMention()->getHead() != 0)
						nodes.insert(mf->getMention()->getHead());
					else
						nodes.insert(mf->getMention()->getNode()->getHead());	
				}
			}
			/*std::wcout<<"Sentence: "<<root->toTextString()<<std::endl;
			std::wcout<<"Selecting best date from: \t";
			BOOST_FOREACH(const ValueMention* dateMention, dateMentions){
				std::wcout<<dateMention->getDocValue()->getTimexVal().to_string()<<", ";
			}
			std::wcout<<"\nWith Argument nodes: \t"<<std::endl;
			BOOST_FOREACH(const SynNode* node, nodes){
				std::wcout<<"("<<node->toTextString()<<"), ";
			}
			std::wcout<<std::endl;*/
			const SynNode* node = (*nodes.begin());
			int d;
			if(bestDate->getStartToken() > node->getEndToken()){
				d = bestDate->getStartToken() - node->getEndToken();
			}
			else{
				d = node->getStartToken() - bestDate->getEndToken();
			}
			int bestDistance = d;
			const SynNode* bestNode = node;
			BOOST_FOREACH(const ValueMention* dateMention, dateMentions){
				//std::wcout<<"Date Mention: "<<dateMention->getDocValue()->getTimexVal().to_string()<<std::endl;
				BOOST_FOREACH(const SynNode* node, nodes){
					//std::wcout<<"\tNode: "<<node->toTextString();
					int d;
					if(dateMention->getStartToken() > node->getEndToken()){
						d = dateMention->getStartToken() - node->getEndToken();
					}
					else{
						d = node->getStartToken() - dateMention->getEndToken();
					}
					std::string txt = node->toDebugTextString();
					//std::wcout<<"\tDistance: "<<d;						
					if(d < bestDistance){
						bestDistance = d;
						bestDate = dateMention;
						//std::wcout<<" \tSET AS BEST";
					}
					//std::wcout<<" ......"<<std::endl;					
				}
			}
		}
	}
	else if (nonSpecificDateMentions.size() == 1){
		/* //these typically are 'current', and are not related to the start/end verbs
		if((*nonSpecificDateMentions.begin())->getDocValue()->getTimexVal() == EIUtils::PRESENT_REF){
			bestDate = (*nonSpecificDateMentions.begin());
		}*/
	}
	return bestDate;
}


bool EIUtils::isStartingRangeDate(const SentenceTheory* sent_theory, const ValueMention* vm){
	std::wstring prev_word = L"";
	std::wstring prev2_word = L"";
	int startToken = vm->getStartToken();
	if(startToken > 0){
		prev_word = sent_theory->getTokenSequence()->getToken(startToken-1)->getSymbol().to_string();
		std::transform(prev_word.begin(), prev_word.end(), prev_word.begin(), ::tolower);
	}
	if(startToken > 1){
		prev2_word = sent_theory->getTokenSequence()->getToken(startToken-2)->getSymbol().to_string();
		std::transform(prev2_word.begin(), prev2_word.end(), prev2_word.begin(), ::tolower);
	}
	if(prev_word == L"from")
		return true;
	if(prev_word == L"since")
		return true;
	else if(prev_word == L"in" && prev2_word == L"starting")
		return true;
	else if(prev_word == L"in" && prev2_word == L"beginning")
		return true;
	return false;
			
}
bool EIUtils::isEndingRangeDate(const SentenceTheory* sent_theory, const ValueMention* vm){
	std::wstring prev_word = L"";
	std::wstring prev2_word = L"";
	int startToken = vm->getStartToken();
	if(startToken > 0){
		prev_word = sent_theory->getTokenSequence()->getToken(startToken-1)->getSymbol().to_string();
		std::transform(prev_word.begin(), prev_word.end(), prev_word.begin(), ::tolower);
	}
	if(startToken > 1){
		prev2_word = sent_theory->getTokenSequence()->getToken(startToken-2)->getSymbol().to_string();
		std::transform(prev2_word.begin(), prev2_word.end(), prev2_word.begin(), ::tolower);
	}
	if(prev_word == L"until")
		return true;
	else if(prev_word == L"to")
		return true;
	else if(prev_word == L"through")
		return true;
	return false;
			
}
// YYYY exact


std::wstring EIUtils::getKBPSpecString(const DocTheory* docTheory, const ValueMention* vm){
	using namespace boost::gregorian;
	if(vm == 0){
		return L"[,]";
	}
	const Value* val =  docTheory->getValueSet()->getValueByValueMention(vm->getUID());

	if(val == 0){
		return L"[,]";
	}
	std::wstringstream rtnStream;
	Symbol timex_val =	val->getTimexVal();
	std::wstring norm_str = L"";
	if(timex_val == Symbol()){
		norm_str = vm->toString(docTheory->getSentenceTheory(vm->getSentenceNumber())->getTokenSequence());
	}
	else{
		norm_str = timex_val.to_string();
	}
	boost::wcmatch matchResult;
	if(timex_val == PAST_REF){
		std::wstring datestr = docTheory->getSentenceTheory(vm->getSentenceNumber())->getTokenSequence()->toString(vm->getStartToken(), vm->getEndToken());
		//std::wcout<<"getKBPSpecString(): PASTREF "<<norm_str<<" from: "<<datestr<<std::endl;
		rtnStream <<"[,"<<val->getTimexAnchorVal().to_string()<<"]";
	} else if(timex_val == PRESENT_REF){
		std::wstring datestr = docTheory->getSentenceTheory(vm->getSentenceNumber())->getTokenSequence()->toString(vm->getStartToken(), vm->getEndToken());
		std::wcout<<"getKBPSpecString(): CURRENT_REF "<<norm_str<<" from: "<<datestr<<std::endl;
		rtnStream <<"["<<val->getTimexAnchorVal().to_string()<<","<<val->getTimexAnchorVal().to_string()<<"]";
	} else if(timex_val == FUTURE_REF){
		std::wstring datestr = docTheory->getSentenceTheory(vm->getSentenceNumber())->getTokenSequence()->toString(vm->getStartToken(), vm->getEndToken());
		std::wcout<<"getKBPSpecString(): FUTURE_REF "<<norm_str<<" from: "<<datestr<<std::endl;
		rtnStream <<"["<<val->getTimexAnchorVal().to_string()<<",]";
	} else{
		std::vector<std::wstring> ymd = TimexUtils::parseYYYYMMDD(norm_str);
		int ymd_size = 0;
		BOOST_FOREACH(wstring s, ymd){
			if(s != L"")
				ymd_size+=1;
		}
		if(ymd_size == 1){//YYYY
			rtnStream <<"["<<ymd[0]<<"-01-01,"<<ymd[0]<<"-12-31]";
		} else if(ymd_size == 2){//YYYY-MM
			try {
				unsigned short year = boost::lexical_cast<unsigned short>(ymd[0]);
				unsigned short month = boost::lexical_cast<unsigned short>(ymd[1]);
				rtnStream <<"["<<ymd[0]<<"-"<<ymd[1]<<"-01,"<<ymd[0]<<"-"<<ymd[1]<<"-" 
					<< gregorian_calendar::end_of_month_day(year, month) << "]";
			} catch (boost::bad_lexical_cast&) {
				rtnStream <<"["<<ymd[0]<<"-"<<ymd[1]<<"-01,"<<ymd[0]<<"-"<<ymd[1]<<"-31]";
			} catch (std::out_of_range&) {
				rtnStream <<"["<<ymd[0]<<"-"<<ymd[1]<<"-01,"<<ymd[0]<<"-"<<ymd[1]<<"-31]";
			}
		} else if(ymd_size == 3){
			rtnStream <<"["<<ymd[0]<<"-"<<ymd[1]<<"-"<<ymd[2]<<","<<ymd[0]<<"-"<<ymd[1]<<"-"<<ymd[2]<<"]";
		} else{
			int year = -1, week = -1;
			if (TimexUtils::parseYYYYWW(norm_str, year, week)) {
				std::wstring weekSpan =DatePeriodToKBPSpecString(
						TimexUtils::ISOWeek(year, week));
				rtnStream << weekSpan;
			} else {
				std::wstring datestr = docTheory->getSentenceTheory(vm->getSentenceNumber())->getTokenSequence()->toString(vm->getStartToken(), vm->getEndToken());
				std::wcout<<"getKBPSpecString(): Can't handle: "<<norm_str<<" from: "<<datestr<<std::endl;
				rtnStream<<"["<<norm_str<<"]";
			}
		}
	}
	return rtnStream.str();
}
std::wstring  EIUtils::getKBPSpecStringHoldsWithin(const DocTheory* docTheory, const ValueMention* vm){
	using namespace boost::gregorian;
	const Value* val =  docTheory->getValueSet()->getValueByValueMention(vm->getUID());
	std::wstringstream rtnStream;
	if(val == 0){
		rtnStream <<"[,]";
	}
	else{
		Symbol timex_val =	val->getTimexVal();
		std::wstring norm_str = L"";
		if(timex_val == Symbol()){
			norm_str = vm->toString(docTheory->getSentenceTheory(vm->getSentenceNumber())->getTokenSequence());
		}
		else{
			norm_str = timex_val.to_string();
		}
		boost::wcmatch matchResult;
		
		if(timex_val == PAST_REF){
			std::wstring datestr = docTheory->getSentenceTheory(vm->getSentenceNumber())->getTokenSequence()->toString(vm->getStartToken(), vm->getEndToken());
			//std::wcout<<"getKBPSpecString(): PASTREF "<<norm_str<<" from: "<<datestr<<std::endl;
			rtnStream <<"[[,],[,"<<val->getTimexAnchorVal().to_string()<<"]]";
		} else if(timex_val == PRESENT_REF){
			std::wstring datestr = docTheory->getSentenceTheory(vm->getSentenceNumber())->getTokenSequence()->toString(vm->getStartToken(), vm->getEndToken());
			std::wcout<<"getKBPSpecString(): CURRENT_REF "<<norm_str<<" from: "<<datestr<<std::endl;
			rtnStream <<"[["<<val->getTimexAnchorVal().to_string()<<","<<
				val->getTimexAnchorVal().to_string()<<"],["<<
				val->getTimexAnchorVal().to_string()<<","
				<<val->getTimexAnchorVal().to_string()<<"]]";
		} else if(timex_val == FUTURE_REF){
			std::wstring datestr = docTheory->getSentenceTheory(vm->getSentenceNumber())->getTokenSequence()->toString(vm->getStartToken(), vm->getEndToken());
			std::wcout<<"getKBPSpecString(): FUTURE_REF "<<norm_str<<" from: "<<datestr<<std::endl;
			rtnStream <<"[["<<val->getTimexAnchorVal().to_string()<<",],[,]]";
		} else{
			std::vector<std::wstring> ymd = TimexUtils::parseYYYYMMDD(norm_str);
			int ymd_size = 0;
			BOOST_FOREACH(wstring s, ymd){
				if(s != L"")
					ymd_size+=1;
			}
			if(ymd_size == 1){//YYYY
				rtnStream <<"[["<<ymd[0]<<"-01-01,"<<ymd[0]<<"-01-01],["
					<<ymd[0]<<"-12-31,"<<ymd[0]<<"-12-31]]";
				//rtnStream <<ymd;
			} else if(ymd_size == 2){
				try {
					unsigned short year = boost::lexical_cast<unsigned short>(ymd[0]);
					unsigned short month = boost::lexical_cast<unsigned short>(ymd[1]);
					unsigned short last_day = gregorian_calendar::end_of_month_day(year, month);
					rtnStream <<"[["<<ymd[0]<<"-"<<ymd[1]<<"-01,"<<ymd[0]<<"-"<<ymd[1]<<"-01],["
						<<ymd[0]<<"-"<<ymd[1]<<"-" << last_day << ","<<ymd[0]<<"-"<<ymd[1]
						<< "-" << last_day << "]]";
				} catch (boost::bad_lexical_cast&) {
					rtnStream <<"[["<<ymd[0]<<"-"<<ymd[1]<<"-01,"<<ymd[0]<<"-"<<ymd[1]<<"-01],["
						<<ymd[0]<<"-"<<ymd[1]<<"-31,"<<ymd[0]<<"-"<<ymd[1]<<"-31]]";
				} catch (std::out_of_range&) {
					rtnStream <<"[["<<ymd[0]<<"-"<<ymd[1]<<"-01,"<<ymd[0]<<"-"<<ymd[1]<<"-01],["
						<<ymd[0]<<"-"<<ymd[1]<<"-31,"<<ymd[0]<<"-"<<ymd[1]<<"-31]]";
				}
			} else if(ymd_size == 3){
				rtnStream <<"[["<<ymd[0]<<"-"<<ymd[1]<<"-"<<ymd[2]<<","<<ymd[0]<<"-"<<ymd[1]<<"-"<<ymd[2]<<"], "
						   <<"["<<ymd[0]<<"-"<<ymd[1]<<"-"<<ymd[2]<<","<<ymd[0]<<"-"<<ymd[1]<<"-"<<ymd[2]<<"]]";
			} else{
				int year = -1, week = -1;
				if (TimexUtils::parseYYYYWW(norm_str, year, week)) {
					std::wstring weekSpan = DatePeriodToKBPSpecString(
							TimexUtils::ISOWeek(year, week));
					rtnStream << L"[" << weekSpan << L"," << weekSpan << L"]";
				} else {
	
					std::wstring datestr = docTheory->getSentenceTheory(vm->getSentenceNumber())->getTokenSequence()->toString(vm->getStartToken(), vm->getEndToken());
					std::wstring origDocID = docTheory->getDocument()->getName().to_string();
					std::wstring dociddate = origDocID.substr(8,8).c_str();
					std::wstring YYYY = dociddate.substr(0,4);
					std::wstring MM = dociddate.substr(4,2);
					std::wstring DD = dociddate.substr(6,2);
					rtnStream<<"[[,],[,]]";
					std::wcout<<"getKBPSpecStringHoldsWithin(): Can't handle: "<<norm_str<<" from: "<<datestr<<" So use: "<<rtnStream.str()<<std::endl;
				}
			}
		}
	}
	return rtnStream.str();
}

std::wstring EIUtils::getKBPSpecString(const DocTheory* docTheory, const ValueMention* vm1, const ValueMention* vm2){
	std::wstringstream rtnStream;
	rtnStream<< "["
		<< getKBPSpecString(docTheory, vm1).c_str()<<","
		<< getKBPSpecString(docTheory, vm2).c_str()<<"]" ;
	return rtnStream.str();

}

boost::gregorian::date_period EIUtils::DatePeriodFromKBPSpecString(const std::wstring kbpSpec) {
	using namespace boost::gregorian;
	static const date_period NULL_DATE(date(2000,Jan,2),date(2000,Jan,1));

	if (kbpSpec.size() < 2) {
		return NULL_DATE;
	}

	if (kbpSpec[0] != L'[' || kbpSpec[kbpSpec.length()-1] != L']') {
		return NULL_DATE;
	}

	size_t idx = kbpSpec.find(L',');
	if (idx == std::wstring::npos) {
		return NULL_DATE;
	}

	std::wstring startDateString = kbpSpec.substr(1,idx - 1);
	std::wstring endDateString = 
		kbpSpec.substr(idx+1, kbpSpec.length() - idx - 2);

	date startDate = startDateString.empty()?date(neg_infin):TimexUtils::dateFromTimex(startDateString);
	date endDate = endDateString.empty()?date(pos_infin):TimexUtils::dateFromTimex(endDateString);

	if (startDate != date(not_a_date_time)
			&& endDate != date(not_a_date_time))
	{
		return date_period(startDate, endDate);
	}

	return NULL_DATE;
}

boost::gregorian::date_period EIUtils::DatePeriodFromValue(const std::wstring& val) {
	using namespace boost::gregorian;
	date_period dp = TimexUtils::datePeriodFromTimex(val);
	if (!dp.is_null()) {
		return dp;
	} else {
		dp = DatePeriodFromKBPSpecString(val);
		// note that if it fails to parse as a spec string
		// we return a null date_period
		return dp;
	}
}

EIUtils::DatePeriodPair EIUtils::DatePeriodPairFromKBPSpecString(const std::wstring& s) {
	using namespace boost::gregorian;
	static const date_period NULL_DATE_PERIOD(date(2000,Jan,2),date(2000,Jan,1));
	DatePeriodPair ret = make_pair(NULL_DATE_PERIOD, NULL_DATE_PERIOD);

	if (s.size() < 2) {
		return ret;
	}

	if (s[0] != L'[' || s[s.size()-1] != L']') {
		return ret;
	}

	size_t idx = s.find(L',');

	if (idx != std::wstring::npos) {
		idx = s.find(L',', idx+1);
		if (idx != std::wstring::npos) {
			std::wstring firstDatePeriod = s.substr(1, idx-1);
			std::wstring secondDatePeriod = s.substr(idx+1, s.length() - idx - 2);
			date_period dp1 = EIUtils::DatePeriodFromKBPSpecString(firstDatePeriod);
			date_period dp2 = EIUtils::DatePeriodFromKBPSpecString(secondDatePeriod);

			if (!dp1.is_null() && !dp2.is_null()) {
				return make_pair(dp1, dp2);
			}
		}
	}
	return ret;
}

std::wstring EIUtils::DatePeriodPairToKBPSpecString(const DatePeriodPair& dpp) {
	std::wstringstream ret;

	ret << L"[" << DatePeriodToKBPSpecString(dpp.first) << L"," 
		<< DatePeriodToKBPSpecString(dpp.second) << L"]";
	return ret.str();
}

std::wstring EIUtils::DatePeriodToKBPSpecString(const boost::gregorian::date_period& dp) {
	using namespace boost::gregorian;
	std::wstringstream ret;

	wstring first = L"";
	wstring second = L"";

	if (dp.begin().is_neg_infinity()) {
		// this is fine, leave it blank
	} else if (dp.begin().is_pos_infinity()) {
		SessionLogger::err("bad_kbp_spec") << "EIUtils::DatePeriodToKBPSpecString "
			<< " saw positive infinity on the left side of an interval";
		// treat it as if it were negative infinity
	} else {
		first = TimexUtils::dateToTimex(dp.begin());
	}

	if (dp.end().is_pos_infinity()) {
		// this is fine, leave it blank
	} else if (dp.end().is_neg_infinity()) {
		SessionLogger::err("bad_kbp_spec") << "EIUtils::DatePeriodToKBPSpecString "
			<< " saw negative infinity on the right side of an interval";
		// treat it as if it were negative infinity
	} else {
		second = TimexUtils::dateToTimex(dp.end());
	}

	ret << L"[" << first << L"," << second << L"]";
	return ret.str();
}

/////////////////////
//                 //
// MERGE FUNCTIONS //
//                 //
/////////////////////

bool EIUtils::isRecentDate(ElfRelationArg_ptr arg) {

	std::wstring date_text = arg->get_text();
	boost::to_lower(date_text);
	if (date_text.find(L"monday") != std::wstring::npos ||
		date_text.find(L"tuesday") != std::wstring::npos ||
		date_text.find(L"wednesday") != std::wstring::npos ||
		date_text.find(L"thursday") != std::wstring::npos ||
		date_text.find(L"friday") != std::wstring::npos ||
		date_text.find(L"saturday") != std::wstring::npos ||
		date_text.find(L"sunday") != std::wstring::npos ||
		date_text.find(L"week") != std::wstring::npos ||
		date_text.find(L"today") != std::wstring::npos ||
		date_text.find(L"yesterday") != std::wstring::npos ||
		date_text.find(L"tomorrow") != std::wstring::npos)
		return true;
	return false;
}

bool EIUtils::isContradictoryRelation(EIDocData_ptr docData, ElfRelation_ptr rel1, ElfRelation_ptr rel2) {

	// This function is quite inefficient, but it will just have to do for now.

	// If they have contradictory dates, that's a contradictory relation situation
	std::vector<ElfRelationArg_ptr> dateArgs1 = rel1->get_args_with_role(L"eru:eventDate");
	std::vector<ElfRelationArg_ptr> dateArgs2 = rel2->get_args_with_role(L"eru:eventDate");
	BOOST_FOREACH(ElfRelationArg_ptr arg1, dateArgs1) {
		BOOST_FOREACH(ElfRelationArg_ptr arg2, dateArgs2) {
			if (isContradictoryDate(arg1, arg2)) {
				std::ostringstream ostr;
				ostr << "Contradictory date: arg1:\n";
				arg1->dump(ostr, /* indent = */ 2);
				ostr << "arg2:\n";
				arg2->dump(ostr, /* indent = */ 2);
				SessionLogger::info("LEARNIT") << ostr.str();
				return true;
			}
		}
	}
	
	// If they have contradictory locations and no overlap, that's a contradictory relation situation
	bool has_contradictory = false;
	bool has_overlap = false;
	BOOST_FOREACH(ElfRelationArg_ptr arg1, rel1->get_args()) {
		if (arg1->get_role() == L"eru:eventLocation" || arg1->get_role() == L"eru:eventLocationGPE") {
			BOOST_FOREACH(ElfRelationArg_ptr arg2, rel2->get_args()) {
				if (arg2->get_role() == L"eru:eventLocation" || arg2->get_role() == L"eru:eventLocationGPE") {
					if (arg1->get_individual() != ElfIndividual_ptr() && 
						arg2->get_individual() != ElfIndividual_ptr() && 
						arg1->get_individual()->get_best_uri() == arg2->get_individual()->get_best_uri())
						has_overlap = true;
					else if (isContradictoryLocation(docData, arg1, arg2))
						has_contradictory = true;
				}
			}
		}
	}
	if (!has_overlap && has_contradictory)
		return true;

	std::set<ElfRelationArg_ptr> patientArgs1 = getPatientArguments(rel1);
	std::set<ElfRelationArg_ptr> patientArgs2 = getPatientArguments(rel2);
	std::vector<ElfRelationArg_ptr> agentArgs1 = rel1->get_args_with_role(
		EITbdAdapter::getAgentRole(EIUtils::getTBDEventType(rel1)));
	std::vector<ElfRelationArg_ptr> agentArgs2 = rel2->get_args_with_role(
		EITbdAdapter::getAgentRole(EIUtils::getTBDEventType(rel2)));

	// If the patient of one is the agent of another, that's a contradiction
	BOOST_FOREACH(ElfRelationArg_ptr patient, patientArgs1) {
		BOOST_FOREACH(ElfRelationArg_ptr agent, agentArgs2) {
			if (patient->get_individual() != ElfIndividual_ptr() && agent->get_individual() != ElfIndividual_ptr() &&
				patient->get_individual()->get_best_uri() == agent->get_individual()->get_best_uri())
				return true;
		}
	}	
	BOOST_FOREACH(ElfRelationArg_ptr patient, patientArgs2) {
		BOOST_FOREACH(ElfRelationArg_ptr agent, agentArgs1) {
			if (patient->get_individual() != ElfIndividual_ptr() && agent->get_individual() != ElfIndividual_ptr() &&
				patient->get_individual()->get_best_uri() == agent->get_individual()->get_best_uri())
				return true;
		}
	}

	return false;
}

bool EIUtils::isContradictoryLocation(EIDocData_ptr docData, ElfRelationArg_ptr arg1, ElfRelationArg_ptr arg2) {

	if (!arg1->get_individual() || !arg1->get_individual()->has_entity_id() ||
		!arg2->get_individual() || !arg2->get_individual()->has_entity_id())
		return false;

	Entity *entity1 = docData->getEntity(arg1->get_individual()->get_entity_id());
	Entity *entity2 = docData->getEntity(arg2->get_individual()->get_entity_id());
	if (entity1 == 0 || entity2 == 0)
		return false;

	if (entity1 == entity2)
		return false;

	// Only do this for entities we know something about
	if (!docData->getDocumentPlaceInfo(entity1->getID()).isInLookupTable() ||
		!docData->getDocumentPlaceInfo(entity2->getID()).isInLookupTable())
	{
		return false;
	}

	std::set<int> superLocations1 = getSuperLocations(docData, entity1->getID());
	std::set<int> superLocations2 = getSuperLocations(docData, entity2->getID());	

	bool found_hierarchy = (superLocations1.find(entity2->getID()) != superLocations1.end()) || (superLocations2.find(entity1->getID()) != superLocations2.end());
	if (!found_hierarchy) {
		//const Mention *ment1 = findMentionForRelationArg(arg1);
		//const Mention *ment2 = findMentionForRelationArg(arg2);
		//SessionLogger::info("LEARNIT") << "\nContradictory locations: " << ment1->getNode()->toDebugTextString() << " ~ " << ment2->getNode()->toDebugTextString() << "\n";
		return true;
	}
	return false;
}


bool EIUtils::isContradictoryDate(ElfRelationArg_ptr arg1, ElfRelationArg_ptr arg2) {

	if (arg1->get_type()->get_value().compare(L"xsd:date") != 0 ||
		arg2->get_type()->get_value().compare(L"xsd:date") != 0)
		return false;

	std::wstring date1 = arg1->get_individual()->get_value();
	std::wstring date2 = arg2->get_individual()->get_value();

	if (date1.size() < 4 || date2.size() < 4)
		return false;

	if (date1.substr(0,4).compare(date2.substr(0,4)) != 0)
		return true;

	if (date1.size() < 7 || date2.size() < 7)
		return false;

	if (date1.at(5) == L'W' || date2.at(5) != L'W') {
		// not sure what to do here, let's just say they are non-contradictory
		return false;
	}

	if (date1.substr(5,2).compare(date2.substr(5,2)) != 0)
		return true;

	if (date1.size() < 10 || date2.size() < 10)
		return false;
	
	if (date1.substr(7,2).compare(date2.substr(7,2)) != 0)
		return true;

	return false;

}

std::set<ElfRelationArg_ptr> EIUtils::getPatientArguments(ElfRelation_ptr rel) {
	std::set<ElfRelationArg_ptr> returnSet;
	std::wstring event_type = EIUtils::getTBDEventType(rel);
	std::set<std::wstring> patient_roles;
	try {
		patient_roles.insert(EITbdAdapter::getPatientRole(event_type, EntityType::getPERType(), EntitySubtype(L"PER.Individual")));
	} catch ( ... ) {}
	try {
		patient_roles.insert(EITbdAdapter::getPatientRole(event_type, EntityType::getPERType(), EntitySubtype(L"PER.Group")));
	} catch ( ... ) {}
	patient_roles.insert(EITbdAdapter::getPatientRole(event_type, EntityType::getORGType(), EntitySubtype::getUndetType()));
	BOOST_FOREACH(ElfRelationArg_ptr arg, rel->get_args()) {
		if (patient_roles.find(arg->get_role()) != patient_roles.end())
			returnSet.insert(arg);
	}
	return returnSet;
}

const Mention *EIUtils::findEmployerForPersonEntity(EIDocData_ptr docData, const Entity *person) {
	const Mention *orgMention = 0;
	bool clash = false; // don't allow if the person is employed by more than one org
	for (int sent = 0; sent < docData->getNSentences(); sent++) {
		const RelMentionSet *rms = docData->getSentenceTheory(sent)->getRelMentionSet();
		for (int r = 0; r < rms->getNRelMentions(); r++) {
			if (rms->getRelMention(r)->getType() != Symbol(L"ORG-AFF.Employment"))
				continue;
			const Entity *leftEnt = docData->getEntityByMention(rms->getRelMention(r)->getLeftMention()->getUID());
			if (leftEnt == person) {
				if (orgMention != 0) {
					if (docData->getEntityByMention(orgMention->getUID()) !=
						docData->getEntityByMention(rms->getRelMention(r)->getRightMention()->getUID()))
					{
						clash = true;
					}
				} else {
					orgMention = rms->getRelMention(r)->getRightMention();
				}						
			}
		}
	}
	if (!clash)
		return orgMention;
	return 0;
}

const Entity* EIUtils::findEntityForRelationArg(EIDocData_ptr docData, const ElfRelationArg_ptr& arg) {
	if (arg.get() != NULL) {
		ElfIndividual_ptr indiv = arg->get_individual();

		if (indiv.get() != NULL && indiv->has_entity_id()) {
			return docData->getEntity(indiv->get_entity_id());
		}
	}

	return NULL;
}

ElfRelation_ptr EIUtils::copyReplacingWithBoundIndividual(
	const ElfRelation_ptr& old_relation, const ElfIndividual_ptr& indiv_to_replace, 
	const wstring& boundID)
{
	std::vector<ElfRelationArg_ptr> arg_copy;
	if (indiv_to_replace) {
		bool replaced = false;
		std::vector<ElfRelationArg_ptr> args = old_relation->get_args();
		BOOST_FOREACH(const ElfRelationArg_ptr& oth_arg, args) {
			ElfIndividual_ptr oth_indiv = oth_arg->get_individual();
			if (oth_indiv && oth_indiv->get_best_uri() == indiv_to_replace->get_best_uri()) {
				ElfRelationArg_ptr new_argument = boost::make_shared<ElfRelationArg>(oth_arg);
				ElfIndividual_ptr indiv_copy = boost::make_shared<ElfIndividual>(oth_indiv);
				indiv_copy->set_bound_uri(boundID);
				new_argument->set_individual(indiv_copy);
				arg_copy.push_back(new_argument);
				replaced = true;
			} else {
				arg_copy.push_back(boost::make_shared<ElfRelationArg>(oth_arg));
			} 
		}
		if (replaced) {
			ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(
				old_relation->get_name(), arg_copy, old_relation->get_text(), 
				old_relation->get_start(), old_relation->get_end(), old_relation->get_confidence(), old_relation->get_score_group());
			new_relation->set_source(old_relation->get_source());
			new_relation->add_source(L"eru:copyReplacingWithBoundIndividual()");
			return new_relation;
		}
	}
	return ElfRelation_ptr();
}

const Mention* EIUtils::getContinent(EIDocData_ptr docData, const Entity* e1){
	if(!e1->getType().matchesGPE())
		return 0;
	const EntitySet* entities = docData->getEntitySet();
	for(int i =0; i< e1->getNMentions(); i++){
		const Mention* m = entities->getMention(e1->getMention(i));
		if(m->getMentionType() != Mention::NAME)
			continue;
		if(m->getHead() == 0)
			continue;
		const SynNode* head = m->getAtomicHead();
		if(!head)
			continue;
		std::wstring head_string = UnicodeUtil::normalizeTextString(head->toTextString());
		if(_continentExpansions.find(head_string) != _continentExpansions.end()){
			return m;
		}
	}
	return 0;
}

/*

void ElfInference::inferAttackInformationFromDocument(EIDocData_ptr docData) {


	// NOT FINISHED, NOT USED FOR EVAL. INCLUDED IN CASE WE WANT TO COME BACK TO THIS

	std::vector<std::vector<ElfRelation_ptr> > sentenceRelations(docData->getNSentences());
	BOOST_FOREACH(ElfRelation_ptr relation, docData->get_relations()) {
		std::wstring event_type = EIUtils::getTBDEventType(relation);
		if (event_type.size() == 0)
			continue;
		if (event_type != L"killing" && event_type != L"injury")
			continue;

		int sentence_number = -1;
		BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
			const Mention *ment = findMentionForRelationArg(arg);
			if (ment != 0) {
				sentence_number = ment->getSentenceNumber();
				break;
			}
		}
		if (sentence_number != -1)
			sentenceRelations[sentence_number].push_back(relation);
	}

	// VERY IMPORTANT: ITERATE BACKWARDS THROUGH DOCUMENT!!!

	// DATES
	for (int sent = docData->getNSentences() - 1; sent > 0; sent--) {

		if (sentenceRelations.at(sent).size() == 0)
			continue;

		// If any date is mentioned in this sentence, don't mess with it!
		if (getDatesFromSentence(sent).size() > 0)
			continue;

		// Look at the previous N sentences and grab dates
		int date_range = 1;
		for (int i = 1; i <= date_range; i++) {
			if (sent - i == 0)
				break;
			int dates_in_sentence = getDatesFromSentence(sent-i).size();
			if (dates_in_sentence > 1)
				break;
			BOOST_FOREACH(ElfRelation_ptr previous_relation, sentenceRelations.at(sent-i)) {
				BOOST_FOREACH(ElfRelationArg_ptr arg, previous_relation->get_args_with_role(L"eru:eventDate")) {
					if (!isRecentDate(arg))
						continue;
                    std::ostringstream ostr;
					ostr << "\nAdded cross-sentence date: ";
					for (int debug = sent-i; debug <= sent; debug++) {
						ostr << docData->getSentenceTheory(debug)->getPrimaryParse()->getRoot()->toDebugTextString();
						ostr << " ";
					}
					SessionLogger::info("LEARNIT") << ostr.str();
					BOOST_FOREACH(ElfRelation_ptr relation, sentenceRelations.at(sent)) {
						relation->insert_argument(arg);
					}
				}
			}
			if (dates_in_sentence > 0)
				break;
		}
	}

	// LOCATIONS

	const Mention *nationState = identifyFocusNationState();
	if (nationState != 0) {
	
		for (int sent = docData->getNSentences() - 1; sent > 0; sent--) {

			if (sentenceRelations.at(sent).size() == 0)
				continue;

			// If any event already has a location, skip this sentence
			bool has_event_location = false;
			BOOST_FOREACH(ElfRelation_ptr relation, sentenceRelations.at(sent)) {
				if (relation->get_args_with_role(L"eru:eventLocation").size() > 0 ||
					relation->get_args_with_role(L"eru:eventLocationGPE").size() > 0)
				{
					has_event_location = true;
					break;
				}
			}
			if (has_event_location)
				continue;

			// Look at the previous N sentences and grab dates
			int loc_range = 1;
			for (int i = 1; i <= loc_range; i++) {
				if (sent - i == 0)
					break;
				std::set<ElfRelationArg_ptr> locations;
				BOOST_FOREACH(ElfRelation_ptr previous_relation, sentenceRelations.at(sent-i)) {
					BOOST_FOREACH(ElfRelationArg_ptr arg, previous_relation->get_args_with_role(L"eru:eventLocation")) {
						locations.insert(arg);						
					}
					BOOST_FOREACH(ElfRelationArg_ptr arg, previous_relation->get_args_with_role(L"eru:eventLocationGPE")) {
						locations.insert(arg);						
					}
				}
				if (locations.size() == 0)
					continue;
				bool is_contradictory = false;
				BOOST_FOREACH(ElfRelationArg_ptr arg1, locations) {
					BOOST_FOREACH(ElfRelationArg_ptr arg2, locations) {
						if (arg1 != arg2 && isContradictoryLocation(arg1, arg2))
							is_contradictory = true;
					}
				}
				if (!is_contradictory) {
                    std::ostringstream ostr;
					ostr << "\nAdded cross-sentence location: ";
					for (int debug = sent-i; debug <= sent; debug++) {
						ostr << docData->getSentenceTheory(debug)->getPrimaryParse()->getRoot()->toDebugTextString();
						ostr << " ";
					}
					SessionLogger::info("LEARNIT") << ostr.str();
					BOOST_FOREACH(ElfRelationArg_ptr arg, locations) {
						BOOST_FOREACH(ElfRelation_ptr relation, sentenceRelations.at(sent)) {
							relation->insert_argument(arg);
						}
					}
				}
				if (locations.size() > 0)
					break;
			}
		}
	}

}
*/

std::set<std::wstring> EIUtils::getCountrySynonymsForIndividual(EIDocData_ptr docData,
																	 const ElfIndividual_ptr& indiv) {
	set<std::wstring> synonyms;
	if (indiv) {
		set<wstring> toLookup;
		const wstring bestname = indiv->get_name_or_desc()->get_string();
		if (!bestname.empty()) {
			toLookup.insert(bestname);
		}
		if (indiv->has_entity_id()) {
			const Entity *entity = docData->getEntity(indiv->get_entity_id());
			if (entity && entity->getType().matchesGPE()) {
				for (int i=0; i<entity->getNMentions(); ++i) {
					const Mention* ment = docData->getEntitySet()->getMention(entity->getMention(i));
					if (ment->getMentionType() == Mention::NAME) {
						const SynNode* node = ment->getAtomicHead();
						std::wstring name = node->toTextString();
						if (!name.empty()) {
							toLookup.insert(name);
						}
					}
				}
			} else {
				if (entity) {
					//SessionLogger::info("LEARNIT") << L"not GPE" << endl;
					for (int i=0; i<entity->getNMentions(); ++i) {
						const Mention* ment = docData->getEntitySet()->getMention(entity->getMention(i));
						if (ment->getMentionType() == Mention::NAME) {
							const SynNode* node = ment->getAtomicHead();
							std::wstring name = node->toTextString();
							//SessionLogger::info("LEARNIT") << "\t" << name << endl;
						}
					}	
				}
			}
		}

		BOOST_FOREACH(const std::wstring& name, toLookup) {
			std::wstring mutable_name = name;
			mutable_name = EIUtils::normalizeCountryName(mutable_name);
			std::set<std::wstring> equivCountryStrings = EIUtils::getEquivalentNames(name);
			std::set<std::wstring> transformedEquiv;
			BOOST_FOREACH(const std::wstring s, equivCountryStrings) {
				std::wstring mut_s = s;
				mut_s = EIUtils::normalizeCountryName(mut_s);
				transformedEquiv.insert(mut_s);
			}
			synonyms.insert(transformedEquiv.begin(), transformedEquiv.end());
		}
	}
	return synonyms;
}

void EIUtils::createPlaceNameIndex(EIDocData_ptr docData){
	//get the set of all place names in the document
	//const DocTheory* docTheory = docData->getDocTheory();
	const EntitySet* entity_set = docData->getEntitySet();
	for(int i = 0; i < entity_set->getNEntities(); i++){	
		const Entity* ent = entity_set->getEntity(i);
		if(ent->getType().matchesGPE() || ent->getType().matchesLOC()){
			for(int j =0; j < entity_set->getEntity(i)->getNMentions(); j++){
				const Mention* ment = entity_set->getMention(ent->getMention(j));
				if(ment->getMentionType() == Mention::NAME){
					std::wstring name_string = UnicodeUtil::normalizeTextString(ment->getHead()->toTextString());
					docData->getPlaceNameToEntity()[name_string].insert(i);
				}
			}
		}
	}
}

/**
 * Call ElfMultiDoc::merge_entities_in_map() on the named entity for the given mention (which we know refers to a person) in the given sentence,
 * given that the following criteria are met:
 * - entity is described in an event mention in this sentence as a victim
 * - entity is named
 * - sentence has only one name mention
 **/
void EIUtils::attemptNewswireLeadFix(const DocTheory *docTheory, int sent_no, const Mention *potentialDescMention) {
	if (sent_no == docTheory->getNSentences() - 1)
		return;

	const EntitySet* ents = docTheory->getEntitySet();
	SentenceTheory *st = docTheory->getSentenceTheory(sent_no);
	EventMentionSet *ems = st->getEventMentionSet();
	bool is_victim = false;
	for (int e = 0; e < ems->getNEventMentions(); e++) {
		const EventMention *em = ems->getEventMention(e);
		for (int a = 0; a < em->getNArgs(); a++) {
			if (em->getNthArgRole(a) == Symbol(L"Victim") &&
				em->getNthArgMention(a) == potentialDescMention)
			{
				is_victim = true;
				break;
			}
		}
	}
	if (!is_victim)
		return;

	// don't act on "descriptor" mentions with names
	Entity *potentialDescEnt = ents->getEntityByMention(potentialDescMention->getUID());	
	for (int m = 0; m < potentialDescEnt->getNMentions(); m++) {
		if (ents->getMention(potentialDescEnt->getMention(m))->getMentionType() == Mention::NAME) {
			return;
		}
	}

	// don't act on sentences with more than one name
	SentenceTheory *nextSentence = docTheory->getSentenceTheory(sent_no + 1);
	const MentionSet *nextMS = nextSentence->getMentionSet();
	const Mention *nameMention = 0;
	for (int m = 0; m < nextMS->getNMentions(); m++) {
		if (nextMS->getMention(m)->getMentionType() == Mention::NAME)
		{
			if (nextMS->getMention(m)->getEntityType() == potentialDescMention->getEntityType()) {
				if (nameMention != 0)
					return;
				else nameMention = nextMS->getMention(m);
			}
		}
	}
	if (nameMention == 0)
		return;

	Entity *nameEnt = ents->getEntityByMention(nameMention->getUID());
	if (nameEnt == 0)
		return;

	std::set<int> descIDSet;
	descIDSet.insert(potentialDescEnt->getID());
	ElfMultiDoc::merge_entities_in_map(docTheory, nameEnt->getID(), descIDSet);
	/*SessionLogger::info("LEARNIT") << "\nLinkage " << potentialDescMention->getEntityType().getName() << ": ";
	SessionLogger::info("LEARNIT") << "Sentence #1: " 
		<< docTheory->getSentenceTheory(sent_no)->getPrimaryParse()->getRoot()->toDebugTextString() << "\n";
	SessionLogger::info("LEARNIT") << "Sentence #2: " 
		<< docTheory->getSentenceTheory(sent_no+1)->getPrimaryParse()->getRoot()->toDebugTextString() << "\n";
	SessionLogger::info("LEARNIT") << "Mention #1: " << potentialDescMention->getNode()->toDebugTextString() << "\n";
	SessionLogger::info("LEARNIT") << "Mention #2: " << nameMention->getNode()->toDebugTextString() << "\n\n";*/
}

bool EIUtils::isLikelyNewswireLeadDescriptor(const Mention *ment) {
	if (ment->getMentionType() != Mention::DESC)
		return false;
	std::wstring str = ment->getNode()->toTextString();
	if (str.find(L"a ") == 0 || str.find(L"an ") == 0)
		return true;	
	return false;
}

bool EIUtils::isLikelyDateline(const SentenceTheory *st) {

	if (st->getSentNumber() > 4)
		return false;

	if (st->getTokenSequence()->getNTokens() < 5)
		return true;

	Symbol lastToken = st->getTokenSequence()->getToken(st->getTokenSequence()->getNTokens() - 1)->getSymbol();
	if (lastToken != Symbol(L"."))
		return true;

	return false;
}


std::vector<ElfRelationArg_ptr> EIUtils::getUnconfidentArguments(EIDocData_ptr docData, 
																	  const std::vector<ElfRelationArg_ptr> & args, 
																	  const std::wstring & uriPrefix){
	std::vector<ElfRelationArg_ptr> unconfidentArgs;
	BOOST_FOREACH(ElfRelationArg_ptr arg, args){
		const Mention* arg_mention = findMentionForRelationArg(docData, arg);
		if(arg_mention != 0){
			if(arg_mention->getMentionType() == Mention::PRON)
				unconfidentArgs.push_back(arg);
		}
	}
	return unconfidentArgs;
}

int EIUtils::getSentenceNumberForArg(const DocTheory* docTheory, ElfRelationArg_ptr arg){
	EDTOffset arg_start = arg->get_start();	
	for(int i = 0; i < docTheory->getNSentences(); i++){
		const TokenSequence* ts = docTheory->getSentenceTheory(i)->getTokenSequence();
		if(ts->getNTokens() > 0 && arg_start >= ts->getToken(0)->getStartEDTOffset() && arg_start <= ts->getToken(ts->getNTokens()-1)->getEndEDTOffset())
			return i;
	}
	return -1;
}

int EIUtils::getSentenceNumberForIndividualStart(const DocTheory* docTheory, ElfIndividual_ptr indiv) {
	EDTOffset start, end;
	indiv->get_spanning_offsets(start, end);
	for (int i=0; i< docTheory->getNSentences(); i++) {
		const TokenSequence* ts = docTheory->getSentenceTheory(i)->getTokenSequence();
		if (start >= ts->getToken(0)->getStartEDTOffset() 
				&& start <= ts->getToken(ts->getNTokens()-1)->getEndEDTOffset()) 
		{
			return i;
		}
	}
	throw UnexpectedInputException("EIUtils::getSentenceNumberForIndividual",
			"No sentence found in document which could contain the individual");
}

/** 
 * This function attempts to determine whether a Nation-State is the predominant ("focus") location for a given article.
 *
 * Approach: Find the most common Nation in the document. Consider it the focus location IF
 *   -- It occurs in the first N sentences of the document
 *   -- No other Nation is mentioned in the first N sentences of the document.
 *
 * The most reasonable value for N appears to be 3, except when those first three sentences are very short,
 *   meaning that they are probably a deeply split-up dateline.
 *
 */
const Mention * EIUtils::identifyFocusNationState(EIDocData_ptr docData, bool ic) {

	const DocTheory *docTheory = docData->getDocTheory();

	// You MUST check this; the constructor will throw if Nation is not a valid subtype
	EntitySubtype nationSubtype;
	try {
		nationSubtype = EntitySubtype(Symbol(L"GPE.Nation"));
	} catch ( ... ) {
		return 0;
	}
	
	// Find the most common entity in the document
	const Entity* most_common_entity = 0;
	ElfIndividual_ptr most_common_individual = ElfIndividual_ptr();
	int max_mentions = 0;
	ElfIndividualSet nationaStateIndividuals = ic?
		docData->get_individuals_by_type(L"ic:NationState")
		:docData->get_individuals_by_type();
	BOOST_FOREACH(ElfIndividual_ptr individual, nationaStateIndividuals) {
		if (!individual->has_entity_id())
			continue;
		Entity *ent = docData->getEntity(individual->get_entity_id());
		if (ent == 0 || ent->guessEntitySubtype(docTheory) != nationSubtype)
			continue;

		if (ent->getNMentions() > max_mentions) {
			most_common_entity = ent;
			most_common_individual = individual;
			max_mentions = ent->getNMentions();
		}
	}

	if (most_common_entity == 0)
		return 0;

	// Look at the first few sentences of the document to find the first name mention of 
	//   our most_common_entity. Keep track of all nations seen.
	std::set<Entity *> earlyNations;
	const Mention *earlyMCEMention = 0;
	int tokens_so_far = 0;
	for (int sentno = 0; sentno < docTheory->getNSentences(); sentno++) {
		MentionSet *ms = docTheory->getSentenceTheory(sentno)->getMentionSet();
		for (int m = 0; m < ms->getNMentions(); m++) {
			const Mention *ment = ms->getMention(m);
			Entity *ent = docTheory->getEntitySet()->getEntityByMention(ment->getUID());
			if (ent != 0 && ent->guessEntitySubtype(docTheory) == nationSubtype)
				earlyNations.insert(ent);
			if (earlyMCEMention == 0 && ent == most_common_entity && ment->getMentionType() == Mention::NAME)
				earlyMCEMention = ment;
		}
		tokens_so_far += docTheory->getSentenceTheory(sentno)->getTokenSequence()->getNTokens();

		// We want to look at the first few sentences, until we either hit 50 tokens, 4 sentences,
		//   or 3 sentences with 20 tokens seen. This last condition allows for us to see a fourth
		//   sentence iff the first few sentences are very short, e.g. a heavily split dateline.
		if (tokens_so_far > 50)
			break;
		if (sentno >= 4)
			break;
		if (sentno >= 3 && tokens_so_far >= 20)
			break;
	}

	// If we saw more than one nation in the first few sentences, return 0.
	// If we did not see a name mention of our most_common_entity in the first few sentences, return 0.
	if (earlyNations.size() > 1 || earlyMCEMention == 0)
		return 0;

	return earlyMCEMention;
}

// Adds arguments. Recursive function.
void EIUtils::getLocations(const DocTheory* docTheory, int sent_no, const Proposition *prop, 
								std::set<const Mention*>& locationMentions) 
{	
	MentionSet *mSet = docTheory->getSentenceTheory(sent_no)->getMentionSet();
	
	Symbol predSym = prop->getPredSymbol();
	if (is_opined_headword(predSym))
		return;

	for (int a = 0; a < prop->getNArgs(); a++) {
		Argument *arg = prop->getArg(a);

		// NOTE: Make sure you do not go down a <ref> tag or you will be here forever

		if (arg->getType() == Argument::MENTION_ARG) {
			const Mention *mention = arg->getMention(mSet);
			if (is_location_prep_headword(arg->getRoleSym()) ||
				arg->getRoleSym() == Argument::MEMBER_ROLE)
			{
				if (mention->getEntityType().matchesFAC() || 
					mention->getEntityType().matchesLOC() || 
					mention->getEntityType().matchesGPE())
						locationMentions.insert(mention);
			}
			PropositionSet *pSet = docTheory->getSentenceTheory(sent_no)->getPropositionSet();
			const Proposition *defProp = pSet->getDefinition(mention->getIndex());
			if (defProp != 0 && defProp != prop)
				getLocations(docTheory, sent_no, defProp, locationMentions);
		}
	}
}

std::vector<ElfRelation_ptr> EIUtils::inferLocationFromSubSuperLocation(EIDocData_ptr docData,
																		const ElfRelation_ptr elf_relation){
	const DocTheory* docTheory = docData->getDocTheory();
	std::vector<ElfRelation_ptr> inferred_relations;
	std::vector<ElfRelationArg_ptr> args = elf_relation->get_args();
	EDTOffset start = elf_relation->get_start();
	EDTOffset end = elf_relation->get_end();
	int start_sentence_number = -1;
	int end_sentence_number = -1;
	for(int i = 0; i< docTheory->getNSentences(); i++){
		const TokenSequence* toks = docTheory->getSentenceTheory(i)->getTokenSequence();
		if(start >= toks->getToken(0)->getStartEDTOffset() && start <= toks->getToken(toks->getNTokens()-1)->getEndEDTOffset())
			start_sentence_number = i;
		if(end >= toks->getToken(0)->getStartEDTOffset() && end <= toks->getToken(toks->getNTokens()-1)->getEndEDTOffset())
			end_sentence_number = i;
	}
	BOOST_FOREACH(ElfRelationArg_ptr arg, args){
		std::wstring role = arg->get_role();
		ElfIndividual_ptr individual = arg->get_individual();
		if ((role == L"eru:birthPlace" || role == L"eru:eventLocation" || 
			role == L"eru:eventLocationGPE" || role == L"eru:locationGPE") &&
			(individual.get() != NULL && individual->has_entity_id())) {
			//For each known super-GPE
			std::set<int> super_locations = EIUtils::getSuperLocations(docData, individual->get_entity_id());
			BOOST_FOREACH(int super_id, super_locations){
				const Entity* super_entity = docTheory->getEntitySet()->getEntity(super_id);
				//if there is a named mention of the super GPE in the same sentence as the relation, 
				// infer the relation also exists with the SUPER GPE
				for(int i = 0; i< super_entity->getNMentions(); i++){
					const Mention* ment = docTheory->getEntitySet()->getMention(super_entity->getMention(i));
					if(ment->getMentionType() == Mention::NAME &&
						ment->getSentenceNumber() >= start_sentence_number &&
						ment->getSentenceNumber() <= end_sentence_number)
					{
						std::pair<EDTOffset, EDTOffset> offsets = EIUtils::getSentenceEDTOffsetsForMention(docData, ment);

						ElfRelation_ptr new_relation = EIUtils::copyRelationAndSubstituteArgs(docData, elf_relation, 
							L"SuperLocationInference", arg, ment, 
							L"ic:GeopoliticalEntity", 
							min(offsets.first, elf_relation->get_start()), max(offsets.second, elf_relation->get_end()));
						inferred_relations.push_back(new_relation);
					}
				}
			}			
		}
	}
	return inferred_relations; 
}

// These are exceptions as in "exceptions to the rule", not thrown exceptions.
void EIUtils::initAllowedDoubleEntityRelations() {
	static bool init = false;
	if (!init) {
		std::string exception_file = 
			ParamReader::getParam("allowed_double_entity_relations");
		if (exception_file != string()) {
			boost::scoped_ptr<UTF8InputStream> exceptionStream_scoped_ptr(UTF8InputStream::build(exception_file.c_str()));
			UTF8InputStream& exceptionStream(*exceptionStream_scoped_ptr);
			std::wstring line;
			while (getline(exceptionStream, line)) {
				std::vector<std::wstring> parts;
				boost::split(parts, line, boost::is_any_of("\t "));
				if (parts.size() == 3) {
					_allowedDoubleEntityRelations.insert(
						boost::make_tuple(parts[0], parts[1], parts[2]));
					_allowedDoubleEntityRelations.insert(
						boost::make_tuple(parts[0], parts[2], parts[1]));
				} else if (line.length() > 0)  {
					SessionLogger::warn("LEARNIT") << "ignoring ill-formed line in " <<
						"double entity relations exceptions file : " <<
						std::endl << line << std::endl;
				}
			}
			exceptionStream.close();
		} else {
			SessionLogger::warn("no_doub_ent_0") << "using no exceptions to double entity filter\n";
		}
	}
}

bool EIUtils::isAllowedDoubleEntityRelation(ElfRelation_ptr relation,
							ElfRelationArg_ptr arg1, ElfRelationArg_ptr arg2)
{
	RelationRoleRoleSet::const_iterator probe =
		_allowedDoubleEntityRelations.find(boost::make_tuple(
			relation->get_name(), arg1->get_role(), arg2->get_role()));
	
	return probe!=_allowedDoubleEntityRelations.end(); 
}

bool EIUtils::isPossessorOfORGOrGPE(EIDocData_ptr docData, const Mention* m) {
	int sn = m->getSentenceNumber();
	const DocTheory* dt = docData->getDocTheory();
	const SentenceTheory* st = dt->getSentenceTheory(sn);
	const PropositionSet* ps = st->getPropositionSet();
	const MentionSet* ms = st->getMentionSet();
	const EntitySet* es = dt->getEntitySet();

	for (int i=0; i<ps->getNPropositions(); ++i) {
		const Proposition* prop = ps->getProposition(i);

		if (prop) {
			const Mention* refMention = prop->getMentionOfRole(Argument::REF_ROLE, ms);
			const Mention* posMention = prop->getMentionOfRole(Argument::POSS_ROLE, ms);

			if (refMention && posMention && posMention == m) {
				EntityType m_et = refMention->getEntityType();
				
				if (m_et.matchesGPE() || m_et.matchesORG()) {
					return true;
				}

				const Entity* refEntity = 
					es->getEntityByMentionWithoutType(refMention->getUID());
				if (refEntity) {
					EntityType e_et = refEntity->getType();
					return e_et.matchesGPE() || e_et.matchesORG();
				}

				break;
			}
		}
	}
	return false;
}

bool EIUtils::passesGPELeadershipFilter(EIDocData_ptr docData, const Mention* leaderMention, 
	const Mention* ledMention, const ElfIndividual_ptr& ledIndividual) 
{
	if(ledMention == NULL)
		return false;
	if(leaderMention == NULL)
		return false;
	if (ledIndividual && ledIndividual->has_entity_id()) {
		const Entity* ledEntity = docData->getEntity(ledIndividual->get_entity_id());
		if (ledEntity && ledEntity->getType().matchesGPE()) {
			// Example: 
			// [[Sri Lanka]'s [opposition] [leader] [Gamini Dissanayake]]
			// where all the brackets indicate mentions, and the only
			// coreffed mentions are "leader" and the whole thing.
			// "leader", however, is not the head of its parent synnode,
			// but rather "Gamini Dissanayake".
			// So we resort to checking for opposition leaders by
			// seeing if the parent synnode contains "opposition",
			// which isn't very satisfying... ~ RMG
			const SynNode* leaderNode = leaderMention->getNode();
			if (leaderNode) {
				const SynNode* parentNode = leaderNode->getParent();
				if (parentNode) {
					const wstring leaderText = leaderNode->toTextString();
					const wstring parentText = parentNode->toTextString();

					if (parentText.find(L"opposition") != wstring::npos) {
						return false;
					}

					// keep prime ministers, but exclude foreign ministers,
					// defense ministers, ministers of silly walks, etc.
					if (leaderText.find(L"minister") != std::wstring::npos &&
						parentText.find(L"prime") == std::wstring::npos) 
					{
						return false;
					}
				}
			}
		}
	}
	return true;
}


/** TODO: this may be somewhat too aggressive. **/
bool EIUtils::isPoliticalParty(EIDocData_ptr docData, const Entity* e){
	const EntitySet* es = docData->getDocTheory()->getEntitySet();
	for (int i=0; i<e->getNMentions(); ++i) {
		const Mention *ment = es->getMention(e->getMention(i));
		const SynNode* mentNode = ment->getAtomicHead();
		if (mentNode) {
			Symbol hd = mentNode->getHeadWord();
			// United Nations is not a political party
			if (hd == Symbol(L"un") || hd == Symbol(L"u.n.")) {
				return false;
			}
			const std::wstring mentText = mentNode->toTextString();
			if (mentText.find(L"party") != std::wstring::npos) {
				return true;
			}

			if (EIUtils::isPartyNameWord(mentNode->getHeadWord())) {
				return true;
			}
		}	
	}
	return false;
}
bool EIUtils::isPoliticalParty(EIDocData_ptr docData, const ElfRelationArg_ptr& arg) {
	const EntitySet* es = docData->getEntitySet();
	const Mention* m = EIUtils::findMentionForRelationArg(docData, arg);

	if (m) {
		const SynNode* mentNode = m->getNode();
		if (mentNode) {
			Symbol hd = mentNode->getHeadWord();
			// United Nations is not a political party
			if (hd == Symbol(L"un") || hd == Symbol(L"u.n.")) {
				return false;
			}
		}
	}

	// check by type
	if (arg->individual_has_type(L"ic:PoliticalParty")) {
		return true;
	}

	// check by word list
	const Entity* e = EIUtils::findEntityForRelationArg(docData, arg);

	if (e) {
		return isPoliticalParty(docData, e);
	}
	return false;
}

// this will only catch the first 
bool EIUtils::isNatBoundArg(EIDocData_ptr docData,
								 const ElfRelationArg_ptr& arg, std::set<std::wstring>& natBoundIDs) 
{
	bool ret = false;
	ElfIndividual_ptr indiv = arg->get_individual();

	if (indiv && indiv->has_entity_id()) {
		Entity *e = docData->getEntity(indiv->get_entity_id());

		if (e) {
			for (int i=0; i<e->getNMentions(); ++i) {
				const Mention* ment = docData->getEntitySet()->getMention(e->getMention(i));
				const SynNode* node = ment->getAtomicHead();
				std::wstring text = node->toTextString();
				boost::trim(text);
				
				BOOST_FOREACH(const EIUtils::NatTuple& natTuple, EIUtils::getNationalityPrefixedXDocFailures()) {
					if (text == natTuple.get<1>()) {
						if (EIUtils::hasCitizenship(docData, indiv, natTuple.get<0>())) {
							natBoundIDs.insert(natTuple.get<2>());
							ret = true;
						}
					}
				}
			}
		}
	}
	return ret;
}

// This is never actually called.
//bool ElfInference::isMemberishRelation(const ElfRelation_ptr& relation) {
//	const wstring name = relation->get_name();
//	return name == L"eru:isLedBy" || name == L"eru:hasMember" || name == L"eru:employs";
//}

///////////////////////////////////////////////////////////////////////
//                                                                  //
// Functions for dealing with informal person groups                //
// (e.g. *7 Iraqi engineers* --> the engineer,                      //
//                                                                  //
// 12/22/2010:  I'm checking this function in, but                  //
// current behavior returns an empty set.  This type of             //
// inference will require more work to remove false positives       //
//////////////////////////////////////////////////////////////////////
std::set<ElfRelation_ptr> EIUtils::getRelationsFromPersonCollections(EIDocData_ptr docData){
	std::set<ElfRelation_ptr> relations_to_add;
	SessionLogger::warn("LEARNIT")<<"ElfInference::getRelationsFromPersonCollections() should not be used without further development."
		<<"It adds more incorrect answers than correct answers"<<std::endl;
	return relations_to_add;

	const EntitySet* entity_set = docData->getEntitySet();
	typedef boost::tuple<const Entity*, const Mention*, Symbol> entity_mention_stem_t; // for BOOST_FOREACH
	std::vector<boost::tuple<const Entity*, const Mention*, Symbol> > collections;
	for(int ent_no = 0; ent_no < entity_set->getNEntities(); ent_no++){
		const Entity* entity = entity_set->getEntity(ent_no);
		if(entity->getType().matchesPER()){
			for(int ment_no = 0; ment_no < entity->getNMentions(); ment_no++){
				const Mention* mention = entity_set->getMention(entity->getMention(ment_no));
				if(mention->getMentionType() == Mention::DESC && mention->getHead() != NULL){
					Symbol hw = mention->getHead()->getHeadWord();
					//look for the stem in wordnet, if the word != stem, then assume that word is plural 
					//(plural/singular is the only inflection morphology in English)
					Symbol stem = WordNet::getInstance()->stem_noun(hw);
					if(hw != stem){
						collections.push_back(boost::tuple<const Entity*, const Mention*, Symbol>(entity, mention, stem));
					}
				}
			}
		}
	}
	//find members of each collection
	//std::vector<boost::tuple<const Entity*, const Mention*, Symbol, const Entity*, const Mention*> > collection_member_list;
	BOOST_FOREACH(entity_mention_stem_t collection, collections){
		const Entity* collection_entity = boost::tuples::get<0>(collection);
		const Mention* collection_mention = boost::tuples::get<1>(collection);
		std::vector<std::pair<const Entity* , std::vector<const Mention*> > > member_list;
		for(int ent_no = 0; ent_no < entity_set->getNEntities(); ent_no++){
			if(ent_no == collection_entity->getID())
				continue;
			const Entity* member_entity = entity_set->getEntity(ent_no);
			if(!member_entity->getType().matchesPER())
				continue;
			Symbol collection_stem = boost::tuples::get<2>(collection);
			std::pair<const Entity*, std::vector<const Mention*> > member_mentions;
			member_mentions.first = member_entity;
			for(int ment_no = 0; ment_no < member_entity->getNMentions(); ment_no++){
				const Mention* member_mention = entity_set->getMention(member_entity->getMention(ment_no));
				if(member_mention->getMentionType() == Mention::DESC && member_mention->getHead() != NULL){
					Symbol hw = member_mention->getHead()->getHeadWord();
					if(hw == collection_stem){
						member_mentions.second.push_back(member_mention);
					}
				}
			}
			if(member_mentions.second.size() > 0)
				member_list.push_back(member_mentions);
		}
		if(member_list.size() == 1){ //collection with one entity to which it belongs
			typedef std::pair<std::wstring, std::vector<ElfRelation_ptr> > relmap_pair_t;
			ElfRelationMap collection_rel_map = EIUtils::findRelationsForEntity(docData, collection_entity->getID());
			ElfRelationMap member_rel_map = EIUtils::findRelationsForEntity(docData, member_list[0].first->getID());
			BOOST_FOREACH(relmap_pair_t pair, collection_rel_map){
				if(!(pair.first == L"eru:hasCitizenship" || pair.first == L"eru:employs" || 
					pair.first == L"eru:attendedSchool" ||
					pair.first == L"eru:groupKillingEvent" || pair.first == L"eru:personKillingEvent" ||
					pair.first == L"eru:killedPersonEvent" || pair.first == L"eru:killedGroupEvent" ||
					pair.first == L"eru:groupKilledPersonEvent" ||
					pair.first == L"eru:injuredPersonEvent" ||
					pair.first == L"eru:hasGender" || pair.first == L"eru:hasMemberPerson"))
				{
					continue; //not a transferable relation
				}
				if(member_rel_map.find(pair.first) != member_rel_map.end()){
					continue; //member already has a relation of this type, so don't add one
				}
				std::vector<std::vector<ElfRelation_ptr> > relation_groups = EIUtils::groupEquivalentRelations(pair.second);
				if(relation_groups.size() != 1){
					continue; //multiple relations of the same type for this collection, some of them are likely to be incorrect
				}
				ElfRelation_ptr elf_relation = (relation_groups[0])[0];
				std::vector<ElfRelationArg_ptr> collection_args = EIUtils::getArgsWithEntity(elf_relation, 
					collection_entity);
				if(collection_args.size() != 1){
					continue; //the collection entity plays multiple roles in this relation, usually that is bad
				}
				BOOST_FOREACH(ElfRelationArg_ptr arg, elf_relation->get_args()){
					if(arg == collection_args[0])
						continue;
					//does the entity mention
				}
				const Mention* member_mention = member_list[0].second[0];
				std::pair<EDTOffset, EDTOffset> collect_mention_offsets = 
					EIUtils::getSentenceEDTOffsetsForMention(docData, collection_mention);
				std::pair<EDTOffset, EDTOffset> member_mention_offsets = 
					EIUtils::getSentenceEDTOffsetsForMention(docData, member_mention);
				EDTOffset start = min(collect_mention_offsets.first, member_mention_offsets.first);
				start = min(start, elf_relation->get_start());
				EDTOffset end = max(collect_mention_offsets.second, member_mention_offsets.second);
				end = max(end, elf_relation->get_end());
				const DocTheory* docTheory = docData->getDocTheory();
				ElfRelation_ptr new_rel = EIUtils::copyRelationAndSubstituteArgs(docData, elf_relation, 
					L"collectionInference", collection_args[0], member_mention, L"ic:Person", start, end);
				relations_to_add.insert(new_rel);
			}
		}
	}
	return relations_to_add;
}

/* */
void EIUtils::addBoundTypesToIndividuals(EIDocData_ptr docData){
	ElfIndividualSet individuals = docData->get_individuals_by_type();
	ElfIndividualSet individuals_to_add;
	BOOST_FOREACH(ElfIndividual_ptr individual, individuals) {
		if (!individual->has_bound_uri())
			continue;
		std::wstring bound_uri = individual->get_bound_uri();
		ElfType_ptr type_to_copy = individual->get_type();
		EDTOffset start;
		EDTOffset end;
		type_to_copy->get_offsets(start, end);
		std::set<std::wstring> types = ElfMultiDoc::get_types_for_bound_uri(bound_uri);
		BOOST_FOREACH(std::wstring type, types){
			if(!individual->has_type(type)){
				// Copy the bound URI-containing individual and replace its type with this one from the lookup table
				ElfIndividual_ptr new_individual = boost::make_shared<ElfIndividual>(individual);
				ElfType_ptr new_type = boost::make_shared<ElfType>(type, type_to_copy->get_string(), start, end);
				new_individual->set_type(new_type);
				individuals_to_add.insert(new_individual);
			}	
		}
	}

	// Insert any generated type assertions
	BOOST_FOREACH(ElfIndividual_ptr individual, individuals_to_add) {
		docData->getElfDoc()->insert_individual(individual);
	}
}

const Mention* EIUtils::getImmediateTitle(const DocTheory* docTheory, const Mention* mention){
	const SentenceTheory* sTheory = docTheory->getSentenceTheory(mention->getSentenceNumber());
	const EntitySet* eSet = docTheory->getEntitySet();
	const Entity* thisEntity = docTheory->getEntityByMention(mention);
	for(int mno = 0; mno < thisEntity->getNMentions(); mno++){	//premods
		const Mention* othMention = eSet->getMention(thisEntity->getMention(mno));
		if(othMention->getSentenceNumber() == mention->getSentenceNumber() && 
			othMention->getMentionType() == Mention::DESC){ //descriptor in this sentence
				//if premod, name mention extent will include descriptor
				if((mention->getNode()->getStartToken() <= othMention->getNode()->getStartToken()) &&
					mention->getNode()->getEndToken() >= othMention->getNode()->getEndToken()){
						return othMention;
				}
		}
	}
	return 0;
}
const bool EIUtils::entityMatchesSymbolGroup(const DocTheory* docTheory, const Entity* entity, Symbol::SymbolGroup sGroup){
	const EntitySet* eSet = docTheory->getEntitySet();
	for(int mno = 0; mno < entity->getNMentions(); mno++){	//premods
		const Mention* othMention = eSet->getMention(entity->getMention(mno));
		if((othMention->getMentionType() == Mention::NAME) || (othMention->getMentionType() == Mention::DESC)){
			const SynNode* head = othMention->getAtomicHead();
			Symbol syms[101];
			int n_syms = head->getTerminalSymbols(syms, 100);
			for(int i =0; i< n_syms; i++){
				if(syms[i].isInSymbolGroup(sGroup))
					return true;
			}
		}
	}
	return false;
}

const bool EIUtils::mentionMatchesSymbolGroup(const DocTheory* docTheory, const Mention* mention, Symbol::SymbolGroup sGroup, bool use_entity){
	const SentenceTheory* sTheory = docTheory->getSentenceTheory(mention->getSentenceNumber());
	Symbol syms[101];
	int n_syms = mention->getAtomicHead()->getTerminalSymbols(syms, 100);
	for(int i =0; i< n_syms; i++){
		if(syms[i].isInSymbolGroup(sGroup))
			return true;
	}
	if(use_entity){
		const Entity* thisEntity = docTheory->getEntityByMention(mention);
		return entityMatchesSymbolGroup(docTheory, thisEntity, sGroup);
	}
	return false;
}
const bool EIUtils::isFutureDate(const DocTheory* docTheory, const Value* v){
	// Assumes XXX_XXX_YYYYMMDD. Can be changed later if necessary!
	std::wstring norm_str = L"";
	std::wstring date_norm_string = L"";
	if(v->getTimexVal() == EIUtils::FUTURE_REF)
		return true;
	if(v->getTimexVal() == Symbol()){
		//date_norm_string = vm->toString(docTheory->getSentenceTheory(vm->getSentenceNumber())->getTokenSequence());
		return false; //not a date so can't be in the future
	}
	else{
		date_norm_string = v->getTimexVal().to_string();
	}	


	std::wstring origDocID = docTheory->getDocument()->getName().to_string();
	//date was normalized to a day after the date of publication
	if (date_norm_string.length() == 10 && origDocID.length() > 16) {
		std::wstring date_value_int_ready = 
			date_norm_string.substr(0,4) + date_norm_string.substr(5,2) + date_norm_string.substr(8,2);
		int date_value_int = _wtoi(date_value_int_ready.c_str());
		int doc_date_int = _wtoi(origDocID.substr(8,8).c_str());
		if (doc_date_int != 0 && doc_date_int < date_value_int) {
			std::wcout<<"Future date: docdate: "<<doc_date_int<<" date: "<<date_value_int<<std::endl;
			return true;
		} 
	}
	if (date_norm_string.length() == 7 && origDocID.length() > 16) {
		std::wstring date_value_int_ready = date_norm_string.substr(0,4) + date_norm_string.substr(5,2);
		int date_value_int = _wtoi(date_value_int_ready.c_str());
		int doc_date_int = _wtoi(origDocID.substr(8,6).c_str());
		if (doc_date_int != 0 && doc_date_int < date_value_int) {
			std::wcout<<"Future date: docdate: "<<doc_date_int<<" date: "<<date_value_int<<std::endl;
			return true;

		} 
	}
	std::wstring sent_string = docTheory->getSentenceTheory(v->getSentenceNumber())->getTokenSequence()->toString();
	const SynNode* root = docTheory->getSentenceTheory(v->getSentenceNumber())->getPrimaryParse()->getRoot();
	Symbol pos[201];
	Symbol words[201];
	Symbol::SymbolGroup futureAndConditional = Symbol::makeSymbolGroup(L"will could would might");
	int nPOS = root->getPOSSymbols(pos, 200);
	int nwords = root->getTerminalSymbols(words, 200);
	bool hasPast = false;
	bool hasFutureConditional = false;
	for(int i = 0; i< nPOS; i++){
		if(pos[i] == Symbol(L"VBD")){
			hasPast = false;
		}
		if(pos[i] == Symbol(L"MD") && words[i].isInSymbolGroup(futureAndConditional)){
			hasFutureConditional = true;
		}
	}

	//if(hasFutureConditional){
	//	std::wcout<<"Future date: found conditional "<<std::endl;
	//	return true;
	//}
	const TokenSequence* toks = docTheory->getSentenceTheory(v->getSentenceNumber())->getTokenSequence();
	std::wstring dateText = toks->toString(v->getStartToken(), v->getEndToken());
	std::transform(dateText.begin(), dateText.end(), dateText.begin(), ::tolower);
	dateText = L" "+dateText+L" ";
	if(dateText.find(L" next ") != std::wstring::npos){
		std::wcout<<"Future date: found next "<<dateText<<std::endl;
		return true;
	}
	if((dateText.find(L" this ") != std::wstring::npos) && hasFutureConditional){
		std::wcout<<"Future date: found this with conditional "<<dateText<<std::endl;
		return true;
	}
	return false;

}
const std::set<ElfRelationArg_ptr> EIUtils::alignArgSets(const DocTheory* docTheory, const std::set<ElfRelationArg_ptr> args1, const std::set<ElfRelationArg_ptr> args2){
	std::set<ElfRelationArg_ptr> alignedArgs;
	BOOST_FOREACH(ElfRelationArg_ptr arg1, args1){
		BOOST_FOREACH(ElfRelationArg_ptr arg2, args2){
			if(arg1->isSameAndContains(arg2, docTheory)){
				alignedArgs.insert(arg1);
			}
			else if(arg2->isSameAndContains(arg1, docTheory)){
				alignedArgs.insert(arg2);
			}
		}
	}
	return alignedArgs;
}

std::vector<ElfRelation_ptr> EIUtils::makeBestCoveringRelation(const DocTheory* docTheory, const ElfRelation_ptr r1, const ElfRelation_ptr r2){
	std::vector<ElfRelation_ptr> newRelations;
	if(r1->get_name() != r2->get_name())
		return newRelations;
	std::set<ElfRelationArg_ptr> r1TemporalArgs;
	std::set<ElfRelationArg_ptr> r2TemporalArgs;
	std::set<ElfRelationArg_ptr> r1OtherArgs;
	std::set<ElfRelationArg_ptr> r2OtherArgs;
	int sno1 = 0;
	int sno2 = 0;
	BOOST_FOREACH(ElfRelationArg_ptr arg, r1->get_args()){
		if(arg->get_role().find(L"t:") == 0){
			r1TemporalArgs.insert(arg);
		}
		else{
			sno1 = EIUtils::getSentenceNumberForArg(docTheory, arg);
			r1OtherArgs.insert(arg);
		}
	}
	BOOST_FOREACH(ElfRelationArg_ptr arg, r2->get_args()){
		if(arg->get_role().find(L"t:") == 0){
			r2TemporalArgs.insert(arg);
		}
		else{
			sno2 = EIUtils::getSentenceNumberForArg(docTheory, arg);
			r2OtherArgs.insert(arg);
		}
	}
	if(sno1 != sno2)	//don't bother with alignment if they are from different sentences
		return newRelations;

	std::set<ElfRelationArg_ptr> alignedOthers = alignArgSets(docTheory, r1OtherArgs, r2OtherArgs);
	if(alignedOthers.size() != std::max(r1OtherArgs.size(), r2OtherArgs.size())) 
		return newRelations;


	std::set<ElfRelationArg_ptr> alignedTemporals;
	if(r1TemporalArgs.size() > 0 && r2TemporalArgs.size() > 0){
		alignedTemporals = alignArgSets(docTheory, r1TemporalArgs, r2TemporalArgs);
	} else if(r1TemporalArgs.size() > 0){
		alignedTemporals.insert(r1TemporalArgs.begin(), r1TemporalArgs.end());
	} else if(r2TemporalArgs.size() > 0){
		alignedTemporals.insert(r2TemporalArgs.begin(), r2TemporalArgs.end());
	}
	if((alignedOthers.size() == std::max(r1OtherArgs.size(), r2OtherArgs.size())) &&
		(alignedTemporals.size() == std::max(r1TemporalArgs.size(), r2TemporalArgs.size()))){ //we've aligned everything so make a new relation
		EDTOffset start = std::min(r1->get_start(), r2->get_start());
		EDTOffset end = std::max(r2->get_end(), r1->get_end());
		LocatedString* relationTextLS = MainUtilities::substringFromEdtOffsets(
			docTheory->getDocument()->getOriginalText(), start, end);
		std::wstring relation_text = relationTextLS->toString();
		delete relationTextLS;
		std::vector<ElfRelationArg_ptr> argsToInclude;
		argsToInclude.insert(argsToInclude.begin(), alignedTemporals.begin(), alignedTemporals.end());
		argsToInclude.insert(argsToInclude.begin(), alignedOthers.begin(), alignedOthers.end());
		ElfRelation_ptr new_relation = boost::make_shared<ElfRelation>(r1->get_name(), 
					argsToInclude, relation_text, start, end, 
					std::max(r1->get_confidence(), r2->get_confidence()), 
					std::min(r1->get_score_group(), r2->get_score_group()));
				new_relation->set_source(r1->get_source());
				//new_relation->add_source(r1->get_source());
				new_relation->add_source(r2->get_source());
				newRelations.push_back(new_relation);
				return newRelations;
	}
	
	
	return newRelations;
}

bool EIUtils::isBadTitleWord(const std::wstring& word) {
	static std::set<std::wstring> BAD_TITLES =
		makeStrSet(L"native graduate -old challenger champion son opponent rival");

	return BAD_TITLES.find(word) != BAD_TITLES.end();
}

bool EIUtils::isCountryWord(const std::wstring& word) {
	return _countryNameEquivalenceTable->isInTable(
			normalizeCountryName(word), 0.0);
}

bool EIUtils::titleFromMention(EIDocData_ptr docData,
	const Mention* m, int& start_token, int& end_token)
{
	static std::set< std::wstring > UNCAPITALIZED_ROYALTY = 
		makeStrSet(L"queen king lord duke prince princess");

	static std::set< std::wstring > FORMER_RETIRED = 
		makeStrSet(L"former retired new black -born past onetime one-time " // Be sure to keep trailing spaces!
			L"-old brother caretaker celebrated conservative english "
			L"full-time gay hard-boiled kid leftist lesbian liberal "
			L"moderate multimillionaire next opposition ousted sack shadow the woman "
			L"first second third one-term two-term three-term four-term five-term six-term "
			L"-term military star fourth fifth sixth");

	static std::set< std::wstring > ACADEMIC_ACTING =
		makeStrSet(L"/ academic acting adjunct administrative advertising aeronautical "
		L"alumni appellate armed artistic assistant associate athletic "
		L"attorney auxiliary central certified chief chief-executive civil civilian classroom "
		L"clinical co-chief co-creative co-executive co-general col. college-football collegiate "
		L"commanding commercial common congressional constitutional contributing coordinating corporate "
		L"cosmonaut court creative cricket crown cub defensive deputy detective diplomatic "
		L"director- district divisional divorce doctoral domestic domestic-policy economic electoral "
		L"electrical elementary emeritus environmental equestrian executive external faculty "
		L"federal finance financial first foreign foreign-policy forensic "
		L"founding four-star full fund-raising funeral general global goodwill graduate handball head "
		L"health hedge-fund high honorary human infantry infirmary inspector institutional interim "
		L"interior internal international investigative joint joint-managing junior juvenile lead "
		L"leading legal legislative leveraged-finance lieutenant limited lower-division lt. "
		L"magistrate main maj. major major-league managing marketing mechanical "
		L"medical metallurgical migrant military minor-league municipal mutual mutual-fund national "
		L"naval non-executive nonexecutive northern nuclear offensive official one-star operating "
		L"outside overseas parliamentary parole pension-fund personal personnel physical poker "
		L"police political presidential primary prime princess principal private "
		L"pro pro- probationary professor programming prosecutor provincial public publishing "
		L"publishing-group regional regulatory resource retail riverboat ruling scientific scouting "
		L"second secretaries- secretary secretary- secretary-general security seminary senior "
		L"soccer social-work space-flight special spiritual staff star state strategic sub- "
		L"superior supervising supreme surgical tactical talk-show technical textbook theatrical "
		L"third top veteran vice vice- vice-chancellor vice-foreign vice-president vocational "
		L"ward wedding wholesale wide working");

	if (!m) {
		//SessionLogger::info("foo") << "Bailed on null mention";
		return false;
	}

	SentenceTheory *sTheory = 
		docData->getSentenceTheory(m->getSentenceNumber());	
	const TokenSequence *ts = sTheory->getTokenSequence();
	const SynNode *root = sTheory->getPrimaryParse()->getRoot();
	const SynNode *headNode = m->getNode()->getHeadPreterm();
	start_token = headNode->getStartToken();
	int original_start_token = start_token;
	end_token = headNode->getEndToken();
	// names like "Albert King"
	if (headNode->getParent() != 0 && headNode->getParent()->getTag() == Symbol(L"NPP"))
		return false;

	// uncapitalized royalty is usually metaphorical
	std::wstring headword = ts->getToken(headNode->getStartToken())->getSymbol().to_string();
	if (UNCAPITALIZED_ROYALTY.find(headword) != UNCAPITALIZED_ROYALTY.end())
		return false;

	if (isBadTitleWord(headNode->getHeadWord().to_string())) {
		// returning true is temporary hack for KBP, may break blitz, will fix after eval
		// the problem is that KBP doesn't want to nuke everything we
		// currently return false for...
		return true;
	}

	bool minister_or_secretary = (headword == L"minister") || (headword == L"Minister")
		|| (headword == L"Secretary") || (headword == L"secretary")
		|| (headword == L"-elect");
	bool saw_non_nnps = false;

	/*SessionLogger::info("foo") << L"TITLE TRIMMING!";
	SessionLogger::info("foo") << L"Head word is: " << headword;
	SessionLogger::info("foo") << L"Initial start token is " << start_token
		<< L"; end token is " << end_token;*/

	while (start_token > 0) {	
		const SynNode *preterm = root->getCoveringNodeFromTokenSpan(start_token-1,start_token-1)->getParent();
		std::wstring prev_word = preterm->getHeadWord().to_string();
		//SessionLogger::info("foo") << L"Considering premod (" << preterm->getTag() << L" " << prev_word << L")";
		if (!minister_or_secretary && preterm->getParent() != 0 
				&& preterm->getParent()->getTag() == Symbol(L"NPP")) {
			break;		
		}
		// some of this is redundant now that we don't allow JJ/NNP/NNPS/VBG except from a word-list
		if (FORMER_RETIRED.find(prev_word) != FORMER_RETIRED.end())
			break;
		// words with numbers are bad, e.g. "1994"
		if (prev_word.at(0) == L'-' ||
				prev_word.find_first_of(L"0123456789") != std::wstring::npos)
			break;

		if (preterm->getTag() == Symbol(L"NN") || preterm->getTag() == Symbol(L"NNS") ||
				ACADEMIC_ACTING.find(prev_word) != ACADEMIC_ACTING.end() )
		{
			saw_non_nnps = true;
			--start_token;
			//SessionLogger::info("LEARNIT") << preterm->getTag() << ": " << prev_word << "\n";
			//SessionLogger::info("foo") << "ACCEPTED!";
		} else if (minister_or_secretary && !saw_non_nnps  && !isCountryWord(prev_word) 
					 && (preterm->getTag() == Symbol(L"NNP")
						 || preterm->getTag() == Symbol(L"NNPS"))) 
		{
			--start_token;
		}

		else break;
	}

	bool has_premod = (start_token != original_start_token);
	if (has_premod) {
		// prevent cases like "prime minister of Japan"
		minister_or_secretary = false;
	}

	if (end_token + 1 < ts->getNTokens()) {
		std::wstring next_word = ts->getToken(end_token+1)->getSymbol().to_string();
		boost::to_lower(next_word);
		if (next_word == L"of") {
			bool good_pp = true;
			// trying to fix these cases was removing good things like
			// "[chief of staff] Samuel Skinner", so I'm commenting it out
			// for now
			/*if (!minister_or_secretary) {
				// check for cases like "chariman of aluminum giant Alcoa"
				const SynNode* IN_node = root->getCoveringNodeFromTokenSpan(end_token+1, end_token+1)->getParent();
				if (IN_node) {
					const SynNode* PP_node = IN_node->getParent();
					if (PP_node && PP_node->getTag() == Symbol(L"PP")) {
						for (int i=PP_node->getStartToken(); i<=PP_node->getEndToken(); ++i) {
							const SynNode* preterm = root->getCoveringNodeFromTokenSpan(i,i)->getParent();
							if (preterm->getTag() == Symbol(L"NNP") ||
									preterm->getTag() == Symbol(L"NNPS")) {
								good_pp = false;
								break;
							}
						}
					}
				}
			}*/

			if (good_pp) {
				int last_nn = end_token;
				end_token++;
				while (end_token + 1 < ts->getNTokens()) {
					const SynNode *preterm = root->getCoveringNodeFromTokenSpan(end_token+1,end_token+1)->getParent();
					/*SessionLogger::info("foo") << L"Considering postmod (" << preterm->getTag() << L" "
					  << preterm->getHeadWord().to_string() << L")";*/
					if (!minister_or_secretary && preterm->getParent() != 0 
							&& preterm->getParent()->getTag() == Symbol(L"NPP")) {
						break;						
					}
					if (preterm->getTag() == Symbol(L"NN") || preterm->getTag() == Symbol(L"NNS") 
							|| (minister_or_secretary && 
								(preterm->getTag() == Symbol(L"NNP") 
								 || preterm->getTag() == Symbol(L"NNPS"))))
					{
						end_token++;
						last_nn = end_token;
						//SessionLogger::info("foo") << L"ACCEPTED!";
					} else if (preterm->getTag() == Symbol(L"JJ")) {
						end_token++;
						//SessionLogger::info("foo") << L"PROVISIONALLY ACCEPTED!";
					} else break;
				}
				end_token = last_nn;
			}
		}
	}

	/*SessionLogger::info("foo") << L"Final start token is " << start_token
		<< L"; end token is " << end_token;*/
	return true;
}

ElfIndividual_ptr EIUtils::dateIndividualFromSpecString(const std::wstring& spec,
		const std::wstring& text, EDTOffset new_start, EDTOffset new_end)
{
	return boost::make_shared<ElfIndividual>(L"xsd:string", spec,
			text, new_start, new_end);
}

bool EIUtils::argSentenceContainsWord(ElfRelationArg_ptr arg,
		EIDocData_ptr docData, const Symbol& word)
{
	int sn = EIUtils::getSentenceNumberForArg(docData->getDocTheory(), arg);
	const SentenceTheory* st = docData->getSentenceTheory(sn);
	const TokenSequence* ts = st->getTokenSequence();

	for (int i=0; i<ts->getNTokens(); ++i) {
		if (ts->getToken(i)->getSymbol() == word) {
			return true;
		}
	}

	return false;
}


void EIUtils::spanOfIndividuals(ElfIndividual_ptr a, ElfIndividual_ptr b,
			EDTOffset& start, EDTOffset& end)
{
	EDTOffset a_start, a_end, b_start, b_end;
	a->get_spanning_offsets(a_start, a_end);
	b->get_spanning_offsets(b_start, b_end);

	start = (std::min)(a_start, b_start);
	end = (std::min)(a_end, b_end);
}

void EIUtils::sentenceSpanOfIndividuals(EIDocData_ptr docData, 
		ElfIndividual_ptr a, ElfIndividual_ptr b,
		EDTOffset& start, EDTOffset& end) 
{
	int sn_a = getSentenceNumberForIndividualStart(docData->getDocTheory(), a);
	int sn_b = getSentenceNumberForIndividualStart(docData->getDocTheory(), b);
	spanOfSentences(docData, sn_a, sn_b, start, end);
}

std::wstring EIUtils::textFromIndividualSentences(EIDocData_ptr docData,
		ElfIndividual_ptr a, ElfIndividual_ptr b) 
{
	int sn_a = getSentenceNumberForIndividualStart(docData->getDocTheory(), a);
	int sn_b = getSentenceNumberForIndividualStart(docData->getDocTheory(), b);
	
	int first_sent = (std::min)(sn_a, sn_b);
	int second_sent = (std::max)(sn_a, sn_b);

	std::wstringstream str;
	
	str << textOfSentence(docData, first_sent);
	
	if (first_sent != second_sent) {
		str << L" ... " << textOfSentence(docData, second_sent);
	}

	return str.str();
}

std::wstring EIUtils::textOfSentence(EIDocData_ptr docData, int sn) {
	EDTOffset start, end;
	spanOfSentences(docData, sn, sn, start, end);
	return textOfSpan(docData, start, end);
}

void EIUtils::spanOfSentences(EIDocData_ptr docData, int sn1, int sn2,
		EDTOffset& start, EDTOffset& end) 
{
	int start_sent = (std::min)(sn1,sn2);
	int end_sent = (std::max)(sn1,sn2);
	TokenSequence* start_ts = docData->getSentenceTheory(start_sent)->getTokenSequence();
	TokenSequence* end_ts = docData->getSentenceTheory(end_sent)->getTokenSequence();
	start = start_ts->getToken(0)->getStartEDTOffset();
	end = end_ts->getToken(end_ts->getNTokens() - 1)->getEndEDTOffset();
}

std::wstring EIUtils::textOfSpan(EIDocData_ptr docData, EDTOffset start,
		EDTOffset end)
{
	LocatedString* textLS = MainUtilities::substringFromEdtOffsets(
			docData->getDocument()->getOriginalText(), start, end);
	std::wstring ret = textLS->toString();
	delete textLS;
	return ret;
}

