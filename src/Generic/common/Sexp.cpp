// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/InputUtil.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8Token.h"  // We use this for its UTF8_TOKEN_BUFFER_SIZE

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

//#define DEBUG_SEXP_MACROS

namespace { // Private (local) constants & Helper functions
	const wchar_t* COMMENT_START = L"<!--";
	const wchar_t* COMMENT_END = L"-->";
	const wchar_t* LEFT_PAREN = L"(";
	const wchar_t* RIGHT_PAREN = L")";

	Symbol INCLUDE_SYM(L"@INCLUDE");
	Symbol SET_SYM(L"@SET");
	Symbol UNSET_SYM(L"@UNSET");
	Symbol IF_SYM(L"@IF");
	Symbol UNLESS_SYM(L"@UNLESS");

	void reportMacroError(Symbol macro, Sexp *child, std::string details) {
		std::ostringstream err;
		err << "Invalid " << macro.to_debug_string() 
			<< " statement: " << child->to_debug_string()
			<< ": " << details;
		throw UnexpectedInputException("Sexp::handle_includes", err.str().c_str());
	}

	typedef boost::shared_ptr<std::wistream> wistream_ptr;
	wistream_ptr find_include(std::string filename, std::vector<std::string> *search_path, bool encrypted) {
		wistream_ptr result;
		if (filename.empty()){
			std::cerr << "Sexp::find_include was called with empty file name \n";
			throw UnexpectedInputException("Sexp::handle_includes", "empty filename found in find_include");
		}
		// Allow %parameter% expansions inside pattern file etc. @INCLUDE directives
		filename = ParamReader::expand(filename);
		boost::algorithm::replace_all(filename, L"/", SERIF_PATH_SEP);
		boost::algorithm::replace_all(filename, L"\\", SERIF_PATH_SEP);
		std::string abs_file_name = filename;
		//std::cerr << "Sexp::find_include called with raw path " << filename << " and search path length " << search_path->size() << "\n";
		bool no_result = 1;
		if (boost::algorithm::starts_with(filename, SERIF_PATH_SEP) ||
			((filename.size()>1) && filename[1] == L':')) {
				//std::cerr << "  Sexp::find_include using provided absolute path " << filename << "\n";
				no_result = 0;
				result.reset(UTF8InputStream::build(filename.c_str(), encrypted));
		} else {
			// Filename contains a relative path; 
			// make sure the rel2abs code understands this...
			if (!(boost::algorithm::starts_with(filename, "."))){
				filename =  std::string(".")+SERIF_PATH_SEP+filename ;
				//std::cerr << "  Sexp::find_include just added explicit dot to filename " << filename << "\n";
				abs_file_name = filename;
			}
			// search for it.
			BOOST_REVERSE_FOREACH(std::string dir, *search_path) {
				//std::cerr << "    looping through search_path with dir " << dir << "\n";
				std::string raw_composite_path = dir+SERIF_PATH_SEP+filename;
				//std::cerr << "    Sexp::find_include trying raw composite path " << raw_composite_path << "\n";
				// reset it each time since rel2abs alters its first arg...
				abs_file_name = filename;
				InputUtil::rel2AbsPath(abs_file_name, dir.c_str());
				//std::cerr << "    Sexp::find_include trying path " << abs_file_name << "\n";
				if (FILE *maybe_file = fopen(abs_file_name.c_str(), "r")) {
					fclose(maybe_file);
				}else{
					continue;
				}
				result.reset(UTF8InputStream::build(abs_file_name.c_str(), encrypted));
				if (!result->fail()) {
					no_result = 0;
					break;
				}
			}
		}
		if (no_result || (result->fail())){
			std::cerr << "Sexp::Sexp include file not found: " << filename << "\n";
			std::ostringstream err;
			err << "Include file not found: \"" << filename << "\"";
			throw UnexpectedInputException("Sexp::handle_includes", err.str().c_str());
		}else{
			// keep up with possible relative path for included includes
			// Add the directory containing the filename to the collection of base paths. 
			std::string base_path = InputUtil::getBasePath(abs_file_name);
			//std::cerr << "  Sexp::find_include found path " << abs_file_name << "\n";
			// remember path only if it is not already there
			if (find(search_path->begin(), search_path->end(), base_path) == search_path->end()){
				search_path->push_back(base_path);
				//std::cerr << "    Sexp::find_include pushed new base_path " << base_path << " onto search_path\n";
			}else{
				//std::cerr << "    Sexp::find_include used previous base path " << base_path << "\n";
			}
			return result;
		}

	}
}

