// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "dynamic_includes/common/SerifRestrictions.h"
#include "theories/DocTheory.h"
#include "common/OutputUtil.h"
#include "common/InternalInconsistencyException.h"
#include "theories/Region.h"
#include "theories/Sentence.h"
#include "theories/SentenceTheory.h"
#include "theories/ValueMentionSet.h"
#include "theories/EntitySet.h"
#include "theories/EventSet.h"
#include "theories/RelationSet.h"
#include "theories/RelMentionSet.h"
#include "theories/Lexicon.h"
#include "theories/Lexicon.h"
#include "reader/DefaultDocumentReader.h"
#include "state/StateSaver.h"
#include "state/StateLoader.h"
#include "state/ObjectIDTable.h"
#include "state/ObjectPointerTable.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/morphAnalysis/SessionLexicon.h"

#include "Generic/PropTree/PropForestFactory.h"
#include "Generic/PropTree/expanders/DistillationExpander.h"

// Stage-specific subtheories
#include "theories/TokenSequence.h"
#include "theories/LexicalTokenSequence.h"
#include "theories/PartOfSpeechSequence.h"
#include "theories/NameTheory.h"
#include "theories/NestedNameTheory.h"
#include "theories/ValueSet.h"
#include "theories/Parse.h"
#include "theories/NPChunkTheory.h"
#include "theories/DependencyParseTheory.h"
#include "theories/MentionSet.h"
#include "theories/PropositionSet.h"
#include "theories/EntitySet.h"
#include "theories/RelMentionSet.h"
#include "theories/EventMentionSet.h"
#include "theories/UTCoref.h"
#include "theories/SpeakerQuotationSet.h"
#include "theories/ActorEntitySet.h"
#include "theories/DocumentActorInfo.h"
#include "theories/FactSet.h"
#include "theories/SentenceTheory.h"
#include "theories/ActorMentionSet.h"
#include "Generic/icews/ICEWSEventMentionSet.h"

// Need this to generate IDs for serialization, due to forward declarations
#include "theories/RelMention.h"

#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

#include "Generic/reader/DefaultDocumentReader.h"

#include "Generic/common/ParamReader.h"
#include <boost/foreach.hpp>

#include <boost/unordered_map.hpp>

#if defined(_WIN32)
#define snprintf _snprintf
#endif

static Symbol POSTDATE_SYM = Symbol(L"POSTDATE");

DocTheory::DocTheory(Document *document)
	: _document(document), 
	  _entitySet(0), _utcoref(0), _valueSet(0), _relationSet(0), _documentRelMentionSet(0), _eventSet(0), 
	_documentValueMentionSet(0), _speakerQuotationSet(0), _actorEntitySet(0), _documentActorInfo(0), _factSet(0), 
	_actorMentionSet(0), _icewsEventMentionSet(0)
{}

