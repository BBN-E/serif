// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.



#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/TimedSection.h"
#include "Generic/common/UnrecoverableException.h"

#if defined(WIN32) || defined(WIN64)
#include <windows.h>
#else
// Unix fill in, though not good at finest grain
#include <ctime>
#endif
#include <iostream>

using namespace std;

// static initialization
ostream * TimedSection::_default_os = &cout;

TimerCommon::TimerCommon () {
	startClock();
}

void TimerCommon::startClock() {
#if defined(WIN32) || defined(WIN64)
	LARGE_INTEGER & ticks1 = *((LARGE_INTEGER*) _ticks);

	// capture time
	QueryPerformanceCounter( &ticks1 );

#else

	_clockTicks1 = clock();

	//	cerr << "TimedSection::TimedSection() is currently not supported on this platform!" << endl;
	//	throw;
#endif
}

double TimerCommon::currentElapsed(){
#if defined(WIN32) || defined(WIN64)
	LARGE_INTEGER & ticks1 = *((LARGE_INTEGER*) _ticks);
	
	// capture time
	LARGE_INTEGER ticks2 = LARGE_INTEGER();
	QueryPerformanceCounter( &ticks2 );
	
	LARGE_INTEGER resolution = LARGE_INTEGER();
	QueryPerformanceFrequency( &resolution );
	return ((double)(ticks2.QuadPart - ticks1.QuadPart) / resolution.QuadPart );
#else
	clock_t clockTicks2 = clock();
	double clockDif = (double)(clockTicks2 - _clockTicks1);
	return ((double)(clockDif / CLOCKS_PER_SEC));
//	std::cerr << "TimedSection::currentElasped() is currently not supported on this platform!" << endl;
//	throw;
//	return -1.f; // not reached
#endif
}

TimedSection::TimedSection( string section_name, bool report_as_leaf /* = false */, 
						   std::ostream & os /* = _default_os */ ) 
						   : _section_name( section_name ), _leaf( report_as_leaf ), 
						   _os( os ) 
{
	if( !report_as_leaf )
		_os << "<timed_section name=\"" << _section_name << "\">\n" << flush;
	
	return;
}

TimedSection::~TimedSection(){

	double elapsed_time = currentElapsed();

	if( _leaf )
		_os << "<timed_section name=\"" << _section_name << "\"> ";

	_os << elapsed_time;
#if defined(WIN32) || defined(WIN64)
#else
	_os << "(ctime)";
#endif
	_os  << " </timed_section>\n" << flush;

	return;
}

void TimedSection::set_default_ostream( std::ostream & os ){
	_default_os = &os;
}

TimedSectionAccumulate::TimedSectionAccumulate(double* accum) : _accumulator(accum)  
{
	if (!_accumulator) {
		throw UnrecoverableException("TimedSectionAccumulate::TimedSectionAccumulate()", 
			"Null pointer passed to constructor");
	}
} 

TimedSectionAccumulate::~TimedSectionAccumulate() {
	*_accumulator+=currentElapsed();
}
