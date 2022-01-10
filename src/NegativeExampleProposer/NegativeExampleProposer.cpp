/** Negative Example Proposer
  *
  * This executable loops through the active seeds of a target, and uses
  * them to try to construct likely negative examples of the target.  For
  * a binary target (aka a relation), it tries pairing each seed's X with
  * new values of Y of the right type; and vice versa.
  *
  */
#include "Generic/common/leak_detection.h" // this should be the first #include
#include <string>
#include <iostream>
#include <vector>
#include <cassert>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include "boost/algorithm/string/find.hpp"
#include <boost/program_options.hpp>
#include "boost/make_shared.hpp"
#include "Generic/common/ParamReader.h"
#include "Generic/common/ConsoleSessionLogger.h"
#include "Generic/common/TimedSection.h"
#include "Generic/theories/Token.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/MentionPattern.h"
#include "Generic/patterns/ValueMentionPattern.h"
#include "LearnIt/db/LearnItDB.h" 
#include "LearnIt/SlotFiller.h"
#include "LearnIt/Target.h"
#include "LearnIt/MainUtilities.h"
#include "LearnIt/util/FileUtilities.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/Seed.h"
#include "LearnIt/MatchInfo.h"
#include "LearnIt/ProgramOptionsUtils.h"

using boost::make_shared;

typedef std::map<int, SlotFiller_ptr> SlotToFillerMap;
typedef std::vector<std::vector<SlotFiller_ptr> > SlotToFillersMap;

void showDistractors(SlotToFillerMap chosenDistractors, SlotToFillersMap distractorMap,
					 const DocTheory* doc, SentenceTheory *sentTheory, Target_ptr target,
					 int pivot_slot, Seed_ptr seed, int &num_distractors, 
					 UTF8OutputStream &out) {
	if ( num_distractors >= 5 ) //Don't process more than 5 distractors per sentence. (Takes too long)
		return;
	if (chosenDistractors.size() == distractorMap.size()) {
		++num_distractors;
		out << L"<NegativeExample target=\"" << seed->getTarget()->getName() 
			<< L"\" pivot=\"" << pivot_slot << L"\">\n";
		for (int slot_num=0; slot_num<seed->getTarget()->getNumSlots(); ++slot_num) {
			out << MatchInfo::slotFillerInfo(sentTheory, target, chosenDistractors[slot_num], slot_num);
		}
		out << L"</NegativeExample>\n";
	} else {
		// Find the first slot that we haven't chosen a distractor for.
		for (int slot_num=0; slot_num<seed->getTarget()->getNumSlots(); ++slot_num) {
			if ((chosenDistractors.find(slot_num) == chosenDistractors.end())) {
				// For each distractor that can go in that slot, recurse.
				BOOST_FOREACH(SlotFiller_ptr distractor, distractorMap[slot_num]) {
					chosenDistractors[slot_num] = distractor;
					showDistractors(chosenDistractors, distractorMap, doc, sentTheory, target, 
						            pivot_slot, seed, num_distractors, out);
				}
				chosenDistractors.erase(slot_num);
				return;
			}
		}
	}
}


