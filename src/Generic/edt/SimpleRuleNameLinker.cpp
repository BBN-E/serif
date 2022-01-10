// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/edt/SimpleRuleNameLinker.h"
#include "Generic/theories/SynNode.h"
#include "Generic/edt/CorefUtilities.h"
#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

const int maxTokenCount = 50;
const int initialTableSize = 5;

SimpleRuleNameLinker::SimpleRuleNameLinker(): _debug(Symbol(L"rule_name_link_debug"))
{
	_noise = _new HashTable(50);
	loadFileIntoTable(ParamReader::getRequiredParam("linker_noise"), _noise);

	_alternateSpellings = _new HashTable(initialTableSize);
	loadAlternateSpellings(ParamReader::getRequiredParam("linker_nations"));
	loadAlternateSpellings(ParamReader::getRequiredParam("linker_alt_spellings"));

	_suffixes = _new HashTable(initialTableSize);
	loadFileIntoTable(ParamReader::getRequiredParam("linker_suffixes"), _suffixes);
	HashTable::iterator iter;


	_designators = _new HashTable(initialTableSize);
	loadFileIntoTable(ParamReader::getRequiredParam("linker_designators"), _designators);

	_capitals = _new HashTable(initialTableSize);
	loadCapitalsAndCountries(ParamReader::getRequiredParam("linker_capitals_and_countries"));

	_mentions = _new HashTable(3);
	_normalizedMentions = _new HashTable(3);
	_distillationMode = false;
	if (ParamReader::getParam(L"simple_rule_name_link_distillation").is_null())
		_debug << "ERROR: simple-rule-name-link-distillation \n";
	else{
		if(ParamReader::getRequiredTrueFalseParam("simple_rule_name_link_distillation")){
			_distillationMode = true;
		}
		CorefUtilities::initializeWKLists();
	}

	_edit_distance_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("simple_rule_name_link_edit_distance_threshold", 0.19);

	_debug << "SimpleRuleNameLinker is initialized. \n";
}

SimpleRuleNameLinker::~SimpleRuleNameLinker()
{
	clearMentions();
	delete _mentions;
	delete _normalizedMentions;

	HashTable::iterator iter;
	for (iter = _noise->begin(); iter != _noise->end(); ++iter) {
		delete [] (*iter).first;
	}
	delete _noise;

	for (iter = _suffixes->begin(); iter != _suffixes->end(); ++iter) {
		delete [] (*iter).first;
	}
	delete _suffixes;

	for (iter = _alternateSpellings->begin(); iter != _alternateSpellings->end(); ++iter) {
		delete [] (*iter).first;
	}
	delete _alternateSpellings;

	for (iter = _designators->begin(); iter != _designators->end(); ++iter) {
		delete [] (*iter).first;
	}
	delete _designators;
}

void SimpleRuleNameLinker::setCurrSolution(LexEntitySet * currSolution) {
	_currSolution = currSolution;
}

void SimpleRuleNameLinker::loadFileIntoTable(std::string filepath, HashTable * table) {
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(filepath.c_str()));
	UTF8InputStream& in(*in_scoped_ptr);
	std::wstring buffer;
	while (!in.eof()) {
		in.getLine(buffer);
		std::transform(buffer.begin(), buffer.end(), buffer.begin(), towlower);
		Symbol *tokens = getSymbolArray(buffer.c_str());
		if (getValueFromTable(table, tokens).is_null())
			(*table)[tokens] = Symbol(L"1");
		else
			delete [] tokens;
	}
	in.close();
}

