#!/bin/env python

import os, sys, re

os.chdir(r"+input_dir+")
os.makedirs(os.path.split(r"+document_list+")[0])
out = open(r"+document_list+", 'wb')
print r'Writing output to +document_list+.'

n = 0
name_map = {}
for sgm_dir in r"+sgm_subdirs+".split():
    print 'Searching in %s...' % sgm_dir
    sys.stdout.flush()
    for root, subdirs, files in os.walk(sgm_dir):
        for filename in files:
            if filename.endswith('sgm'):
                # Check for duplicates.
                name_map.setdefault(filename,[]).append(root)
                if len(name_map[filename]) > 1: continue
                # Write the file to the file list.
                out.write(os.path.join(root, filename)+'\n')
                n += 1
                if n%100 == 0:
                    print '  [%5d]' % n
                    sys.stdout.flush()
print 'Found %d sgm files' % n
sys.stdout.flush()
out.close()

# Complain about duplicates.
for (filename,dirs) in name_map.items():
    if len(dirs) > 1:
        print 'Duplicates for %s:' % filename
        for dirname in dirs:
            print '  - %s' % dirname
