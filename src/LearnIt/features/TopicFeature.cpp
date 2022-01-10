#pragma warning(disable:4996)
#include "Generic/common/leak_detection.h"

#include <string>
#include <vector>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>
#include "LearnIt/Seed.h"
#include "LearnIt/Target.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/features/TopicFeature.h"
#include "Generic/theories/DocTheory.h"

TopicFeature::TopicFeature(Target_ptr target, 
	const std::wstring& name, const std::wstring& metadata) : Feature()
{
	// The metadata should look like this: topic_id\ttopic_weight
	std::vector<std::wstring> metadata_parts;
	boost::split(metadata_parts, metadata, boost::is_any_of(L"\t"));
	if (metadata_parts.size() == 2) {
		_topic_id = boost::lexical_cast<int>(metadata_parts[0]);
		//double _topic_weight_d = boost::lexical_cast<double>(metadata_parts[1]);
		//float _topic_weight_f = boost::lexical_cast<float>(metadata_parts[1]);
		_topic_weight = metadata_parts[1];
	} else {
		std::stringstream errMsg; 
		errMsg << "Invalid metadata for TopicFeature: '" << 
			metadata << "'" << std::endl;
		throw UnexpectedInputException("TopicFeature::TopicFeature",
			errMsg.str().c_str());
	}
}

bool TopicFeature::matchesMatch(const SlotFillerMap& match,
	const SlotFillersVector& slotFillersVector,
	const DocTheory* doc, int sent_no) const 
{
	//std::vector<std::pair<int, std::wstring>> LDA_topics = doc->getLDATopics();
	//for (unsigned int i=0; i < LDA_topics.size(); i++) {
	//	if (LDA_topics[i].first == _topic_id && 
	//		LDA_topics[i].second.compare(_topic_weight) == 0) {
	//		return true;
	//	}
	//}
	return false;
}
