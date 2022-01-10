#!/bin/env python
#
# Copyright 2013 by Raytheon BBN Technologies Corp.
# All Rights Reserved.

from serifxml import ActorMention, Document, Entity, EventMention, EventMentionArg, Genericity, ICEWSEventMention, Mention, MentionType, Polarity, PredType, PropStatus, RelMention, Sentence, Tense
from xml.sax.saxutils import escape as escape_xml
import hashlib, os, platform, re, shutil, stat, subprocess, sys, textwrap, time
from collections import defaultdict

VERBOSE = False

PRUNE_PROP_TREES = True

USE_ORIGINAL_TEXT_FOR_TOKENS = False

# If false, then prune out any entities that aren't connected via
# an edge to anything else.
INCLUDE_DISCONNECTED_ENTITIES_IN_MERGED_GRAPH = True

FORCE_REDRAW_DOT = True

BLANK_PAGE = ''
GUILLEMETS = u'\xab%s\xbb'
MINUS='&#8722;'

# This is just used for debugging; it should be left false:
SKIP_SENTENCE_PAGES = False

TAGSET = 'penn-treebank'
#TAGSET = 'ancora'

###########################################################################
## Graphviz Configuration
###########################################################################

PROP_EDGE_STYLE = {
    '<ref>': 'color="#880000",fontcolor="#880000"',
    '<sub>': 'color="#008800",fontcolor="#008800"',
    '<obj>': 'color="#000088",fontcolor="#000088"',
    '<head>': 'color="#000000:#000000"',
    }


DOT_GRAPH_OPTIONS = '''
ranksep="%(ranksep)s";
nodesep=".1";
rankdir="LR";
node [fontsize="10",tooltip=""];
edge [fontsize="10",tooltip=""];
graph [bgcolor=transparent];
'''

###########################################################################
## HTML Templates
###########################################################################

FRAMES_HTML = r'''
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Frameset//EN"
          "DTD/xhtml1-frameset.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
  <title> SerifXML Viewer </title>
</head>
<frameset cols="20%,80%">
  <frame src="nav.html" name="serifxmlDocNavFrame"
         id="serifxmlDocNavFrame" />
  <frameset rows="1*,1*,25">
    <frame src="blank_page.html" name="serifxmlTextFrame"
           id="serifxmlTextFrame"/>
    <frame src="blank_page.html" name="serifxmlDetailsFrame"
           id="serifxmlDetailsFrame"/>
    <frame src="blank_page.html" name="serifxmlNavFrame"
           id="serifxmlNavFrame" />
  </frameset>
</frameset>
</html>
'''.lstrip()

# Template for HTML pages.  Arguments: title, script, body
HTML_PAGE = '''\
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
   "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
  <meta http-equiv="content-type" content="text/html; charset=utf-8"/>
  <title>%(title)s</title>
  <link rel="stylesheet" href="serifxml_viewer.css" type="text/css" />
  <script type="text/javascript" src="jquery.js" >
  </script>
  <script type="text/javascript" src="jstree.js"></script>
  <script type="text/javascript" src="serifxml_viewer.js"></script>
  <script type="text/javascript">
    %(script)s
  </script>
</head>
<body class="%(css)s">
%(body)s
</body>
</html>
'''

# Body for the document navigation page.
DOC_NAV_BODY = '''
<h1>SerifXML Viewer</h1>
<div id="doc-nav">
<ul>
%s
</ul>
</div>'''

# Body for the sentence context page.
SENT_CONTEXT_BODY = '''
<a href="#" id="prev-sent">&larr;Prev (p)</a>
<span class="sentno">Sentence %(sent_no)s in
<span class="docid">%(docid)s</span></span>
<a href="#" id="next-sent">(n) Next&rarr;</a>
</div>
'''

# Script for the document text page.  Should these global/shared
# javascript variables be moved to the frameset itself?
DOCUMENT_TEXT_SCRIPT = '''
var selected_tab_id="parse";
var doc_url_prefix="%(doc_url_prefix)s";
$(document).ready(function() {setupTextFrame(); });
'''

# Pane for the sentence details page.
SENTENCE_DETAILS_PANE = '''
  <div id="%(tab_id)s-pane" class="nav-pane">
    <div class="%(tab_id)s">
%(body)s
    </div>
  </div>
'''

DOC_ENTITY_GRAPH = '''
  <div class="doc-entity-graph-wrapper">
    <a class="show-entity-graph" href="#">Show entity graph</a>
    <div class="doc-entity-graph collapsed">
%s
    </div>
  </div>
'''

ZOOMABLE_GRAPH = '''
<div class="zoomable-graph">
[<a href="#" class="graph-zoom-out">Zoom Out</a> |
<a href="#" class="graph-zoom-in">Zoom In</a>]
<div class="graph">
%s
</div>
</div>
'''

###########################################################################
## Parse -> HTML
###########################################################################
def parse_to_html(sentence, highlights={}):
    """
    @param highlights: A dictionary mapping SynNode->css class
    """
    parse = None
    score = float("-inf")
    for p in sentence.parses or []:
        if p.score > score:
            parse = p
            score = p.score
    if parse is None or parse.root is None:
        return '<i>No parse is available for this sentence</i>\n'
    s = '<div class="syntree">\n<ul><li>\n'
    s += syn_to_html(parse.root, highlights)
    s += '</li></ul>\n<div class="clear"></div>\n</div>\n'
    return s

