#!/bin/env python

import re, sys, os, apf, math, bisect, random, textwrap
from collections import defaultdict
import histogram; reload(histogram) # testing
reload(apf) # testing
from histogram import *
from scipy import stats

"""
Filter: a class that embodies a single kind of filter.  Instances
  are actual filters.

- generate a bunch of trial filters.
- repeatedly:
  - determine the f-score that would result from applying each filter
    - use an alpha that biases us towards preccision over recall.
  - pick the most useful one.
"""

FMEASURE_ALPHA = None # not used any more.

######################################################################
# Serif Decisions About Relation Mentions
######################################################################

class SerifRelationDecision(object):
    """
    Information about a single decision made by the serif relation
    finder model.
    """

    RELATION_LABEL = (r'(NONE|ART.[\w-]+|GEN-AFF.[\w-]+|ORG-AFF.[\w-]+|'
                      r'PART-WHOLE.[\w-]+|PER-SOC.[\w-]+|PHYS.[\w-]+|'
                      r'REL)')

    DEBUG_RECORD_RE = re.compile(
        (r'LHS\n'
         r' +Character Span: \[(?P<lhs_start>\d+):(?P<lhs_end>\d+)\]\n'
         r' +DocID: (?P<lhs_docid>.*)\n'
         r' +Text: (?P<lhs_text>.*)\n' +
         r' +Mention Type: (?P<lhs_mention_type>.*)\n' +
         r' +Entity Type: (?P<lhs_entity_type>.*)\n' +
         r' +Entity Subtype: (?P<lhs_entity_subtype>.*)\n') +
        (r'RHS\n'
         r' +Character Span: \[(?P<rhs_start>\d+):(?P<rhs_end>\d+)\]\n'
         r' +DocID: (?P<rhs_docid>.*)\n'
         r' +Text: (?P<rhs_text>.*)\n' +
         r' +Mention Type: (?P<rhs_mention_type>.*)\n' +
         r' +Entity Type: (?P<rhs_entity_type>.*)\n' +
         r' +Entity Subtype: (?P<rhs_entity_subtype>.*)\n') +
        r'VECTOR: (?P<vector_answer>.*)\n' +
        r'CONFIDENT VECTOR: (?P<confident_vector_answer>.*)\n' +
        r'TREE: (?P<tree_answer>.*)\n' +
        (r'(%s)?' % (r'P1: (?P<p1_answer>.*)\n' +
                     r'(%s)' % (
                         r'NONE:\n' +
                         r'(?P<p1_none_features>((?!SCORE).*\n)*)' +
                         r'SCORE: (?P<p1_none_score>.*)\n') +
                     r'(%s)?' % (
                         RELATION_LABEL + r':\n' +
                         r'(?P<p1_rel_features>((?!SCORE).*\n)*)' +
                         r'SCORE: (?P<p1_rel_score>.*)\n'))) +
        (r'(%s)?' % (r'MAXENT: (?P<maxent_answer>.*)\n' +
                      r'(?P<maxent_feature_list>(%s){1,2})?' % (
                          RELATION_LABEL + ':\n' +
                          r'((?!SCORE).*\n)*' +
                          r'SCORE: .*\n') +
                      r'(?P<maxent_feature_table>(%s))?' % (
                          r'MAXENT FEATURE TABLE:\n'
                          r'\t\t(?P<maxent_header>.*)\n' +
                          r'(?P<maxent_rows>(\t(?!TOTAL:).*\n)*)' +
                          r'\tTOTAL:\t(?P<maxent_footer>.*\n)') +
                      r'MAXENT SCORE: (?P<maxent_score>.*)\n')) +
         (r'(%s)?' % (r'SECONDARY P1: (?P<secondary_p1_answer>.*)\n')) +
        r'(?P<use_p1_answer>USE P1 ANSWER\n)?'
        r'(?P<use_cvec_answer>FALL BACK TO CONFIDENT VECTOR ANSWER\n)?' +
        r'(?P<use_p1_answer_vec_tree>VECTOR=TREE, SO USE P1 ANSWER\n)?'
        r'(?P<secondary_selected>SECONDARY SELECTED: .*\n)?'
        r'(ETHNIC TYPE HEURISTIC: (?P<ethnic_heuristic>.*)\n)?' +
#         # Old debug output for reversing:
#         r'(%s)?' % (r'Reversed: (?P<reversed>.*)\n' +
#                     r'Not reversed: (?P<not_reversed>.*)\n'
#                     r'(?P<reverse>SO... reverse\n)?') +
        r'(%s)?' % (r'Reverse Score: (?P<reversed>.*)\n' +
                    r'(?P<reverse>SO... reverse\n)?') +
        r'(%s)?' % (r'Confidence Metric:\n' +
                    r'(?P<confidence_breakdown>(  \d.*\n)*)') +
        r'(FINAL ANSWER: (?P<answer>.*)\n)?' +
        r'(  Confidence: (?P<confidence>.*)\n)?' +
        r'(?P<tail>[\s\S]*)'
        )

    def __init__(self, s):
        m = self.DEBUG_RECORD_RE.match(s)
        if m is None:
            print 'OUCH'
            print '='*75
            print s
            print '='*75
            raise ValueError('regexp did not match')
        #print m.group('answer')
        if m.group('tail'):
            print 'WARNING: NON-EMPTY TAIL:'
            print m.group('tail')
            print '-'*75
        # Copy the groups from the regexp to our instance.
        self.__dict__.update(m.groupdict())
        # Set some types appropriately
        self.lhs_start = int(self.lhs_start)
        self.lhs_end = int(self.lhs_end)
        self.rhs_start = int(self.rhs_start)
        self.rhs_end = int(self.rhs_end)
        self.maxent_score = float(self.maxent_score or 0)
        self.reversed = float(self.reversed or 0)
        #self.not_reversed = float(self.not_reversed or 0)
        self.not_reversed = 0
        self.p1_none_score = float(self.p1_none_score or 0)
        self.p1_rel_score = float(self.p1_rel_score or 0)
        self.p1_score = self.p1_rel_score - self.p1_none_score
        # Check docid.
        assert self.lhs_docid == self.rhs_docid
        self.docid = self.lhs_docid
        if self.docid.endswith('.sgm'):
            print '  Warning: Stripping ".sgm" from docid!'
            self.docid = self.lhs_docid = self.rhs_docid = self.docid[:-4]
        # Process feature info??
        # Answers
        self.answers = set(v for (k,v) in m.groupdict().items()
                           if k.endswith('_answer')
                           and v not in ('NONE', None, 'REL'))

        # Process the maxent table.
        maxent_tags = self.maxent_header.split('\t')
        self.maxent_features = dict((t,{}) for t in maxent_tags)
        for row in self.maxent_rows.split('\n'):
            if not row.strip(): continue
            pieces = row.split('\t')[1:]
            assert len(maxent_tags) == len(pieces)-1, `row`
            feature = pieces[0]
            for tag, weight in zip(maxent_tags, pieces[1:]):
                self.maxent_features[tag][feature] = float(weight)
        maxent_scores = self.maxent_footer.strip().split('\t')
        self.maxent_scores = dict((tag, float(score)) for (tag, score) in
                                  zip(maxent_tags, maxent_scores))
        #print self.maxent_features
        #print self.maxent_scores

        # Pre-compute a key
        self.key = serif_relation_decision_key(self)

    @classmethod
    def parse(cls, s, interesting_record_out=None):
        debug_records = []
        n = 0
        for m in re.finditer(r'LHS\n'
                             r'(.*\S.*\n)*', # non-blank lines
                             s):
            debug_records.append(SerifRelationDecision(m.group()))
            if interesting_record_out and debug_records[-1].answer != 'NONE':
                interesting_record_out.write(m.group()+'\n')
        return debug_records