void SimpleRuleNameLinker::loadAlternateSpellings(std::string filepath) {
	// Read ";" separated values file. The first value is the head.
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(filepath.c_str()));
	UTF8InputStream& in(*in_scoped_ptr);
	std::wstring buf;
	while (!in.eof()) {
		in.getLine(buf);
		//buf = buf.substr(0, buf.length() - 1);
		size_t begin = 0;
		size_t iter = buf.find(';', 0);
		Symbol head;
		if (iter != string::npos) {
			std::wstring entry = buf.substr(begin, iter - begin);
			std::transform(entry.begin(), entry.end(), entry.begin(), towlower);
			head = Symbol(entry.c_str());
			iter += 2;
			begin = iter;
			while((iter = buf.find(';', iter)) != string::npos) {
				entry = buf.substr(begin, iter - begin);
				std::transform(entry.begin(), entry.end(), entry.begin(), towlower);
				Symbol *tokens = getSymbolArray(entry.c_str());
				if (getValueFromTable(_alternateSpellings, tokens).is_null())
					(*_alternateSpellings)[tokens] = head;
				else
					delete [] tokens;
				iter += 2;
				begin = iter;
			}
			entry = buf.substr(begin, buf.length() - begin);
			std::transform(entry.begin(), entry.end(), entry.begin(), towlower);
			Symbol * tokens = getSymbolArray(entry.c_str());
			if (getValueFromTable(_alternateSpellings, tokens).is_null())
				(*_alternateSpellings)[tokens] = head;
			else
				delete [] tokens;
		}
	}
	in.close();
}

void SimpleRuleNameLinker::loadCapitalsAndCountries(std::string filepath) {
	// Read ";" separated values file. The first value is the head.
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(filepath.c_str()));
	UTF8InputStream& in(*in_scoped_ptr);
	std::wstring buf;
	while (!in.eof()) {
		in.getLine(buf);
		//buf = buf.substr(0, buf.length() - 1);
		size_t begin = 0;
		size_t iter = buf.find(';', 0);
		Symbol head;
		if (iter != string::npos) {
			std::wstring entry = buf.substr(begin, iter - begin);
			std::transform(entry.begin(), entry.end(), entry.begin(), towlower);
			head = Symbol(entry.c_str());
			iter += 2;
			begin = iter;
			while((iter = buf.find(';', iter)) != string::npos) {
				entry = buf.substr(begin, iter - begin);
				std::transform(entry.begin(), entry.end(), entry.begin(), towlower);
				Symbol * tokens = getSymbolArray(entry.c_str());
				if (getValueFromTable(_capitals, tokens).is_null())
					(*_capitals)[tokens] = head;
				else
					delete [] tokens;
				iter += 2;
				begin = iter;
			}
			entry = buf.substr(begin, buf.length() - begin);
			std::transform(entry.begin(), entry.end(), entry.begin(), towlower);
			Symbol * tokens = getSymbolArray(entry.c_str());
			if (getValueFromTable(_capitals, tokens).is_null())
				(*_capitals)[tokens] = head;
			else
				delete [] tokens;
		}
	}
	in.close();
}

Symbol * SimpleRuleNameLinker::getSymbolArray(const wchar_t * str) {
	Symbol * symArray = _new Symbol[maxTokenCount];
	wchar_t buffer[1024];
	wcscpy(buffer, str);
#ifdef _WIN32
	const wchar_t *token = wcstok(buffer, L" ");
#else
	wchar_t *state;
	const wchar_t *token = wcstok(buffer, L" ", &state);
#endif
	int counter = 0;
	while((counter < maxTokenCount) && (token != NULL)){
		symArray[counter] = Symbol(token);
		counter++;
#ifdef _WIN32
		token = wcstok(NULL, L" ");
#else
		token = wcstok(NULL, L" ", &state);
#endif
	} 
	return symArray;
}

int SimpleRuleNameLinker::getTokenCount(Symbol * tokens) {
	int counter = 0;
	while((counter < maxTokenCount) && (!tokens[counter].is_null())) {
		counter++;
	} 	
	return counter;
}

bool SimpleRuleNameLinker::isOKToLink(Mention * currMention, EntityType linkType, EntityGuess * entGuess) {
	// delete all previous mentions
	clearMentions();
	// load all the name mentions in the entity into a hashtable
	loadNameMentions(_currSolution->getEntity(entGuess->id));
	bool okToLink = false;
	// always link if this is the fisrt name mention
	if (_nNameMentions == 0)
		okToLink = true;
	if (linkType.matchesPER())
		okToLink = isOKToLinkPER(currMention, entGuess);
	else if (linkType.matchesORG())
		okToLink = isOKToLinkORG(currMention, entGuess);
	else if (linkType.matchesGPE())
		okToLink = isOKToLinkGPE(currMention, entGuess);
	else
		okToLink = isOKToLinkOTH(currMention, entGuess);

	if (!okToLink)
		_debug << "SimpleRuleNameLinker: Don't link mention ID " << currMention->getUID() << " with Entity ID " << entGuess->id << "\n";

	return okToLink;
}

