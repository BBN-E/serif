//Copyright 2011 BBN Technologies
//All rights reserved
#include "Generic/common/leak_detection.h"

#include "English/values/en_PatternEventValueRecognizer.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/EventSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/wordnet/xx_WordNet.h"

EnglishPatternEventValueRecognizer::EnglishPatternEventValueRecognizer() {
	//read in the files for the various EventValue Types

	for (int i = 0; i <= SENTENCE; i++) {
		// Decide where to read the patterns from.
		std::string patternFileLocation;
		if (i == CRIME) {
			patternFileLocation = ParamReader::getRequiredParam("crime_value_pattern_file");
		} else if (i == POSITION) {
			patternFileLocation = ParamReader::getRequiredParam("position_value_pattern_file");
		} else if (i == SENTENCE) {
			patternFileLocation = ParamReader::getRequiredParam("sentence_value_pattern_file");
		} else {
			throw InternalInconsistencyException(
				"EnglishPatternEventValueRecognizer::EnglishPatternEventValueRecognizer",
				"Unexpected enum value");
		}
		// Read the patterns.
		PatternSet_ptr ps = boost::make_shared<PatternSet>(patternFileLocation.c_str());
		_eventValuePatternSets.push_back(ps);
	}

}

EnglishPatternEventValueRecognizer::~EnglishPatternEventValueRecognizer() {
	_valueMentions.clear();
}