def load_serif_relations(root):
    interesting_record_out = open(
        os.path.join(root, 'relation-debug.interesting'), 'wb')
    serif_relations = []
    for (dir, subdirs, files) in os.walk(root):
        for filename in files:
            if (filename.startswith('relation-debug') and
                not filename.endswith('interesting')):
                path = os.path.join(dir, filename)
                print 'Reading %s...' % path
                s = open(os.path.join(root, path)).read()
                rels = SerifRelationDecision.parse(s, interesting_record_out)
                serif_relations.extend(rels)
    interesting_record_out.close()
    return serif_relations

######################################################################
# Filters
######################################################################

class Filter(object):
    def __init__(self, name, predicate, cache=False, **kwargs):
        self.name = name
        self.predicate = predicate
        self.kwargs = kwargs
        if cache:
            self.cache = {}
        else:
            self.cache = None

    def check(self, relation):
        if self.cache is None:
            return self.predicate(relation, **self.kwargs)
        else:
            result = self.cache.get(relation)
            if result is None:
                result = self.predicate(relation, **self.kwargs)
                self.cache[relation] = result
            return result

class BinaryFilterMaker(object):
    def __init__(self, name, func, min_freq=0.005):
        self.name = name
        self.func = func

    def make_filters(self, serif_relations):
        return [Filter('%s==True' % self.name,
                       (lambda r: bool(self.func(r)))),
                Filter('%s==False' % self.name,
                       (lambda r: not bool(self.func(r))))]
            
class DiscreteFilterMaker(object):
    def __init__(self, name, func, min_freq=0.005):
        self.name = name
        self.func = func
        self.min_freq=min_freq

    def make_filters(self, serif_relations):
        counts = defaultdict(int)
        for r in serif_relations: counts[self.func(r)] += 1
        values = [v for (v,c) in counts.items()
                  if (float(c)/len(serif_relations)>self.min_freq)]

        def filters_for(value):
            return [Filter('%s==%r' % (self.name, value),
                           (lambda r: self.func(r) == value)),
                    Filter('%s!=%r' % (self.name, value),
                           (lambda r: self.func(r) != value))]

        return sum((filters_for(value) for value in values), [])

class FeatureFilterMaker(object):
    def __init__(self, min_freq=0.005):
        self.min_freq=min_freq

    def make_filters(self, serif_relations):
        counts = defaultdict(int)
        weights = {}
        for r in serif_relations:
            for feature in r.maxent_features['REL']:
                counts[feature] += 1
                weights[feature] = (r.maxent_features['REL'][feature] -
                                    r.maxent_features['NONE'][feature])
        values = [v for (v,c) in counts.items()
                  if (float(c)/len(serif_relations)>self.min_freq)]

        def filters_for(value):
            return [Filter('%s (w=%.2f)' % (value, weights[value]),
                           (lambda r: value in r.maxent_features['REL'])),
                    Filter('!%s (w=%.2f)' % (value, weights[value]),
                           (lambda r: value not in r.maxent_features['REL']))]

        return sum((filters_for(value) for value in values), [])

class CutoffFilterMaker(object):
    def __init__(self, name, func, slices=20):
        self.name = name
        self.func = func
        self.slices = slices

    def make_filters(self, serif_relations):
        values = sorted(self.func(r) for r in serif_relations)
        filters = []
        prev_value = None
        for slice in range(self.slices):
            cutoff = slice*len(values)/self.slices
            value = values[cutoff]
            if value == prev_value: continue
            filters.append(Filter('%s>%s' % (self.name, value),
                                  lambda r,value=value: self.func(r)>value))
            filters.append(Filter('%s<%s' % (self.name, value),
                                  lambda r,value=value: self.func(r)<value))
            prev_value = value
        return filters

class FixedFilterMaker(object):
    def __init__(self, filters):
        self.filters = filters
    def make_fitlers(self, serif_relations):
        return self.filters

######################################################################
# Relation Performance Evaluation
######################################################################

def apf_relmention_key(rel):
    return (rel.relation.docid,
            rel.relation.rel_type, rel.relation.rel_subtype, 
            rel.arg1.start_offset, rel.arg1.end_offset,
            rel.arg2.start_offset, rel.arg2.end_offset)

def serif_relation_decision_key(rel):
    if rel.answer in ('NONE', None):
        rel_type = rel_subtype = None
    else:
        (rel_type, rel_subtype) = rel.answer.lower().split('.', 1)
    return (rel.docid, rel_type, rel_subtype,
            rel.lhs_start, rel.lhs_end,
            rel.rhs_start, rel.rhs_end)

def make_apf_relmention_dict(apf_relations):
    return dict( (apf_relmention_key(r), r) for r in apf_relations)

