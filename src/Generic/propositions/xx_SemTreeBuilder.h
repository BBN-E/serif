// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/propositions/SemTreeBuilder.h"
#include "Generic/propositions/sem_tree/SemNode.h"
#include "Generic/propositions/sem_tree/SemBranch.h"
#include "Generic/theories/SynNode.h"


class DefaultSemTreeBuilder : public SemTreeBuilder {
private:
	friend class DefaultSemTreeBuilderFactory;

public:
	void initialize() {}

	SemNode *buildSemTree(const SynNode *synNode,
								 const MentionSet *mentionSet)
	{
		return _new SemBranch(0, synNode);
	}

	bool isNegativeAdverb(const SynNode &node) {
		return false;
	}
	bool isCopula(const SynNode &node) {
		return false;
	}
	bool isModalVerb(const SynNode &node) {
		return false;
	}
	bool canBeAuxillaryVerb(const SynNode &node) {
		return false;
	}

	SemOPP *findAssociatedPredicateInNP(SemLink &link) {
		return 0;
	}

	bool uglyBranchArgHeuristic(SemOPP &parent, SemBranch &child) {
		return false;
	}

	int mapSArgsToLArgs(SemNode **largs, SemOPP &opp,
							   SemNode *ssubj, SemNode *sarg1, SemNode *sarg2,
							   int n_links, SemLink **links)
	{
		largs[0] = 0;
		largs[1] = 0;
		largs[2] = 0;
		return 3;
	}
};



class DefaultSemTreeBuilderFactory: public SemTreeBuilder::Factory {
	DefaultSemTreeBuilder* utils;
	virtual SemTreeBuilder *get() { 
		if (utils == 0) {
			utils = _new DefaultSemTreeBuilder(); 
		} 
		return utils;
	}

public:
	DefaultSemTreeBuilderFactory() { utils = 0;}
};



#if 0 
package logicalanalysis;

import java.util.*;

import descriptors.NPClassifier;
import parsetree.ParseNode;
import utils.Symbol;
import utils.SymbolMap;
import parsetree.Tag;
import propositions.PredTypes;
import lexicon.*;
import relations.RelationUtils;

// this class is for all the overtly English-specific stuff

public class EnglishSemTreeBuilder extends TreeBuilder
{
  public static final boolean USE_MENTION_SETS = false;

  static String state_abbrev_list[] = _new String[] {
    "ak", "al", "ala", "alas", "ar", "ariz", "ark", "az", "ca", "cal",
    "calif", "co", "col", "colo", "conn", "ct", "de", "del", "fl", "fla",
    "ga", "ha", "hawaii", "hi", "ia", "id", "ida", "il", "ill", "in", "ind",
    "kan", "kans", "ks", "ky", "la", "ma", "mass", "md", "me", "mi", "mich",
    "minn", "miss", "mn", "mo", "mon", "mont", "ms", "mt", "nc", "nd", "ndak",
    "ne", "neb", "nebr", "nev", "nh", "nj", "nm", "nmex", "nv", "ny", "o",
    "oh", "ok", "okla", "or", "ore", "pa", "penna", "ri", "sc", "sd", "sdak",
    "tenn", "tex", "tn", "tx", "ut", "utah", "va", "vt", "wa", "wash", "wi",
    "wis", "wv", "wva", "wy", "wyo"};

  static Hashtable state_abbrev_table;
  
  static Hashtable non_noun_temporals;
  
  
  public EnglishSemTreeBuilder() {
    state_abbrev_table = _new Hashtable();
    for (int i = 0; i < state_abbrev_list.length; i++)
      state_abbrev_table.put(state_abbrev_list[i], _new Boolean(true));
    
    non_noun_temporals = _new Hashtable();
    for (int i = 0; i < RelationUtils.past.length; i++)
      non_noun_temporals.put(RelationUtils.past[i], _new Boolean(true));
    for (int i = 0; i < RelationUtils.pres.length; i++)
      non_noun_temporals.put(RelationUtils.pres[i], _new Boolean(true));
    for (int i = 0; i < RelationUtils.future.length; i++)
      non_noun_temporals.put(RelationUtils.future[i], _new Boolean(true));
  }
  

