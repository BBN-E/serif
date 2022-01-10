// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/CorefUtilities.h"
#include "Generic/common/SymbolArray.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/WordConstants.h"

#include <boost/algorithm/string.hpp>  
#include <boost/algorithm/string/trim.hpp>
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#include <boost/regex.hpp>
#pragma warning(pop)
#include <boost/scoped_ptr.hpp>

#include <set>

const size_t CorefUtilities::MAX_MENTION_NAME_SYMS_PLUS = 20;
bool CorefUtilities::_use_gpe_world_knowledge_feature = false;
bool CorefUtilities::_wk_initialized = false;
Symbol CorefUtilities::GPE = Symbol(L"GPE");
Symbol CorefUtilities::ORG = Symbol(L"ORG");
Symbol CorefUtilities::PER = Symbol(L"PER");
SymbolToIntMap CorefUtilities::team_map;
WKMap CorefUtilities::_wkMap; 

// There should be 32 (= 8 rows of 4) teams in the following array.
const std::wstring TEAMS[] = {
L"49ers", L"bears", L"bengals", L"bills", 
L"broncos", L"browns", L"buccaneers", L"cardinals", 
L"chargers", L"chiefs", L"colts", L"cowboys",
L"dolphins", L"eagles", L"falcons", L"giants", 
L"jaguars", L"jets", L"lions", L"packers", 
L"panthers", L"patriots", L"raiders", L"rams", 
L"ravens", L"redskins", L"saints", L"seahawks", 
L"steelers", L"titans", L"texans", L"vikings"};

const int TEAM_COUNT = sizeof(TEAMS)/sizeof(std::wstring);

Symbol CorefUtilities::TELEPHONE_SOURCE_SYM = Symbol(L"telephone");
Symbol CorefUtilities::BROADCAST_CONV_SOURCE_SYM = Symbol(L"broadcast conversation");
Symbol CorefUtilities::USENET_SOURCE_SYM = Symbol(L"usenet");
Symbol CorefUtilities::WEBLOG_SOURCE_SYM = Symbol(L"weblog");
Symbol CorefUtilities::EMAIL_SOURCE_SYM = Symbol(L"email");
Symbol CorefUtilities::DF_SYM = Symbol(L"discussion_forum");

void WKMap::addGpe(SymbolArray* sa, int id) {
	_wkGpe[sa] = id;
	if ((*_wkGpe.find(sa)).first != sa)
		delete sa;
}
void WKMap::addGpeSingleton(SymbolArray* sa) {
	_wkGpeSingletons.insert(sa);
	if (*_wkGpeSingletons.find(sa) != sa)
		delete sa;
}
int WKMap::getGpeId(SymbolArray *sa) {
	int* value = _wkGpe.get(sa);
	return value ? *value : 0;
}
bool WKMap::isSingletonGpe(SymbolArray *sa) {
	return _wkGpeSingletons.exists(sa);
}
void WKMap::addOrg(SymbolArray* sa, int id) {
	_wkOrg[sa] = id;
	if ((*_wkOrg.find(sa)).first != sa)
		delete sa;
}
void WKMap::addOrgSingleton(SymbolArray* sa) {
	_wkOrgSingletons.insert(sa);
	if (*_wkOrgSingletons.find(sa) != sa)
		delete sa;
}
int WKMap::getOrgId(SymbolArray *sa) {
	int* value = _wkOrg.get(sa);
	return value ? *value : 0;
}
bool WKMap::isSingletonOrg(SymbolArray *sa) {
	return _wkOrgSingletons.exists(sa);
}
void WKMap::addPer(SymbolArray* sa, int id) {
	_wkPer[sa] = id;
	if ((*_wkPer.find(sa)).first != sa)
		delete sa;
}
void WKMap::addPerSingleton(SymbolArray* sa) {
	_wkPerSingletons.insert(sa);
	if (*_wkPerSingletons.find(sa) != sa)
		delete sa;
}
int WKMap::getPerId(SymbolArray *sa) {
	int* value = _wkPer.get(sa);
	return value ? *value : 0;
}
bool WKMap::isSingletonPer(SymbolArray *sa) {
	return _wkPerSingletons.exists(sa);
}
WKMap::WKMap(): _wkGpe(), _wkOrg(), _wkPer(), _wkGpeSingletons(100), _wkOrgSingletons(100), _wkPerSingletons(100) {}
WKMap::~WKMap() {
	for (SymbolArrayIntegerMap::iterator iter=_wkGpe.begin(); iter!=_wkGpe.end(); ++iter)
		delete (*iter).first;
	for (SymbolArraySet::iterator iter=_wkGpeSingletons.begin(); iter!=_wkGpeSingletons.end(); ++iter)
		delete *iter;
	for (SymbolArrayIntegerMap::iterator iter=_wkOrg.begin(); iter!=_wkOrg.end(); ++iter)
		delete (*iter).first;
	for (SymbolArraySet::iterator iter=_wkOrgSingletons.begin(); iter!=_wkOrgSingletons.end(); ++iter)
		delete *iter;
	for (SymbolArrayIntegerMap::iterator iter=_wkPer.begin(); iter!=_wkPer.end(); ++iter)
		delete (*iter).first;
	for (SymbolArraySet::iterator iter=_wkPerSingletons.begin(); iter!=_wkPerSingletons.end(); ++iter)
		delete *iter;
}

