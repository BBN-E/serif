// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_HEADFINDER_H
#define ch_HEADFINDER_H

#include <cstddef>
#include <string>
#include "Generic/common/hash_map.h"
#include "Generic/trainers/HeadFinder.h"

class Symbol;
class Production;
class UTF8InputStream;

/** Makes an educated guess about the head index of a headless 
  * parse node, given its production rule (for example, NP -> DNP NP)
  */
class ChineseHeadFinder : public HeadFinder{
private:
	friend class ChineseHeadFinderFactory;

public:

	~ChineseHeadFinder();

	/** Finds the most likely head index given a production rule.
	  *	@param production a pointer to the production whose head we want to find
	  *	@return the right-hand-side index of the most likely head
      */  
	int get_head_index();

private:
	ChineseHeadFinder();
	struct HashKey {
        size_t operator()(const Production *p) const {
			size_t val = p->left.hash_code();
			for (int i = 0; i < p->number_of_right_side_elements; i++)
				val = (val << 2) + p->right[i].hash_code();
			return val;
        }
	};

    struct EqualKey {
        bool operator()(const Production *p1, const Production *p2) const {
			if (p1->left != p2->left)
				return false;
			if (p1->number_of_right_side_elements != 
				p2->number_of_right_side_elements)
				return false;
			for (int i = 0; i < p1->number_of_right_side_elements; i++) {
				if (p1->right[i] != p2->right[i]) 
                   return false;
			}
			return true;
		}
	}; 

public:
	typedef serif::hash_map <Production*, int, HashKey, EqualKey> ProductionHeadTable;
	
private:
	ProductionHeadTable *headTable;
	void initializeTable(UTF8InputStream &stream);	

	static int findHeadADJP();
	static int findHeadADVP();
	static int findHeadCLP();
	static int findHeadCP();
	static int findHeadDNP();
	static int findHeadDP();
	static int findHeadDVP();
	static int findHeadFRAG();
	static int findHeadIP();
	static int findHeadLST();
	static int findHeadLCP();
	static int findHeadNP();
	static int findHeadPP();
	static int findHeadPRN();
	static int findHeadQP();
	static int findHeadUCP();
	static int findHeadVCD();
	static int findHeadVCP();
	static int findHeadVNV();
	static int findHeadVP();
	static int findHeadVPT();
	static int findHeadVRD();
	static int findHeadVSB();
	
	static int priority_scan_from_left_to_right (Symbol* set, int size);
	static int priority_scan_from_right_to_left (Symbol* set, int size);
	static int scan_from_left_to_right (Symbol* set, int size);
	static int scan_from_right_to_left (Symbol* set, int size);

};

class ChineseHeadFinderFactory: public HeadFinder::Factory {
	virtual HeadFinder *build() { return _new ChineseHeadFinder(); }
};


#endif
