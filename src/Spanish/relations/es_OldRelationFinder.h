// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
#include "Generic/common/limits.h"
#include "Generic/common/UTF8OutputStream.h"

#include "Generic/theories/RelationConstants.h"
#include "Generic/theories/RelMention.h"
#include "Generic/CASerif/correctanswers/CorrectDocument.h"
#include "Generic/common/AutoGrowVector.h"

class Parse;
class Proposition;
class PropositionSet;
class Mention;
class MentionSet;
class RelMentionSet;
class Argument;
class PotentialRelationInstance;
class RelationModel;
class SpecialRelationCases;

class OldRelationFinder {
public:
	OldRelationFinder();
	~OldRelationFinder();

	void resetForNewSentence();

	RelMentionSet *getRelationTheory(const Parse *parse,
			                       MentionSet *mentionSet,
			                       const PropositionSet *propSet);

	static UTF8OutputStream _debugStream;
	static bool DEBUG;
	static int _relationFinderType;
	enum { NONE, ACE, EELD, ITEA };

	void allowTypeCoercing() { _allow_type_coercing = true; }
	void disallowTypeCoercing() { _allow_type_coercing = false; }

	CorrectDocument *currentCorrectDocument;

private:
	RelationModel *_model;
	
	bool _allow_type_coercing;
	void changeMentionType(const Mention *ment, EntityType type);

	AutoGrowVector<Proposition *> _definitions;
	std::vector<Argument*> _propTakesPlaceAt;
	void collectDefinitions(const PropositionSet *propSet);

	PotentialRelationInstance *_instance;
	SpecialRelationCases *_specialRelationCases;
	
	void classifyLocationProposition(Proposition *prop);
	void classifyCopulaProposition(Proposition *prop);
	void classifySetProposition(Proposition *prop);
	void classifyProposition(Proposition *prop);

	bool isGroupMention(const Mention* mention);

	
	void classifyArgumentPair(Argument *leftArg, Argument *rightArg, Proposition *prop);

	void findNestedInstances(Argument *first, Argument *intermediate, Proposition *prop);
	bool findWithCoercedType(Argument *leftArg, Argument *rightArg, Proposition *prop);
	bool tryToCoerceInstance(const Mention *left, const Mention *right, const Mention *mentionToBeChanged, 
						 EntityType newType);
	void findListInstances(Argument *left, Argument *right, 
		Proposition *prop, bool leftIsList) ;

	bool classifyInstance(const Mention *first, const Mention *second);


	// SRS -- propositional relations
	bool _enable_raw_relations;
	void addRawRelations(Proposition *prop,
						 const MentionSet *mentionSet);

	void addPartitiveRelations();

	// When doing rule-based relation finding, do we want to look for slots
	// nested inside entities that are not core types (like QUANTITY as in
	// "45 kg of plutonium"
	bool _look_inside_non_core_types;

	RelMention *_relations[MAX_SENTENCE_RELATIONS];
	int _n_relations;
	void addRelation(const Mention *first, 
		const Mention *second, int type,
		float score);

	MentionSet *_currentMentionSet;
	EntitySet *_currentEntitySet;
	int _currentSentenceIndex;

	bool _use_correct_answers;
};
