// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef METADATA_H
#define METADATA_H

#include <cstddef>
#include <iostream>
#include "Generic/common/GrowableArray.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Theory.h"
#include "Generic/theories/Span.h"
#include "Generic/theories/SpanCreator.h"

class Metadata : public Theory
{
private:
	class SpanComparator {
	public:
        SpanComparator() {}
		virtual ~SpanComparator() {}
		virtual EDTOffset selectOffset(Span *span) const = 0;
		virtual bool lessThan(EDTOffset offset1, EDTOffset offset2) const = 0;
		bool lessThan(Span *span1, Span *span2) const {
			return lessThan(selectOffset(span1), selectOffset(span2));
		}
		bool lessThan(Span *span1, EDTOffset offset2) const {
			return lessThan(selectOffset(span1), offset2);
		}
	};

	class StartComparator : public SpanComparator {
	public:
        StartComparator() {}
		virtual EDTOffset selectOffset(Span *span) const {
			return span->getStartOffset();
		}
		virtual bool lessThan(EDTOffset offset1, EDTOffset offset2) const {
			return (offset1 <= offset2);
		}
	};

	class EndComparator : public SpanComparator {
	public:
        EndComparator() {}
		virtual EDTOffset selectOffset(Span *span) const {
			return span->getEndOffset();
		}
		virtual bool lessThan(EDTOffset offset1, EDTOffset offset2) const {
			return (offset1 < offset2);
		}
	};

	struct HashKey {
        size_t operator()(const EDTOffset key) const {
            return key.value();
        }
    };
	struct EqualKey {
        bool operator()(const EDTOffset key1, const EDTOffset key2) const {
            return (key1 == key2);
        }
    };
	struct SymbolHashKey {
        size_t operator()(const Symbol s) const {
			return s.hash_code();
        }
    };

    struct SymbolEqualKey {
        bool operator()(const Symbol s1, const Symbol s2) const {
			return s1 == s2;
		}
    };

	typedef serif::hash_map<Symbol, SpanCreator*, SymbolHashKey, SymbolEqualKey> SpanCreatorTable;

	static const StartComparator startCmp;
	static const EndComparator endCmp;
	static const float targetLoadingFactor;

public:
	typedef GrowableArray<Span *> SpanList;

	/// A filter class for returning selected spans at a document offset.
	class SpanFilter {
	public:
		SpanFilter() {}
		virtual ~SpanFilter() {}
		virtual bool keep(Span *span) const = 0;
	};

	/// A filter class for returning all spans at a document offset.
	class KeepAllFilter : public SpanFilter {
	public:
		KeepAllFilter() {}
		virtual bool keep(Span *span) const { return true; }
	};

	/// A filter class for returning all spans of a given span type at a document offset.
	class SpanTypeFilter : public SpanFilter {
	public:
		SpanTypeFilter(Symbol type) { _type = type; }
		virtual bool keep(Span *span) const { return span->getSpanType() == _type; }
	private:
		Symbol _type;
	};

private:
	// TODO: if this becomes a bottleneck, make it a balanced tree
	//       (red-black or AVL); we're tending to get pretty unbalanced
	//       trees due to the order in which the trees are being created
	class SpanTree {
	public:
		SpanTree(Span *span, const SpanComparator &cmp, bool view = true)
			: _span(span), _cmp(cmp), _view(view), _left(NULL), _right(NULL) {}
		~SpanTree();

		int size() const;
		bool contains(Span *span) const;
		void insert(Span *span);
		SpanTree *headTree(EDTOffset maxOffset) const;
		SpanTree *tailTree(EDTOffset minOffset) const;
		void find(EDTOffset offset, const SpanFilter *filter, SpanList *result) const;
		void find(const SpanFilter *filter, SpanList *result) const;
		void intersection(SpanTree *other, const SpanFilter *filter, SpanList *result) const;
		void dump(std::ostream &out) const;

	private:
		SpanTree(Span *span, const SpanComparator &cmp, SpanTree *left, SpanTree *right, bool view = true)
			: _span(span), _cmp(cmp), _left(left), _right(right), _view(view) {}

		bool _view;
		const SpanComparator &_cmp;
		Span *_span;
		SpanTree *_left;
		SpanTree *_right;
	};

	int creator_count;
	int span_count;
	SpanCreatorTable* creatorTable;
	SpanTree* spanStartTree;
	SpanTree* spanEndTree;

	Span * getSmallestSpan(const SpanList *list) const;
	void insert(Span *span);

public:
	Metadata();
	~Metadata();
	
	Span * getStartingSpan(EDTOffset start_offset, const Symbol type) const;
	Span * getStartingSpan(EDTOffset start_offset) const;
	SpanList * getStartingSpans(EDTOffset start_offset) const;
	SpanList * getStartingSpans(EDTOffset start_offset, const SpanFilter *filter) const;

	Span * getEndingSpan(EDTOffset end_offset, const Symbol type) const;
	Span * getEndingSpan(EDTOffset end_offset) const;
	SpanList * getEndingSpans(EDTOffset end_offset) const;
	SpanList * getEndingSpans(EDTOffset end_offset, const SpanFilter *filter) const;

	SpanList * getSpans() const;
	SpanList * getSpans(const SpanFilter *filter) const;

	Span * getCoveringSpan(EDTOffset offset, const Symbol type) const;
	Span * getCoveringSpan(EDTOffset offset) const;
	SpanList * getCoveringSpans(EDTOffset offset) const;
	SpanList * getCoveringSpans(EDTOffset offset, const SpanFilter *filter) const;

	SpanList * getContainedSpans(EDTOffset start_offset, EDTOffset end_offset) const;
	SpanList * getContainedSpans(EDTOffset start_offset, EDTOffset end_offset, const SpanFilter *filter) const;

	void dump(std::ostream &out) const;

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit Metadata(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;
	virtual bool hasXMLId() const { return false; }

	void initialize();
	Span* newSpan(Symbol type, EDTOffset start_offset, EDTOffset end_offset, const void *param = NULL);
	void addSpanCreator(Symbol type, SpanCreator *creator);
	SpanCreator* lookupSpanCreator(Symbol type);
	int get_creator_count() const;
	int get_span_count() const;
};

#endif
