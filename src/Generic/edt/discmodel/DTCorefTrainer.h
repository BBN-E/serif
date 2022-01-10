// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DT_COREF_TRAINER_H
#define DT_COREF_TRAINER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/hash_map.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/Mention.h"
#include "Generic/discTagger/DTFeature.h"
#include "Generic/edt/HobbsDistance.h"

class PropositionFinder;
class DTFeatureTypeSet;
class DTTagSet;
class DTCorefObservation;
class DTNoneCorefObservation;
class DTObservation;
class P1Decoder;
class RelationObservation;
class MaxEntModel;
class Entity;
class EntitySet;
class Parse;
class MentionSet;
class SynNode;
class CorefItemList;
class CorefDocument;
class DocTheory;
class MetonymyAdder;
class DocumentMentionInformationMapper;
class SymbolArray;

class DTCorefTrainer {
public:
	DTCorefTrainer();
	~DTCorefTrainer();

	void train();
	void devTest();

private:
	int MODEL_TYPE;
	enum {P1, MAX_ENT, BOTH, P1_RANKING};

	int MODE;
	enum {TRAIN, DEVTEST};

	int TRAIN_SOURCE;
	enum {STATE_FILES, AUG_PARSES};

	int SEARCH_STRATEGY;
	enum {CLOSEST, BEST};

	Mention::Type TARGET_MENTION_TYPE;

	bool _filter_by_entity_type;
	bool _filter_by_entity_subtype;

	// as opposed to names then nominals and then pronouns (false is closer to a serif run)
	bool _train_from_left_to_right_on_sentence; 

	bool _considerOutsideSentenceLinksOnly;
	bool _considerWithinSentenceLinksOnly;

	std::string _training_file;
	bool _list_mode;
	std::string _model_file;
	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;
	double *_tagScores;

	std::vector<const Parse *> _previousParses;
	typedef std::map<int, const Mention*> MentionMap;
	MentionMap _appositiveLinks;
	MentionMap _copulaLinks;
	MentionMap _specialLinks;

	const static int MAX_CANDIDATES;
	int _n_observations;
	std::vector<DTNoneCorefObservation *> _observations;

	// a map of mention ids to their relevant information
	DocumentMentionInformationMapper *_infoMap;

	bool _use_metonymy;
	MetonymyAdder *_metonymyAdder;

	PropositionFinder *_propositionFinder;

	struct EntityIDPair {
		int entity1;
		int entity2;
		Symbol linkType;

		EntityIDPair() : entity1(-1), entity2(-1) {}
		EntityIDPair(int e1, int e2, Symbol lnk) : entity1(e1), entity2(e2), linkType(lnk) {}
	};

	std::vector<EntityIDPair> _entityLinks;
	std::vector<EntityIDPair> _mentionLinks;

	static const int ENTITY_LINK_MAX;


	bool _print_model_every_epoch;

	void trainP1StateFiles();

	// MAX_ENT
	MaxEntModel *_maxEntDecoder;
	DTFeature::FeatureWeightMap *_maxEntWeights;
	int _pruning;
	int _percent_held_out;
	int _mode;
	int _max_iterations;
	double _variance;
	double _likelihood_delta;
	int _stop_check_freq;
	std::string _train_vector_file;
	std::string _test_vector_file;
	double _link_threshold;

	void trainMaxEntStateFiles();

	// P1
	P1Decoder *_p1Decoder;
	DTFeature::FeatureWeightMap *_p1Weights;
	int _epochs;
	int _n_instances_seen;
	int _n_correct;
	int _total_n_correct;
	
	// P1 RANKING
	// use non-ace entities as candidates for linking resulting in no-link decisions.
	bool _use_non_ace_entities_as_no_links;
	// the number of non-ace candidates added when non-ace entities as candidates option is used
	int _max_non_ace_candidates;
	// use non-ace mentions as examples for no-linking decision.
	bool _use_no_link_examples;
	// first create name entities (like in the linker) - allows forward coref within a single sentence
	bool _do_names_first;
	bool _limit_to_names; // an option for name coreference only
	bool _use_p1_averaging;
	int _p1_required_margin; // margin training

	DTFeatureTypeSet *_noneFeatureTypes;
	DTFeatureTypeSet **_featureTypesArr;

	class LinkRecord {
	public:
		int link_index;
		int tag_index;
		Entity *entity;
		double score;

		LinkRecord(int l_index, int t_index, Entity *ent, double s) : link_index(l_index), tag_index(t_index), entity(ent), score(s) {}

		/* Order by descending score.  Use descending link index as a 
		 * tie-breaker except for 0 link index (no-link), which is always first.
		 */
		bool operator<(const LinkRecord rhs) const {
			if (score == rhs.score) {
				if (rhs.link_index == 0)
					return false;
				if (link_index == 0)
					return true;
				else
					return (link_index > rhs.link_index);			
			}
			return (score > rhs.score);
		}
	};
	// END P1 RANKING

	// STATE_FILES
	std::vector<DocTheory *> _docTheories;

	void loadTrainingDataFromStateFiles();
	void loadTrainingDataFromStateFile(const wchar_t *filename);
	void loadTrainingDataFromStateFile(const char *filename);
	void processDocument(DocTheory *docTheory);
	void processSentenceEntitiesFromStateFile(SentenceTheory *sentTheory,
												MentionSet *mentionSet,
												EntitySet *entitySet,
												EntitySet *corrEntitySet,
												PropositionSet *propSet);