def syn_to_html(syn_node, highlights, css_class='svn-root', tagset=None):
    if tagset is None:
        if syn_node.document.language == 'Spanish':
            tagset = 'ancora'
        else:
            tagset = 'penn-treebank'

    assert len(syn_node) != 0

    if tagset == 'ancora':
        if syn_node.tag in ('SP', 'P', 'p'): css_class = 'syn-p'
        elif syn_node.tag in ('SN', 'GRUP.NOM', 'n'): css_class = 'syn-n'
        elif syn_node.tag in ('GRUP.VERB', 'v'): css_class = 'syn-v'
        elif syn_node.tag in ('S', 'SENTENCE'): css_class = 'syn-s'
        elif syn_node.tag.startswith('S.F.'): css_class = 'syn-s'
        elif syn_node.tag in ('SADV', 'GRUP.ADV', 'r'): css_class = 'syn-adv'
        elif syn_node.tag in ('SA', 'S.A', 'GRUP.ADJ', 'a'): css_class = 'syn-adj'
        elif syn_node.tag in ('CONJ'): css_class='syn-conj'
    else:
        if syn_node.tag.startswith('PP'): css_class = 'syn-p'
        elif syn_node.tag.startswith('N'): css_class = 'syn-n'
        elif syn_node.tag.startswith('V'): css_class = 'syn-v'
        elif syn_node.tag.startswith('S'): css_class = 'syn-s'

    # Node information
    css = [css_class]
    if len(syn_node)==1 and len(syn_node[0])==0:
        css.append('preterminal')
    if syn_node.is_head:
        css.append('head')
    if syn_node in highlights:
        css.append(highlights[syn_node])
    css = css and (' class="%s"' % ' ' .join(css)) or ''
    s = '<h3 title="%s"%s>%s' % (syn_node.tag, css, syn_node.tag)

    mention = syn_node.mention
    if mention is not None: #and mention.mention_type.value != 'none':
        s += '<div class="info">'
        s += mention.mention_type.value
        if mention.entity_type != 'UNDET':
            s += ': '
            if len(syn_node.text) < 10:
                s += '<br/>'
            etype = mention.entity_type
            #if etype == 'OTH': etype = ''
            if mention.entity_subtype != 'UNDET':
                etype += '.' + mention.entity_subtype
            s += etype
        s += '</div>'
    s += '</h3>'

    # Children
    if len(syn_node)==1 and len(syn_node[0])==0:
        # n.b.: we use the token's text, not its tag.
        s += '<div class="word">%s</div>' % (syn_node[0].text,)
    else:
        s += '<ul>'
        for child in syn_node:
            child_html = syn_to_html(child, highlights, css_class, tagset)
            s += '<li>%s</li>' % (child_html,)
        s += '</ul>'
    return s

###########################################################################
## Graphviz/Dot: graph generation
###########################################################################

_VALID_DOT_ID_RE = re.compile(r'^[a-zA-Z][a-zA-Z0-9\-\_]*$')
def dot_id(v):
    id_attr = getattr(v, 'id', None)
    if id_attr is not None and _VALID_DOT_ID_RE.match(id_attr):
        return id_attr
    else:
        return 'n%s' % id(v)

def escape_dot(s, wrap=False, justify='c', quotes=False):
    # Choose what kind of newline to use (based on justify)
    if justify == 'c': newline = '\\n'
    elif justify == 'l': newline = newline='\\l'
    elif justify == 'r': newline = newline='\\r'
    else: assert 0, 'bad justify value!'
    # Add quotes if requested
    if quotes:
        s='"%s"' % s
    # Normalize whitespace
    s = ' '.join(s.split())
    # Wrap if requested
    if wrap:
        s = textwrap.fill(s, 25)
    # Escape characters that have special meaning
    s = re.sub(r'([\"<>\\|@{}])', r'\\\1', s)
    # Replace newlines
    s = s.replace('\n', newline)+newline
    # That's all!
    return s

def quote_dot(s, justify='l'):
    return escape_dot(s, True, justify, True)

def is_boring_prop(prop):
    if not PRUNE_PROP_TREES: return False
    # All verb props are interesting.
    if prop.pred_type in (PredType.verb, PredType.copula): #@UndefinedVariable
        return False
    # A proposition that doesn't connect anything is boring.
    if len(prop.arguments) == 1:
        return True
    # Name props are always boring
    if prop.pred_type == PredType.name: #@UndefinedVariable
        return True
    # Everything else is interesting
    return False

def prop_arg_dir(pred_type, arg_role):
    if arg_role == '<ref>' and pred_type != PredType.modifier: #@UndefinedVariable
        return 'back'
    if arg_role == '<sub>':
        return 'back'
    else:
        return 'forward'

def mention_port(mention):
    return 'm'+hashlib.sha224(mention.text.encode('utf-8')).hexdigest()

def entity_type(entity):
    etype = entity.entity_type
    if entity.entity_subtype != 'UNDET':
        etype += '.%s' % entity.entity_subtype
    return etype

def entity_dot_label(entity):
    etype = entity_type(entity)
    mention_texts = set((m.text, mention_port(m)) for m in entity.mentions)
    mentions = '|'.join('<%s>%s' % (port, quote_dot(text))
                        for (text, port) in mention_texts)
    return '%s|%s' % (etype, mentions)

def sent_no_for_theory(theory):
    sent = theory.owner_with_type(Sentence)
    if sent: return sent.sent_no
    if isinstance(theory, Entity):
        return min(m.owner_with_type(Sentence).sent_no
                   for m in theory.mentions)
    if isinstance(theory, ActorMention):
        return theory.mention.owner_with_type(Sentence).sent_no
    if isinstance(theory, ICEWSEventMention):
        return min(p.actor.mention.owner_with_type(Sentence).sent_no
                   for p in theory.participants)
    return None

class DotNode(object):
    width = '1'
    margin = '0.05,0.05'
    style = 'filled'
    outline = '#000000'
    fontsize = 'default'
    def tooltip(self): return ''
    def url(self):
        sent_no = sent_no_for_theory(self.value)
        if sent_no is None: return None
        doc = self.value.document
        return ('javascript:selectSentenceFromSentNo('
                '\\"sent-%s-%d\\", true);' % (doc.html_prefix, sent_no))
    def __init__(self, value): self.value = value
    def to_dot(self, include_href=False):
        href = ''
        if include_href:
            href = self.url() or ''
            if href: href = ', href="%s"' % href
        return '  %s [label="%s",fillcolor="%s",tooltip="%s"%s];\n' % (
            dot_id(self.value), self.label(), self.fill_color(),
            escape_dot(self.tooltip()), href)
    @staticmethod
    def nodes_to_dot(nodes, include_href):
        s = ''
        grouped_nodes = defaultdict(dict)
        for node in nodes:
            grouped_nodes[type(node)][node.value] = node
        for (typ, group) in grouped_nodes.items():
            if typ.fontsize == 'default': fontsize=''
            else: fontsize=',fontsize="%s"' % typ.fontsize
            s += ('{node [width="%s",height="0",margin="%s"'
                  'shape="record",style="%s",color="%s"%s];\n') % (
                typ.width, typ.margin, typ.style, typ.outline, fontsize)
            for node in group.values():
                s += node.to_dot(include_href)
            s += '}\n'
        return s

