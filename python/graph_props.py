#!/usr/bin/env python
###
# graph_props.py
#
# Copyright 2011 by BBN Technologies.
# All Rights Reserved.
#
# Draws a graphviz visualization of the proposition structure and ace relations
# in serifxml documents. Requires graphviz and pygraph.
#
# Legend:
#    Diamond: proposition
#        blue: vprop
#        gray: nprop
#        green: other
#    Circle: mention or entity (dependent on mention_tying argument)
#        light orange: PER
#        orange: ORG
#        purple: LOC or GPE
#        white: other
#    Red dashed line: ace relation
#    Blue dotted line: coreferent mention chain (only if mention_tying is false)
#
# Michael Shafir
# mshafir@bbn.com
# 2011.7.12
###

import os, sys

# Import pygraph
from pygraph.classes.graph import graph
from pygraph.classes.digraph import digraph
from pygraph.algorithms.searching import breadth_first_search
from pygraph.readwrite.dot import write

import serifxml

class GraphTracker:
   def __init__(self,directed=False):
      if directed:
         self.graph = digraph()
      else:
         self.graph = graph()
      self.nodes = set()
      self.edges = set()

   def add_node(self,name,lbl,attrs,ptype=None):
      attrs.append(('label',lbl))
      if name not in self.nodes:
         if not ptype is None:
            attrs += get_prop_attrs(ptype)
         self.graph.add_node(name,attrs)
         self.nodes.add(name)
      return name

   def add_edge(self,a,b,label,attrs):
      if not (a,b) in self.edges:
           self.graph.add_edge((a,b),wt=1,label=label,
                       attrs=attrs)
           self.edges.add((a,b))

   def write(self,filename):
      dot = write(self.graph)
      f = open(filename,'w')
      f.write(dot)
      f.close()

cid = 0
def new_id():
   global cid
   cid+=1
   return 'o'+str(cid)

def get_entity(serif_doc,mention,mention_tying):
    for e in serif_doc.entity_set:
        if mention in e.mentions:
            if mention_tying:
               return [e.id +'-'+ e.mentions[0].text,e.mentions[0].text]
            else:
               return [e.id+'-'+str(e.mentions.index(mention))+ \
                       '-'+mention.text,mention.text]
    return [new_id() + '-' + mention.text,mention.text]

def get_mention(sent,m_id):
    for m in sent.mention_set:
        if m.id == m_id:
            return m
    return None

def get_mention_attrs(mention):
    ret = [('style','filled')]
    if mention.entity_type == 'PER':
        ret.append(('fillcolor','burlywood1'))
    elif mention.entity_type == 'ORG':
        ret.append(('fillcolor','orange2'))
    elif mention.entity_type == 'LOC' or mention.entity_type == 'GPE':
        ret.append(('fillcolor','mediumpurple'))
    else:
        ret.append(('fillcolor','white'))
    return ret

def get_prop_attrs(ptype):
    ret = [('shape','diamond'),('style','filled')]
    if ptype == ptype.noun:
        ret.append(('fillcolor','gray'))
    elif ptype == ptype.verb:
        ret.append(('fillcolor','lightblue'))
    else:
        ret.append(('fillcolor','darkseagreen1'))
    return ret

def add_props(serif_doc,graph,mention_tying,exclude_none = True):
   for s in serif_doc.sentences:
       for p in s.proposition_set:
           if p.head is None:
               node = new_id()
               label = 'None'
           else:
               node = p.id + '-' + p.head.headword
               label = p.head.headword
           if label != 'None' or not exclude_none:
               graph.add_node(node,label,[],ptype=p.pred_type)
               for a in p.arguments:
                  try:
                      if a.mention is None:
                          if type(a.value) == serifxml.Proposition:
                              name = graph.add_node(a.value.id+'-'+a.value.head.headword,
                                              a.value.head.headword,[],
                                              ptype=a.value.pred_type)
                          else:
                              name = graph.add_node(a.value.headword,a.value.headword,[])
                      else:
                          ment = get_mention(s,a.mention.id)
                          [name,label] = get_entity(serif_doc,ment,mention_tying)
                          attrs = get_mention_attrs(ment)
                          name = graph.add_node(name,label,attrs)
                      graph.add_edge(node,name,a.role,[])
                  except:
                     print 'failed adding argument, prop:'+node

def make_entity_node(serif_doc,graph,entity,mention_tying):
   ment = entity.mentions[0]
   attrs = get_mention_attrs(ment)
   [name,label] = get_entity(serif_doc,ment,mention_tying)
   return graph.add_node(name,label,attrs)

def add_relations(serif_doc,graph,mention_tying):
   for rs in serif_doc.relation_set:
      n1 = make_entity_node(serif_doc,graph,rs.left_entity,mention_tying)
      n2 = make_entity_node(serif_doc,graph,rs.right_entity,mention_tying)
      graph.add_edge(n1,n2,rs.relation_type,
               [('style','dashed'),('color','red')])

def add_entity_ties(serif_doc,graph):
   for es in serif_doc.entity_set:
      last_lbl = es.mentions[-1].text
      last_name = es.id+'-'+str(len(es.mentions)-1)+'-'+last_lbl
      prev_node = graph.add_node(last_name,last_lbl,[])
      for i in range(len(es.mentions)):
          lbl = es.mentions[i].text
          name = es.id+'-'+str(i)+'-'+lbl
          cur_node = graph.add_node(name,lbl,[])
          graph.add_edge(prev_node,cur_node,'',[('color','blue'),
                                                ('dir','none'),
                                                ('style','dotted'),
                                                ('penwidth',2)])
          prev_node = cur_node

def handle_doc(serif_doc,outfilename,directed,mention_tying):
    gr = GraphTracker(directed)
    add_props(serif_doc,gr,mention_tying)
    add_relations(serif_doc,gr,mention_tying)
    if not mention_tying:
       add_entity_ties(serif_doc,gr)
    gr.write(outfilename+'.dot')
    os.system('dot -Tsvg -Goverlap=false -o'+outfilename+'.svg '+outfilename+'.dot')

def handle_file(filename,outdir,directed,mention_tying):
    serif_doc = serifxml.Document(filename)
    handle_doc(serif_doc,outdir+'/'+os.path.basename(filename),directed,mention_tying)

def do_graph(input_path,outdir,directed,mention_tying):
    if not os.path.exists(outdir):
        os.makedirs(outdir)
    if os.path.isfile(input_path) and input_path.endswith(".xml"):
        print 'reading '+input_path+'...'
        handle_doc(input_path,outdir,directed,mention_tying)
    elif os.path.isdir(input_path):
        print 'reading '+input_path+'...'
        for filename in sorted(os.listdir(input_path)):
            do_graph(input_path+filename,outdir,directed,mention_tying)
    else:
        print 'skipping '+input_path

def main():
    # Get input files
    usage = """usage: graph_props.py <serifxml file/dir> <output dir>
                        <directed True/False> <mention tying True/False>"""
    if len(sys.argv) != 5:
        sys.stderr.write(usage + "\n")
        sys.exit(2)
    # args
    directed = (sys.argv[3] == 'True')
    mention_tying = (sys.argv[4] == 'True')
    outdir = sys.argv[2]
    input_path = sys.argv[1]
    do_graph(input_path,outdir,directed,mention_tying)

# Do not run on import
if __name__ == "__main__":
    # Call main function
    main()
