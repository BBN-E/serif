// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Spanish/trainers/es_HeadFinder.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/trainers/Production.h"
#include "Spanish/parse/es_STags.h"
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

// XX is LHS a prefix??
namespace {
	Symbol RIGHTMOST_SYM(L"rightmost");
	Symbol LEFTMOST_SYM(L"leftmost");
	Symbol ONLY_ONE_SYM(L"only_one");
	Symbol EMPTY_SYM(L"");
	// Note: we currently ignore the (|.co$) suffix that is used to
	// mark coordination.
	boost::wregex HEAD_RULE_RE
	(L"([a-zA-Z0-9\\.\\*]+) *= *(rightmost|leftmost|only_one)? *([<{]?)([a-zA-Z0-9\\.\\*]+)(>?)(\\(\\|\\.co\\)\\$)?");
	std::set<std::wstring> defaultMatches;
}

class AncoraHeadRule {
	enum Operator {RIGHT_MOST, LEFT_MOST, ONLY_ONE};
public:
	typedef boost::shared_ptr<AncoraHeadRule> AncoraHeadRule_ptr;

	AncoraHeadRule(const std::wstring &parentTag, Operator op, const std::wstring &childTag, bool childIsPos, bool isPrefix, std::wstring ruleString) 
		: _parentTag(parentTag), _operator(op), _childTag(childTag), 
		  _childIsPos(childIsPos), _isPrefix(isPrefix), _ruleString(ruleString) {}
			
	int match(const Production& production) const 
	{
		std::wstring lhs(production.left.to_string());
		boost::to_lower(lhs);
		if (!boost::starts_with(lhs, _parentTag))
			return -1;
		//if (production.left != _parentTag) return -1;

		switch(_operator) {
		case ONLY_ONE:
			return matchOnlyOneChild(production);
		case LEFT_MOST:
			return matchEdgeChild(production, 0, production.number_of_right_side_elements, 1);
		case RIGHT_MOST:
			return matchEdgeChild(production, production.number_of_right_side_elements-1, -1, -1);
		default:
			throw InternalInconsistencyException("AncoraHeadRule::match",
							 "Unknown rule operator");
		}
	}

	static std::vector<boost::shared_ptr<AncoraHeadRule> > load(const char* filename) {
		std::vector<boost::shared_ptr<AncoraHeadRule> > rules;
		boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(filename));
		UTF8InputStream& stream(*stream_scoped_ptr);
		if (stream.fail())
			throw UnexpectedInputException("AncoraHeadRule::load()", "Problem opening ", filename);
		boost::wsmatch match;
		while (!(stream.eof() || stream.fail())) {
			std::wstring line;
			std::getline(stream, line);
			line = line.substr(0, line.find_first_of(L'#')); // Remove comments.
			boost::trim(line);
			if (!line.empty()) {
				if (!boost::regex_match(line, match, HEAD_RULE_RE))
					throw UnexpectedInputException("AncoraHeadRule::load()", 
												   "Badly formatted line");
				std::wstring parentTag(match.str(1));
				std::wstring childTag(match.str(4));
				bool childIsPos = (match.str(3) == L"<");
				// Do case-insensitive comparisons.
				boost::to_lower(parentTag);
				boost::to_lower(childTag);
				// Check if we're matching a full tag or a prefix.
				bool isPrefix = ((match.str(3) == L"{") || 
								 (childIsPos && match.str(5) != L">"));
				// Get the operator.
				Operator oper;
				Symbol op(match.str(2).c_str());
				if (op == RIGHTMOST_SYM)
					oper = RIGHT_MOST;
				else if (op == LEFTMOST_SYM)
					oper = LEFT_MOST;
				else if (op == ONLY_ONE_SYM)
					oper = ONLY_ONE;
				else if (op == EMPTY_SYM)
					oper = RIGHT_MOST; // Is this the right default?? [XX]
				else
					throw UnexpectedInputException("AncoraHeadRule::load()", 
												   "Unexpected operator", op.to_debug_string());
				rules.push_back(boost::make_shared<AncoraHeadRule>
								(parentTag, oper, childTag, childIsPos, isPrefix, line));
			}
		}
		stream.close();
		std::cerr << "Loaded " << rules.size() << " head rules" << std::endl;
		return rules;
	}

	const std::wstring& ruleString() const { return _ruleString; }

private:
	int matchEdgeChild(const Production& production, 
					   int startIndex, int endIndex, int delta) const
	{
		for (int i=startIndex; i!=endIndex; i+=delta) {
			if (childTagMatches(production.right[i]))
				return i;
		}
		return -1;
	}

	int matchOnlyOneChild(const Production& production) const 
	{
		int result = -1;
		for (int i=0; i<production.number_of_right_side_elements; ++i) {
			if (childTagMatches(production.right[i])) {
				if (result == -1)
					result = i;
				else
					return -1; // More than one found.
			}
		}
		return result;
	}

	bool childTagMatches(Symbol childTag) const {
		std::wstring rhs(childTag.to_string());
		boost::to_lower(rhs);
		if (_isPrefix)
			return boost::starts_with(rhs, _childTag);
		else
			return (_childTag == rhs);
	}

private:
	std::wstring _parentTag;
	std::wstring _childTag;
	Operator _operator;
	bool _childIsPos;
	bool _isPrefix;
	std::wstring _ruleString; // for debug display
};

SpanishHeadFinder::SpanishHeadFinder() {
	std::string filename = ParamReader::getRequiredParam("spanish_head_table");
	_headRules = AncoraHeadRule::load(filename.c_str());
	_verbose = ParamReader::isParamTrue("spanish_head_table_verbose");
}

int SpanishHeadFinder::get_head_index()
{
	int result = -1;
	BOOST_FOREACH(AncoraHeadRule_ptr rule, _headRules) {
		result = rule->match(production);
		if (result != -1) {
			if (_verbose)
				std::wcerr << L"Matched [" << rule->ruleString()
						  << L"] at " << result << std::endl;
			return result;
		}
	}
	if (_verbose) {
		std::wcerr << L"No rule matched; returning default (leftmost): "
				   << production.left << "->";
		for (int i=0; i<production.number_of_right_side_elements; ++i)
			std::wcerr << production.right[i] << " ";
		std::wcerr << std::endl;
	}
	if (production.number_of_right_side_elements>1) {
		std::wostringstream context;
		context << production.left << "->";
		for (int i=0; i<production.number_of_right_side_elements; ++i)
			context << production.right[i] << " ";
		defaultMatches.insert(context.str());
	}
	// Default -- rightmost or leftmost??
	return 0;
}

SpanishHeadFinder::~SpanishHeadFinder() {
	BOOST_FOREACH(const std::wstring& s, defaultMatches)
		std::wcout << L"Default match: " << s << std::endl;
}