def evaluate(serif_relations, apf_relmention_dict, alpha=0.5, filter=None):
    """
    Return (precision, recall, fscore).

    @param alpha: alpha for the f-score: 1=emphasize precision,
        0=emphasize recall.
    """
    # Discard anything indicated by the filter.
    if filter:
        serif_relations = [r for r in serif_relations if filter.check(r)]
    
    tp = sum(serif_rel.key in apf_relmention_dict
             for serif_rel in serif_relations)
    if tp == 0: return 0, 0, 0
    n_gold = len(apf_relmention_dict)
    n_test = len(serif_relations)
    precis = float(tp)/n_test
    recall = float(tp)/n_gold
    return precis, recall, fscore(precis, recall, alpha)

def fscore(precis, recall, alpha):
    if precis==0 or recall==0: return 0
    else: return 1.0/(alpha/precis + (1-alpha)/recall)
    
def max_recall(serif_relations, apf_relations):
    A = set(serif_relation_decision_key(r) for r in serif_relations)
    B = set(apf_relmention_key(r) for r in apf_relations)
    return float(len(A)) / len(A.union(B))

######################################################################
# Filter Choosers
######################################################################

def choose_filters(apf_relations, serif_relations, filter_makers,
                   min_delta=0.01, depth=10, width=8,
                   narrowing_factor=0.5, alpha=0.5):
    apf_relmention_dict = make_apf_relmention_dict(apf_relations)

    print '='*75
    print 'F-Measure Alpha: %s   Depth: %d   Width: %d' % (
        alpha, depth, width)
    print '='*75
    p, r, f = evaluate(serif_relations, apf_relmention_dict, alpha)
    print '%-38s p=%4.1f r=%4.1f f=%4.1f         n=%4d' % (
        'Initial Score', 100.*p, 100.*r, 100.*f, len(serif_relations))
    return _choose_filters(apf_relmention_dict, serif_relations,
                           filter_makers, min_delta, depth, width,
                           narrowing_factor, alpha, prefix='  ')

def _choose_filters(apf_relmention_dict, serif_relations, filter_makers,
                    min_delta, depth, width, narrowing_factor, alpha, prefix):
    initial_score = evaluate(serif_relations, apf_relmention_dict, alpha)[-1]
    filters = sum((filter_maker.make_filters(serif_relations)
                   for filter_maker in filter_makers), [])
    filter_scores = [(f, evaluate(serif_relations, apf_relmention_dict,
                                  alpha, f))
                     for f in filters]
    filter_scores.sort(reverse=True, key=lambda kv:kv[1][-1])
    
    # Choose prefix for sub-filters
    if width > 1:
        if width*narrowing_factor <= 1 and depth>2:
            child_prefix = prefix + '   and'
        else:
            child_prefix = prefix + '  '
    else:
        width = 1
        child_prefix = prefix

    # Try out each of the top `width` filters.
    chosen_filters = []
    for (filter, (p,r,f)) in filter_scores:
        # Check if we're done.
        if (f-initial_score) <= (min_delta/100.): break
        if len(chosen_filters) >= width: break
        # Skip filters that look too similar to ones we've already used.
        m = re.match('^(.*[<>])[\d\.]+$', filter.name)
        if m and any(f.name.startswith(m.group(1)) for f in chosen_filters):
            #print '         (skip %s)' % filter.name
            continue
        # Apply the filter and evaluate the result
        chosen_filters.append(filter)
        filtered_relations = [r for r in serif_relations if filter.check(r)]
        p, r, f = evaluate(filtered_relations, apf_relmention_dict, alpha)
        n = len(filtered_relations)
        _show_filter(filter, p, r, f, initial_score, n, prefix)
        # Recurse to nested filters
        if depth>1:
            chosen_filters += _choose_filters(
                apf_relmention_dict, filtered_relations,
                filter_makers, min_delta=min_delta,
                depth=depth-1, width=width*narrowing_factor,
                narrowing_factor=narrowing_factor,
                prefix=child_prefix, alpha=alpha)
    return chosen_filters
            
def choose_filters_2(apf_relations, serif_relations, filter_makers,
                     alphas, nbest=5, display_alphas=None):
    apf_relmention_dict = make_apf_relmention_dict(apf_relations)
    
    initial_p, initial_r = evaluate(serif_relations, apf_relmention_dict)[:2]
    
    # Evaluate the filters.  Use arbitrary alpha for now -- just keep
    # precision and recall.
    filters = sum((filter_maker.make_filters(serif_relations)
                   for filter_maker in filter_makers), [])
    noop_filter = Filter('Initial Score', lambda r:True)
    filters.append(noop_filter)
    filter_scores = [(f, evaluate(serif_relations, apf_relmention_dict,
                                  0.5, f)[:2])
                     for f in filters]
    precis = dict((f.name, p) for (f,(p,r)) in filter_scores)
    
    # Get the N best filters for each alpha value.
    filters = {noop_filter.name: noop_filter}
    for alpha in alphas:
        initial_fscore = fscore(initial_p, initial_r, alpha)
        def sort_key(scored_filter):
            (filter, (p,r)) = scored_filter
            return -fscore(p,r,alpha)
        filter_scores.sort(key=sort_key)
        for (filter, (p,r)) in filter_scores[:nbest]:
            if fscore(p,r,alpha) > initial_fscore:
                filters[filter.name] = filter

    # Display them.
    if display_alphas is None: display_alphas = alphas
    print '='*(50+6*len(display_alphas))
    print ' '*41 + ' F(Alpha) '.center(6*len(display_alphas)-1, '_')
    fmt = 'Filter'.center(40) + (' %5.1f'*len(display_alphas)) + '   Precis'
    print fmt % tuple(100*alpha for alpha in display_alphas)
    filter_scores = dict((fname, tuple(evaluate(serif_relations,
                                                apf_relmention_dict,
                                                alpha, filter)[-1]
                                       for alpha in display_alphas))
                         for (fname, filter) in filters.items())
    print '-'*(50+6*len(display_alphas))
    _show_fscores(noop_filter.name, filter_scores, precis)
    print '-'*(50+6*len(display_alphas))
    for fname in sorted(filter_scores, key=lambda n:-filter_scores[n][0]):
        if fname != noop_filter.name:
            _show_fscore_diffs(fname, filter_scores, precis)
    print '='*(50+6*len(display_alphas))
    return filters.values()

