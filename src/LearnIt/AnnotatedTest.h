#pragma once
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "LearnIt/Target.h"
#include "boost/noncopyable.hpp"
#include "boost/shared_ptr.hpp"
#include "LearnIt/Seed.h"


class AnnotatedTest;
typedef boost::shared_ptr<AnnotatedTest> AnnotatedTest_ptr;

class AnnotatedTest: private boost::noncopyable {
public:
//	typedef enum {RECALL_ANNOTATED, PRECISION_ANNOTATED} AnnotationType;
	typedef enum {RECALL_ANNOTATED, PRECISION_ANNOTATED} AnnotationType;

	AnnotatedTest(std::string doc_path, std::string doc_id, AnnotationType annotation_type, Target_ptr target, 
		std::wstring x, std::wstring y, int sent, bool correct_answer);
	static std::vector<AnnotatedTest_ptr>readAnnotatedTestsFromFile(Target_ptr target, AnnotationType annotation_type, std::string filename);
	Seed_ptr getSeed() {return _seed;};
	std::string getFileName(){return _document_file;};
	std::string getDocID(){return _document_id;};
	
	std::vector<bool> judgments;
	std::vector<int> annotated_sentences;
	
private:
	std::string _document_file;
	std::string _document_id;

	std::wstring _slot_x;
	std::wstring _slot_y;
	Seed_ptr _seed;
	AnnotationType _annotation_type; //assumes that all sentences in a document have the same annotation type

};





	






/*
class RecallTest{
public:
	std::string document_id;
	std::string document_id_short_string;

	std::wstring slot_x;
	std::wstring slot_y;
	Seed_ptr seed;
	std::vector<int> recall_sentences;
	RecallTest(std::string d, std::string d_short, Target_ptr target, std::wstring x, std::wstring y, int sent){
			document_id = d;
			document_id_short_string = d_short;
			slot_x = x;
			slot_y = y;
			seed = Seed_ptr(new Seed(target, slot_x, slot_y, true));

			recall_sentences.push_back(sent);	
	}
} ;
*/
