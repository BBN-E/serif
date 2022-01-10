#!/opt/Python-2.5.4-x86_64/bin/python
"""
Searches the English regression test data for instances of ACE
relations and prints examples of the syntactic nodes expressing those
relations and their right- and left-hand sides.  For exploratory
purposes when designing patterns to capture relations.
"""

ROOT = '/nfs/raid58/u14/serif/testdata/regtest/130329/x86_64/NoThread/Classic/Release/English/Best/'

EXAMPLES = 3

import os, sys, re, collections, random
try: HERE=os.path.split('__file__')[0]
except: HERE = '.'
sys.path.append(os.path.join(HERE, '../python'))
import serifxml

def common_ancestor(n1, n2):
    node = n1
    while ((not (node.start_char <= n2.start_char <=
               n2.end_char <= node.end_char)) or
           (node.parent and len(node.parent)==1)):
        node = node.parent
    return node

def syn_path(n1, n2):
    # Find path from n1 to common ancestor
    up_path = '/%s' % n1.tag
    node = n1
    while ((not (node.start_char <= n2.start_char <=
                 n2.end_char <= node.end_char)) or
           (node.parent and len(node.parent)==1)):
        node = node.parent
        up_path = '%s/%s' % (up_path, node.tag)
    ancestor = node
    # Find path from n2 to common ancestor
    node = n2
    down_path = ''
    while ((not (node.start_char <= n1.start_char <=
                 n1.end_char <= node.end_char)) or
           (node.parent and len(node.parent)==1)):
        down_path = '%s\\%s' % (node.tag, down_path)
        node = node.parent
    if ancestor is not node:
        print 'huh'
        print n1
        print n2
        print node
        print ancestor
    assert ancestor is node
    return up_path + '\\' + down_path

class RelationInfo(object):
    def __init__(self, rm):
        self.type = rm.type
        self.lhs_etype = rm.left_mention.entity_type
        self.rhs_etype = rm.right_mention.entity_type
        self.syn_path = syn_path(rm.left_mention.syn_node,
                                 rm.right_mention.syn_node)

    def key(self):
        return tuple(sorted(self.__dict__.items()))

    def __cmp__(self, other):
        return cmp(self.key(), other.key())

    def __hash__(self):
        return hash(self.key())

    def __str__(self):
        return '%s(%s, %s) %s' % (
            self.type, self.lhs_etype, self.rhs_etype,
            self.syn_path)

def get_relations():
    rms = []
    for directory, subdirs, files in os.walk(ROOT):
        for f in files:
            if f.endswith('.xml'):
                print len(rms), f
                doc = serifxml.Document(os.path.join(directory, f))
                for sent in doc.sentences:
                    if sent.rel_mention_set:
                        sent.save_text
                        for rm in sent.rel_mention_set:
                            rm._sent = sent # prevent GC
                        rms.extend(sent.rel_mention_set)
    return rms

############################
# Copied from eval_coref
############################

def get_ancestors(n):
    """includes n itself"""
    ancestors = [n]
    while n.parent:
        n = n.parent
        ancestors.append(n)
    return ancestors

def get_linkpath(node1, node2):
    assert node1 is not node2
    n1_ancestors = get_ancestors(node1)
    n2_ancestors = get_ancestors(node2)
    common_ancestor = None
    while (len(n1_ancestors)>0 and len(n2_ancestors)>0 and
           n1_ancestors[-1] == n2_ancestors[-1]):
        common_ancestor = n1_ancestors.pop()
        n2_ancestors.pop()
    included_nodes = set(n1_ancestors[1:]+n2_ancestors[1:]+
                         [common_ancestor])
    linkpath = trim_linkpath(common_ancestor, included_nodes,
                             set([node1, node2]))
    return linkpath

def trim_linkpath(node, included, marked, include_all=False):
    if node in marked:
        s = u"(**%s**" % node.tag
    else:
        s = u"(%s" % node.tag
    s = s.encode('ascii', 'xmlcharrefreplace')
    if (node in included) or include_all:
        for child in node:
            if (child in included) or (child in marked) or include_all:
                s += " %s" % trim_linkpath(child, included,
                                           marked, include_all)
            elif child.tag=='GRUP.VERB':
                s += " %s" % trim_linkpath(child, included, marked, True)
            else:
                s += " %s" % child.tag
    return s+")"


def main(rms):
    rels = collections.defaultdict(list)
    for rm in rms:
        relinfo = RelationInfo(rm)
        rels[relinfo].append(rm)

    for relinfo, rms in sorted(rels.items(), key=lambda kv:len(kv[1])):
        print '%10d %s' % (len(rms), relinfo)
        random.shuffle(rms)
        lps = collections.defaultdict(int)
        for rm in rms:
            lps[get_linkpath(rm.left_mention.syn_node,
                             rm.right_mention.syn_node)] += 1
        for lp, n in sorted(lps.items(), key=lambda kv:kv[1])[-3:]:
            print '%15s %s' % (n, lp)
        if EXAMPLES:
            for rm in rms[-EXAMPLES:]:
                ca = common_ancestor(rm.left_mention.syn_node,
                                     rm.right_mention.syn_node)
                print ' '.join(str(ca).split())


if __name__ == '__main__':
    try: rms
    except NameError: rms = get_relations()
    main(rms)
