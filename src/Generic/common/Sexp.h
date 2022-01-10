// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SEXP_H
#define SEXP_H

#include "Generic/common/Symbol.h"
#include "Generic/common/InternalInconsistencyException.h"
#include <string>
#include <sstream>
#include <vector>

/** A class for parsing and storing lisp-style s-expressions.  Sexps are
  * represented in text by parenthesized, whitespace-separated sequences 
  * of character strings, as in "(= 4 (+ 2 2))".  
  *
  * Each Sexp object is either a LIST, an ATOM, or a VOID -- you can check
  * the type using isAtom(), isList(), and isVoid().  LIST Sexps have
  * children, which are accessed via getNthChild() and getNumChildren().
  * ATOM Sexps have values, where are accessed via getValue().
  */
class Sexp {
public:
	/** Read an s-expression from the given stream, and use it to create
	  * a new Sexp object.  This constructor will stop reading from the
	  * stream after it finds a single complete s-expression.  
	  *
	  * @param use_quotes Indicates whether quoted string should be treated
	  * as individual atoms.  In particular, if use_quotes is false, then
	  * quotes are treated like any other character.  If use_quotes is true,
	  * then you may use quote marks (<code>"</code>) to specify atoms that  
	  * include whitespace.  Currently, there is no way to escape a quote 
	  * mark.  However, quote marks that occur in the middle of an atom are
	  * not treated specially.  For example, the quote mark in the string 
	  * <code>he"llo</code> is not treated specially.  The quote marks are
	  * <i>not</i> stripped from the atom.
	  * 
	  * Default is false, but currently (Nov 2013) set to true inside included files.
	  *
	  * @param use_hash_comments Indicates whether hash marks (<code>#</code>)
	  * should be interpreted as marking comments.  If true, then any 
	  * characters starting with a hash mark and ending with the end of the
	  * string are treated as a comment (and not included in the sexp).  One
	  * exception is if the hash mark occurs in a quoted string (and 
	  * use_quotes is true), in which case it is not interpreted as starting
	  * a comment.  
	  *
	  * Default is false, but currently (Nov 2013) set to true inside included files .
	  */
    Sexp(std::wistream& stream, bool use_quotes = false, bool use_hash_comments = false);

	/** Read an s-expression from the given file, and use it to create a new
	  * Sexp object.  See the stream-based constructor for documentation on 
	  * the "use_quotes" and "use_hash_comments" parameters. 
	  *
	  * @param expand_macros: If true, then expand macros immediately after 
	  * reading the s-expression.
	  * Defaults false, but the code currently (Nov 2013) always expands macros.
	  *
	  * @param include_once: If true, then any @INCLUDE macro that specifies 
	  * a file that has already been included will simply be deleted 
	  * without re-including the file).  (Ignored if expand_macros=false)
	  *
	  * @param search_path: The search path for @INCLUDE statements.  The 
	  * directory containing <code>filename</code> will always be appended 
	  * to the end of the search path.
	  *
	  * @param encrypted: If true, then open the sexp file (and any files that
	  * it includes) as encrypted files.
	  */
	Sexp(const char* filename, bool use_quotes = false, bool use_hash_comments=false,
		bool expand_macros=false, bool include_once=true, 
		std::vector<std::string> search_path=std::vector<std::string>(),
		bool encrypted = false);

