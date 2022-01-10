// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOC_THEORY_H
#define DOC_THEORY_H

#include "Generic/theories/Theory.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/UTCoref.h"
#include "Generic/theories/Metadata.h"
#include "Generic/theories/SentenceTheoryBeam.h"
#include "Generic/actors/Identifiers.h"
#include <boost/shared_ptr.hpp>

class Sentence;
class SentenceTheory;
class SentenceTheoryBeam;
class ValueSet;
class ValueMentionSet;
class EventSet;
class RelationSet;
class RelMentionSet;
class SpeakerQuotationSet;
class ActorEntitySet;
class DocumentActorInfo;
class FactSet;
class ActorMentionSet;
class ICEWSEventMentionSet;

class DocPropForest;
typedef boost::shared_ptr<DocPropForest> DocPropForest_ptr;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED DocTheory : public Theory {
public:
	/** 
	  * Constructs a new DocTheory object associated
	  * with Document document.
	  */
	DocTheory(Document *document);

	/**
	 * Merges previously split DocTheory objects.
	 **/
	DocTheory(std::vector<DocTheory*> splitTheories);

	~DocTheory();
	
	Document* getDocument() const { return _document; }
	int getNSentences() const { return static_cast<int>(_sentences.size()); }

	enum { MIXED, UPPER, LOWER };
	int getDocumentCase() const;

	/** Populates the DocTheory's list of Sentences 
	  * associated with the document. DocTheory takes
	  * over sole-ownership of Sentences (but not the array of pointers
	  * you pass in), meaning that DocTheory will delete them,
	  * not you.
	  *
	  * @param n_sentences the number of sentences passed in
	  *	@param sentences an array of pointers to the sentences
	  */
	void setSentences(int n_sentences, Sentence **sentences);

	/** Returns the i-th Sentence from the document
	  * associated with this DocTheory.
	  *
	  * @param i the index of the Sentence to be returned
	  */
	const Sentence* getSentence(int i) const;

	/** Associates SentenceTheory at theory with this 
	  * DocTheory's i-th Sentence. DocTheory takes
	  * over sole ownership of the SentenceTheory.
	  *
	  * @param i the index of the Sentence the theory represents
	  * @param theory a pointer to the theory to be set
	  */
	void setSentenceTheory(int i, SentenceTheory *theory);
	
	/** Returns the SentenceTheory associated with the 
	  * i-th Sentence of the DocTheory.
	  *
	  * Note: returns null if a Theory does not yet
	  * exist for the i-th Sentence.
	  *
	  * @param i the index of the SentenceTheory to be returned
	  */
	class SentenceTheory* getSentenceTheory(int i) const;

	SentenceTheoryBeam* getSentenceTheoryBeam(int i);

	/** Associates the given SentenceTheoryBeam with this 
	  * DocTheory's i-th Sentence. DocTheory takes
	  * over sole ownership of the SentenceTheory.  If the
	  * DocTheory already had a SentenceTheoryBeam for this
	  * sentence, then it is deleted.
	  *
	  * @param i the index of the Sentence the theory represents
	  * @param theory a pointer to the theory to be set
	  */
	void setSentenceTheoryBeam(int i, SentenceTheoryBeam *beam);

	const SentenceTheoryBeam* getSentenceTheoryBeam(int i) const;


	/** Accessor to EntitySet of document theory:
	  * This is kept up-to-date when new sentence theories
	  * are added. This method is preferred to getting
	  * the entity set from the last sentence theory
	  */
	EntitySet *getEntitySet() const { return _entitySet; }

	void setEntitySet(EntitySet* new_set){
		if(_entitySet != NULL)	delete _entitySet; 
		_entitySet = new_set;
	}

	void setUTCoref(UTCoref *new_utcoref) {
	   if (_utcoref != NULL ) delete _utcoref;
	   _utcoref = new_utcoref;
	}

	UTCoref *getUTCoref() const { return _utcoref; }

	SpeakerQuotationSet *getSpeakerQuotationSet() const { return _speakerQuotationSet; }

	// these are not kept up to date (would be meaningless for relations, anyway)
	RelationSet *getRelationSet() const { return _relationSet; }
	EventSet *getEventSet() const { return _eventSet; }
	ValueSet *getValueSet() const { return _valueSet; }
	RelMentionSet *getDocumentRelMentionSet() const { return _documentRelMentionSet; }
	ValueMentionSet *getDocumentValueMentionSet() const { return _documentValueMentionSet; }
	ActorEntitySet *getActorEntitySet() const { return _actorEntitySet; }
	DocumentActorInfo *getDocumentActorInfo() const { return _documentActorInfo; }
	FactSet *getFactSet() const { return _factSet; }
	ActorMentionSet *getActorMentionSet() const { return _actorMentionSet; }
	ICEWSEventMentionSet *getICEWSEventMentionSet() const { return _icewsEventMentionSet; }
	
	// assumes ownership of set
	void takeValueSet(ValueSet *set);
	void takeRelationSet(RelationSet *set);
	void takeEventSet(EventSet *set);
	void takeDocumentRelMentionSet(RelMentionSet *set);
	void takeDocumentValueMentionSet(ValueMentionSet *set);
	void takeSpeakerQuotationSet(SpeakerQuotationSet *newSpeakerQuotationSet);
	void takeActorEntitySet(ActorEntitySet *set);
	void takeDocumentActorInfo(DocumentActorInfo *info);
	void takeFactSet(FactSet *set);
	void takeActorMentionSet(ActorMentionSet *set);
	void takeICEWSEventMentionSet(ICEWSEventMentionSet *set);
	
	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out,
									 const DocTheory &it)
		{ it.dump(out, 0); return out; }

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	/*
	//MRF: if load_sent_entity_sets = false, the document loader will not create an entity set for every sentence
	//	this is desirable for distillation 
	//		*sent-level-entity sets are unused 
	//		*loading time dominates AF time 
	//		*sent-level-enity-sets take ~16% of load time
	//  this will most likely break normal serif and trainers, so unless you're working in distillation (or trying to optimize state file loading for another reason, 
	//	leave load_sent_entity_sets = true
	*/
	void loadDocTheory(StateLoader *stateLoader,  bool load_sent_entity_sets = true);
	void resolvePointers(StateLoader * stateLoader);
	void loadSubtheory(StateLoader *stateLoader, SentenceTheory *sTheory, bool load_sent_entity_sets);
	void loadSentenceBreaksFromStateFile(StateLoader *stateLoader);
	Symbol loadFakedDocTheory(StateLoader *stateLoader, const wchar_t *state_tree_name, bool load_sent_entity_sets = true, int* sentenceStartOffsets = 0, int* sentenceEndOffsets = 0);
	void saveSentenceBreaksToStateFile(StateSaver *stateSaver, Symbol* docName = 0, int* sentenceStartOffsets = 0, int* sentenceEndOffsets = 0);
	Symbol getSourceType(){
		return _document->getSourceType();
	}
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit DocTheory(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;

	Metadata* getMetadata(){
		return _document->getMetadata();
	}
	const Metadata* getMetadata() const{
		return _document->getMetadata();
	}

	bool isSpeakerSentence(int sentno) const;
	bool isPostdateSentence(int sentno);
	bool isReceiverSentence(int sentno) const;

	/** Convenience method: return the entity that owns the given mention. 
	  * The entity is looked up in the document's entity set. */
	Entity* getEntityByMention(const Mention* ment) const;

	/** Convenience method: return the mention for a given MentionUID.
	 */
	const Mention* getMention(MentionUID ment) const;

	//ActorId getDocumentCountryActorId() const;

	void setDocumentCountryActorId(ActorId actor_id);

	/** Return the DocPropForest for this document.  The first time this method
	  * is called, it will construct a new DocPropForest for this document.  
	  * Subsequent calls will return the same forest, even if the DocTheory has
	  * been modified (unless force_reconstruction is true).  The DocPropForest
	  * is constructed using PropForestFactory::getDocumentForest() followed 
	  * by DistillationDocExpander::expandForest(). */
	DocPropForest_ptr getPropForest(bool force_reconstruction=true) const;

	/** Return the highest start stage that we should use for the
	 * given document, based on its contents.  E.g., if it already
	 * contains sentence segmentation but not tokenization, we'd
	 * return Stage("tokens"). */
	Stage getMaxStartStage() const;

	/** Register a new subtheory type that is associated with each document.
	  * This is used by feature modules to add support for new theory 
	  * types, in cases where it is not appropriate to include those theory
	  * types in the Generic library.  This must be called exactly once
	  * for each new subtheory type you wish to add, and must be called 
	  * before any calls to takeSubtheory<T> or getSubtheory<T>.  The 
	  * specified xmlTag is used for serifxml serialization and 
	  * deserialization. 
	  *
	  * Warning: state files currently ignore all subtheories that are added
	  * using registerSubtheoryType().
	  */
	template <typename TheoryType> 
	static void registerSubtheoryType(std::string xmlTag) {
		std::string typeName(typeid(TheoryType).name());
		ensureSubtheoryTypeIsNotRegistered(typeName);
		_subtheoryRecords().push_back(boost::shared_ptr<SubtheoryRecord>(_new SubtheoryRecordFor<TheoryType>(xmlTag)));
	}

	/** Set this document's subtheory with the specified type to the given
	  * value, and assume ownership of that subtheory.  This method should
	  * only be used for subtheory types that were registered by feature 
	  * modules using DocTheory::registerSubtheoryType(); for the "standard"
	  * document subtheories (such as EventSet and RelationSet), you should
	  * use the corresponding named methods instead (takeEventSet(), etc). */
	template<typename TheoryType> 
	void takeSubtheory(TheoryType *subtheory) {
		std::string typeName(typeid(TheoryType).name());
		ensureSubtheoryTypeIsRegistered(typeName);
		if (_subtheories[typeName])
			delete _subtheories[typeName];
		_subtheories[typeName] = subtheory;
	}

	/** Return the current value of the given subtheory for this document.
	  * This method should * only be used for subtheory types that were 
	  * registered by feature modules using DocTheory::registerSubtheoryType(); 
	  * for the "standard" document subtheories (such as EventSet and 
	  * RelationSet), you should use the corresponding named methods instead 
	  * (getEventSet(), etc). */
	template<typename TheoryType>
	TheoryType* getSubtheory() const {
		std::string typeName(typeid(TheoryType).name());
		ensureSubtheoryTypeIsRegistered(typeName);
		std::map<std::string, Theory*>::const_iterator it = _subtheories.find(typeName);
		return (it == _subtheories.end()) ? 0 : dynamic_cast<TheoryType*>(it->second);
	}

private:
	Document *_document;
	std::vector<const Sentence*> _sentences;
	std::vector<SentenceTheoryBeam*> _sentTheoryBeams;
	EntitySet *_entitySet;
	ValueSet *_valueSet;
	RelationSet *_relationSet;
	EventSet *_eventSet;
	RelMentionSet *_documentRelMentionSet;
	ValueMentionSet *_documentValueMentionSet;
	UTCoref *_utcoref;
	SpeakerQuotationSet *_speakerQuotationSet;
	ActorEntitySet *_actorEntitySet;
	DocumentActorInfo *_documentActorInfo;
	FactSet *_factSet;
	ActorMentionSet *_actorMentionSet;
	ICEWSEventMentionSet *_icewsEventMentionSet;

	mutable DocPropForest_ptr _propForest;
	
	void fixEntitySets();
	void fixSentenceSubtheoryPointers(SentenceTheory *sTheory);
	void saveLexicalEntries(SerifXML::XMLTheoryElement elem) const; // used by xml serialization
	void loadLexicalEntries(SerifXML::XMLTheoryElement elem); // used by xml serialization

	//======================================================================
	// Support for addding new document Subtheories from feature modules
	// (see registerSubtheoryType(), takeSubtheory(), and getSubtheory())
	struct SubtheoryRecord {
		std::string typeName;
		SerifXML::xstring xmlTag;
		SubtheoryRecord(std::string typeName, std::string xmlTag): typeName(typeName), xmlTag(SerifXML::transcodeToXString(xmlTag.c_str())) {}
		virtual Theory* loadXML(SerifXML::XMLTheoryElement documentElem, DocTheory *docTheory) = 0;
		virtual ~SubtheoryRecord() {}
	};
	template <typename TheoryType>
	struct SubtheoryRecordFor: public SubtheoryRecord {
		SubtheoryRecordFor(std::string xmlTag): SubtheoryRecord(typeid(TheoryType).name(), xmlTag) {}
		virtual Theory* loadXML(SerifXML::XMLTheoryElement documentElem, DocTheory *docTheory) {
			return _new TheoryType(documentElem, docTheory); 
		}
	};
	static void ensureSubtheoryTypeIsRegistered(const std::string& typeName);
	static void ensureSubtheoryTypeIsNotRegistered(const std::string& typeName);
	static std::vector<boost::shared_ptr<SubtheoryRecord> > &_subtheoryRecords();
	std::map<std::string, Theory*> _subtheories;
};

#endif
