
template<class T>
class FeatureCollection {
public:
	FeatureCollectionIterator<SentenceMatchableFeature> applicableFeature(
		const SlotFillersVector& slotFillersVector, DocumentInfo& docInfo,
		int sent_no, const MentionToEntityMap& mentionToEntityMap);

};