def _colwrap(*columns):
    # copy.
    columns = [(w,s) for (w,s) in columns]
    # destructively assemlbe the result.
    lines = []
    while any(s for (w,s) in columns):
        if lines: indent = 2
        else: indent = 0
        lines.append(''.join([(' '*indent)+s[:w-indent].ljust(w-indent)+' '
                              for (w,s) in columns]).rstrip())
        columns = [(w,s[w-indent:]) for (w,s) in columns]
    return '\n'.join(lines)

def _show_fscores(fname, filter_scores, precis):
    scores = tuple(100*score for score in filter_scores[fname])
    print _colwrap([40, fname],
                   [100, ( (' %5.1f'*(len(scores))) % scores +
                           '   %5.1f' % (100*precis[fname]))])
                                     
def _show_fscore_diffs(fname, filter_scores, precis):
    diffs = tuple(100*(s1-s2) for (s1,s2) in
                  zip(filter_scores[fname], filter_scores['Initial Score']))
    print _colwrap([40, fname],
                   [100, ( (' %+5.1f'*(len(diffs))) % diffs +
                           '   %5.1f' % (100*precis[fname]))])
                                     
def _show_filter(filter, p, r, f, score, n, prefix):
    delta = f-score
    print _colwrap([len(prefix), prefix],
                   [37-len(prefix), filter.name],
                   [100, 'p=%4.1f r=%4.1f f=%4.1f (%+5.1f) n=%4d' % (
                       100.*p, 100.*r, 100.*f, 100.*delta, n)])

######################################################################
# Filter functions
######################################################################

def entity_types(rel):
    return '%s+%s' % (rel.lhs_entity_type,rel.rhs_entity_type)

def mention_types(rel):
    return '%s+%s' % (rel.lhs_mention_type,rel.rhs_mention_type)

def maxent_score(rel):
    return -math.log(1.000001-rel.maxent_score)

def max_pro_rel_feature(rel):
    return max(w1-rel.maxent_features['NONE'][feat]
               for feat, w1 in rel.maxent_features['REL'].items())
               
def max_anti_rel_feature(rel):
    return max(rel.maxent_features['NONE'][feat]-w1
               for feat, w1 in rel.maxent_features['REL'].items())
               
def overlap_type(rel):
    if rel.rhs_start == rel.lhs_start:
        return 's=s'
    elif rel.rhs_end == rel.lhs_end:
        return 'e=e'
    elif nested_args(rel):
        return 'nested'
    elif argdist(rel) == 1:
        return 'adjacent'
    else:
        return 'distinct'

def arglen(rel):
    # unit here is number of characters...
    rhs_chars = rel.rhs_end+1-rel.rhs_start
    lhs_chars = rel.lhs_end+1-rel.lhs_start
    rhs_words = len(rel.rhs_text.split())
    lhs_words = len(rel.lhs_text.split())
    #return max(lhs_chars, rhs_chars)
    return lhs_words+rhs_words
    return min(lhs_chars, rhs_chars)
    #return rhs_words + lhs_words

def nested_args(rel):
    return ((rel.rhs_start <= rel.lhs_start <= rel.rhs_end+1) or
            (rel.lhs_start <= rel.rhs_start <= rel.lhs_end+1))

def argdist(rel):
    # unit here is number of characters...
    if nested_args(rel): return 0
    return min(abs(rel.rhs_end+1-rel.lhs_start),
               abs(rel.lhs_end+1-rel.rhs_start))

######################################################################
# Fitler Evaluation
######################################################################

def eval_filters(filters, apf_relations, serif_relations,
                 heldout_apf_relations, heldout_serif_relations,
                 alpha=0.9, sort_by='fscore'):
    apf_relmention_dict = make_apf_relmention_dict(apf_relations)
    heldout_apf_relmention_dict = make_apf_relmention_dict(heldout_apf_relations)
    
    # Get some baseline scores
    base_p, base_r, base_f = evaluate(serif_relations, apf_relmention_dict)
    
    # Evaluate the filters based on the data we trained them with.
    f_scores = [(evaluate(serif_relations, apf_relmention_dict, alpha, f), f)
                for f in filters]

    # Display them side-by-side, sorted by the f-scores that we got
    # from the training data.
    print '='*79
    print 'Alpha = %.2f' % alpha
    print '-'*79
    print '%-40s %s | %s' % ('', 'Train'.center(17), 'Heldout'.center(17))
    print '_'*40 + '___P_____R_____F___|___P_____R_____F__'
    def sortkey(scored_filter):
        scores, filter = scored_filter
        return scores[{'precision':0, 'recall':1, 'fscore':2}[sort_by]]
    for (scores, filter) in sorted(f_scores, key=sortkey, reverse=True):
        train_p, train_r, train_f = scores
        if train_f <= base_f: continue
        test_p, test_r, test_f = evaluate(heldout_serif_relations,
                                          heldout_apf_relmention_dict,
                                          alpha, filter)
        print _colwrap([40, filter.name],
                       [100, '%+5.1f %+5.1f %+5.1f | %+5.1f %+5.1f %+5.1f' % (
                           100*(train_p-base_p), 100*(train_r-base_r),
                           100*(train_f-base_f), 100*(test_p-base_p),
                           100*(test_r-base_r), 100*(test_f-base_f))])
    print '='*79

######################################################################
# Confidence Metrics
######################################################################

def _evenly_distribute(ys):
    """
    Given a list of 0's and 1's, try to distribute them 'evenly'.
    E.g., [100101111111] -> [101110111011]
    """
    ones = ys.count(1)
    zeros = ys.count(0)
    assert ones+zeros == len(ys)
    if ones==0 or zeros==0: return ys
    ratio = float(ones)/zeros
    result = []
    for i in range(len(ys)):
        if zeros==0:
            result.append(1)
        elif ones==0:
            result.append(0)
        elif float(ones)/zeros < ratio:
            result.append(0)
            zeros -= 1
        else:
            result.append(1)
            ones -= 1
    assert len(result) == len(ys)
    assert result.count(1) == ys.count(1), (result.count(1), ys.count(1))
    assert result.count(0) == ys.count(0), (result.count(0), ys.count(0))
    return result

