// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/edt/LexEntitySet.h"
#include "Generic/edt/AbbrevTable.h"
#include "Generic/edt/NameLinkFunctions.h"
#include "Generic/theories/SynNode.h"


DebugStream &LexEntitySet::_debugOut = DebugStream::referenceResolverStream;

LexEntitySet::LexEntitySet(int nSentences)
	: EntitySet(nSentences), _data(_new LexData())
{}

LexEntitySet::LexEntitySet(const LexEntitySet &other) : EntitySet(other) {
	_data = _new LexData(*other._data);
}


LexEntitySet::LexEntitySet(const EntitySet &other, const LexData &data) : EntitySet(other, false), 
	_data(_new LexData(data)) {}

void LexEntitySet::addNew(MentionUID uid, EntityType type) {
	_debugOut << "\n                adding " << uid
			  << " to new " << type.getName().to_debug_string();
	_data->lexEntities.add(NULL);
	EntitySet::addNew(uid, type);
	_debugOut << "\n                done adding new\n";
}

void LexEntitySet::add(MentionUID uid, int entityID) {
	_debugOut << "\n                adding " << uid << " to " << entityID;

	Mention *mention = getMention(uid);
	EntitySet::add(uid, entityID);

	if(mention->getMentionType() == Mention::NAME) {
		_debugOut << "\n                name\n";
		//add this mention's lexical data
		//if the array of LexEntities is not big enough, grow it
		//if(entityID >= _data->lexEntities.length())
		//	_data->lexEntities.setLength(_entities.length());
		//if there is no LexEntity for this ID yet, create one
		if(_data->lexEntities[entityID] == NULL)
			_data->lexEntities[entityID] = _new LexEntity(_entities[entityID]->getType());

		Symbol words[32], resolved[32], lexicalItems[100];
		int nWords, nResolved, nLexicalItems;
		nWords = mention->getHead()->getTerminalSymbols(words, 32);
		nResolved = AbbrevTable::resolveSymbols(words,nWords, resolved, 32);
		nLexicalItems = NameLinkFunctions::getLexicalItems(resolved, nResolved, lexicalItems, 100);
		
		int i;
		_debugOut << "\n                UNRESOLVED: ";
		for (i=0; i<nWords; i++) 
			_debugOut << words[i].to_string() << " "; //Marj changed from debug_string
		_debugOut << "\n                RESOLVED: ";
		for (i=0; i<nResolved; i++) 
			_debugOut << resolved[i].to_string() << " "; //Marj changed from debug_string
		_debugOut << "\n                LEXICAL ITEMS: ";
		for (i=0; i<nLexicalItems; i++) 
			_debugOut << lexicalItems[i].to_string() << " "; //Marj changed from debug_string
		_debugOut << "\n";

		_data->lexEntities[entityID]->learn(lexicalItems, nLexicalItems);
	}
	
}

LexEntitySet::~LexEntitySet() {
	delete _data;
}

LexEntity *LexEntitySet::getLexEntity(int ID) {
	if(_data->lexEntities.length() <= ID)
		return NULL;
	else return _data->lexEntities[ID];
}

LexEntitySet *LexEntitySet::fork() const {
	return _new LexEntitySet(*this);
}

LexEntitySet::LexData::LexData(const LexData &other) {
	for(int i=0; i<other.lexEntities.length(); i++) {
		if(other.lexEntities[i] != NULL)
			lexEntities.add(_new LexEntity(*other.lexEntities[i]));
		else lexEntities.add(NULL);
	}
}

LexEntitySet::LexData::~LexData() {
	for(int i=0; i<lexEntities.length(); i++) {
		delete lexEntities[i];
		lexEntities[i] = NULL;
	}
}
