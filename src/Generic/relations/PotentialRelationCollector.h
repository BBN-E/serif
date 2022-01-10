// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef POTENTIAL_RELATION_COLLECTOR_H
#define POTENTIAL_RELATION_COLLECTOR_H

#include <boost/shared_ptr.hpp>

#include "Generic/results/ResultCollector.h"
#include "Generic/common/limits.h"

class DocTheory;
class PotentialTrainingRelation;
class PotentialRelationInstance;
class RelationTypeMap;
class Parse;
class MentionSet;
class PropositionSet;


class PotentialRelationCollector : public ResultCollector {
public:
	/** Create and return a new PotentialRelationCollector. */
	static PotentialRelationCollector *build(int collectionMode) { return _factory()->build(collectionMode); }
	static PotentialRelationCollector *build(int collectionMode, RelationTypeMap *relationTypes) { return _factory()->build(collectionMode, relationTypes); }
	/** Hook for registering new PotentialRelationCollector factories */
	struct Factory { 
		virtual PotentialRelationCollector *build(int collectionMode) = 0; 
		virtual PotentialRelationCollector *build(int collectionMode, RelationTypeMap *relationTypes) = 0; 
	};
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~PotentialRelationCollector() { }

	virtual void loadDocTheory(DocTheory *docTheory) = 0;

	virtual void produceOutput(const wchar_t *output_dir, const wchar_t *document_filename) = 0;
	virtual void produceOutput(std::wstring *results) { ResultCollector::produceOutput(results); }

	virtual void outputPacketFile(const char *output_dir, const char *packet_name) = 0;

	virtual int getNRelations() = 0;
	virtual PotentialTrainingRelation* getPotentialTrainingRelation(int i) = 0;
	virtual PotentialRelationInstance* getPotentialRelationInstance(int i) = 0;

	virtual void resetForNewSentence() {}
	virtual void collectPotentialDocumentRelations() {}
	virtual void collectPotentialSentenceRelations(const Parse *parse, MentionSet *mentionSet, 
		const PropositionSet *propSet) {}


protected:
	PotentialRelationCollector() { }

private:
	static boost::shared_ptr<Factory> &_factory();
};

//// language-specific includes determine which implementation is used
//#ifdef CHINESE_LANGUAGE
//	#include "Chinese/relations/ch_PotentialRelationCollector.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/relations/ar_PotentialRelationCollector.h"
//#else	
//	#include "Generic/relations/xx_PotentialRelationCollector.h"
//#endif


#endif