class ConfidenceMetric(object):
    name = None
    def evaluate(self, apf_relations, serif_relations, outfile=None,
                 verbosity=0, prec_running_avg=True):
        apf_relmention_dict = make_apf_relmention_dict(apf_relations)

        x_to_ys = defaultdict(list)
        score_set = set()
        for serif_rel in serif_relations:
            confidence_score = self.score(serif_rel)
            score_set.add(confidence_score)
            if serif_rel.key in apf_relmention_dict:
                x_to_ys[confidence_score].append(1.0)
            else:
                x_to_ys[confidence_score].append(0.0)
        max_score = float(max(x_to_ys))
        min_score = float(min(x_to_ys))
        results = []
        smoothed_results = []
        for (x,ys) in sorted(x_to_ys.items()):
            # Normalize scores:
            x = (x-min_score)/(max_score-min_score)
            if verbosity>0:
                print 'x=%8.2f y=%5.2f    count=%d' % (x, avg(ys), len(ys))
            smoothed_results += [(x, avg(ys))] * len(ys)
            ys = _evenly_distribute(ys)
            #random.shuffle(ys)
            results += [(x,y) for y in ys]

        # Do a linear regression:
        gradient, y_intercept, r_value, _, _ = (
            stats.linregress(*zip(*results)))
        x_intercept = -y_intercept/gradient
        
        N = float(len(results))
        rank_gradient, _, _, _, _ = (
            stats.linregress([i/N for i in range(len(results))],
                             [r[1] for r in results]))

        top_n_prec = dict((i/100., 100*avg([r[1] for r in
                                            results[int(N*i/100):]]))
                          for i in range(100))
        bot_n_prec = dict((i/100., 100*avg([r[1] for r in
                                            results[:int(N*(i+1)/100)]]))
                          for i in range(100))

        bins = 10
        avg_prec = avg([r[1] for r in results])
        bin_prec = [avg([r[1] for r in
                         results[int(N*i/bins):int(N*(i+1)/bins)]])
                    for i in range(bins)]

        # bin_diff calculates how different each bin is from the average
        # value.  In particular, we want high scores to correspond to
        # high precisions and low scores to correspond to low precisions.
        def bin_diff_for_intersect(intersect):
            return (sum((avg_prec-bp) for bp in bin_prec[:intersect]) +
                    sum((bp-avg_prec) for bp in bin_prec[intersect:]))
        bin_diff = max(bin_diff_for_intersect(i) for i in range(11))
        #for i in range(11):
        #    print 'intersect %d: %.3f' % (i, bin_diff_for_intersect(i))

        # bin_diff_2 calculates how closely the binned scores approximate
        # a streight line, starting at a low precision and ending at a
        # high precision.  Low numbers are good.
        bin_diff_2 = sum([abs(bp-float(i)/(bins-1)*bin_prec[-1])
                          for (i, bp) in enumerate(bin_prec)])
        #print 'bin diff 2:'
        #for (i, bp) in enumerate(bin_prec):
        #    want = float(i)/(bins-1)*bin_prec[-1]
        #    print 'bin %d: %.4f-%.4f = %+.4f' % (i, want, bp, want-bp)
        
        if verbosity>=0:
            window = len(smoothed_results)/50
            if prec_running_avg:
                prec_results = running_avg(smoothed_results, window)
                prec_title = 'Prec vs Score (Running avg)'
            else:
                prec_results = smoothed_results
                prec_title = 'Precision vs Score'
            print (' %s ' % self.name).center(75, '=')
            print
            print columns(
                draw_histogram(prec_results, title=prec_title,
                               combine=avg, width=28, max_bar=1.0,
                               min_value=0.0, max_value=1.0),
                draw_histogram([(float(i)/N, 100.*r[1]) for (i,r)
                                in enumerate(results)],
                                 combine=avg, width=28,
                               max_bar=100, title='Precision vs Rank'),
                align='bottom')
            print columns(
                draw_histogram([r[0] for r in results], width=28,
                               min_value=0.0, max_value=1.0,
                               normalize=True, title='Density'),
                #draw_histogram(top_n_prec, combine=avg, width=28, max_bar=100,
                #               title='(Precision|rank>i) vs rank',
                #               min_value=0.0, max_value=1.0),
                draw_histogram([(float(i)/N, r[0]) for (i,r)
                                in enumerate(results)],
                                 combine=avg, width=28,
                               max_bar=1, title='Score vs Rank'),
                align='bottom')
            
            print columns(
                '   ',
                ('Prec(top 10%%):%3d%%\n' % top_n_prec[1-.1] +
                 'Prec(top 25%%):%3d%%\n' % top_n_prec[1-.25] +
                 'Prec(top 50%%):%3d%%\n' % top_n_prec[1-.50] +
                 'Prec(bot 50%%):%3d%%\n' % bot_n_prec[.50] +
                 'Prec(bot 25%%):%3d%%'   % bot_n_prec[.25]),
                ('     Gradient:%6.3f\n'  % gradient +
                 '    R-Squared:%6.3f\n'  % (r_value**2) +
                 'Rank Gradient:%6.3f\n'  % rank_gradient +
                 '  x-intercept:%6.3f\n'  % x_intercept +
                 '  y-intercept:%6.3f'    % y_intercept),
                ('     Bin Diff:%5.2f (high=good)\n'  % bin_diff +
                 '   Bin Diff 2:%5.2f (low=good)\n'   % bin_diff_2 +
                 '      bin[-1]:%5.2f (high=good)\n'    % bin_prec[-1] +
                 '    max score: %.2f\n' % max_score +
                 'unique scores: %d' % len(score_set)),
                #draw_histogram(list(enumerate(bin_err)),
                #               width=len(bin_err), max_bar=1,
                #               title='Bin Error', height=5),
                )

        if outfile:
            out = open(outfile, 'wb')
            for x,y in results:
                out.write('%f\t%f\n' % (x,y))
            out.close()

        # What should we use to pick a good metric??
        return bin_prec[-1] - bin_diff_2
        #return bin_diff

    def score(self, serif_rel):
        raise AssertionError('abstract method')


