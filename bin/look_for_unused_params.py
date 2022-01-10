import os
import re
import sys

param_file_re = re.compile('(.*): ')
std_out_re = re.compile('ParamReader::outputUsedParamName\((.*)\)')


param_file, std_out = sys.argv[1:]

param_file_names = set()
lines = open(param_file).readlines()
for line in lines:
    if line[0] == '#':
        continue
    match = param_file_re.search(line)
    if match:
        param_file_names.add(match.group(1))
print "Read %d unique param names from param_file" % len(param_file_names)

std_out_names = set()
lines = open(std_out).readlines()
for line in lines:
    match = std_out_re.search(line)
    if match:
        std_out_names.add(match.group(1))
print "Read %d unique param names from std_out" % len(std_out_names)

for name in param_file_names:
    if name not in std_out_names:
        print "Only in param file: %s" % name

# Don't worry about these, we have lots of optional parameters that we need to
# check, and most of them aren't set in the param file
#for name in std_out_names:
#    if name not in param_file_names:
#        print "Only in std_out: %s" % name
