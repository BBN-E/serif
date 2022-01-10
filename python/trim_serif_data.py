#!/bin/env python

# Copyright 2010 by BBN Technologies Corp.
# All Rights Reserved.

"""
This module runs serif to determine which subset of the data directory
it actually uses when run with a given collection of parameter files.
It then copies those files to a chosen destination directory.

If you run this module as a script, it will build a trimmed directory
based on the configuration variables defined at the top of the module.

Alternatively, you can import this module and call the functions:

  - copy_files_used_by_serif()
  - test_copy()

"""
import os, shutil, subprocess, sys, tempfile, re

############ Configuration ##############

# Paths to files within the text group repository.
TEXTDIR = 'C:/TextGroup'
SERIF_BIN_DIR = os.path.join(TEXTDIR, 'Core', 'SERIF')
SERIF_PAR_DIR = os.path.join(TEXTDIR, 'Projects', 'SERIF', 'par')
SERIF_DATA_DIR = os.path.join(TEXTDIR, 'Data', 'SERIF')
SERIF_SCORE_DIR = os.path.join(TEXTDIR, 'Projects', 'SERIF', 'scoring')

# Where should we put the subset of the data dir that we use?
DESTINATION = os.path.join(TEXTDIR, 'Data.trimmed')

# A list of (language, parfile) tuples.
CONFIGURATIONS = [
    ('Arabic', 'all.speed.best-arabic.par'),
    ('English', 'all.speed.best-english.par'),
    ('Arabic', 'all.best-arabic.par'),
    ('English', 'all.best-english.par'),
    ]

# End stage; this should be 'output' or possibly 'END'.
END_STAGE = 'output'

# If CLEANUP is false, then don't delete the temp dirs we create.
CLEANUP = True

############### Script ##################

SERIF_BIN_NAMES = [
    ['SerifMain', 'Release', 'Serif.exe'],
    ['SerifMain', 'Serif'],
    ['Serif.exe'],
    ['Serif'],
    ['IDX'],
    ['BBN_ACCENT'],
    ]
def get_serif_bin(serif_bin_dir):
    for path in SERIF_BIN_NAMES:
        p = os.path.join(serif_bin_dir, *path)
        if os.path.exists(p): return p
    raise ValueError("No Serif binary found in %r" % serif_bin_dir)

def make_parfile(parfile, expt_dir, parfile_template, serif_par_dir,
                 serif_score_dir, serif_data_dir, license_file):
    # Read the template
    template_path = os.path.join(serif_par_dir, parfile_template)
    s = open(template_path).read()
    # Fill in variables
    s = s.replace('+start_stage+', 'START')
    s = s.replace('+end_stage+', END_STAGE)
    s = s.replace('+parallel+', '000')
    s = s.replace('+state_saver_stages+', 'NONE')
    s = s.replace('+serif_data+', serif_data_dir)
    s = s.replace('+serif_score+', serif_score_dir)
    s = s.replace('+par_dir+', serif_par_dir)
    s = s.replace('+expt_dir+', expt_dir)
    # ICEWS-specific variables
    icews_lib_dir = os.path.join(
        os.path.split(serif_data_dir)[0], 'icews', 'lib')
    s = s.replace('+icews_lib_dir+', icews_lib_dir)
    s = s.replace('%experiment_dir%', expt_dir) # regtest file refers to output db this way
    # AWAKE-specific variable
    bbn_actor_db = 'sqlite://' + os.path.join(
        os.path.split(serif_data_dir)[0], 'awake', 'lib', 'database', 'actor_db.icews.sqlite') + \
        '?readonly&copy&cache_size=30000&temp_store=MEMORY'
    s = s.replace('+bbn_actor_db+', bbn_actor_db)
    awake_lib_dir = os.path.join(
        os.path.split(serif_data_dir)[0], 'awake', 'lib')
    s = s.replace('+awake_lib_dir+', awake_lib_dir)
    # Distillation-specific variables
    s = s.replace('+serif_rundir+', expt_dir)
    s = s.replace('+source_format+', 'sgm')
    s = s.replace('+seg_field+', 'src')
    s = s.replace('+output_format+', 'MT')
    s = s.replace('+do_sentence_breaking+', 'true')
    s = s.replace('+sentence_breaker_dateline_mode+', 'NONE')
    s = s.replace('+use_wms_sentence_break_heuristics+', 'false')
    # Relative paths/includes
    s = re.sub(r' \./', r' %s/' % serif_par_dir, s)
    s = re.sub(r' \.\./', r' %s/../' % serif_par_dir, s)
    # Track files read.
    s += '\ntrack_files_read: true\n'
    # Set experiment dir
    s += '\nOVERRIDE experiment_dir: %s\n' % expt_dir
    if license_file is not None:
        s += '\nOVERRIDE license_file: %s\n' % license_file
    # Write the templated par file
    out = open(parfile, 'w')
    out.write(s)
    out.close()

