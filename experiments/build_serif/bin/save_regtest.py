#!/bin/env python
#
# Save SERIF Regression Test Output
#
import os, sys, re, shutil, optparse
from collections import defaultdict

TESTDATA_HOME = '/d4m/serif/testdata'

VERBOSE = False

def copy_testdata_dirs(src, dst, cpy_dirs, dry_run):
    # Create the target directory.
    if dry_run:
        print "mkdir %r" % os.path.join(dst)
    else:
        os.makedirs(os.path.join(dst))
    for cp_dir in cpy_dirs:
        #copy the extra scoring data
        src_subdir = os.path.join(src, cp_dir)
        dst_subdir = os.path.join(dst, cp_dir)
        if os.path.exists(src_subdir):
            if dry_run:
                print" shutil.copytree(%s, %s)" % (src_subdir, dst_subdir)
            else:
                shutil.copytree(src_subdir, dst_subdir)
 

def copy_testdata(src, dst, score_only, dry_run):
    # Create the target directory.
    if dry_run:
        print "mkdir %r" % os.path.join(dst)
    else:
        os.makedirs(os.path.join(dst))
    # Copy the score file.
    src_file = os.path.join(src, 'score-summary.txt')
    dst_file = os.path.join(dst, 'score-summary.txt')
    if os.path.exists(src_file):
        if dry_run:
            print "cp %r %r" % (src_file, dst_file)
        else:
            shutil.copy(src_file, dst_file)
    elif src[-2][:3] == "Ace":
        print 'Warning: no score-summary.txt found in:\n    %s' % src
    # Copy the output dir (and other needed dirs)
    if not score_only:
        for tail in ("output", "compare_events"):
            src_subdir = os.path.join(src, tail)
            dst_subdir = os.path.join(dst, tail)
            if os.path.exists(src_subdir):
                if dry_run:
                    print "cp -r %r %r" % (src_subdir, dst_subdir)
                else:
                    shutil.copytree(src_subdir, dst_subdir)
            elif tail == "output":
                print 'Warning: no output subdir found in:\n    %s' % src


USAGE = """\
Usage: %prog EXPT_DIR YYMMDD

EXPT_DIR: The path to the build_serif experiment whose results you wish to
            save.  E.g.: /d4m/serif/daily_build/SerifDaily20110926.010519
  YYMMDD: The date that should be used to identify the results.  
            E.g.: 110926 (for Sept 26, 2011)."""
def main():
    parser = optparse.OptionParser(usage=USAGE)
    parser.add_option("--dry-run", dest='dry_run', action='store_true',
                      help="Don't actually copy anything; just print "
                      "what would be copied.")
    parser.add_option("--verbose", "-v", dest='verbose', action='store_true',
                      help="Generate verbose output.")
    parser.add_option("--score-only", dest='score_only', action='store_true',
                      help="Copy only score files (not output files)")
    (options, args) = parser.parse_args()
    if len(args) != 2:
        parser.error("Expected two arguments")
    (exptdir, date) = args
    if not re.match('^\d\d\d\d\d\d$', date):
        parser.error("Date must have format YYMMDD")
    if not os.path.isdir(exptdir):
        parser.error("%r is not a directory" % exptdir)
    test_dir = os.path.join(exptdir, 'expts', 'test')
    if not os.path.exists(test_dir):
        parser.error("%r not found" % test_dir)
    if not os.path.isdir(test_dir):
        parser.error("%r is not a directory" % test_dir)
    
    revisions = os.listdir(test_dir)
    if len(revisions) == 0: 
        parser.error("%r is empty" % test_dir)
    if len(revisions) > 1:
        parser.error("%r contains multiple svn revisions: %s" % 
                     (test_dir, ', '.join(revisions)))

    datedir = os.path.join(TESTDATA_HOME, 'regtest', date)
    if os.path.exists(datedir):
        if options.dry_run:
            print "date dir %s already exists" % datedir
        else:
            parser.error('Error: %s already exists!' % datedir)
                      
    num_dirs = 0
    empty_dirs = 0
    revision = sorted(revisions)[0]
    root = os.path.join(test_dir, revision)
    num_files_per_config = defaultdict(int)
    print 'Saving regression test output...'
    print '    Source: %s' % exptdir
    print '      Dest: %s' % datedir
    print '  Revision: %s' % revision
    for directory, subdirs, files in os.walk(root):
        do_files = 'session-log.txt' in files
        do_dirs = 'compare' in  subdirs or 'display' in subdirs
        if do_files or do_dirs:
            relpath = directory[len(root):].strip('/')
            src = directory
            dst = os.path.join(datedir, relpath)
            config_key = (os.path.split(src)[0], os.path.split(dst)[0])
            if do_files:
                copy_testdata(src, dst, options.score_only, options.dry_run)
                num_files_per_config[config_key] += 1
            if do_dirs:
                copy_testdata_dirs(src, dst, ('compare', 'display'), options.dry_run)
                num_files_per_config[config_key] += 1

    if sum(num_files_per_config.values()) == 0:
        print 'Error: nothing to copy!'
        sys.exit(-1)
    else:
        if options.verbose:
            for (src, dst), num_files in num_files_per_config.items():
                print '%s\n  -> %s' % (src, dst)
                print '     (%d files)\n' % num_files

        print 'Saved %d regression test results (%d files).' % (
            len(num_files_per_config), sum(num_files_per_config.values()))
        print ('Remember to update the *_TESTDATA_DATE variables in '
               'build_serif.py!')
            
if __name__ == '__main__':
    main()

