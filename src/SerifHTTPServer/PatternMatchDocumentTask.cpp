// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include <boost/algorithm/string/replace.hpp>

#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/SimpleSlot.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/features/GenericPFeature.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLSerializedPatternFeatureSetVector.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Mention.h"
#include "SerifHTTPServer/PatternMatchDocumentTask.h"
#include "SerifHTTPServer/IncomingHTTPConnection.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/Sexp.h"

using namespace SerifXML;
using namespace xercesc;


PatternMatchDocumentTask::PatternMatchDocumentTask(xercesc::DOMDocument *document, xercesc::DOMDocument *slot1, 
												   xercesc::DOMDocument	*slot2, xercesc::DOMDocument *slot3, 
												   std::map<std::wstring, float> slot1Weights, std::map<std::wstring, float> slot2Weights,
												   std::map<std::wstring, float> slot3Weights,
												   const std::map<std::wstring,std::map<std::wstring,double> > &equiv_names,
												   OptionMap *optionMap, 
												   boost::asio::io_service &ioService, IncomingHTTPConnection_ptr connection)
: PatternSetTask(ioService, connection), _document(document), _slot1(slot1), _slot2(slot2), _slot3(slot3), 
	_slot1Weights(slot1Weights), _slot2Weights(slot2Weights), _slot3Weights(slot3Weights), _equiv_names(equiv_names), _optionMap(optionMap) {}

PatternMatchDocumentTask::~PatternMatchDocumentTask() {
	if (_document) _document->release();
	if (_slot1) _slot1->release();
	if (_slot2) _slot2->release();
	if (_slot3) _slot3->release();
	delete _optionMap;
}

std::wstring getXMLPrintableString(const std::wstring & str_in) {
	std::wstring str(str_in);
	// Special case: the input string may contain quotation marks that are 
	// already XML-escaped.  If so, then don't double-escape them.
	boost::algorithm::replace_all(str, "&quot;", "\"");
	boost::algorithm::replace_all(str, L"&", L"&amp;");
	boost::algorithm::replace_all(str, L"<", L"&lt;");
	boost::algorithm::replace_all(str, L">", L"&gt;");
	boost::algorithm::replace_all(str, L"'", L"&apos;");
	boost::algorithm::replace_all(str, L"\"", L"&quot;");

	return str;
}

std::wstring getStringFromXML(const std::wstring & str_in) {
	std::wstring str(str_in);
	// Special case: the input string may contain quotation marks that are 
	// already XML-escaped.  If so, then don't double-escape them.
	boost::algorithm::replace_all(str, "&quot;", "\"");
	boost::algorithm::replace_all(str, L"&amp;", L"&");
	boost::algorithm::replace_all(str, L"&lt;", L"<");
	boost::algorithm::replace_all(str, L"&gt;", L">");
	boost::algorithm::replace_all(str, L"&apos;", L"'");

	return str;
}