void CorefUtilities::readWKOrderedHash(UTF8InputStream& uis, Symbol type){
	UTF8Token tok;
	int lineno = 0;
	Symbol toksyms[MAX_MENTION_NAME_SYMS_PLUS];
	Symbol discardToks[MAX_MENTION_NAME_SYMS_PLUS * 2];
	// Note that last{Gpe,Org,Per}Single are set before they are checked, but it's
	// better to initialize them to ensure that this remains the case.
	SymbolArray * lastGpe = 0;
	int lastGpeId = -1;
	bool lastGpeSingle(true);
	SymbolArray * lastOrg = 0;
	int lastOrgId = -1;
	bool lastOrgSingle(true);
	SymbolArray * lastPer = 0;
	int lastPerId = -1;
	bool lastPerSingle(true);
	int singles = 0;
	int multiples = 0;
	int truncatedNames = 0;
	bool reportLongNames = false;
	while(!uis.eof()){
		uis >> tok;	// this should be the opening '('
		if(uis.eof()){
			break;
		}
		uis >> tok;	
		int ntoks = 0;
		int ndtoks = 0;
		while(tok.symValue() != Symbol(L")")){
			if ((size_t)ntoks < MAX_MENTION_NAME_SYMS_PLUS) {
				toksyms[ntoks++] = tok.symValue();
			}else if ((size_t)ndtoks < MAX_MENTION_NAME_SYMS_PLUS * 2){
				discardToks[ndtoks++] = tok.symValue();
				
			}
			uis >> tok;
		}
		if (ndtoks > 0){
			truncatedNames++;
			if (reportLongNames){
				std::ostringstream ostr;
				ostr << "CorefUtilities : load world knowledge hash found over-long name; kept [" ;
				for (int nt = 0; nt < ntoks; nt++){
					ostr << toksyms[nt].to_debug_string() << " ";
				}
				ostr << "]\n" << "\t discarding [";
				for (int nt = 0; nt < ndtoks; nt++){
					ostr << discardToks[nt].to_debug_string() << " ";
				}
				if ((size_t)ndtoks == (MAX_MENTION_NAME_SYMS_PLUS * 2)){
					ostr <<" ...";
				}
				ostr << "]\n";	
				SessionLogger::warn("coref_utilities") << ostr.str();
			}
		}
		uis >> tok;
		int id = _wtoi(tok.chars());
		lineno++;
		SymbolArray * sa = _new SymbolArray(toksyms, ntoks);

		//if((lineno % 50000) == 0){
		//	std::wcout <<" lines: "<< lineno
		//		<<" \t"<< id <<" \t" << type.to_string() << " SA = " ;
		//	for (int nt = 0; nt < ntoks; nt++){
		//		std::wcout << toksyms[nt] << " ";
		//	}
		//	std::wcout <<std::endl;
		//}
		
		if (type == Symbol(L"GPE")){
			if (id == lastGpeId) {
				_wkMap.addGpe(sa, id);
				lastGpeSingle = false;
				multiples++;
			} else {
				if (lastGpeId != -1){
					if (lastGpeSingle){
						_wkMap.addGpeSingleton(lastGpe);
						singles++;
					}else{
						_wkMap.addGpe(lastGpe, lastGpeId);
						multiples++;
					}
				}
				lastGpe = sa;
				lastGpeId = id;
				lastGpeSingle = true;
			}
		} else if (type == Symbol(L"ORG")){
			if (id == lastOrgId) {
				_wkMap.addOrg(sa, id);
				lastOrgSingle = false;
				multiples++;
			} else {
				if (lastOrgId != -1){
					if (lastOrgSingle){
						_wkMap.addOrgSingleton(lastOrg);
						singles++;
					}else{
						_wkMap.addOrg(lastOrg, lastOrgId);
						multiples++;
					}
				}
				lastOrg = sa;
				lastOrgId = id;
				lastOrgSingle = true;
			}
		} else if (type == Symbol(L"PER")) {
			if (id == lastPerId) {
				_wkMap.addPer(sa, id);
				lastPerSingle = false;
				multiples++;
			} else {
				if (lastPerId != -1){
					if (lastPerSingle){
						_wkMap.addPerSingleton(lastPer);
						singles++;
					}else{
						_wkMap.addPer(lastPer, lastPerId);
						multiples++;
					}
				}
				lastPer = sa;
				lastPerId = id;
				lastPerSingle = true;
			}
		} else {
			throw UnrecoverableException("CorefUtilities::readOrderedHash()", "Unrecognized type");	
		}
	}// end of read loop

	// store possible held Per and/or Org
	if (lastGpeId != -1){
		if (lastGpeSingle){
			_wkMap.addGpeSingleton(lastGpe);
			singles++;
		} else {
			_wkMap.addGpe(lastGpe, lastGpeId);
			multiples++;
		}
	}
	if (lastOrgId != -1){
		if (lastOrgSingle){
			_wkMap.addOrgSingleton(lastOrg);
			singles++;
		} else {
			_wkMap.addOrg(lastOrg, lastOrgId);
			multiples++;
		}
	}
	if (lastPerId != -1){
		if (lastPerSingle){
			_wkMap.addPerSingleton(lastPer);
			singles++;
		} else {
			_wkMap.addPer(lastPer, lastPerId);
			multiples++;
		}
	}
	//std::wcout << "readOrderedHash loaded " << singles << " singletons and " 
	//	<< multiples << " linked names of type " << type << std::endl;
	if (reportLongNames && (truncatedNames > 0)) {
		SessionLogger::dbg("coref_utilities") << "CorefUtilities: Truncated " << truncatedNames << " over-long names";
	}
}