void Sexp::expandMacros(std::vector<std::string> *search_path, bool include_once,
						bool use_quotes, bool use_hash_comments, bool encrypted) {
	std::set<Symbol> defined_symbols;
	std::vector<std::wstring> macros = ParamReader::getWStringVectorParam("sexp_macros");
	BOOST_FOREACH(std::wstring macro, macros) {
		defined_symbols.insert(Symbol(macro));
	}
	std::set<Symbol> files_to_skip;
	expandMacrosHelper(search_path, include_once, use_quotes, use_hash_comments, encrypted, defined_symbols, files_to_skip, 0);
}

// Returns a pointer to the replacement value, and also modifies in place.
void Sexp::expandMacrosHelper(std::vector<std::string> *search_path, 
							  bool include_once, bool use_quotes, 
							  bool use_hash_comments, bool encrypted,
							  std::set<Symbol> &defined_symbols,
							  std::set<Symbol> &files_to_skip,
							  int depth) {
	static const bool debug = true;
	if (_type != LIST)
		return;

	#ifdef DEBUG_SEXP_MACROS
		for (int i=0; i<depth; ++i) std::cout<<"  ";
		std::cout << "Before macros: " << to_debug_string() << std::endl;
	#endif
	Sexp **ptr_to_child = &_children;
	while (*ptr_to_child) {
		Sexp *child = *ptr_to_child;

		if ((!child->isList()) || (child->getNumChildren() == 0) ||
			(!child->getFirstChild()->isAtom())) {
			// Not a macro -- advance to the next child.
			ptr_to_child = &((*ptr_to_child)->_next);
			continue;
		}

		// Check if it's a macro.
		Symbol firstAtom = child->getFirstChild()->getValue();
		if ((firstAtom == SET_SYM) || (firstAtom == UNSET_SYM)) {
			// Sanity check on syntax
			if ((child->getNumChildren() != 2) || !child->getSecondChild()->isAtom())
				reportMacroError(firstAtom, child, "Needs 2 elements, second must be an atom");
			if (firstAtom == SET_SYM)
				defined_symbols.insert(child->getSecondChild()->getValue());
			else
				defined_symbols.erase(child->getSecondChild()->getValue());
			removeChild(ptr_to_child); 
		} else if ((firstAtom == IF_SYM) || (firstAtom == UNLESS_SYM)) {
			// Sanity check on syntax
			if ((child->getNumChildren() <= 2) || !child->getSecondChild()->isAtom())
				reportMacroError(firstAtom, child, "Needs at least 3 elements, including atomic tag and one or more Sexps");
			// Determine whether to include the body or not
			Symbol sym = child->getSecondChild()->getValue();
			bool sym_defined = (defined_symbols.find(sym) != defined_symbols.end());
			bool include_body = ((sym_defined && (firstAtom==IF_SYM)) ||
			                     (!sym_defined && (firstAtom==UNLESS_SYM)));
			// Splice the body out of the if statement.
			Sexp *body = child->getThirdChild();
			child->getSecondChild()->_next = 0;
			// Remove the macro
			removeChild(ptr_to_child); 
			if (include_body) {
				// Find the tail of the body.
				Sexp *tail = body;
				while (tail->_next) tail = tail->_next;
				// Splice the body in.
				tail->_next = *ptr_to_child;
				*ptr_to_child = body;
				// We'll expand macros in the body in the next pass through the loop.
			} else {
				// Delete the body if we're not including it.
				delete body;
			}
		} else if (firstAtom == INCLUDE_SYM) {
			// Sanity check on syntax
			if ((child->getNumChildren() != 2) || !child->getSecondChild()->isAtom())
				reportMacroError(firstAtom, child, "Needs 2 elements, second an atomic filepath");
			// Extract the filename.
			Symbol filename_sym = child->getSecondChild()->getValue();
			std::string filename_str = UnicodeUtil::toUTF8StdString(filename_sym.to_string());
			if ((filename_str.length()>0) && (filename_str[0]=='"') && (filename_str[filename_str.length()-1]=='"'))
				filename_str = filename_str.substr(1, filename_str.length()-2);
			// Remove the macro
			removeChild(ptr_to_child); 
			// Don't include a file if we've already included it.
			// this code needs to canonicalize the file name to be safer....
			//std::cerr << "@INCLUDE in Sexp wants file " << filename_str << "\n";
			if (files_to_skip.find(filename_sym) == files_to_skip.end()) {
				if (include_once)
					files_to_skip.insert(filename_sym);
				// Find the file to include.
				// used to work -- wistream_ptr stream = find_include(filename_str.c_str(), search_path, encrypted);
				//std::cerr << "Sexp::expand_macro_helper calling find_include:  search_path len " << search_path->size() << ";  file " <<  filename_str << "\n";
				wistream_ptr stream = find_include(filename_str, search_path, encrypted);
				// Read sexpressions and splice them in.
				Sexp **splice_ptr = ptr_to_child;
				while (!stream->eof()) {
					// Read a child.
					//  the use_quotes/use_hash_values should come from the outer call
					Sexp* incl_child = _new Sexp(*stream, true, true);
					if (incl_child->isVoid()) {
						break; // End of file.
					} else {
						// Splice it in.
						incl_child->_next = *splice_ptr;
						*splice_ptr = incl_child;
						// When we splice in the next child, place it after this one.
						splice_ptr = &(incl_child->_next);
					}
				}
				// We'll expand macros in the included sexps in the next pass
				// through the loop.
				// (this means we cannot use a simple stack for the possible base patch for relative names)
			}else{
				//std::cerr << "Skipping repeated include file " << filename_str << " due to 'include once'\n";
			}
		} else {
			// Recurse to children
			child->expandMacrosHelper(search_path, include_once, use_quotes, use_hash_comments, 
				encrypted, defined_symbols, files_to_skip, depth+1);
			// Advance to the next child.
			ptr_to_child = &((*ptr_to_child)->_next);
		}
	}
	#ifdef DEBUG_SEXP_MACROS
		for (int i=0; i<depth; ++i) std::cout<<"  ";
		std::cout << " After macros: " << to_debug_string() << std::endl;
	#endif
}

