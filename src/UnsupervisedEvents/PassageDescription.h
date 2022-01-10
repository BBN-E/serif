#ifndef _PASSAGE_DESCRIPTION_H_
#define _PASSAGE_DESCRIPTION_H_
#include <vector>
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(PassageDescription)
class DocTheory;
typedef std::map<std::wstring, PassageDescription_ptr> PassageDescriptions;
typedef boost::shared_ptr<PassageDescriptions> PassageDescriptions_ptr;

class PassageDescription {
public:
	PassageDescription(const std::wstring& docid, unsigned int start_sentence,
			unsigned int end_sentence) : _docid(docid),
	_start_sentence(start_sentence), _end_sentence(end_sentence), _passage() {}

	const std::wstring& docid() const { return _docid;}
	unsigned int startSentence() const { return _start_sentence; }
	unsigned int endSentence() const { return _end_sentence; }
	const std::wstring& passage() const { return _passage; }
	void attachDocTheory(const DocTheory* dt);
	
	static PassageDescriptions_ptr loadPassageDescriptions(
			const std::string& filename);

private:
	std::wstring _docid;
	unsigned int _start_sentence;
	unsigned int _end_sentence;
	std::wstring _passage;
};

#endif

