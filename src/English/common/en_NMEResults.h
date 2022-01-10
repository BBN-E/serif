// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_NMERESULTS_H
#define EN_NMERESULTS_H
#include <list>
#include <vector>

class NMEResult;

typedef std::vector < NMEResult > NMEResultVector;

/**	This structure stores a wstring of a discovered entity along with the literal starts
	and stops of that entity relative to the beginning of the source file. 
	The structure NMEResultVector stores a sequence of such structures.
	MSeldin. 2/10/2003
*/
class NMEResult {
private:
	std::wstring * _resultstring;
	int _startlocation;
	int _endlocation;
public:
	NMEResult (std::wstring * res, int st,int end) : _resultstring(res),_startlocation(st),_endlocation(end) {};
	std::wstring * getResult() const {return _resultstring;}
	int getStart() const {return _startlocation;}
	int getEnd() const {return _endlocation;}
	
};



#endif
