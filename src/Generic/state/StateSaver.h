// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STATE_SAVER_H
#define STATE_SAVER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/OutputStream.h"
#include <fstream>


class StateSaver {
public:
	// Make a single StateSaver object per output file. You may,
	// however, use it to save multiple state trees.
	explicit StateSaver(const char *file_name);
	explicit StateSaver(const wchar_t *file_name);
	StateSaver(const char *file_name, bool binary);
	StateSaver(const wchar_t *file_name, bool binary);
	StateSaver(OutputStream& text_out);  // text output
	~StateSaver();

	// Call this to kick off a state tree. state_description would
	// be something like "After name-finding" I guess.
	void beginStateTree(const wchar_t *state_description);
	// Always call this when you're done. Then you may start
	// another state tree, or destruct the StateSaver.
	void endStateTree();

	// These functions save atomic things:

	// This one saves an unquoted string. It MUST have no spaces,
	// parens, #'s, double-quotes, or whatever other syntactically
	// significant characters there may be.
	void saveWord(const wchar_t *word);
	// This one saves a quoted string, which may contain any
	// characters:
	void saveString(const wchar_t *str);
	void saveSymbol(Symbol symbol);
	void saveInteger(int integer);
	void saveInteger(size_t unsignedInteger) { saveInteger(static_cast<int>(unsignedInteger)); }
	void saveReal(float real);
	void saveUnsigned(unsigned x);
	// This saves the integer ID of the object pointed to by
	// pointer. This only works if that object has been added
	// to the ObjectIDTable.
	void savePointer(const void *pointer);

	// And for non-atomic things, we have lists. Lists may be
	// anonymous or named. Named lists may have object IDs.
	// To make a list, you call beginList(), then save all the
	// stuff inside it (including other lists), and then call
	// endList().

	// This version of beginList() is for anonymous arrays, or
	// any object that you just don't want to name for some reason.
	void beginList() { beginList(0, 0); }
	// This one is for named objects, AKA tuples. If you provide
	// a valid pointer to an object that has been added to
	// ObjectIDTable(), then that object's ID is associated with
	// the list.
	void beginList(const wchar_t *name, const void *pointer = 0);
	void endList();

	static std::pair<int, int> getVersion() { return _version; }
private:
    bool _binary; // This flag tells us whether we are in binary mode or not
	OutputStream *_text_out;
    std::ofstream _bin_out;
	bool _this_object_created_text_out;

	int _depth;
	bool _current_list_empty_so_far;
	bool _current_list_multiline;
	static const std::pair<int, int> _version;

	void advanceToNextLine();
	void initialize(const char* file_name, bool binary);
};

#endif
