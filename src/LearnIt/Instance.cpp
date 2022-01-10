#include "Generic/common/leak_detection.h"
#include <vector>
#include <string>
#include <sstream>
#include "Generic/theories/DocTheory.h"
#include "Target.h"
#include "Instance.h"

using std::wstring;
using std::vector;

Instance::Instance(Target_ptr target, const vector<wstring> & slots,
				   int iteration, float score, const wstring& docid,
				   int sentence, const wstring& source) :
	ObjectWithSlots(target, slots), _iteration(iteration), 
		_score(score), _docid(docid), _sentence(sentence), _source(source) { }
Instance::~Instance() {}

int Instance::iteration() const {return _iteration; }
float Instance::score() const {return _score; }
const Symbol& Instance::docid() const {return _docid; }
int Instance::sentence() const { return _sentence; }
const std::wstring& Instance::source() const {return _source;}

wstring Instance::toString() const {
	std::wstringstream wss;
	wss << getTarget()->getName() << L"(" << 
		commaSeparatedSlots() << ", " << docid() << ", " << sentence() << L")";
	return wss.str();
}

bool Instance::findInSentence( 
		const SlotFillersVector& slotFillersVector,
		const DocTheory* dt, const Symbol docid, int sent_no,
		std::vector<SlotFillerMap>& output,
        const std::set<Symbol>& overlapIgnoreWords) 
{
	if (sent_no == _sentence && docid == _docid) {
		return ObjectWithSlots::findInSentence(slotFillersVector, dt,
			docid, sent_no, output);
	} else {
		return false;
	}
}