void EnglishPatternEventValueRecognizer::createEventValues(DocTheory* docTheory) {
	//check to make sure docTheory is valid
	if(docTheory == 0)
		return;

	//remove any valueMentions that may still be around
	_valueMentions.clear();

	EventSet* eventSet = docTheory->getEventSet();

	if(eventSet != 0) {

		//make our pattern matchers
		std::vector<PatternMatcher_ptr> patternMatchers;
		BOOST_FOREACH(PatternSet_ptr ps, _eventValuePatternSets) {
			PatternMatcher_ptr pm = PatternMatcher::makePatternMatcher(docTheory, ps);
			patternMatchers.push_back(pm);
		}

		for(int i = 0; i < eventSet->getNEvents(); i++) {
			//essentially, we are going through each event mention in this document
			Event *event = eventSet->getEvent(i);
			Event::LinkedEventMention* vment = event->getEventMentions();
			while(vment != 0) {
				int sentno = vment->eventMention->getSentenceNumber();
				SentenceTheory* sTheory = docTheory->getSentenceTheory(sentno);

				std::vector<PatternFeatureSet_ptr> matchingResults;

				bool foundPosition = false;
				bool foundSentence = false;

				//see if the event mention is of the correct type, and therefore if we should try to match to it
				if(getBaseType(vment->eventMention->getEventType()) == Symbol(L"Justice") &&
					vment->eventMention->getFirstValueForSlot(Symbol(L"Crime")) == 0) 
				{
					//we have a Justice event without a crime - see if we can find a Crime value
					std::vector<PatternFeatureSet_ptr> tempResults = patternMatchers[CRIME]->getSentenceSnippets(sTheory, 0, true);
					if (tempResults.size() > 0) {
						SessionLogger::dbg("EnglishPatternEventValueRecognizer") << "Event " << i << " sent " << sentno << " has " << tempResults.size() << " crime values";
					}
					matchingResults.insert(matchingResults.end(), tempResults.begin(), tempResults.end());
				} 
				if(vment->eventMention->getEventType() == Symbol(L"Justice.Sentence") &&
					vment->eventMention->getFirstValueForSlot(Symbol(L"Sentence")) == 0) 
				{
					//see if there is a Sentence value we should match
					std::vector<PatternFeatureSet_ptr> tempResults = patternMatchers[SENTENCE]->getSentenceSnippets(sTheory, 0, true);
					if (tempResults.size() > 0) {
						SessionLogger::dbg("EnglishPatternEventValueRecognizer") << "Event " << i << " sent " << sentno << " has " << tempResults.size() << " sentence values";
						foundSentence = true;
					}
					matchingResults.insert(matchingResults.end(), tempResults.begin(), tempResults.end());
				}
				if(getBaseType(vment->eventMention->getEventType()) == Symbol(L"Personnel") &&
					vment->eventMention->getFirstMentionForSlot(Symbol(L"Position")) == 0)
				{
					//There might be a Position value we should match
					std::vector<PatternFeatureSet_ptr> tempResults = patternMatchers[POSITION]->getSentenceSnippets(sTheory, 0, true);
					//see if we were sucessful
					if (tempResults.size() > 0) {
						SessionLogger::dbg("EnglishPatternEventValueRecognizer") << "Event " << i << " sent " << sentno << " has " << tempResults.size() << " position values";
						foundPosition = true;
					}
					matchingResults.insert(matchingResults.end(), tempResults.begin(), tempResults.end());
				}

				//We should now have PatternFeatureSets for each of the values we have found
				//add their information to the event mention
				BOOST_FOREACH(PatternFeatureSet_ptr pfs, matchingResults) {
					
					//set the coverage for this set
					pfs->setCoverage(docTheory);


					for (size_t f = 0; f < pfs->getNFeatures(); f++) {
						PatternFeature_ptr feat = pfs->getFeature(f);
						if(ReturnPatternFeature_ptr rf = boost::dynamic_pointer_cast<ReturnPatternFeature>(feat)) {
							std::map<std::wstring, std::wstring> returnValuesMap = rf->getPatternReturn()->getCopyOfReturnValuesMap();
							typedef std::pair<std::wstring, std::wstring> str_pair;
							BOOST_FOREACH(str_pair keyVal, returnValuesMap) {
								if(keyVal.first == L"value") {
									if(MentionReturnPFeature_ptr mrf = boost::dynamic_pointer_cast<MentionReturnPFeature>(rf)) {
										const SynNode* node = mrf->getMention()->getNode();
										while (node != 0 && node->getParent() != 0 && node->getParent()->getHead() == node) {
											node = node->getParent();
										}
										if(node)
											addValueMention(docTheory, vment->eventMention, mrf->getMention()->getNode(), keyVal.second, 1.0);
									} else if (ValueMentionReturnPFeature_ptr vmrf=boost::dynamic_pointer_cast<ValueMentionReturnPFeature>(rf)) {
										const SynNode* node = sTheory->getPrimaryParse()->getRoot()->getNodeByTokenSpan(vmrf->getStartToken(), vmrf->getEndToken());
										while (node != 0 && node->getParent() != 0 && node->getParent()->getHead() == node) {
											node = node->getParent();
										}
										if(node)
											addValueMention(docTheory, vment->eventMention, node, keyVal.second, 1.0);
									} else if (PropositionReturnPFeature_ptr prf = boost::dynamic_pointer_cast<PropositionReturnPFeature>(rf)) {
										//figure out what node we want by looking, starting at the prop's predhead for a parent node that has itself as it's head
										//in other words, find the head of this prop phrase?
										const SynNode* node = prf->getProp()->getPredHead();
										while (node != 0 && node->getParent() != 0 && node->getParent()->getHead() == node) {
											node = node->getParent();
										}
										if(node)
											addValueMention(docTheory, vment->eventMention, node, keyVal.second, 1.0);

									} else if (TokenSpanReturnPFeature_ptr tsrf = boost::dynamic_pointer_cast<TokenSpanReturnPFeature>(rf)) {
										const SynNode* node = sTheory->getPrimaryParse()->getRoot()->getNodeByTokenSpan(tsrf->getStartToken(), tsrf->getEndToken());
										while (node != 0 && node->getParent() != 0 && node->getParent()->getHead() == node) {
											node = node->getParent();
										}
										if(node)
											addValueMention(docTheory, vment->eventMention, node, keyVal.second, 1.0);
									}
								}
							}
						}
					}
				}

				//add value mentions not using patterns
				//see if our event mention has a Position already, and add it if it doesn't (addPosition will do the necessary checks if this the right kind of event
				if(vment->eventMention->getFirstMentionForSlot(Symbol(L"Position")) == 0)
					addPosition(docTheory, vment->eventMention);

				//If we have found a Crime value
				//This adds that crime value to any other EventMention in any argument in the sentence that might need it (I think).
				if(vment->eventMention->getFirstValueForSlot(Symbol(L"Crime"))) {
					transferCrime(vment->eventMention, docTheory->getSentenceTheory(sentno)->getMentionSet(), docTheory->getSentenceTheory(sentno)->getPropositionSet(), docTheory->getSentenceTheory(sentno)->getEventMentionSet());
				}

				////If we have not yet found a Justice sentence value, check every prop in the sentence (gramatical structure) for a CertainSentence
				////As of 8/10/11, this just checks every prop in the sentence for a noun phrase involving a day, month, year, maximum, or minimum.
				//if( vment->eventMention->getEventType() == Symbol(L"Justice.Sentence") &&
				//	vment->eventMention->getFirstValueForSlot(Symbol(L"Sentence")) == 0) {
				//		//getSentenceSnippets checks every prop in the sentence.
				//		std::vector<PatternFeatureSet_ptr> certainSentencesResults = patternMatchers[CERTAIN_SENTENCES]->getSentenceSnippets(sTheory);
				//		
				//}

				vment = vment->next;
			}
		}
	}
	//Create the value mention set. Since it's a document-level value mention set, it has a NULL tokenSequence pointer
	ValueMentionSet* valueMentionSet = _new ValueMentionSet(NULL, static_cast<int>(_valueMentions.size()));
	for(size_t j = 0; j < _valueMentions.size(); ++j) {
		valueMentionSet->takeValueMention(static_cast<int>(j), _valueMentions[j]);
	}
	docTheory->takeDocumentValueMentionSet(valueMentionSet);
}

