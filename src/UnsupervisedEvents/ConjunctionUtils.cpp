#include "Generic/common/leak_detection.h"

#include "ConjunctionUtils.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"

std::vector<const Mention*> ConjunctionUtils::conjoinedMentions(
		const MentionSet* ms, const Mention* m)
{
	std::vector<const Mention*> ret;
	for (int i=0; i<ms->getNMentions(); ++i) {
		const Mention* other = ms->getMention(i);

		if (other->getMentionType() == Mention::LIST) {
			const Mention* kid = other->getChild();
			bool contains_target = false;

			while (kid) {
				if (kid == m) {
					contains_target = true;
					break;
				}
				kid = kid->getNext();
			}

			if (contains_target) {
				kid = other->getChild();
				while (kid) { 
					if (kid != m) {
						ret.push_back(kid);
					}
					kid = kid->getNext();
				}
				break;
			}
		}
	}

	return ret;
}

size_t ConjunctionUtils::numConjoinedMentions(const MentionSet* ms,
		const Mention* m)
{
	size_t ret = 0;
	for (int i=0; i<ms->getNMentions(); ++i) {
		const Mention* other = ms->getMention(i);
		ret = 0;

		if (other->getMentionType() == Mention::LIST) {
			const Mention* kid = other->getChild();
			bool contains_target = false;

			while (kid) {
				if (kid == m) {
					contains_target = true;
				}
				++ret;
			}

			if (contains_target) {
				break;
			} else {
				ret = 0;
			}
		}
	}

	return ret;
}