DocTheory::DocTheory(std::vector<DocTheory*> splitDocTheories) : _utcoref(0) {
	// Merge the documents by appending Regions, and assuming their original text is the same
	std::vector<Document*> splitDocuments;
	BOOST_FOREACH(DocTheory* splitDocTheory, splitDocTheories) {
		splitDocuments.push_back(splitDocTheory->getDocument());
	}
	_document = _new Document(splitDocuments);

	// Accumulate sentence theories, tracking various document-level things
	int sentence_offset = 0;
	int region_offset = 0;
	int entity_offset = 0;
	int event_offset = 0;
	std::vector<int> sentenceOffsets;
	std::vector<int> entityOffsets;
	std::vector<SentenceTheory*> mergedSentenceTheories;
	std::vector<ValueSet*> splitValueSets;
	std::vector<ValueMentionSet*> mergedValueMentionSets;
	std::vector<MentionSet*> mergedMentionSets;
	std::vector<EntitySet*> splitEntitySets;
	std::vector<EventSet*> splitEventSets;
	std::vector<EventMentionSet*> mergedEventMentionSets;
	std::vector<RelationSet*> splitRelationSets;
	std::vector<RelMentionSet*> mergedRelMentionSets;
	std::vector<RelMentionSet*> splitDocRelMentionSets;
	std::vector<ValueMentionSet*> splitDocValueMentionSets;
	std::vector<SpeakerQuotationSet*> splitSpeakerQuotationSets;
	std::vector<ActorEntitySet*> splitActorEntitySets;
	std::vector<DocumentActorInfo*> splitDocumentActorInfo;
	std::vector<FactSet*> splitFactSets;
	std::vector<ActorMentionSet*> splitActorMentionSets;
	std::vector<ICEWSEventMentionSet*> splitIcewsEventMentionSets;
	bool hasValueSet = false;
	bool hasEntitySet = false;
	bool hasEventSet = false;
	bool hasRelationSet = false;
	bool hasDocRelMentionSet = false;
	bool hasDocValueMentionSet = false;
	bool hasSpeakerQuotationSet = false;
	bool hasActorEntitySet = false;
	bool hasDocumentActorInfo = false;
	bool hasFactSet = false;
	bool hasActorMentionSet = false;
	bool hasIcewsEventMentionSet = false;

	// Store sentence offsets and document ValueMentionSets
	int total_merged_sentences = 0;
	sentenceOffsets.push_back(0);
	for (size_t i = 0; i < splitDocTheories.size(); i++) {
		DocTheory* splitDocTheory = splitDocTheories[i];
		if (i != splitDocTheories.size() - 1) // we added 0 before the loop, so the size of sentenceOffsets will be same size as splitDocTheories
			sentenceOffsets.push_back(sentenceOffsets.at(sentenceOffsets.size() - 1) + splitDocTheories[i]->getNSentences());
		total_merged_sentences += splitDocTheories[i]->getNSentences();
		splitDocValueMentionSets.push_back(splitDocTheory->getDocumentValueMentionSet());
		if (splitDocTheory->getDocumentValueMentionSet())
			hasDocValueMentionSet = true;
	}

	// Create document ValueMentionSet before sentence level 
	// because the sentence level EventMention can point 
	// to the ValueMentions within. We keep a mapping between 
	// split ValueMentions and merged ValueMentions to be used in
	// merged EventMention and Value creation.
	std::vector<ValueMentionSet::ValueMentionMap> documentValueMentionMaps;
	if (hasDocValueMentionSet)
		_documentValueMentionSet = _new ValueMentionSet(splitDocValueMentionSets, sentenceOffsets, documentValueMentionMaps, total_merged_sentences);
	else
		_documentValueMentionSet = NULL;

	for (size_t i = 0; i < splitDocTheories.size(); i++) {
		DocTheory* splitDocTheory = splitDocTheories[i];
		for (int s = 0; s < splitDocTheory->getNSentences(); s++) {
			// Copy the sentence and attach it to the correct copied Region
			const Sentence* splitSentence = splitDocTheory->getSentence(s);
			int r;
			for (r = 0; r < splitDocTheory->getDocument()->getNRegions(); r++)
				if (splitDocTheory->getDocument()->getRegions()[r] == splitSentence->getRegion())
					break;
			Sentence* mergedSentence = _new Sentence(_document, _document->getRegions()[region_offset + r], sentence_offset + s, splitSentence->getString());
			_sentences.push_back(mergedSentence);

			// Deep copy each SentenceTheory in the beam
			SentenceTheoryBeam* splitSentenceBeam = splitDocTheory->getSentenceTheoryBeam(s);
			SentenceTheoryBeam* mergedSentenceBeam = _new SentenceTheoryBeam(mergedSentence, splitSentenceBeam->getNTheories());
			for (int b = 0; b < splitSentenceBeam->getNTheories(); b++) {
				// Build a new SentenceTheory by copying subtheories and offsetting sentences as needed
				SentenceTheory* splitSentenceTheory = splitSentenceBeam->getTheory(b);
				SentenceTheory* mergedSentenceTheory = _new SentenceTheory(mergedSentence, splitSentenceTheory->getPrimaryParseSym(), _document->getName());

				// If we ever change SubtheoryTypes, we need to add to/reorder this block
				TokenSequence* splitTokenSequence = splitSentenceTheory->getTokenSequence();
				if (splitTokenSequence != NULL) {
					// Most SentenceSubtheory types depend on the token sequence
					TokenSequence* mergedTokenSequence = _new TokenSequence(*splitTokenSequence, sentence_offset);
					mergedSentenceTheory->adoptSubtheory(SentenceTheory::TOKEN_SUBTHEORY, mergedTokenSequence);

					PartOfSpeechSequence* splitPOSSequence = splitSentenceTheory->getPartOfSpeechSequence();
					if (splitPOSSequence != NULL) {
						PartOfSpeechSequence* mergedPOSSequence = _new PartOfSpeechSequence(*splitPOSSequence, mergedTokenSequence);
						mergedSentenceTheory->adoptSubtheory(SentenceTheory::POS_SUBTHEORY, mergedPOSSequence);
					}

					NameTheory* splitNameTheory = splitSentenceTheory->getNameTheory();
					if (splitNameTheory != NULL) {
						NameTheory* mergedNameTheory = _new NameTheory(*splitNameTheory, mergedTokenSequence);
						mergedSentenceTheory->adoptSubtheory(SentenceTheory::NAME_SUBTHEORY, mergedNameTheory);
					}

					// Document Value pointers are set later
					ValueMentionSet* splitValueMentionSet = splitSentenceTheory->getValueMentionSet();
					if (splitValueMentionSet != NULL) {
						ValueMentionSet* mergedValueMentionSet = _new ValueMentionSet(*splitValueMentionSet, mergedTokenSequence);
						mergedSentenceTheory->adoptSubtheory(SentenceTheory::VALUE_SUBTHEORY, mergedValueMentionSet);
					}

					// Only one of these, the primary parse, will be non-null, MentionSet depends on it
					NPChunkTheory* splitNPChunkTheory = splitSentenceTheory->getNPChunkTheory();
					if (splitNPChunkTheory != NULL) {
						NPChunkTheory* mergedNPChunkTheory = _new NPChunkTheory(*splitNPChunkTheory, mergedTokenSequence);
						mergedSentenceTheory->adoptSubtheory(SentenceTheory::NPCHUNK_SUBTHEORY, mergedNPChunkTheory);
					}
					Parse* splitParse = splitSentenceTheory->getFullParse();
					if (splitParse != NULL) {
						Parse* mergedParse = _new Parse(*splitParse, mergedTokenSequence);
						mergedSentenceTheory->adoptSubtheory(SentenceTheory::PARSE_SUBTHEORY, mergedParse);
					}
					DependencyParseTheory* splitDependencyParseTheory = splitSentenceTheory->getDependencyParseTheory();
					if (splitDependencyParseTheory != NULL) {
						DependencyParseTheory* mergedDependencyParseTheory = _new DependencyParseTheory(*splitDependencyParseTheory, mergedTokenSequence);
						mergedSentenceTheory->adoptSubtheory(SentenceTheory::DEPENDENCY_PARSE_SUBTHEORY, mergedDependencyParseTheory);
					}

					// Remaining sentence theories depend on Mentions
					MentionSet* splitMentionSet = splitSentenceTheory->getMentionSet();
					if (splitMentionSet != NULL) {
						MentionSet* mergedMentionSet = _new MentionSet(*splitMentionSet, sentence_offset, mergedSentenceTheory->getPrimaryParse());
						mergedSentenceTheory->adoptSubtheory(SentenceTheory::MENTION_SUBTHEORY, mergedMentionSet);

						PropositionSet* splitPropositionSet = splitSentenceTheory->getPropositionSet();
						if (splitPropositionSet != NULL) {
							PropositionSet* mergedPropositionSet = _new PropositionSet(*splitPropositionSet, sentence_offset, mergedMentionSet);
							mergedSentenceTheory->adoptSubtheory(SentenceTheory::PROPOSITION_SUBTHEORY, mergedPropositionSet);
						}

						// We assume that split/merge is end-to-end, and we don't preserve sentence-level EntitySets because they get giant

						RelMentionSet* splitRelMentionSet = splitSentenceTheory->getRelMentionSet();
						if (splitRelMentionSet != NULL) {
							RelMentionSet* mergedRelMentionSet = _new RelMentionSet(*splitRelMentionSet, sentence_offset, mergedMentionSet, mergedSentenceTheory->getValueMentionSet());
							mergedSentenceTheory->adoptSubtheory(SentenceTheory::RELATION_SUBTHEORY, mergedRelMentionSet);
						}

						EventMentionSet* splitEventMentionSet = splitSentenceTheory->getEventMentionSet();
						if (splitEventMentionSet != NULL) {
							EventMentionSet* mergedEventMentionSet = _new EventMentionSet(*splitEventMentionSet, sentence_offset, event_offset, mergedSentenceTheory->getPrimaryParse(), mergedMentionSet, mergedSentenceTheory->getValueMentionSet(), mergedSentenceTheory->getPropositionSet(), _documentValueMentionSet, documentValueMentionMaps[i]);
							mergedSentenceTheory->adoptSubtheory(SentenceTheory::EVENT_SUBTHEORY, mergedEventMentionSet);
						}
					}
				}

				// Add this deep-copied sentence theory to the merged beam for this sentence
				mergedSentenceBeam->addTheory(mergedSentenceTheory);
			}

			// Accumulate mentions so we can do lookup at the document level
			mergedValueMentionSets.push_back(mergedSentenceBeam->getBestTheory()->getValueMentionSet());
			mergedMentionSets.push_back(mergedSentenceBeam->getBestTheory()->getMentionSet());
			mergedEventMentionSets.push_back(mergedSentenceBeam->getBestTheory()->getEventMentionSet());
			mergedRelMentionSets.push_back(mergedSentenceBeam->getBestTheory()->getRelMentionSet());
			mergedSentenceTheories.push_back(mergedSentenceBeam->getBestTheory());

			// Link it all up to the new document theory
			_sentTheoryBeams.push_back(mergedSentenceBeam);
		}

		// Track counts so we can offset IDs properly
		sentence_offset += splitDocTheory->getNSentences();
		region_offset += splitDocTheory->getDocument()->getNRegions();
		if (splitDocTheory->getEventSet() != NULL)
			event_offset += splitDocTheory->getEventSet()->getNEvents();
		entityOffsets.push_back(entity_offset);
		if (splitDocTheory->getEntitySet() != NULL)
			entity_offset += splitDocTheory->getEntitySet()->getNEntities();

		// Accumulate document-level theories
		splitValueSets.push_back(splitDocTheory->getValueSet());
		splitEntitySets.push_back(splitDocTheory->getEntitySet());
		splitEventSets.push_back(splitDocTheory->getEventSet());
		splitRelationSets.push_back(splitDocTheory->getRelationSet());
		splitDocRelMentionSets.push_back(splitDocTheory->getDocumentRelMentionSet());
		splitSpeakerQuotationSets.push_back(splitDocTheory->getSpeakerQuotationSet());
		splitActorEntitySets.push_back(splitDocTheory->getActorEntitySet());
		splitDocumentActorInfo.push_back(splitDocTheory->getDocumentActorInfo());
		splitFactSets.push_back(splitDocTheory->getFactSet());
		splitActorMentionSets.push_back(splitDocTheory->getActorMentionSet());
		splitIcewsEventMentionSets.push_back(splitDocTheory->getICEWSEventMentionSet());

		// Check if we have at least one non-NULL theory of each type
		if (splitDocTheory->getValueSet())
			hasValueSet = true;
		if (splitDocTheory->getEntitySet())
			hasEntitySet = true;
		if (splitDocTheory->getEventSet())
			hasEventSet = true;
		if (splitDocTheory->getRelationSet())
			hasRelationSet = true;
		if (splitDocTheory->getDocumentRelMentionSet())
			hasDocRelMentionSet = true;
		if (splitDocTheory->getSpeakerQuotationSet())
			hasSpeakerQuotationSet = true;
		if (splitDocTheory->getActorEntitySet())
			hasActorEntitySet = true;
		if (splitDocTheory->getDocumentActorInfo())
			hasDocumentActorInfo = true;
		if (splitDocTheory->getFactSet())
			hasFactSet = true;
		if (splitDocTheory->getActorMentionSet())
			hasActorMentionSet = true;
		if (splitDocTheory->getICEWSEventMentionSet())
			hasIcewsEventMentionSet = true;
	}

	// Deep copy the various sets with references, if necessary
	if (hasValueSet)
		_valueSet = _new ValueSet(splitValueSets, sentenceOffsets, mergedValueMentionSets, _documentValueMentionSet, documentValueMentionMaps);
	else
		_valueSet = NULL;
	if (hasEntitySet)
		_entitySet = _new EntitySet(splitEntitySets, sentenceOffsets, mergedMentionSets);
	else
		_entitySet = NULL;
	if (hasEventSet)
		_eventSet = _new EventSet(splitEventSets, sentenceOffsets, mergedEventMentionSets);
	else
		_eventSet = NULL;
	if (hasRelationSet)
		_relationSet = _new RelationSet(splitRelationSets, entityOffsets, sentenceOffsets, mergedRelMentionSets);
	else
		_relationSet = NULL;
	if (hasDocRelMentionSet)
		_documentRelMentionSet = _new RelMentionSet(splitDocRelMentionSets, sentenceOffsets, mergedMentionSets);
	else
		_documentRelMentionSet = NULL;
	if (hasSpeakerQuotationSet)
		_speakerQuotationSet = _new SpeakerQuotationSet(splitSpeakerQuotationSets, sentenceOffsets, mergedMentionSets);
	else
		_speakerQuotationSet = NULL;
	if (hasActorEntitySet)
		// ncw - 2013.11.25 - Merging ActorEntitySets not yet supported, merged doc will have an empty one
		_actorEntitySet = _new ActorEntitySet();
	else
		_actorEntitySet = NULL;
	if (hasFactSet)
		// ahz - 2014.03.13- Merging FactSets not yet supported, merged doc will have an empty one
		_factSet = _new FactSet();
	else
		_factSet = NULL;

	// mapping of split ActorMentions to mergedActorMention, will be needed to produce the ICEWSEventMentions
	boost::unordered_map<ActorMention_ptr, ActorMention_ptr> actorMentionMap;
	if (hasActorMentionSet)
		_actorMentionSet = _new ActorMentionSet(splitActorMentionSets, sentenceOffsets, mergedMentionSets, mergedSentenceTheories, actorMentionMap);
	else
		_actorMentionSet = NULL;

	if (hasIcewsEventMentionSet)
		_icewsEventMentionSet = _new ICEWSEventMentionSet(splitIcewsEventMentionSets, actorMentionMap);
	else
		_icewsEventMentionSet = NULL;

	// ahz - 2014.03.13- Merging DocumentActorInfo not yet supported, merged doc will not have one
	_documentActorInfo = NULL;
}

