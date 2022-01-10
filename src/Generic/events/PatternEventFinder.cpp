// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/PatternEventFinder.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Sexp.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/EventMention.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/relations/xx_RelationUtilities.h"

#include <boost/foreach.hpp>
#include <string.h>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>

PatternEventFinder::PatternEventFinder() : _docTheory(0), _sentence_number(-1) {

	
	std::vector<std::string> filenames;

	// You can specify one or many (this is mostly for backwards compatibility)
	std::string pattern_set_file = ParamReader::getParam("event_pattern_set");
	std::string pattern_set_list = ParamReader::getParam("event_pattern_set_list");
	if (pattern_set_file != "") {
		if (pattern_set_list != "") {
			throw UnexpectedInputException("PatternEventFinder::PatternEventFinder", 
				"Only one of parameters 'event_pattern_set' or 'event_pattern_set_list' should be specified");
		}
		filenames.push_back(pattern_set_file);
	} else if (pattern_set_list != "") {

		boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(pattern_set_list.c_str()));
		UTF8InputStream& stream(*stream_scoped_ptr);
		if( (!stream.is_open()) || (stream.fail()) ){
			std::stringstream errmsg;
			errmsg << "Failed to open event_pattern_set_list file \"" << pattern_set_list << "\"";
			throw UnexpectedInputException("PatternEventFinder::PatternEventFinder()", errmsg.str().c_str());
		}

		while (!(stream.eof() || stream.fail())) {
			std::wstring wline;
			std::getline(stream, wline);
			wline = wline.substr(0, wline.find_first_of(L'#')); // Remove comments.
			boost::trim(wline);
			std::string line = UnicodeUtil::toUTF8StdString(wline);
			
			if (!line.empty()) {
				// Expand parameter variables.
				try {
					line = ParamReader::expand(line);
				} catch (UnexpectedInputException &e) {
					std::ostringstream prefix;
					prefix << "while processing event_pattern_set_list file " << pattern_set_list << ": ";
					e.prependToMessage(prefix.str().c_str());
					throw;
				}				

				// Normalize filename
				boost::algorithm::replace_all(line, "/", SERIF_PATH_SEP);
				boost::algorithm::replace_all(line, "\\", SERIF_PATH_SEP);
				filenames.push_back(line);
			}
		}
	} else {
		throw UnexpectedInputException("PatternEventFinder::PatternEventFinder", 
				"Either parameter 'event_pattern_set' or 'event_pattern_set_list' must  be specified");
	}


	BOOST_FOREACH(std::string filename, filenames) {
		try {
			_patternSets.push_back(boost::make_shared<PatternSet>(filename.c_str()));
		} catch (UnrecoverableException &exc) {
			std::ostringstream prefix;
			prefix << "While processing \"" << filename << "\": ";
			exc.prependToMessage(prefix.str().c_str());
			throw;
		}
	}

	_splitListMentions = ParamReader::isParamTrue("event_patterns_split_list_mentions");
}

void PatternEventFinder::resetForNewSentence(DocTheory *docTheory, int sentence_num) {

	_docTheory = docTheory;
	_sentence_number = sentence_num;
}

EventMentionSet *PatternEventFinder::getEventTheory(const Parse* parse) 
{
	// This code directly echoes PatternRelationFinder
	
	// This should have gotten set...
	if (_docTheory == 0)
		return _new EventMentionSet(parse);

	EventMentionSet *emSet = _new EventMentionSet(parse);

	SentenceTheory *sTheory = _docTheory->getSentenceTheory(_sentence_number);

	std::vector<bool> is_false_or_hypothetical = RelationUtilities::get()->identifyFalseOrHypotheticalProps(sTheory->getPropositionSet(), sTheory->getMentionSet());

	BOOST_FOREACH(PatternSet_ptr patternSet, _patternSets) {
		addEventMentions(sTheory, patternSet, emSet);
	}
	
	return emSet;

}