bool PatternMatchDocumentTask::run(const Symbol::HashMap<PatternSet_ptr> * patternSets) {
	bool success = false;
	std::pair<const Document*, DocTheory*> docPair(0,0);
	std::pair<const Document*, DocTheory*> slot1Pair(0,0);
	std::pair<const Document*, DocTheory*> slot2Pair(0,0);
	std::pair<const Document*, DocTheory*> slot3Pair(0,0);
	Symbol::HashMap<const AbstractSlot*> slots;
	typedef Symbol::HashMap<const AbstractSlot*>::const_iterator SlotIter;

	try {

		// Construct our slots if we've got them.
		if (_slot1) {
			XMLSerializedDocTheory slotSerializedDocTheory(_slot1);
			_slot1 = 0; // slotSerializedDocTheory took over ownership.
			slot1Pair = slotSerializedDocTheory.generateDocTheory();
			int slot_start_offset = boost::lexical_cast<int>(transcodeToStdWString((*_optionMap)[X_slot1_start_offset].c_str()));
			slots[Symbol(L"SLOT1")] = _new SimpleSlot(Symbol(L"SLOT1"), slot1Pair.second, slot_start_offset, _equiv_names, _slot1Weights);
		}
		if (_slot2) {
			XMLSerializedDocTheory slotSerializedDocTheory(_slot2);
			_slot2 = 0; // slotSerializedDocTheory took over ownership.
			slot2Pair = slotSerializedDocTheory.generateDocTheory();
			int slot_start_offset = boost::lexical_cast<int>(transcodeToStdWString((*_optionMap)[X_slot2_start_offset].c_str()));
			slots[Symbol(L"SLOT2")] = _new SimpleSlot(Symbol(L"SLOT2"), slot2Pair.second, slot_start_offset, _equiv_names, _slot2Weights);
		}
		if (_slot3) {
			XMLSerializedDocTheory slotSerializedDocTheory(_slot3);
			slot3Pair = slotSerializedDocTheory.generateDocTheory();
			_slot3 = 0; // slotSerializedDocTheory took over ownership.
			int slot_start_offset = boost::lexical_cast<int>(transcodeToStdWString((*_optionMap)[X_slot3_start_offset].c_str()));
			slots[Symbol(L"SLOT3")] = _new SimpleSlot(Symbol(L"SLOT3"), slot3Pair.second, slot_start_offset, _equiv_names, _slot3Weights);
		}

		PatternSet_ptr patternSet;
		if (_optionMap->find(X_pattern_match_pattern) != _optionMap->end()) {
			std::wistringstream wiss (getStringFromXML(transcodeToStdWString((*_optionMap)[X_pattern_match_pattern].c_str())));
			std::wcout << getStringFromXML(transcodeToStdWString((*_optionMap)[X_pattern_match_pattern].c_str())) << L"\n";
			Sexp sexp(wiss,true);
			patternSet = boost::make_shared<PatternSet>(&sexp);
		} else {
			Symbol patternSetName(transcodeToStdWString((*_optionMap)[X_pattern_set_name].c_str()).c_str());
			SessionLogger::dbg("PatternMatcher") << "Pattern Set Name: " << patternSetName.to_string() << "\n";
			if (patternSetName ==  Symbol(L"COUNT_NODES")) {
				// This is a special request to return the nodes in each Slot.
				// Make sure to call this with some <Document />, even if it is empty, to correctly find the right number of slots.
				std::wstringstream s;
				s << "<SerifXML>\n";
				for (SlotIter it = slots.begin(); it != slots.end(); ++it) {
					const SimpleSlot* slot = static_cast<const SimpleSlot*>((*it).second);
					s << "\t<Slot label=\"" << (*it).first.to_string() << "\" num_nodes=\"" << slot->getMatcher(AbstractSlot::NODE)->getNumNodes() << "\" />\n";
				}
				s << "</SerifXML>\n";
				
				std::wstring wsresult = s.str();
				std::string sresult(wsresult.begin(), wsresult.end());
				sresult.assign(wsresult.begin(), wsresult.end());
				sendResponse(sresult);
				
				success = true;
				
			} else if (patternSets->find(patternSetName) == patternSets->end())
				throw UnexpectedInputException("PatternMatchDocumentTask::run()", "Unknown pattern set name");
			else
				patternSet = (*(patternSets->find(patternSetName))).second;
		}		

		if (!success) {
			XMLSerializedDocTheory serializedDocTheory(_document);
			_document = 0; // serializedDocTheory took over ownership.
			serializedDocTheory.setOptions(*_optionMap);

			// Deserialize the document
			docPair = serializedDocTheory.generateDocTheory();
			const Document *document = docPair.first;
			DocTheory* docTheory = docPair.second;		

			// sanity check on document
			if (docTheory->getEventSet() == 0 || docTheory->getEntitySet() == 0)
				throw UnexpectedInputException("PatternMatchDocumentTask::run()", "DocTheory appears to have not been run all the way through Serif, pattern matching not allowed");

			// Run pattern matching
			PatternMatcher_ptr pm = PatternMatcher::makePatternMatcher(docTheory, patternSet, 0, PatternMatcher::DO_NOT_COMBINE_SNIPPETS, slots, &_equiv_names);
			std::vector<PatternFeatureSet_ptr> all_results;
			std::wstringstream string_result;
			for (int i = 0; i < docTheory->getNSentences(); i++) {
				SentenceTheory *sTheory = docTheory->getSentenceTheory(i);			
				std::vector<PatternFeatureSet_ptr> results = pm->getSentenceSnippets(sTheory, 0, true);
				all_results.insert(all_results.end(), results.begin(), results.end());

				if (_optionMap->find(X_pattern_match_find_seeds) != _optionMap->end()) {
					BOOST_FOREACH(PatternFeatureSet_ptr pfs, results) {
						size_t index = 0;
						string_result << L"<match text=\"";
						std::wstring text = getXMLPrintableString(sTheory->getTokenSequence()->toString(pfs->getStartToken(),pfs->getEndToken()).c_str());
						string_result << text << L"\" ";
						std::set<std::wstring> return_labels;
						while (index < pfs->getNFeatures()) {
							MentionReturnPFeature_ptr ret = boost::dynamic_pointer_cast<MentionReturnPFeature>(pfs->getFeature(index));
							if (ret) {
								std::wstring return_label = ret->getReturnLabel().to_string();
								if (return_labels.find(return_label) == return_labels.end()) {
									return_labels.insert(return_label);
									string_result << return_label << L"=\"";
									Entity* e = ret->getMention()->getEntity(docTheory);
									std::wstring arg;
									if (e) {
										arg = getXMLPrintableString(ret->getMention()->getEntity(docTheory)->getBestName(docTheory));
										if (arg == L"NO_NAME")
											arg = getXMLPrintableString(ret->getMention()->getHead()->toCasedTextString(sTheory->getTokenSequence()));
									} else {
										arg = getXMLPrintableString(ret->getMention()->getHead()->toCasedTextString(sTheory->getTokenSequence()));
									}
									string_result << arg << L"\" ";
								}
							}
							index++;
						}
						string_result << L" />\n";
					}
				}
			}
			

			if (_optionMap->find(X_pattern_match_find_seeds) != _optionMap->end()) {
				std::wstring wsresult = string_result.str();
				std::string sresult(wsresult.begin(), wsresult.end());
				sresult.assign(wsresult.begin(), wsresult.end());
				sendResponse(sresult);
			} else {
				// Loop over the feature sets and remove any GenericPFeatures
				BOOST_FOREACH(PatternFeatureSet_ptr pfs, all_results) {
					size_t index = 0;
					while (index < pfs->getNFeatures()) {
						if (boost::dynamic_pointer_cast<GenericPFeature>(pfs->getFeature(index))) {
							pfs->removeFeature(index);
						} else {
							index += 1;
						}
					}
				}

				// Send our response
				XMLSerializedPatternFeatureSetVector result(all_results, serializedDocTheory.getIdMap());
				sendResponse(result.adoptXercesXMLDoc());
			}
			success = true;	
		} 
	}

	catch (const UnexpectedInputException &exc) {
		reportError(400, exc.getMessage()); // 400 = client error
	}
	catch (const InternalInconsistencyException &exc) {
		reportError(500, exc.getMessage()); // 500 = server error
	}
	catch (const UnrecoverableException &exc) {
		reportError(500, exc.getMessage()); // 500 = server error 
	}	
	catch (const std::exception &exc) {
		reportError(500, exc.what()); // 500 = server error 
	}

	delete docPair.first;  // the Document (or NULL)
	delete docPair.second; // the DocTheory (or NULL)

	delete slot1Pair.first;  // the Document (or NULL)
	delete slot1Pair.second; // the DocTheory (or NULL)

	delete slot2Pair.first;  // the Document (or NULL)
	delete slot2Pair.second; // the DocTheory (or NULL)

	delete slot3Pair.first;  // the Document (or NULL)
	delete slot3Pair.second; // the DocTheory (or NULL)

	for (Symbol::HashMap<const AbstractSlot*>::iterator iter=slots.begin(); iter!=slots.end(); ++iter) {
		delete (*iter).second; // AbstractSlot (or NULL)
	}

	return success;

}
