#pragma once

#include <vector>
#include <string>
#include "Generic/common/bsp_declare.h"
#include "ObjectWithSlots.h"

class Target;
class DocTheory;
typedef boost::shared_ptr<Target> Target_ptr;

class Instance : public ObjectWithSlots {
public:
	Instance(Target_ptr target, const std::vector<std::wstring> & slots, 
		int iteration, float score, const std::wstring& docid, int sentence, 
		const std::wstring& source);
	~Instance();

	int iteration() const;
	float score() const;
	const Symbol& docid() const;
	int sentence() const;
	const std::wstring& source() const;

	std::wstring toString() const;

	bool findInSentence( 
		const SlotFillersVector& slotFillersVector,
		const DocTheory* doc, Symbol docid, int sent_no,
		std::vector<SlotFillerMap>& output,
        const std::set<Symbol>& overlapIgnoreWords = std::set<Symbol>());
private:
	int _iteration;
	float _score;
	Symbol _docid;
	int _sentence;
	std::wstring _source;
};

typedef boost::shared_ptr<Instance> Instance_ptr;