Symbol EnglishPatternEventValueRecognizer::getBaseType(Symbol fullType) {
	std::wstring str = fullType.to_string();
	size_t index = str.find(L".");
	return Symbol(str.substr(0, index).c_str());
}

void EnglishPatternEventValueRecognizer::addValueMention(DocTheory* docTheory, EventMention* vment, const Mention* ment, std::wstring returnValue, float score) 
{
	addValueMention(docTheory, vment, ment->getNode(), returnValue, score);
}

void EnglishPatternEventValueRecognizer::addValueMention(DocTheory* docTheory, EventMention* vment, const SynNode* node, std::wstring returnValue, float score) {
	//make the appropriate symbols for the Value type and ValueArgument type
	Symbol valueType;
	Symbol argumentType;
	if(returnValue.compare(L"Crime") == 0) {
		valueType = Symbol(L"Crime");
		argumentType = Symbol(L"Crime");
	} else if (returnValue.compare(L"Sentence") == 0) {
		valueType = Symbol(L"Sentence");
		argumentType = Symbol(L"Sentence");
	} else if (returnValue.compare(L"Position") == 0) {
		//watch out for this one - it seems like the hyphen can sometimes throw off ValueType if it isn't the right hyphen.
		valueType = Symbol(L"Job-Title");
		argumentType = Symbol(L"Position");
	} else {
		std::stringstream errorMessage;
		errorMessage<< "Pattern returned an unrecognized value type: '";
		errorMessage<< returnValue;
		errorMessage<< "'";
		throw UnexpectedInputException("EnglishPatternEventValueRecognizer", errorMessage.str().c_str());
	}

	ValueMention* val = 0;
	//check for duplicates
	for(size_t i = 0; i < _valueMentions.size(); i++) {
		if( (_valueMentions[i]->getSentenceNumber() == vment->getSentenceNumber()) &&
			(_valueMentions[i]->getFullType().getNameSymbol() == valueType || _valueMentions[i]->getFullType().getNicknameSymbol() == valueType) &&
			(_valueMentions[i]->getStartToken() == node->getStartToken()) &&
			(_valueMentions[i]->getEndToken() == node->getEndToken())) 
		{
			val = _valueMentions[i];
			break;
		}
	}

	//i.e. if we have not already added this ValueMeniton, create the object
	if(val == 0) {
		try {
			ValueMentionUID uid = ValueMention::makeUID(docTheory->getNSentences(), static_cast<int>(_valueMentions.size()), true);
			val = _new ValueMention(vment->getSentenceNumber(), uid, node->getStartToken(), node->getEndToken(), valueType);
			_valueMentions.push_back(val);		
		} catch (InternalInconsistencyException &e) {
			SessionLogger::err("value_mention_uid") << e;
		}
	}

	////check for duplicates
	//for(int valIndex = 0; valIndex < vment->getNValueArgs(); ++valIndex) {
	//	//see if it's the correct role type
	//	if(vment->getNthArgValueRole(valIndex) == argumentType) {
	//		//if it is, see if the stored ValueMention equals the one we want to add
	//		const ValueMention* inPlaceMention = vment->getNthArgValueMention(valIndex);
	//		if( (inPlaceMention->getSentenceNumber() == val->getSentenceNumber()) &&
	//			(inPlaceMention->getFullType().getNameSymbol() == valueType || inPlaceMention->getFullType().getNicknameSymbol() == valueType) &&
	//			(inPlaceMention->getStartToken() == val->getStartToken()) &&
	//			(inPlaceMention->getEndToken() == val->getEndToken())
	//			)
	//		{
	//			//we're readding a ValueMention with the same content
	//			//don't do it!
	//			//_valueMentions.pop_back();
	//			return;
	//		}
	//	}
	//}	

	//add the ValueMention to the EventMention as an argument
	vment->addValueArgument(argumentType, val, score);
}