  // this is the basic bag of heuristics for constructing the initial semantic tree
  public SemUnit makeSemUnit(ParseNode parse_node) {
    // create vector of children; each entry is a SemUnit if possible,
    // or just the original parse_node otherwise
    Vector parse_children = _new Vector(parse_node.getNChildren());
    Vector analyzed_children = _new Vector(parse_node.getNChildren());
    for (int i = 0; i < parse_node.getNChildren(); i++) {
      ParseNode child = parse_node.getChild(i);
      SemUnit sem_child = makeSemUnit(child);
      if (sem_child != null) {
        parse_children.addElement(sem_child);
        analyzed_children.addElement(sem_child);
      }
      else {
        parse_children.addElement(child);
      }
    }
    
    // create array of analyzed (SemUnit) children
    SemUnit child_array[] = _new SemUnit[analyzed_children.size()];
    for (int i = 0; i < child_array.length; i++)
      child_array[i] = (SemUnit) analyzed_children.elementAt(i);
    
    SemUnit result;
    
    Symbol type_sym = parse_node.getTag();

    // now we branch off according to the constit type
    if (type_sym == STags::TOP) {
      // for a TOP, just pass child up
      if (child_array.length > 0)
        result = child_array[0];
      else
        result = null;
    }
    else if (type_sym == STags::S ||
             type_sym == STags::SBAR ||
             type_sym == STags::SINV ||
             type_sym == STags::FRAG ||
             type_sym == STags::FRAGMENTS)
    {
      // for sentence, make a branch node ordinarily; but if there's a 
      // preposition or WH-word linking it back to it's parent, make
      // a link
      
      Symbol first_child_sym = null;
      if (parse_node.getNChildren() > 0)
        first_child_sym = parse_node.getChild(0).getTag();
      
      if (first_child_sym == STags::IN) {
        result = _new SemLink(child_array, parse_node, SemLink.PREPOSITION_LINK, parse_node.getChild(0));
      }
      else if (first_child_sym == STags::WHADVP && 
               parse_node.getChild(0).getNChildren() == 1)
      {
        result = _new SemLink(child_array, parse_node, SemLink.PREPOSITION_LINK, parse_node.getChild(0).getChild(0));
      } 
      else {
        // no linker word -- just a regular branch
        
        // if there's a negative here, move it to any opp we find
        ParseNode negative = null;
        for (int i = 0; i < parse_children.size(); i++) {
          if (parse_children.elementAt(i) instanceof ParseNode &&
              isNegativeAdverb((ParseNode) parse_children.elementAt(i)))
          {
            negative = (ParseNode) parse_children.elementAt(i);
            
            break;
          }
        }
        if (negative != null) {
          for (int i = 0; i < child_array.length; i++) {
            if (child_array[i] instanceof SemOPP)
              ((SemOPP) child_array[i]).addNegative(negative);
            else if (child_array[i] instanceof SemOPPSet)
              ((SemOPPSet) child_array[i]).addNegative(negative);
          }
        }
        
        result = _new SemBranch(child_array, parse_node);
      }
    }
    else if (type_sym == STags::PRN) {
      // first, check for a "(Dem.-Mass.)" construction
      ParseNode rep_parts[] = getRepresentativeConstructionParts(parse_node);
      if (rep_parts != null) {
        ParseNode party_node = rep_parts[0];
        ParseNode state_node = rep_parts[1];
        
        SemOPP party_name = _new SemOPP(_new SemUnit[] {}, party_node,
                                       party_node, PredTypes.NAME_PREDICATION);
        SemMention party_mention = _new SemMention(_new SemUnit[] {party_name},
                                                  party_node, true);
        
        SemOPP state_name = _new SemOPP(_new SemUnit[] {}, state_node,
                                       state_node, PredTypes.NAME_PREDICATION);
        SemMention state_mention = _new SemMention(_new SemUnit[] {state_name},
                                                  state_node, true);
        
        SemOPP rep_opp = _new SemOPP(_new SemUnit[] {party_mention, state_mention},
                                    parse_node, parse_node, 
                                    SymbolMap.add("<representative>"),
                                    PredTypes.REPRESENTATIVE_PREDICATION);
        result = _new SemMention(_new SemUnit[] {rep_opp}, parse_node, false);
      }
      else {
      // just a regular parenthetical -- pass up the child if there is only one,
      // and create a branch to contain all the children if more than one
        if (child_array.length == 0) {
          result = null;
        }
        else if (child_array.length == 1) {
          result = child_array[0];
        }
        else {
          result = _new SemBranch(child_array, parse_node);
        }
      }
    }
    else if (type_sym == STags::NP ||
             type_sym == STags::NPA ||
             type_sym == STags::NX ||
             type_sym == STags::NPPOS ||
             type_sym == STags::NPP ||
             type_sym == STags::DATE)
    {
      // see if it's a "City, State" constrcution
      ParseNode city_and_state[] = getCityStateComponents(parse_node);
      if (city_and_state != null) {
        SemOPP state_name_opp = _new SemOPP(_new SemUnit[] {}, city_and_state[1],
                                           city_and_state[1], PredTypes.NAME_PREDICATION);
        SemMention state = _new SemMention(_new SemUnit[] {state_name_opp},
                                          city_and_state[1], true);
        SemOPP state_opp = _new SemOPP(_new SemUnit[] {state}, city_and_state[1],
                                      city_and_state[1], PredTypes.LOCATION_PREDICATION);
        SemOPP city_name_opp = _new SemOPP(_new SemUnit[] {}, city_and_state[0],
                                          city_and_state[0], PredTypes.NAME_PREDICATION);
        result = _new SemMention(_new SemUnit[] {city_name_opp, state_opp},
                                city_and_state[0], true);
      }
      else if (isNPList(parse_node)) {
        // looking at list of mentions
        if (USE_MENTION_SETS) {
          result = _new SemMentionSet(child_array, parse_node);
        }
        else {
          // replace all mentions with a "member" link to that mention
          for (int i = 0; i < child_array.length; i++) {
            if (child_array[i] instanceof SemMention) {
              child_array[i] = _new SemLink(_new SemUnit[] {child_array[i]},
                                           child_array[i].getParseNode(),
                                           SemLink.MEMBER_LINK,
                                           child_array[i].getParseNode());
            }
          }
          SemOPP set_opp = _new SemOPP(child_array, parse_node,
                                      parse_node, SymbolMap.add("<set>"),
                                      PredTypes.SET_PREDICATION);
          result = _new SemMention(_new SemUnit[] {set_opp}, parse_node, false);
        }
      }
      else {
        // looking at non-list -- create SemMention
       
        boolean is_definite = isNPDefinite(parse_node);
        
        // get partitive head if it's a single-mention partitive
        ParseNode partitive_head = ParseNode.getEnglishPartitiveHead(parse_node);

        if (parse_node.isName()) {
          // names are easy -- no further analysis needed
          result = _new SemOPP(_new SemUnit[] {}, parse_node, parse_node, PredTypes.NAME_PREDICATION);
          result = _new SemMention(_new SemUnit[] {result}, parse_node, is_definite);
        }
        else if (partitive_head != null) {
          // for partitives, we have to decide whether to create a 
          // separate mention or not

          SemUnit object = makeSemUnit(partitive_head);
          // this is only a single-mention partitive if the object
          // is an indefinite NP

          // otherwise, create a separate mention with a partitive predicate
          SemLink of_link = _new SemLink(_new SemUnit[] {object}, partitive_head,
                                        SemLink.POSSESSIVE_LINK, parse_node);
          SemOPP partitive_opp = _new SemOPP(_new SemUnit[] {of_link}, parse_node, parse_node.getEnglishNPHeadPreterm(),
                                            PredTypes.PARTITIVE_PREDICATION);
          result = _new SemMention(_new SemUnit[] {partitive_opp}, parse_node, is_definite);
        }
        else {
          // before applying the big main NP heuristic, see if we have
          // a "such-and-such -based" or "such-and-such -led"
          for (int i = 0; i < parse_children.size() - 1; i++) {
            if (parse_children.elementAt(i) instanceof SemMention &&
                parse_children.elementAt(i+1) instanceof SemOPP)
            {
              SemOPP opp = (SemOPP) parse_children.elementAt(i+1);
              if (opp.getHeadSymbol().toString().startsWith("-") &&
                  opp.getHeadSymbol().toString().length() > 3 /* &&
                  opp.getHeadSymbol().toString().endsWith("ed")*/)
              {
                SemOPP new_node = _new SemOPP(_new SemUnit[] {(SemUnit) parse_children.elementAt(i)},
                                             opp.getParseNode(), opp.getParseNode(),
                                             PredTypes.MODIFIER_PHRASE_PREDICATION);
                parse_children.setElementAt(new_node, i);
                parse_children.removeElementAt(i+1);
              }
            }
          }
            
          // any SemMention child which has only one child itself should be 
          // replaced by its child ( this is mainly for NPP nodes)
          for (int i = 0; i < parse_children.size(); i++) {
            if (parse_children.elementAt(i) instanceof SemMention &&
                ((SemMention) parse_children.elementAt(i)).getNChildren() == 1)
            {
              parse_children.setElementAt(((SemMention) parse_children.elementAt(i)).getChild(0),
                                          i);
            }
          }

          Vector new_children = _new Vector();
            
          ParseNode mentions[] = getAppositiveParts(parse_node);
          if (mentions == null) {
            // no appositive -- just apply our name/noun/name heuristic
            new_children = applyNameNounNameHeuristic(parse_children);
          }
          else {
            // this is an appositive, so we keep all the children except for
            // simple modifiers
                
            // create apposition predicate
            SemMention lhs = _new SemMention(_new SemUnit[] {}, mentions[0], is_definite);
            SemMention rhs = _new SemMention(_new SemUnit[] {}, mentions[1], is_definite);
            SemLink lhs_link = _new SemLink(_new SemUnit[] {lhs}, mentions[0],
                                           SemLink.MEMBER_LINK, mentions[0]);
            SemLink rhs_link = _new SemLink(_new SemUnit[] {rhs}, mentions[1],
                                           SemLink.MEMBER_LINK, mentions[1]);
            SemOPP appos_opp = _new SemOPP(_new SemUnit[] {lhs_link, rhs_link},
                                          parse_node, parse_node,
                                          SymbolMap.add("<appositive>"),
                                          PredTypes.APPOSITIVE_PREDICATION);

            for (int i = 0; i < parse_children.size(); i++) {
              if (parse_children.elementAt(i) instanceof SemUnit) {
                if (parse_children.elementAt(i) instanceof SemOPP) {
                  int pred_type = ((SemOPP) parse_children.elementAt(i)).getPredicateType();
                  if (pred_type != PredTypes.MODIFIER_PREDICATION) {
                    if (pred_type != PredTypes.SET_PREDICATION) {
                      // for normal predicate, just add it
                      new_children.addElement(parse_children.elementAt(i));
                    }
                    else {
                      // for set predicate, attach to one of the appositive parts
                      SemOPP set = (SemOPP) parse_children.elementAt(i);
                        
                      if (lhs.getParseNode() == set.getParseNode()) {
                        lhs.appendChild(set);
                      }
                      else if (rhs.getParseNode() == set.getParseNode()) {
                        rhs.appendChild(set);
                      }
                      else {
                        // if that didn't work, then put that set members' children
                        // on the top mention node
                        System.err.println("APPOS: (Tell Sam about this) found set but not corresponding appos member: " + parse_node.toString());
                        for (int j = 0; j < set.getNChildren(); j++) {
                          if (set.getChild(j) instanceof SemLink &&
                              ((SemLink) set.getChild(j)).getLinkType() == SemLink.MEMBER_LINK)
                          {
                            SemLink member_link = (SemLink) set.getChild(j);
                              
                            if (member_link.getNChildren() == 1) {
                              SemUnit member = member_link.getChild(0);
                              for (int k = 0; k < member.getNChildren(); k++)
                                new_children.addElement(member.getChild(k));
                            }
                          }
                          else {
                            new_children.addElement(set.getChild(j));
                          }
                        }
                      }
                    }
                  }
                }
                else {
                  new_children.addElement(parse_children.elementAt(i));
                }
              }
            }
              
            new_children.addElement(appos_opp);
          }
              
          child_array = _new SemUnit[new_children.size()];
          for (int i = 0; i < child_array.length; i++)
            child_array[i] = (SemUnit) new_children.elementAt(i);
              
          result = _new SemMention(child_array, parse_node, is_definite);
        }

        if (isTemporalNP(parse_node)) {
          // make a link for the temporal np
          result = _new SemLink(_new SemUnit[] {result}, parse_node,
                               SemLink.TEMPORAL_LINK, null);
        }
        else if (type_sym == STags::NPPOS ||
                 (parse_node.getNChildren() > 0 &&
                  parse_node.getChild(parse_node.getNChildren()-1).getTag() == STags::POS))
        {
          // make a possessive link
          result = _new SemLink(_new SemUnit[] {result}, parse_node,
                               SemLink.POSSESSIVE_LINK, null);
        }
      }
    }
    else if (type_sym == STags::VP &&
             isVPList(parse_node))
    {
      // looking at list of predicates -- create SemOPPSet
      
      result = _new SemOPPSet(child_array, parse_node);
    }
    else if (type_sym == STags::VP ||
             type_sym == STags::ADJP ||
             type_sym == STags::JJ ||
             type_sym == STags::JJR ||
             type_sym == STags::JJS ||
             type_sym == STags::CD ||
             type_sym == STags::NN ||
             type_sym == STags::NNS ||
             type_sym == STags::NNP ||
             type_sym == STags::NNPS ||
             type_sym == STags::PRP)
    {
      // looking at (non-list) predicate -- create SemOPP
      
      // depending on the constit type, we set the predicate type, head
      // node, and for verbs, negative modifier
      int type;
      ParseNode head;
      ParseNode negative = null;
      
      if (type_sym == STags::VP) {
        type = PredTypes.VERB_PREDICATION;
        head = findNodeOfType(parse_children, _new Symbol[] {STags::VB, STags::VBD, STags::VBG, STags::VBN, STags::VBP, STags::VBZ, STags::MD, STags::TO});
        
        // for verb, we try to find a negative adverb as well
        negative = null;
        for (int i = 0; i < parse_children.size(); i++) {
          if (parse_children.elementAt(i) instanceof ParseNode) {
            ParseNode child = (ParseNode) parse_children.elementAt(i);
            if (isNegativeAdverb(child)) {
              negative = child;
              break;
            }
          }
        }
      }
      else if (type_sym == STags::ADJP) {
        type = PredTypes.MODIFIER_PHRASE_PREDICATION;
        head = findNodeOfType(parse_children, _new Symbol[] {STags::JJ, STags::JJR, STags::JJS});
        // getting more desperate:
        if (head == null)
          head = findNodeOfType(parse_children, _new Symbol[] {STags::RB});
      }
      else if (type_sym == STags::JJ || type_sym == STags::JJR || type_sym == STags::JJS || type_sym == STags::CD) {
        type = PredTypes.MODIFIER_PREDICATION;
        head = parse_node;
      }
      else if (type_sym == STags::PRP) {
        type = PredTypes.PRONOUN_PREDICATION;
        head = parse_node;
      }
      else /* only types left are noun types NN[P][S] */ {
        type = PredTypes.NOUN_PREDICATION;
        head = parse_node;
      }

      if (head == null) {
        result = _new SemOPP(child_array, parse_node, parse_node, SemOPP.SYMBOL_NONE, type);
      }
      else {
        result = _new SemOPP(child_array, parse_node, head, type);
        ((SemOPP) result).addNegative(negative);
      }
    }
    else if (type_sym == STags::PP)
    {
      // looking at a PP -- create SemLink
      
      ParseNode head = findNodeOfType(parse_children, _new Symbol[] {STags::IN, STags::TO, STags::VBG});
      
      // we also want to see if there's a nested PP
      SemLink link = null;
      Vector other_children = _new Vector(child_array.length);
      for (int i = 0; i < child_array.length; i++) {
        if (child_array[i] instanceof SemLink) {
          if (link != null) {
            // we just found a second link, meaning this is probably a PP conjuction
            // so just set link back to null and stop looking
            link = null;
            break;
          }
          else {
            link = (SemLink) child_array[i];
          }
        }
        else {
          other_children.addElement(child_array[i]);
        }
      }
      
      if (link != null) {
        // there's a nested link, so merge the children
        child_array = _new SemUnit[other_children.size() + link.getNChildren()];
        int j = 0;
        for (int i = 0; i < other_children.size(); i++)
          child_array[j++] = (SemUnit) other_children.elementAt(i);
        for (int i = 0; i < link.getNChildren(); i++)
          child_array[j++] = link.getChild(i);

        if (head != null && link.getLinkType() != SemLink.TEMPORAL_LINK) {
          // _new head symbol is combination of this head and the link's head
          Symbol head_sym = SymbolMap.add(head.getHeadWord().toString() + '_' + link.getHeadSymbol().toString());
        
          result = _new SemLink(child_array, parse_node, SemLink.PREPOSITION_LINK, head, head_sym);
        }
        else {
          // link, but no head, so use _new child array, but get head from the link
          result = _new SemLink(child_array, parse_node, SemLink.PREPOSITION_LINK, link.getHead(), link.getHeadSymbol());
        }
      }
      else {
        if (head == null)
          result = _new SemLink(child_array, parse_node, SemLink.PREPOSITION_LINK, head, SemLink.SYMBOL_NONE);
        else
          result = _new SemLink(child_array, parse_node, SemLink.PREPOSITION_LINK, head);
      }
    }
    else if (type_sym == STags::ADVP) {
      // the easiest case is "here" or "there"
      if (isLocativePronoun(parse_node)) {
        SemOPP pronoun_opp = _new SemOPP(_new SemUnit[] {}, parse_node,
                                        parse_node, PredTypes.PRONOUN_PREDICATION);
        SemMention pronoun_mention = _new SemMention(_new SemUnit[] {pronoun_opp},
                                                    parse_node, true);
        result = _new SemLink(_new SemUnit[] {pronoun_mention}, parse_node,
                             SemLink.LOCATION_LINK, parse_node);
      }
      else {
        // for other ADVPs, it's possible that we'll want to create link
        
        // first thing we need is an adv head
        ParseNode head = findNodeOfType(parse_children, _new Symbol[] {STags::RB, STags::RBR});
        
        // second thing we need is a link -- and a list of the other children
        SemLink link = null;
        Vector other_children = _new Vector(child_array.length);
        for (int i = 0; i < child_array.length; i++) {
          if (link == null && child_array[i] instanceof SemLink)
            link = (SemLink) child_array[i];
          else
            other_children.addElement(child_array[i]);
        }
        
        if (link != null && head != null) {
          // if we have both, then make a _new Link
          
          // put together list of all children of both this node and the link child
          child_array = _new SemUnit[/*other_children.size() +*/ link.getNChildren()];
          int j = 0;
  /*        for (int i = 0; i < other_children.size(); i++)
            child_array[j++] = (SemUnit) other_children.elementAt(i); */
          for (int i = 0; i < link.getNChildren(); i++)
            child_array[j++] = link.getChild(i);
          
          if (isLocativePronoun(head)) {
            // this is a case of "here in Bangkok"
            SemOPP pronoun_opp = _new SemOPP(_new SemUnit[] {link}, head,
                                            head, PredTypes.PRONOUN_PREDICATION);
            SemMention pronoun_mention = _new SemMention(_new SemUnit[] {pronoun_opp},
                                                        parse_node, true);
            result = _new SemLink(_new SemUnit[] {pronoun_mention}, parse_node,
                                 SemLink.LOCATION_LINK, head);
          }
          else {
            // this a case of "out of ..." (double-preposition)
            // _new head symbol is combination of this head and the link's head
            Symbol head_sym = SymbolMap.add(head.getHeadWord().toString() + '_' + link.getHeadSymbol().toString());
          
            result = _new SemLink(child_array, parse_node, SemLink.PREPOSITION_LINK, head, head_sym);
          }
        }
        else {
          result = null;
          
          // it's also possible that there's a name here which we should keep
          for (int i = 0; i < child_array.length; i++) {
            if (child_array[i] instanceof SemMention)
              result = child_array[i];
          }
        }
      }
    }
    else if (type_sym == STags::PRPDOLLAR ||
             (type_sym == STags::NPPRO &&
              parse_node.getNChildren() == 1 &&
              parse_node.getHead().getTag() == STags::PRPDOLLAR))
    {
      // if we have a possessive pronoun, make a special link
      
      // need to create a reference for the pronoun
      SemUnit grandchild = _new SemOPP(_new SemUnit[] {}, parse_node, parse_node, PredTypes.PRONOUN_PREDICATION);
      SemUnit child = _new SemMention(_new SemUnit[] {grandchild}, parse_node, false);
      result = _new SemLink(_new SemUnit[] {child}, parse_node, SemLink.POSSESSIVE_LINK, parse_node);
    }
    else {
      result = null;
    }
    
    return result;
  }
  
  
  // divide list of NP children into comma/CC-separated groups
  static Vector getNPChildGroups(Vector parse_children) {
    Vector groups = _new Vector();
    Vector cur_group = _new Vector();
    for (int i = 0; i < parse_children.size(); i++) {
      if (parse_children.elementAt(i) instanceof ParseNode) {
        Symbol node_type = ((ParseNode) parse_children.elementAt(i)).getTag();
        if (node_type == STags::CC ||
            node_type == STags::CONJP ||
            node_type == STags::COMMA)
        {
          if (cur_group.size() > 0) {
            groups.addElement(cur_group);
            cur_group = _new Vector();
          }
        }
      }
      else if (parse_children.elementAt(i) instanceof SemUnit) {
        cur_group.addElement(parse_children.elementAt(i));
      }
    }
    if (cur_group.size() > 0) {
      groups.addElement(cur_group);
    }
    return groups;
  }
  
  
  // apply name/noun/name heuristic for NP's 
  // (For stuff like "IBM president Bob Smith" and its sundry variations)
  private static Vector applyNameNounNameHeuristic(Vector group) {
    Vector result = _new Vector();
    
    // the bins we'll be putting nodes into:
    Vector premods = _new Vector();
    Vector postmods = _new Vector();
    SemOPP pre_name = null;
    SemOPP pre_name2 = null;
    SemOPP noun = null;
    SemOPP name = null;
              
    // for each node, assign it to one of the bins
    for (int i = 0; i < group.size(); i++) {
      if (group.elementAt(i) instanceof SemOPP) {
        SemOPP opp = (SemOPP) group.elementAt(i);
        if (opp.getPredicateType() == PredTypes.NAME_PREDICATION) {
          if (name != null) {
            if (pre_name == null)
              pre_name = name;
            else
              pre_name2 = name;
          }
          name = opp;
        }
        else if (opp.getPredicateType() == PredTypes.NOUN_PREDICATION) {
          noun = opp;
          if (name != null) { // we can't have a name that is followed by a noun
            if (pre_name == null)
              pre_name = name;
            else
              pre_name2 = name;
            name = null;
          }
        }
        else if (opp.getPredicateType() != PredTypes.MODIFIER_PREDICATION) {
          if (noun == null && name == null)
            premods.addElement(opp);
          else
            postmods.addElement(opp);
        }
      }
      else if (group.elementAt(i) instanceof SemUnit) {
        if (noun == null && name == null)
          premods.addElement(group.elementAt(i));
        else
          postmods.addElement(group.elementAt(i));
      }
    }
              
    // make an unknown-role link for the premodifier name
    SemLink pre_name_link = null;
    if (pre_name != null) {
      SemMention pre_name_mention = _new SemMention(_new SemUnit[] {pre_name}, pre_name.getParseNode(), true);
      pre_name_link = _new SemLink(_new SemUnit[] {pre_name_mention}, pre_name.getParseNode(), SemLink.UNKNOWN_LINK, null);
    }
    SemLink pre_name_link2 = null;
    if (pre_name2 != null) {
      SemMention pre_name_mention = _new SemMention(_new SemUnit[] {pre_name2}, pre_name2.getParseNode(), true);
      pre_name_link2 = _new SemLink(_new SemUnit[] {pre_name_mention}, pre_name2.getParseNode(), SemLink.UNKNOWN_LINK, null);
    }
              
    // now add what we found to _new list of children
    for (int i = 0; i < premods.size(); i++)
      result.addElement(premods.elementAt(i));
    if (pre_name_link != null)
      result.addElement(pre_name_link);
    if (pre_name_link2 != null && nodesSeparatedByHyphen(pre_name_link, pre_name_link2))
      result.addElement(pre_name_link2);
    if (noun != null)
      result.addElement(noun);
    if (name != null)
      result.addElement(name);
    for (int i = 0; i < postmods.size(); i++)
      result.addElement(postmods.elementAt(i));
    
    return result;
  }
  
