#!/usr/bin/env python

# Copyright 2010 by BBN Technologies Corp.
# All Rights Reserved.

"""
Run serif on a given list of SGM files.
"""

USAGE="""\
Usage: %prog [options] inputs...

  inputs: A list of sgm files or directories that should be processed.
          Directories will be searched for any files named '*.sgm'."""

from create_pipeline_dir import PipelineInstaller, SERVER_BYBLOS_DRIVER
import optparse, os, os.path, tempfile, textwrap, subprocess
import shutil, time, signal, sys

def find_sgm_files(parser, args, verbosity, sgm_extension='.sgm'):
    """Find all the *.sgm files to process."""
    sgm_files = []
    for arg in args:
        if os.path.isdir(arg):
            for root, subdirs, files in os.walk(arg):
                new_sgm_files = [os.path.join(root, f) for f in files
                                 if f.endswith(sgm_extension)]
                if not new_sgm_files:
                    parser.error('No *.sgm files found in %r' % arg)
                if verbosity > 0:
                    print '%d files found in %r' % (len(new_sgm_files), arg)
                sgm_files.extend(new_sgm_files)
        elif arg.endswith(sgm_extension):
            if not os.path.exists(arg):
                parser.error('File %r not found' % arg)
            sgm_files.append(arg)
        else:
            parser.error('%s is not an sgm file or a directory')
    return sgm_files

def run_serif(release_dir, pipeline_dir, sgm_files, job_name,
              pipeline_xml, verbosity, num_threads=1):
    # Set up the pipeline directory.
    installer = PipelineInstaller(release_dir, pipeline_dir, verbosity)
    installer.install()

    # Start the server
    if verbosity >= 0:
        print 'Starting the SERIF server'
    server_byblos_driver = os.path.join(installer.release_dir,
                                        SERVER_BYBLOS_DRIVER)
    server_cmd = [server_byblos_driver, "-j%s" % num_threads,
                  "--trigger-ext=go", os.path.join(pipeline_dir, pipeline_xml)]
    if verbosity > 0:
        print textwrap.fill(' '.join(server_cmd), initial_indent='RUN: ',
                            subsequent_indent='      ')
    if verbosity > 0:
        stdout = None # to the screen
    else:
        stdout = open(os.path.join(pipeline_dir, 'server.log'), 'wb')
    server = subprocess.Popen(server_cmd, cwd=pipeline_dir, stdout=stdout)
    try:
        input_dir = os.path.join(pipeline_dir, 'pipeline-1000', 'input_dir')
        doclist = os.path.join(input_dir, job_name+'.source_files')
        go_file = os.path.join(input_dir, job_name+'.go')
        output_dir = os.path.join(pipeline_dir, 'pipeline-1000', 'output_dir')

        setup_batch_job(sgm_files, doclist, verbosity)
        start_batch_job(go_file, verbosity, len(sgm_files))
        return wait_for_batch_job(server, output_dir, job_name, verbosity)
    finally:
        if server.returncode is None:
            if verbosity >= 0:
                print '\nShutting down the server'
            os.kill(server.pid, signal.SIGKILL) #@UndefinedVariable
        if stdout is not None:
            stdout.close()


def setup_batch_job(sgm_files, doclist, verbosity):
    if verbosity > 0:
        print 'Writing file list:\n  %s' % doclist
    if not os.path.exists(os.path.split(doclist)[0]):
        os.makedirs(os.path.split(doclist)[0])
    out = open(doclist, 'wb')
    for sgm_file in sgm_files:
        out.write('%s\n' % os.path.abspath(sgm_file))
    out.close()

def start_batch_job(go_file, verbosity, num_files):
    # Start the batch job.
    if verbosity > 0:
        print 'Writing batch go file:\n  %s' % go_file
    if verbosity >= 0:
        print 'Starting a SERIF batch job to process %d files.' % num_files
    open(go_file, 'wb').close()

def wait_for_batch_job(server, output_dir, job_name, verbosity, delay=5):
    # Wait for it to finish
    while True:
        if os.path.exists(output_dir):
            for subdir in os.listdir(output_dir):
                if subdir == job_name:
                    if verbosity >= 0:
                        print '  Job finished successfully'
                    time.sleep(5) # time to copy files
                    return True
                elif subdir == job_name+'-ERROR':
                    if verbosity >= 0:
                        print '  Job failed!'
                    time.sleep(5) # time to copy files
                    return False
                elif server.poll() is not None:
                    if verbosity >= 0:
                        print 'Server exited unexpectedly!'
                    return False
        if verbosity > 0:
            print 'waiting for %s...' % job_name
        time.sleep(delay)


