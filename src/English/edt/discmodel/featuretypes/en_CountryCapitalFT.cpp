// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/AbbrevTable.h"
#include "Generic/edt/NameLinkFunctions.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntitySet.h"

#include "English/edt/discmodel/featuretypes/en_CountryCapitalFT.h"
#include <boost/scoped_ptr.hpp>



int EnglishEn_CountryCapitalFT::extractFeatures(const DTState &state,
							DTFeature **resultArray) const
{
	DTCorefObservation *o = static_cast<DTCorefObservation*>(
		state.getObservation(0));
	const Mention *mention = o->getMention();
	EntityType linkType = mention->getEntityType();
	if(!linkType.matchesGPE())
		return 0;

	const EntitySet *eset = o->getEntitySet();
	Entity *entity = o->getEntity();

	const MentionSymArrayMap *HWMap = o->getHWMentionMapper();
	const MentionSymArrayMap *abbrevMap = o->getAbbrevMentionMapper();
	SymbolArray *mentionAbbrevArray = *abbrevMap->get(mention->getUID());
	SymbolArray *mentionHWArray = *HWMap->get(mention->getUID());

	Symbol mentionRole = SymbolConstants::nullSymbol;
	Symbol entityMentionLevel = o->getEntityMentionLevel();

	int n_feat = 0;
	bool discard = false;

	SymbolArray *country = NULL;
	Symbol countryWords[MAX_NUM_WORDS_IN_MENTION_HEAD];
	Symbol resolvedWords[MAX_NUM_WORDS_IN_MENTION_HEAD];
	Symbol entityCapitalWords[MAX_NUM_WORDS_IN_MENTION_HEAD];

	bool mentCapitalSameAsCountry = false;
	int nCountryWords, nResolvedWords, nEntityCapitalWords;;//, nEntityCountryWords
	
	country = lookupPhrase(mentionHWArray, capital2country);
	if(country!=NULL){
		nResolvedWords  = AbbrevTable::resolveSymbols(country->getArray(), country->getLength(), resolvedWords, MAX_NUM_WORDS_IN_MENTION_HEAD);
		nCountryWords = NameLinkFunctions::getLexicalItems(resolvedWords, nResolvedWords, countryWords, MAX_NUM_WORDS_IN_MENTION_HEAD);

		//// check whether the capital and the country are the same
		if(nCountryWords == mentionHWArray->getLength())	{
			bool is_equal = true;
			const Symbol *hws = mentionHWArray->getArray();
			for(int i=0;i<country->getLength(); i++)
				if(countryWords[i] != hws[i]) is_equal=false;
			mentCapitalSameAsCountry = is_equal;
		}

		if (mention->hasRoleType()) // set in the metonymy model
		{
			mentionRole = mention->getRoleType().getName();
		}
	}

	// if the mention is a capital and the capital name is the same as the country name don't fire.
	if(mentCapitalSameAsCountry)
		return 0;


	bool city2countryFound = false;
	bool country2cityFound = false;
	for (int m = 0; m < entity->getNMentions(); m++) {

		if(n_feat>MAX_FEATURES_PER_EXTRACTION-10)
			return n_feat;

		Mention* entMent = o->getEntitySet()->getMention(entity->getMention(m));
		// the mention is a capital city that can be mapped to a country
		if (country!=NULL){ // the mention is a capital city
			SymbolArray *entityAbbrevArray = *abbrevMap->get(entity->getMention(m));
			////check equality
			if (nCountryWords == entityAbbrevArray->getLength()){
				const Symbol * entitySyms = entityAbbrevArray->getArray();
				bool is_equal = true;
				for(int i=0;i<country->getLength(); i++)
					if(countryWords[i] != entitySyms[i]) is_equal=false;
				if(is_equal) {
					city2countryFound = true;
					if (n_feat >= (DTFeatureType::MAX_FEATURES_PER_EXTRACTION-2)){
								discard = true;
								break;
					}
					//std::cerr<<"Debug EnglishEn_CountryCapitalFT slotA adding nfeat "<<n_feat<<"\n";
					resultArray[n_feat++] = _new DTQuintgramFeature(this, state.getTag(), PER_MENTION, CITI_TO_COUNTRY, mentionRole, entityMentionLevel);
				}
			}
		}else { // The mention is not a capital city but may be a country.
				// Check whether there is a match when we map the entity as a capital to a country and compare
			SymbolArray *entityHWArray = *HWMap->get(entity->getMention(m));
			SymbolArray *entityCapital = lookupPhrase(entityHWArray, country2capital);
			if (entityCapital!=NULL && !mentCapitalSameAsCountry){
				nResolvedWords  = AbbrevTable::resolveSymbols(entityCapital->getArray(), entityCapital->getLength(), resolvedWords, MAX_NUM_WORDS_IN_MENTION_HEAD);
				nEntityCapitalWords = NameLinkFunctions::getLexicalItems(resolvedWords, nResolvedWords, entityCapitalWords, MAX_NUM_WORDS_IN_MENTION_HEAD);
				////check equality
				if (mentionAbbrevArray->getLength() == nEntityCapitalWords){
					const Symbol *mentWords = mentionAbbrevArray->getArray();
					bool is_equal = true;
					for(int i=0;i<entityCapital->getLength(); i++)
						if(mentWords[i] != entityCapitalWords[i]) is_equal=false;
					if(is_equal) {
						Symbol entityCapitalRole = (mention->hasRoleType()) ? mention->getRoleType().getName() : SymbolConstants::nullSymbol;
						country2cityFound = true;
						if (n_feat >= (DTFeatureType::MAX_FEATURES_PER_EXTRACTION-2)){
								discard = true;
								break;
						}

						//std::cerr<<"Debug EnglishEn_CountryCapitalFT slotB adding nfeat "<<n_feat<<"\n";

						resultArray[n_feat++] = _new DTQuintgramFeature(this, state.getTag(), PER_MENTION, COUNTRY_TO_CITI, entityCapitalRole, entityMentionLevel);
					}
				}
			}
		}//else
	}//for


					
							
			
	if (city2countryFound){
		if (n_feat >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION){
			discard = true;	
		}else{
			resultArray[n_feat++] = _new DTQuintgramFeature(this, state.getTag(), PER_ENTITY, CITI_TO_COUNTRY, mentionRole, entityMentionLevel);
		}
	}
	if (country2cityFound){
		if (n_feat >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION){
			discard = true;	
		}else{
			resultArray[n_feat++] = _new DTQuintgramFeature(this, state.getTag(), PER_ENTITY, COUNTRY_TO_CITI, SymbolConstants::nullSymbol, entityMentionLevel);
		}
	}
	if (discard) {
		SessionLogger::warn("DT_feature_limit") <<"EnglishEn_CountryCapitalFT discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
		//std::cerr<<"Debug EnglishEn_CountryCapitalFT discarded some kept nfeat "<<n_feat<<"\n";
	}
	//if (n_feat > 0) std::cerr<<"Debug EnglishEn_CountryCapitalFT reached nfeat "<<n_feat<<"\n";
	return n_feat;
}