class ScoredFilterConfidenceMetric(ConfidenceMetric):
    SCORE = 'fscore'
    def __init__(self, apf_relations, serif_relations, filters,
                 alpha=0.9, num_filters=5, cutoff=0.1):
        self.scored_filters = []
        
        apf_relmention_dict = make_apf_relmention_dict(apf_relations)
        base_p, base_r, base_f = evaluate(serif_relations, apf_relmention_dict)
        for filter in filters:
            p, r, f = evaluate(serif_relations, apf_relmention_dict,
                               alpha, filter)
            if self.SCORE == 'precision':
                score = 100.*(p-base_p)
            elif self.SCORE == 'fscore':
                score = 100.*(f-base_f)
            else:
                assert 0, 'bad self.SCORE'
            if f > base_f and score > cutoff:
                self.scored_filters.append( (filter, score) )
                
        self.scored_filters.sort(key=lambda fs:-fs[1])
        self.scored_filters = self.scored_filters[:num_filters]
        self.name = 'FilterMetric(alpha=%s; cutoff=%s%%; %d filters)' % (
            alpha, cutoff, len(self.scored_filters))

    NUM_FILTERS_TO_SHOW = 10
    def evaluate(self, *args, **kwargs):
        verbosity = kwargs.get('verbosity', 0)
        if not self.scored_filters:
            if verbosity >= 0:
                print (' %s ' % self.name).center(75, '=')
                print '  (No filters!)'
            return 0
        else:
            cm_score = ConfidenceMetric.evaluate(self, *args, **kwargs)
        if verbosity >= 0 and self.NUM_FILTERS_TO_SHOW>0:
            print '  Filters:'
            for filter, score in self.scored_filters[:self.NUM_FILTERS_TO_SHOW]:
                print textwrap.fill('%8.3f %s' % (score, filter.name),
                                    initial_indent=' '*2,
                                    subsequent_indent=' '*10)
            if len(self.scored_filters) > self.NUM_FILTERS_TO_SHOW:
                print '         ... %d more filters...' % (
                    len(self.scored_filters) - self.NUM_FILTERS_TO_SHOW)
        return cm_score
            
    def score(self, serif_rel):
        return sum(filter_score
                   for (filter, filter_score) in self.scored_filters
                   if filter.check(serif_rel))

class ScoredFilterConfidenceMetric2(ScoredFilterConfidenceMetric):
    """
    This version picks filters one at a time, and then applies them to
    the dev set.  That way, the scores assigned to subsequent filters
    are appropriately adjusted.
    """
    SCORE='fscore'
    def __init__(self, apf_relations, serif_relations, filters,
                 alpha=0.9, num_filters=5, cutoff=0.1):
        apf_relmention_dict = make_apf_relmention_dict(apf_relations)
        base_p, base_r, base_f = evaluate(serif_relations, apf_relmention_dict)
        
        self.scored_filters = []
        for i in range(num_filters):
            # Score the filters:
            def score_filter(f):
                return evaluate(serif_relations, apf_relmention_dict, alpha, f)
            scored_filters = [(f, 100.*(score_filter(f)[2]-base_f))
                              for f in filters]
            # Pick the best one:
            scored_filters.sort(key=lambda fs:-fs[1])
            filter, score = scored_filters[0]
            if score <= 0: continue
            self.scored_filters.append(scored_filters[0])
            # Apply the filter:
            serif_relations = [r for r in serif_relations if filter.check(r)]
        self.scored_filters.sort(key=lambda fs:-fs[1])
            
        self.name = 'FilterMetric2(alpha=%s; cutoff=%s%%; %d filters)' % (
            alpha, cutoff, len(self.scored_filters))
    
class MaxentConfidenceMetric(ConfidenceMetric):
    name = 'Maxent'
    def score(self, serif_rel):
        return maxent_score(serif_rel)
        
class SerifConfidenceMetric(ConfidenceMetric):
    name = "SERIF"
    def score(self, serif_rel):
        return float(serif_rel.confidence)
        
######################################################################
# Helper Functions
######################################################################

def columns(*blocks, **kwargs):
    align = kwargs.pop('align', 'top')
    if kwargs: raise TypeError('Unexpected kwarg %s' % kwargs.keys()[0])
    s = ''
    blocks = [block.split('\n') for block in blocks]
    colwidths = [max(len(line)+1 for line in block) for block in blocks]
    if align == 'bottom':
        height = max(len(block) for block in blocks)
        blocks = [['']*(height-len(block))+block for block in blocks]
    for lines in map(None, *blocks):
        for colwidth, line in zip(colwidths, lines):
            s += (line or '').ljust(colwidth) + ' '
        s += '\n'
    return s

def running_avg(xy_pairs, window=50, debug=False):
    result = []
    for i in range(window/2):
        result.append( (avg([x for (x,y) in xy_pairs[:i+window/2]]),
                        avg([y for (x,y) in xy_pairs[:i+window/2]])) )
        if debug:
            print '%3d [%3d:%3d]  (%5.2f,%5.2f)  ->  (%5.2f, %5.2f)' % (
                i, 0, i+window/2, xy_pairs[i][0], xy_pairs[i][1],
                result[-1][0], result[-1][1])
    for i in range(window/2, len(xy_pairs)-window/2):
        start = max(0, i-window/2)
        stop = min(len(xy_pairs)-1, i+window/2)
        result.append((avg([x for (x,y) in xy_pairs[start:stop]]),
                       avg([y for (x,y) in xy_pairs[start:stop]])))
        if debug:
            print '%3d [%3d:%3d]  (%5.2f,%5.2f)  ->  (%5.2f, %5.2f)' % (
                i, start, stop, xy_pairs[i][0], xy_pairs[i][1],
                result[-1][0], result[-1][1])
    for i in range(len(xy_pairs)-window/2, len(xy_pairs)):
        result.append( (avg([x for (x,y) in xy_pairs[i:]]),
                        avg([y for (x,y) in xy_pairs[i:]])) )
        if debug:
            print '%3d [%3d:%3d]  (%5.2f,%5.2f)  ->  (%5.2f, %5.2f)' % (
                i, i, len(xy_pairs), xy_pairs[i][0], xy_pairs[i][1],
                result[-1][0], result[-1][1])
    return result


######################################################################
# Data Loading
######################################################################

def _filter_by_docid(relations, docids, name):
    result = [r for r in relations if r.docid in docids]
    print '%s: %d->%d' % (name, len(relations), len(result))
    print '  (%f%%)' % (100.*len(result)/len(relations))
    return result