class SynDotNode(DotNode):
    outline = '#606060'
    def fill_color(self): return '#c0c0ff'
    def label(self):
        return '%s|%s' % (self.value.tag, quote_dot(self.value.text))
    def tooltip(self): return self.value.text
class EntityDotNode(DotNode):
    small = False
    width = property(lambda self:[DotNode.width,0][self.small])
    margin = property(lambda self:[DotNode.margin,'0,0.01'][self.small])
    outline = '#606060'
    def fill_color(self): return '#c0ffc0'
    def label(self):
        if self.small: return escape_dot(name_for_entity(self.value), wrap=True)
        else: return entity_dot_label(self.value)
    def tooltip(self): return name_for_entity(self.value)
    def url(self):
        if not self.small: return DotNode.url(self)
        return 'javascript:showDocEntity(\\"entity-%s\\");' % dot_id(self.value)
class FocusEntityDotNode(EntityDotNode):
    outline = '#000000'
    style='filled,bold'
    def fill_color(self): return '#a0ffa0'
class MentionDotNode(DotNode):
    small = False
    outline = '#606060'
    def fill_color(self): return '#c0ffc0'
    def label(self):
        if self.small: return escape_dot(self.value.text, wrap=True)
        else: return '%s|<%s>%s' % (entity_type(self.value),
                                    mention_port(self.value),
                                    quote_dot(self.value.text))
    def tooltip(self): return self.value.text
class ValMentionDotNode(DotNode):
    outline = '#606060'
    def fill_color(self): return '#c0c0c0'
    def label(self):
        return '%s|%s' % (self.value.value_type, quote_dot(self.value.text))
    def tooltip(self): return self.value.text
class PropDotNode(DotNode):
    style='rounded,filled'
    margin='0.05,0.02'
    def fill_color(self):
        return {
            'verb':     '#ffff00', 'set':      '#c0c0ff',
            'copula':   '#ffff00', 'comp':     '#c0c0ff',
            'modifier': '#c0c000', 'noun':     '#ffc0c0',
            'poss':     '#c0c000', 'pronoun':  '#ffc0c0',
            'loc':      '#c0c000', 'name':     '#ffc0c0',
            }.get(self.value.pred_type.value, '#ffff80')
    def label(self):
        prop = self.value
        label = prop.pred_type.value
        if hasattr(prop, 'status'):
            statuses = [prop.status]
        else:
            statuses = prop.statuses or ()
        for status in statuses:
            if status is not None and status != PropStatus.Default: #@UndefinedVariable
                label += '|%s' % status.value
        if prop.head:
            label += '|%s' % quote_dot(prop.head.text, justify='c')
        return label
    def tooltip(self):
        if self.value.head: return self.value.head.text
        else: return self.value.pred_type.value
class RelDotNode(DotNode):
    style='rounded,filled'
    margin='0.15,0.02'
    def fill_color(self): return '#ffff00'
    def label(self):
        rel = self.value
        if isinstance(rel, RelMention):
            reltype = rel.type
        else:
            reltype = rel.relation_type
        label = escape_dot('Relation %s' % reltype)
        if isinstance(rel, RelMention) and rel.raw_type:
            label += ' (%s)' % rel.raw_type
        if rel.tense and rel.tense != Tense.Unspecified: #@UndefinedVariable
            label += '|tense: %s' % rel.tense.value
        if rel.modality:
            label += '|modality: %s' % rel.modality.value
        if isinstance(rel, RelMention) and rel.time_arg:
            label += '|'
            if rel.time_arg_role:
                label += 'time (%s): ' % rel.time_arg_role
            else:
                label += 'time: '
            label += quote_dot(rel.time_arg.text, justify='c')
        return label
    def tooltip(self):
        if isinstance(self.value, RelMention): return self.value.type
        else: return self.value.relation_type
class EventDotNode(DotNode):
    style='rounded,filled'
    margin='0.15,0.02'
    #outline = '#000000:#000000'
    def fill_color(self): return '#ffff00'
    def label(self):
        event = self.value
        label = escape_dot('Event %s' % event.event_type)
        if event.tense and event.tense != Tense.Unspecified: #@UndefinedVariable
            label += '|tense: %s' % event.tense.value
        if event.modality:
            label += '|modality: %s' % event.modality.value
        if event.genericity and event.genericity != Genericity.Specific: #@UndefinedVariable
            label += '|genericity: %s' % event.genericity.value
        if event.polarity and event.polarity != Polarity.Negative: #@UndefinedVariable
            label += '|polarity: %s' % event.polarity.value
        return label
    def tooltip(self):
        return self.value.event_type
class ICEWSEventMentionDotNode(DotNode):
    style='rounded,filled'
    margin='0.15,0.02'
    def fill_color(self): return '#ffff00'
    def label(self):
        return 'Event %s|%s' % (escape_dot(self.value.event_code),
                                escape_dot(self.value.pattern_id))
    def tooltip(self):
        return self.value.event_code
class ICEWSActorMentionDotNode(DotNode):
    outline = '#606060'
    def fill_color(self): return '#c0ffc0'
    def label(self):
        if self.value.name:
            return '%s|%s' % (escape_dot(self.value.name),
                              quote_dot(self.value.text))
        else:
            return quote_dot(self.value.text)
    def tooltip(self):
        return self.value.name or self.value.text

