#!/usr/bin/python

# Copyright 2010 by BBN Technologies Corp.
# All Rights Reserved.


USAGE="""\
Usage: %%prog [RELEASE_DIR] PIPELINE_DIR [options...]

  PIPELINE_DIR: The directory created by this script.  This directory
                will contain the files necessary to run SERIF.
   RELEASE_DIR: The SERIF release directory.  Defaults to:
                %s"""

import os, sys, optparse, textwrap, shutil

#----------------------------------------------------------------------
# Configuration

BYBLOS_DIR = 'Cube2/'
SERVER_BYBLOS_DTD = os.path.join(BYBLOS_DIR, 'install-optimize-static',
                                 'etc', 'server_byblos.dtd')
SERVER_BYBLOS_DRIVER = os.path.join(BYBLOS_DIR, 'install-optimize-static',
                                    "bin", "server_byblos_driver")

CONFIG_FILES = [
    'pipeline_config_server_english.xml',
    'pipeline_config_english.xml',
    'pipeline_config_fbf_english.xml']

CONFIG_PLACEHOLDERS = [
    (r'***PATH***', '%(working_dir)s'),
    (r'***DATA***', '%(release_dir)s/data'),
    (r'***BUILD***', '%(release_dir)s/bin'),
    (r'***SCORE***', '%(release_dir)s/scoring'),
    # not sure about what this one should point to:
    #(r'***REGRESSION**', '%(working_dir)s/regression'),
    ]

FBF_CONFIG_PLACEHOLDERS = [
    (r'***PATH***', '%(working_dir)s'),
    (r'***DATA***', '%(release_dir)s/data'),
    (r'***BUILD***', '%(release_dir)s/bin/%(language)s'),
    (r'***SCORE***', '%(release_dir)s/scoring'),
    # not sure about what this one should point to:
    #(r'***REGRESSION**', '%(working_dir)s/regression'),
    ]

SERIF_SERVER_START_PLACEHOLDERS = [
    (r'$BYBLOS_DIST', '%(byblos_dist)s'),
    ]

SERIF_PROCESS_SGM_PLACEHOLDERS = [
    (r'$SERIF_PIPELINES', '%(serif_pipelines)s'),
    ]

DEFAULT_RELEASE_DIR = os.path.split(
    os.path.split(os.path.abspath(sys.argv[0]))[0])[0]

#----------------------------------------------------------------------
# Installer.