bool SimpleRuleNameLinker::isCityAndCountry(Mention* currMention, EntityType linkType, EntityGuess *entGuess){
	if(!linkType.matchesGPE())
		return false;
	bool currIsCity = false;
	bool currIsCountry = false;
	if(currMention->getEntitySubtype().isDetermined()){
		if(currMention->getEntitySubtype().getName() == Symbol(L"Population-Center")){
			currIsCity = true;
		}
		if(currMention->getEntitySubtype().getName() == Symbol(L"Nation")){
			currIsCountry = true;
		}
	}
	bool othIsCity = false;
	bool othIsCountry = false;
	const Entity* oth_entity = _currSolution->getEntity(entGuess->id);
	for (int i = 0; i < oth_entity->getNMentions(); i++) {
		Mention * oth = _currSolution->getMention(oth_entity->getMention(i));
		if(oth->getEntitySubtype().isDetermined()){															
			if(oth->getEntitySubtype().getName() == Symbol(L"Population-Center")){
				othIsCity = true;
			}
			if(oth->getEntitySubtype().getName() == Symbol(L"Nation")){
				othIsCountry = true;
			}
		}
	}
	if(currIsCity && othIsCountry)
		return true;
	if(currIsCountry && othIsCity)
		return true;
	return false;
}
bool SimpleRuleNameLinker::isMetonymyLinkCase(Mention *currMention, EntityType linkType, EntityGuess *entGuess) {
	//mrf 2007: don't add metonymy links for distillation
	//if(_distillationMode) return false;	
	if (!linkType.matchesGPE() || !currMention->hasRoleType() || !currMention->getRoleType().matchesORG())
		return false;

	// delete all previous mentions
	clearMentions();
	// load all the name mentions in the entity into a hashtable
	loadNameMentions(_currSolution->getEntity(entGuess->id));

	Symbol currTokens[maxTokenCount];
	int nTokens = currMention->getHead()->getTerminalSymbols(currTokens, maxTokenCount);

	// check for capital city name match
	Symbol country = getValueFromTable(_capitals, currTokens);
	if (!country.is_null()) {
		Symbol *countryTokens = getSymbolArray(country.to_string());
		if (!getValueFromTable(_mentions, countryTokens).is_null()) {
			_debug << "SimpleRuleNameLinker: Found metonymy link case: mention ID " << currMention->getUID() << " with Entity ID " << entGuess->id << "\n";
			delete[] countryTokens;
			return true;
		}
		delete[] countryTokens;
	}

	return false;
}

void SimpleRuleNameLinker::clearMentions() {
	HashTable::iterator iter;
	for (iter = _mentions->begin(); iter != _mentions->end(); ++iter) {
		delete [] (*iter).first;
	}
	_mentions->clear();
	for (iter = _normalizedMentions->begin(); iter != _normalizedMentions->end(); ++iter) {
		delete [] (*iter).first;
	}
	_normalizedMentions->clear();

}

void SimpleRuleNameLinker::loadNameMentions(Entity * entity) {
	_nNameMentions = 0;
	for (int i = 0; i < entity->getNMentions(); i++) {
		Mention * ment = _currSolution->getMention(entity->getMention(i));
		if (ment->getMentionType() == Mention::NAME) {
			Symbol * mentTokens = _new Symbol[maxTokenCount];
			int nMentTokens = ment->getHead()->getTerminalSymbols(mentTokens, maxTokenCount);
			if (getValueFromTable(_mentions, mentTokens).is_null()) {
				(*_mentions)[mentTokens] = Symbol(L"1");
				_nNameMentions++;
				Symbol * normMentTokens = _new Symbol[maxTokenCount];
				int l_norm = 0;
				for(int j = 0; j <  nMentTokens; j++ ){
					Symbol n = CorefUtilities::getNormalizedSymbol(mentTokens[j]);
					if(n != Symbol(L"") && n != Symbol(L" ")){
						normMentTokens[l_norm++] = n;
					}
				}
				if(l_norm > 0 && getValueFromTable(_normalizedMentions, normMentTokens).is_null()){
					(*_normalizedMentions)[normMentTokens] = Symbol(L"1");
				} else {
					delete [] normMentTokens;
				}
			} else {
				delete [] mentTokens;
			}


			mentTokens = _new Symbol[maxTokenCount+1];
			nMentTokens = ment->getHead()->getTerminalSymbols(mentTokens, maxTokenCount);
			mentTokens[nMentTokens] = Symbol();

			// SRS: now we add "alternate" names
			if (entity->getType().matchesGPE() ||
				entity->getType().matchesORG())
			{
				Symbol alternateName = getValueFromTable(_alternateSpellings,
														 mentTokens);
				if (!alternateName.is_null()) {
					Symbol *altTokens = getSymbolArray(alternateName.to_string());
					if (getValueFromTable(_mentions, altTokens).is_null()) {
						(*_mentions)[altTokens] = Symbol(L"1");
					} else
						delete[] altTokens;
				}
			}
			delete[] mentTokens;
		}
	}
}

