// Copyright (c) 2009 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef GOLD_STANDARD_STORAGE_H
#define GOLD_STANDARD_STORAGE_H

#include "Generic/common/Symbol.h"
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

class UTF8OutputStream;
class DocTheory;
class SentenceTheory;
class SynNode;
class Mention;
class EntitySet;
class Entity;
class ValueMention;

class FactGoldStandardStorage
{
public:

	struct StringData
	{
		std::wstring string;
		StringData *next;
	};

	struct EntityData
	{
		Symbol docId;
		int entity_id;
		EntityData *next;
	};

	struct FactData
	{
		int id;
		std::wstring slot1Guid;
		Symbol relationType;
		Symbol slot2SlotType;

		StringData *slot2Data;
		StringData *answerData;
		EntityData *slot2EntityData;

		int count_in_documents;

		EntityData *entityData;
	};

	FactGoldStandardStorage(const char *file_name);
	~FactGoldStandardStorage();

	void outputCounts(UTF8OutputStream &stream);
	void processDocuments(std::vector<DocTheory *> & docTheories);

private:

	// Slot types
	static Symbol PER;
	static Symbol ORG;
	static Symbol GPE;
	static Symbol DATE;
	static Symbol TEXT;

	FactData *_factData[2000000];
	EntityData *_entityData[2000000];

	int _n_fact_entries;
	int _n_entity_entries;

	std::wstring convertToLower(std::wstring &str);

	void processFact(FactData* factData, std::vector<DocTheory *> & docTheories);
	void countFactInstances(FactData *factData, DocTheory *docTheory, Symbol docId, int entity_id);
	bool slot2InSentence(DocTheory *docTheory, FactData *fd, const SentenceTheory *st, Symbol docId, int fact_entity_id);
	
	const std::wstring getName(const Mention *m, const EntitySet *es);
	void storeNameData(FactData *fd, DocTheory *docTheory, int fact_entity_id, const Entity *e);
	void storeDateData(FactData *fd, DocTheory *docTheory, int fact_entity_id, const ValueMention *e);
	void storeTextData(FactData *fd, DocTheory *docTheory, int fact_entity_id, std::wstring &str);
};

#endif