void EnglishPatternEventValueRecognizer::addValueMention(DocTheory* docTheory, EventMention* vment, int startToken, int endToken, std::wstring returnValue, float score) {
//make the appropriate symbols for the Value type and ValueArgument type
	Symbol valueType;
	Symbol argumentType;
	if(returnValue.compare(L"Crime") == 0) {
		valueType = Symbol(L"Crime");
		argumentType = Symbol(L"Crime");
	} else if (returnValue.compare(L"Sentence") == 0) {
		valueType = Symbol(L"Sentence");
		argumentType = Symbol(L"Sentence");
	} else if (returnValue.compare(L"Position") == 0) {
		//watch out for this one - it seems like the hyphen can sometimes throw off ValueType if it isn't the right hyphen.
		valueType = Symbol(L"Job-Title");
		argumentType = Symbol(L"Position");
	} else {
		std::stringstream errorMessage;
		errorMessage<< "Pattern returned an unrecognized value type: '";
		errorMessage<< returnValue;
		errorMessage<< "'";
		throw UnexpectedInputException("EnglishPatternEventValueRecognizer", errorMessage.str().c_str());
	}

	ValueMention* val = 0;
	//check for duplicates
	for(size_t i = 0; i < _valueMentions.size(); i++) {
		if( (_valueMentions[i]->getSentenceNumber() == vment->getSentenceNumber()) &&
			(_valueMentions[i]->getFullType().getNameSymbol() == valueType || _valueMentions[i]->getFullType().getNicknameSymbol() == valueType) &&
			(_valueMentions[i]->getStartToken() == startToken) &&
			(_valueMentions[i]->getEndToken() == endToken) )
		{
			val = _valueMentions[i];
			break;
		}
	}

	//i.e. if we have not already added this ValueMeniton, create the object
	if(val == 0) {
		try {
			ValueMentionUID uid = ValueMention::makeUID(docTheory->getNSentences(), static_cast<int>(_valueMentions.size()), true);
			val = _new ValueMention(vment->getSentenceNumber(), uid, startToken, endToken, valueType);
			_valueMentions.push_back(val);
		} catch (InternalInconsistencyException &e) {
			SessionLogger::err("value_mention_uid") << e;
		}
	}

	////check for duplicates
	//for(int valIndex = 0; valIndex < vment->getNValueArgs(); ++valIndex) {
	//	//see if it's the correct role type
	//	if(vment->getNthArgValueRole(valIndex) == argumentType) {
	//		//if it is, see if the stored ValueMention equals the one we want to add
	//		const ValueMention* inPlaceMention = vment->getNthArgValueMention(valIndex);
	//		if( (inPlaceMention->getSentenceNumber() == val->getSentenceNumber()) &&
	//			(inPlaceMention->getFullType().getNameSymbol() == valueType || inPlaceMention->getFullType().getNicknameSymbol() == valueType) &&
	//			(inPlaceMention->getStartToken() == val->getStartToken()) &&
	//			(inPlaceMention->getEndToken() == val->getEndToken())
	//			)
	//		{
	//			//we're readding a ValueMention with the same content
	//			//don't do it!
	//			//_valueMentions.pop_back();
	//			return;
	//		}
	//	}
	//}

	//add the ValueMention to the EventMention as an argument
	if (val != 0)
		vment->addValueArgument(argumentType, val, score);
}


bool EnglishPatternEventValueRecognizer::isJobTitle(Symbol word) {
	WordNet* wordNet = WordNet::getInstance();

	if(wordNet->isHyponymOf(word, "leader"))
		return true;
	if(wordNet->isHyponymOf(word, "worker"))
		return true;

	return false;
}

