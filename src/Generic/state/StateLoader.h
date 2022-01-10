// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STATE_LOADER_H
#define STATE_LOADER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/ByteBuffer.h"
#include <memory>
#include <vector>

#define MAX_SERIF_TOKEN_LENGTH 204799

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class Parse;
class SynNode;

class SERIF_EXPORTED StateLoader {
public:
	// Make a single StateLoader object per input file. You may,
	// however, use it to load multiple state trees.
	// If the file is not found, you'll get an
	// UnexpectedInputException.
	explicit StateLoader(const char* file_name);
	StateLoader(const char* file_name, bool binary);
	explicit StateLoader(const wchar_t* file_name);
	StateLoader(const wchar_t* file_name, bool binary);
	StateLoader(std::wistream& in);  // text input
	StateLoader(std::istream& in);  // binary input
	~StateLoader();

	// Try reading byte buffer as a byte buffer, not as a stream.
	StateLoader(ByteBuffer* byteBuffer);
	ByteBuffer* _stateByteBuffer;
#define STATE_LOADER_USE_BYTE_BUFFER (_stateByteBuffer != NULL)

	// Check if state-file token-integer compression was enabled
	bool useCompressedState() { return _use_compressed_state; }
	static unsigned int IntegerCompressionStart; //Initial token integer replacement
	static unsigned int IntegerCompressionTokenCount; //How many of the most common tokens to replace

	// This order has to match that set in DocConvert
	enum IntegerCompressionOffset {
		EntitySetOffset,
        SynNodeOffset,
        TokenOffset,
        PartOfSpeechOffset,
        ArgumentOffset,
        PropositionArgsOffset,
        PropositionOffset,
        MentionOffset,
	};

	// Call this to kick off a state tree. state_description would
	// be something like "After name-finding" I guess.
	void beginStateTree(const wchar_t *state_description);
	// Always call this when you're done. Then you may start
	// another state tree, or destruct the StateSaver.
	void endStateTree();

	// The rest of these functions are for loading pieces of
	// information saved by a StateSaver. If it is not able to read
	// in the type of information you request, or if calls to
	// beginList() or endList() occur anywhere other than list
	// boundaries, then an UnexpectedInputException is thrown.
	//
	// They all put whatever string is read in into
	// token_string below. The ones that return strings always
	// return a pointer to token_string. All of these functions,
	// including beginList() and endList(), will modify
	// token_string, so you must use all string values before you
	// call any of those functions again.

	// This one loads a string saved with
	// StateSaver::saveString() or StateSaver::saveWord() and
	// returns a pointer to token_string;
	wchar_t *loadString();
	// loadSpecificString() ensures that the next token is the
	// given word. If it is not, it throws an
	// UnexpectedInputException using the provided message.
	void loadSpecificString(const wchar_t *str, const char *message);

	// And so on...
	Symbol loadSymbol();
	int loadInteger();
	unsigned loadUnsigned();
	float loadReal();

	// This gives you the object ID of a pointer saved with
	// StateSaver::savePointer(). Use it later to resolve to a real
	// pointer.
	void *loadPointer();

	// beginList() and endList() are analogous to their
	// counterparts in StateSaver. If you specify a name, the
	// list must have the given name (or an UnexpectedInputException
	// will be thrown). If the list has an object ID, that ID
	// will be returned by beginList() (otherwise, -1 is returned).
	int beginList(const wchar_t *name = 0);
	void endList();

	ObjectPointerTable & getObjectPointerTable() { return opt; }

	// Return the version used by the state tree that is currently being
	// read by this state loader.  If no state tree is currently being
	// read, then raise an exception.
	std::pair<int,int> getVersion();

	// These methods are used when loading an old state file -- state
	// files formerly contained the SynNode for a MentionSet, but they
	// now contain the Parse.  So we need a way to map from a SynNode
	// to the corresponding Parse object.  We do this by registering
	// each parse as we load it, and then using getParseByRoot to look
	// up the correct parse for a given SynNode.  The registry must
	// be cleared before loading each new state tree to prevent 
	// accessing stale pointers.
	void registerParse(const Parse* parse);
	void clearParseRegistry();
	const Parse* getParseByRoot(const SynNode* root) const;

private:
    bool _binary; // This flag tells us whether we are in binary mode or not
    bool _this_object_opened_the_input_stream; // Used to know whether we should close the stream when we are done or not
	bool _use_compressed_state; //Does the state file contain integer-mapped tokens

	std::wistream* _text_in;                    // Test stream
    std::istream* _bin_in;                     // Binary stream	

	/** The version string for the state file that we're currently loading.  This
	  * gets set by beginStateTree(), and cleared by endStateTree(). */
	std::pair<int,int> _version;
	
	ObjectPointerTable opt;
	// This is the token buffer. Feel free to access it directly.
	// beginList(), endList(), and all of the loadSuchAndSuch()
	// functions modify this string.
	wchar_t token_string[MAX_SERIF_TOKEN_LENGTH+1];  // Only text-mode processing should use this to avoid confusion
	void readNextToken();
	wchar_t skipWhiteSpace();
	void ensureToken(const wchar_t *str, const char *message = 0);
	void initialize(const char* file_name, bool binary);

	wchar_t getCharSafely() {
		wchar_t c = _text_in->get();
		if (c == L'\0' && _text_in->eof())
			throw UnexpectedInputException(
				"StateLoader::getCharSafely()",
				"Unexpected end of file");
		return c;
	}
	void _parse_version(const std::wstring &version_str);

	std::vector<const Parse*> _parses;
	int _lineno;
};

#endif
