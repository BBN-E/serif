// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TIMED_SECTION_H
#define TIMED_SECTION_H

#include <ostream>
#include <string>

/** Provides a mechanism for instrumented timing & reporting of code sections.
	
	Uses a recursive output format, like:
	<timed_section name="foo"> 450.2 </timed_section>

	<timed_section name="bar"> <timed_section name="bar_subpart_1"> 50.1 </timed_section> 70.8 </timed_section>
*/

class TimerCommon {
protected:
	TimerCommon();
	double currentElapsed();
	void startClock();
	unsigned char _ticks[16];
#if defined(WIN32) || defined(WIN64)
#else
	clock_t _clockTicks1;
#endif

};

class TimedSection : public TimerCommon {
public:
	
	TimedSection(std::string section_name, bool report_as_leaf = false, std::ostream & os = *_default_os );
	~TimedSection();
	
	static void set_default_ostream( std::ostream & os );

private:

	std::string _section_name;
	std::ostream & _os;
	bool _leaf;

	static std::ostream * _default_os;

public:
	void flushTS(){
		_os.flush();
	};

};

class TimedSectionAccumulate : public TimerCommon {
public:
	TimedSectionAccumulate(double* accum);
	~TimedSectionAccumulate();
private:
	double* _accumulator;
};

#endif // #ifndef PTIMER_H