DocTheory::~DocTheory() {
	for (size_t i = 0; i < _sentTheoryBeams.size(); i++)
		delete _sentTheoryBeams[i];
	for (size_t i = 0; i < _sentences.size(); i++)
		delete _sentences[i];
	delete _entitySet;
	delete _utcoref;
	delete _valueSet;
	delete _eventSet;
	delete _relationSet;
	delete _documentRelMentionSet;
	delete _documentValueMentionSet;
	delete _speakerQuotationSet;
	delete _actorEntitySet;
	delete _documentActorInfo;
	delete _factSet;
	delete _actorMentionSet;
	delete _icewsEventMentionSet;
	typedef std::pair<std::string, Theory* > SubtheoryPair;
	BOOST_FOREACH(SubtheoryPair subtheory_pair, _subtheories) {
		delete subtheory_pair.second;
	}
}

void DocTheory::takeValueSet(ValueSet *set) {
	delete _valueSet;
	_valueSet = set;
}

void DocTheory::takeEventSet(EventSet *set) {
	delete _eventSet;
	_eventSet = set;
}

void DocTheory::takeRelationSet(RelationSet *set) { 
	delete _relationSet;
	_relationSet = set; 
}

void DocTheory::takeDocumentRelMentionSet(RelMentionSet *set) {
	delete _documentRelMentionSet;
	_documentRelMentionSet = set;
}

void DocTheory::takeDocumentValueMentionSet(ValueMentionSet *set) {
	delete _documentValueMentionSet;
	_documentValueMentionSet = set;
}


void DocTheory::takeSpeakerQuotationSet(SpeakerQuotationSet *newSpeakerQuotationSet) {
   delete _speakerQuotationSet;
   _speakerQuotationSet = newSpeakerQuotationSet;
}

void DocTheory::takeActorEntitySet(ActorEntitySet *set) {
	delete _actorEntitySet;
	_actorEntitySet = set;
}

void DocTheory::takeDocumentActorInfo(DocumentActorInfo *info) {
	delete _documentActorInfo;
	_documentActorInfo = info;
}

void DocTheory::takeFactSet(FactSet *set) {
	delete _factSet;
	_factSet = set;
}

void DocTheory::takeActorMentionSet(ActorMentionSet *set) {
	delete _actorMentionSet;
	_actorMentionSet = set;
}

void DocTheory::takeICEWSEventMentionSet(ICEWSEventMentionSet *set) {
	delete _icewsEventMentionSet;
	_icewsEventMentionSet = set;
}


void DocTheory::setSentences(int n_sentences, Sentence **sentences) {
	if (_sentences.size() == 0) {
		for (int i=0; i < n_sentences; ++i) {
			if (sentences != 0) {
				_sentences.push_back(sentences[i]);
			} else {
				_sentences.push_back(0);
			}
			_sentTheoryBeams.push_back(0);
		}
	} else {
		throw InternalInconsistencyException("DocTheory::setSentences()", "Sentences already set");
	}
}

const Sentence* DocTheory::getSentence(int i) const {
	if ((0<=i) && (i < getNSentences()))
		return _sentences[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"DocTheory::getSentence()", getNSentences(), i);
}

void DocTheory::setSentenceTheory(int i, SentenceTheory *theory) {
	if ((0<=i) && (i < getNSentences())) {
		if (_sentTheoryBeams[i] == 0) {
			// assign sentence theory
			_sentTheoryBeams[i] = _new SentenceTheoryBeam(_sentences[i]);
			_sentTheoryBeams[i]->addTheory(theory);
		}
		else
			throw UnrecoverableException("DocTheory::setSentTheories()", "SentTheory already set");
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"DocTheory::setSentTheory()", getNSentences(), i);
	}
}

SentenceTheory* DocTheory::getSentenceTheory(int i) const {
	if ((0<=i) && (i < getNSentences())) {
		if ((_sentTheoryBeams[i] != 0) && (_sentTheoryBeams[i]->getBestTheory() != 0))
			return _sentTheoryBeams[i]->getBestTheory();
		else
			return NULL;
			/*throw InternalInconsistencyException("DocTheory::getSentenceTheory",
				"Sentence theory has not been set");*/
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"DocTheory::getSentTheory()", getNSentences(), i);
	}
}

void DocTheory::setSentenceTheoryBeam(int i, SentenceTheoryBeam *beam) {
	if ((0<=i) && (i < getNSentences())) {
		if (beam != _sentTheoryBeams[i]) {
			delete _sentTheoryBeams[i];
			_sentTheoryBeams[i] = beam;
		}
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"DocTheory::setSentTheory()", getNSentences(), i);
	}
}

SentenceTheoryBeam* DocTheory::getSentenceTheoryBeam(int i) {
	if ((unsigned) i < (unsigned) getNSentences())
		return _sentTheoryBeams[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"DocTheory::getSentTheoryBeam()", getNSentences(), i);
}

const SentenceTheoryBeam* DocTheory::getSentenceTheoryBeam(int i) const {
	if ((unsigned) i < (unsigned) getNSentences())
		return _sentTheoryBeams[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"DocTheory::getSentTheoryBeam()", getNSentences(), i);
}

void DocTheory::dump(std::ostream &out, int indent) const {

	#ifdef BLOCK_FULL_SERIF_OUTPUT
	
	throw UnexpectedInputException("DocTheory::dump", "Dump theories not supported");

	#endif


	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Document Theory "
		<< "(document " << _document->getName().to_debug_string() << "):\n";
	
	for (int i = 0; i < getNSentences(); i++) {
		if (_sentTheoryBeams[i] == 0)
			out << "No theory returned!\n";
		else if (_sentTheoryBeams[i]->getBestTheory() == 0)
			out << "No theory returned!\n";
		else
			_sentTheoryBeams[i]->getBestTheory()->dump(out, indent + 2);
		out << "\n\n";
	}

	delete[] newline;
}

int DocTheory::getDocumentCase() const {
	bool is_all_upper = true;
	bool is_all_lower = true;
	for (int j = 0; j < getNSentences(); j++) {
		const LocatedString *sent = getSentence(j)->getString();
		for (int k = 0; k < sent->length(); k++) {
			if (iswupper(sent->charAt(k))) {
				is_all_lower = false;
				if (!is_all_upper)
					return MIXED;
			} else if (iswlower(sent->charAt(k))) {
				is_all_upper = false;
				if (!is_all_lower)
					return MIXED;
			} 
		}
	}

	if (is_all_lower)
		return LOWER;
	else if (is_all_upper)
		return UPPER;
	else return MIXED;
}



