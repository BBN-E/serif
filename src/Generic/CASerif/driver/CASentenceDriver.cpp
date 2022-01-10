// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/limits.h"
#include "Generic/CASerif/driver/CASentenceDriver.h"
#include "Generic/CASerif/correctanswers/CorrectAnswers.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"

// Stage-specific components
#include "Generic/tokens/Tokenizer.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"
#include "Generic/morphAnalysis/SessionLexicon.h"
#include "Generic/morphSelection/MorphSelector.h"
#include "Generic/normalizer/MTNormalizer.h"
#include "Generic/partOfSpeech/PartOfSpeechRecognizer.h"
#include "Generic/names/NameRecognizer.h"
#include "Generic/values/ValueRecognizer.h"
#include "Generic/parse/Parser.h"
#include "Generic/descriptors/DescriptorRecognizer.h"
#include "Generic/propositions/PropositionFinder.h"
#include "Generic/metonymy/MetonymyAdder.h"
#include "Generic/edt/DummyReferenceResolver.h"
#include "Generic/relations/RelationFinder.h"
#include "Generic/events/EventFinder.h"
#include "Generic/events/patterns/DeprecatedPatternEventFinder.h"
#include "Generic/PNPChunking/NPChunkFinder.h"

// Stage-specific subtheories
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/EventMentionSet.h"

using namespace std;

static const Symbol _symbol_ca_sentence = Symbol(L"CA_sentence");
CASentenceDriver::CASentenceDriver() 
	: _names_loaded(false), _nestedNames_loaded(false), _values_loaded(false), _relations_loaded(false)
{
	_correctAnswers = &CorrectAnswers::getInstance();

	if (_tokens_beam_width != 1) {
		cout << "CASentenceDriver::CASentenceDriver(), "
			 << "Tokens beam width must be equal to 1 for CorrectAnswerSerif\n";
		_tokens_beam_width = 1;
	}

}

bool CASentenceDriver::stageModelsAreLoaded(Stage stage) {
	return (((stage == _tokens_Stage) && (_tokenizer != 0)) ||
		    ((stage == _partOfSpeech_Stage) && (_posRecognizer != 0)) ||
//		    ((stage == _names_Stage) && (_nameRecognizer != 0)) ||
//		    ((stage == _values_Stage) && (_valueRecognizer != 0)) ||
		    ((stage == _parse_Stage) && (_parser != 0)) ||
		    ((stage == _npchunk_Stage) && (_npChunkFinder != 0)) ||
		    ((stage == _mentions_Stage) && (_descriptorRecognizer != 0)) ||
		    ((stage == _props_Stage) && (_propositionFinder != 0)) ||
		    ((stage == _metonymy_Stage) && (_metonymyAdder != 0)) ||
		    ((stage == _entities_Stage) && (_dummyReferenceResolver != 0)) ||
		    ((stage == _events_Stage) && (_eventFinder != 0)) ||
//		    ((stage == _relations_Stage) && (_relationFinder != 0)));	
			((stage == _names_Stage) && _names_loaded) ||
			((stage == _nestedNames_Stage) && _nestedNames_loaded) ||
			((stage == _values_Stage) && _values_loaded) ||
			((stage == _relations_Stage) && _relations_loaded));
}

void CASentenceDriver::loadNameModels() {
	_max_name_theories = ParamReader::getRequiredIntParam("name_branch");
	//_nameRecognizer = NameRecognizer::build();
	_nameTheoryBuf = _new NameTheory*[_max_name_theories];
	_names_loaded = true;
}

void CASentenceDriver::loadNestedNameModels() {
	_max_nested_name_theories = ParamReader::getRequiredIntParam("nested_name_branch");
	_nestedNameTheoryBuf = _new NestedNameTheory*[_max_nested_name_theories];
	_nestedNames_loaded = true;
}

void CASentenceDriver::loadValueModels() {
	_max_value_sets = ParamReader::getRequiredIntParam("value_branch");
	//_valueRecognizer = ValueRecognizer::build();
	_valueSetBuf = _new ValueMentionSet*[_max_value_sets];
	_values_loaded = true;
}

void CASentenceDriver::loadEventModels() {
	_eventFinder = _new EventFinder(_symbol_ca_sentence);
	_eventFinder->disallowMentionSetChanges(); // after all, we're using correct mentions, right?
}

