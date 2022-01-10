// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef GENERIC_INTERNAL_TIMER_H
#define GENERIC_INTERNAL_TIMER_H

#if defined(_WIN32)
#include <windows.h>

class GenericInternalTimer {
public:
	GenericInternalTimer() {
		QueryPerformanceFrequency(&lfreq);
		resetTimer();
	};

	LARGE_INTEGER getLfreq() const { return lfreq; }
	LARGE_INTEGER getStart() const { return start; }
	LARGE_INTEGER getEnd() const { return end; }
	LONGLONG getClock() const { return timer; }
	unsigned long getCount() const { return count; }
	double getTime() const { return double(timer)/lfreq.QuadPart*1000; }

	void resetTimer() {
		timer = 0;
		count = 0;
	};

	void startTimer() {
		QueryPerformanceCounter(&start);
	};

	void stopTimer() {
		QueryPerformanceCounter(&end);
		timer += end.QuadPart - start.QuadPart;
	};

	void increaseCount() {
		count++;
	};

protected:
	LARGE_INTEGER lfreq, start, end;
	LONGLONG      timer;
	unsigned long count;
};

#else
// Linux
#include <ctime>

class GenericInternalTimer {
public:
        GenericInternalTimer() {
          resetTimer();
	};

        void resetTimer() {
          start = 0;
          elapsed = 0;
          count = 0;
        }
        
        void startTimer() {
          start = clock();
        }

        void stopTimer() {
          if (start > 0) {
            clock_t end = clock();
            elapsed = elapsed + end - start;
            start = 0;
          }
        }

        void increaseCount() {
          ++count;
        }
        
        unsigned long getCount() const {
          return count;
        }
        
        __int64 getClock() const {
          return 0;
        }

        double getTime() const {
          double time = 0;
          if (elapsed > 0) {
            time = ((double) elapsed) * 1000.0 / CLOCKS_PER_SEC;
          }
          return time; // msec (but accurate to 10 msec)
          
        }

private:
        clock_t start;
        clock_t elapsed;
        unsigned long count;
};

#endif

#endif
