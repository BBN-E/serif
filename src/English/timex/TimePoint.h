// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef EN_TIME_POINT_H
#define EN_TIME_POINT_H

#include <string>

class TimePoint;
class TimeUnit;

using namespace std;

class TimePoint {

private:
	TimeUnit *vYear;
	TimeUnit *vMonth;
	TimeUnit *vDay;
	TimeUnit *vHour;
	TimeUnit *vMin;
	TimeUnit *vSec;

	wstring readUntil(const wstring &str, wchar_t end, int *pos);
	bool is_gmt;
	wstring additionalTime;

public:
	TimePoint();
	~TimePoint();
	TimePoint(const wstring &dateStr);
	
	TimePoint* clone();
	bool isWeekMode();
	bool isEmpty();
	bool isEmpty(const wchar_t *slot);
	wstring toString();
	void addContext(TimePoint *context);
	int compareTo(TimePoint *t);
	void set(const wchar_t *slot, const wstring &val);
	void set(const wchar_t *slot, TimeUnit *val);
	TimeUnit* get(const wchar_t *slot);
	void normalizeTime();
	void normalizeSlot(const wchar_t *slot);
	void setRelativeTime(TimePoint *context, const wstring &verbTense, const wstring &mod, const wstring &unit, float amt);
	void setRelativeTime(TimePoint *context, const wstring &verbTense, const wstring &mod, const wstring &unit, int num);
	void clear();

	bool getIsGmt() { return is_gmt; }
	void setIsGmt(bool gmt) { is_gmt = gmt; }

	wstring getAdditionalTime() { return additionalTime; }
	void setAdditionalTime(wstring &str) { additionalTime = str; }

	
};


#endif