void DocTheory::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);

	if (_entitySet != 0)
		_entitySet->updateObjectIDTable();
	if (_utcoref != 0)
	   _utcoref->updateObjectIDTable();
	if (_relationSet != 0)
		_relationSet->updateObjectIDTable();
	if (_eventSet != 0)
		_eventSet->updateObjectIDTable();
	if (_documentRelMentionSet != 0)
		_documentRelMentionSet->updateObjectIDTable();
	if (_valueSet != 0)
		_valueSet->updateObjectIDTable();
	if (_documentValueMentionSet != 0)
		_documentValueMentionSet->updateObjectIDTable();

	for (int i = 0; i < getNSentences(); i++) {
		_sentTheoryBeams[i]->updateObjectIDTable();
	}
}

void DocTheory::saveState(StateSaver *stateSaver) const {

	stateSaver->beginList(L"DocTheory", this);

	stateSaver->saveInteger(getNSentences());

	stateSaver->beginList(L"DocTheory::_sentTheories");
	
	for (int i = 0; i < getNSentences(); i++) {
		SentenceTheory *sentTheory = _sentTheoryBeams[i]->getBestTheory();
		stateSaver->beginList(L"SentenceTheory");
		stateSaver->saveSymbol(sentTheory->getPrimaryParseSym());
		stateSaver->saveSymbol(sentTheory->getDocID());

		// subtheories
		stateSaver->beginList(L"SentenceTheory::subtheories");
		int n_subtheories = 0;
		for (int j = 0; j < SentenceTheory::N_SUBTHEORY_TYPES; j++) { 
			if (sentTheory->getSubtheory(j) != 0) 
				n_subtheories++;
		}
		stateSaver->saveInteger(n_subtheories);
		for (int k = 0; k < SentenceTheory::N_SUBTHEORY_TYPES; k++) { 
			if (sentTheory->getSubtheory(k) != 0) {
				stateSaver->saveInteger(sentTheory->getSubtheory(k)->getSubtheoryType());
				sentTheory->getSubtheory(k)->saveState(stateSaver);
			}
		}
		stateSaver->endList();

		stateSaver->endList();
	}
	stateSaver->endList();

	if (_entitySet == 0) {
		stateSaver->saveInteger(0);
	} else {
		stateSaver->saveInteger(1);
		_entitySet->saveState(stateSaver);
	}

	if (ParamReader::isParamTrue("utcoref_enable")) {
	   if (_utcoref == 0) {
		  stateSaver->saveInteger(0);
	   } else {
		  stateSaver->saveInteger(1);
		  _utcoref->saveState(stateSaver);
	   }
	}

	if (_relationSet == 0) {
		stateSaver->saveInteger(0);
	} else {
		stateSaver->saveInteger(1);
		_relationSet->saveState(stateSaver);
	}

	if (_eventSet == 0) {
		stateSaver->saveInteger(0);
	} else {
		stateSaver->saveInteger(1);
		_eventSet->saveState(stateSaver);
	}

	if (_documentRelMentionSet == 0) {
		stateSaver->saveInteger(0);
	} else {
		stateSaver->saveInteger(1);
		_documentRelMentionSet->saveState(stateSaver);
	}

	if (_valueSet == 0) {
		stateSaver->saveInteger(0);
	} else {
		stateSaver->saveInteger(1);
		_valueSet->saveState(stateSaver);
	}

	if (_documentValueMentionSet == 0) {
		stateSaver->saveInteger(0);
	} else {
		stateSaver->saveInteger(1);
		_documentValueMentionSet->saveState(stateSaver);
	}

	if (getSpeakerQuotationSet()) {
		SessionLogger::warn("save_state") << "SpeakerQuotationSet is not currently serialized by DocTheory::saveState()" << std::endl;
	}

	if (_actorEntitySet) {
		SessionLogger::warn("save_state") << "ActorEntitySet is not currently serialized by DocTheory::saveState()" << std::endl;
	}
	if (_documentActorInfo) {
		SessionLogger::warn("save_state") << "DocumentActorInfo is not currently serialized by DocTheory::saveState()" << std::endl;
	}
	if (_factSet) {
		SessionLogger::warn("save_state") << "FactSet is not currently serialized by DocTheory::saveState()" << std::endl;
	}
	if (_actorMentionSet) {
		SessionLogger::warn("save_state") << "ActorMentionSet is not currently serialized by DocTheory::saveState()" << std::endl;
	}
	if (_icewsEventMentionSet) {
		SessionLogger::warn("save_state") << "ICEWSEventMentionSet is not currently serialized by DocTheory::saveState()" << std::endl;
	}

	stateSaver->endList();
}

// Requires: setSentences() must already have been called.
void DocTheory::loadDocTheory(StateLoader *stateLoader, bool load_sent_entity_sets) {
	if (_sentences.size() == 0)
		SessionLogger::warn("load_doc_theory") << "DocTheory::loadDocTheory() _sentences.size() == 0. "
										"Either you have a blank input document or else setSentences() must be called before calling loadDocTheory().";

	int id = stateLoader->beginList(L"DocTheory");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	if (stateLoader->loadInteger() != getNSentences())
		throw InternalInconsistencyException("DocTheory::loadDocTheory",
			"Number of sentence theories does not match number of sentences"); 
	
	stateLoader->beginList(L"DocTheory::_sentTheories");
	for (int i = 0; i < getNSentences(); i++) {
		stateLoader->beginList(L"SentenceTheory");
		Symbol pparse = stateLoader->loadSymbol();
		Symbol docid = stateLoader->loadSymbol();
		SentenceTheory *sentTheory = _new SentenceTheory(_sentences[i], pparse, docid);
		// subtheories
		stateLoader->beginList(L"SentenceTheory::subtheories");
		int n_subtheories = stateLoader->loadInteger();
		for (int j = 0; j < n_subtheories; j++) {
			loadSubtheory(stateLoader, sentTheory, load_sent_entity_sets);
		}

		stateLoader->endList();
		stateLoader->endList();
		setSentenceTheory(i, sentTheory);
	}
	stateLoader->endList();
	if (stateLoader->loadInteger())
		_entitySet = _new EntitySet(stateLoader);
	if (ParamReader::isParamTrue("utcoref_enable")) {
	   if (stateLoader->loadInteger()) {
		  _utcoref = _new UTCoref(stateLoader);
	   }
    }
	if (stateLoader->loadInteger())
		_relationSet = _new RelationSet(stateLoader);
	if (stateLoader->loadInteger())
		_eventSet = _new EventSet(stateLoader);
	if (stateLoader->loadInteger())
		_documentRelMentionSet = _new RelMentionSet(stateLoader);
	if (stateLoader->loadInteger()) 
		_valueSet = _new ValueSet(stateLoader);
	if (stateLoader->loadInteger()) 
		_documentValueMentionSet = _new ValueMentionSet(stateLoader);


	stateLoader->endList();
}

void DocTheory::loadSubtheory(StateLoader *stateLoader, 
							  SentenceTheory *sTheory, 
							  bool load_sent_entity_sets)
{
	SentenceTheory::SubtheoryType subtheoryType = 
		(SentenceTheory::SubtheoryType) stateLoader->loadInteger();
	if (subtheoryType == SentenceTheory::TOKEN_SUBTHEORY) {
		if (TokenSequence::getTokenSequenceTypeForStateLoading() == TokenSequence::LEXICAL_TOKEN_SEQUENCE)
			sTheory->adoptSubtheory(subtheoryType, _new LexicalTokenSequence(stateLoader));
		else
			sTheory->adoptSubtheory(subtheoryType, _new TokenSequence(stateLoader));
	} else if (subtheoryType == SentenceTheory::POS_SUBTHEORY)
		sTheory->adoptSubtheory(subtheoryType, _new PartOfSpeechSequence(stateLoader));
	else if (subtheoryType == SentenceTheory::NAME_SUBTHEORY)
		sTheory->adoptSubtheory(subtheoryType, _new NameTheory(stateLoader));
	else if (subtheoryType == SentenceTheory::NESTED_NAME_SUBTHEORY) 
		sTheory->adoptSubtheory(subtheoryType, _new NestedNameTheory(stateLoader));
	else if (subtheoryType == SentenceTheory::VALUE_SUBTHEORY)
		sTheory->adoptSubtheory(subtheoryType, _new ValueMentionSet(stateLoader));
	else if (subtheoryType == SentenceTheory::NPCHUNK_SUBTHEORY)
		sTheory->adoptSubtheory(subtheoryType, _new NPChunkTheory(stateLoader));
	else if (subtheoryType == SentenceTheory::DEPENDENCY_PARSE_SUBTHEORY)
		sTheory->adoptSubtheory(subtheoryType, _new DependencyParseTheory(stateLoader));
	else if (subtheoryType == SentenceTheory::PARSE_SUBTHEORY)
		sTheory->adoptSubtheory(subtheoryType, _new Parse(stateLoader));
	else if (subtheoryType == SentenceTheory::MENTION_SUBTHEORY)
		sTheory->adoptSubtheory(subtheoryType, _new MentionSet(stateLoader));
	else if (subtheoryType == SentenceTheory::PROPOSITION_SUBTHEORY)
		sTheory->adoptSubtheory(subtheoryType, _new PropositionSet(stateLoader));
	else if (subtheoryType == SentenceTheory::ENTITY_SUBTHEORY){
		if(load_sent_entity_sets){
			sTheory->adoptSubtheory(subtheoryType, _new EntitySet(stateLoader));
		} else {
			EntitySet::FakeEntitySet(stateLoader);
            sTheory->adoptSubtheory(subtheoryType, _new EntitySet(0));
		}
	}
	else if (subtheoryType == SentenceTheory::EVENT_SUBTHEORY)
		sTheory->adoptSubtheory(subtheoryType, _new EventMentionSet(stateLoader));
	else if (subtheoryType == SentenceTheory::RELATION_SUBTHEORY)
		sTheory->adoptSubtheory(subtheoryType, _new RelMentionSet(stateLoader));
	else
		throw UnexpectedInputException("DocTheory::loadSubtheory()", "Invalid subtheory type");
}

