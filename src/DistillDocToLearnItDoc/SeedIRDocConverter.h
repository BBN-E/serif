#ifndef _SEED_DOC_IR_CONVERTER_H_
#define _SEED_DOC_IR_CONVERTER_H_

#include <set>
#include <string>
#include <iostream>
#include "LearnItDocConverter.h"

class DocTheory;
class SentenceTheory;

class SeedIRDocConverter : public LearnItSentenceConverter {
public:
	SeedIRDocConverter() : LearnItSentenceConverter() {}
protected:
	virtual void processSentence(const DocTheory* dt, int sn,
		std::wostream& out);
private:
	void printPotentialSlotFillers(const DocTheory* dt, const SentenceTheory* st, 
		std::wostream& output) const;
	void getMentionSlotFillerStrings(const DocTheory* dt, const SentenceTheory* st,
		std::set<std::wstring>& slotFillerStrings) const;
	void getValueSlotFillerStrings(const DocTheory* dt, const SentenceTheory* st,
		std::set<std::wstring>& slotFillerStrings) const;
	void addBackedOffDates(const std::wstring& timexString,
		std::set<std::wstring>& slotFillerStrings) const;


	static std::wstring ENTITY_LIST, _ENTITY_LIST;
};
#endif