	/* hash to keep track of coref ids.
	* Mapps between the entityID in the document and the entity position in our
	* set of entities during training (which can be different for some training schemes)
	*/
	struct HashKey {
		size_t operator()(const int a) const {
			return a;
		}
	};

	struct EqualKey {
		bool operator()(const int a, const int b) const {
		return (a == b);
		}
	}; 
	typedef serif::hash_map<int, int, HashKey, EqualKey> IntegerMap;
	IntegerMap *_alreadySeenDocumentEntityIDtoTrainingID;


	// DEVTEST
	UTF8OutputStream _devTestStream;
	int _correct_link;
	int _correct_nolink;
	int _missed;
	int _wrong_link;
	int _spurious;
	int _correct_ACE_nolink;
	int _ace_spurious;
	int _correct_nonACE_nolink;
	int _nonACE_spurious;

	int _wrong_link_outside_sentence;
	int _wrong_link_within_sentence;
	int _missed_outside_sentence;
	int _missed_within_sentence;
	int _correct_outside_sentence;
	int _correct_within_sentence;
	int _spurious_nolink_new_entity;
	int _spurious_nolink_within_document;
	int _spurious_nolink_within_sentence;
	int _correct_nolink_new_entity;
	int _correct_nolink_within_sentence;
	int _correct_nolink_within_document;

	void devTestStateFiles();
	void decodeToP1Distribution(DTObservation *observation);

	// MISC
	void trainMention(const Mention *ment, EntityType mentionType, int id, 
						EntitySet *entitySet, SentenceTheory * sentTheory = 0);
	void writeWeights(int epoch = -1);
	void dumpTrainingParameters(UTF8OutputStream &out);

	void sortEntities(const GrowableArray <Entity *> &allEntities,
					const GrowableArray <Entity *> &entitiesByType,
					const Mention *ment,
					int entityID,
					const EntitySet *entSet,
					int nCandidates,
					GrowableArray <Entity *>& sortedEnts,
					int max_non_ace_candidates);

	int getHobbsDistance(EntitySet *entitySet, Entity *entity,
					 HobbsDistance::SearchResult *hobbsCandidates, int nHobbsCandidates);
	int getPreLinkID(EntitySet *entitySet, const Mention *ment, const Mention *preLink);

	// coref all the name mentions first for possible forward nominal (or pronomial) coref
	void correctCorefAllNameMentions(EntitySet *entitySet, EntitySet *corrEntitySet, MentionSet *mentionSet);
	void correctCorefAllNameMentions(EntitySet *entitySet, int ids[], MentionSet *mentionSet);

	bool DEBUG;
	UTF8OutputStream _debugStream;

	void computeConfidence(std::vector<LinkRecord> candidates, double correct_score, double &best_conf, double &sec_best_conf, double &correct_conf);

	// Utility methods for printing debug and devtest output
	void printEntity(UTF8OutputStream &out, EntitySet *entitySet, Entity *entity);
	void printDebugScores(DTCorefObservation *observation, UTF8OutputStream& stream);
	void printDebugScores(MentionUID mentionID, int entityID, int hobbs_distance, UTF8OutputStream& stream); 
	void printDebugScores(DTNoneCorefObservation *hyp_obs,
									  int hyp_tag, double hyp_score, double conf,
									  Symbol hypothesized_decision,
									  Entity *hyp_link,
									  DTNoneCorefObservation *sec_hyp_obs,
									  int sec_hyp_tag, double sec_score, double sec_conf,
									  Symbol sec_hypothesized_decision,
									  Entity *sec_hyp_link,
									  UTF8OutputStream& stream,
									  EntitySet *entitySet);
	void printHeadWords(SymbolArray **hwArray, UTF8OutputStream &stream);
	std::wstring getCurrentDebugSentence(SentenceTheory *sentTheory, const Mention *mention);
	
	/*

	// AUG_PARSES
	void trainMaxEntAugParses();
	void trainP1AugParses();
	void trainEpochAugParses();
	void devTestAugParses();

	typedef GrowableArray<CorefItem *> CorefItemList;
	AnnotatedParseReader _inputSet;

	ClassifierTreeSearch _searcher;
	PartitiveClassifier _partitiveClassifier;
	AppositiveClassifier _appositiveClassifier;
	ListClassifier _listClassifier;
	NestedClassifier _nestedClassifier;
	PronounClassifier* _pronounClassifier;
	OtherClassifier _otherClassifier;
	SubtypeClassifier _subtypeClassifier;
	
	void processDocument(CorefDocument *document);
	void findSentenceCorefItems(CorefItemList &items, const SynNode *node, 
								CorefDocument *document);
	void addCorefItemsToMentionSet(MentionSet* mentionSet, CorefItemList &items, int ids[]);
	void processMentions(const SynNode* node, MentionSet *mentionSet);
	void processNode(const SynNode* node);
	void generateSubtypes(const SynNode *node, MentionSet *mentionSet);
	void processSentenceEntitiesFromAugParses(MentionSet *mentionSet, EntitySet *entitySet, 
									  PropositionSet *propSet, int ids[]);
	// END AUG_PARSES
	*/
};

#endif