def run_serif(language, parfile_template, serif_bin_dir,
              serif_par_dir, serif_score_dir, serif_data_dir,
              license_file):
    exptdir = tempfile.mkdtemp()
    print ' Running Serif (trim_serif_data) '.center(70, '=')
    print 'Working directory: %r' % exptdir
    print 'Language: %s' % language
    print 'Parfile template: %s' % parfile_template
    try:
        serif_bin = get_serif_bin(serif_bin_dir)
        if not os.path.exists(serif_bin):
            raise ValueError('Could not find %s' % serif_bin)
        source_dir = 'sample-batch'
        source_ext = 'sgm'
        if parfile_template.find("xmltext") != -1:
            source_dir = 'sample-batch-icews-xmltext'
            source_ext = 'xml'
        full_source_dir = os.path.join(serif_data_dir,
                               language.lower(), 'test',
                               source_dir, 'source')
        source_files = [os.path.join(full_source_dir, f) for f in os.listdir(full_source_dir)
                     if f.endswith(source_ext)]
        parfile = os.path.join(exptdir, 'serif.par')
        make_parfile(parfile, exptdir, parfile_template, serif_par_dir,
                     serif_score_dir, serif_data_dir, license_file)
        subprocess.check_call([serif_bin, parfile, source_files[0]])
        files_read = open(os.path.join(exptdir, 'files_read.log')).read()
        file_list = [f.strip() for f in files_read.split('\n') if f.strip()]
        # remove the test file from the list
        normalized_filename = normalize(source_files[0])
        if normalized_filename in file_list:
            file_list.remove(normalized_filename)
        for subdir in ['test', 'Software']:
            p = os.path.join(serif_data_dir, language.lower(), subdir)
            if os.path.exists(p): file_list.append(p)
        return file_list
    finally:
        if CLEANUP:
            shutil.rmtree(exptdir)
        else:
            print 'trim_serif_data: not deleting %r' % exptdir
            print ' Done Running Serif (trim_serif_data) '.center(70, '=')
            print

def normalize(path):
    if sys.platform.startswith('win'):
        return path.replace('/', '\\')
    else:
        return path.replace('\\', '/')

# Get a list of all files used by all configurations.
def get_list_of_files_used_by_serif(configurations, serif_bin_dir,
                                    serif_par_dir, serif_score_dir,
                                    serif_data_dir, license_file,
                                    verbose=False):
    files_used = set()
    serif_data_dir = normalize(serif_data_dir)
    print 'Finding data files used by Serif...'
    for (language, parfile_template) in configurations:
        for filename in run_serif(language, parfile_template, serif_bin_dir,
                                  serif_par_dir, serif_score_dir,
                                  serif_data_dir, license_file):
            filename = normalize(filename)
            if filename.startswith(serif_data_dir):
                relpath = filename[len(serif_data_dir):].lstrip('/\\')
                if verbose:
                    print '  * %r' % relpath
                files_used.add(relpath)
    return files_used

def copy_files_used_by_serif(configurations, serif_bin_dir, serif_par_dir,
                             serif_score_dir, serif_data_dir, destination,
                             license_file, copy_binary=None, verbose=False,
                             copy_binary_exceptions=[]):
    """
    If copy_binary is specified, then use it to copy files.
    """
    files_used = get_list_of_files_used_by_serif(
        configurations, serif_bin_dir, serif_par_dir,
        serif_score_dir, serif_data_dir, license_file,
        verbose)

    #if os.path.exists(destination):
    #    print 'Deleting', destination
    #    shutil.rmtree(destination)

    # Copy those files to the destination directory.
    print 'Copying %d data files used by serif...' % len(files_used)
    for relpath in files_used:
        src = os.path.join(serif_data_dir, relpath)
        dst = os.path.join(destination, relpath)
        if verbose:
            print '  * %r' % relpath
        assert os.path.exists(src)
        parentdir = os.path.split(dst)[0]
        if not os.path.exists(parentdir):
            os.makedirs(parentdir)
        if os.path.isdir(src):
            assert os.path.split(src)[1] in ('Software', 'test')
            shutil.copytree(src, dst)
        elif copy_binary is None or os.path.basename(src) in copy_binary_exceptions:
            shutil.copy(src, dst)
        else:
            subprocess.check_call([copy_binary, src, dst])
        #if os.path.isdir(src):
        #    shutil.copytree(src, dst)
        #else:
        #    shutil.copy(src, dst)

def remove_svn_files(destination):
    for root, dirs, files in os.walk(destination, topdown=False):
        for name in files:
            if '.svn' in os.path.join(root, name):
                os.remove(os.path.join(root, name))
        for name in dirs:
            if '.svn' in os.path.join(root, name):
                os.rmdir(os.path.join(root, name))

def test_copy(configurations, serif_bin_dir, serif_par_dir,
              serif_score_dir, new_data_dir, license_file):
    """
    Run serif using the data in 'new_data_dir' (which was created
    by copy_files_used_by_serif()), to make sure that it does not
    fail.  If it does fail, then we forgot to copy some file(s).
    """
    for (language, parfile_template) in configurations:
        run_serif(language, parfile_template, serif_bin_dir,
                  serif_par_dir, serif_score_dir,
                  new_data_dir, license_file)

if __name__ == '__main__':
    copy_files_used_by_serif(CONFIGURATIONS, SERIF_BIN_DIR, SERIF_PAR_DIR,
                             SERIF_SCORE_DIR, SERIF_DATA_DIR, DESTINATION, None)
    test_copy(CONFIGURATIONS, SERIF_BIN_DIR, SERIF_PAR_DIR,
               SERIF_SCORE_DIR, DESTINATION, None)
    remove_svn_files(DESTINATION)