def load_data(serif_root, apf_roots, docid_filter_file=None):
    print 'Loading serif output...'
    serif_relation_decisions = load_serif_relations(serif_root)
    print 'Loading APF (eval) data...'
    apf_data = apf.read_apf(APF_ROOTS)
    apf_relations = sum([r.relmentions for r in apf_data.values() if
                         hasattr(r, 'relmentions')], [])

    # If requested, then filter out any relations that aren't in the
    # given list of docids.
    if docid_filter_file is not None:
        train_docids = set(open(docid_filter_file).read().split())
        serif_relation_decisions = _filter_by_docid(serif_relation_decisions,
                                                    train_docids, 'Serif')
        apf_relations = _filter_by_docid(apf_relations, train_docids, 'Apf')

    # If there are any files that we have serif output for, but do not
    # have any apf output for, then discard that serif output --
    # otherwise, it will show up as a false positive.  Similarly, if
    # we have any files that we have apf output for but no serif
    # output, then discard them.
    apf_docids = set(v.docid for v in apf_data.values())
    serif_docids = set(r.docid for r in serif_relation_decisions)
    serif_extras = serif_docids-apf_docids
    if serif_extras:
        print ('Discarding unmatched serif output for %d/%d docs' %
               (len(serif_extras), len(serif_docids)))
        serif_relation_decisions = [r for r in serif_relation_decisions
                           if r.docid not in serif_extras]
    apf_extras = apf_docids-serif_docids
    if apf_extras:
        print ('Discarding unmatched apf output for %d/%d docs' %
               (len(apf_extras), len(apf_docids)))
        apf_relations = [r for r in apf_relations
                           if r.docid not in apf_extras]

    return serif_relation_decisions, apf_relations

# Pick some heldout documents for later evaluation.
def split_data(serif_relations, apf_relations):
    all_docids = list(set(r.docid for r in serif_relations))
    random.shuffle(all_docids)
    N = len(all_docids)/5
    dev_docids = set(all_docids[N:])
    heldout_docids = set(all_docids[:N])

    print 'Selecting dev set and heldout set'
    print '%6d dev docs' % len(dev_docids)
    print '%6d heldout docs' % len(heldout_docids)
    heldout_serif_relations = [r for r in serif_relations
                               if r.docid in heldout_docids]
    serif_relations = [r for r in serif_relations
                       if r.docid not in heldout_docids]
    heldout_apf_relations = [r for r in apf_relations
                               if r.docid in heldout_docids]
    apf_relations = [r for r in apf_relations
                       if r.docid not in heldout_docids]
    return ((serif_relations, apf_relations),
            (heldout_serif_relations, heldout_apf_relations))

######################################################################
# Decide Which Filters to Use
######################################################################

def choose_filter_candidates(apf_relations, serif_relations):
    filter_makers = [
        # Filters based on entity types & mention types
        DiscreteFilterMaker('lhs_entity_type',
                            (lambda r: r.lhs_entity_type)),
        DiscreteFilterMaker('lhs_entity_subtype',
                            (lambda r: r.lhs_entity_subtype)),
        DiscreteFilterMaker('rhs_entity_type',
                            (lambda r: r.rhs_entity_type)),
        DiscreteFilterMaker('rhs_entity_subtype',
                            (lambda r: r.rhs_entity_subtype)),
        DiscreteFilterMaker('entity_types', entity_types),
        DiscreteFilterMaker('lhs_mention_type', (lambda r: r.lhs_mention_type)),
        DiscreteFilterMaker('rhs_mention_type', (lambda r: r.rhs_mention_type)),
        DiscreteFilterMaker('mention_types', mention_types),
        # Filters based on the maxent model
        CutoffFilterMaker('maxent_score', maxent_score, slices=100),
        CutoffFilterMaker('max_pro_rel_feature', max_pro_rel_feature),
        CutoffFilterMaker('max_anti_rel_feature', max_anti_rel_feature),
        # Other features
        DiscreteFilterMaker('overlap', overlap_type),
        CutoffFilterMaker('arglen', arglen, slices=30),
        DiscreteFilterMaker('nested_args', nested_args),
        CutoffFilterMaker('argdist', argdist, slices=30),
        # Check for the presence of features used by maxent/p1 models
        FeatureFilterMaker(),
        # Check some characterstics of how we assembled the answer:
        BinaryFilterMaker('use_p1_answer', lambda r:r.use_p1_answer),
        BinaryFilterMaker('use_cvec_answer', lambda r:r.use_cvec_answer),
        BinaryFilterMaker('secondary_selected', lambda r:r.secondary_selected),
        BinaryFilterMaker('use_p1_answer_vec_tree',
                          lambda r:r.use_p1_answer_vec_tree),
        DiscreteFilterMaker('ethnic_heuristic', lambda r:r.ethnic_heuristic),
        CutoffFilterMaker('reversed', (lambda r:r.reversed-r.not_reversed)),
        ]

    filters = []

    # Explore lots of different alpha's.
    filters += choose_filters_2(apf_relations, serif_relations,
                                filter_makers, nbest=10,
                                alphas=[0.01*n for n in range(30, 100)],
                                display_alphas=[0.99, 0.95, 0.9, 0.5])

    # Find some very-high-precision filters
    filters += choose_filters(apf_relations, serif_relations, filter_makers,
                              depth=1, width=20, narrowing_factor=1./20,
                              alpha=0.999)
    filters += choose_filters(apf_relations, serif_relations, filter_makers,
                              depth=1, width=20, narrowing_factor=1./20,
                              alpha=0.99)
    # Find some good filters, emphasizing precision
    filters += choose_filters(apf_relations, serif_relations, filter_makers,
                              depth=50, width=1,
                              alpha=0.9)
    # Find some good filters, emphasizing precision a little less
    filters += choose_filters(apf_relations, serif_relations, filter_makers,
                              depth=50, width=1,
                              alpha=0.8)
    # See what we find if we maximize F(0.5):
    filters += choose_filters(apf_relations, serif_relations, filter_makers,
                              depth=50, width=1,
                              alpha=0.5)
    filters += choose_filters(apf_relations, serif_relations, filter_makers,
                              depth=5, width=5,
                              alpha=0.5)
    
    # Remove any duplicate filters (assume names are unique):
    filters = dict((f.name, f) for f in filters).values()

    return filters

