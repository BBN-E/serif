#ifndef _BESTNAME_H_
#define _BESTNAME_H_

#include <string>
#include "Generic/theories/Mention.h"

class SentenceTheory;
class DocTheory;

class BestName {
public:
	BestName() : _bestName(L"NO_NAME"), _confidence(0.0), 
		_mentionType(Mention::NONE), _startOffset(-1), _endOffset(-1) {}
	BestName(const std::wstring& bestName, double confidence, 
		Mention::Type mentionType, EDTOffset startOffset, EDTOffset endOffset) 
		: _bestName(bestName), _confidence(confidence), _mentionType(mentionType), 
		_startOffset(startOffset), _endOffset(endOffset) {}
	
	/** Creates a copy of a bestname with the confidence multiplied by
		some penalty */
	BestName(const BestName& bn, double penalty) :
		_bestName(bn._bestName), _confidence(penalty*bn._confidence),
			_mentionType(bn._mentionType), _startOffset(bn._startOffset),
			_endOffset(bn._endOffset) {}

	const std::wstring& bestName() const {return _bestName;}
	const double confidence() const {return _confidence;}
	const Mention::Type mentionType() const {return _mentionType;}
	const EDTOffset startOffset() const {return _startOffset;}
	const EDTOffset endOffset() const {return _endOffset;}
	void setConfidence(double confidence) {_confidence=confidence;}

	static BestName calcBestNameForMention(const Mention* mention, 
		const SentenceTheory* st, const DocTheory* dt, bool fullText=false,
		bool prefer_better_coref_name=false);
private:
	std::wstring _bestName;
	double _confidence;
	Mention::Type _mentionType;
	EDTOffset _startOffset;
	EDTOffset _endOffset;

	static BestName _findBestMentionName(const Mention* mention, 
		const SentenceTheory* st, bool fullText, 
		bool prefer_better_coref_name=false);
	static BestName _findBestEntityName(const Mention* mention, 
		const SentenceTheory* st, const DocTheory* dt, 
		bool prefer_better_coref_name=false);

};
#endif

