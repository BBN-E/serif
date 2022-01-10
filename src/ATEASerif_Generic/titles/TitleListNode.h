// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

/****************************************************************************/
/* Copyright 2003 by BBN Technologies, LLC                                  */
/*                                                                          */
/* Use, duplication, or disclosure by the Government is subject to          */
/* restrictions as set forth in subparagraph (c)(1)(ii) of the Rights in    */
/* Technical Data and Computer Software clause at DFARS 252.227-7013.       */
/*                                                                          */
/*      BBN Technologies                                                    */
/*      10 Moulton St.                                                      */
/*      Cambridge, MA 02138                                                 */
/*      617-873-3411                                                        */
/*                                                                          */
/****************************************************************************/

#ifndef TITLELISTNODE_H
#define TITLELISTNODE_H

class Symbol;

class TitleListNode
{

public:

	TitleListNode(Symbol* symbolList, int length);
	~TitleListNode();

	Symbol getTitleWord(int n);
	int getTitleLength() { return  _title_length; }

	TitleListNode* getNext() { return _next; }
	TitleListNode* getPrev() { return _prev; }

	void setNext(TitleListNode* node) { _next = node; }
	void setPrev(TitleListNode* node) { _prev = node; }


private:
	int _title_length;
	Symbol *_title;

	TitleListNode *_next;
	TitleListNode *_prev;

};

#endif
