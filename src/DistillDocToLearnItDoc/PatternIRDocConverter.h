#ifndef _PATTERN_IR_DOC_CONVERTER_H_
#define _PATTERN_IR_DOC_CONVERTER_H_

#include <string>
#include <vector>
#include <iostream>
#include "LearnItDocConverter.h"

class Annotation;
class Entity;
class EntitySet;
class SentenceTheory;

class PatternIRDocConverter : public LearnItSentenceConverter {
public:
	PatternIRDocConverter() : LearnItSentenceConverter() {}
	void processSentence(const DocTheory* dt, int sn, 
		std::wostream& out);

	static std::pair<bool, bool> entityHasNameDescMention(const Entity* entity, 
		const EntitySet* es);

	//	void prepare(const DocumentInfo_ptr& doc_info) {};
private:
	void initAnnotationRegexes();
	void addRegexAnnotations(const DocTheory* dt,
		SentenceTheory* st, std::vector<Annotation>& annotations);
	void addMentionAnnotations(const SentenceTheory* st, 
		const EntitySet* es, std::vector<Annotation>& annotations);
	void addValueAnnotations(const SentenceTheory* st, 
		std::vector<Annotation>& annotations);
	void printSentenceFullText(const DocTheory* dt, SentenceTheory* st, 
		const EntitySet* es, std::wostream& out);
	
	static std::wstring MENTION_ANN, MIN_ENT_DESC_ANN, MIN_ENT_NAME_ANN,
				MENTION_NAME, MENTION_DESC,
				FULL_TEXT, _FULL_TEXT, PROP, _PROP, PRED, _PRED;

	typedef std::pair<std::wstring, std::wstring> AnnotationRegex;
	static std::vector<AnnotationRegex> REGEXES;
};

#endif
