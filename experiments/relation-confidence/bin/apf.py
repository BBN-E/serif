"""
Classes for reading APF xml files.  This does not provide a complete
interface; it was only implemented to pull out the information that
we need for LearnIt evaluation.  (But it could be extended.)
"""

"""
NOTES:...

1. Read the apf files, and generate a list of seeds.

2. Generate APF output.  Use serif's notion of entities and mentions.
   And use patterns to propose relation mentions.  This part is
   probably best to do in C++.
   
"""

import os, sys, re
from xml.etree import ElementTree as ET
from google_translate import translate
from collections import defaultdict

######################################################################
# Data Classes
######################################################################

#//////////////////////////
# Base classes
#//////////////////////////
class APFObject(object):
    """
    Base class for APF objects.
    """
    def __init__(self, xml, ref_dict):
        """
        @param ref_dict: A dictionary mapping from identifiers to
            APFObjects.  Each APFObject should register itself in
            this reference dictionary when it is created.
        """
        self.id = xml.get('ID')
        if self.id in ref_dict:
            raise ValueError("Duplicate ID: %s" % self.id)
        ref_dict[self.id] = self

class Mention(APFObject):
    def __init__(self, xml, ref_dict, language):
        APFObject.__init__(self, xml, ref_dict)
        charseq_xml = xml.find('extent/charseq')
        if charseq_xml is None:
            self.start_offset = -1
            self.end_offset = -1
            self.original_text = 'N/A'
        else:
            self.start_offset = int(charseq_xml.get('START'))
            self.end_offset = int(charseq_xml.get('END'))
            self.original_text = ' '.join(charseq_xml.text.split())
        self.text = {language: self.original_text}

#//////////////////////////
# Relations
#//////////////////////////
class Relation(APFObject):
    """
    An ACE relation.  A relation occurs inside a specified document
    (identified by `rel.docid`), and has an identifier (`rel.id`).
    It also has a relation type (`rel.rel_type`), subtype
    (`rel.rel_subtype`), a dictionary of args (`rel.args`) indexed
    by argument name (eg 'ARG-1'), and a list of relation mentions
    (`rel.relmentions`).
    """
    def __init__(self, docid, source_file, relation_xml, ref_dict, language):
        """
        @param docid: The docid of the document containing this relation.
        @param relation_xml: The ElementTree DOM object for the XML
            describing this relation.
        @param language: The language used to express this relation.
        """
        self.docid = docid
        self.source_file = source_file
        APFObject.__init__(self, relation_xml, ref_dict)
        self.rel_type = relation_xml.get('TYPE').lower()
        self.rel_subtype = relation_xml.get('SUBTYPE', '').lower()
        self.args = {}
        for arg_xml in relation_xml.findall('relation_argument'):
            role = arg_xml.get('ROLE')
            self.args[role] = ref_dict[arg_xml.get('REFID')]
        # Read the relation mentions.
        self.relmentions = [RelMention(self, relmention_xml, ref_dict, language)
                            for relmention_xml in
                            relation_xml.findall('relation_mention')]
        
    arg1 = property(lambda self: self.args['Arg-1'])
    arg2 = property(lambda self: self.args['Arg-2'])

class RelMention(Mention):
    """
    A class used to encode the information about a single relation
    mention from an ACE apf.xml file.
    """
    def __init__(self, relation, relmention_xml, ref_dict, language):
        self.relation = relation
        Mention.__init__(self, relmention_xml, ref_dict, language)
        # Arguments:
        self.args = {}
        for arg_xml in relmention_xml.findall('relation_mention_argument'):
            role = arg_xml.get('ROLE')
            self.args[role] = ref_dict[arg_xml.get('REFID')]

        #: The index of the sentence containing this RelMention.  This
        #: is set later, if --segment-spans is used.
        self.sent_num = -1

    # This is used in the generated relmentions.xml file, which is
    # used for evaluation purposes.
    _TOXML = ('<relmention docid="%(docid)s" sentnum="%(sent_num)s" '
              'type="%(rel_type)s" subtype="%(rel_subtype)s"  />')
    def toxml(self):
        return self._TOXML % dict(
            docid=self.relation.docid,
            sent_num=self.sent_num,
            rel_type=self.relation.rel_type,
            rel_subtype=self.relation.rel_subtype)

    arg1 = property(lambda self: self.args['Arg-1'])
    arg2 = property(lambda self: self.args['Arg-2'])
    docid = property(lambda self: self.relation.docid)

