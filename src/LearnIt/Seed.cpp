#include "Generic/common/leak_detection.h"
#include <string>
#include <sstream>
#include <stdexcept>

#include "Seed.h"
#include "Target.h"
#include "SlotFiller.h"

Seed::Seed(Target_ptr target, std::vector<std::wstring> const &slots, bool active, 
		   std::wstring const &source, std::wstring const &language) : 
ObjectWithSlots(target, slots), _active(active),  _source(source),  _language(language)
{ }

Seed::~Seed() {}

std::wstring Seed::toString() const {
	std::wstringstream wss;
	wss << getTarget()->getName() << L"(" << getSlot(0)->name();
	for (size_t i=1; i<numSlots(); ++i) {
		wss << L", " << getSlot(i)->name();
	}
	wss << L")";
	return wss.str();
}