/** Given a pointer to a Sexp_ptr, update the pointer to point to
  * Sexp_ptr's next child, and delete Sexp_ptr. */
void Sexp::removeChild(Sexp** ptr_to_child) {
	Sexp* child_to_remove = *ptr_to_child;
	*ptr_to_child = child_to_remove->_next;
	child_to_remove->_next = 0;
	delete child_to_remove;
}


// We assume in this method that the constructor has already initialized value, children, and next to 0
// If we read an atom, we return with that atom's value in the buffer
// If we read a list, we return with a right paren in the buffer
void Sexp::read(std::wistream& stream, wchar_t* buffer,
				bool use_quotes, bool use_hash_comments) {
    if (wcscmp(buffer, LEFT_PAREN) != 0) {  // We have an atomic element
		_type = ATOM;
		if (use_quotes && (buffer[0] == L'"')) {
			//std::cout << "Buffer: |" << buffer << "|\n";
			wchar_t wch;
			size_t i = wcslen(buffer);
			if (i == 1 || buffer[i - 1] != L'"') {
				do {
					if (i >= UTF8_TOKEN_BUFFER_SIZE - 3) {
						std::ostringstream ostr;
						std::wstring str_begin(buffer, 10);
						std::wstring str_end(&buffer[i-10], 10);
						ostr << "quoted string beginning with \"" << str_begin << "\""
							 << " and ending with \"" << str_end << "\""
						     << " is too long (" << i << "; max " << UTF8_TOKEN_BUFFER_SIZE - 4 << ")";
						throw UnexpectedInputException("Sexp::read", ostr.str().c_str());
					}
					wch = stream.get();
					if (stream.eof())
						break;
					if (wch == 0x00) 
						throw UnexpectedInputException("UTF8Sexp::read",
							"Unexpected NULL (0x00) character in stream");
					buffer[i++] = wch;
				} while (wch != L'"');
			}
			buffer[i] = L'\0';
		}	
        _value = Symbol(buffer);
	} else {  // We have a list
		_type = LIST;
        getNextValidToken(stream, buffer, use_hash_comments);  // Read the first element in our list
        if (wcscmp(buffer, RIGHT_PAREN) != 0) {  // Make sure we don't have an empty list
            if (stream.eof()) {
                throw UnexpectedInputException("Sexp::read", "unclosed sexp at EOF (2)");
            }
			_children = _new Sexp();
			_children->read(stream, buffer, use_quotes, use_hash_comments);
			Sexp *node = _children;
            getNextValidToken(stream, buffer, use_hash_comments);  // Read the first element after our first child
            while (wcscmp(buffer, RIGHT_PAREN) != 0) {  // Keep reading as long as we don't have a right paren
                if (stream.eof()) {
                    throw UnexpectedInputException("Sexp::read", "unclosed sexp at EOF (3)");			
                }
				node->_next = _new Sexp();  // Set the next in our most recent child
				node->getNext()->read(stream, buffer, use_quotes, use_hash_comments);  // Populate the next in our most recent child
				node = node->getNext();  // Set our current node to be the (new) most recent child
                getNextValidToken(stream, buffer, use_hash_comments);  // Read the first element after our most recent child
			}
		}
	}
}