void PatternEventFinder::addEventMentions(SentenceTheory *sTheory, PatternSet_ptr patternSet, EventMentionSet *emSet)  {

	// Find all results in this sentence	
	PatternMatcher_ptr pm = PatternMatcher::makePatternMatcher(_docTheory, patternSet);
	/*	Get sentence snippets can take 3 arguments
	*	a UTF8OutputStream (not used in PatternMatcher) and a bool specifying if we want to multi-match sentences
	*	we want to force multi-matching, so we're going to pass a 0 for the UTF8OutputStream, and a true for the 3rd argument.
	*/
	std::vector<PatternFeatureSet_ptr> results = pm->getSentenceSnippets(sTheory, 0, true);

	// For each result, turn it into an EventMention
	// We expect (event_type MY_EVENT_TYPE) labels on the trigger (one per event)
	// We expect (role MY_ARG_ROLE) labels on the mentions and value mentions that serve as arguments
	int event_mention_count = 0;
	BOOST_FOREACH(PatternFeatureSet_ptr pfs, results) {
		EventMention *vm = _new EventMention(_sentence_number, event_mention_count++);
		
		for (size_t f = 0; f < pfs->getNFeatures(); f++) {
			PatternFeature_ptr feat = pfs->getFeature(f);
			if (ReturnPatternFeature_ptr rf=boost::dynamic_pointer_cast<ReturnPatternFeature>(feat)) {
				std::map<std::wstring, std::wstring> ret_values_map = rf->getPatternReturn()->getCopyOfReturnValuesMap();
				Symbol atomicLabel = rf->getReturnLabel();
				if (!atomicLabel.is_null()){
					// we have a role from an ICEWS style pattern
					ret_values_map[L"role"] = atomicLabel.to_string();
				}
				typedef std::pair<std::wstring, std::wstring> str_pair;
				BOOST_FOREACH(str_pair key_val, ret_values_map) {
					if (MentionReturnPFeature_ptr mrf=boost::dynamic_pointer_cast<MentionReturnPFeature>(rf)) {
						if (key_val.first == L"role") {
							const Mention* m = mrf->getMention();
							Symbol role(key_val.second);

							// Add children of list mentions instead of the mention itself
							if (_splitListMentions && m->getMentionType() == Mention::LIST && m->getChild() != NULL) {
								Mention* c = m->getChild();
								while (c != NULL) {
									vm->addArgument(role, c);
									c = c->getNext();
								}
							} else
								vm->addArgument(role, m);
						} else if (key_val.first == L"event_type") {
							addEventType(vm, key_val.second);
							addAnchor(vm, mrf->getMention()->getHead(), sTheory);
						}
					} else if (ValueMentionReturnPFeature_ptr vmrf=boost::dynamic_pointer_cast<ValueMentionReturnPFeature>(rf)) {
						if (key_val.first == L"role")
							vm->addValueArgument(Symbol(key_val.second), vmrf->getValueMention());
						else if (key_val.first == L"event_type")
							addEventType(vm, key_val.second);
					} else if (PropositionReturnPFeature_ptr prf=boost::dynamic_pointer_cast<PropositionReturnPFeature>(rf)) {
						if (key_val.first == L"event_type") {
							addEventType(vm, key_val.second);
							addAnchor(vm, prf->getProp()->getPredHead(), sTheory);
						}else if (key_val.first == L"role") {
							int start = 0;
							int end = 0;
							prf->getProp()->getStartEndTokenProposition(sTheory->getMentionSet(),start,end);
							vm->addSpanArgument(Symbol(key_val.second),start,end);
						}else if (key_val.first == L"pattern_id") {
							vm->setPatternID(Symbol(key_val.second));
						}else if (key_val.first == L"gainLoss") {
							vm->setGainLoss(Symbol(key_val.second));
						}else if (key_val.first == L"indicator") {
							vm->setIndicator(Symbol(key_val.second));
						}
					} else if (ParseNodeReturnPFeature_ptr prf=boost::dynamic_pointer_cast<ParseNodeReturnPFeature>(rf)) {
						if (key_val.first == L"event_type") {
							addEventType(vm, key_val.second);
							addAnchor(vm, prf->getNode(), sTheory);
						}else if (key_val.first == L"pattern_id") {
							vm->setPatternID(Symbol(key_val.second));
						}else if (key_val.first == L"gainLoss") {
							vm->setGainLoss(Symbol(key_val.second));
						}else if (key_val.first == L"indicator") {
							vm->setIndicator(Symbol(key_val.second));
						}
					} else if (TokenSpanReturnPFeature_ptr tsrf=boost::dynamic_pointer_cast<TokenSpanReturnPFeature>(rf)) {
						if (key_val.first == L"event_type")
							addEventType(vm, key_val.second);
						if (key_val.first == L"role") {
							int start_token = tsrf->getStartToken(); 
							int end_token = tsrf->getEndToken();
							if (key_val.second == L"anchor") { // Anchors are a special case

								if (start_token != end_token) {
									SessionLogger::warn("pattern_event_finder") << "Anchors must be one token long.";
								} else {
									const SynNode *node = sTheory->getPrimaryParse()->getRoot()->getNodeByTokenSpan(start_token, end_token);
									while (!node->isPreterminal() && node->getNChildren() == 1)
										node = node->getChild(0);
									addAnchor(vm, node, sTheory);
								}
							} else {
								vm->addSpanArgument(Symbol(key_val.second), start_token, end_token);
							}
						}
					}
				}
			}  
		}

		//If we did not get an event type, display a very descriptive error message.
		if (vm->getEventType() == SymbolConstants::nullSymbol )
		//	|| vm->getEventType() == Symbol(L"block")) 
		{
			std::stringstream listOfRoles;
			for (int i = 0; i < vm->getNArgs(); ++i) {
				if (listOfRoles.str().size() != 0)
					listOfRoles << ",";
				listOfRoles << vm->getNthArgRole(i) << ": ";
				if(vm->getNthArgMention(i))
					listOfRoles << vm->getNthArgMention(i)->getNode()->toDebugTextString();
			}
			for (int i = 0; i < vm->getNValueArgs(); ++i) {
				if (listOfRoles.str().size() != 0)
					listOfRoles << ",";				
				listOfRoles << vm->getNthArgValueRole(i) << ": ";
				if(vm->getNthArgValueMention(i))
					listOfRoles << vm->getNthArgValueMention(i)->toDebugString(sTheory->getTokenSequence());
			}			
			if (vm->getEventType() == SymbolConstants::nullSymbol)
				SessionLogger::warn("event_type_not_set") << "An event pattern fired without a specified event type in sentence " << pfs->getStartSentence() << ". The roles and arguments for the pattern are : '" << listOfRoles.str() << "'";

			delete vm;
		} else if (vm->getAnchorNode() == 0) {
			SessionLogger::warn("anchor_not_set") << "An event pattern fired without an anchor in sentence " << pfs->getStartSentence();

			delete vm;
		} else {
			// This might well be -1, but that's OK
			vm->setScore(pfs->getScore());

			//Don't take BLOCK event mentions
			if (vm->getEventType() != Symbol(L"block"))
				emSet->takeEventMention(vm);

			// For debugging, this can be very useful
			/*
			std::string foo = vm->toDebugString(sTheory->getTokenSequence());
			if (true || foo.find("Sentiment") != std::string::npos) {
				std::cout << sTheory->getTokenSequence()->toDebugString() << "\n";
				if (pfs->getTopLevelPatternLabel() != Symbol())
					std::cout << pfs->getTopLevelPatternLabel();
				else std::cout << "NO LABEL";
				std::cout << vm->toDebugString(sTheory->getTokenSequence());
			}
			*/

			if (vm->getEventType() == Symbol(L"block"))
				delete vm;
		}
	}
}

void PatternEventFinder::addEventType(EventMention *vm, std::wstring event_type) 
{
	if (vm->getEventType() == SymbolConstants::nullSymbol)
		vm->setEventType(event_type);
	else 
		SessionLogger::warn("pattern_event_finder") << "Tried to add event type to already-typed EventMention in sentence " << vm->getSentenceNumber() << ". Previous type = '" << vm->getEventType().to_debug_string() << "' New type = '" << event_type << "'";
}

void PatternEventFinder::addAnchor(EventMention *vm, const SynNode *node, SentenceTheory *sTheory) {
	if (vm->getAnchorNode() == 0) 
		vm->setAnchor(node, sTheory->getPropositionSet());
	else
		SessionLogger::warn("pattern_event_finder") << "Tried to add anchor to EventMention that already has one.";
}