// This is necessary when we're loading an old version of the state file,
// because we formerly didn't keep pointers between subtheories.  This
// reconstructs them.
void DocTheory::fixSentenceSubtheoryPointers(SentenceTheory *sTheory) {
	const TokenSequence *tokseq = sTheory->getTokenSequence();
	if (tokseq) {
		if (sTheory->getPartOfSpeechSequence())
			sTheory->getPartOfSpeechSequence()->setTokenSequence(tokseq);
		if (sTheory->getNameTheory())
			sTheory->getNameTheory()->setTokenSequence(tokseq);
		if (sTheory->getValueMentionSet())
			sTheory->getValueMentionSet()->setTokenSequence(tokseq);
		if (sTheory->getNPChunkTheory())
			sTheory->getNPChunkTheory()->setTokenSequence(tokseq);
		if (sTheory->getFullParse())
			sTheory->getFullParse()->setTokenSequence(tokseq);
		if (sTheory->getPropositionSet())
			sTheory->getPropositionSet()->setMentionSet(sTheory->getMentionSet());
	}
}

void DocTheory::resolvePointers(StateLoader * stateLoader) {
	// We explicitly do *not* call resolvePointers on the SentenceTheory objects
	// themselves -- DocTheory::loadSubtheory() created them with actual pointers,
	// not serialized pseudo-pointers.
	for (int i = 0; i < getNSentences(); i++) {
		for (int k = 0; k < SentenceTheory::N_SUBTHEORY_TYPES; k++) { 
			if ((_sentTheoryBeams.size() != 0) && (getSentenceTheory(i) != 0) && (getSentenceTheory(i)->getSubtheory(k) != 0)) {
				getSentenceTheory(i)->getSubtheory(k)->resolvePointers(stateLoader);
			}
		}
	}
	if (_entitySet != 0)
		_entitySet->resolvePointers(stateLoader);
	if (_utcoref != 0)
	   _utcoref -> resolvePointers(stateLoader);
	if (_relationSet != 0)
		_relationSet->resolvePointers(stateLoader);
	if (_eventSet != 0)
		_eventSet->resolvePointers(stateLoader);
	if (_documentRelMentionSet != 0)
		_documentRelMentionSet->resolvePointers(stateLoader);
	if (_valueSet != 0)
		_valueSet->resolvePointers(stateLoader);
	if (_documentValueMentionSet != 0)
		_documentValueMentionSet->resolvePointers(stateLoader);

	if (stateLoader->getVersion() <= std::make_pair(1,5)) {
		for (int i = 0; i < getNSentences(); i++) {
			fixSentenceSubtheoryPointers(getSentenceTheory(i));
		}
	}

	fixEntitySets();
}

void DocTheory::fixEntitySets() {
	int sent;
	
	
	for (sent = 0; sent < getNSentences(); sent++) {
		EntitySet *eset = getSentenceTheory(sent)->getEntitySet();
		if(eset != 0){ //eset will be 0, if sentence level entity sets were disabled with load_sent_entity_sets in loadDocTheory
			// each eset should have one copied mentionSet (i.e. one that belongs to the EntitySet and
			//  can be safely deleted. So we load the first n-1 without copying, and then load
			//  the last with copying.
			for (int prev_sent = 0; prev_sent < sent; prev_sent++) {
				eset->loadDoNotCopyMentionSet(getSentenceTheory(prev_sent)->getMentionSet());
			}
			eset->loadMentionSet(getSentenceTheory(sent)->getMentionSet());
		}
	}

	if (getEntitySet() == 0) return;
	// same principle holds
	for (sent = 0; sent < getNSentences(); sent++) {
		MentionSet *mset = getSentenceTheory(sent)->getMentionSet();
		if (sent == getNSentences() - 1)
			getEntitySet()->loadMentionSet(mset);
		else getEntitySet()->loadDoNotCopyMentionSet(mset);
	}
}

void DocTheory::loadSentenceBreaksFromStateFile(StateLoader *stateLoader){
	if (stateLoader == NULL)
		throw UnrecoverableException("DocTheory::loadSentenceBreaksFromStateFile", 
		                             "stateLoader is NULL (no experiment_dir specified?");
	wchar_t state_tree_name[100];
	wcscpy(state_tree_name, L"Sentence breaks: ");

	//const wchar_t *p = _document->getName().to_string();
	//wchar_t *q = state_tree_name + wcslen(state_tree_name);
	//while (*q++ = (wchar_t) *p++);
	stateLoader->beginStateTree(state_tree_name);
	Symbol name = stateLoader->loadSymbol();
	if(name != _document->getName()){
		char message[500];
		snprintf(message, 499, 
			"State for incorrect doc name, expecting Document: %s, State File Document: %s",
			name.to_debug_string(), 
			_document->getName().to_debug_string()  );
		throw UnrecoverableException("DocTheory::loadSentenceBreaksFromStateFile()", message);	
	}


	int nsent = stateLoader->loadInteger();
	Sentence *sentences[MAX_DOCUMENT_SENTENCES];
	stateLoader->beginList(L"sentence-breaks");
	/*if(_document->getSourceType()==DefaultDocumentReader::DF_SYM){
			const Zone* const* docZones = _document->getZones();
		int nzones = _document->getNZones();
		for(int i = 0; i < nsent; i++){
	//It is necessary to go from original offsets to positions in region substrings in either saving
	//or loading.  Choose loading b/c regions can change.
			sentences[i] = 0;
			Symbol ann_flag = stateLoader->loadSymbol();

			CharOffset start(stateLoader->loadInteger());
			CharOffset end(stateLoader->loadInteger());
			for(int j = 0; j< nzones; j++){
				const Zone * docZone = docZones[j];
				const LocatedString *docZoneString = docZone->getString();
				int startpos = docZoneString->positionOfStartOffset(start);
				int endpos = docZoneString->positionOfEndOffset(end);
				//std::cout<<"region: "<<j<<" start- "<<docRegionString->start<CharOffset>()
				//	<<" end- "<<docRegionString->end<CharOffset>()<<" check for: start: "<<start<< " end: "<<end<<std::endl;
				//docRegionString->dumpDetails(std::cout, 5);
				//std::cout<<std::endl;
				if((startpos != -1 ) && (endpos != -1)){
					LocatedString* sentstring = docZoneString->substring(startpos, endpos+1);
					sentences[i] = _new Sentence(_document, docZone, i, sentstring);
					delete sentstring;
					break;
				}

			}
			if(sentences[i] == 0) {
				std::stringstream ss;
				ss << "No region covers sentence: " << i << " of " << nsent << ", start: " << start.value() << " end: " << end.value() << "\n";
				ss << "Region start end\n";
				for(int j = 0; j< nzones; j++){
					const Zone * docZone = docZones[j];
					const LocatedString *docZoneString = docZone->getString();
					CharOffset zone_start = docZoneString->start<CharOffset>();
					CharOffset zone_end = docZoneString->end<CharOffset>();
					ss << j << " " << zone_start << " " << zone_end << "\n";
				}
				throw UnrecoverableException("DocTheory::loadSentenceBreaksFromStateFile()", ss.str());			
			}
			if(ann_flag == Symbol(L"ANNOTATABLE_REGION")){
				sentences[i]->setAnnotationFlag(true);
			}
			else{
				sentences[i]->setAnnotationFlag(false);
			}

		}
		stateLoader->endList();
	stateLoader->endStateTree();
	setSentences(nsent, sentences);
	}*/
	//else{
		const Region* const* docRegions = _document->getRegions();
		int nregions = _document->getNRegions();
		for(int i = 0; i < nsent; i++){
	//It is necessary to go from original offsets to positions in region substrings in either saving
	//or loading.  Choose loading b/c regions can change.
			sentences[i] = 0;
			Symbol ann_flag = stateLoader->loadSymbol();

			CharOffset start(stateLoader->loadInteger());
			CharOffset end(stateLoader->loadInteger());
			for(int j = 0; j< nregions; j++){
				const Region * docRegion = docRegions[j];
				const LocatedString *docRegionString = docRegion->getString();
				int startpos = docRegionString->positionOfStartOffset(start);
				int endpos = docRegionString->positionOfEndOffset(end);
				//std::cout<<"region: "<<j<<" start- "<<docRegionString->start<CharOffset>()
				//	<<" end- "<<docRegionString->end<CharOffset>()<<" check for: start: "<<start<< " end: "<<end<<std::endl;
				//docRegionString->dumpDetails(std::cout, 5);
				//std::cout<<std::endl;
				if((startpos != -1 ) && (endpos != -1)){
					LocatedString* sentstring = docRegionString->substring(startpos, endpos+1);
					sentences[i] = _new Sentence(_document, docRegion, i, sentstring);
					delete sentstring;
					break;
				}

			}
			if(sentences[i] == 0) {
				std::stringstream ss;
				ss << "No region covers sentence: " << i << " of " << nsent << ", start: " << start.value() << " end: " << end.value() << "\n";
				ss << "Region start end\n";
				for(int j = 0; j< nregions; j++){
					const Region * docRegion = docRegions[j];
					const LocatedString *docRegionString = docRegion->getString();
					CharOffset region_start = docRegionString->start<CharOffset>();
					CharOffset region_end = docRegionString->end<CharOffset>();
					ss << j << " " << region_start << " " << region_end << "\n";
				}
				throw UnrecoverableException("DocTheory::loadSentenceBreaksFromStateFile()", ss.str());			
			}
			if(ann_flag == Symbol(L"ANNOTATABLE_REGION")){
				sentences[i]->setAnnotationFlag(true);
			}
			else{
				sentences[i]->setAnnotationFlag(false);
			}

		}
		stateLoader->endList();
	stateLoader->endStateTree();
	setSentences(nsent, sentences);
	//}
	
	
}