Sexp::Sexp(const Sexp &s) : _type(s._type), _value(s._value), _children(0), _next(0) {
    if (s._children != 0) {
		_children = _new Sexp(*s._children);
    }
    if (s._next != 0) {
		_next = _new Sexp(*s._next);
    }
}

Sexp::~Sexp() {
	
	delete this->_children;
	// To avoid stack overflow, we make the first sibling in a chain 
	// responsible for iterative deletion of the entire chain.
	
	Sexp * t = this->_next;
	while( t ){
		delete t->_children;
		t->_children = 0;

		Sexp * p = t->_next;
		t->_next = 0;
		delete t;
		
		t = p;
	}
}

void Sexp::swap(Sexp &s) {
	std::swap(this->_type, s._type);
	std::swap(this->_value, s._value);
	std::swap(this->_children, s._children);
	std::swap(this->_next, s._next);
}

Sexp& Sexp::operator=(const Sexp &s) {
	Sexp copy(s);
	this->swap(copy); 
	return *this;
}


Sexp::Sexp(const char *filename, bool use_quotes, bool use_hash_comments,
		   bool expand_macros, bool include_once, std::vector<std::string> search_path,
		   bool encrypted) 
: _value(Symbol()), _children(0), _next(0) 
{
	// Add the directory containing the filename to the collection of base paths.  
	std::string strfile(filename);
	//std::cerr << "Top level call into Sexp::Sexp(file) with file " << strfile <<" and search_path len " <<search_path.size() << "\n";
	std::string base_path = InputUtil::getBasePath(strfile);
	// our base path (and any found in included includeds) only belong inside scope of this call
	//std::cerr << "Sexp::Sexp(file) is adding " << base_path << " to search_paths\n";
	search_path.push_back(base_path);

	// Read the sexp from the file.
	boost::scoped_ptr<UTF8InputStream> instream(UTF8InputStream::build(filename, encrypted));
	read(*instream, use_quotes, use_hash_comments);
	instream->close();

	// Expand any macros it contains (including @INCLUDE statements)
	expandMacros(&search_path, include_once, use_quotes, use_hash_comments, encrypted);

}

Sexp::Sexp(std::wistream& stream, bool use_quotes, bool use_hash_comments) : _value(Symbol()), _children(0), _next(0) {
	read(stream, use_quotes, use_hash_comments);
}

void Sexp::read(std::wistream& stream, bool use_quotes, bool use_hash_comments)
{
	if (stream.eof()) {
		_type = VOID1;
		return;
	}
	wchar_t buffer[UTF8_TOKEN_BUFFER_SIZE]; // This buffer will get reused by all the descendants of this node
    getNextValidToken(stream, buffer, use_hash_comments);
	if (stream.eof()) {
		_type = VOID1;
		return;
	}
	try {
		read(stream, buffer, use_quotes, use_hash_comments);
	} catch (UnrecoverableException uc) {
		// Display our location; then pass the exception on.
		std::cerr << "IN: " << this->to_debug_string() << " -- \n";
		throw;
	}
}

Sexp* Sexp::getNthChild(int n) const { 
	if (!isList())
		throw InternalInconsistencyException("Sexp::getNthChild()", "Sexp is not a list");
	Sexp *node = _children;
	for (int i = 0; i < n; i++) {
		node = node->getNext(); 
		if (node == 0)
			return node;
	}
	return node;
}

int Sexp::getNumChildren() const { 
	if (!isList())
		throw InternalInconsistencyException("Sexp::getNthChild()", "Sexp is not a list");
	Sexp *node = _children;
	int i = 0;
	while (node != 0) {
		node = node->getNext(); 
		i++;
	}
	return i;
}

Symbol Sexp::getValue() const { 
	if (isList()) {
		std::stringstream error;
		error << "ERROR: tried to get atom value of list: " << to_debug_string();
		throw InternalInconsistencyException("Sexp::getValue()", error.str().c_str());
	}
	return _value; 
}
	
std::wstring Sexp::to_string() const {
	if (isAtom())
		return _value.to_string();
	else {
		std::wstring str = L"(";
		Sexp *node = _children;
		while (node != 0) {
			str += node->to_string();
			node = node->getNext();
			if (node != 0)
                str += L" ";
		}
		str += L")";
		return str;
	}
}