def filter_propositions(propositions):
    """
    Given a set of Propositions, return the subset of Propositions
    that are 'interesting'.
    """
    filtered_propositions = set()
    if propositions is not None:
        for prop in propositions:
            if not is_boring_prop(prop):
                filtered_propositions.add(prop)
            for arg in prop.arguments:
                if arg.proposition:
                    filtered_propositions.add(arg.proposition)
    return filtered_propositions

def proposition_dot_nodes(doc, propositions):
    """
    Return a dot string that declares nodes for the given set of
    propositions, along with any entities or syn_nodes reachable from
    those propositions.
    """
    # Collect all nodes.
    prop_node_values = set()
    syn_node_values = set()
    mention_node_values = set()
    entity_node_values = set()
    for prop in propositions:
        prop_node_values.add(prop)
        for arg in prop.arguments:
            if arg.mention:
                entity = doc.mention_to_entity.get(arg.mention)
                if entity:
                    entity_node_values.add(entity)
                else:
                    mention_node_values.add(arg.mention)
            elif arg.syn_node:
                syn_node_values.add(arg.syn_node)
            else:
                prop_node_values.add(arg.value)
        #if prop.head: syn_node_values.add(prop.head)
        if prop.particle: syn_node_values.add(prop.particle)
        if prop.adverb: syn_node_values.add(prop.adverb)
        if prop.negation: syn_node_values.add(prop.negation)
        if prop.modal: syn_node_values.add(prop.modal)

    # Sort syn nodes by position.
    def syn_node_sort_key(syn_node):
        return (syn_node.start_char, -syn_node.end_char)
    syn_node_values = sorted(syn_node_values, key=syn_node_sort_key)

    return ([SynDotNode(v) for v in syn_node_values] +
            [MentionDotNode(v) for v in mention_node_values] +
            [EntityDotNode(v) for v in entity_node_values] +
            [PropDotNode(v) for v in prop_node_values])

def proposition_dot_edges(doc, propositions, focus=None,
                          use_mention_ports=True):
    edges = '{edge [labeldistance="0"];\n'
    for prop in propositions:
        args = [(arg.value, arg.role) for arg in prop.arguments]
        # pseudo-arguments:
        if prop.particle: args.append( (prop.particle, '<particle>') )
        if prop.adverb: args.append( (prop.adverb, '<adverb>') )
        if prop.negation: args.append( (prop.negation, '<negation>') )
        if prop.modal: args.append( (prop.modal, '<modal>') )
        #if prop.head:
        #    args.append( (prop.head, '<head>') ) # pseudo-arg for head.
        for arg_value, arg_role in args:
            src = dot_id(prop)
            entity = None
            if isinstance(arg_value, Mention):
                entity = doc.mention_to_entity.get(arg_value)
                dst = dot_id(entity or arg_value)
                if use_mention_ports:
                    dst += ':%s' % mention_port(arg_value)
            else:
                dst = dot_id(arg_value)
            style = PROP_EDGE_STYLE.get(arg_role, '')
            my_dir = prop_arg_dir(prop.pred_type, arg_role)
            if (focus is not None and focus==entity): my_dir = 'back'
            if my_dir!='forward': (src,dst) = (dst,src)
            if arg_role.startswith('<') and arg_role.endswith('>'):
                arg_role = arg_role[1:-1]
                if style: style += ','
                style += 'fontname="Italic"'
            edges += '  %s->%s [dir="%s",label="%s",%s];\n' % (
                src, dst, my_dir, escape_dot(arg_role), style)
    return edges + '}\n'

def event_dot_nodes(doc, events, include_prop_anchors=False):
    """
    Return a dot string that declares nodes for the given set of
    events, along with any entities or syn_nodes reachable from
    those events.
    """
    # Collect node values
    event_node_values = set()
    entity_node_values = set()
    mention_node_values = set()
    valmention_node_values = set()
    syn_node_values = set()
    prop_node_values = set()
    for event in events:
        event_node_values.add(event)
        if isinstance(event, EventMention):
            if event.anchor_node is not None:
                syn_node_values.add(event.anchor_node)
            if include_prop_anchors and event.anchor_prop is not None:
                prop_node_values.add(event.anchor_prop)
        else:
            for em in event.event_mentions:
                if em.anchor_node is not None:
                    syn_node_values.add(em.anchor_node)
                if include_prop_anchors and em.anchor_prop is not None:
                    prop_node_values.add(em.anchor_prop)
        for arg in event.arguments:
            if isinstance(arg, EventMentionArg):
                if arg.mention:
                    entity = doc.mention_to_entity.get(arg.mention)
                    if entity: entity_node_values.add(entity)
                    else: mention_node_values.add(arg.mention)
                if arg.value_mention:
                    valmention_node_values.add(arg.value_mention)
            else:
                if arg.entity:
                    entity_node_values.add(arg.entity)
                if arg.value_entity:
                    valmention_node_values.add(arg.value.value_mention)
    # Display all nodes.
    return ([MentionDotNode(v) for v in mention_node_values] +
            [ValMentionDotNode(v) for v in valmention_node_values] +
            [SynDotNode(v) for v in syn_node_values] +
            [PropDotNode(v) for v in prop_node_values] +
            [EntityDotNode(v) for v in entity_node_values] +
            [EventDotNode(v) for v in event_node_values])