  static boolean nodesSeparatedByHyphen(SemUnit sem_node1, SemUnit sem_node2) {
    ParseNode node1 = sem_node1.getParseNode();
    ParseNode node2 = sem_node2.getParseNode();
    ParseNode parent = node1.getParent();
    
    if (parent == null)
      return false;
    
    // find first node
    int i = 0;
    while (i < parent.getNChildren() &&
           parent.getChild(i) != node1 &&
           parent.getChild(i) != node2)
      i++;
    
    // make sure next node is hyphen
    i++;
    if (!(i < parent.getNChildren() &&
          parent.getChild(i).getText().equals("-")))
      return false;
    
    // make sure next node is the next node
    i++;
    if (!(i < parent.getNChildren() &&
          (parent.getChild(i) == node1 ||
           parent.getChild(i) == node2)))
      return false;
    
    return true;
  }
              
  // interpret "Fooville, Fooia" constructions by returning array {city-node, state-node}
  // (or null if not of this form)
  ParseNode[] getCityStateComponents(ParseNode node) {
    if (node.getNChildren() >= 3 &&
        node.getChild(0).isName() &&
        node.getChild(1).getTag() == STags::COMMA &&
        node.getChild(2).isName() &&
        (node.getNChildren() == 3 ||
         (node.getNChildren() == 4 &&
          node.getChild(3).getTag() == STags::COMMA)))
    {
      return _new ParseNode[] {node.getChild(0),
                              node.getChild(2)};
    } else {
      return null;
    }
  }
  