def main():
    # Process command-line args
    parser = optparse.OptionParser(usage=USAGE)
    parser.add_option("-v", action='count', dest='verbose', default=0,
                      help="Generate more verbose output")
    parser.add_option("-q", action='count', dest='quiet', default=0,
                      help="Generate less verbose output")
    parser.add_option("--pipeline-dir", dest='pipeline_dir', metavar='DIR',
                      help="Pipeline directory for SERIF.  If none is "
                      "specified, then a temporary directory is used.")
    parser.add_option("--keep-pipeline-dir", action='store_true',
                      dest='keep_pipeline_dir',
                      default=False, help="If specified, then do not delete "
                      "the pipeline directory when processing is complete.")
    parser.add_option("-o", "--output-dir", dest='output_dir', metavar='DIR',
                      help='The directory where '
                      "output should be written.")
    parser.add_option("--serif-release", dest='release_dir',
                      help="SERIF release directory")
    parser.add_option("--pipeline-xml-file", dest='pipeline_xml_file',
                      metavar='FILE',
                      help="SERIF pipeline xml configuration file.  E.g.: "
                      "pipeline_config_english.xml",
                      default='pipeline_config_english.xml')
    parser.add_option("--fbf", action='store_const', dest='pipeline_xml_file',
                      const="pipeline_config_fbf_english.xml",
                      help="shortcut for '--pipeline-xml-file = "
                      "pipeline_config_fbf_english.xml'")
    parser.add_option("--server", action='store_const',
                      dest='pipeline_xml_file',
                      const="pipeline_config_server_english.xml",
                      help="shortcut for '--pipeline-xml-file = "
                      "pipeline_config_server_english.xml'")
    parser.add_option("--extension", metavar='EXT', dest='sgm_extension',
                      help="The file extension that is used for input files. "
                      "This will be used if the list of inputs contains one "
                      "or more directories to find files to process.  "
                      "Default='.sgm'", default='.sgm')
    parser.add_option("--job-name", dest='job_name', metavar='NAME',
                      help="Job name.  This will be used as the directory "
                      "name for the output files.")
    (options, args) = parser.parse_args()
    if len(args) == 0:
        parser.error('Expected sgm files or directories')
    verbosity = options.verbose-options.quiet
    release_dir = options.release_dir

    if options.output_dir and os.path.exists(options.output_dir):
        parser.error('Directory %s already exists' % options.output_dir)

    sgm_files = find_sgm_files(parser, args, verbosity, options.sgm_extension)

    # Pick a job name if one wasn't specified
    job_name = (options.job_name or
                '%s-%s' % (os.environ.get('USER', 'serif'), os.getpid()))

    # Set up the pipeline dir.
    pipeline_dir = options.pipeline_dir or tempfile.mkdtemp('-serif')
    pipeline_dir = os.path.abspath(pipeline_dir)
    serif_output_dir = os.path.join(pipeline_dir, 'pipeline-1000',
                                    'output_dir', job_name)
    if verbosity > 0:
        print 'Pipeline dir:\n  %s' % pipeline_dir
    try:
        # Run serif!
        success = False
        success = run_serif(options.release_dir, pipeline_dir, sgm_files,
                            job_name, options.pipeline_xml_file, verbosity)
    except Exception, e:
        print "%s failed: %s" % (sys.argv[0], e)
        print "Not deleting pipeline directory:\n  %s" % pipeline_dir
        raise

    if not success:
        print 'Serif failed!'
        print "Not deleting pipeline directory:\n  %s" % pipeline_dir

    else:
        # Decide where to write the output.
        output_dir = options.output_dir or job_name
        if os.path.exists(output_dir):
            n = 1
            while os.path.exists('%s-%s' % (output_dir, n)): n += 1
            output_dir = '%s-%s' % (output_dir, n)

        # Copy the output from the pipeline dir to the output dir.
        if verbosity > 0:
            print 'Copying output to:\n  %s' % output_dir
        shutil.copytree(serif_output_dir, output_dir)

        # Delete the pipeline dir unless we're told not to.
        if not options.keep_pipeline_dir:
            if verbosity >= 0:
                print 'Deleting pipeline dir:\n  %s' % pipeline_dir
            try: shutil.rmtree(pipeline_dir)
            except: print '  Error deleting %r' % pipeline_dir

        # Tell the user where they can find the output.
        if verbosity == -1:
            print 'Output is in: %s' % output_dir
        elif verbosity >= 0:
            print '\nOutput is in:\n  %s' % output_dir

if __name__ == '__main__':
    main()
