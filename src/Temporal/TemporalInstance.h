#ifndef _TEMPORAL_INSTANCE_
#define _TEMPORAL_INSTANCE_

#include <string>
#include <vector>
#include "Generic/common/Symbol.h"
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(ElfRelation)
BSP_DECLARE(TemporalAttribute)
BSP_DECLARE(TemporalInstance)
class Mention;
class MentionSet;

class TemporalInstance {
public:
	TemporalInstance(const Symbol& docid, unsigned int sent, ElfRelation_ptr relation, 
		TemporalAttribute_ptr temporalAttribute, const std::wstring& previewString,
		const std::wstring& proposalSource = L"");
	const ElfRelation_ptr relation() const;
	const TemporalAttribute_ptr attribute() const;
	const Symbol& docid() const;
	unsigned int sentence() const;
	size_t fingerprint() const;
	const std::wstring& previewString() const;
	const std::wstring& proposalSource() const;

	const Mention* mentionForRole(const std::wstring& role, const MentionSet* ms);
private:
	const ElfRelation_ptr _relation;
	const TemporalAttribute_ptr _temporalAttribute;
	Symbol _docid;
	unsigned int _sent;
	size_t fingerprint();
	std::wstring _previewString;
	std::wstring _proposalSource;
};
typedef std::vector<TemporalInstance_ptr> TemporalInstances;
#endif