Symbol SimpleRuleNameLinker::getValueFromTable(HashTable *table, Symbol * key) {
	HashTable::iterator iter;
	iter = table->find(key);
	if (iter == table->end()) {
		return Symbol();
	}
	return (*iter).second;
}


bool SimpleRuleNameLinker::isOKToLinkPER(Mention * currMention, EntityGuess * entGuess) {
	if(ParamReader::getRequiredTrueFalseParam("simple_rule_name_link_conservative")){
		if(currMention->getMentionType() != Mention::NAME)
			return true;
	}
	Symbol currTokens[maxTokenCount];
	Symbol currTokensNormalized[maxTokenCount];
	int nTokens = currMention->getHead()->getTerminalSymbols(currTokens, maxTokenCount); //mrf, terminal symbols will be lower case
	std::wstring curr_ment_string = L"";
	for(int i = 0; i<nTokens; i++){
		curr_ment_string = curr_ment_string + std::wstring(currTokens[i].to_string()) + L" ";
	}
	int nTokensNorm = 0;
	for(int j = 0; j <  nTokens; j++ ){
		Symbol n = CorefUtilities::getNormalizedSymbol(currTokens[j]);
		if(n != Symbol(L"") && n != Symbol(L" ")){
			currTokensNormalized[nTokensNorm++] = n;
		}
	}

	// Check for a suffix and get the token before it.
	Symbol currSuffix = Symbol(L"NONE");
	bool hasKnownSuffix = false;
	/*********************************************************/
	/*  CONDITION: Word before a suffix MUST be appear  in   */
	/*             _normalizedMentions (the Entity)          */
	/*                                                       */
	/*********************************************************/
	if (nTokensNorm > 1) {
		Symbol * suffix = getSymbolArray(currTokensNormalized[nTokensNorm - 1].to_string());
		currSuffix = suffix[0];
		std::wstring preSuffixWord = L"-NONE-";
		if( !getValueFromTable(_suffixes, suffix).is_null()){
			hasKnownSuffix = true;
			preSuffixWord = currTokensNormalized[nTokensNorm - 2].to_string();
		}
		if (hasKnownSuffix && preSuffixWord != L"-NONE-" && !hasTokenMatch(preSuffixWord.c_str(), true))
		{
			delete[] suffix;
			return false;
		}
		delete [] suffix;
	}
	if(_distillationMode){
		//mrf: 2011 reworked these heuristics to account for edit distance and world knowledge
		//    The filter is still too strict for: initials (e.g. G. Bush --> George Bush),
		//    nicknames (e.g. Judy --> Judith), and syntax that gives alias 
		//    (e.g. Bob Smith, who was born as Joe Smith).
		//    Despite this, _distillationMode is an overall when for preventing overlinking between family
		//    members

		//Symbols in _mentions and currTokens are lower cased because they come from the parse terminals

		/**********************************************************/
		/*  ALLOW: Entity does not have a name mention, let model */
		/*         try to link.  (It probably won't.)             */
		/**********************************************************/
		if(_mentions->begin() == _mentions->end()){
			//the model is unlikely to link these, but let it try
			return true;
		}

		bool suffix_clash = false;
		/**********************************************************/
		/*  PREVENT:   If Entity and Mention both have suffixes,  */
		/*             they must MATCH                            */
		/**********************************************************/
		for (SimpleRuleNameLinker::HashTable::iterator iter = _mentions->begin(); iter != _mentions->end(); ++iter) {
			Symbol * mentTokens = (*iter).first;
			int counter = 0;
			while ((counter < maxTokenCount) && (!mentTokens[counter].is_null())) {
				counter++;				
			}
			if(hasKnownSuffix){
				if (counter > 1) {					
					Symbol * suffix = getSymbolArray(mentTokens[counter - 1].to_string());
					if(!getValueFromTable(_suffixes, suffix).is_null() && CorefUtilities::getNormalizedSymbol(suffix[0]) != currSuffix){
						suffix_clash = true;
					}			
				}	
			}
		}
		if(suffix_clash){
			return false; //e.g. JR != SR
		}

		/***********************************************************/
		/*  ALLOW: If Entity and Mention are both in WK dictionary */
		/*             and match, allow link                       */
		/***********************************************************/
		int currMentionWK = CorefUtilities::lookUpWKName(currMention);
		int othWK = -100;
		if(currMentionWK != 0 && currMentionWK != -1){
			const Entity* oth_entity = _currSolution->getEntity(entGuess->id);
			for (int i = 0; i < oth_entity->getNMentions(); i++) {
				Mention * oth = _currSolution->getMention(oth_entity->getMention(i));
				othWK = CorefUtilities::lookUpWKName(oth);
				if(othWK == currMentionWK){
					break;
				}
			}
		}
		if(othWK == currMentionWK){
			return true;	//dictionary says these are the same
		}

		/***********************************************************/
		/*  ALLOW: If Entity or Mention are a part of the other    */
		/*         allow link                                      */
		/***********************************************************/
		bool part_of_longest = isPartOfLongestNormalized(currTokensNormalized, nTokensNorm);
		if(part_of_longest){
			return true;	
		}

		/***********************************************************/
		/*  ALLOW: If the full string of a mention in Entity has   */
		/*         low edit distance with the full string of       */
		/*         mention allow (normalized)                      */
		/***********************************************************/
		float minMentionEditDistance = 1000;
		std::wstring currStr = L"";
		for(int i = 0; i < nTokensNorm; i++){
			if(i != 0){
				currStr +=L" ";
			}
			currStr += currTokensNormalized[i].to_string();
		}
		for (SimpleRuleNameLinker::HashTable::iterator iter = _normalizedMentions->begin(); iter != _normalizedMentions->end(); ++iter) {
			std::wstring mentStr = L"";
			Symbol * mentTokens = (*iter).first;
			int counter = 0;
			float minPerNameToken = 1000;
			while ((counter < maxTokenCount) && (!mentTokens[counter].is_null())){
				if(counter != 0)
					mentStr +=L" ";
				mentStr += mentTokens[counter].to_string();
				counter++;
			}
			int e = CorefUtilities::editDistance(mentStr, currStr);
			float d = (float)e/min(mentStr.length(), currStr.length());
			if(d < minMentionEditDistance){
				minMentionEditDistance = d;
			}
		}
		if (minMentionEditDistance < _edit_distance_threshold){
			return true;
		} 
		return false;
	}
	else{
		// Find the last token in the current mention
		if (!hasTokenMatch(currTokens[nTokens - 1].to_string()))
			return false;
	}
	// Check whether the currMention is part of another mention or vice versa
	//mrf 4-2007, This incorrectly prevents links if we are dealing with initials George Bush - > G Bush
	//but don't fix for now
	return isPartOf(currTokens, nTokens);

}
bool SimpleRuleNameLinker::isWKAllowable(Mention * currMention, EntityGuess * entGuess) {
	bool has_name = false;
	int currMentionWK = CorefUtilities::lookUpWKName(currMention);
	if(currMentionWK == 0){
		return true; //unknown
	}
	const Entity* oth_entity = _currSolution->getEntity(entGuess->id);
	bool mention_match = false;
	for (int i = 0; i < oth_entity->getNMentions(); i++) {
		Mention * oth = _currSolution->getMention(oth_entity->getMention(i));
		if(oth->getMentionType() == Mention::NAME){
			has_name = true;
			if(!mention_match){
				mention_match = CorefUtilities::symbolArrayMatch(oth, currMention);	
			}
		}
	}
	if(!has_name){
		return true;
	}
	if(mention_match){
		return true;
	}
	int othWK = -100;
	bool has_singleton = false;
	std::wstring singleton = L"";
	std::wstring group = L"";
	for (int i = 0; i < oth_entity->getNMentions(); i++) {
		Mention * oth = _currSolution->getMention(oth_entity->getMention(i));
		if(oth->getMentionType() == Mention::NAME){
			othWK = CorefUtilities::lookUpWKName(oth);
			if(othWK == -1){
				singleton = oth->getHead()->toTextString();
				has_singleton = true;
			}
			if(othWK != 0 && othWK != -1){
				group = oth->getHead()->toTextString();
				break;
			}
		}
	}
	if(has_singleton && currMentionWK == -1 && !mention_match){		
		return false;	//both singletons, not matching
	}
	else if(has_singleton && currMentionWK != 0 && !mention_match){
		return false;
	}
	else if((othWK != 0) && (currMentionWK != 0) && (othWK != currMentionWK)){
		return false;	//dictionary says these are different
	} 
	return true;	
}
bool SimpleRuleNameLinker::isOKToLinkORG(Mention * currMention, EntityGuess * entGuess) {
	if(_distillationMode){
		/***********************************************************/
		/*  ALLOW: If Entity and Mention are both in WK dictionary */
		/*             and match, allow link                       */
		/***********************************************************/
		if(ParamReader::getRequiredTrueFalseParam("simple_rule_name_link_conservative")){
			if(currMention->getMentionType() != Mention::NAME)
				return true;
			if(CorefUtilities::hasTeamClash(currMention, _currSolution->getEntity(entGuess->id), _currSolution))
				return false;
			if(isWKAllowable(currMention, entGuess))
				return true;
			if(CorefUtilities::isAcronym(currMention, _currSolution->getEntity(entGuess->id), _currSolution)){
				SessionLogger::info("allow_acr_0") <<"Allow Acronymy: "<<currMention->getHead()->toTextString()<<std::endl;
				return true;
			}
			return false;
		}
		else{
			return true;
		}
	}
	else{	//mrf 4-2007, this seems to lower ACE preformance use it with caution
		Symbol currTokens[maxTokenCount];
		int nTokens = currMention->getHead()->getTerminalSymbols(currTokens, maxTokenCount);
		// see if the full name matches
		if (!getValueFromTable(_mentions, currTokens).is_null())
			return true;
		// check for alternative name match
		Symbol alternateName = getValueFromTable(_alternateSpellings, currTokens);
		if (!alternateName.is_null()) {
			Symbol * altTokens = getSymbolArray(alternateName.to_string());
			if (!getValueFromTable(_mentions, altTokens).is_null()) {
				delete [] altTokens;
				return true;
			}
			delete [] altTokens;
		}
		// check for corporate designators
		for (int i = 0; i < nTokens; i++) {
			Symbol designator[maxTokenCount];
			for (int j = 0; j <=i; j++) {
				designator[j] = currTokens[nTokens - 1 - i + j]; // get last tokens from the mention name
				if (!getValueFromTable(_designators, designator).is_null()) {
					if ((nTokens - i - 2) >= 0)
						return hasTokenMatch(currTokens[nTokens - i - 2].to_string());
					else
						return false;
				}
			}
		}
		// find the last token
		return hasTokenMatch(currTokens[nTokens - 1].to_string());
	}
}