UTF8Token EnglishEn_CountryCapitalFT::FileReader::getToken(int specifier) {
	UTF8Token token;
	if(_nTokenCache > 0)
		token = _tokenCache[--_nTokenCache];
	else _file >> token;

	if(token.symValue() == SymbolConstants::leftParen) {
		if(!(specifier & LPAREN))
			throw UnexpectedInputException("EnglishEn_CountryCapitalFT::FileReader::getToken()", "\'(\' not expected in edt_capital_to_country_file.");
	} 
	else if(token.symValue() == SymbolConstants::rightParen) {
		if(!(specifier & RPAREN))
			throw UnexpectedInputException("EnglishEn_CountryCapitalFT::FileReader::getToken()", "\')\' not expected in edt_capital_to_country_file.");
	}
	else if(token.symValue() == Symbol(L"")) {
		if(!(specifier & EOFTOKEN))
			throw UnexpectedInputException("EnglishEn_CountryCapitalFT::FileReader::getToken()", "EOF not expected in edt_capital_to_country_file.");
	}
	else if(!(specifier & WORD))
		throw UnexpectedInputException("EnglishEn_CountryCapitalFT::FileReader::getToken()", "Word not expected in edt_capital_to_country_file.");

	return token;
}
	
bool EnglishEn_CountryCapitalFT::FileReader::hasMoreTokens()
{
	if(_nTokenCache > 0) return true;
	UTF8Token token;
	_file >> token;
	if(token.symValue() == Symbol(L""))
		return false;
	else {
		if(_nTokenCache < MAX_CACHE)
			_tokenCache[_nTokenCache++] = token;
		return true;
	}
}

int EnglishEn_CountryCapitalFT::FileReader::getSymbolArray(Symbol results[], int max_results) throw(UnexpectedInputException)
{
	UTF8Token token = getToken(LPAREN | WORD);
	int nResults = 0;
	if(token.symValue() == SymbolConstants::leftParen)
		while((token = getToken(RPAREN | WORD)).symValue() != SymbolConstants::rightParen && nResults<max_results)
			results[nResults++] = token.symValue();
	else results[nResults++] = token.symValue();
	return nResults;
}

