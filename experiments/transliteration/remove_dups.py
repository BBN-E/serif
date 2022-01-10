"""
Python script to remove duplicate entries from a name transliteration
mapping file.  A name transliteration mapping file is a file that maps
names in a non-English language to English equivalents.  It is used
by SERIF during name transliteration.  These files should be stored
in UTF-8, and should contain one transliteration per line.  Each line
should contain a non-English name, followed by a tab character, followed
by an English name.  

This script scans through an existing transliteration file, and checks
for duplicates.  It then chooses a unique English transliteration for
each non-English name, using heuristics based on a large unlabeled 
corpus (mostly frequency).

Currently, there is no command-line interface; you must modify the 
constants at the top of this file and then run it.  (Todo: add a 
command-line interface.)
"""

import codecs, collections, re, os, sys

#######################################################
# INPUT PARAMETERS
#######################################################

# The source file (contains duplicates):
SRC_FILE = '/nfs/titan/u70/users/abaron/Projects/Distillation/docs/Arabic_Wikipedia_Interlinks.txt'

# The destination file (duplicates are removed):
DST_FILE = '/tmp/x/Arabic_Wikipedia_Interlinks.txt'

# A corpus used to find dst name frequencies:
LARGE_CORPUS = '/nfs/titan/u70/Distillation/GALEY4/source/english_newswire/' 

#######################################################
# CONSTANTS
#######################################################
LARGE_CORPUS_EXT = '.sgm'
LARGE_CORPUS_ENCODING = 'utf-8'
SRC_ENCODING = 'utf-16'
DST_ENCODING = 'utf-8'

# Read everything in.
print 'Reading name map'
name_map = collections.defaultdict(set)
for line in codecs.open(SRC_FILE, encoding=SRC_ENCODING):
    line=line.strip()
    if not line: continue
    src_name, dst_name = line.split('\t')
    name_map[src_name].add(dst_name)

# Select out the ambiguous names.
ambiguous = {}        # src_name -> dst_names
unambiguous = {}      # src_name -> dst_name
dst_name_count = {}   # dst_name -> count
dst_name_count_i = {} # dst_name.lower() -> count
too_ambiguous = 0
print 'Sorting through names'
while name_map:
    src_name, dst_names = name_map.popitem()
    if len(dst_names) > 20:
        print 'Discarding name with excess ambiguity (%d)' % len(dst_names)
        too_ambiguous += 1
    elif len(dst_names) > 1:
        ambiguous[src_name] = dst_names
        for dst_name in dst_names:
            dst_name_count[dst_name] = 0
            dst_name_count_i[dst_name.lower()] = 0
    else:
        unambiguous[src_name] = dst_names.pop()
orig_num_unambiguous = len(unambiguous)
orig_num_ambiguous = len(ambiguous)
print len(unambiguous), 'unambiguous names'
print len(ambiguous), 'ambiguous names'
print too_ambiguous, 'names were discarded because they are too ambiguous'
print len(dst_name_count), 'names to look up'

name_regex = dict((name, re.compile(r'\b%s\b' % re.escape(name)))
                  for name in dst_name_count)
name_regex_i = dict((name, re.compile(r'\b%s\b' % re.escape(name), 
                                      re.IGNORECASE))
                    for name in dst_name_count_i)

# Count how many times each word occurs.
ndocs = 0
try:
    for dir, subdirs, files in os.walk(LARGE_CORPUS):
        for filename in files:
            if filename.endswith(LARGE_CORPUS_EXT):
                s = codecs.open(os.path.join(dir, filename), 
                                encoding=LARGE_CORPUS_ENCODING).read()
                s_lower = s.lower()
                for dst_name in dst_name_count:
                    if dst_name.lower() in s_lower:
                        # Case sensitive count:
                        dst_name_count[dst_name] += (
                            len(name_regex[dst_name].findall(s)))
                        # Case insensitive count:
                        dst_name_count_i[dst_name.lower()] += (
                            len(name_regex_i[dst_name.lower()].findall(s_lower)))
                ndocs += 1
                if ndocs%10 == 0:
                    sys.stdout.write('.'); sys.stdout.flush()
                if ndocs%200 == 0: 
                    print sum(dst_name_count.values()), sum(dst_name_count_i.values())
                    print ' avg:%6.1f   unseen-dst:%5d   unseen-src:%5d' % (
                        1.*sum(dst_name_count.values())/len(dst_name_count),
                        sum(v==0 for v in dst_name_count.values()),
                        sum(sum(dst_name_count[n] 
                                for n in ambiguous[src_name])==0
                            for src_name in ambiguous))
except KeyboardInterrupt:
    print '\nKeyboard Interrupt!'
print

ambiguous_items = sorted(ambiguous.items(),
                         key=lambda sd: sum(dst_name_count[n] for n in sd[1]))
for src_name, dst_names in ambiguous_items:

    # Reject zero-count names
    dst_names = [n for n in dst_names if dst_name_count[n]>0]

    # Reject translations that usually occur in lower case:
    dst_names = [n for n in dst_names 
                 if dst_name_count[n] > 0.5*dst_name_count_i[n.lower()]]

    # Sort the remaining names:
    dst_names = sorted(dst_names, key=dst_name_count.__getitem__, reverse=True)

    # Add the most common translation to the unambiguous list
    if len(dst_names)==1 or len(dst_names)>1 and (dst_name_count[dst_names[0]] >
                                                  dst_name_count[dst_names[1]]):
        unambiguous[src_name] = dst_names[0]
        ambiguous.pop(src_name)

print '\n---------------'
print 'Resolved  %4d/%d ambiguous names' % (
    len(unambiguous)-orig_num_unambiguous, orig_num_ambiguous)
print 'Discarded %4d/%d ambiguous names' % (
    orig_num_ambiguous-(len(unambiguous)-orig_num_unambiguous), orig_num_ambiguous)

# Generate our output
out = codecs.open(DST_FILE, 'wb', encoding=DST_ENCODING)
for src_name, dst_name in unambiguous.items():
    out.write('%s\t%s\n' % (src_name, dst_name))
out.close()

    