  private static Symbol party_syms[] = _new Symbol[] {SymbolMap.add("rep"), 
                                                     SymbolMap.add("r"),
                                                     SymbolMap.add("dem"), 
                                                     SymbolMap.add("d")};
  private static Symbol symbol_lrb = SymbolMap.add("-lrb-");
  private static Symbol symbol_rrb = SymbolMap.add("-rrb-");
  private static Symbol symbol_hyphen = SymbolMap.add("-");
  
  // interpret "(Rep.-Miss.)" representative constructions by
  // returning array {party, state}
  private static ParseNode[] getRepresentativeConstructionParts(ParseNode node) {
    ParseNode party;
    ParseNode state;
    
    Symbol syms[] = node.getSymbolArray();
    
    if (syms.length < 5 || syms.length > 8)
      return null;
    
    if (syms[0] != symbol_lrb)
      return null;
    
    if (syms[2] != symbol_hyphen)
      return null;
    
    boolean party_found = false;
    for (int i = 0; i < party_syms.length; i++) {
      if (syms[1] == party_syms[i]) {
        party_found = true;
        break;
      }
    }
    if (!party_found)
      return null;
    
    party = getNodeThatStartsAt(node, 1);
    state = getNodeThatStartsAt(node, 3);
    if (party != null && state != null)
      return _new ParseNode[] {party, state};
    else
      return null;
  }
  
