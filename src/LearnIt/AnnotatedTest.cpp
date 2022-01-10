#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include <vector>
#include "boost/shared_ptr.hpp"
#include "boost/foreach.hpp"
#include "boost/regex.hpp"
#include "boost/make_shared.hpp"
#include "Generic/common/UnicodeUtil.h"
#include "LearnIt/Target.h"
#include "LearnIt/MainUtilities.h"
#include "LearnIt/AnnotatedTest.h"

std::vector<AnnotatedTest_ptr> AnnotatedTest::readAnnotatedTestsFromFile(Target_ptr target, AnnotationType annotation_type, std::string filename){

	std::string directory = "\\\\titan2\\u21\\IterativeLearning\\old_english_pipeline\\distill-docs-7z\\gigaword\\";
	std::vector<AnnotatedTest_ptr> annotated_test_vector;
	std::ifstream docs_wifstream(filename.c_str());
		
	std::string target_name = UnicodeUtil::toUTF8StdString(target->getName());
	std::string simple_target_name;
	//remove (X,Y) from target names
	boost::regex target_regexp("^(\\S+)\\(\\S+\\)$");
	boost::match_results<std::string::const_iterator> what;
	bool matched = boost::regex_search(target_name, what, target_regexp);

	if(matched)
		simple_target_name = MainUtilities::normalizeString(what[1]);
	else
		simple_target_name = MainUtilities::normalizeString(target_name);
	

	boost::regex line_regexp;
	//boost::match_results<std::string::const_iterator> what;
	int n_lines = 0;
	int n_unique_instances = 0;
	
	
	if(annotation_type == RECALL_ANNOTATED){
		line_regexp = boost::regex("^(\\S+)\\tgigaword[\\\\\\/](\\S+)\\t(\\d+)\\t(\\S.*\\S)\\t(\\S.*\\S)\\s*$");
	}
	else if(annotation_type == PRECISION_ANNOTATED){
		line_regexp = boost::regex("^(\\S+)\\t(\\S+)\\tgigaword[\\\\\\/](\\S+)\\t(\\d+)\\t(\\S.*\\S)\\t(\\S.*\\S)\\s*$");
	}
	std::string line; 
	while (getline(docs_wifstream, line)) {
		bool matched = boost::regex_search(line, what, line_regexp);
		if(!matched){
			SessionLogger::info("LEARNIT")<<"WARNING: Didn't match line: "<<line<<std::endl;
			continue;
		}
		std::string normalized_input_target = "UNKNOWN";
		std::string document_id = what[2];
		int sent_num; 
		std::string slot_x; 
		std::string slot_y;
		bool correct_answer = true;
		if(annotation_type == AnnotatedTest::RECALL_ANNOTATED){
			normalized_input_target = MainUtilities::normalizeString(what[1]);
			document_id = what[2];
			sent_num = atoi(what[3].str().c_str());
			slot_x = what[4];
			slot_y = what[5];
		}
		else if(annotation_type == PRECISION_ANNOTATED){
			if(what[1] == "n"){
				correct_answer = false;
			}
			normalized_input_target = MainUtilities::normalizeString(what[2]);
			document_id = what[3];
			sent_num = atoi(what[4].str().c_str());
			slot_x = what[5];
			slot_y = what[6];
		}
		else{
			SessionLogger::info("LEARNIT")<<"Warning: AnnotatedTest::readAnnotatedTestsFromFile() unknown annotation type"<<std::endl; 
		}

		if(simple_target_name == normalized_input_target ){

			std::string docid_prefix = document_id.substr(0,9);

			std::string document_full_path = directory+docid_prefix+"\\"+document_id+".distill_doc.7z";

			AnnotatedTest_ptr last_t = 
				boost::make_shared<AnnotatedTest>("NONE", "NONE", annotation_type,  target, L"NONE", L"NONE", 0, true);
			
			if(!annotated_test_vector.empty())
				last_t = annotated_test_vector.back();

			if(annotated_test_vector.empty() || last_t->_document_id.compare(document_id) != 0){
				AnnotatedTest_ptr test = boost::make_shared<AnnotatedTest>(document_full_path, document_id, annotation_type, target, 
					UnicodeUtil::toUTF16StdString(slot_x), UnicodeUtil::toUTF16StdString(slot_y), sent_num, correct_answer);
				annotated_test_vector.push_back(test);
				n_unique_instances++;
			}
			else if(last_t->_document_id.compare(document_id) == 0){ //assumptions based on file format, assume that sentences change before seeds
				if( annotation_type == AnnotatedTest::RECALL_ANNOTATED){
					if((int)last_t->annotated_sentences.back() != sent_num){
						annotated_test_vector[annotated_test_vector.size()-1]->annotated_sentences.push_back(sent_num);
						annotated_test_vector[annotated_test_vector.size()-1]->judgments.push_back(correct_answer);
						n_unique_instances++;
					}
				}
				if( annotation_type == AnnotatedTest::PRECISION_ANNOTATED){
					AnnotatedTest_ptr test = boost::make_shared<AnnotatedTest>(document_full_path, document_id, annotation_type, target, 
						UnicodeUtil::toUTF16StdString(slot_x), UnicodeUtil::toUTF16StdString(slot_y), sent_num, correct_answer);
					if(((int)last_t->annotated_sentences.back() != sent_num) && (last_t->_slot_x == test->_slot_x) && (last_t->_slot_y == test->_slot_y) ){
						; //slots are the same, skip
					}
					else{ 
						//should refactor to move slots to the sentence level, but for now just load the document twice
						annotated_test_vector.push_back(test);
						n_unique_instances++;
					}				
				}
			}
			
			n_lines++;
			}
		}
	
	return annotated_test_vector;
}


AnnotatedTest::AnnotatedTest(std::string doc_path, std::string doc_id, AnnotationType annotation_type, Target_ptr target, 
							 std::wstring x, std::wstring y, int sent, bool correct_answer){
	_document_id = doc_id;
	_document_file = doc_path;
	_slot_x = x;
	_slot_y = y;
	std::vector<std::wstring> slots;
	slots.push_back(_slot_x);
	slots.push_back(_slot_y);
	_seed = Seed_ptr(new Seed(target, slots, true, L"AnnotatedTest"));
	annotated_sentences.push_back(sent);
	judgments.push_back(correct_answer);
}