# class RelMentionArgument(Mention):
#     """
#     A class used to encode information about a single relation mention
#     argument from an ACE apf.xml file
#     """
#     def __init__(self, relarg_xml, ref_dict, language):
#         self.role = relarg_xml.get('ROLE')
#         self.mention_id = relarg_xml.get('REFID')
#         self.mention = mentions.get(self.mention_id)
#         if self.mention_id and not self.mention:
#             raise ValueError('Mention not found: %s' % self.mention_id)
#         charseq_xml = relarg_xml.find('extent/charseq')
#         if charseq_xml:
#             self.start_offset = int(charseq_xml.get('START'))
#             self.end_offset = int(charseq_xml.get('END'))
#             self.original_text = ' '.join(charseq_xml.text.split())
#         elif self.mention:
#             self.start_offset = self.mention.start_offset
#             self.end_offset = self.mention.end_offset
#             self.original_text = self.mention.original_text
#         else:
#             self.start_offset = -1
#             self.end_offset = -1
#             self.original_text = None
#            
#         self.text = {language: self.original_text}

#//////////////////////////
# Entities
#//////////////////////////
class Entity(APFObject):
    def __init__(self, docid, source_file, entity_xml, ref_dict, language):
        self.docid = docid
        self.source_file = source_file
        APFObject.__init__(self, entity_xml, ref_dict)
        self.entity_type = entity_xml.get('TYPE')
        mentions = [EntityMention(mention_xml, ref_dict, language)
                    for mention_xml in entity_xml.findall('entity_mention')]
        self.mentions = dict((mention.id, mention)
                             for mention in mentions)

    @property
    def text(self):
        return self.best_mention.text

    @property
    def best_mention(self):
        #print 'MS', [self._score_mention(m) for m in self.mentions.values()]
        #print 'MS', sorted([(self._score_mention(v), v.text['en'])
        #                    for v in self.mentions.values()],
        #                   reverse=True)
        return sorted(self.mentions.values(), 
                      key=self.score_mention)[-1]

    def score_mention(self, mention):
        score = 0
        if mention.mention_type == 'nam': score += 10
        elif mention.mention_type == 'nom': score += 5
        num_words = len(mention.original_text.split())
        if mention.mention_type == 'nam':
            score += {3:5, 2:4, 4:3, 5:2, 6:1}.get(num_words, 0)
        if num_words > 8: score -= 1
        if mention.mention_subtype == 'individual': score += 2
        return score

class EntityMention(Mention): 
    def __init__(self, mention_xml, ref_dict, language):
        Mention.__init__(self, mention_xml, ref_dict, language)
        self.mention_type = mention_xml.get('TYPE', '').lower()
        self.mention_subtype = mention_xml.get('SUBTYPE', '').lower()
        
#//////////////////////////
# Time Expressions
#//////////////////////////
class Timex2(APFObject):
    def __init__(self, docid, source_file, timex2_xml, ref_dict, language):
        self.docid = docid
        self.source_file = source_file
        APFObject.__init__(self, timex2_xml, ref_dict)
        self.timex2_type = timex2_xml.get('TYPE')
        mentions = [Timex2Mention(mention_xml, ref_dict, language)
                    for mention_xml in timex2_xml.findall('timex2_mention')]
        self.mentions = dict((mention.id, mention)
                             for mention in mentions)

class Timex2Mention(Mention):
    pass
        

######################################################################
# XML Parsing
######################################################################

def read_apf_relations(paths, language='en'):
    """
    Read all '.apf.xml' or '.apf' files in the specified directory;
    and return a dictionary containing all relations in those files.
    This dictionary maps from relation id's to `Relation` objects.
    """
    return read_apf(paths, Relation, language)
    