  static ParseNode getNodeThatStartsAt(ParseNode node, int pos) {
    if (pos == 0)
      return node;
    int child_pos = 0;
    for (int i = 0; i < node.getNChildren(); i++) {
      ParseNode result = getNodeThatStartsAt(node.getChild(i), pos - child_pos);
      if (result != null)
        return result;
      child_pos += node.getChild(i).countTokens();
      if (child_pos > pos)
        return null;
    }
    return null;
  }
  
  
  private static Symbol symbol_be             = SymbolMap.add("be");
  private static Symbol symbol_is             = SymbolMap.add("is");
  private static Symbol symbol_are            = SymbolMap.add("are");
  private static Symbol symbol_was            = SymbolMap.add("was");
  private static Symbol symbol_were           = SymbolMap.add("were");
  private static Symbol symbol_been           = SymbolMap.add("been");
  private static Symbol symbol_being          = SymbolMap.add("being");
  private static Symbol symbol_become         = SymbolMap.add("become");
  private static Symbol symbol_becomes        = SymbolMap.add("becomes");
  private static Symbol symbol_became         = SymbolMap.add("became");
  private static Symbol symbol_becoming       = SymbolMap.add("becoming");

  public boolean isCopula(ParseNode parse_node) {
    Symbol label = parse_node.getHeadWord();
    if (label == symbol_be ||
        label == symbol_is ||
        label == symbol_are ||
        label == symbol_was ||
        label == symbol_were ||
        label == symbol_been ||
        label == symbol_being /*||
        label == symbol_become ||
        label == symbol_becomes ||
        label == symbol_became ||
        label == symbol_becoming*/)
      return true;
    else
      return false;
  }
  
  
  private static Symbol symbol_not    = SymbolMap.add("not");
  private static Symbol symbol_n_t    = SymbolMap.add("n't");
  private static Symbol symbol_never  = SymbolMap.add("never");
  