Symbol DocTheory::loadFakedDocTheory(StateLoader *stateLoader, const wchar_t *state_tree_name, bool load_sent_entity_sets, int* sentenceStartOffsets, int* sentenceEndOffsets) {

	stateLoader->beginStateTree(L"Sentence breaks: ");
	Symbol docName = stateLoader->loadSymbol();
	int nsent = stateLoader->loadInteger();
	bool annotated_regions[MAX_DOCUMENT_SENTENCES];
	stateLoader->beginList(L"sentence-breaks");
	int i;
	for(i = 0; i < nsent; i++){
		Symbol ann_flag = stateLoader->loadSymbol(); //annotated region
		if(ann_flag == Symbol(L"ANNOTATABLE_REGION")){
			annotated_regions[i] = true;
		}
		else{
			annotated_regions[i] = false;
		}
		
        int startOffset = stateLoader->loadInteger(); //start offset of sentence
		int endOffset = stateLoader->loadInteger(); //end offset of sentence

        // If we have been passed in arrays for sentence start/end offsets, populate them
        if (sentenceStartOffsets != 0) {
            sentenceStartOffsets[i] = startOffset;
        }
        if (sentenceEndOffsets != 0) {
            sentenceEndOffsets[i] = endOffset;
        }
	}
	stateLoader->endList();
	stateLoader->endStateTree();

	stateLoader->beginStateTree(state_tree_name);
	int doc_sentences = stateLoader->loadInteger();
	Sentence* sentences[MAX_DOCUMENT_SENTENCES]; 
	for(i =0; i<doc_sentences; i++){
		//if(this->getDocument()->getSourceType()==DefaultDocumentReader::DF_SYM){
		//const Zone* temp=0;
		//sentences[i] = _new Sentence(0, temp, i, 0);
		//}
		//else{
		const Region* temp=0;
		sentences[i] = _new Sentence(0, temp, i, 0);
		//}

		
		sentences[i]->setAnnotationFlag(annotated_regions[i]);
	}

	setSentences(doc_sentences, sentences);
    loadDocTheory(stateLoader, load_sent_entity_sets);	
	stateLoader->endStateTree();
    return docName;
}

void DocTheory::saveSentenceBreaksToStateFile(StateSaver *stateSaver, Symbol* docName, int* sentenceStartOffsets, int* sentenceEndOffsets) {
	wchar_t state_tree_name[100];
	wcscpy(state_tree_name, L"Sentence breaks: ");
	ObjectIDTable::initialize();
	stateSaver->beginStateTree(state_tree_name);
    if (docName == 0) {
        stateSaver->saveSymbol(_document->getName());
    } else {
        stateSaver->saveSymbol(*docName);
    }
	stateSaver->saveInteger(getNSentences());
    stateSaver->beginList(L"sentence-breaks");

	for(int j = 0; j < getNSentences(); j++){
		const Sentence* sent = getSentence(j);
		if(sent->isAnnotated()){
			stateSaver->saveSymbol(Symbol(L"ANNOTATABLE_REGION"));
		}
		else{
			stateSaver->saveSymbol(Symbol(L"UNANNOTATABLE_REGION"));
		}
        if (sentenceStartOffsets == 0) {
            stateSaver->saveInteger(sent->getString()->start<CharOffset>().value());
        } else {
            stateSaver->saveInteger(sentenceStartOffsets[j]);
        }
        if (sentenceEndOffsets == 0) {
            stateSaver->saveInteger(sent->getString()->end<CharOffset>().value());
        } else {
            stateSaver->saveInteger(sentenceEndOffsets[j]);
        }
	}
	stateSaver->endList();
	stateSaver->endStateTree();
	ObjectIDTable::finalize();
}

bool DocTheory::isSpeakerSentence(int sentno) const {
	const Sentence *sent = getSentence(sentno);
	const Region *region = sent->getRegion();
	//if(_document->getSourceType()==DefaultDocumentReader::DF_SYM)
	//	return false;
	bool result = (region != 0) ? region->isSpeakerRegion() : false;
	return result;
}

bool DocTheory::isPostdateSentence(int sentno) {
	EDTOffset sent_offset = getSentence(sentno)->getStartEDTOffset();
	return (getMetadata() != 0 &&
		(getMetadata()->getCoveringSpan(sent_offset, POSTDATE_SYM) != 0));
}

bool DocTheory::isReceiverSentence(int sentno) const {
	const Sentence *sent = getSentence(sentno);
	const Region *region = sent->getRegion();
	//if(_document->getSourceType()==DefaultDocumentReader::DF_SYM)
	//	return false;
	
	bool result = (region != 0) ? region->isReceiverRegion() : false;
	return result;
}

Entity* DocTheory::getEntityByMention(const Mention* ment) const {
	if (getEntitySet()==0)
		throw InternalInconsistencyException("DocTheory::getEntityByMention",
			"Entity set is not defined for this document");
	return getEntitySet()->getEntityByMention(ment->getUID()); 
}

DocPropForest_ptr DocTheory::getPropForest(bool force_reconstruction) const {
	static DistillationDocExpander expander;
	if (force_reconstruction || !_propForest) {
		_propForest = PropForestFactory::getDocumentForest(this);
		expander.expandForest(*_propForest);
		//if (ParamReader::isParamTrue("verbose")) {
		//	for (int i = 0; i < getNSentences(); i++) {
		//		std::wcerr << "After expanding sentence #" << i << "\n";
		//		PropNodes_ptr sent_nodes = (*_propForest)[i];
		//		for (size_t j = 0; j < sent_nodes->size(); j++) {
		//			(*sent_nodes)[j]->compactPrint(std::wcerr, true, true, 0);
		//			std::wcerr << "\n";
		//		}
		//	}
		//}
	}
	return _propForest;
}