int EnglishEn_CountryCapitalFT::FileReader::getOptionalSymbolArray(Symbol results[], int max_results) throw(UnexpectedInputException)
{
	UTF8Token token = getToken(LPAREN | WORD | RPAREN);
	int nResults = 0;
	if(token.symValue() == SymbolConstants::rightParen) 
		pushBack(token);
	else if(token.symValue() == SymbolConstants::leftParen)
		while((token = getToken(RPAREN | WORD)).symValue() != SymbolConstants::rightParen && nResults<max_results)
			results[nResults++] = token.symValue();
	else results[nResults++] = token.symValue();
	return nResults;
}

void EnglishEn_CountryCapitalFT::FileReader::pushBack(UTF8Token token) {
	if(_nTokenCache < MAX_CACHE)
		_tokenCache[_nTokenCache++] = token;
}

/** while countries we abbreviate and put into the table, capital cities we do not abbreviate
* the reason is that 'washington d.c.' or 'wash.' are usually not metonymic.
* in order to add abbreviation for a capital to country map you can add an entity to the file.
* While this will add an entry for the map from capitals to countries, the country to capital map 
* will only hold the last entered value.
*/
void EnglishEn_CountryCapitalFT::initializeTable() {
	std::string filename = ParamReader::getRequiredParam("edt_capital_to_country_file");
	boost::scoped_ptr<UTF8InputStream> instream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& instream(*instream_scoped_ptr);
	instream.open(filename.c_str());
	FileReader reader(instream);

	//initialize hash maps
	_table[country2capital] = _new SymbolArraySymbolArrayMap(300);
	_table[capital2country] = _new SymbolArraySymbolArrayMap(300);

	const int MAX_ARRAY_SYMBOLS = 16;
	Symbol array[MAX_ARRAY_SYMBOLS], keyArray[MAX_ARRAY_SYMBOLS], valueArray[MAX_ARRAY_SYMBOLS];
	Symbol resolvedWords[MAX_ARRAY_SYMBOLS];
	int nResolvedWords, nArray = 0, nKeyArray = 0, nValueArray = 0;

	_debugOut.init(Symbol(L"country_capital_debug_file"));
	while(reader.hasMoreTokens()) {
		reader.getLeftParen();
		
		nArray = reader.getSymbolArray(array, MAX_ARRAY_SYMBOLS);
		nResolvedWords  = AbbrevTable::resolveSymbols(array, nArray, resolvedWords, MAX_ARRAY_SYMBOLS);
		int nValueArray = NameLinkFunctions::getLexicalItems(resolvedWords, nResolvedWords, valueArray, MAX_ARRAY_SYMBOLS);
		
		_debugOut << "found VALUE: ";
		for(int i=0; i<nValueArray; i++) 
			_debugOut << valueArray[i].to_debug_string() << " ";
		_debugOut << "\n";
		
		nArray = reader.getSymbolArray(array, MAX_SYMBOLS);

		_debugOut << "      KEY: ";
		for(int j=0; j<nArray; j++) 
			_debugOut << array[j].to_debug_string() << " ";
		_debugOut << "\n";

		add(array, nArray, valueArray, nValueArray, capital2country);
		add(valueArray, nValueArray, keyArray, nKeyArray, country2capital);


		while((nArray = reader.getOptionalSymbolArray(array, MAX_SYMBOLS))!= 0) {
			_debugOut << "      KEY: ";
			for(int i=0; i<nArray; i++) 
				_debugOut << array[i].to_debug_string() << " ";
			_debugOut << "\n";
			
			add(array, nArray, valueArray, nValueArray, capital2country);
			add(valueArray, nValueArray, keyArray, nKeyArray, country2capital);
		}
		reader.getRightParen();
	}
}


void EnglishEn_CountryCapitalFT::add(Symbol *key, size_t nKey, Symbol *value, size_t nValue, size_t table_index) {

	SymbolArray *keyArray = _new SymbolArray(key, nKey);
	SymbolArray *valueArray = _new SymbolArray(value, nValue);

	if((*_table[table_index]).get(keyArray) == NULL) {
		(*_table[table_index])[keyArray] = valueArray;
	}
	else {
		SymbolArray *oldValue = *(*_table[table_index]).get(keyArray);
		(*_table[table_index])[keyArray] = valueArray;
		delete oldValue;
		delete keyArray;
	}
}

SymbolArray* EnglishEn_CountryCapitalFT::lookupPhrase(SymbolArray *key, size_t index) const{
	SymbolArray **value = (*_table[index]).get(key);
	if(value != NULL) {
		return *value;
	}
	return NULL;
}