  public boolean isNegativeAdverb(ParseNode node) {
    if (node.getTag() == STags::ADVP &&
        node.getNChildren() > 0)
    {
      node = node.getHead();
    }
        
    if (node.getTag() == STags::RB) {
      Symbol headsym = node.getHeadWord();
      if (headsym == symbol_not ||
          headsym == symbol_n_t ||
          headsym == symbol_never)
        return true;
    }
    return false;
  }
  
  
  private static Symbol symbol_would   = SymbolMap.add("would");
  private static Symbol symbol_will    = SymbolMap.add("will");
  private static Symbol symbol_wo      = SymbolMap.add("wo");
  private static Symbol symbol_could   = SymbolMap.add("could");
  private static Symbol symbol_should  = SymbolMap.add("should");
  private static Symbol symbol_can     = SymbolMap.add("can");
  private static Symbol symbol_ca      = SymbolMap.add("ca");
  private static Symbol symbol_might   = SymbolMap.add("might");
  private static Symbol symbol_may     = SymbolMap.add("may");
  private static Symbol symbol_shall   = SymbolMap.add("shall");
  
  public boolean isModalVerb(ParseNode node) {
    if (node == null)
      return false;
/*    Symbol headsym = node.getHeadWord();
    if (headsym == symbol_would ||
        headsym == symbol_will ||
        headsym == symbol_wo ||
        headsym == symbol_could ||
        headsym == symbol_should ||
        headsym == symbol_can ||
        headsym == symbol_ca ||
        headsym == symbol_might ||
        headsym == symbol_may ||
        headsym == symbol_shall)*/
    if (node.getTag() == STags::MD)
      return true;
    return false;
  }
  
  
  private static Symbol symbol_here  = SymbolMap.add("here");
  private static Symbol symbol_there = SymbolMap.add("there");
  
  boolean isLocativePronoun(ParseNode parse_node) {
    Symbol tokens[] = parse_node.getSymbolArray();
    if (tokens.length == 1 &&
        (tokens[0] == symbol_here ||
         tokens[0] == symbol_there))
      return true;
    else
      return false;
  }


  // this is for links that we want to modify nouns -- it returns
  // the noun to modify or null if none found
  public SemOPP findAssociatedPredicateInNP(SemLink link) {
    if (link.getParent() != null &&
        link.getParent() instanceof SemMention)
    {
      SemMention parent = (SemMention) link.getParent();
      
      SemOPP noun = null;
      if (link.getLinkType() == SemLink.POSSESSIVE_LINK ||
          link.getLinkType() == SemLink.UNKNOWN_LINK)
      {
        // for possessives and described-with's, search forwards
        for (int i = link.getIndex() + 1; i < parent.getNChildren(); i++) {
          if (parent.getChild(i) instanceof SemOPP &&
              ((SemOPP) parent.getChild(i)).getPredicateType() == PredTypes.NOUN_PREDICATION)
          {
            noun = (SemOPP) parent.getChild(i);
            break;
          }
        }
      }
      else {
        // for other links, search backwards
        for (int i = link.getIndex() - 1; i >= 0; i--) {
          if (parent.getChild(i) instanceof SemOPP &&
              ((SemOPP) parent.getChild(i)).getPredicateType() == PredTypes.NOUN_PREDICATION)
          {
            noun = (SemOPP) parent.getChild(i);
            break;
          }
        }
      }
      
      return noun;
    }
    return null;
  }

  
  // this attempts to decide whether a branch (corresponding to an S or SBAR)
  // is acting as an argument to a verb or is set apart from the rest of the
  // clause by looking for a comma
  public boolean uglyBranchArgHeuristic(SemOPP parent, SemBranch child) {
    ParseNode parse = parent.getParseNode();
    if (parse == null) // i'm positive this can't happen, but not totally positive
      return true;
    for (int i = 0; i < parse.getNChildren(); i++) {
      if (parse.getChild(i).getTag() == STags::COMMA)
        return false;
    }
    return true;
  }
  
  
  private static final Symbol symbol_by = SymbolMap.add("by");
  
  // map syntactic args to logical args
  // the array output has at least 3 terms (any of which may be null) corresponding
  // to the 3 basic roles, plus any links
  public SemUnit[] mapSArgsToLArgs(SemOPP opp,
                                   SemUnit ssubj, SemUnit sarg1, SemUnit sarg2,
                                   SemLink[] links)
  {
    SemUnit largs[];
    
    if (opp.getPredicateType() == PredTypes.VERB_PREDICATION &&
        isPassiveVerb(opp.getHead()))
    {
      // passive verb
      SemUnit by_obj = null;
      Vector other_links = _new Vector();
      for (int i = 0; i < links.length; i++) {
        if (links[i].getHead() != null && 
            links[i].getHead().getHeadWord() == symbol_by)
        {
          by_obj = links[i].getObject();
        }
        else {
          other_links.addElement(links[i]);
        }
      }
      
      largs = _new SemUnit[3+other_links.size()];
      
      largs[0] = by_obj;
      largs[1] = ssubj;
      largs[2] = sarg1;
      for (int i = 0; i < other_links.size(); i++)
        largs[3+i] = (SemUnit) other_links.elementAt(i);
    }
    else {
      // active verb or simply not a verb
      largs = _new SemUnit[3 + links.length];
      largs[0] = ssubj;
      largs[1] = sarg1;
      largs[2] = sarg2;
      for (int i = 0; i < links.length; i++)
        largs[3+i] = links[i];
    }
    
    return largs;
  }
  
