#ifndef _ACE_EVENT_GOLD_STANDARD_H_
#define _ACE_EVENT_GOLD_STANDARD_H_

#include <string>
#include <utility>
#include <map>
#include <vector>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/DocumentTable.h"

class DocTheory;
class Mention;
class Entity;
class ACEACEPassageDescription;
class ACEPassageDescription;
BSP_DECLARE(ProblemDefinition)
BSP_DECLARE(ACEEvent)
BSP_DECLARE(PassageEntityMapping)

typedef std::vector<int> GoldAnswers;
typedef boost::shared_ptr<GoldAnswers> GoldAnswers_ptr;

class GoldStandardACEInstance {
public:
	GoldStandardACEInstance(const ACEEvent_ptr& serifEvent,
			const PassageEntityMapping_ptr& entityMap,
			const GoldAnswers_ptr& goldAnswers) : serifEvent(serifEvent),
	entityMap(entityMap), goldAnswers(goldAnswers) {}

	ACEEvent_ptr serifEvent;
	PassageEntityMapping_ptr entityMap;
	GoldAnswers_ptr goldAnswers;
};

class PassageEntityMapping {
public:
	PassageEntityMapping(const ACEPassageDescription& passage, 
			const DocTheory* serifTheory, const DocTheory* goldTheory);
	static double scoreMentionAlignment(const Mention* source,
			const Mention* target);
	static const std::vector<const Entity*> entitiesInPassage(const DocTheory* dt,
			const ACEPassageDescription& passage) ;
	static double scoreEntityAlignment(const Entity* source,
				const Entity* target, const DocTheory* sourceDT,
				const DocTheory* targetDT);
	typedef std::vector<std::vector<double> > SimilarityMatrix;

	SimilarityMatrix similarityMatrix;
};

class ACEEventGoldStandard {
public:
	ACEEventGoldStandard(const ProblemDefinition_ptr& problem,
			const std::string& gsDocTable,
			const std::string& serifDocTable);
	std::vector<GoldStandardACEInstance> goldStandardInstances(
		size_t fileIdx);
	size_t size() const { return _gsDocTable.size(); }
private:
	ProblemDefinition_ptr _problem;

	DocumentTable::DocumentTable _gsDocTable;
	DocumentTable::DocumentTable _serifDocTable;

};

class SkipThisDocument {};

#endif

