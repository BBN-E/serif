#!/bin/env python
#
# Extract the total processing time from a given log file, and compare
# it to the number of bytes in the processed source files to arrive at
# an overall throughput speed.  Save the output to OUT_FILE

import re, os, sys
from collections import defaultdict

LOGFILES = r'''+logfiles+'''.split('\n')
BATCH_INPUT_FILE = r'+batch_input_file+'
TPUT_FILE = r'+tput_file+'

parent_dir = os.path.split(os.path.abspath(TPUT_FILE))[0]
if not os.path.exists(parent_dir):
    os.makedirs(parent_dir)

# This dictionary is used to normalize some section names -- e.g.,
# the classic & modularized versions of serif use different names
# for the parse time.
CANONICAL_SECTION_NAME = {
    # Document Driver
    'Sentence Analysis Engine': 'sentence',
    'SGML Zoner Analysis Engine': 'zoner',
    # Sentence Driver
    'Syntactic Parse Analysis Engine': 'parse',
    'DocRelationsEvents Analysis Engine': 'doc-relations-events',
    'Values Analysis Engine': 'values',
    'Token Analysis Engine': 'tokens',
    'DocValues Analysis Engine': 'doc-values',
    'Name Recognizer Analysis Engine': 'names',
    'Propositions Analysis Engine': 'props',
    'Mentions Analysis Engine': 'mentions',
    'NPChunk Analysis Engine': 'npchunk',
    'Metonymy Analysis Engine': 'metonymy',
    'Part-Of-Speech Analysis Engine': 'part-of-speech',
    'Reference Resolver Analysis Engine': 'xdoc', # is this right?
    'GenericsFilter Analysis Engine': 'generics',
    }

# Determine the total number of bytes in the source files.
source_files = [f.strip() for f in open(BATCH_INPUT_FILE).readlines()]
source_files = [re.sub(r'^//', '/nfs/', f) for f in source_files]
bytes = sum(os.stat(f).st_size for f in source_files if f)

# Extract the total processing time from the log file.  We only grab
# times if they occur in a '* Processing Time' section; and we don't
# include any times associated with scoring.
processing_time = 0.0
processing_time_by_section = defaultdict(float)
for logfile in LOGFILES:
    # Get the session log.
    print 'reading logfile: %r' % logfile
    m = re.search('Session log is in: (.*)', open(logfile, 'rb').read())
    assert m, 'No session log found??'
    session_log = m.group(1)
    print 'reading session log: %r' % session_log
    section = None
    for line in open(session_log):
        line = line.rstrip()
        #print `line`
        m = re.match(r'(?m)^\s*Process\(exc.score\)\s*(.*) msec$', line)
        if line.startswith('Process(exc.'): assert m, `line`
        if m:
            assert section is None
            processing_time = float(m.group(1))
            #print '  -> TOTAL TIME'
        m = re.match('\s*(\w+) (Process(?:ing)?|Load) Time', line)
        if m:
            if m.group(2).startswith('Process'):
                section = m.group(1)
                #print '  -> SECTION'
            else:
                section = None # Ignore load times for throughput.
        if section and 'score' not in line:
            m = re.match('\s*(.*)\t(.*) msec', line)
            if m:
                #print '  -> TIME'
                t = float(m.group(2))
                subsection = m.group(1)
                #processing_time += t
                subsection = CANONICAL_SECTION_NAME.get(subsection, subsection)
                section_name = '%s: %s' % (section, subsection)
                processing_time_by_section[section_name] += t

# Get throughput in MB/hour.  3.6=(3,600,000 msec/hr)/(1,000,000mb/byte)
tput = 3.6*bytes/processing_time

# Display and save the result.
result = ''
for (section, sec_time) in sorted(processing_time_by_section.items(),
                                  key=lambda kv:kv[1]):
    result += 'Time (%s): %8d msec\n' % (section, sec_time)
result += ('Time: %8d msec\n'    % processing_time +
           'Size: %8d bytes\n'   % bytes +
           'Tput: %8.2f MB/hr\n' % tput)
print result
out = open(TPUT_FILE, 'wb')
out.write(result)
out.close()
