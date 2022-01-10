#include <vector>
#include <set>
#include <map>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/Symbol.h"
#include "TemporalTypes.h"

class TemporalInstance;
class DocTheory;
class EntitySet;
class RelMention;
class Mention;
class EventMention;
class SentenceTheory;
class TokenSequence;
class Symbol;
class SynNode;
BSP_DECLARE(TemporalNormalizer);
BSP_DECLARE(FeatureMap)
BSP_DECLARE(ElfRelation)
BSP_DECLARE(ElfRelationArg)
BSP_DECLARE(TemporalInstance)
BSP_DECLARE(TemporalFeature)
BSP_DECLARE(TemporalAdjacentWordsFeatureProposer)
BSP_DECLARE(TemporalTreePathFeatureProposer)
	
class TemporalFeatureVectorGenerator {
public:
	TemporalFeatureVectorGenerator(FeatureMap_ptr featureMap);
	TemporalFV_ptr fv(const DocTheory* dt, int sn, const TemporalInstance& inst);
	/*void observe(const TemporalInstance& inst, const SentenceTheory* st, 
			const DocTheory* dt);
	void finishObservations();*/
private:
	void addTimexShapeFeatures(const TemporalInstance& inst, 
			std::vector<TemporalFeature_ptr>& fv);
	void addRelationFeatures(const DocTheory* dt, unsigned int sn, 
		const TemporalInstance& inst, std::vector<TemporalFeature_ptr>& fv);
	void addEventFeatures(const DocTheory* dt, unsigned int sn,
		const TemporalInstance& inst, std::vector<TemporalFeature_ptr>& fv);

	static bool relMatchesRel(const RelMention* serifRel, ElfRelation_ptr elfRel,
		const EntitySet* es);
	static bool relArgMatchesMention(ElfRelationArg_ptr arg, const Mention* ment,
		const EntitySet* es);
	static bool matchesEvent(const EventMention* event, const TemporalInstance& inst,
		const EntitySet* es);
	static Symbol getTemporalRoleName(const EventMention* event,
		const TemporalInstance& inst);
	static std::wstring anchorArg(const EventMention* event);

	TemporalNormalizer_ptr _tempNormalizer;
	FeatureMap_ptr _featureMap;

	void addWordFeatures(const TemporalInstance& inst, const SentenceTheory* st, 
			std::vector<TemporalFeature_ptr>& fv);
	void addWordFeatures(const TemporalInstance& inst, const TokenSequence* ts,
			const SynNode* node, std::vector<TemporalFeature_ptr>& fv);
	void addPropSpines(const TemporalInstance& inst, const SentenceTheory* st,
			std::vector<TemporalFeature_ptr>& fv);
	//void observeWords(const TokenSequence* ts, const SynNode* node);
	//void observePropSpines(const SentenceTheory* st);

/*	std::set<Symbol> _goodWords;
	typedef std::map<Symbol, unsigned int> WordCountsMap;
	WordCountsMap _wordCounts;

	typedef std::map<size_t, unsigned int> SpineCountsMap;
	SpineCountsMap _spineCounts;*/

	TemporalAdjacentWordsFeatureProposer_ptr _adjacentWords;
	TemporalTreePathFeatureProposer_ptr _treePaths;
};

