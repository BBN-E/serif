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

#ifndef TITLEMAP_H
#define TITLEMAP_H

#include <cstddef>
#include "common/hash_map.h"
#include "common/Symbol.h"
#include <string>

class TitleListNode;
class UTF8InputStream;

class TitleMap
{
private:
	static const float _target_loading_factor;

	struct HashKey {
		size_t operator()(const Symbol& s) const {
			return s.hash_code();
		}
	};
    struct EqualKey {
        bool operator()(const Symbol& s1, const Symbol& s2) const {
            return s1 == s2;
        }
    };

public:
	TitleMap(UTF8InputStream& in);

	typedef hash_map<Symbol, TitleListNode*, HashKey, EqualKey> Map;
	TitleListNode* lookup(const Symbol s) const;

private:
	Map* _map;
	void _insertNewNode(TitleListNode* newNode, TitleListNode* node);
	std::wstring TitleMap::trim(std::wstring line);

};


#endif
