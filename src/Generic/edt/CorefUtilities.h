// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef COREFUTILITIES_H
#define COREFUTILITIES_H

#include <iostream>
#include <set>
#include <map>
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolArray.h"
#include "Generic/common/SymbolConstants.h"
class UTF8InputStream;
class Mention;
class Entity;
class EntitySet;
class DocTheory;

typedef Symbol::HashMap<int> SymbolToIntMap;
#define MAXSTRINGLEN 300
class WKMap {
	private:
		SymbolArrayIntegerMap _wkGpe;
		SymbolArraySet _wkGpeSingletons;
		SymbolArrayIntegerMap _wkOrg;
		SymbolArraySet _wkOrgSingletons;
		SymbolArrayIntegerMap _wkPer;
		SymbolArraySet _wkPerSingletons;
	public:
		// All of these methods take ownership of the SymbolArray argument:
		void addGpe(SymbolArray* sa, int id);
		void addGpeSingleton(SymbolArray* sa);
		int getGpeId(SymbolArray *sa);
		bool isSingletonGpe(SymbolArray *sa);
		void addOrg(SymbolArray* sa, int id);
		void addOrgSingleton(SymbolArray* sa);
		int getOrgId(SymbolArray *sa);
		bool isSingletonOrg(SymbolArray *sa);
		void addPer(SymbolArray* sa, int id);
		void addPerSingleton(SymbolArray* sa);
		int getPerId(SymbolArray *sa);
		bool isSingletonPer(SymbolArray *sa);
		WKMap();
		~WKMap(); 
	};
class CorefUtilities {

public:
	static void readWKOrderedHash(UTF8InputStream& uis, Symbol type);
	// Reads values of gpe_file, org_file, and per_file from param file, then calls next overloaded version.
	static void initializeWKLists();
	// Call this version if you want more control over the filenames that you work with.
	static void initializeWKLists(const std::string & gpe_file, const std::string & org_file, const std::string & per_file);

	static int lookUpWKName(const std::wstring & name, Symbol type); 
	static int lookUpWKName(SymbolArray *sa, Symbol type); 
	static int lookUpWKName(const Mention* ment); 
	static SymbolArray * getNormalizedSymbolArray(const Mention* ment);
	static bool symbolArrayMatch(const Mention* ment1, const Mention* ment2);

	//mrf: These were copied from MentMentEditDistance3FT. (They also appear in
	// MentMentEditDistance2FT, MentMentEditDistanceFT, en_NamePersonClashFT.h.) 
	// They should be moved to a utility class.  
	static int editDistance(const wchar_t *s1, size_t n1, const wchar_t *s2, size_t n2) ;
	static int editDistance(const std::wstring & s1, const std::wstring & s2) ;
	static Symbol getNormalizedSymbol(Symbol s);
	static bool isAcronym(const Mention* mention, const Entity* entity,  const EntitySet* entity_set);
	static bool hasTeamClash(const Mention* mention, const Entity* entity,  const EntitySet* entity_set);

	static bool hasSpeakerSourceType(const DocTheory *docTheory);

private:
	static WKMap _wkMap;
	static Symbol GPE;
	static Symbol ORG;
	static Symbol PER;
	static bool _use_gpe_world_knowledge_feature;
	static SymbolToIntMap team_map;
	static bool _wk_initialized;
	static const size_t MAX_MENTION_NAME_SYMS_PLUS;
	static int min(int a, int b) ;
	static size_t min(size_t a, size_t b) ;
	static std::set<std::wstring> getPossibleAcronyms(const Mention* m);
	static int getTeamID(const Mention* mention);


	static Symbol TELEPHONE_SOURCE_SYM;
	static Symbol BROADCAST_CONV_SOURCE_SYM;
	static Symbol USENET_SOURCE_SYM;
	static Symbol WEBLOG_SOURCE_SYM;
	static Symbol EMAIL_SOURCE_SYM;
	static Symbol DF_SYM;
	//mutable int dist[(MAXSTRINGLEN+1)*(MAXSTRINGLEN+1)] ;
};

#endif