class PipelineInstaller(object):
    def __init__(self, release_dir, working_dir, verbosity=0, dryrun=False):
        if release_dir is None:
            release_dir = DEFAULT_RELEASE_DIR
        self.release_dir = release_dir
        self.working_dir = working_dir
        self.verbosity = verbosity
        self.dryrun = dryrun

    #----------------------------------------------------------------------
    # Helper functions
    #
    # These all assume that we're copying things from self.release_dir
    # and writing them to self.working_dir

    def copy_dir(self, dirname, dirname2=None):
        src = os.path.join(self.release_dir, dirname)
        dst = os.path.join(self.working_dir, dirname2 or dirname)
        parent_dir = os.path.split(dst)[0]
        if not os.path.exists(parent_dir): self.make_dir(parent_dir)
        if self.verbosity > 0:
            self.log('copy dir %s -> %s' % (src, dst))
        if not self.dryrun:
            shutil.copytree(src, dst)

    def make_dir(self, dirname):
        dst = os.path.join(self.working_dir, dirname)
        if self.verbosity > 0:
            self.log('make dir %s' % dst)
        if not self.dryrun:
            os.makedirs(dst)

    def copy_file(self, filename, filename2=None):
        src = os.path.join(self.release_dir, filename)
        dst = os.path.join(self.working_dir, filename2 or filename)
        parent_dir = os.path.split(dst)[0]
        if not os.path.exists(parent_dir): self.make_dir(parent_dir)
        if self.verbosity > 0:
            self.log('copy file %s -> %s' % (src, dst))
        if not self.dryrun:
            shutil.copy(src, dst)

    def replace_placeholders(self, filename, placeholders, **repl_vars):
        filename = os.path.join(self.working_dir, filename)
        if self.verbosity > 0:
            self.log('Replacing placeholders in %s' % filename)
        if not self.dryrun:
            s = open(filename, 'rU').read()
            for (placeholder, repl) in placeholders:
                repl = repl % repl_vars
                s = s.replace(placeholder, repl)
            out = open(filename, 'w')
            out.write(s)
            out.close()

    def log(self, msg):
        if self.dryrun: msg = '[dryrun] %s' % msg
        print textwrap.fill(msg, subsequent_indent='        ')

    #----------------------------------------------------------------------
    # Main Script

    def install(self):
        if self.verbosity >= 0:
            print 'Creating a SERIF pipeline in:'
            print '  %r' % self.working_dir

        # Create the working directory
        if not os.path.exists(self.working_dir):
            self.make_dir('')

        # Create a templates directory and put the parameter files there.
        self.copy_dir('par', 'templates')

        # Copy over the server_byblos.dtd file.
        self.copy_file(SERVER_BYBLOS_DTD, 'server_byblos.dtd')

        # Copy the configuration files
        for config_file in CONFIG_FILES:
            self.copy_file(os.path.join('config', config_file), config_file)

        # Replace placeholders in the config files.
        self.replace_placeholders('pipeline_config_server_english.xml',
                             CONFIG_PLACEHOLDERS,
                             working_dir=self.working_dir,
                             release_dir=self.release_dir)
        self.replace_placeholders('pipeline_config_english.xml',
                             CONFIG_PLACEHOLDERS,
                             working_dir=self.working_dir,
                             release_dir=self.release_dir)
        self.replace_placeholders('pipeline_config_fbf_english.xml',
                             FBF_CONFIG_PLACEHOLDERS,
                             working_dir=self.working_dir,
                             release_dir=self.release_dir,
                             language='English')

        # Copy the server script and edit it to point to server_byblos_driver.
        self.copy_file('bin/serif_server_start.sh', 'serif_server_start.sh')
        self.replace_placeholders('serif_server_start.sh',
                             SERIF_SERVER_START_PLACEHOLDERS,
                             byblos_dist=os.path.join(self.release_dir,
                                                      BYBLOS_DIR))

        # Copy the process script and edit it to point to the working dir
        for script in ['serif_process_sgm_file.sh']:
            self.copy_file(os.path.join('bin', script), script)
            self.replace_placeholders(script, SERIF_PROCESS_SGM_PLACEHOLDERS,
                                      serif_pipelines=self.working_dir)

    def print_instructions(self):
        print
        print ' SERIF INSTRUCTIONS '.center(75, '=')
        print
        print 'To start a SERIF server:'
        print '  $ cd %s' % self.working_dir
        print '  $ serif_server_start.sh pipeline_config_english.xml'
        print
        print 'To queue a job to process a single SGM file:'
        print '  $ cd %s' % self.working_dir
        print '  $ serif_process_sgm_file.sh /ABSOLUTE/PATH/TO/FILE.SGM'
        print
        print 'To queue a job to process multiple SGM files:'
        print '  $ cd %s' % self.working_dir
        print ('  $ cp /PATH/TO/FILE/LIST '
               'pipeline-1000/input_dir/JOB_NAME.source_files')
        print '  $ touch pipeline-1000/input_dir/JOB_NAME.go'

if __name__ == '__main__':
    parser = optparse.OptionParser(usage=USAGE % DEFAULT_RELEASE_DIR)
    parser.add_option("-v", action='count', dest='verbose', default=0,
                      help="Generate more verbose output")
    parser.add_option("-q", action='count', dest='quiet', default=0,
                      help="Generate less verbose output")
    parser.add_option('--dryrun', dest='dryrun', action='store_true',
                      default=False)
    parser.add_option('-f', '--force', dest='force', action='store_true',
                      default=False)
    (options, args) = parser.parse_args()
    if len(args) == 2:
        (release_dir, working_dir) = args
    elif len(args) == 1:
        (release_dir, working_dir) = (None, args[0])
    elif len(args) == 0:
        parser.print_help()
        sys.exit(-1)
    else:
        parser.error('Too many arguments')
    if release_dir is None:
        release_dir = DEFAULT_RELEASE_DIR
    release_dir = os.path.abspath(release_dir)
    working_dir = os.path.abspath(working_dir)
    if not os.path.exists(release_dir):
        parser.error('Release dir %r not found' % release_dir)
    if not os.path.isdir(release_dir):
        parser.error('Release dir %r is not a directory' % release_dir)
    if os.path.exists(working_dir):
        if options.force:
            print 'Deleting old pipeline dir: %r' % working_dir
            shutil.rmtree(working_dir)
        else:
            parser.error('Pipeline dir %r already exists' % working_dir)

    #verbosity = (1 + options.verbose - options.quiet)
    verbosity = (options.verbose - options.quiet)

    installer = PipelineInstaller(release_dir, working_dir, verbosity,
                                  options.dryrun)
    installer.install()
    installer.print_instructions()