	/** Search this Sexp for any macros, and expand them.  Currently, the
	  * following macros are defined:
	  * 
	  * <code>(@INCLUDE filename)</code>: 
	  *    Read all s-expressions from the given file, and replace the
	  *    include macro with those s-expressions.  Relative filenames
	  *    are expanded using the "search_path" parameter.  If the
	  *    "include_once" parameter is set, then any @INCLUDE macro 
	  *    that specifies a file that has already been included will 
	  *    simply be deleted (without re-including the file).
	  *
	  * <code>(@SET symbol)</code>: Record the fact that a symbol has a 
	  *    given value, and then remove the macro from the list that
	  *    contains it.
	  *
	  * <code>(@UNSET symbol)</code>: Record the fact that a symbol has a 
	  *    given value, and then remove the macro from the list that
	  *    contains it.
	  *
	  * <code>(@IF symbol body...)</code>: If the given symbol was set
	  *    by a @SET macro, then replace this macro with the Sexps
	  *    that make up its body.  Otherwise, remove this macro from the
	  *    list that contains it.
	  *
	  * <code>(@UNLESS symbol body...)</code>: If the given symbol was
	  *    set by a @SET macro, then remove this macro from the list that
	  *    contains it.  Otherwise, replace this macro with the Sexps
	  *    that make up its body.
	  */
	void expandMacros(std::vector<std::string> *search_path, bool include_once,
		bool use_quotes, bool use_hash_comments, bool encrypted=false);

	/** Return true iff this Sexp is a list. */
	bool isList() const { return _type == LIST; }
	/** Return true iff this Sexp is an atom. */
	bool isAtom() const { return _type == ATOM; }
	/** Return true iff this Sexp is void (i.e., neither a list nor an atom). */
	bool isVoid() const { return _type == VOID1; }

	/** Equivalent to getNthChild(0) */
	Sexp *getFirstChild() const { return _children; }
	/** Equivalent to getNthChild(1) */
	Sexp *getSecondChild() const { return getNthChild(1); }
	/** Equivalent to getNthChild(2) */
	Sexp *getThirdChild() const { return getNthChild(2); }

	/** Return the nth child of this list Sexp, or NULL if this Sexp has 
	  * fewer than n children.  If Sexp is not a list (i.e., is an atom 
	  * or is void), then raise an InternalInconsistencyException. */
	Sexp *getNthChild(int n) const;

	/** Return the number of children that this list Sexp has.  If this 
	  * Sexp is not a list, then raise an InternalInconsistencyException. */
	int getNumChildren() const;

	/** Return the Symbol value of this atom Sexp.  If this Sexp is not
	  * an atom, then raise an InternalInconsistencyException. */
	Symbol getValue() const;

	/** Destructor */
	~Sexp();
	
	/** Copy constructor */
	Sexp(const Sexp &s);

	/** Assignment operator */
	Sexp& operator=(const Sexp &s);

	/** Swap the state of this Sexp with the state of the given Sexp */
	void swap(Sexp& s);

	// Serialization/debug printing:
	std::wstring to_string() const;
	std::string to_debug_string() const;	
	std::wstring to_token_string() const;

	// This is used exactly once; do we need to expose it?
	Sexp *getNext() const { return _next; }
private:
	typedef enum { LIST, ATOM, VOID1 } SexpType;
	SexpType _type;
	Symbol _value;
    Sexp* _children;
    Sexp* _next;
	
	Sexp(): _value(Symbol()), _next(0), _children(0), _type(VOID1) {}

	// Helper methods for constructors
	void read(std::wistream& stream, bool use_quotes, bool use_hash_comments);
	void read(std::wistream& stream, wchar_t* buffer, bool use_quotes, bool use_hash_comments);

    void getNextValidToken(std::wistream& stream, wchar_t* buffer, bool use_hash_comments);  // Use me - I ignore ALL comments
    void getNextValidTokenRaw(std::wistream& stream, wchar_t* buffer);  // I ignore <!-- --> comments
    void getNextTokenIncludingComments(std::wistream& stream, wchar_t* buffer);  // I don't ignore any comments

	// Helper methods for expandMacros
	void expandMacrosHelper(std::vector<std::string> *search_path, 
							bool include_once, bool use_quotes, 
							bool use_hash_comments, bool encrypted,
							std::set<Symbol> &defined_symbols,
							std::set<Symbol> &files_to_skip,
							int depth);
	void removeChild(Sexp** ptr_to_child);
};

#endif