bool SimpleRuleNameLinker::isOKToLinkGPE(Mention * currMention, EntityGuess * entGuess) {
	if(_distillationMode){
		if(ParamReader::getRequiredTrueFalseParam("simple_rule_name_link_conservative")){
			if(currMention->getMentionType() != Mention::NAME)
				return true;
			bool wk =  isWKAllowable(currMention, entGuess);
			if(wk)
				return true;			
			if(!wk){
				const Entity* oth_entity = _currSolution->getEntity(entGuess->id);
				std::wstring curr_str = currMention->getHead()->toTextString();
				for (int i = 0; i < oth_entity->getNMentions(); i++) {
					Mention * oth = _currSolution->getMention(oth_entity->getMention(i));
					if(oth->getMentionType() == Mention::NAME){
						std::wstring oth_str = oth->getHead()->toTextString();
						if(std::abs((int)(curr_str.size() - oth_str.size())) < 4){
							if(oth_str.find(curr_str) != std::wstring::npos)
								return true;
							if(curr_str.find(oth_str) != std::wstring::npos)
								return true;
						}
					}
				}
				return false;
			}			
		}
		else{
			return true;
		}
		return true;
	}
	else{
		//mrf: 4-2007, this code lowers ACE scores, and seems to do the wrong thing in most places.
		//the _alternateSpellings table does not correctly account for nationalites (Iraq -> Iraqi)
		Symbol currTokens[maxTokenCount];
		int nTokens = currMention->getHead()->getTerminalSymbols(currTokens, maxTokenCount);
		// see if the full name matches
		if (!getValueFromTable(_mentions, currTokens).is_null())
			return true;
		// check for alternative name match
		Symbol alternateName = getValueFromTable(_alternateSpellings, currTokens);
		if (!alternateName.is_null()) {
			Symbol * altTokens = getSymbolArray(alternateName.to_string());
			if (!getValueFromTable(_mentions, altTokens).is_null()) {
				delete [] altTokens;
				return true;
			}
			delete [] altTokens;
		}
		//*** Hard-coded case for America-United States ***/
		if ((nTokens > 1) && ((wcscmp(currTokens[nTokens-1].to_string(), L"america")) ||
			(wcscmp(currTokens[nTokens-1].to_string(), L"american"))))
			return false;

		//***------------------------------------------***/
		for (int i = 0; i < nTokens; i++) {
			// check for the token in the mentions
			if (hasTokenMatch(currTokens[i].to_string()))
				return true;
			// check for the alternate name to the token in mentions
			Symbol * token = getSymbolArray(currTokens[i].to_string());
			Symbol alternateName = getValueFromTable(_alternateSpellings, token);
			delete [] token;
			if (!alternateName.is_null()) {
				if (hasTokenMatch(alternateName.to_string()))
					return true;
			}
		}
		return false;
	}

}

