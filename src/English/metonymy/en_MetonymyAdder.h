// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_METONYMY_ADDER_H
#define en_METONYMY_ADDER_H

#include "Generic/common/SymbolHash.h"
#include "Generic/common/DebugStream.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/theories/EntitySubtype.h"
#include "Generic/metonymy/MetonymyAdder.h"

#define MAX_SPORTS_PHRASES 1000

class SymbolHash;
class Mention;
class MentionSet;
class SynNode;
class PropositionSet;
class Proposition;
class Argument;
class EntitySubtype;
class SymbolListMap;

class EnglishMetonymyAdder : public MetonymyAdder {
private:
	friend class EnglishMetonymyAdderFactory;

public:
	~EnglishMetonymyAdder();

	void resetForNewSentence() {}
	void resetForNewDocument(DocTheory *docTheory);

	virtual void addMetonymyTheory(const MentionSet *mentionSet,
				                   const PropositionSet *propSet);

private:
	bool _use_metonymy;
	bool _use_gpe_roles;
	
	// TAC
	bool _do_metonymy_for_tac;

	int _total_words;
	int _sports_words;
	bool _is_sports_story;
	bool _saw_university;
	bool _saw_olympics;

	EntitySubtype _sportsTeamSubtype;

	EnglishMetonymyAdder();
	void processNounOrNamePredicate(Proposition *prop, const MentionSet *mentionSet);
	void processVerbPredicate(Proposition *prop, const MentionSet *mentionSet);
	void processSetPredicate(Proposition *prop, const MentionSet *mentionSet);
	void processLocPredicate(Proposition *prop, const MentionSet *mentionSet);
	void processModifierPredicate(Proposition *prop, const MentionSet *mentionSet);

	void processGPENounArgument(Argument *arg, const Mention *ref, const MentionSet *mentionSet);
	void processOrgNounArgument(Argument *arg, const Mention *ref, const MentionSet *mentionSet);
	void processFacNounArgument(Argument *arg, const Mention *ref, const MentionSet *mentionSet);
	void processGPEVerbArgument(Argument *arg, const SynNode *verbNode, const MentionSet *mentionSet);
	void processFacVerbArgument(Argument *arg, const SynNode *verbNode, const MentionSet *mentionSet);
	void processOrgVerbArgument(Argument *arg, const SynNode *verbNode, const MentionSet *mentionSet);

	void countSportsWords(const SynNode *node);
	bool isGPEPerson(Mention *ment);
	bool isNationalityWord(Symbol word);
	bool isOrgFacSpecialCase(Mention *mention);
	bool isFacOrgSpecialCase(Mention *mention);

	bool theIsOnlyPremod(const SynNode *node);
	Symbol getSmartVerbHead(const SynNode *node); 
	bool getRoleFromParent(Mention *mention);
	bool hasSpecialHeadWord(Mention *mention);
	bool isDateline(Mention *mention, const MentionSet *mentionSet);
	bool somethingIsBasedThere(Mention *mention);
	bool isSignoff(Mention *mention, const MentionSet *mentionSet);
	bool isLocativeReference(Mention *mention);
	bool isLocationPreposition(Symbol preposition);
	bool isSportsTeam(Mention *mention);
	bool isCapitalCity(Mention *mention);
	bool isCountry(Mention *mention);

	bool _isObjectOfLocativeProposition(Mention *mention, const PropositionSet *propSet, const MentionSet *mentionSet);
	void _loadSportsPhrasesList(UTF8InputStream &stream);
	std::wstring *_sportsPhrases[MAX_SPORTS_PHRASES];
	int _num_sports_phrases;

	void addMetonymyToMention(Mention *mention, EntityType type);
	void addRoleToMention(Mention *mention, EntityType type);
	void loadSymbolHash(SymbolHash *hash, const char* file);

	void changeGPEintoSportsTeam(Mention *mention);
	
	// TAC
	void changeTypeORGintoFAC(Mention *mention);
	void changeTypeFACintoORG(Mention *mention);
	void resolveMetonymyForTAC(const MentionSet *mentionSet, const PropositionSet *propSet);
	//

	SymbolHash *_orgVerbs;
	SymbolHash *_gpeVerbs;
	SymbolHash *_peopleVerbs;
	SymbolHash *_meaninglessVerbs;
	SymbolHash *_adjectivalGPEs;
	SymbolHash *_sportsWords;
	SymbolHash *_capitalCities;
	SymbolHash *_countries;
	SymbolHash *_peopleWords;
	SymbolHash *_governmentOrgs;
	SymbolHash *_directionWords;
	SymbolHash *_locativeReferenceWords;
	SymbolHash *_travelWords;
	SymbolHash *_teamVerbs;
	SymbolHash *_governmentNouns;
	SymbolHash *_states;

	SymbolHash *_functioningAsFacVerbs;
	SymbolHash *_functioningAsOrgVerbs;

	DebugStream _debug;
};

class EnglishMetonymyAdderFactory: public MetonymyAdder::Factory {
	virtual MetonymyAdder *build() { return _new EnglishMetonymyAdder(); }
};


#endif