int processSentence(const DocTheory* doc, int sent_index, const std::vector<Seed_ptr>& seeds,	UTF8OutputStream &out,
					TargetToFillers& targetToFiller, const MentionToEntityMap& mentionToEntity) 
{
	int num_distractors = 0;

	BOOST_FOREACH(Seed_ptr seed, seeds){
		if( seed->active() ){
			Target_ptr target = seed->getTarget();
			SentenceTheory *sentTheory = doc->getSentenceTheory(sent_index);
			TokenSequence *tok_seq = sentTheory->getTokenSequence();
	
			// This program doesn't work for unary targets.
			if (target->getAllSlotConstraints().size() == 1)
				continue;

			// slotFillerPossibilities[N] is a list of all possible slot fillers for slot N that match the seed
			SlotFillersVector slotFillerPossibilities(target->getNumSlots());
			for (SlotFillersVector::iterator it=slotFillerPossibilities.begin();
				it!=slotFillerPossibilities.end(); ++it) 
			{
				*it = boost::make_shared<SlotFillers>();
			}

			for( int i = 0; i < target->getNumSlots(); i++ ){
				SlotFiller::filterBySeed(*targetToFiller[target][i],
					*slotFillerPossibilities[i], *seed->getSlot(i),
					seed->getTarget()->getSlotConstraints(i), mentionToEntity);
			}

			int num_slots_filled = 0; //There can be 0, 1, or many
			int pivot_slot = 0;

			for( size_t i = 0; i < slotFillerPossibilities.size(); ++i ){
				if( slotFillerPossibilities[i]->size() != 0 ){
					pivot_slot = i;
					num_slots_filled += 1;
				}
				if( num_slots_filled > 1 ) //If this is true, then there are many, and just leave it at 2
					break;
			}
		
			// We're only interested in sentences where exactly one slot filler appears.
			// Fewer than one, and there's nothing to do.  More than one, and it's too
			// likely that we'd accidentally propose a known good seed as a negative example.
			if (num_slots_filled != 1) {
				continue;
			}
		
			// Find a list of distractors for each slot (other than the pivot slot).  If 
			// we can't find any distractors for any of the slots, then give up immediatley.
			std::vector< std::vector<SlotFiller_ptr> > distractors;
			for( int i = 0; i < seed->getTarget()->getNumSlots(); ++i ){
				distractors.push_back(std::vector<SlotFiller_ptr>());
			}
			for (int slot_num=0; slot_num<seed->getTarget()->getNumSlots(); ++slot_num) {
				if (slot_num != pivot_slot) {
					distractors[slot_num] = SlotFiller::getSlotDistractors(
						doc, sentTheory, target, seed, slot_num);
					if (distractors[slot_num].size() == 0) continue; // Give up.
				}
			}
	
			// Pick a slot filler to use for the pivot slot.  We require that its best name
			// exactly matches the seed string.  Other than that, we just take the first 
			// one that we find.
			BOOST_FOREACH(SlotFiller_ptr pivotSlotFiller, *slotFillerPossibilities[pivot_slot]) {
				if (pivotSlotFiller->getBestName() == seed->getSlot(pivot_slot)->name()) {
					distractors[pivot_slot].push_back(pivotSlotFiller);
					break;
				}
			}
			if (distractors[pivot_slot].empty()) continue;
	
	
			out << L"<Sentence>\n";
			out << MatchInfo::sentenceTheoryInfo(doc, sentTheory, sent_index, false);
		
			showDistractors(SlotToFillerMap(), distractors, doc, sentTheory, target, pivot_slot, seed, num_distractors, out);
		
			out << L"</Sentence>\n";
		}
	}
	return num_distractors;
}


int processDocument(const DocTheory* doc_theory, Symbol docid,
					std::vector<Seed_ptr> seeds, std::vector<Target_ptr> targets, 
					UTF8OutputStream &out, double maxTime) {
	double currTime = 0.0;
	int num_distractors = 0;

	const MentionToEntityMap_ptr mentionToEntity = 
		MentionToEntityMapper::getMentionToEntityMap(doc_theory);

	for (int sent_index = 0; sent_index < doc_theory->getNSentences(); sent_index++) {
		SentenceTheory *sent_theory = doc_theory->getSentenceTheory(sent_index);

		//This precomputes a map that given targetToFillers[target][slot_num] gives you a vector
		//of SlotFiller_ptr objects for that target and slot_num.  (Prevents re-computation)
		TargetToFillers targetToFillers	= SlotFiller::getAllSlotFillers(doc_theory, 
			docid, sent_theory, targets);

		//Finds each distractor:
		TimedSectionAccumulate timer(&currTime);
		num_distractors += processSentence(doc_theory, sent_index, seeds, out, targetToFillers, *mentionToEntity);
		if( currTime >= maxTime )
			break;
	}
	return num_distractors;
}