bool SimpleRuleNameLinker::isOKToLinkOTH(Mention * currMention, EntityGuess * entGuess) {
	if(_distillationMode){
		//no rules constraining Other linking for distillation, trust the model
		return true;
	}
	Symbol currTokens[maxTokenCount];
	int nTokens = currMention->getHead()->getTerminalSymbols(currTokens, maxTokenCount);
	return (!getValueFromTable(_mentions, currTokens).is_null());
}

bool SimpleRuleNameLinker::hasTokenMatch(const wchar_t * token, bool normalized) {
	HashTable::iterator iter;
	HashTable* table = _mentions;
	if(normalized){
		table = _normalizedMentions;
	}

	for (iter = table->begin(); iter != table->end(); ++iter) {
		Symbol * mentTokens = (*iter).first;
		int counter = 0;
		while ((counter < maxTokenCount) && (!mentTokens[counter].is_null())) {
			if (wcscmp(mentTokens[counter].to_string(), token) == 0)
				return true;
			counter++;
		}
	}
	return false;
}
/* Only check for matches in multiple word names */
bool SimpleRuleNameLinker::hasTokenMatchInMultWords(const wchar_t * token) {
	HashTable::iterator iter;
	int max_tokens = 0;

	for (iter = _mentions->begin(); iter != _mentions->end(); ++iter) {
		Symbol * mentTokens = (*iter).first;
		int counter = 0;
		while ((counter < maxTokenCount) && (!mentTokens[counter].is_null())) {
			if (wcscmp(mentTokens[counter].to_string(), token) == 0)
				return true;
			counter++;
		}
	}
	return false;
}