void CorefUtilities::initializeWKLists() {
	std::string gpe_file = ParamReader::getRequiredParam("dtnamelink_gpe_wk_file");
	std::string org_file = ParamReader::getRequiredParam("dtnamelink_org_wk_file");
	std::string per_file = ParamReader::getRequiredParam("dtnamelink_per_wk_file");
	initializeWKLists(gpe_file, org_file, per_file);
}

void CorefUtilities::initializeWKLists(const std::string & gpe_file, const std::string & org_file, const std::string & per_file){
	if (!ParamReader::isInitialized()) return;
	if (_wk_initialized) return;
	for (int t = 0; t < TEAM_COUNT; ++t) {
		team_map.insert(std::make_pair<Symbol, int>(Symbol(TEAMS[t]), t));
	}

	SessionLogger::info("gpe_wk_0") << "Loading GPE WK file " << gpe_file << "..." << std::endl;
	boost::scoped_ptr<UTF8InputStream> gpe_uis_scoped_ptr(UTF8InputStream::build(gpe_file.c_str()));
	UTF8InputStream& gpe_uis(*gpe_uis_scoped_ptr);
	readWKOrderedHash(gpe_uis, GPE);

	SessionLogger::info("org_wk_0") << "Loading ORG WK file " << org_file << "..." << std::endl;
	boost::scoped_ptr<UTF8InputStream> org_uis_scoped_ptr(UTF8InputStream::build(org_file.c_str()));
	UTF8InputStream& org_uis(*org_uis_scoped_ptr);
	readWKOrderedHash(org_uis, ORG);

	SessionLogger::info("per_wk_0") << "Loading PER WK file " << per_file << "..." << std::endl;
	boost::scoped_ptr<UTF8InputStream> per_uis_scoped_ptr(UTF8InputStream::build(per_file.c_str()));
	UTF8InputStream& per_uis(*per_uis_scoped_ptr);
	readWKOrderedHash(per_uis, PER);
	_wk_initialized = true;
}

/**
 * Returns -1 if sa is a singleton, 0 if sa does not have an ID at all, otherwise a positive value.
 **/