void DocTheory::saveLexicalEntries(SerifXML::XMLTheoryElement elem) const {
	using namespace SerifXML;
	const LexicalEntry::LexicalEntrySet &lexEntries = elem.getXMLSerializedDocTheory()->getLexicalEntriesToSerialize();
	if (lexEntries.empty()) return;
	XMLTheoryElement lexElem = elem.addChild(X_Lexicon);
	BOOST_FOREACH(const LexicalEntry* lexEntry, lexEntries) {
		lexElem.saveChildTheory(X_LexicalEntry, lexEntry);
	}
}

void DocTheory::loadLexicalEntries(SerifXML::XMLTheoryElement elem) {
	using namespace SerifXML;
	XMLTheoryElement lexElem = elem.getOptionalUniqueChildElementByTagName(X_Lexicon);
	if (!lexElem.isNull()) {
		Lexicon *lexicon = SessionLexicon::getInstance().getLexicon();
		// Load all the lexical entries described in the XML lexicon.
		std::vector<XMLTheoryElement> lexEntryElems = lexElem.getChildElementsByTagName(X_LexicalEntry);
		std::vector<LexicalEntry*> newLexEntries;
		for (size_t i=0; i<lexEntryElems.size(); ++i)
			newLexEntries.push_back(_new LexicalEntry(lexEntryElems[i], lexicon->getNextID()));
		for (size_t i=0; i<newLexEntries.size(); ++i)
			newLexEntries[i]->resolvePointers(lexEntryElems[i]);

		// Search for the lexical entries in the lexicon.  If an entry is not found,
		// then add it to the lexicon.  Otherwise, override the entry's id to point
		// to the lexical entry in the lexicon; and delete our copy.
		XMLIdMap *idMap = elem.getXMLSerializedDocTheory()->getIdMap();
		std::vector<size_t> entriesToDelete;
		for (size_t i=0; i<newLexEntries.size(); ++i) {
			LexicalEntry *new_le = newLexEntries[i];
			LexicalEntry *existing_le = lexicon->findEntry(new_le);
			if (existing_le == NULL) {
				new_le->setID(lexicon->getNextID());
				lexicon->addDynamicEntry(new_le);
			} else {
				idMap->overrideId(idMap->getId(new_le).c_str(), existing_le);
				entriesToDelete.push_back(i);
			}
		}
		BOOST_FOREACH(size_t i, entriesToDelete) {
			delete newLexEntries[i];
			newLexEntries[i] = 0;
		}

		// Re-run the resolve pointers command, to make sure that the entries
		// don't point to anything we deallocated.
		for (size_t i=0; i<newLexEntries.size(); ++i)
			if (newLexEntries[i] != 0)
				newLexEntries[i]->resolvePointers(lexEntryElems[i]);
	}
}



void DocTheory::saveXML(SerifXML::XMLTheoryElement documentElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("DocTheory::saveXML", "Expected context to be NULL");
	
	#ifdef BLOCK_FULL_SERIF_OUTPUT
	
	throw UnexpectedInputException("DocTheory::saveXML", "SerifXML not supported");

	#endif

	// Serialize information about the document itself (using the same
	// DOM element that we're using for the DocTheory).
	Document* document = getDocument();
	document->saveXML(documentElem);

	// First generate ids for the document-level ValueMentions, since they can
	// be referred to by sentence-level EventMentions
	if (getDocumentValueMentionSet()) {
		for (int i = 0; i < _documentValueMentionSet->getNValueMentions(); i++) {
			documentElem.generateChildId(_documentValueMentionSet->getValueMention(i));
		}
	}

	// Generate ids for document-level RelMentions, since they can
	// be referred to be document-level Relations
	if (getDocumentRelMentionSet()) {
		for (int i = 0; i < _documentRelMentionSet->getNRelMentions(); i++) {
			documentElem.generateChildId(_documentRelMentionSet->getRelMention(i));
		}
	}

	// Serialize the individual sentences and their theories.  We'll
	// use a single element for each sentence, which will combine the
	// information about the sentence location and the annotations for
	// the sentence -- i.e., we're encoding both a Sentence and its
	// SentenceTheory using the same XML element.
	XMLTheoryElement sentencesElem = documentElem.addChild(X_Sentences);
	int n_sents = getNSentences();
	for (int sent_no=0; sent_no<n_sents; ++sent_no) {
		// Serialize both the sentence and the SentenceTheory to the same DOM element.
		const Sentence *sent = getSentence(sent_no);
		const SentenceTheoryBeam *sentTheoryBeam = getSentenceTheoryBeam(sent_no);
		XMLTheoryElement sentenceElem = sentencesElem.saveChildTheory(X_Sentence, sent);

		if (sentTheoryBeam != 0)
			sentTheoryBeam->saveXML(sentenceElem);

		// Sanity checks:
		assert(sent->getDocument() == document);
		assert(sent->getSentNumber() == sent_no);
		if (sentTheoryBeam != 0) {
			for (int i=0; i<sentTheoryBeam->getNTheories(); ++i) {
				SentenceTheory *sentTheory = sentTheoryBeam->getTheory(i);
				assert(sentTheory->getDocID() == document->getName());
				TokenSequence *tokSeq = sentTheory->getTokenSequence();
				assert((tokSeq==0) || (tokSeq->getSentenceNumber()==sent_no));
			}
		}
	}

	// Encode each document-level annotation layer.
	if (getEntitySet())
		documentElem.saveChildTheory(X_EntitySet, getEntitySet());
	if (getValueSet())
		documentElem.saveChildTheory(X_ValueSet, getValueSet());
	if (getRelationSet())
		documentElem.saveChildTheory(X_RelationSet, getRelationSet(), getEntitySet());
	if (getEventSet())
		documentElem.saveChildTheory(X_EventSet, getEventSet(), this);
	if (getUTCoref())
		documentElem.saveChildTheory(X_UTCoref, getUTCoref());
	if (getDocumentValueMentionSet())
		documentElem.saveChildTheory(X_ValueMentionSet, getDocumentValueMentionSet(), this);
	if (getDocumentRelMentionSet())
		documentElem.saveChildTheory(X_RelMentionSet, getDocumentRelMentionSet());
	if (getSpeakerQuotationSet())
		documentElem.saveChildTheory(X_SpeakerQuotationSet, getSpeakerQuotationSet(), this);
	if (getActorEntitySet())
		documentElem.saveChildTheory(X_ActorEntitySet, getActorEntitySet(), this);
	if (getDocumentActorInfo())
		documentElem.saveChildTheory(X_DocumentActorInfo, getDocumentActorInfo(), this);
	if (getFactSet())
		documentElem.saveChildTheory(X_FactSet, getFactSet(), this);
	if (getActorMentionSet())
		documentElem.saveChildTheory(X_ActorMentionSet, getActorMentionSet(), this);
	if (getICEWSEventMentionSet())
		documentElem.saveChildTheory(X_ICEWSEventMentionSet, getICEWSEventMentionSet(), this);

	// Save any additional subtheories (in the order in which they were registered)
	BOOST_FOREACH(boost::shared_ptr<SubtheoryRecord> subtheoryRecord, _subtheoryRecords()) {
		std::map<std::string, Theory*>::const_iterator it = _subtheories.find(subtheoryRecord->typeName);
		if (it != _subtheories.end()) {
			Theory *subtheory = (*it).second;
			documentElem.saveChildTheory(subtheoryRecord->xmlTag.c_str(), subtheory, this);
		}
	}

	// Save any lexical entries that we referenced.
	saveLexicalEntries(documentElem);

}

