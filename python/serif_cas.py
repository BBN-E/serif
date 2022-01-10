#!/usr/bin/env python

# Copyright 2010 by BBN Technologies Corp.
# All Rights Reserved.


"""
Library for reading SerifCAS XML files.

Example use:

  >>> cas = SerifCAS('myfile.sgm.xml')
  >>> print cas.propositions()[0]
  <proposition>
    <setPredicate>
      <referenceArg>
        <NP, mention, OTH_Type, listMention, parentMention>
          ...
      <memberArg>
          ...

If run from the command line, it will use a short demo script at the
bottom to print out all the parses in the given SerifCAS XML file
(in treebank format).
"""

from xml.etree import ElementTree as ET
import textwrap, re, optparse, sys

class Annotatable(object):
    """
    A base class for objects that can participate in SerifCAS
    annotation graphs.  Annotatable has three subclasses:

      - `Sofa`: The 'subject of annotation'; i.e., a document
        that is annotated, including its text and information
        about the offests of its characters.
      - `Annotation`: A simple annotation.
      - `OffsetAnnotation`: An annotation that has a `start`
        and an `end`.

    Each annotatable is characterized by

      - A list of `target` that this annotatable points to.
        The list of targets is an ordered list, and may be
        empty (e.g., the sofa); may include a single target
        annotatable (e.g., a token); or may include multipe
        targets (e.g., a parse tree node).

      - A list of `features` that characterize this annotatable.
        Features (also known as annotation types) are used to
        identify what kind of annotation edge this is, and to
        provide additional information about it.
    """
    targets = None
    """A list of the targets that this annotation points to.  This will
       be `None` until Annotatable.link() is called."""

    features = None
    """A list of this annotation's features (aka annotation types).
       This will be `None` until Annotatable.link() is called."""

    def __new__(cls, etree):
        # Delegate to an appopriate subclass:
        if cls is Annotatable:
            if etree.get('type') == 'sofa':
                cls = Sofa
            elif etree.get('type') == 'offset':
                cls = OffsetAnnotation
            elif etree.get('type') == 'annotation':
                cls = Annotation
        return object.__new__(cls)

    def __init__(self, etree):
        self.id = int(etree.get('id'))
        self.kind = etree.get('type')
        self.fids = [fid.text for fid in etree.findall('FeatureSet/FID')]
        self.tids = [tid.text for tid in etree.findall('Targets/TID')]

    def link(self, id_to_feature, id_to_annotation):
        """
        Given two dictionaries, one mapping feature identifiers to
        features, and the other mapping target identifiers to
        annotations, initialize this annotation's `features` and
        `targets` attributes.
        """
        self.features = [id_to_feature[int(fid)] for fid in self.fids]
        self.targets = [id_to_annotation[int(tid)] for tid in self.tids]

    def has_feature(self, feature_name, include_ancestors=True):
        for feature in self.features:
            if feature.name == feature_name:
                return True
            if include_ancestors and feature.has_ancestor(feature_name):
                return True
        return False

    def __str__(self):
        return self.pprint()

    def __repr__(self):
        return self._pprint('')

    def pprint(self, prefix='', max_depth=10):
        """Display this annotation and its targets"""
        result = textwrap.fill(self._pprint(prefix),
                               initial_indent=prefix,
                               subsequent_indent=prefix+'      ')
        if self.targets is not None and max_depth>0:
            for target in self.targets:
                # Don't bother to explicitly mention the sofa.
                if target.has_feature('sofa'): continue
                result += '\n' + target.pprint(prefix+'  ', max_depth-1)
        return result

    def _pprint(self, prefix):
        """Display this annotation (not targets)"""
        features = self._sorted_features()
        return '<%s>' % (', '.join(f.name for f in features))

    def _sorted_features(self):
        """Helper for pprint"""
        return sorted(self.features or self.fids, key=self.__feature_sort_key)

    def __feature_sort_key(self, feature):
        """Helper for _sorted_features()"""
        if isinstance(feature, Feature):
            if feature.has_ancestor('parseNode'):
                return (0, feature.fid)
            else:
                return (1, feature.fid)
        else:
            return (-1, feature)

    def treebank_str(self):
        parse_node = None
        for feature in self.features:
            if feature.name == 'parse':
                parse_node = 'PARSE'
            elif feature.has_ancestor('parseNode'):
                parse_node = feature.name
            elif feature.name == 'token':
                return self.text
        if parse_node:
            children = [c.treebank_str() for c in self.targets]
            children = [c for c in children if c is not None]
            if parse_node == 'PARSE' and len(children) == 1:
                return children[0]
            else:
                child_str = ' '.join(c for c in children)
                return '(%s %s)' % (parse_node, child_str)
        else:
            return None

