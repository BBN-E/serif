// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LEXENTITYSET_H
#define LEXENTITYSET_H

#include "Generic/edt/LexEntity.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/DebugStream.h"

class LexEntitySet : public EntitySet {
public:
	class LexData;

	LexEntitySet(int nSentences);
	LexEntitySet(const LexEntitySet &other);
	LexEntitySet(const EntitySet &other, const LexData &data);
	~LexEntitySet();

	virtual void addNew(MentionUID uid, EntityType type); 
	virtual void add(MentionUID uid, int entityID);
	LexEntitySet *fork() const;
	LexData *getLexData() { return _data; }
	LexEntity *getLexEntity(int ID);
	void setLexData(LexData *data) { _data = data; }

	class LexData {
	public:
		LexData() {}
		LexData(const LexData &other);
		~LexData();
		GrowableArray <LexEntity *> lexEntities;
	};


private:
	LexData *_data;
	static DebugStream &_debugOut;
};

#endif