DocTheory::DocTheory(SerifXML::XMLTheoryElement documentElem)
: _document(0), _entitySet(0), 
_utcoref(0), _valueSet(0), _relationSet(0), _documentRelMentionSet(0), 
_eventSet(0), _documentValueMentionSet(0), _speakerQuotationSet(0)
{
	using namespace SerifXML;
	_document = _new Document(documentElem);

	// Load the lexicon (if it's present).
	loadLexicalEntries(documentElem);

	// Collect mention sets for the document-level entity set theory.  The
	// document-level entity set theory may only link into the mention sets
	// of the best SentenceTheory for each sentence.
	std::vector<MentionSet*> sentenceMentionSets;

	// Read each sentence.  The "Sentence" xml element actually contains
	// information for both the Sentence object itself, and for its
	// SentenceTheoryBeam (including all SentenceSubtheories).
	if (XMLTheoryElement sentencesElem = documentElem.getOptionalUniqueChildElementByTagName(X_Sentences)) {
		std::vector<XMLTheoryElement> sentenceElems = sentencesElem.getChildElementsByTagName(X_Sentence);
		int n_sentences = static_cast<int>(sentenceElems.size());
		//_sentences = _new Sentence*[getNSentences()];
		//_sentTheories = _new SentenceTheory*[getNSentences()];
		// Read each sentence.
		for (int i=0; i<static_cast<int>(sentenceElems.size()); ++i) {
			Sentence *sent = _new Sentence(sentenceElems[i], _document, i);
			_sentences.push_back(sent);
			SentenceTheoryBeam *sentTheoryBeam = _new SentenceTheoryBeam(sentenceElems[i], sent, _document->getName());
			if (sentTheoryBeam->getNTheories() > 0) {
				_sentTheoryBeams.push_back(sentTheoryBeam);
				sentenceMentionSets.push_back(sentTheoryBeam->getBestTheory()->getMentionSet());
			} else {
				// If there are no sentence theories, then just set the beam to NULL; we'll 
				// initialize it once we start processing the sentence.
				_sentTheoryBeams.push_back(0);
				delete sentTheoryBeam; 
			}
		}
	}

	// Read document-level annotation layers.
	_documentValueMentionSet = documentElem.loadOptionalChildTheory<ValueMentionSet>(X_ValueMentionSet, this);
	_entitySet = documentElem.loadOptionalChildTheory<EntitySet>(X_EntitySet, sentenceMentionSets);
	_valueSet = documentElem.loadOptionalChildTheory<ValueSet>(X_ValueSet);
	_documentRelMentionSet = documentElem.loadOptionalChildTheory<RelMentionSet>(X_RelMentionSet, -1);
	_relationSet = documentElem.loadOptionalChildTheory<RelationSet>(X_RelationSet);
	_eventSet = documentElem.loadOptionalChildTheory<EventSet>(X_EventSet);
	_actorEntitySet = documentElem.loadOptionalChildTheory<ActorEntitySet>(X_ActorEntitySet);
	_documentActorInfo = documentElem.loadOptionalChildTheory<DocumentActorInfo>(X_DocumentActorInfo);
	_factSet = documentElem.loadOptionalChildTheory<FactSet>(X_FactSet);
	_utcoref = documentElem.loadOptionalChildTheory<UTCoref>(X_UTCoref);
	_speakerQuotationSet = documentElem.loadOptionalChildTheory<SpeakerQuotationSet>(X_SpeakerQuotationSet);
	_actorMentionSet = documentElem.loadOptionalChildTheory<ActorMentionSet>(X_ActorMentionSet);
	_icewsEventMentionSet = documentElem.loadOptionalChildTheory<ICEWSEventMentionSet>(X_ICEWSEventMentionSet);
	//std::cout << "todo: reconstruct documentRelMentionSet" << std::endl;
	// [XX] TODO: do I need something like fixEntitySets() here?

	// Reconstruct per-sentence entity sets.  We need to make a copy of the 
	// document-level entity set because the current document-level entity set 
	// will get replaced after the doc-entities stage, leaving us with dangling
	// sentence level pointers.
	if (_entitySet) {
		EntitySet *sentenceEntitySet = _new EntitySet(*_entitySet, false);  // Do we need to copy the mention set?
		for (int i=0; i<getNSentences(); ++i) {
			getSentenceTheory(i)->adoptSubtheory(SentenceTheory::ENTITY_SUBTHEORY, sentenceEntitySet);
		}
	}

	// Resolve any remaining pointers now that everything has been loaded
	if (XMLTheoryElement sentencesElem = documentElem.getOptionalUniqueChildElementByTagName(X_Sentences)) {
		std::vector<XMLTheoryElement> sentenceElems = sentencesElem.getChildElementsByTagName(X_Sentence);
		for (int i=0; i<static_cast<int>(sentenceElems.size()); ++i) {
			getSentenceTheoryBeam(i)->resolvePointers(sentenceElems[i]);
		}
	}

	// Load any additional subtheories (in the order in which they were registered)
	BOOST_FOREACH(boost::shared_ptr<SubtheoryRecord> subtheoryRecord, _subtheoryRecords()) {
		XMLTheoryElement childElem = documentElem.getOptionalUniqueChildElementByTagName(subtheoryRecord->xmlTag.c_str());
		if (childElem)
			_subtheories[subtheoryRecord->typeName] = subtheoryRecord->loadXML(childElem, this);
	}

}

const wchar_t* DocTheory::XMLIdentifierPrefix() const {
	return L"doc";
}

Stage DocTheory::getMaxStartStage() const{
	if (getNSentences() == 0)
		return Stage("sent-break");

    bool do_np_chunk = ParamReader::getOptionalTrueFalseParamWithDefaultVal("do_np_chunk", false);
	bool do_dependency_parsing = ParamReader::getOptionalTrueFalseParamWithDefaultVal("do_dependency_parsing", false);

	// Use a loop to check stages in order, since the default (compile-time)
	// ordering of stages can be changed at run time -- e.g., if we're using
	// NP chunking constraints for the parser
	for (Stage stage=Stage::getFirstStage(); stage < Stage::getEndStage(); ++stage) {
		SentenceTheory *sentTheory = getSentenceTheory(0);

		if ((stage == Stage("tokens") && sentTheory->getTokenSequence() == 0) ||
			(stage == Stage("part-of-speech") && sentTheory->getPartOfSpeechSequence() == 0) ||
			(stage == Stage("names") && sentTheory->getNameTheory() == 0) ||
			(stage == Stage("values") && sentTheory->getValueMentionSet() == 0) ||
            (stage == Stage("parse") && sentTheory->getFullParse() == 0) ||
			(stage == Stage("npchunk") && sentTheory->getNPChunkTheory() == 0 && do_np_chunk) ||
			(stage == Stage("dependency-parse") && sentTheory->getDependencyParseTheory() == 0 && do_dependency_parsing) ||
			(stage == Stage("mentions") && sentTheory->getMentionSet() == 0) ||
			(stage == Stage("props") && sentTheory->getPropositionSet() == 0) ||
			(stage == Stage("metonymy") && false) || // [xx] how do we check this stage?
			(stage == Stage("entities") && sentTheory->getEntitySet() == 0) ||
			(stage == Stage("events") && sentTheory->getEventMentionSet() == 0) ||
			(stage == Stage("relations") && sentTheory->getRelMentionSet() == 0) ||
			(stage == Stage("doc-entities") && getEntitySet() == 0) ||
			(stage == Stage("doc-relations-events") && getRelationSet() == 0) ||
			(stage == Stage("doc-relations-events") && getEventSet() == 0) ||
			(stage == Stage("doc-values") && getValueSet() == 0) ||
			(stage == Stage("doc-actors") && getActorEntitySet() == 0) ||
			(stage == Stage("factfinder") && getFactSet() == 0) ||
			(stage == Stage("generics") && false) || // [xx] how do we check this stage?
			(stage == Stage("clutter") && false) || // [xx] how do we check this stage?
			(stage == Stage("xdoc") && false)) // [xx] how do we check this stage?
		{
			//std::cerr << "getMaxStartStage returning " << stage.getName() << std::endl;
			return stage;
		}
	}
	return Stage("output"); // This document has already been fully processed!
}

void DocTheory::ensureSubtheoryTypeIsRegistered(const std::string& typeName) {
	bool is_registered = false;
	BOOST_FOREACH(boost::shared_ptr<SubtheoryRecord> record, _subtheoryRecords()) {
		if (record->typeName == typeName)
			is_registered = true;
	}
	if (!is_registered)
		throw InternalInconsistencyException("DocTheory::ensureSubtheoryTypeIsRegistered",
			"Attempt to use subtheory type extension without registering it.");
}

void DocTheory::ensureSubtheoryTypeIsNotRegistered(const std::string& typeName) {
	for(std::vector<boost::shared_ptr<SubtheoryRecord> >::iterator iter = _subtheoryRecords().begin(); iter != _subtheoryRecords().end(); iter++) {
		boost::shared_ptr<SubtheoryRecord> record = *iter;
		if (record->typeName == typeName)
			throw InternalInconsistencyException("DocTheory::registerSubtheoryType()",
				"A subtheory type with this name is already registered.");
	}
}

std::vector<boost::shared_ptr<DocTheory::SubtheoryRecord> > &DocTheory::_subtheoryRecords() {
	static std::vector<boost::shared_ptr<SubtheoryRecord> > records;
	return records;
}

const Mention* DocTheory::getMention(MentionUID ment) const {
	return getSentenceTheory(Mention::getSentenceNumberFromUID(ment))
		->getMentionSet()->getMention(ment);
}

