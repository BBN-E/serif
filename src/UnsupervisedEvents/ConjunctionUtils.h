#ifndef _CONJUNCTION_UTIL_H_
#define _CONJUNCTION_UTIL_H_

#include <vector>
class Mention;
class MentionSet;

namespace ConjunctionUtils {
	std::vector<const Mention*> conjoinedMentions(const MentionSet* ms,
			const Mention* m);
	size_t numConjoinedMentions(const MentionSet* ms,
			const Mention* m);
};


#endif

