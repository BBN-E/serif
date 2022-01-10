#include <boost/shared_ptr.hpp>
#include "Generic/theories/Mention.h"
#include "Generic/patterns/multilingual/AlignedDocSet.h"
class Entity;
class DocTheory;

typedef std::map<MentionUID, Entity*> MentionToEntityMap;
typedef boost::shared_ptr<MentionToEntityMap> MentionToEntityMap_ptr;

namespace MentionToEntityMapper {
	// Gets a mapping from mention IDs to their entity.
	MentionToEntityMap_ptr getMentionToEntityMap(const DocTheory* doc_theory);
	MentionToEntityMap_ptr getMentionToEntityMap(const AlignedDocSet_ptr& doc_theory);
}

