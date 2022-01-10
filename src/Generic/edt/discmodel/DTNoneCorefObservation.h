// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DT_NONE_COREF_OBSERVATION_H
#define DT_NONE_COREF_OBSERVATION_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolArray.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/edt/discmodel/DocumentMentionInformationMapper.h"

class MentionSet;
class Mention;
class EntitySet;

class DocumentRelMentionSet;

/** Observation that holds information that is only related to the mention being cofererenced
  * DTCorefObservation will hold the rest of the information relevant for the entity.
  * DTNoneCorefObservation is used in Ranking where one of the options is not to link.
  * Features can be derived from the mention by itself.
  */

class DTNoneCorefObservation : public DTObservation {
public:
	DTNoneCorefObservation() : DTObservation(_className), _infoMap(0) {}
	DTNoneCorefObservation(const DocumentMentionInformationMapper *infoMap) 
		: DTObservation(_className), _infoMap(infoMap) {}

	virtual DTObservation *makeCopy();

	/// To be able to recycle the instance, reset these values for each sentence
	/* this function should be removed later */
	virtual void resetForNewSentence(const MentionSet *mentionSet);

	/// To be able to recycle the instance, reset these values for each decision point
	void populate(MentionUID mentionID);

	const MentionSet *getMentionSet() { return _mentionSet; }
	const Mention *getMention() { return _ment; }
	const EntitySet *getEntitySet() const{ return _entitySet; }
	MentionUID getMentionUID() const{ return _ment_id; }
//	GrowableArray<const MentionSet *> *getAllMentionSets() { return _documentMentionSets; }
	
	static const Symbol _noLinkSym;
	static const Symbol _linkSym;

	// TODO:remove this function later and use the other(longer) function instead
	virtual void resetForNewDocument(const EntitySet *entSet);
	virtual void resetForNewDocument(const EntitySet *entSet, const DocumentMentionInformationMapper *infoMap);
	virtual void setDocumentInformationMapper(const DocumentMentionInformationMapper *infoMap) { _infoMap = infoMap; }
	const MentionSymArrayMap* getHWMentionMapper() const { return _infoMap->getHWMentionMapper(); }
	const MentionSymArrayMap* getAbbrevMentionMapper() const { return _infoMap->getAbbrevMentionMapper(); }
	const MentionSymArrayMap* getPremodMentionMapper() const { return _infoMap->getPremodMentionMapper(); }
	const MentionSymArrayMap* getPostmodMentionMapper() const { return _infoMap->getPostmodMentionMapper(); }
	const MentionSymArrayMap* getMentionWordsMentionMapper() const { return _infoMap->getMentionWordsMentionMapper(); }
	const MentionSymArrayMap* getWordsWithinBracketsMapper() const { return _infoMap->getWordsWithinBracketsMapper(); }
	const MentionToSymbolMap* getMentionGenderMapper() const { return _infoMap->getMentionGenderMapper(); }
	const MentionToSymbolMap* getMentionNumberMapper() const { return _infoMap->getMentionNumberMapper(); }

protected:
	void copyTo(DTNoneCorefObservation *other);

	const DocumentMentionInformationMapper *_infoMap;
//	GrowableArray<const MentionSet *> *_documentMentionSets;
	const MentionSet *_mentionSet;
	const EntitySet *_entitySet;

	MentionUID _ment_id;
	const Mention *_ment;

	static const Symbol _className;

};

#endif
