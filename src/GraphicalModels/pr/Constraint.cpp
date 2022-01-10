#include "Constraint.h"

void GraphicalModel::DumpUtils::dumpIntVector(const std::vector<unsigned int>& vec, std::wostream& out) {
	bool first = true;
	for (std::vector<unsigned int>::const_iterator it = vec.begin();
			it != vec.end(); ++it)
	{
		if (!first) {
			out << L", ";
		}
		first = false;
		out << *it;
	}
}

