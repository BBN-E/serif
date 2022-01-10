// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <limits>

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/UTCoref/UTWordLists.h"
#include <boost/scoped_ptr.hpp>

// ported with use_first_sense=False
// both 'F' and 'U' map to false
bool UTWordLists::isParent(const std::wstring &child_word, const std::wstring &parent_synset) const {
   std::wstring stem;
   lookupStem(child_word, stem);

   std::map<std::wstring, std::vector<std::wstring> >::const_iterator sense_iter = word_map.find(stem);
   if (sense_iter == word_map.end()) {
	  return false; // not in wn
   }
   
   const std::vector<std::wstring> senses = sense_iter->second;

   BOOST_FOREACH(const std::wstring &sense, senses) {
	  std::vector<const std::wstring *> ancestors;
	  getAncestors(sense, ancestors);
	  
	  BOOST_FOREACH(const std::wstring *ancestor, ancestors) {
		 if (parent_synset == *ancestor) {
			return true;
		 }
	  }
   }

   return false;
}

void UTWordLists::getAncestors(const std::wstring &synset, std::vector<const std::wstring *> &ancestors) const {

   ancestors.push_back(&synset);

   std::map<std::wstring, std::vector<std::wstring> >::const_iterator it = hypernym_tree.find(synset);
   if (it == hypernym_tree.end()) {
	  return; /* no hypernyms */
   }

   BOOST_FOREACH(const std::wstring &parent, it->second) {
	  ancestors.push_back(&parent);
	  getAncestors(parent, ancestors);
   }
}

void UTWordLists::lookupStem(const std::wstring &word, std::wstring &stem) const {
   std::map<std::wstring, std::wstring>::const_iterator it = stems.find(word);
   if (it == stems.end()) {
	  stem = word;
   }
   else {
	  stem = it->second;
   }
}

// ported with use_first_sense=True
void UTWordLists::lca(const std::wstring &word1, const std::wstring &word2,
					  std::wstring &common_ancestor) const {
   // words should be lowercase

   common_ancestor = L"None";

   std::wstring stem1, stem2;

   lookupStem(word1, stem1);
   lookupStem(word2, stem2);

   std::map<std::wstring, std::vector<std::wstring> >::const_iterator it1 = word_map.find(stem1);
   std::map<std::wstring, std::vector<std::wstring> >::const_iterator it2 = word_map.find(stem2);

   if (it1 == word_map.end() || it2 == word_map.end() ) {
	  return; /* one of them is not in wordnet, so no common ancestor */
   }

   if (it1->second.size() == 0 || it2->second.size() == 0) {
	  return; /* no senses for some reason */
   }

   // using first sense
   std::wstring sense1, sense2;
   sense1 = it1->second[0];
   sense2 = it2->second[0];

   std::vector<const std::wstring *> ancestors1;
   std::vector<const std::wstring *> ancestors2;

   getAncestors(sense1, ancestors1);
   getAncestors(sense2, ancestors2);

   unsigned int best_ancestor_height = std::numeric_limits<unsigned int>::max();

   for (unsigned int height_1 = 0 ; height_1 < ancestors1.size() ; height_1++) {
	  for (unsigned int height_2 = 0 ; height_2 < ancestors2.size() ; height_2++) {
		 if (*(ancestors1[height_1]) == *(ancestors2[height_2]))
		 {
			if (/*best_ancestor_height == -1 || */height_1 + height_2 < best_ancestor_height) {
			   best_ancestor_height = height_1 + height_2;
			   common_ancestor = *ancestors1[height_1];
			}
		 }
	  }
   }
   // if a common_ancestor was not found it will be None
}




void UTWordLists::populateTable(const std::string &fname, std::map<std::wstring, std::wstring> &table) {

   boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build(fname.c_str()));
   UTF8InputStream& uis(*uis_scoped_ptr);
   std::wstring cur_line;
   size_t space_pos;

   while (uis.getLine(cur_line)) {
	  space_pos = cur_line.find(L' ');
	  if(space_pos != std::wstring::npos) {
		 const std::wstring key = cur_line.substr(0,space_pos);
		 const std::wstring val = cur_line.substr(space_pos+1);
		 table.insert(std::pair<const std::wstring,const std::wstring>(key,val));
	  }
   }
}


void UTWordLists::populateTree(const std::string &fname, std::map<std::wstring, std::vector<std::wstring> > &tree) {

   boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build(fname.c_str()));
   UTF8InputStream& uis(*uis_scoped_ptr);
   std::wstring cur_line;
   static boost::char_separator<wchar_t> div(L" ");
   size_t space_pos;


   while (uis.getLine(cur_line)) {
	  space_pos = cur_line.find(L' ');
      if(space_pos != std::wstring::npos && space_pos+1 < cur_line.length()) {
		 std::wstring before_space = cur_line.substr(0,space_pos);
		 std::wstring after_space = cur_line.substr(space_pos+1);

		 std::vector<std::wstring> &list = tree[before_space];
		 boost::tokenizer<boost::char_separator<wchar_t>,
			std::wstring::const_iterator, std::wstring > tokens(after_space, div);

		 BOOST_FOREACH(const std::wstring &str, tokens) {
			list.push_back(str);
		 }
	  }
   }
}

void UTWordLists::load() {

   std::string hypernym_tree_fname = ParamReader::getRequiredParam("utcoref_hypernym_tree_fname");
   std::string stems_fname = ParamReader::getRequiredParam("utcoref_stems_fname");
   std::string word_map_fname = ParamReader::getRequiredParam("utcoref_word_map_fname");

   populateTable(stems_fname, stems);
   populateTree(hypernym_tree_fname, hypernym_tree);
   populateTree(word_map_fname, word_map);

}