void EnglishPatternEventValueRecognizer::addPosition(DocTheory *docTheory, EventMention *vment) {
	//This method was taken from the DepreceatedEventValueRecognizer, which I did not write and does not have any comments. I'm trying to comment it as best as I can, but I'm not 100% sure I understand what's going on. -WR 8/9/11
	if(getBaseType(vment->getEventType()) != Symbol(L"Personnel"))
		return;

	SentenceTheory* thisSentence = docTheory->getSentenceTheory(vment->getSentenceNumber());

	for (int i = 0; i < vment->getNArgs(); ++i) {
		if(vment->getNthArgRole(i) == Symbol(L"Person")) {
			//if it is a normal mention that WordNet likes, add it's value mention.
			const Mention* personMent = vment->getNthArgMention(i);
			if(personMent->mentionType == Mention::DESC &&
				isJobTitle(personMent->getNode()->getHeadWord())) 
			{
				addValueMention(docTheory, vment, personMent->getNode(), L"Position", 0.5);
				return;
			}
			//Get the entity that is refered to by the mention?
			Entity* entity = docTheory->getEntitySet()->getEntityByMention(personMent->getUID());
			if(entity == 0)
				continue;
			for (int j = 0; j < entity->getNMentions(); j++) {
				if(Mention::getSentenceNumberFromUID(entity->getMention(j)) == vment->getSentenceNumber()) {
					//try to get the mention in a different way, maybe one that has an approprate Value?
					int index = Mention::getIndexFromUID(entity->getMention(j));
					const Mention* ment = thisSentence->getMentionSet()->getMention(index);
					if(ment->mentionType == Mention::DESC && isJobTitle(ment->getNode()->getHeadWord())) {
						addValueMention(docTheory, vment, ment->getNode(), L"Position", 0.5);
						return;
					}
				}
			}
		}
	}

}

void EnglishPatternEventValueRecognizer::transferCrime(EventMention *vment, MentionSet *mentionSet,
										 const PropositionSet *propSet,
										 EventMentionSet* vmSet)
{
	for (int p = 0; p < propSet->getNPropositions(); p++) {
		const Proposition *prop = propSet->getProposition(p);
		Symbol propRole = Symbol();
		if (prop == vment->getAnchorProp())
			propRole = Symbol(L"trigger");
		else {
			for (int a = 0; a < prop->getNArgs(); a++) {
				Argument *arg = prop->getArg(a);
				if (arg->getType() == Argument::PROPOSITION_ARG && 
					arg->getProposition() == vment->getAnchorProp())
				{
					propRole = arg->getRoleSym();
					break;
				} else if (arg->getType() == Argument::MENTION_ARG && 
					arg->getMention(mentionSet)->getNode()->getHeadPreterm() 
					== vment->getAnchorNode()->getHeadPreterm())
				{
					propRole = arg->getRoleSym();
					break;
				}
			}
		}
		if (propRole.is_null())
			continue;
		for (int v = 0; v < vmSet->getNEventMentions(); v++) {
			Symbol vment2Role = Symbol();
			EventMention *vment2 = vmSet->getEventMention(v);
			if (vment2->getFirstValueForSlot(L"Crime") != 0)
				continue;
			if (getBaseType(vment2->getEventType()) != Symbol(L"Justice"))
				continue;
			if (vment == vment2)
				continue;
			if (prop == vment2->getAnchorProp())
				vment2Role = Symbol(L"trigger");
			else {
				for (int a = 0; a < prop->getNArgs(); a++) {
					Argument *arg = prop->getArg(a);
					if (arg->getType() == Argument::PROPOSITION_ARG && 
						arg->getProposition() == vment2->getAnchorProp())
					{
						vment2Role = arg->getRoleSym();
						break;
					} else if (arg->getType() == Argument::MENTION_ARG && 
						arg->getMention(mentionSet)->getNode()->getHeadPreterm() 
						== vment2->getAnchorNode()->getHeadPreterm())
					{
						vment2Role = arg->getRoleSym();
						break;
					}
				}
			}
			if (vment2Role.is_null())
				continue;
			const ValueMention *crime = vment->getFirstValueForSlot(Symbol(L"Crime"));
			if (crime != 0) {
				vment2->addValueArgument(Symbol(L"Crime"), crime, 0);
			}
		}
	}
}