class Annotation(Annotatable):
    """
    A simple annotation object.
    """

class OffsetAnnotation(Annotatable):
    """
    An annotation object with a start offset and an end offset.
    """
    def __init__(self, etree):
        Annotatable.__init__(self, etree)
        self.start = int(etree.get('offsetstart'))
        self.end = int(etree.get('offsetend'))

    @property
    def text(self):
        sofas = [ann for ann in self.targets if ann.has_feature('sofa')]
        if len(sofas) != 1:
            return Annotatable._pprint(self)
        start = sofas[0].offsets[self.start]
        end = sofas[0].offsets[self.end]
        return sofas[0].original_text[start:end+1]

    def _pprint(self, prefix=''):
        sofas = [ann for ann in self.targets if ann.has_feature('sofa')]
        if len(sofas) != 1:
            return Annotatable._pprint(self, prefix)
        start = sofas[0].offsets[self.start]
        end = sofas[0].offsets[self.end]
        text = sofas[0].original_text[start:end+1]
        features = self._sorted_features()
        return '<%s> [%s:%s] %r' % (', '.join(f.name for f in features),
                                    start, end, text)

class Sofa(Annotatable):
    """
    A subject of annotation (aka a base text).
    """
    def __init__(self, etree):
        Annotatable.__init__(self, etree)
        self.original_text = etree.find('SofaAnnotation/OriginalText').text
        self.sofa_text = etree.find('SofaAnnotation/SofaText').text

        # Dictionary mapping from the offsets used in the SerifCAS xml
        # to the 'real' offsets in the original text file:
        offsets = etree.find('SofaAnnotation/OriginalOffsets')
        self.offsets = dict(zip(
            [int(e.text) for e in offsets.findall('OrigEDTSt')],
            [int(e.text) for e in offsets.findall('OriginalOffset')],
            ))

class Feature(object):
    """
    A 'feature' (aka an 'annotation type') of an annotation.  Features
    identify types for annotation edge, and provide additional
    information about them.  Features form a hierarchy, with each
    feature optionally having a parent.
    """
    parent = None

    def __init__(self, etree):
        self.fid = int(etree.get('id'))
        self.name = etree.find('AnnTypeName').text
        self.parent_fid = etree.find('AnnTypeParentID').text
        if self.parent_fid is not None:
            self.parent_fid = int(self.parent_fid)

    def link(self, id_to_feature):
        if self.parent_fid:
            self.parent = id_to_feature[self.parent_fid]

    def has_ancestor(self, feature_name):
        return ( (self.parent is not None) and
                 ( (self.parent.name == feature_name) or
                   self.parent.has_ancestor(feature_name) ) )

class SerifCAS(object):
    def __init__(self, filename):
        self.filename = filename
        root = ET.parse(filename).getroot()

        # Read information about features.
        self.features = [Feature(e) for e in
                         root.findall('AnnotationTypes/AnnotationType')]
        self.id_to_feature = dict( (e.fid, e) for e in self.features)
        self.name_to_feature = dict( (e.name, e) for e in self.features)

        # Read annotations.
        self.annotations = [Annotatable(e) for e in
                            root.findall('Annotatables/Dimension/Annotatable')]
        self.id_to_annotation = dict( (e.id, e) for e in self.annotations)

        # Link everything together.
        for ann in self.annotations:
            ann.link(self.id_to_feature, self.id_to_annotation)
        for feature in self.features:
            feature.link(self.id_to_feature)

    def sofas(self):
        return [a for a in self.annotations if a.has_feature('sofa')]

    def sofa(self):
        sofas = self.sofas()
        if len(sofas) != 1:
            raise ValueError('%s has %d sofas' % (self.filename, len(sofas)))
        return sofas[0]

    def sentences(self):
        return [a for a in self.annotations if a.has_feature('sentence')]

    def sentence_theories(self):
        return [a for a in self.annotations if a.has_feature('sentenceTheory')]

    def propositions(self):
        return [a for a in self.annotations if a.has_feature('proposition')]

    def parses(self):
        return [a for a in self.annotations if a.has_feature('parse')]


###########################################################################
# Demo Script
###########################################################################

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print "Usage: %s FILE.xml" % sys.argv[0]
        sys.exit(-1)
    cas = SerifCAS(sys.argv[1])
    for parse in cas.parses():
        print parse.treebank_str()
    #for prop in cas.propositions():
    #    print prop