def event_dot_edges(doc, events, include_prop_anchors=False, focus=None,
                    use_mention_ports=True):
    edges = '{edge [labeldistance="0",dir="none"];\n'
    for event in events:
        for arg in event.arguments:
            label = escape_dot(arg.role)
            entity = None
            if isinstance(arg, EventMentionArg):
                if arg.mention:
                    entity = doc.mention_to_entity.get(arg.mention)
                    dst = dot_id(entity or arg.mention)
                    if use_mention_ports:
                        dst += ':%s' % mention_port(arg.mention)
                else:
                    dst = dot_id(arg.value_mention)
            else:
                if arg.entity:
                    entity = arg.entity
                    dst = dot_id(arg.entity)
                else:
                    dst = dot_id(arg.value.value_mention)
            if focus is not None and entity==focus:
                edges += '  %s->%s [label="%s",dir="back"];\n' % (
                    dst, dot_id(event), label)
            else:
                edges += '  %s->%s [label="%s"];\n' % (dot_id(event),
                                                       dst, label)

    edges += '}\n'
    edges += '{edge [labeldistance="0",dir="none",color="#00ff00"];\n'
    for event in events:
        if isinstance(event, EventMention):
            anchors = [event.anchor_node]
        else:
            anchors = [em.anchor_node for em in event.event_mentions]
        for anchor in anchors:
            if anchor is None: continue
            edges += '  %s->%s [label="anchor"];\n' % (dot_id(anchor),
                                                       dot_id(event))
    edges += '}\n'
    if include_prop_anchors:
        edges += '{edge [labeldistance="0",dir="none",color="#00ff00"];\n'
        for event in events:
            if isinstance(event, EventMention):
                anchors = [event.anchor_prop]
            else:
                anchors = [em.anchor_prop for em in event.event_mentions]
            for anchor in anchors:
                if anchor is None: continue
                edges += '  %s->%s [label="anchor-prop"];\n' % (
                    dot_id(anchor), dot_id(event))
        edges += '}\n'
    return edges

def empty_graph_msg(items, src):
    context = type(src).__name__
    return '<i>This %s contains no %s.</i>' % (context, items)