int CorefUtilities::lookUpWKName(SymbolArray *sa, Symbol type) {
	if(!_wk_initialized){  
		// When trained with WK features, the EDT model relies on them heavily, 
		// so this is a particularly problematic error.
		throw UnrecoverableException("CorefUtilities::lookUpWKName()", 
		"CorefUtilities::lookUpWKName() is being called without initializing the world knowledge list. \
		Make sure WorldKnowledgeFT and/or SimpleRuleNameLinker are correctly calling CorefUtilities::initializeWKLists()");
	}
	int value = -1;
	if (type == GPE){
		value = (_wkMap.isSingletonGpe(sa) ? -1 : _wkMap.getGpeId(sa));
	} else if (type == ORG){
		value = (_wkMap.isSingletonOrg(sa) ? -1 : _wkMap.getOrgId(sa));
	} else if (type == PER){
		value = (_wkMap.isSingletonPer(sa) ? -1 : _wkMap.getPerId(sa));
	} else {
		throw UnrecoverableException("CorefUtilities::lookUpWKName()", "Unrecognized type");	
	}

	//removing 'the' lowers performance 
	//std::wcout<<"("<<str<<")  ...... " <<value<<std::endl;
	return value;
}

SymbolArray * CorefUtilities::getNormalizedSymbolArray(const Mention* ment){
	/*Symbol terms[MAX_MENTION_NAME_SYMS_PLUS];
	int n_terms = ment->getHead()->getTerminalSymbols(terms, MAX_MENTION_NAME_SYMS_PLUS);

	Symbol  normMentTokens[MAX_MENTION_NAME_SYMS_PLUS];
	int l_norm = 0;
	for(int j = 0; j <  n_terms; j++ ){
		Symbol n = CorefUtilities::getNormalizedSymbol(terms[j]);
		if(n != Symbol(L"") && n != Symbol(L" ")){
			normMentTokens[l_norm++] = n;
		}
	}
	SymbolArray * normedSA = _new SymbolArray(normMentTokens, (size_t)l_norm);
	return normedSA;
	*/

	Symbol terms[MAX_MENTION_NAME_SYMS_PLUS];
	Symbol kterms[MAX_MENTION_NAME_SYMS_PLUS];
	Symbol fterms[MAX_MENTION_NAME_SYMS_PLUS];
	int n_terms = ment->getHead()->getTerminalSymbols(terms, MAX_MENTION_NAME_SYMS_PLUS);
	int value = 0;
	//annoying code to normalize these names without regular expressions
	//static const boost::wregex e1(L"^[^[:alpha:]]+\\s+");
	//static const boost::wregex e2(L"\\s+");
	//str = boost::regex_replace(str, e1, L""/*, boost::match_default | boost::format_sed*/);	
	//str = boost::regex_replace(str, e2, L" "/*, boost::match_default | boost::format_sed*/);	
	int keptSymbols = 0;
	//std::wcout << "getNormalizedSymbolArray called with mention " << ment << std::endl;
	//std::wcout << "\t length " << n_terms  << "\t";;
	//for (int nt = 0; nt < n_terms; nt++) {
	//	std::wcout << terms[nt].to_string() << " ";
	//}
	//std::wcout << std::endl;	
	//ASSERT (n_terms <= MAX_MENTION_NAME_SYMS);
	for (int ns = 0; ns < n_terms; ns++){
		std::wstring str = terms[ns].to_string();
		size_t strlen = str.length();
		wchar_t * temp = _new wchar_t[strlen+1];
		int cno = 0;
		bool prev_alpha = false;
		bool changed = false;
		for (int j = 0; j < (int)strlen; j++){
			if(iswpunct(str.c_str()[j]) || iswspace(str.c_str()[j])){
				changed = true;
				if(prev_alpha){
					prev_alpha = false;
					temp[cno++] = L' ';
				}
			}
			else{
				temp[cno++] = str.c_str()[j];
				prev_alpha = true;
			}
		}
		if (changed){
			changed = false;
			if (cno > 0){
				temp[cno] = L'\0';
				kterms[keptSymbols++] = Symbol(temp);
			}
		}else{
			kterms[keptSymbols++] = terms[ns];
		}
		delete [] temp;
	}
	//std::wcout << "kept symbols " << keptSymbols << " ";
	//for (int nt = 0; nt < keptSymbols; nt++){
	//	std::wcout << kterms[nt].to_string() << " ";
	//}
	//std::wcout << std::endl;
	int nks = 0;
	if (keptSymbols > 0){
		for (int tns = 0; tns < keptSymbols; tns++){
			bool changed = false;
			Symbol ksym = kterms[tns];
			std::wstring str = ksym.to_string();
			size_t strlen = str.length();
			wchar_t * temp = _new wchar_t[strlen+1];
			int cno;
			for (cno = 0; cno < (int)strlen; cno++){
				temp[cno] = str[cno];
			}
			temp[strlen] = L'\0';
			cno = (int)strlen -1;
			while (cno >= 0 && temp[cno] == L' '){
				temp[cno] = L'\0';
				changed = true;
				cno--;
			}
			if (!changed){
				fterms[nks++] = ksym;
			}else{
				if (temp[0] == L'\0') {
					;
				}else{
					kterms[nks++] = Symbol(temp);
				}
			}
			delete [] temp;
		}
	}
	//std::wcout << "making SA from " << nks << " symbols ";
	//for (int nk=0; nk < nks; nk++){
	//	std::wcout << kterms[nk] << " ";
	//}
	//std::wcout << std::endl;

	SymbolArray * normedSA = _new SymbolArray(kterms, (size_t)keptSymbols);

	int saLen = (int)normedSA->getSizeTLength();
	const Symbol * sar = normedSA->getArray();

	if (saLen <= 0){
		std::ostringstream ostr;
		ostr << "WorldKnowledgeFT: normalizeSA reduced this mention to zero length  [";
		for (int i=0;i<n_terms;i++){
			ostr << terms[i].to_debug_string() << " ";
		}
		ostr << "]\n";
		SessionLogger::warn("coref_utilities") << ostr.str();
	}
	//std::wcout << "returning normedSA size " << saLen << "  ";
	//for (int nk=0; nk<saLen; nk++) {
	//	std::wcout << sar[nk].to_string() << " ";
	//}
	//std::wcout << std::endl;
		return normedSA;
			

}

