#ifndef _PROP_IR_DOC_CONVERTER_H_
#define _PROP_IR_DOC_CONVERTER_H_

#include "LearnItDocConverter.h"
#include "Generic/PropTree/PropNode.h"
#include "Generic/PropTree/DocPropForest.h"
#include <boost/shared_ptr.hpp>

class UTF8OutputStream;
class SentenceTheory;
class Entityset;

class PropIRDocConverter : public LearnItSentenceConverter {
public:
	PropIRDocConverter() : LearnItSentenceConverter() {}
	void processSentence(const DocTheory* dt, int sn, 
		std::wostream& out);
private:
	void printPropsForSentence(
		const PropNode::PropNodes_ptr& sentenceRoots, 
		const SentenceTheory* st, const EntitySet* es, 
		const ValueMentionSet* vms, std::wostream& out);
	void printTriple(const std::wstring& a, const std::wstring& b, 
		const std::wstring& c, std::wostream& out); 
	void printQuadruple(const std::wstring& a, const std::wstring& b, 
		const std::wstring& c, const std::wstring& d, std::wostream& out); 
	void printPropTree(const PropNode& node, const SentenceTheory* st,
						const EntitySet* dt, const ValueMentionSet* vms,
						std::wostream& out);
	void printPropAnnotations(const std::wstring& head, 
		const std::wstring& roleString, const PropNode& kidNode, 
		const EntitySet* es, const ValueMentionSet* vms, std::wostream& out); 

	DocPropForest_ptr& getForestWithCache(const DocTheory* dt);
	std::wstring _cached_forest_docid;
	DocPropForest_ptr _cached_forest;

	typedef std::vector<PropNode::ChildRole> ChildRoleList;
	static ChildRoleList getChildRolesThroughConjunctions(
			const PropNode& node);

	static const std::wstring PROP_MENTION_ANN, PROP_NAME_MENTION_ANN,
		PROP_DESC_MENTION_ANN, PROP_MEL_NAME, PROP_MEL_DESC;

};

#endif