USE_SVG = True
def render_graphviz(graph, src, kind, binary='dot', zoomable=True):
    if isinstance(graph, unicode):
        graph = graph.encode('utf-8')

    if USE_SVG: my_format = '-Tsvg'
    else: my_format = '-Tpng'
    p = subprocess.Popen([binary, my_format], stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    (dot_result, stderr) = p.communicate(graph)
    if p.returncode != 0:
        print stderr
        raise ValueError('Graphviz failed!')
    # Remove the xmldecl and doctype
    if USE_SVG:
        svg_graph = re.sub(r'^<\?xml[^>]+>\s*<!DOCTYPE[^>]+>', '', dot_result)
        svg_graph = svg_graph.replace('\r\n', '\n')
        if zoomable:
            svg_graph = ZOOMABLE_GRAPH % svg_graph
        return svg_graph.decode('utf-8', 'replace')
    else:
        import base64
        img_data = base64.b64encode(dot_result)
        return '<img src=data:image/gif;base64,%s />' % img_data

def count_propositions(src):
    """src should be a sentence or a document."""
    if isinstance(src, Sentence):
        propositions = src.proposition_set
    else:
        propositions = sum([[] if s.proposition_set is None else list(s.proposition_set) for s in src.sentences], [])
    propositions = filter_propositions(propositions)
    if not propositions:
        return 0
    return len(propositions)


def count_events(src):
    if isinstance(src, Sentence): events = src.event_mention_set
    else: events = src.event_set
    if not events:
        return 0
    return len(events)

def count_relations(src):
    if isinstance(src, Sentence): relations = src.rel_mention_set
    else: relations = src.relation_set
    if not relations:
        return 0
    return len(relations)

def count_icews_events(doc):
    icews_events = doc.icews_event_mention_set
    if not icews_events:
        return 0
    return len(icews_events)

def merged_graph_to_html(src):
    doc = src.document
    extra_nodes = []
    if isinstance(src, Sentence):
        propositions = src.proposition_set
        events = src.event_mention_set
        relations = src.rel_mention_set
        if INCLUDE_DISCONNECTED_ENTITIES_IN_MERGED_GRAPH and src.mention_set is not None:
            for mention in src.mention_set:
                entity = doc.mention_to_entity.get(mention)
                if entity: extra_nodes.append(EntityDotNode(entity))
    else:
        propositions = sum([[] if s.proposition_set is None else list(s.proposition_set) for s in src.sentences], [])
        events = src.event_set
        relations = src.relation_set
        if INCLUDE_DISCONNECTED_ENTITIES_IN_MERGED_GRAPH:
            extra_nodes += [EntityDotNode(e) for e in doc.entity_set or ()]
    propositions = filter_propositions(propositions)
    if not (propositions or relations or events):
        return empty_graph_msg('propositions, relations or events', src)
    dot_nodes = (relation_dot_nodes(doc, relations) +
                 proposition_dot_nodes(doc, propositions) +
                 event_dot_nodes(doc, events, include_prop_anchors=True) +
                 extra_nodes)
    graph = 'digraph merged {\n%s%s%s}\n' % (
        DOT_GRAPH_OPTIONS % dict(ranksep=0.1),
        DotNode.nodes_to_dot(dot_nodes, isinstance(src,Document)),
        (relation_dot_edges(doc, relations) +
         proposition_dot_edges(doc, propositions) +
         event_dot_edges(doc, events, include_prop_anchors=True)))
    return render_graphviz(graph, src, 'merged-graph')

def relation_dot_nodes(doc, relations):
    # Collect node values.
    rel_node_values = set()
    entity_node_values = set()
    mention_node_values = set()
    for relation in relations:
        rel_node_values.add(relation)
        if isinstance(relation, RelMention):
            for mention in (relation.left_mention, relation.right_mention):
                entity = doc.mention_to_entity.get(mention)
                if entity:
                    entity_node_values.add(entity)
                else:
                    mention_node_values.add(mention)
        else:
            for entity in (relation.left_entity, relation.right_entity):
                entity_node_values.add(entity)

    # Display all nodes.
    return ([MentionDotNode(v) for v in mention_node_values] +
            [EntityDotNode(v) for v in entity_node_values] +
            [RelDotNode(v) for v in rel_node_values])

def relation_dot_edges(doc, relations, focus=None, use_mention_ports=True):
    edges = '{edge [labeldistance="0"];\n'
    for rel in relations:
        swap_dir = False
        if isinstance(rel, RelMention):
            mentions = [rel.left_mention, rel.right_mention]
            swap_dir = (focus is not None and
                        focus==doc.mention_to_entity.get(rel.right_mention))
            sides = []
            for mention in mentions:
                entity = doc.mention_to_entity.get(mention)
                side = dot_id(entity or mention)
                if use_mention_ports:
                    side += ':%s' % mention_port(mention)
                sides.append(side)
        else:
            swap_dir = (focus is not None and focus==rel.right_entity)
            sides = [dot_id(e) for e in (rel.left_entity, rel.right_entity)]
        if swap_dir:
            edges += '  %s->%s->%s [dir="back"];\n' % (
                sides[1], dot_id(rel), sides[0])
        else:
            edges += '  %s->%s->%s;\n' % (sides[0], dot_id(rel), sides[1])
    return edges + '}\n'

def prop_uses_entity(prop, entity):
    if prop.pred_type not in (PredType.verb, PredType.copula): return False #@UndefinedVariable
    doc = entity.document
    return any(entity == doc.mention_to_entity.get(arg.mention)
               for arg in prop.arguments)
    return False

def event_uses_entity(event, entity):
    return any(entity == arg.entity for arg in event.arguments)

def relation_uses_entity(rel, entity):
    return entity in (rel.left_entity, rel.right_entity)

def entity_neighborhood_to_html(entity):
    doc = entity.document
    propositions = [prop for sent in doc.sentences
                    for prop in (sent.proposition_set or ())
                    if prop_uses_entity(prop, entity)]
    propositions = filter_propositions(propositions)
    events = [event for event in (doc.event_set or ())
              if event_uses_entity(event, entity)]
    relations = [rel for rel in (doc.relation_set or ())
                 if relation_uses_entity(rel, entity)]
    focus = FocusEntityDotNode(entity)
    if not (propositions or relations or events):
        return ''
    dot_nodes = (relation_dot_nodes(doc, relations) +
                 proposition_dot_nodes(doc, propositions) +
                 event_dot_nodes(doc, events))
    dot_nodes = [focus] + [n for n in dot_nodes if n.value!=entity]
    for node in dot_nodes: node.small = True
    if len(dot_nodes) == 1: return '' # boring graph
    options = DOT_GRAPH_OPTIONS % dict(ranksep=0.5)
    extra_node = ('start [label="",width=0.1,height=0.1,'
                  'style="filled",fillcolor="#00ff00"];\n')
    extra_edge= 'start -> %s;\n' % dot_id(entity)
    graph = 'digraph merged {\n%s%s%s}\n' % (
        options,
        extra_node+DotNode.nodes_to_dot(dot_nodes, True),
        (extra_edge+
         relation_dot_edges(doc, relations, focus=entity,
                            use_mention_ports=False) +
         proposition_dot_edges(doc, propositions, focus=entity,
                               use_mention_ports=False) +
         event_dot_edges(doc, events, focus=entity,
                         use_mention_ports=False)))
    return render_graphviz(graph, entity, 'entity-neighborhood')

def entities_to_html(doc):
    edge_weights = defaultdict(lambda:1)

    def connect_all_pairs(entities, weight):
        for e1 in entities:
            for e2 in entities:
                if id(e1)<id(e2):
                    edge_weights[e1,e2] += weight

    # Increase weight for entities that are mentioned in the same
    # sentence.
    for sent in doc.sentences:
        if sent.mention_set is None:
            continue
        entities = set(doc.mention_to_entity[m] for m in sent.mention_set
                       if m in doc.mention_to_entity)
        connect_all_pairs(entities, 0.1)

    # Increase weight for entities that are related by propositions
    for sent in doc.sentences:
        if sent.proposition_set is None:
            continue
        for prop in sent.proposition_set:
            mention_args = set(arg.mention for arg in prop.arguments
                               if arg.mention is not None)
            entity_args = set(doc.mention_to_entity[m] for m in mention_args
                              if m in doc.mention_to_entity)
            connect_all_pairs(entity_args, 1)

    # Prune out "boring" entities
    interesting_entities = set()
    # Any weight over 1.1 makes it interesting.
    for (e1,e2), weight in edge_weights.items():
        if weight > 1.1:
            interesting_entities.add(e1)
            interesting_entities.add(e2)
    # Any entity that appears in >1 sentence is interesting.
    for entity in doc.entity_set or ():
        if len(set(m.owner_with_type(Sentence)
                   for m in entity.mentions)) > 1:
            interesting_entities.add(entity)

    # Draw nodes.
    dot_nodes = [EntityDotNode(e) for e in interesting_entities]
    for node in dot_nodes: node.small = True

    # increase edge weight based on mention frequency.
    for (e1, e2), weight in edge_weights.items():
        weight *= min(len(e1.mentions),5)/2.0
        weight *= min(len(e2.mentions),5)/2.0
        edge_weights[e1,e2] = weight

    # Draw edges
    edges = '{\n'
    if edge_weights:
        MAX_WEIGHT=max(edge_weights.values())
        for (e1, e2), weight in edge_weights.items():
            if e1 not in interesting_entities: continue
            if e2 not in interesting_entities: continue

            assert id(e1)<id(e2)
            w = 1.0*min(weight, MAX_WEIGHT)/MAX_WEIGHT
            color = '#%02x%02x%02x' % (220, (240-240*w), (240-240*w))
            attribs = 'weight=%s' % weight
            attribs += ',len=%s' % (1.5 - 1.3 * min(w, MAX_WEIGHT))
            attribs += ',color="%s"' % color
            if weight<2: attribs += ',style="dashed"'
            elif weight>MAX_WEIGHT/3 and weight>3: attribs += ',style="bold"'
            #print attribs
            edges += '  %s -- %s [%s];\n' % (dot_id(e1), dot_id(e2), attribs)
    edges += '}\n'

    options = 'node [fontsize="8"];\n'
    #options += 'ratio=0.5;\n'

    graph = 'graph entities {\n%s%s%s}\n' % (
        options, DotNode.nodes_to_dot(dot_nodes, True), edges)

    # neato, twopi, circo, fdp, sfdp
    return DOC_ENTITY_GRAPH % (
        render_graphviz(graph, doc, 'entities', 'fdp'))

def icews_events_to_html(doc):
    icews_events = doc.icews_event_mention_set
    if not icews_events:
        return empty_graph_msg('icews_events', doc)
    dot_nodes = icews_event_dot_nodes(doc, icews_events)
    graph = 'digraph icews_events {\n%s%s%s}\n' % (
        DOT_GRAPH_OPTIONS % dict(ranksep=0.5),
        DotNode.nodes_to_dot(dot_nodes, True),
        icews_event_dot_edges(doc, icews_events))
    return render_graphviz(graph, doc, 'icews_events')

def icews_event_dot_nodes(doc, icews_events):
    # Collect node values.
    event_values = set()
    actor_values = set()
    for event in icews_events:
        event_values.add(event)
        for participant in event.participants:
            actor_values.add(participant.actor)
    return ([ICEWSEventMentionDotNode(v) for v in event_values] +
            [ICEWSActorMentionDotNode(v) for v in actor_values])

def icews_event_dot_edges(doc, icews_events):
    edges = '{\n'
    for event in icews_events:
        for participant in event.participants:
            if participant.role == 'SOURCE':
                edges += '  %s -> %s;' % (
                    dot_id(participant.actor), dot_id(event))
            elif participant.role == 'TARGET':
                edges += '  %s -> %s;' % (
                    dot_id(event), dot_id(participant.actor))
            else:
                edges += ('  %s -> %s [label="%s",color="#800000",'
                          'dir="none"];' % (
                              dot_id(event), dot_id(participant.actor),
                              escape_dot(participant.role)))
    return edges + '}\n'

###########################################################################
## Sentence Details
###########################################################################


###########################################################################
## Document Entities
###########################################################################

DOC_ENTITIES_BODY = '''
<table class="entities">
%s
</table>
'''


ENTITY_HTML = '''
<div class="entity collapsed-entity %(css)s" id="%(anchor)s">
<div class="mention-info">%(mention_info)s</div>
<h2><a class="expand-entity" href="#"
name="%(anchor)s">%(name)s</a></h2>
<div class="entity-mentions">
<table class="entity-mentions">
%(mention_rows)s
</table>
<div class="entity-neighborhood">
%(neighborhood_graph)s
</div>
</div>
</div>
'''

ENTITY_MENTION_ROW = '''
<tr class="mention">
<td class="location"><a
href="%(html_prefix)s-sent-%(sent_no)d-details.html"
>Sentence %(sent_no)s</a></td>
<td class="left">%(left)s</td>
<td class="mention">%(mention)s</td>
<td class="right">%(right)s</td>
'''

def name_for_entity(entity):
    names = defaultdict(int)
    for mention in entity.mentions:
        score = min(20, len(mention.text))
        if len(mention.text) > 20: score -= 0.001*len(mention.text)
        if mention.mention_type == MentionType.name: score += 40
        names[mention.text] += score
    best_name, _best_score = sorted(names.items(), key=lambda t:t[1])[-1]
    #print 'Best name: %r (%s)' % (best_name, best_score)
    return best_name


###########################################################################
## Document Text
###########################################################################

TEXT_PAGE_BODY = '''
<div class="text-options">
<input id="toggle-prons" type="checkbox" checked> Mark Pronouns
<input id="toggle-names" type="checkbox" checked> Mark Names
<input id="toggle-descs" type="checkbox" checked> Mark Descriptions
%(other_mark)s
<input id="toggle-unprocessed" type="checkbox" checked> Show Unprocessed
</div>
<div class="doc-body mark-prons mark-names mark-desc mark-icews-actors"
     id="doc-body">
%(body)s
</div>
'''

MARK_ICEWS = '''
<input id="toggle-icews-actors" type="checkbox" checked> Mark ICEWS Actors
'''





def icews_same_actor_as_parent(actor, mention_to_icews_actor):
    node = actor.mention.syn_node.parent
    ok_node_types = (MentionType.name, MentionType.pron,MentionType.desc) #@UndefinedVariable
    while node is not None:
        if ( (node.mention is not None) and
             (node.mention.mention_type in ok_node_types)):
            p = mention_to_icews_actor.get(node.mention)
            if p:
                return (actor.actor_uid == p.actor_uid and
                        actor.paired_actor_uid == p.paired_actor_uid and
                        actor.paired_agent_uid == p.paired_agent_uid)
        node = node.parent
    return False


###########################################################################
## Navigation/Context Frames
###########################################################################



###########################################################################
## Document-level graphs
###########################################################################



###########################################################################
## Helper Functions
###########################################################################


def is_newer(f1, f2):
    if not os.path.exists(f1): return False
    return (os.stat(f1)[stat.ST_MTIME] > os.stat(f2)[stat.ST_MTIME])

def assign_html_filenames(inputs, parser):
    html_files = {}
    used_html_names = set()
    for filename in inputs:
        if not os.path.exists(filename):
            parser.error("File %s not found" % filename)
        html_file = os.path.split(filename)[-1]+'.html'
        if html_file in used_html_names:
            n = 2
            while ('%s-%s.html' % (html_file[:-4], n)) in used_html_names:
                n += 1
            html_file = '%s-%s.html' % (html_file[:-4], n)
        html_files[filename] = html_file
    return html_files

###########################################################################
## Command-line Interface
###########################################################################

###########################################################################
## Document View Pages
###########################################################################

def count_document_parts(doc):
	#context_file = os.path.join(out_dir, doc.html_prefix+'-nav.html')
	# Sentence pages
	print "\tsents:%d, props:%d, rels:%d, events:%d, icews_events:%d \n" % (len(doc.sentences), count_propositions(doc), count_relations(doc), count_events(doc), count_icews_events(doc))


###########################################################################
## Searches
###########################################################################

SEARCH_PAGE_BODY = '''
<div class="text-options">
<input id="toggle-prons" type="checkbox"> Mark Pronouns
<input id="toggle-names" type="checkbox"> Mark Names
<input id="toggle-descs" type="checkbox"> Mark Descriptions
<input id="toggle-icews-actors" type="checkbox"> Mark ICEWS Actors
</div>
<div class="doc-body" id="doc-body">
%(body)s
</div>
'''

SEARCHER_MATCH_SENTENCE_HTML = '''
<div class="match %(even_or_odd)s-match">
  <div class="match-location">%(sent_location)s</div>
  <div class="context">%(left_context)s</div>
%(text)s
  <div class="context">%(right_context)s</div>
  <div class="clear"></div>
</div>
'''




TERM_SEARCH_BOX = '''
<div class="searcher-info">
<h3>%(name)s: Search Terms</h3>
<div class="info-body">
%(terms)s
</div>
</div>
'''


@staticmethod
def stem(word):
	return re.sub(r'(...)[ey]$', r'\1', word).lower()


###########################################################################
## All Pages
###########################################################################

def count_all_docs(html_files):

    # Process each document.
    n = 0
    tuples = sorted(html_files.items())
    for index in range(len(tuples)):
        src_file, html_file = tuples[index]
        n += 1
        doc = Document(src_file)
        doc.filename = src_file
        doc.html_prefix = html_file[:-5]

        # create mention->entity map; this is a bit of a hack.
        doc.mention_to_entity = dict((m,e) for e in doc.entity_set or ()
                                     for m in e.mentions)


        sys.stdout.write(' %s...' % ( doc.docid,) )
        sys.stdout.flush()
        count_document_parts(doc)
          
    print '%d doc [done]' % (n, )

def is_exe(fpath): # Is the given path an executable?
    return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

def which(program): # Can I find the named binary in the path?
    fpath, _fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file
    return None

###########################################################################
## Command-line Interface
###########################################################################

USAGE="""\
Usage: %prog [options] INPUTS -o OUTPUT_DIR

Render each input file as an HTML file.  Use -h to see options.
"""
def cli():
    import optparse

    parser = optparse.OptionParser(usage=USAGE)
    parser.add_option("-o", "--out-dir", dest="out_dir", metavar='DIR',
                      help='Directory where output should be written [REQUIRED]')
    parser.add_option("--nav-only", dest="nav_only",
                      action='store_true', default=False,
                      help='Only generate the navigation index page, '
                      'and not the document pages.')
    parser.add_option("-f", dest="force", action='store_true', default=False,
                      help='Rebuild all output files, even if an existing '
                      'output file is newer than the input file.')
    parser.add_option("-p", "--path", dest="path", default = '',
                      help='Path to Graphviz binaries')
    parser.add_option("--no-doc-pages", dest='show_doc_pages',
                      action='store_false', default=True,
                      help='Do not generate the document-view pages.  This '
                      'option is usually used in conjunction with another '
                      'option (such as --find-terms) that displays other '
                      'pages.')
    parser.add_option("--no-doc-entities", dest='show_doc_entities',
                      action='store_false', default=True,
                      help='Do not generate html for entities at the doc level.')
    parser.add_option("--no-doc-graphs", dest='show_doc_graphs',
                      action='store_false', default=True,
                      help='Do not generate the graphs at the doc level.')
    parser.add_option("--no-merged-graphs", dest='show_merged_graphs',
                      action='store_false', default=True,
                      help='Do not generate the merged graphs.')
    parser.add_option("--find-terms", metavar='FILE',
                      action='append', dest='term_search_files',
                      help='Run a text search for all of the patterns that '
                      'are listed in the specified file.')
    parser.add_option("--find-props", metavar='FILE',
                      action='append', dest='prop_search_files',
                      help='Run a prop search for all of the patterns that '
                      'are listed in the specified file.')
    parser.add_option("--index-per-file", action="store_true", dest="index_per_file",
                        default=False, help='Create a separate directory and  '
                        'index for each file. If this is specified, there must '
                        'be a single input file with two tab-separated columns: the directory '
                        'name first (typically a document ID) and the SerifXML '
                        'file to view second')
    parser.add_option("--slice", metavar='NUMBER', dest='sliceNum', default=0, 
                        help='Slice of file list to render.  Requires '
                        '--index-per-file and --parallel. Must be >= 0 and < --parallel. '
                        'Typically used for runjobs scripts')
    parser.add_option('--parallel', metavar='NUMBER', dest='parallel', default=1,
                        help='Number of parallel slices.  Must be >= 0. Requires '
                        '--index-per-file and --slice. Typically used for runjobs scripts')

    (options, inputs) = parser.parse_args()
    if options.path: # Extend our path if need be
        os.environ["PATH"] += os.pathsep + options.path

    if not inputs:
        parser.error("Expected at least one input file")

	out_dir = ""
    data_dir = ""

    run_with_single_index(inputs, parser, data_dir, options)

def run_with_single_index(inputs, parser, data_dir, options):
    # If the input is a directory, use the files in it as inputs
    if len(inputs) == 1 and os.path.isdir(inputs[0]):
        input_dir = inputs[0]
        inputs = [ os.path.join(input_dir, f) for f in os.listdir(input_dir) if os.path.isfile(os.path.join(input_dir,f)) ]

    # Pick a unique html filename for each input file
    html_files = assign_html_filenames(inputs, parser)
    count_all_docs(html_files)


def run_with_index_per_file(inputs, parser, data_dir, out_dir, options):
    if len(inputs) != 1 or not os.path.isfile(inputs[0]):
        sys.exit("If --index-per-file is specified, must have a single input file, a file mapping document IDs to SerifXML files")

    makedirsHandlingRaceCondition(out_dir)

    sliceNum = int(options.sliceNum)
    parallel = int(options.parallel)
    count = 0
    for (docID, serifXMLFile) in parseFileMap(inputs[0]):
        if count % parallel == sliceNum:
            documentDir = os.path.join(out_dir, docID)
            run_with_single_index([serifXMLFile], parser, data_dir, documentDir, options)
        count += 1

def parseFileMap(f):
    ret = []
    for line in open(f):
        ret.append(line.strip().split('\t'))
    return ret

def makedirsHandlingRaceCondition(path):
    # create directory with race condition handling
    # (a real problem with runjobs)
    try:
        os.makedirs(path)
    except OSError:
        if not os.path.isdir(path):
            raise



if __name__ == '__main__':
    start_time = time.time()
    cli()
    end_time = time.time()
    print "Program execution took %.2f seconds" % (end_time - start_time)