######################################################################
# Decide Which Metrics to Use
######################################################################

def choose_metrics(dev_apf_relations, dev_serif_relations,
                   heldout_apf_relations, heldout_serif_relations,
                   filters, num_metrics=5, verbose=True):
    #NUM_FILTERS = [5, 10, 20, 30, 40, 50, 1000]
    #NUM_FILTERS = [5, 10, 20, 30, 40, 50]
    NUM_FILTERS = [10, 11, 12, 13, 14, 15]
    ALPHA = [0.5, 0.8, 0.9, 0.95, 0.98, 0.99, 0.999, 0.9999]
    #NUM_FILTERS = [5, 30]
    #ALPHA = [0.8, 0.9]

    metrics = []
    # Try out various meta-parameters:
    for num_filters in NUM_FILTERS:
        for alpha in ALPHA:
            for metrictype in [ScoredFilterConfidenceMetric,
                               ScoredFilterConfidenceMetric2
                               ]:
                cm = metrictype(
                    dev_apf_relations, dev_serif_relations,
                    filters, alpha=alpha, num_filters=num_filters)
                cm_score = cm.evaluate(heldout_apf_relations,
                                       heldout_serif_relations,
                                       verbosity=-1)
                print '[%.3f] %s' % (cm_score, cm.name)
                metrics.append( (cm_score, cm) )

    metrics.sort()
    metrics = [cm for (cm_score,cm) in metrics[-num_metrics:]]
    if verbose:
        display_metrics(metrics, heldout_apf_relations,
                        heldout_serif_relations)
    return metrics

def display_metrics(metrics, heldout_apf_relations, heldout_serif_relations):
    for cm in metrics:
        cm.__class__ = globals()[type(cm).__name__]
        cm.evaluate(heldout_apf_relations, heldout_serif_relations)


######################################################################
# Main Script:
######################################################################
# We do all this in the global namespace to make it easier to
# experiment with interactively (using emacs python-mode).  All
# the "if 1:" or "if 0:" statements are so I can repeatedly send
# this buffer to a python interactive shell and not repeat steps
# I've already done (when experimenting with other steps).  If
# you're running this from scratch, then all the "if 0"'s should
# probably be changed to "if 1".

SERIF_ROOT = r'//traid07/u16/eloper/runjobs/expts/relation-confidence/'
APF_ROOTS = [r'//traid01/speed/Ace/data-2005-v4.0/English',
             (r'//traid01/speed/Training-Data/English/'+
              r'bbn-ace2005-relation-training/fixed-apf')]
DOCID_FILTER_FILE = (r'//titan3/u70/users/eloper/code/relation-confidence/'
                     r'lib/train_docids')

# Disable this block if you want to skip data loading.
if 0:
    print 'Loading data...'
    serif_relation_decisions, apf_relations = load_data(SERIF_ROOT, APF_ROOTS)
    serif_relations = [r for r in serif_relation_decisions
                       if r.answer != 'NONE']
if 0:
    print 'Max recall (over all data): %.1f%%' % (100*max_recall(
        serif_relation_decisions, apf_relations))

# Split the data into a dev set and a heldout set.
if 0:
    ((dev_serif_relations, dev_apf_relations),
     (heldout_serif_relations, heldout_apf_relations)) = split_data(
        serif_relations, apf_relations)

if 0:
    print '\nChoosing filter candidates...'
    filters = choose_filter_candidates(dev_apf_relations, dev_serif_relations)

# Display the performance of individual filters:
if 0:
    print '\nEvaluating filter candidates...'
    eval_filters(filters, dev_apf_relations, dev_serif_relations,
                 heldout_apf_relations, heldout_serif_relations,
                 alpha=0.95)


# Choose a few good metric candidates and display them.
if 0:
    print '\nChoosing metrics...'
    metrics = choose_metrics(dev_apf_relations, dev_serif_relations,
                             heldout_apf_relations, heldout_serif_relations,
                             filters, num_metrics=10, verbose=False)

if 0:
    display_metrics(metrics, heldout_apf_relations, heldout_serif_relations)

if 0:
    m = metrics[-1]
    m.NUM_FILTERS_TO_SHOW = 0
    #print 'On development data:'
    #display_metrics([m], dev_apf_relations, dev_serif_relations)
    m.NUM_FILTERS_TO_SHOW = 100
    #print 'On heldout data:'
    display_metrics([m], heldout_apf_relations, heldout_serif_relations)

######################################################################
# SANITY CHECK
######################################################################
# This code checks to make sure that the metric is doing what
# it's supposed to.  It assumes that 'm' contains the selected
# metric, and that that metric is the same one that's used by
# SERIF to calculate its confidence score.

if 1:
    display_metrics([SerifConfidenceMetric()],
                    apf_relations, serif_relations)

if 0:
    #m = metrics[-1]
    NORMALIZE_FACTOR = 124.
    filter_count = dict((f,0) for (f,s) in m.scored_filters)
    for rel in serif_relations:
        for (f,s) in m.scored_filters:
            if f.check(rel): filter_count[f] += 1
        lhs = '\n'.join(sorted('  %.3f %s' % (s/NORMALIZE_FACTOR,f.name)
                               for (f,s) in m.scored_filters
                               if f.check(rel)))
        rhs = '\n'.join(sorted(rel.confidence_breakdown.rstrip().split('\n')))
        if rel.ethnic_heuristic and float(rel.confidence)==1:
            continue
        
        if abs(float(rel.confidence) -
               min(1, m.score(rel)/NORMALIZE_FACTOR)) > 0.01:
            print 'WARNING: Python & SERIF metrics disagree:'
            print columns(
                '%s\nScore: %.3f\n%s' % ('Python Metric'.center(30),
                                         m.score(rel)/NORMALIZE_FACTOR, lhs),
                '%s\nScore: %.3f\n%s' % ('SERIF Metric'.center(30),
                                         float(rel.confidence), rhs)
                )

    print 'Filter counts:'
    filter_scores = dict(m.scored_filters)
    for (f, c) in sorted(filter_count.items(), key=lambda kv:-kv[1]):
        print '%5d%% [%5.3f] %s' % (
            100.*c/len(serif_relations),
            filter_scores[f]/NORMALIZE_FACTOR, f.name)
