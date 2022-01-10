// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// -*- c++ -*-
#ifndef EN___TEMPORAL_PARSER_H__

#include <string>

#ifdef BOOST
#pragma warning(push, 0)
#include <boost/date_time/posix_time/posix_time.hpp>
#pragma warning(pop)
#endif

struct temporal_timex2
{
  std::wstring VAL;
  std::wstring MOD;
  std::wstring ANCHOR_VAL;
  std::wstring ANCHOR_DIR;
  std::wstring SET;

#ifdef BOOST
  boost::posix_time::ptime anchor;
#endif

  void set_anchor (const std::wstring& anchor);

  void clear () {
	VAL = L"";
	MOD = L"";
	ANCHOR_VAL = L"";
	ANCHOR_DIR = L"";
	SET = L"";
  }
};

extern bool normalize_temporal (temporal_timex2& timex2, 
				const wchar_t *expr);
extern bool normalize_temporal (temporal_timex2& timex2, 
				const std::wstring& expr);

#endif