int main(int argc, char** argv) {
#ifdef NDEBUG
	try{
#endif
        std::string param_file;
        std::string doclist_filename;
        std::string learnit_db_name;
        std::string output_name;
        
        boost::program_options::options_description desc("Options");
	desc.add_options()
		("param-file,P", boost::program_options::value<std::string>(&param_file),
			"[required] parameter file")
		("doc-list,D", boost::program_options::value<std::string>(&doclist_filename),
			"[required] document list")
		("learnit-db,L", boost::program_options::value<std::string>(&learnit_db_name),
			"[required] learnit database file")
		("output,O", boost::program_options::value<std::string>(&output_name),
			"[required] output file name");

	boost::program_options::positional_options_description pos;
	pos.add("param-file", 1).add("doc-list", 1).add("learnit-db", 1).add("output", 1);

	boost::program_options::variables_map var_map;
	try {
		boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).positional(pos).run(), var_map);
	} catch (exception & exc) {
		std::cerr << "Command-line parsing exception: " << exc.what() << std::endl;
	}

	boost::program_options::notify(var_map);

	validate_mandatory_unique_cmd_line_arg(desc, var_map, "param-file");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "doc-list");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "learnit-db");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "output");

    ParamReader::readParamFile(param_file);
    ConsoleSessionLogger logger(std::vector<std::wstring>(), L"[NEP]");
    SessionLogger::setGlobalLogger(&logger);
	// When the unsetter goes out of scope, due either to normal termination or an exception, it will unset the logger ptr for us.
	SessionLoggerUnsetter unsetter;
	LearnItDB_ptr learnit_db = make_shared<LearnItDB>(learnit_db_name);
	UTF8OutputStream out(output_name.c_str());

	int num_distractors = 0;
	
	std::vector<Target_ptr> targets = learnit_db->getTargets();
	std::vector<Seed_ptr> seeds = learnit_db->getSeeds();
	
	// Read the list of documents to process.
	std::vector<DocIDName> w_doc_files = FileUtilities::readDocumentList(doclist_filename);
	out << L"<NegativeExamples>\n";

	// Load and process each document.
	double max_time_per_doc = 360.0 / w_doc_files.size();

	BOOST_FOREACH(DocIDName w_doc_file, w_doc_files) {
		// Load the SerifXML and get the DocTheory
		std::pair<Document*, DocTheory*> doc_pair = SerifXML::XMLSerializedDocTheory(w_doc_file.second.c_str()).generateDocTheory();

		// Set up the object id table.  We'll need this to seralize sentences.
		ObjectIDTable::initialize();
		doc_pair.second->updateObjectIDTable();	

		// Request that token sequences be saved using the default token sequence format,
		// even if we're processing a lexical token sequence (e.g., in Arabic).  This
		// makes life a little easier when we read the serialized sentences.
		Token::_saveLexicalTokensAsDefaultTokens = true;

		// For each scan the document for occurences of that seed.
		int distractors = 0;
		
		distractors += processDocument(doc_pair.second, doc_pair.first->getName(), seeds, 
			targets, out, max_time_per_doc);
		
		SessionLogger::info("LEARNIT") << "  " << distractors << " distractors" << std::endl;
		num_distractors += distractors;

		// Clean up
		delete doc_pair.second;
	}
	out << L"</NegativeExamples>\n";
	SessionLogger::info("LEARNIT") << "Found " << num_distractors << " distractors." << std::endl;

#ifdef NDEBUG
	// If something broke, then say what it was.
	} catch (UnrecoverableException &e) {
		std::cerr << "\n" << e.getMessage() << std::endl;
		std::cerr << "Error Source: " << e.getSource() << std::endl;
		return -1;
	} catch (std::exception &e) {
		std::cerr << "Uncaught Exception: " << e.what() << std::endl;
		return -1;
	}
#endif
}