void CASentenceDriver::loadRelationModels() {
	if (!CorrectAnswers::getInstance().usingCorrectRelations()) {
		_relationFinder =  RelationFinder::build();
		_relationFinder->disallowMentionSetChanges(); // after all, we're using correct mentions, right?
	} else {
		if (_use_sentence_level_relation_finding) {
			throw UnexpectedInputException("CASentenceDriver::loadRelationModels()",
					"use_sentence_level_relation_finding is not compatible with "
					"using correct relations.");
		}
	}

	_relations_loaded = true;
}

void CASentenceDriver::beginDocument(DocTheory *docTheory) {
	SentenceDriver::beginDocument(docTheory);
	CorrectAnswers::getInstance().resetForNewDocument();
}

// Todo: check to make sure that we're initialized for the given range
// of stages -- e.g. it would be bad if we were called with a startstage
// of names if we only initialized the stages after mentions.
SentenceTheoryBeam *CASentenceDriver::run(DocTheory *docTheory, int sent_no,
									Stage startStage, Stage endStage)
{
	// If we're using lazy model loading, then make sure the models are loaded now.
	for (Stage stage=startStage; stage<=endStage; ++stage) {
		if (_sessionProgram->includeStage(stage))
			loadModelsForStage(stage);
	}

	const Sentence* sentence = docTheory->getSentence(sent_no);

	if (_n_docs_processed < MAX_INTENTIONAL_FAILURES &&
		_intentional_failures[_n_docs_processed])
	{
		_n_docs_processed++; // because endDocument() gets no chance to
		throw UnexpectedInputException(
			"CASentenceDriver::run()",
			"The sky is falling! Head for the hills! Where are the children?!");
	}

	SentenceTheoryBeam *currentBeam = docTheory->getSentenceTheoryBeam(sent_no);

	// If we're starting from scratch, then create a beam with an empty theory.
	if (currentBeam == 0) {
		currentBeam = _new SentenceTheoryBeam(sentence, _beam_width);
		currentBeam->addTheory(_new SentenceTheory(sentence, _primary_parse, docTheory->getDocument()->getName()));
		docTheory->setSentenceTheoryBeam(sent_no, currentBeam);
	}

	// limit stage range to sentence-level stages
	if (startStage < _tokens_Stage)
		startStage = _tokens_Stage;
	if (Stage::getLastSentenceLevelStage() < endStage)
		endStage = Stage::getLastSentenceLevelStage();

	// This loop takes the sentence through each stage
	for (Stage stage = startStage; stage <= endStage; ++stage) {
		if (!_sessionProgram->includeStage(stage))
			continue;


		_sessionLogger->updateContext(STAGE_CONTEXT, stage.getName());

		char source[100];
		sprintf(source, "CASentenceDriver::run(); before stage %s",
						stage.getName());

//		HeapChecker::checkHeap(source);

#ifdef SERIF_SHOW_PROGRESS
		clearLine();
		cout << "Processing sentence " << sentence->getSentNumber()
			 << ": " << stage.getName() << "...";
#endif

		// notify each subtheory generator that we're starting a new sentence
		if (stage == _tokens_Stage) {
			_tokenizer->resetForNewSentence(docTheory->getDocument(),
											sentence->getSentNumber());
			_morphAnalysis->resetForNewSentence();

			if (_do_morph_selection)
				_morphSelector->resetForNewSentence();

			if (_do_mt_normalization)
				_mtNormalizer->resetForNewSentence();
		}
		else if (stage == _partOfSpeech_Stage) {
			_posRecognizer->resetForNewSentence();
		}
		//else if (stage == _names_Stage) {
			//_nameRecognizer->resetForNewSentence();
		//}
		//else if (stage == _values_Stage) {
		//	_valueRecognizer->resetForNewSentence();
		//}
		else if (stage == _parse_Stage) {
			_parser->resetForNewSentence();
		}
		else if (stage == _npchunk_Stage){
			if(_npChunkFinder)
				_npChunkFinder->resetForNewSentence();
		}
		else if (stage == _mentions_Stage) {
			_descriptorRecognizer->resetForNewSentence();
		}
		else if (stage == _props_Stage) {
			_propositionFinder->resetForNewSentence(sentence->getSentNumber());
		}
		else if (stage == _metonymy_Stage) {
			_metonymyAdder->resetForNewSentence();
		}
		else if (stage == _entities_Stage) {
			if (_dummyReferenceResolver != 0)
				_dummyReferenceResolver->resetForNewSentence(docTheory, sentence->getSentNumber());
		}
		else if (stage == _events_Stage) {
			if (_eventFinder != 0) {
				_eventFinder->resetForNewSentence(docTheory, sentence->getSentNumber());
				
				_eventFinder->_deprecatedPatternEventFinder->_correctAnswers = _correctAnswers;
			}
		}
		else if (stage == _relations_Stage && !_correctAnswers->usingCorrectRelations()) {
			if (_relationFinder != 0)
				_relationFinder->resetForNewSentence(docTheory, sent_no);
		}

		// For each old theory, combine with new subtheory(ies) and put
		// resulting theory into new beam.
		SentenceTheoryBeam *nextBeam = 0;

		for (int theory_no = 0;
			 theory_no < currentBeam->getNTheories();
			 theory_no++)
		{
			sprintf(source, "CASentenceDriver::run(); stage %s; before theory %d",
							stage.getName(), theory_no);
//			HeapChecker::checkHeap(source);
			SentenceTheory *currentTheory = currentBeam->getTheory(theory_no);
			//always set the lexicon
			currentTheory->setLexicon(SessionLexicon::getInstance().getLexicon());

			// create new subtheory(ies) depending on what stage we're on
			if (stage == _tokens_Stage) {         // *** Tokenization
				_n_token_sequences = _tokenizer->getTokenTheories(
					_tokenSequenceBuf, _max_token_sequences,
					sentence->getString());
				for(int i =0; i< _n_token_sequences; i++){
					_morphAnalysis->getMorphTheories(_tokenSequenceBuf[i]);
				}
				if (_do_morph_selection) {
					std::vector<CharOffset> constraints = 
						_correctAnswers->getMorphConstraints(sentence, docTheory->getDocument()->getName());
					int num_constraints = static_cast<int>(constraints.size());
					CharOffset* constraint_array = (num_constraints==0)?0:(&constraints[0]);
					for(int i=0; i<_n_token_sequences; i++){
						_morphSelector->selectTokenization(sentence->getString(), _tokenSequenceBuf[i], constraint_array, num_constraints);
					}
				}

				if (_do_mt_normalization) {
			 		for(int i=0; i<_n_token_sequences; i++){
						_mtNormalizer->normalize(_tokenSequenceBuf[i]);
					}
				}

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _tokens_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::TOKEN_SUBTHEORY, _n_token_sequences,
					(SentenceSubtheory **) _tokenSequenceBuf);

				//for(int i=0;i<_tokenSequenceBuf[0]->getNTokens();i++){
				//	cout << _tokenSequenceBuf[0]->getToken(i)->getSymbol() <<"\n";
				//}
			}
			else if (stage == _partOfSpeech_Stage) {     // *** Part of Speech Recognition
				_n_pos_sequences = _posRecognizer->getPartOfSpeechTheories(
					_partOfSpeechSequenceBuf, _max_pos_sequences,
					currentTheory->getTokenSequence());

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _pos_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::POS_SUBTHEORY, _n_pos_sequences,
					(SentenceSubtheory **) _partOfSpeechSequenceBuf);
			}

			else if (stage == _names_Stage) {     // *** Name Recognition
				_n_name_theories = _correctAnswers->getNameTheories(
					_nameTheoryBuf, currentTheory->getTokenSequence(),
					docTheory->getDocument()->getName());

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _names_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::NAME_SUBTHEORY, _n_name_theories,
					(SentenceSubtheory **) _nameTheoryBuf);
			}
			else if (stage == _nestedNames_Stage) {
				_n_nested_name_theories = _correctAnswers->getNestedNameTheories(
					_nestedNameTheoryBuf, currentTheory->getTokenSequence(), docTheory->getDocument()->getName(),
					currentTheory->getNameTheory());
				
				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _nested_names_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::NESTED_NAME_SUBTHEORY, _n_nested_name_theories,
					(SentenceSubtheory **) _nestedNameTheoryBuf);
			}
			else if (stage == _values_Stage) {     // *** Value Recognition

				_n_value_sets = _correctAnswers->getValueTheories(
					_valueSetBuf, currentTheory->getTokenSequence(),
					docTheory->getDocument()->getName());

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _values_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::VALUE_SUBTHEORY, _n_value_sets,
					(SentenceSubtheory **) _valueSetBuf);
			}
			else if (stage == _parse_Stage) {     // *** Parsing
				// in case we are restarting
                _correctAnswers->setSentAndTokenNumbersOnCorrectMentions(
					currentTheory->getTokenSequence(),
					docTheory->getDocument()->getName());

				std::vector<Constraint> constraints = _correctAnswers->getConstraints(
										currentTheory->getTokenSequence(),
										docTheory->getDocument()->getName());

				int num_constraints = static_cast<int>(constraints.size());
				Constraint* constraint_array = (num_constraints==0)?0:(&constraints[0]);

				_n_parses = _parser->getParses(
					_parseBuf, _max_parses,
					currentTheory->getTokenSequence(),
					currentTheory->getPartOfSpeechSequence(),
					currentTheory->getNameTheory(),
					currentTheory->getNestedNameTheory(),
					currentTheory->getValueMentionSet(),
					constraint_array,
					num_constraints);

				_correctAnswers->adjustParseScores(_parseBuf, _n_parses,
					currentTheory->getTokenSequence(), docTheory->getDocument()->getName());

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _parse_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::PARSE_SUBTHEORY, _n_parses,
					(SentenceSubtheory **) _parseBuf);
			}
			else if(stage == _npchunk_Stage){
				if(_npChunkFinder){
					_n_np_chunk= _npChunkFinder->getNPChunkTheories(_npChunkBuf, 1,
						currentTheory->getTokenSequence(), currentTheory->getFullParse(),
						currentTheory->getNameTheory());

					//since we are only using a 1 best chunk theory, no need to branch
					//branching will requiring adding a new stage....
					//currentTheory->adoptSubtheory(SentenceTheory::NPCHUNK_SUBTHEORY, _npChunkBuf[0]);
					if (nextBeam == 0)
						nextBeam = _new SentenceTheoryBeam(sentence, _npchunk_beam_width);
					addSubtheories(nextBeam, currentTheory,
						SentenceTheory::NPCHUNK_SUBTHEORY, _n_np_chunk,
						(SentenceSubtheory **) _npChunkBuf);


					//replace the Parse with the parse from the NPChunkTheory-

					//Parse* npchunkparse = _new Parse(_npChunkBuf[0]->_root,_npChunkBuf[0]->score);
					//currentTheory->adoptSubtheory(SentenceTheory::PARSE_SUBTHEORY, npchunkparse);
				}

			}

			else if (stage == _mentions_Stage) {  // *** Desc Recognition
				// in case we are restarting
                _correctAnswers->setSentAndTokenNumbersOnCorrectMentions(
					currentTheory->getTokenSequence(),
					docTheory->getDocument()->getName());

				// in case we are restarting
				_correctAnswers->hookUpCorrectValuesToSystemValues(
					currentTheory->getTokenSequence()->getSentenceNumber(),
					currentTheory->getValueMentionSet(),
					docTheory->getDocument()->getName());

				_correctAnswers->assignSynNodesToCorrectMentions(
					currentTheory->getTokenSequence()->getSentenceNumber(),
					currentTheory->getPrimaryParse(), docTheory->getDocument()->getName());

				CompoundMentionFinder::getInstance()->setCorrectAnswers(_correctAnswers);
				CompoundMentionFinder::getInstance()->setCorrectDocument(
					_correctAnswers->getCorrectDocument(docTheory->getDocument()->getName()));
				CompoundMentionFinder::getInstance()->setSentenceNumber(sentence->getSentNumber());

				_n_mention_sets = _descriptorRecognizer->getDescriptorTheories(
					_mentionSetBuf, _max_mention_sets,
					currentTheory->getPartOfSpeechSequence(),
					currentTheory->getPrimaryParse(),
					currentTheory->getNameTheory(),
					currentTheory->getNestedNameTheory(),
					currentTheory->getTokenSequence(),
					sentence->getSentNumber());

				_n_mention_sets = _correctAnswers->correctMentionTheories(
					_mentionSetBuf,
					sentence->getSentNumber(),
					currentTheory->getTokenSequence(),
					docTheory->getDocument()->getName(),
					_correctAnswers->usingCorrectTypes());

				if (!_correctAnswers->usingCorrectSubtypes()) {
					for (int mset = 0; mset < _n_mention_sets; mset++) {
						for (int m = 0; m < _mentionSetBuf[mset]->getNMentions(); m++) {
							_mentionSetBuf[mset]->getMention(m)->setEntitySubtype(EntitySubtype::getUndetType());
						}
						_descriptorRecognizer->regenerateSubtypes(_mentionSetBuf[mset],
							currentTheory->getPrimaryParse()->getRoot());
					}
				}

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _mentions_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::MENTION_SUBTHEORY, _n_mention_sets,
					(SentenceSubtheory **) _mentionSetBuf);

			}
			else if (stage == _props_Stage) {  // *** Proposition Finding
				_propositionSetBuf = _propositionFinder->getPropositionTheory(
					currentTheory->getPrimaryParse(),
					currentTheory->getMentionSet());

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _props_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::PROPOSITION_SUBTHEORY, 1,
					(SentenceSubtheory **) &_propositionSetBuf);
			}
			else if (stage == _metonymy_Stage) {  // *** Metonymy Adding
				_metonymyAdder->addMetonymyTheory(
					currentTheory->getMentionSet(),
					currentTheory->getPropositionSet());
				// modifies the existing beam, so no need to create a new one
			}
			else if (stage == _entities_Stage) {  // *** Reference Resolution
				_entitySetBuf = _dummyReferenceResolver->createDefaultEntityTheory(
						currentTheory->getMentionSet());

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _entities_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::ENTITY_SUBTHEORY, 1,
					(SentenceSubtheory **) &_entitySetBuf);
			}
			else if (stage == _events_Stage) {  // *** Event Finding

				if (_use_sentence_level_event_finding) {
					_eventSetBuf = _eventFinder->getEventTheory(
						currentTheory->getTokenSequence(),
						currentTheory->getValueMentionSet(),
						currentTheory->getPrimaryParse(),
						currentTheory->getMentionSet(),
						currentTheory->getPropositionSet());
				} else _eventSetBuf = _new EventMentionSet(currentTheory->getPrimaryParse());

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _events_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::EVENT_SUBTHEORY, 1,
					(SentenceSubtheory **) &_eventSetBuf);

			}
			else if (stage == _relations_Stage) {  // *** Relation Finding

				// correct event finding will be done at the document-level

				if (_use_sentence_level_relation_finding){
					_relationFinder->currentCorrectDocument = _correctAnswers->getCorrectDocument(docTheory->getDocument()->getName());
					_relationSetBuf = _relationFinder->getRelationTheory(
						currentTheory->getPrimaryParse(),
						currentTheory->getMentionSet(),
						currentTheory->getValueMentionSet(),
						currentTheory->getPropositionSet(),
						0);
				} else _relationSetBuf = _new RelMentionSet();

				if (nextBeam == 0)
					nextBeam = _new SentenceTheoryBeam(sentence, _relations_beam_width);
				addSubtheories(nextBeam, currentTheory,
					SentenceTheory::RELATION_SUBTHEORY, 1,
					(SentenceSubtheory **) &_relationSetBuf);
			}
			else {
				// We don't do anything for this stage, so there's no point
				// in continuing for the other theories
				break;
			}
		}

		// Now that we're out of the stage loop, let the session logger know
		// not to print out stage context info
		_sessionLogger->updateContext(STAGE_CONTEXT, "");

		// If a new beam was created, adopt it as the current beam.
		// (Note: the docTheory, as the owner of the old beam, is
		// responsible for deleting it.)
		if (nextBeam != 0) {
			docTheory->setSentenceTheoryBeam(sent_no, nextBeam);
			currentBeam = nextBeam;
		}

		// When a component needs to bail out, but manages not to crash, it
		// returns 0 subtheories. This results in a 0-sized beam. We need to
		// add a "default" theory, which simply says nothing, so there is at
		// least a valid beam.
		// IH, 3/3/11: 
		// A change in the logic of addSubtheories makes this step unnecessary
		// and removes the side-effect of previous subtheories being erased.
		// We don't ever want this to be called, if it is, exit Serif.
		if (currentBeam->getNTheories() == 0) {
			std::string message("Found no subtheories for stage <"	);
			message.append(stage.getName());
			message.append(">. Is there a model specified?\n");
			SessionLogger::err("no_subtheories_0") << message;
			throw 
				InternalInconsistencyException("CASentenceDriver::run",
					message.c_str());
		}

		saveBeamState(stage, currentBeam, sentence->getSentNumber());
	}

	return currentBeam;
}
