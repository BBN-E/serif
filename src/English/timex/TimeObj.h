// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef EN_TIME_OBJ_H
#define EN_TIME_OBJ_H

#include <string>

using namespace std;


class TimeUnit;
class TimeAmount;
class TimePoint;

class TimeObj 
{
private:

	TimePoint *_dateAndTime;
	TimePoint *_anchor;
	TimeAmount *_duration;
	wstring _generic;
	wstring _mod;
	wstring _anchor_dir;

	TimeAmount *_granularity;
	TimeAmount *_periodicity;
	bool _set;

public:
	TimeObj();
	~TimeObj();
	TimeObj(const wstring &str);
	TimeObj* clone();
	bool isWeekMode();
	bool isDuration();
	bool isSpecificDate();
	bool isGeneric();
	bool isSet();
	bool isEmpty();
	bool isEmpty(const wchar_t *slot);
	wstring toString();
	wstring valToString();
	wstring modToString();
	void addContext(TimePoint *context);
	void addAnchorContext(TimePoint *context);
	int compareTo(TimeObj *to);
	int compareTo(TimePoint *tp);
	void setMod(const wchar_t *mod);
	void setMod(const wstring &mod);
	void setAnchorDir(const wchar_t *anchor_dir);
	void setAnchorDir(const wstring &anchor_dir);
	void setDuration(const wchar_t *slot, int amt);
	void setDuration(const wchar_t *slot, float amt);
	int getDuration(const wchar_t *slot);
	void setGeneric(const wstring &g);
	void setTime(const wchar_t *slot, TimeUnit *val);
	void setTime(const wchar_t *slot, const wchar_t *val);
	void setTime(const wchar_t *slot, const wstring& val);
	TimeUnit* getTime(const wchar_t *slot);
	void normalizeTime();
	void normalizeAnchor();
	void setRelativeTime(TimePoint *context, const wstring &verbTense, const wstring &mod, const wstring &unit, int num);
	void setRelativeTime(TimePoint *context, const wstring &verbTense, const wstring &mod, const wstring &unit, float amt);
	void setRelativeAnchorTime(TimePoint *context, const wstring &verbTense, const wstring &mod, const wstring &unit, int num);
	void setGranularity(const wchar_t *slot, int amt);
	void setPeriodicity(const wchar_t *slot, int amt);
	void setSet();
	void setSet(const wstring &unit);
	void setSet(const wchar_t *unit);

	TimeAmount* getDuration();
	TimePoint* getDateAndTime();
	TimePoint* getAnchor();
	wstring getGeneric();
	wstring getMod();
	wstring getAnchorDir();
	TimeAmount* getGranularity();
	TimeAmount* getPeriodicity();
};


#endif

