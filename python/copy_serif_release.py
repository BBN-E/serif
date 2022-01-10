#!/bin/env python

# Copyright 2010 by BBN Technologies Corp.
# All Rights Reserved.


import optparse, os, shutil, textwrap

USAGE="""\
Usage: %prog [options] build_expt rev release_dir

  build_ext: The path to the build_serif experiment used to build binaries
  rev: The revision string from the build
  release_dir: The directory where release should be created."""

#----------------------------------------------------------------------
# Process Command-line arguments.

parser = optparse.OptionParser(usage=USAGE)
parser.add_option("-v", action='count', dest='verbose', default=0,
                  help="Generate more verbose output")
parser.add_option("-q", action='count', dest='quiet', default=0,
                  help="Generate less verbose output")
parser.add_option('-f', '--force', dest='force', action='store_true',
                  default=False)
parser.add_option('--dryrun', '--noop', dest='dryrun', action='store_true',
                  default=False)

(options, args) = parser.parse_args()
if len(args) != 3:
    parser.error('Expected a build experiment, a revision string and a release dir')
build_expt, build_rev, release_dir = args
if not os.path.exists(build_expt):
    parser.error('Build experiment %r not found' % build_expt)
if not os.path.isdir(build_expt):
    parser.error('Build experiment %r is not a directory' % build_expt)
if os.path.exists(release_dir) and not options.force:
    parser.error('Release dir %r already exists' % release_dir)
verbosity = (1 + options.verbose - options.quiet)
build_expt = os.path.abspath(build_expt)
release_dir = os.path.abspath(release_dir)
if verbosity > 1:
    print '\nOptions:'
    print '  %-20s %r' % ('Build expt', build_expt)
    print '  %-20s %r' % ('Build rev', build_rev)
    print '  %-20s %r' % ('Release dir', release_dir)
    for (k,v) in options.__dict__.items():
        if not k.startswith('_'):
            print '  %-20s %r' % (k,v)
    print

#----------------------------------------------------------------------
# Helper functions

def copy_dir(dirname, dirname2=None):
    src = os.path.join(build_expt, dirname)
    dst = os.path.join(release_dir, dirname2 or dirname)
    if not os.path.isdir(src):
        raise ValueError('Directory %r not found!' % src)
    _make_parent(dst)
    if verbosity > 0:
        log('copy dir %s -> %s' % (src, dst))
    if not options.dryrun:
        shutil.copytree(src, dst)

def copy_dir_with_rev(dirname, dirname2=None, optional=True):
    src = os.path.join(build_expt, dirname, build_rev)
    dst = os.path.join(release_dir, dirname2 or dirname)
    if not os.path.isdir(src):
        if optional: return
        raise ValueError('Directory %r not found!' % src)
    _make_parent(dst)
    if verbosity > 0:
        log('copy dir %s -> %s' % (src, dst))
    if not options.dryrun:
        shutil.copytree(src, dst)

def make_dir(dirname):
    dst = os.path.join(release_dir, dirname)
    if verbosity > 0:
        log('make dir %s' % dst)
    if not options.dryrun:
        os.makedirs(dst)

def _make_parent(dst):
    parent_dir = os.path.split(dst)[0]
    if not os.path.exists(parent_dir):
        make_dir(os.path.abspath(parent_dir))

def log(msg):
    if options.dryrun:
        print textwrap.fill(msg, initial_indent='[DRYRUN] ',
                            subsequent_indent='              ')
    else:
        print textwrap.fill(msg, subsequent_indent='        ')

def create_file(filename, contents):
    path = os.path.join(release_dir, filename)
    if verbosity > 0:
        log('create %s' % path)
    if not options.dryrun:
        out = open(path, 'wb')
        out.write(contents)
        out.close()

# Copy the build experiment directories
copy_dir('bin')
copy_dir('sequences')
copy_dir('templates')

# Copy the build experiment output directories
copy_dir_with_rev('ckpts')
copy_dir_with_rev('etemplates')
copy_dir_with_rev(os.path.join('expts','Serif'))
copy_dir_with_rev(os.path.join('expts', 'build'))
copy_dir_with_rev(os.path.join('expts', 'install'))
copy_dir_with_rev(os.path.join('expts', 'test'), optional=True)
copy_dir_with_rev('logfiles')

# Add a readme file that records the svn revision that this
# release was built from.
RELEASE_README = '''\
Serif Release Information

   Build Experiment: %(build_expt)s
       SVN Revision: %(build_rev)s
  Release Directory: %(release_dir)s
'''
create_file('README.txt', RELEASE_README % dict(
        build_expt=build_expt,
        build_rev=build_rev,
        release_dir=release_dir))

# Make symlink to data dir
#if verbosity > 0:
#    log('make symlink %s -> %s' % ('d4m/serif/data', os.path.join(release_dir, 'expts', 'Serif', 'data')))
#if not options.dryrun:
#    os.symlink('/d4m/serif/data', os.path.join(release_dir, 'expts', 'Serif', 'data'))

print