bool CorefUtilities::symbolArrayMatch(const Mention* ment1, const Mention* ment2){
	if(ment1->getMentionType() != Mention::NAME || ment2->getMentionType() != Mention::NAME)
		return false;

	SymbolArray * sa1 = getNormalizedSymbolArray(ment1);
	SymbolArray * sa2 = getNormalizedSymbolArray(ment2);
	
	bool match =  (*sa1) == (*sa2);
	delete sa1;
	delete sa2;
	return match;

}
int CorefUtilities::lookUpWKName(const std::wstring & name, Symbol type){
	std::vector<std::wstring> words;
	//std::cout << "Looking up WK name <" << name << ">..." << std::endl;
	boost::algorithm::split(words, name,  boost::algorithm::is_from_range(L' ', L' '));
	if(words.size() > MAX_MENTION_NAME_SYMS_PLUS)
		return 0;
	Symbol word_sym[MAX_MENTION_NAME_SYMS_PLUS];
	for(size_t i =0; i< words.size(); i++){
		word_sym[i] = Symbol(words[i]);
	}
	SymbolArray * sa = new SymbolArray(word_sym, words.size());
	int wk = lookUpWKName(sa, type);
	delete sa;
	return wk;
}
int CorefUtilities::lookUpWKName(const Mention* ment){
	if(ment->getMentionType() != Mention::NAME)
		return 0;
	SymbolArray * sa = getNormalizedSymbolArray(ment);
	if (sa->getSizeTLength() == 0)
		return 0;
	Symbol entityType;
	if(ment->getEntityType().matchesPER())
		entityType = CorefUtilities::PER;
	else if(ment->getEntityType().matchesORG())
		entityType = CorefUtilities::ORG;
	else if(ment->getEntityType().matchesGPE())
		entityType = CorefUtilities::GPE;
	else
		return 0;

	return lookUpWKName(sa, entityType);
}
int CorefUtilities::min(int a, int b)  {
	return ((a < b) ? a : b);
}
size_t CorefUtilities::min(size_t a, size_t b)  {
	return ((a < b) ? a : b);
}

