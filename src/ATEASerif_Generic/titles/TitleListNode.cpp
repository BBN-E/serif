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

#include "common/leak_detection.h"

#include "ATEASerif_generic/titles/TitleListNode.h"
#include "common/UnexpectedInputException.h"
#include "common/Symbol.h"

TitleListNode::TitleListNode(Symbol *symbolArray, int length) 
{
	_title_length = length;
	_title = new Symbol[_title_length];

	int i;
	for (i = 0; i < length; i++) 
		_title[i] = symbolArray[i];

	_next = NULL;
	_prev = NULL;
}

TitleListNode::~TitleListNode()
{
	delete [] _title;
}

Symbol TitleListNode::getTitleWord(int n) 
{
	if (n >= _title_length) 
		throw UnexpectedInputException("TitleListNode::getTitleWord", "index equals or exceeds title length");
	else 
		return _title[n];
}
