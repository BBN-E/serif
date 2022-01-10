// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_WORD_LISTS_H
#define UT_WORD_LISTS_H

#include <string>
#include <vector>
#include <map>

class UTWordLists {
private:
   std::map<std::wstring, std::wstring> stems;
   std::map<std::wstring, std::vector<std::wstring> > hypernym_tree;
   std::map<std::wstring, std::vector<std::wstring> > word_map;

   void populateTable(const std::string &fname, std::map<std::wstring, std::wstring> &table);
   void populateTree(const std::string &fname, std::map<std::wstring, std::vector<std::wstring> > &tree);

public:
   UTWordLists() {}

   void load();
   void getAncestors(const std::wstring &synset, std::vector<const std::wstring *> &ancestors) const;
   bool isParent(const std::wstring &child_word, const std::wstring &parent_synset) const;
   void lca(const std::wstring &word1, const std::wstring &word2, std::wstring &common_ancestor) const;
   void lookupStem(const std::wstring &word, std::wstring &stem) const;

};

#endif