int CorefUtilities::editDistance(const wchar_t *s1, size_t n1, const wchar_t *s2, size_t n2)   {
	int cost_del = 1;
	int cost_ins = 1;
	int cost_sub = 1;
	int dist[(MAXSTRINGLEN+1)*(MAXSTRINGLEN+1)] ;
	if( n1 > MAXSTRINGLEN || n2 > MAXSTRINGLEN){
		int ret(MAXSTRINGLEN*MAXSTRINGLEN);
		SessionLogger::warn("coref_utilities") << "CorefUtilities::editDistance(): " <<
			"returning edit distance of " << ret << " for string that is longer than MAXSTRINGLEN\n";
		return ret;
	}
	static const int cost_switch = 1; // switching 2 characters (e.g. ba insteadof ab)
	size_t i,j;

	dist[0] = 0;

	for (i = 1; i <= n1; i++) 
		dist[i*(n2+1)] = dist[(i-1)*(n2+1)] + cost_del;

	for (j = 1; j <= n2; j++) 
		dist[j] = dist[j-1] + cost_ins;

	//i=1;
	for (j = 1; j <= n2; j++) {
		int dist_del = dist[j] + cost_del;
		int dist_ins = dist[n2+1+j-1] + cost_ins;
		int dist_sub = dist[j-1] + 
						(s1[0] == s2[j-1] ? 0 : cost_sub);
		dist[n2+1+j] = min(min(dist_del, dist_ins), dist_sub);
	}

	//j=1;
	for (i = 1; i <= n1; i++) {
		int dist_del = dist[(i-1)*(n2+1)+1] + cost_del;
		int dist_ins = dist[i*(n2+1)] + cost_ins;
		int dist_sub = dist[(i-1)*(n2+1)] + 
						(s1[i-1] == s2[0] ? 0 : cost_sub);
		dist[i*(n2+1)+1] = min(min(dist_del, dist_ins), dist_sub);
	}

	for (i = 2; i <= n1; i++) {
		for (j = 2; j <= n2; j++) {
			int dist_del = dist[(i-1)*(n2+1)+j] + cost_del;
			int dist_ins = dist[i*(n2+1)+(j-1)] + cost_ins;
			int dist_sub = dist[(i-1)*(n2+1)+(j-1)] + 
							(s1[i-1] == s2[j-1] ? 0 : cost_sub);
			int dist_switch = dist[(i-2)*(n2+1)+(j-2)] + cost_switch;
			dist[i*(n2+1)+j] = min(min(dist_del, dist_ins), dist_sub);
			if(s1[i-1] == s2[j-2] && s1[i-2] == s2[j-1])
				dist[i*(n2+1)+j] = min(dist[i*(n2+1)+j], dist_switch);
		}
	}

	int tmp = dist[n1*(n2+1)+n2];
	return tmp;
}


int CorefUtilities::editDistance(const std::wstring & s1, const std::wstring & s2) {
	return editDistance(s1.c_str(), s1.length(), s2.c_str(), s2.length());
};
std::set<std::wstring> CorefUtilities::getPossibleAcronyms(const Mention* m){
	std::set<std::wstring> acronyms;
	if(m->getMentionType() != Mention::NAME)
		return acronyms;
	if(m->getHead() == 0)
		return acronyms;
	Symbol ment_symbols[20];
	int n_terms = m->getHead()->getTerminalSymbols(ment_symbols, 19);
	if(n_terms == 1)
		return acronyms;
	std::wstring a1 = L"";
	std::wstring a2 = L"";
	std::wstring a3 = L"";
	std::vector<wchar_t> characters;
	for(size_t i =0; i <(size_t)n_terms; i++){
		Symbol s = ment_symbols[i];
		if(WordConstants::isAcronymStopWord(s))	
			continue; //don't worry about the rare acronym that includes a stop word
		wchar_t c = s.to_string()[0];
		a1.push_back(c);
		a2.push_back(c);
		if(i !=  characters.size()-1)
			a2.push_back(L'.');	 //don't worry about the rare acronym that ends with a stop word
	}
	a3 = a2;
	a3.push_back(L'.');
	acronyms.insert(a1);
	acronyms.insert(a2);
	acronyms.insert(a3);
	return acronyms;
}
//Boost supports UTF8, and so chinese hieroglyphs and arabic letters will be correctly recognized as [:alnum:]
Symbol CorefUtilities::getNormalizedSymbol(Symbol s) {
	std::wstring str=s.to_string();
	static const boost::wregex e1(L"(-[LR][RC]B-)|(&(amp;)+)"), e3(L"[^[:alnum:]]+"), e4(L"[\\t ]+"), e5(L"^ "), e6(L" $");
	//replace special tokens by spaces
	str = boost::regex_replace(str, e1, L" ", boost::match_default | boost::format_sed);
	//replace non-alphanumerics by spaces
	str = boost::regex_replace(str, e3, L" ", boost::match_default | boost::format_sed);
	//multiple spaces ==> one ' '
	str = boost::regex_replace(str, e4, L" ", boost::match_default | boost::format_sed);
	//leading and trailing spaces
	str = boost::regex_replace(str, e5, L"", boost::match_default | boost::format_sed);
	str = boost::regex_replace(str, e6, L"", boost::match_default | boost::format_sed);
	//remove capitalization
	std::transform(str.begin(), str.end(), str.begin(), std::towlower);
	static const boost::wregex e7(L"^[^[:alpha:]]+\\s+");
	str = boost::regex_replace(str, e7, L"", boost::match_default | boost::format_sed);
	return Symbol(str);
}