def read_apf(paths, selected_types=(Relation, Entity, Timex2),
             language='en'):
    """
    Read all '.apf.xml' or '.apf' files in the specified directory;
    and return a dictionary containing all relations in those files.
    This dictionary maps from relation id's to `Relation` objects.
    """
    if isinstance(paths, basestring):
        paths = [paths]
        
    ref_dict = {}
    errors = []
    for path in paths:
        if os.path.isdir(path):
            walk_iter = os.walk(path)
        else:
            root, filename = os.path.split(path)
            walk_iter = [(root, (), [filename])]
        for root, dirs, files in walk_iter:
            apf_files = [f for f in files if
                         f.endswith('.apf') or f.endswith('.apf.xml')]
            if apf_files:
                print root
                         
            for filename in apf_files:
                # Get the entities
                try:
                    file_ref_dict = read_apf_file(os.path.join(root, filename),
                                                  language)
                    ref_dict.update((k,v) for (k,v) in file_ref_dict.items()
                                    if isinstance(v, selected_types))
                except Exception, e:
                    print '  %s Error in %s:\n    * %s' % (
                        type(e).__name__, filename, e)
                    errors.append((filename, e))
                print '[%6d] %s' % (len(ref_dict), filename)
    if errors:
        print '%d ERRORS!' % len(errors)

    return ref_dict

def read_apf_file(filename, language='en'):
    apf_xml = ET.parse(filename)
    docid = apf_xml.find('document').get('DOCID')
    if docid.endswith('.sgm'):
        print '    Warning: Stripping ".sgm" from docid!'
        docid = docid[:-4]
    ref_dict = {}
    
    # Find all entities & mentions first.
    for child in apf_xml.find('document'):
        if child.tag == 'entity':
            Entity(docid, filename, child, ref_dict, language)
        elif child.tag == 'relation':
            Relation(docid, filename, child, ref_dict, language)
        elif child.tag == 'timex2':
            Timex2(docid, filename, child, ref_dict, language)

    return ref_dict

######################################################################
# Translation
######################################################################

def translate_relations(relations, source_language, target_language,
                        verbose=True):
    if verbose:
        print "Translating from %s -> %s" % (source_language, target_language)
    mentions = set(mention for relation in relations.values()
                           for entity in relation.args.values()
                           for mention in entity.mentions.values())
    translate_mentions(mentions, source_language, target_language)
        
def translate_mentions(entity_mentions, source_language, target_language):
    s = '\n'.join('%s' % m.text[source_language]
                  for m in entity_mentions)
    t = translate(s,
                  source_language=source_language,
                  target_language=target_language)
    translations = t.split('\n')

    # reverse so we can pop them off one at a time.
    translations = list(reversed(translations))
    for mention in entity_mentions:
        mention.text[target_language] = translations.pop()
    assert len(translations) == 0

######################################################################
# Relation Entity Type Detection
######################################################################

def find_relation_entity_types(relations, verbose=True):
    """
    Determine the set of permissible entity types for each relation
    type.  This is done by scanning through a given set of
    relations, and checking which entity types occur for each
    relation type's arguments.  Any entity type that occurs for less
    than 1% of a given relation type argument will be discounted.
    Return a dictionary mapping from rel_type->arg_name->entity_types,
    where entity_types is a list of entity type names.
    """
    counts = defaultdict(lambda: defaultdict(lambda: defaultdict(int)))
    entity_types = set()
    for relation in relations.values():
        rel_type = relation.rel_type
        for argname, arg in relation.args.items():
            if argname.startswith('Time'): continue
            counts[rel_type][argname][arg.entity_type] += 1
            entity_types.add(arg.entity_type)
    entity_types = sorted(entity_types)

    if verbose:
        print '%26s' % '',
        for entity_type in entity_types:
            print '%4s' % entity_type,
        print
        for rel_type in counts:
            print '-'*75
            for i, argname in enumerate(sorted(counts[rel_type])):
                if i == 0: 
                    print '%-20s %s' % (rel_type[:20], argname),
                else:
                    print '%-20s %s' % ('', argname),
                total = sum(counts[rel_type][argname].values())
                for entity_type in entity_types:
                    n = counts[rel_type][argname][entity_type]
                    if n == 0: n = ''
                    elif n < total/100: n = '.' # a little bit
                    print '%4s' % n,
                print
                
    result = {}
    for rel_type in counts:
        result[rel_type] = {}
        for i, argname in enumerate(sorted(counts[rel_type])):
            result[rel_type][argname] = set()
            total = sum(counts[rel_type][argname].values())
            for entity_type in entity_types:
                n = counts[rel_type][argname][entity_type]
                if n > 0 and n >= total/100:
                    result[rel_type][argname].add(entity_type)
    return result
