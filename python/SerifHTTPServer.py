#!/bin/env python

# Copyright 2011 by BBN Technologies Corp.
# All Rights Reserved.


import optparse, os, socket, subprocess, tempfile, time

USAGE="""\
Usage: %prog [options] param_file
"""

###########################################################################
## Default Configuration
###########################################################################

# Set these to the location of your default binary directory and parameter
# file. The following variables will be overridden from the parameter
# file: start, end, experiment_dir.
SERVER_HOME = '%serif_home%'
SERVER_BIN = SERVER_HOME + '/bin/x86_64'
SERVER_PAR = SERVER_HOME + '/par/all.best-english.par'

###########################################################################
## Script
###########################################################################

class SerifHTTPServer:
    """
    A simple class that can be used to start up a server.  Currently,
    it doesn't do any clean-up after itself.
    """

    def __init__(self, par=SERVER_PAR, bin_dir=SERVER_BIN,
                 language='english', port=8000,
                 start_stage='START', end_stage='output'):
        self._pipe = None
        self.port = port
        start_stage = start_stage
        end_stage = end_stage
        # Create a working directory.
        rootdir = tempfile.mkdtemp()
        # Create the parameter file.
        parfile = os.path.join(rootdir, 'serif.par')
        out = open(parfile, 'w')
        out.write(self.PAR_FILE_TEMPLATE % dict(
            start=start_stage, end=end_stage, include=par,
            rootdir=rootdir))
        out.close()
        # Construct path to SERIF binary
        bin_path = self.construct_bin_path(bin_dir, language)
        # Start the server as a subprocess.
        self._pipe = subprocess.Popen(
            [bin_path, parfile, '-p', str(self.port)])

    def __del__(self):
        if self.is_live():
            print 'Killing server...'
            try: self._pipe.kill()
            except: print 'Unable to kill server'

    def is_live(self):
        if self._pipe is None:
            return False
        self._pipe.poll()
        return self._pipe.returncode is None

    def construct_bin_path(self, bin_dir, language):
        bin_name = 'SerifHTTPServer_' + language.title()
        if os.name is 'nt':
            bin_name += '.exe'
        if not os.path.isdir(bin_dir):
            raise ValueError('Binary dir not found: ' + bin_dir)
        bin_path = os.path.join(bin_dir, bin_name)
        if not os.path.isfile(bin_path):
            raise ValueError('SERIF binary not found: ' + bin_path)
        return bin_path

    def shutdown(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(('localhost',self.port))
        s.send("POST Shutdown HTTP/1.0\r\n")
        s.send("content-length: 0\r\n\r\n")
        # Read the response.
        response = ''
        while 1:
            received = s.recv(1024)
            if not received: break
            response += received
        print response

    PAR_FILE_TEMPLATE = (
        'INCLUDE %(include)s\n'
        'OVERRIDE start_stage: %(start)s\n'
        'OVERRIDE end_stage: %(end)s\n'
        'OVERRIDE experiment_dir: %(rootdir)s\n')

if __name__ == '__main__':

    parser = optparse.OptionParser(usage=USAGE)
    parser.add_option('-p', dest='port', default=8000,
                      help="port number that the server should use to " \
                           "listen for incoming connections (default: %default)")
    parser.add_option('-s', dest='start_stage', default='START',
                      help="stage where SERIF should begin processing " \
                            "(default: %default)")
    parser.add_option('-e', dest='end_stage', default='output',
                      help="stage where SERIF should end processing "\
                          "(default: %default)")
    parser.add_option('-l', dest='language', default='english',
                      help="language of the text to process "\
                          "(default: %default)")
    parser.add_option('-b', dest='bin', default=".",
                      help="directory where SERIF binaries are located "\
                          "(default: %default)")

    (opts, args) = parser.parse_args()

    if len(args) != 1:
        parser.error('Expected a param_file')
    par = args[0]

    print 'Starting server...'
    server = SerifHTTPServer(par, opts.bin, opts.language, opts.port,
                             opts.start_stage, opts.end_stage)
    while server.is_live():
        time.sleep(1)