bool CorefUtilities::isAcronym(const Mention* mention, const Entity* entity, const EntitySet* entity_set){
	std::set<std::wstring> acronyms;
	if(mention->getMentionType() != Mention::NAME)
		return false;
	if(mention->getHead() == 0)
		return false;
	
	Symbol ment_symbols[20];
	int n_terms = mention->getHead()->getTerminalSymbols(ment_symbols, 19);
	if(n_terms == 1){ //this might be an acronym, check other mentions to see if there is a match
		std::wstring ment_str = ment_symbols[0].to_string();
		for(int i =0; i < entity->getNMentions(); i++){
			const Mention* oth = entity_set->getMention(entity->getMention(i));
			std::set<std::wstring> acronyms = getPossibleAcronyms(oth);
			if(acronyms.find(ment_str) != acronyms.end())
				return true;
		}
	}
	else{
		std::set<std::wstring> acronyms = getPossibleAcronyms(mention);
		for(int i =0; i < entity->getNMentions(); i++){
			const Mention* oth = entity_set->getMention(entity->getMention(i));
			if(oth->getMentionType() != Mention::NAME)
				continue;
			Symbol oth_symbols[20];
			int oth_n_terms = oth->getHead()->getTerminalSymbols(oth_symbols, 19);
			if(oth_n_terms == 1){
				if(acronyms.find(oth_symbols[0].to_string()) != acronyms.end())
					return true;
			}
		}
	}
	return false;	
}
int CorefUtilities::getTeamID(const Mention* mention){
	if(mention->getMentionType() != Mention::NAME)
		return -1;
	if(!(	mention->getEntityType().matchesGPE() || mention->getEntityType().matchesORG()))
		return -1;
	if(mention->getHead() == 0)
		return -1;
	// Historical questions to resolve:
	// (1) Why do we only look at the first symbol?
	// (1) Why the arbitrary size of 20 when we only look at the first symbol? 
	Symbol ment_symbols[20];
	int n_terms = mention->getHead()->getTerminalSymbols(ment_symbols, 19);
	if (n_terms == 1){
		SymbolToIntMap::const_iterator iter = team_map.find(ment_symbols[0]);
		if (iter != team_map.end()) {
			return (*iter).second;
		} else {
			SessionLogger::warn("coref_utilities") << ment_symbols[0].to_debug_string() << " not found in team map.";
		}
	}
	return -1;
}
bool CorefUtilities::hasTeamClash(const Mention* mention, const Entity* entity,  const EntitySet* entity_set){
	int mteamid = getTeamID(mention);
	if(mteamid == -1)
		return false;
	for(int i =0; i < entity->getNMentions(); i++){
		const Mention* oth = entity_set->getMention(entity->getMention(i));
		int eteamid = getTeamID(oth);
		if(eteamid != -1 && eteamid != mteamid)
			return true;
	}
	return false;
}

bool CorefUtilities::hasSpeakerSourceType(const DocTheory *docTheory) {
	return (docTheory->getDocument()->getSourceType() == TELEPHONE_SOURCE_SYM ||
		docTheory->getDocument()->getSourceType() == BROADCAST_CONV_SOURCE_SYM ||
		docTheory->getDocument()->getSourceType() == WEBLOG_SOURCE_SYM ||
		docTheory->getDocument()->getSourceType() == USENET_SOURCE_SYM ||
		docTheory->getDocument()->getSourceType() == EMAIL_SOURCE_SYM ||
		docTheory->getDocument()->getSourceType() == DF_SYM); 
}