std::string Sexp::to_debug_string() const {
	if (isAtom()) {
		const char* value_str = _value.to_debug_string();
		return value_str ? value_str : "NULL";
	} else {
		std::string str = "(";
		Sexp *node = _children;
		while (node != 0) {
			str += node->to_debug_string();
			node = node->getNext();
			if (node != 0)
                str += " ";
		}
		str += ")";
		return str;
	}
}

std::wstring Sexp::to_token_string() const {
	if (isAtom()) {
		return _value.to_string();
	} else {
		std::wstring str = L"";
		Sexp *node = _children;
		while (node != 0) {
			str += node->to_token_string();
			node = node->getNext();
			if (node != 0)
				str += L" ";
		}
		return str;
	}
}


void Sexp::getNextValidToken(std::wistream& stream, wchar_t* buffer, bool use_hash_comments) {
    getNextValidTokenRaw(stream, buffer); // this will remove any <!-- --> comments

	// Read past any single-line comments if necessary
	while (use_hash_comments && (buffer[0] == L'#')) {  
		// reset buffer
		*buffer = L'\0';

		// read to the end of the line
		wchar_t wch = 0x00;
		while (wch != L'\n') { 
			wch = stream.get();
			if (stream.eof()) { return; }
			if (wch == 0x00) 
				throw UnexpectedInputException("UTF8Sexp::read",
					"Unexpected NULL (0x00) character in stream");
		}

		// get the first valid token on the next line
		 getNextValidTokenRaw(stream, buffer);
    }
}

void Sexp::getNextValidTokenRaw(std::wistream& stream, wchar_t* buffer) {
    getNextTokenIncludingComments(stream, buffer);
    while (wcscmp(buffer, COMMENT_START) == 0) {  // Read past any comments if necessary
        getNextTokenIncludingComments(stream, buffer);
        while (wcscmp(buffer, COMMENT_END) != 0) {  // Read tokens until we get to the comment end
            if (stream.eof()) {
                throw UnexpectedInputException("Sexp::read", "unclosed comment at EOF");
            }
            getNextTokenIncludingComments(stream, buffer);
        }
        getNextTokenIncludingComments(stream, buffer);  // Read the first token after the comment end
    }
}

void Sexp::getNextTokenIncludingComments(std::wistream& stream, wchar_t* buffer) {
    wchar_t wch = stream.get();
	if (stream.eof()) {  // Check if we've read an eof
		*buffer = L'\0';
		return;
	}
	if (wch == 0x00) 
		throw UnexpectedInputException("UTF8Sexp::read",
			"Unexpected NULL (0x00) character in stream");
	while (iswspace(wch)) {  // Read past whitespace
		wch = stream.get();
		if (stream.eof()) {
			*buffer = L'\0';
			return;
		}
		if (wch == 0x00) 
			throw UnexpectedInputException("UTF8Sexp::read",
				"Unexpected NULL (0x00) character in stream");
	}
    *buffer++ = wch;
    if ((wch == L'(') || (wch == L')')) {  // Return if we have an opening or closing parenthesis
        *buffer = L'\0';
        return;
    }
    size_t i = 1;	
	wch = stream.get();
    while (!stream.eof() && !iswspace(wch) && wch != L'(' && wch != L')' && stream.good()) {
		//SessionLogger::dbg("wch_0") << wch << " " << static_cast<char>(wch);
        if (i < (UTF8_TOKEN_BUFFER_SIZE - 1)) {
            *buffer++ = wch;
            i++;
			//SessionLogger::dbg("incr_i_0") << i;
        } else {
			if (*buffer != L'\0') {
				*buffer = L'\0';
                std::cerr << "Token too long ("
					<< (int) i << "/" << UTF8_TOKEN_BUFFER_SIZE << "): "
					<< OutputUtil::convertToChar(buffer) << "\n";
			}
        }
		wch = stream.get();
    }
    *buffer = L'\0';
    if (!stream.eof() && (wch == L'(' || wch == L')' || iswspace(wch))){
        stream.putback(wch);
    }
    return;
}


// for fixing stack overflow run-time error
/*
#if defined(_DEBUG)

static Sexp *freeSexp (Sexp *node)
{
   for (Sexp *child (node->getFirstChild()); child != 0; )
     child = freeSexp (child);

   Sexp *next (node->getNext());
   ::operator delete (node);

   return next;
}

void *Sexp::operator new (size_t size)
{
   return ::operator new (size);
}

void Sexp::operator delete (void *p)
{
   Sexp *sexp = static_cast<Sexp *>(p);
   if (sexp != 0)
     {
       do
	{
	  Sexp *next (sexp->getNext());
	  freeSexp (sexp);
	  sexp = next;
	}
       while (sexp != 0);
     }
}

#endif
*/
