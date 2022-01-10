// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_TAG_SET_H
#define D_T_TAG_SET_H

#include "Generic/common/Symbol.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/InternalInconsistencyException.h"

#include <algorithm>
#include <vector>

class DTTagSet {
public:
	DTTagSet(const char *tag_set_file, bool generate_st_co_suffix,
			 bool add_start_end_tags, bool interleave_st_co_tags = false);
	~DTTagSet();

	/// Get number of distinct tags, including nested and start & end tags if they are defined
	int getNTags() const { return static_cast<int>(_tagArray.size()); }

	/// Get number of tags, including nested tags (if using them) and -ST and -CO tags
	/// but not including START & END
	int getNRegularTags() const;

	/// Look up tag Symbol from index
	const Symbol &getTagSymbol(size_t index) const {
		if (index >= 0 && index < _tagArray.size()) {
			return _tagArray[index]->_tag;
		} else {
			throw InternalInconsistencyException::arrayIndexException(
				"DTTagSet::getTagSymbol()", _tagArray.size(), index);
		}
	}

	/// Look up tag index from Symbol; returns -1 if not found
	int getTagIndex(const Symbol &tagSym) const {
		TagMap::const_iterator iter = _tagMap.find(tagSym);
		if (iter == _tagMap.end()) return -1;
		return (*iter).second;
	}

	// Look up outer CO tag index for a nested tag (for X=Y-ST, this returns Y-CO, for X-ST, this returns X-CO)
	int getOuterCOIndex(int index) const;

	// Look up outer start tag index for a nested tag (for X=Y-ST, this returns Y-ST, for X-ST, this returns X-ST)
	int getOuterSTIndex(int index) const;

	/// Look up reduced tag symbol
	/// (for nested tags of the form X=Y-ST, this returns X; for non-nested tags of the form X-ST
	/// it also returns X)
	const Symbol &getReducedTagSymbol(int index) const {
		if (index >= 0 && (size_t)index < _tagArray.size()) {
			return _tagArray[index]->_reducedTag;
		} else {
			throw InternalInconsistencyException::arrayIndexException(
				"DTTagSet::getReducedTagSymbol()", _tagArray.size(), index);
		}
	}

	/// Look up semi reduced tag symbol 
	/// (for nested tags of the form X=Y-ST, this returns X=Y; for non-nested tags it's 
	/// identical to getReducedTagSymbol)
	const Symbol &getSemiReducedTagSymbol(int index) const;

	// The successor and predecessor tag arrays are only available 
	// if generate_st_co_suffix was turned on.
	// successor tags are for tag x, all tags that may come after it
	// predecessor tags are for tag x, all tags that may precede it
	// in the java implementation of this code, these were learned from 
	// the training.
	// in this implementation, CO tags have predecessor tag of -ST or -CO tag
	// of same type, ST tags have predecessor tag of all tags.  
	std::set<int> getSuccessorTags(int index) const;
	std::set<int> getPredecessorTags(int index) const;

	// The java implementation of the perceptron 
	// learned transition rules from the training data.  These methods
	// allow implementations of the discTagger to do the same

	// set the number of SucessorTags for every tag to 0
	void resetSuccessorTags();
	
	// set the number of PredecessorTags for every tag to 0
	void resetPredecessorTags();

	// add transition to successor and predecessor tags
	void addTransition(const Symbol &prevTag, const Symbol &nextTag);
	void addTransition(int prevIndex, int nextIndex);

	// dynamically update tag set
	void addTag(const Symbol &reducedTag);

	int getNoneTagIndex() const { return 0; }

	/// Returns -1 if there is no start tag
	int getStartTagIndex() const { return _start_tag_index; }
	
	/// Returns -1 if there is no end tag
	int getEndTagIndex() const { return _end_tag_index; }

	/// Returns -1 if there is no link tag
	int getLinkTagIndex() const;

	const Symbol &getNoneTag() const { return _noneTag; }
	const Symbol &getNoneSTTag() const { return _noneSTTag; }
	const Symbol &getNoneCOTag() const { return _noneCOTag; }
	const Symbol &getStartTag() const { return _startTag; }
	const Symbol &getEndTag() const { return _endTag; }

	bool isSTTag(int index) const;
	bool isCOTag(int index) const; 
	bool isNestedTag(int index) const;
	bool isNoneTag(int index) const;
	
	void dump() const;
	
	void writeTransitions(const char* file) const;
	void readTransitions(const char* file);

private:

	class DTTag {
	public:
		DTTag(const Symbol &tag, const Symbol &reducedTag, const Symbol &semiReducedTag) : _isST(false), _isCO(false), _isNested(false), _coTag(-1) {
			_tag = tag;
			_reducedTag = reducedTag;
			_semiReducedTag = semiReducedTag;
		}

		~DTTag() { }

		void addPred(int index) {
			_pred.insert(index);
		}

		const std::set<int> getPredecessorTags() const {
			return _pred;
		}

		void resetPredecessorTags() {
			_pred.clear();
		}

		void addSucc(int index) {
			_succ.insert(index);
		}

		const std::set<int> getSuccessorTags() const {
			return _succ;
		}

		void resetSuccessorTags() {
			_succ.clear();
		}

		void dump() const {
			printf("%-10s%-10s%-10s\t", 
				_tag.to_debug_string(),
				_reducedTag.to_debug_string(),
				_semiReducedTag.to_debug_string());
			printf("st:%d co:%d nested:%d cotag:%-3d\n",
				_isST,
				_isCO,
				_isNested,
				_coTag);
		}

		Symbol _tag;
		Symbol _reducedTag;
		Symbol _semiReducedTag;
		bool _isST;
		bool _isCO;
		bool _isNested;
		int _coTag;  // the -ST tag if this is a -CO tag, and vice versa, or -1 if no -ST -CO tags
					 // the cotag of -STST is the corresponding -CO tag, just as for any other -ST

		std::set<int> _succ;
		std::set<int> _pred;
	};

	typedef Symbol::HashMap<int> TagMap;

	TagMap _tagMap;  // gets tag index from its Symbol

	bool _use_nested_names;

	std::vector<Symbol> _reducedTags;
	std::vector<DTTag*> _tagArray;

	Symbol _noneTag;
	Symbol _noneSTTag;
	Symbol _noneCOTag;
	Symbol _startTag;
	Symbol _endTag;
	Symbol _linkTag1;
	Symbol _linkTag2;

	int _start_tag_index;
	int _end_tag_index;
	bool _generate_st_co_suffix;
	bool _add_start_end_tags;
	bool _interleave_st_co_tags;

	int indexFromSymbol(const Symbol &tag) {
		TagMap::iterator iter = _tagMap.find(tag);
		if (iter == _tagMap.end())
			return -1;
		else
			return (*iter).second;
	}

	// convenience functions to make the constructor a bit more readable
	int calcTotalNumTags(int n_reduced_tags) const;
	void addNoneTags() { addTag(_noneTag, _noneTag); }
	void addStartEndTags();
	void addTag(const Symbol &reducedTag, const Symbol &semiReduced, bool makeSTST = false);
	void setupSuccPred();
};

#endif