  private static boolean isPassiveVerb(ParseNode verb) {
    // sanity check -- we need to look at parent
    if (verb.getParent() == null)
      return false;
    
    // find topmost VP in verb cluster
    ParseNode top_vp = verb;
    while (top_vp.getParent() != null &&
           top_vp.getParent().getTag() == STags::VP)
      top_vp = top_vp.getParent();
    
    // figure out if we're in a reduced relative clause
    if (top_vp.getParent() != null &&
        top_vp.getParent().getTag() == STags::NP)
    {
      // in reduced relative, just see if verb is VBN
      if (verb.getTag() == STags::VBN)
        return true;
      else
        return false;
    }
    else {
      // not in reduced relative, so look for copula and VBN
      if (verb.getTag() == STags::VBN &&
          verb.getParent().getParent() != null &&
          verb.getParent().getParent().getTag() == STags::VP &&
          TreeBuilder.builder.isCopula(verb.getParent().getParent().getChild(0)))
      {
        return true;
      } else {
        return false;
      }
    }
  }
  
  
  private static Symbol[] temporal_symbols = _new Symbol[] {
      SymbolMap.add("yesterday"),
      SymbolMap.add("today"),
      SymbolMap.add("tomorrow"),
      SymbolMap.add("morning"),
      SymbolMap.add("afternoon"),
      SymbolMap.add("night"),
      SymbolMap.add("tonight"),
      SymbolMap.add("monday"),
      SymbolMap.add("tuesday"),
      SymbolMap.add("wednesday"),
      SymbolMap.add("thursday"),
      SymbolMap.add("friday"),
      SymbolMap.add("saturday"),
      SymbolMap.add("sunday"),
      SymbolMap.add("week"),
      SymbolMap.add("weeks"),
      SymbolMap.add("month"),
      SymbolMap.add("months"),
      SymbolMap.add("january"),
      SymbolMap.add("february"),
      SymbolMap.add("march"),
      SymbolMap.add("april"),
      SymbolMap.add("may"),
      SymbolMap.add("june"),
      SymbolMap.add("july"),
      SymbolMap.add("august"),
      SymbolMap.add("september"),
      SymbolMap.add("october"),
      SymbolMap.add("november"),
      SymbolMap.add("december"),
      SymbolMap.add("feb"),
      SymbolMap.add("mar"),
      SymbolMap.add("apr"),
      SymbolMap.add("may"),
      SymbolMap.add("jul"),
      SymbolMap.add("aug"),
      SymbolMap.add("sep"),
      SymbolMap.add("sept"),
      SymbolMap.add("oct"),
      SymbolMap.add("nov"),
      SymbolMap.add("feb."),
      SymbolMap.add("mar."),
      SymbolMap.add("apr."),
      SymbolMap.add("jun."),
      SymbolMap.add("jul."),
      SymbolMap.add("aug."),
      SymbolMap.add("sep."),
      SymbolMap.add("sept."),
      SymbolMap.add("oct."),
      SymbolMap.add("nov."),
      SymbolMap.add("spring"),
      SymbolMap.add("winter"),
      SymbolMap.add("summer"),
      SymbolMap.add("fall"),
      SymbolMap.add("1980"),
      SymbolMap.add("1981"),
      SymbolMap.add("1982"),
      SymbolMap.add("1983"),
      SymbolMap.add("1984"),
      SymbolMap.add("1985"),
      SymbolMap.add("1986"),
      SymbolMap.add("1987"),
      SymbolMap.add("1988"),
      SymbolMap.add("1989"),
      SymbolMap.add("1990"),
      SymbolMap.add("1991"),
      SymbolMap.add("1992"),
      SymbolMap.add("1993"),
      SymbolMap.add("1994"),
      SymbolMap.add("1995"),
      SymbolMap.add("1996"),
      SymbolMap.add("1997"),
      SymbolMap.add("1998"),
      SymbolMap.add("1999"),
      SymbolMap.add("2000"),
      SymbolMap.add("2001"),
      SymbolMap.add("2002"),
      SymbolMap.add("2003"),
      SymbolMap.add("2004"),
      SymbolMap.add("2005"),
      SymbolMap.add("2006"),
      SymbolMap.add("2007"),
      SymbolMap.add("2008"),
      SymbolMap.add("2009")};

  public boolean isTemporalNP(ParseNode parse_node) {
    if (parse_node.getParent() == null ||
        parse_node.getParent().getTag() == STags::NP)
      return false;
    
    if (parse_node.getTag() == STags::DATE)
      return true;
    
    Symbol word = parse_node.getHeadWord();
    for (int i = 0; i < temporal_symbols.length; i++) {
      if (word == temporal_symbols[i])
        return true;
    }
    return false;
  }
  

  SemMention getSingleMentionPartitiveHead(ParseNode parse_node) {
    // first see if it's a partitive
    ParseNode head = ParseNode.getEnglishPartitiveHead(parse_node);
    if (head == null) {
      return null;
    }
    else {
      SemUnit result = makeSemUnit(head);
      
      // this is only a *single-mention* partitive if the object
      // is an indefinite NP
      if (result instanceof SemMention &&
          ((SemMention) result).isDefinite() == false)
      {
        result.setParseNode(parse_node);
        return (SemMention) result;
      }
    }
    return null;
  }
  
  
  static Symbol definite_det_1 = SymbolMap.add("the");
  static Symbol definite_det_2 = SymbolMap.add("this");
  static Symbol definite_det_3 = SymbolMap.add("these");
  static Symbol definite_det_4 = SymbolMap.add("that");
  static Symbol definite_det_5 = SymbolMap.add("those");
  
  static boolean isNPDefinite(ParseNode node) {
    for (int i = 0; i < node.getNChildren(); i++) {
      ParseNode child = node.getChild(i);
      if (child.getTag() == STags::DT) {
        Symbol word = child.getHeadWord();
        if (word == definite_det_1 ||
            word == definite_det_2 ||
            word == definite_det_3 ||
            word == definite_det_4 ||
            word == definite_det_5)
        {
          return true;
        }
      }
    }
    return false;
  }
  
  
  // returns a 2-vector of NP nodes comprising appositive, or null
  // if it doesn't look like an appositive
  public ParseNode[] getAppositiveParts(ParseNode node) {
    // first, make sure we don't have an NN, because surely that's
    // not an appositive even if there are NP premods
    if (node.getHead().getTag() == STags::NN ||
        node.getHead().getTag() == STags::NNS ||
        node.getHead().getTag() == STags::NNP ||
        node.getHead().getTag() == STags::NNPS)
    {
      return null;
    }

    ParseNode result[] = _new ParseNode[] {null, null};

    boolean comma_needed = false;
    
    for (int i = 0; i < node.getNChildren(); i++) {
      if (node.getChild(i).getTag() == STags::COMMA) {
        comma_needed = false;
      }
      else if (isNPType(node.getChild(i).getTag()) &&
               !isNumber(node.getChild(i)) &&
               comma_needed == false)
      {
        comma_needed = true;
        
        if (result[0] == null) {
          result[0] = node.getChild(i);
        }
        else {
          if (result[1] == null) {
            result[1] = node.getChild(i);
          }
          else {
            result[0] = result[1];
            result[1] = node.getChild(i);
          }
        }
      }
    }
    
    if (result[0] == null || result[1] == null)
      return null;
    else
      return result;
  }
  