bool SimpleRuleNameLinker::isPartOfLongestNormalized(Symbol * currTokens, int nTokens){
	HashTable::iterator iter;
	int lml = 0;
	for (iter = _normalizedMentions->begin(); iter != _normalizedMentions->end(); ++iter) {
		Symbol * mentTokens = (*iter).first;
		int mentTokenCount = getTokenCount(mentTokens);
		if(mentTokenCount > lml){
			lml = mentTokenCount;
		}		
	}
	if(lml == nTokens){
		for (iter = _normalizedMentions->begin(); iter != _normalizedMentions->end(); ++iter) {
			Symbol * mentTokens = (*iter).first;
			int mentTokenCount = getTokenCount(mentTokens);
			if(mentTokenCount != lml)
				continue;
			bool all_found = true;
			for (int i = 0; i < nTokens; i++) { // do an exact match comparison
				if (getLowerSymbol(currTokens[i].to_string()) != getLowerSymbol(mentTokens[i].to_string()))
					all_found = false;
			}
			if (all_found)
				return true;
		}
	}
	else if(lml < nTokens){
		for (iter = _normalizedMentions->begin(); iter != _normalizedMentions->end(); ++iter) {
			Symbol * mentTokens = (*iter).first;
			int mentTokenCount = getTokenCount(mentTokens);
			if(mentTokenCount != lml)
				continue;
			bool all_found = true;
			for (int i = 0; i < mentTokenCount; i++) {
				Symbol mentSym = getLowerSymbol(mentTokens[i].to_string());
				bool found = false;
				for (int j = 0; j < nTokens; j++) {
					if (mentSym == getLowerSymbol(currTokens[j].to_string())){
						//std::wcout<<"(1)Match: "<<mentSym.to_string()<<", "<<currTokens[j].to_string()<<std::endl;
						found = true;
					}
				}
				if (!found)
					all_found = false;
			}
			if(all_found)
				return true;
		}
	}
	else if(lml > nTokens){
		for (iter = _normalizedMentions->begin(); iter != _normalizedMentions->end(); ++iter) {
			Symbol * mentTokens = (*iter).first;
			int mentTokenCount = getTokenCount(mentTokens);
			if(mentTokenCount != lml)
				continue;
			bool all_found = true;
			for (int i = 0; i < nTokens; i++) {
				Symbol currSym = getLowerSymbol(currTokens[i].to_string());
				bool found = false;
				for (int j = 0; j < mentTokenCount; j++) {
					if (currSym == getLowerSymbol(mentTokens[j].to_string())){
						//std::wcout<<"(2)Match: "<<currSym.to_string()<<", "<<mentTokens[j].to_string()<<std::en
						found = true;
					}
				}
				if(!found)
					all_found = false;
			}
			if(all_found)
				return true;
		}			
	}
	return false;
}
bool SimpleRuleNameLinker::isPartOf(Symbol * currTokens, int nTokens) {
	HashTable::iterator iter;
	for (iter = _mentions->begin(); iter != _mentions->end(); ++iter) {
		Symbol * mentTokens = (*iter).first;
		int mentTokenCount = getTokenCount(mentTokens);
		if (mentTokenCount < 2) // a check for a last name has already been done
			continue;
		if (getLowerSymbol(mentTokens[0].to_string()) == getLowerSymbol(currTokens[0].to_string())) // if first names match
			continue;
		// check whether the mention is contained in another mention or vice versa
		if (nTokens == mentTokenCount) {
			for (int i = 0; i < nTokens; i++) { // do an exact match comparison
				if (getLowerSymbol(currTokens[i].to_string()) != getLowerSymbol(mentTokens[i].to_string()))
					return false;
			}
		} else {
			if (mentTokenCount < nTokens) { // find each of the tokens in currTokens
				for (int i = 0; i < mentTokenCount; i++) {
					Symbol mentSym = getLowerSymbol(mentTokens[i].to_string());
					bool found = false;
					for (int j = 0; j < nTokens; j++) {
						if (mentSym == getLowerSymbol(currTokens[j].to_string()))
							found = true;
					}
					if (!found)
						return false;
				}
			} else {
				for (int i = 0; i < nTokens; i++) {
					Symbol currSym = getLowerSymbol(currTokens[i].to_string());
					bool found = false;
					for (int j = 0; j < mentTokenCount; j++) {
						if (currSym == getLowerSymbol(mentTokens[j].to_string()))
							found = true;
					}
					if (!found)
						return false;
				}
			}
		}
	}
	return true;
}

Symbol SimpleRuleNameLinker::getLowerSymbol(const wchar_t * str) {
	std::wstring buffer(str);
	std::transform(buffer.begin(), buffer.end(), buffer.begin(), towlower);
	return Symbol(buffer.c_str());
}