  // this is where we figure out if something is an appositive
  static boolean isNPList(ParseNode node) {
    if (PropositionFinder.TRAINING)
      return node.isList();
	
    Vector children = _new Vector(node.getNChildren());
    for (int i = 0; i < node.getNChildren(); i++)
      children.addElement(node.getChild(i));

    Vector nps = _new Vector(4);
    int recent_nps_seen = 0;
    int commas_seen = 0;
    int ccs_seen = 0;
    for (int i = 0; i < children.size(); i++) {
      ParseNode child = (ParseNode) children.elementAt(i);
      if ((child.getTag() == STags::NP ||
           child.getTag() == STags::NPA ||
           child.getTag() == STags::NX ||
           child.getTag() == STags::NPP) &&
          !isNumber(child))
      {
        nps.addElement(child);
        recent_nps_seen++;
      }
      else if (child.getTag() == STags::COMMA)
      {
        recent_nps_seen = 0;
        commas_seen++;
      }
      else if (child.getTag() == STags::CC ||
               child.getTag() == STags::CONJP ||
               child.getTag() == STags::COLON) // allow colons too
      {
        recent_nps_seen = 0;
        ccs_seen++;
      }
/*      else {
        // if we encounter other type of node, it's not a list
        return false;
      }*/
      
      /*if (recent_nps_seen > 1)
        return false;*/
    }
    
    // do appositive check
    if (nps.size() == 2 && 
        (commas_seen == 1 || commas_seen == 2 || commas_seen == 3) &&
        ccs_seen == 0)
    {
      if (!NPClassifier.initialized)
        return false;
      
      // OK here's the really ugly part. we classify each side and 
      // make sure the types agree before calling it an appositive
      MentionInfo lhs_mention = _new MentionInfo();
      MentionInfo rhs_mention = _new MentionInfo();
      NPClassifier.populateMentionInfo((ParseNode) nps.elementAt(0), lhs_mention);
      NPClassifier.populateMentionInfo((ParseNode) nps.elementAt(1), rhs_mention);
      if (lhs_mention.getEDTType() == rhs_mention.getEDTType()) {
        return false;
      } else {
        
        // EMB 8/22/02
        // if the word can be found in the inventory with the matching type,
        //  coerce it.
        
        if (lhs_mention.mention_type == MentionInfo.NAME_MENTION &&
            rhs_mention.mention_type == MentionInfo.DESC_MENTION) {
          Symbol descword = ((ParseNode) nps.elementAt(1)).getHeadWord(); 
          Vector entries 
            = relations.DiverseRelDescFinder._lexicon.getNounEntries(descword);
          for (int i = 0; i < entries.size(); i++) {
            if (((NounEntry)entries.elementAt(i)).ref_type == lhs_mention.getEDTType()) {
              System.err.print("forcing desc (" + rhs_mention.getEDTType().toString() + ") --> name (");
              System.err.println(lhs_mention.getEDTType().toString() + ") in appositive: " + node.getText());
              return false;
            }
          }
        } else if (rhs_mention.mention_type == MentionInfo.NAME_MENTION &&
                   lhs_mention.mention_type == MentionInfo.DESC_MENTION) {
          Symbol descword = ((ParseNode) nps.elementAt(0)).getHeadWord(); 
          Vector entries 
            = relations.DiverseRelDescFinder._lexicon.getNounEntries(descword);
          for (int i = 0; i < entries.size(); i++) {
            if (((NounEntry)entries.elementAt(i)).ref_type == rhs_mention.getEDTType())
              System.err.print("forcing desc (" + lhs_mention.getEDTType().toString() + ") --> name (");
              System.err.println(rhs_mention.getEDTType().toString() + ") in appositive: " + node.getText());
              return false;
          }
              
        } 

      }
       
    }
    
    if (nps.size() > 1 && commas_seen + ccs_seen > 0)
      return true;
    else
      return false;
  }
  
  static boolean isVPList(ParseNode node) {
    int vps_seen = 0;
    int recent_vps_seen = 0;
    int separators_seen = 0;
    for (int i = 0; i < node.getNChildren(); i++) {
      Symbol label = node.getChild(i).getTag();
      if (label == STags::VP) {
        vps_seen++;
        recent_vps_seen++;
      }
      else if (label == STags::COMMA ||
               label == STags::CC ||
               label == STags::CONJP)
      {
        recent_vps_seen = 0;
        separators_seen++;
      }
      else {
        // if we encounter other type of node, it's not a list
        return false;
      }
      
      if (recent_vps_seen > 1)
        return false;
    }
    if (vps_seen > 1)
      return true;
    else
      return false;
  }
  
  static boolean isNumber(ParseNode node) {
    String s = node.getText();
    char chars[] = s.toCharArray();
    for (int i = 0; i < chars.length; i++) {
      if (chars[i] < '0' || chars[i] > '9') {
        return false;
      }
    }
    return true;
  }
  
  
  static ParseNode getHighestParentBelow(ParseNode node, ParseNode top) {
    if (node.getParent() == null) {
      return null;
    }
    else if (node.getParent() == top) {
      return node;
    }
    else {
      return getHighestParentBelow(node.getParent(), top);
    }
  }
  
  static boolean nodesContainAnyOf(Vector nodes, Symbol[] symbols) {
    Enumeration nodes_e = nodes.elements();
    while (nodes_e.hasMoreElements()) {
      ParseNode node = extractParseNode(nodes_e.nextElement());
      for (int i = 0; i < symbols.length; i++) {
        if (node.getTag() == symbols[i])
          return true;
      }
    }
    return false;
  }
  
  static ParseNode findNodeOfType(Vector nodes, Symbol[] symbols) {
    Enumeration nodes_e = nodes.elements();
    while (nodes_e.hasMoreElements()) {
      ParseNode node = extractParseNode(nodes_e.nextElement());
      for (int i = 0; i < symbols.length; i++) {
        if (node.getTag() == symbols[i])
          return node;
      }
    }
    return null;
  }
  
  static boolean isNPType(Symbol sym) {
    return (sym == STags::NP ||
            sym == STags::NPA ||
            sym == STags::NPP ||
            sym == STags::NX ||
            sym == STags::NPPRO);
  }
  
  static ParseNode extractParseNode(Object it) {
    if (it instanceof ParseNode)
      return (ParseNode) it;
    else if (it instanceof SemUnit)
      return ((SemUnit) it).getParseNode();
    else
      return null;
  }
}
#endif
