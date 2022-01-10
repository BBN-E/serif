#!/bin/env python
#

# Copyright 2015 by BBN Technologies Corp.
# All Rights Reserved.
#
# Example usage: using files in a directory as inputs, and reading
# from an sqlite database.
#
#     python serif_icews.py /tmp/icews_serif_queue \
#            --input-dir ~/icews_test_set \
#            --icews-db 'sqlite:///home/eloper/icews.r973.sqlite' \
#            --output-db 'sqlite:///home/eloper/test-output.sqlite'
#
# Example usage: reading from an oracle database, and writing results
# back to that database.
#
#     python serif_icews.py /tmp/icews_serif_queue \
#            --input-dir ~/icews_test_set \
#            --icews-db 'oracle://e-oracle-02.bbn.com:1521/orcl.bbn.com?user=XXX&password=YYY' \
#            --output-db 'oracle://e-oracle-02.bbn.com:1521/orcl.bbn.com?user=XXX&password=YYY' \
#            --output-table serif_events
#
# Note that the system assumes the gazetteer and the ICEWS db are one and the same if
#  not otherwise specified. (Same goes for the database type.)


import serif_queue_manager
import os, sys, re, shutil, optparse
import compat

ICEWS_PAR_TEMPLATE = '''
parallel:           000
state_saver_stages: NONE
serif_score:        NONE
serif_data:         %(serif_data)s
icews_lib_dir:      %(icews_lib_dir)s

INCLUDE %(par_dir)s/master.english.par
INCLUDE %(par_dir)s/master.icews.english.par
INCLUDE %(par_dir)s/icews.english.xmltext.par

OVERRIDE icews_save_events_to_database_table: %(icews_output_table)s
OVERRIDE icews_output_db: %(icews_output_db)s
OVERRIDE icews_verbosity: 0

OVERRIDE use_feature_module_BasicCipherStream: true
OVERRIDE cipher_stream_always_decrypt: true

OVERRIDE linux_temp_dir: %(temp_dir)s
'''

ADU_PAR_TEMPLATE = '''
parallel:           0000
serif_data:         %(serif_data)s
serif_score:        NO_SCORE
start_stage:        START
end_stage:          output
icews_lib_dir:	    %(icews_lib_dir)s

INCLUDE %(par_dir)s/master.english.par
INCLUDE %(par_dir)s/master.awake.english.par

use_filename_as_docid: true
timex_get_doc_date_from_docid: true
input_type: rawtext
OVERRIDE sentence_breaker_dateline_mode: very_aggressive
OVERRIDE use_dateline_mode_on_whole_document: true

sector_fact_pattern_list: %(serif_data)s/english/facts/sectors/sector_fact_pattern_files.list

OVERRIDE use_metonymy: false 

bbn_actor_db: sqlite://%(bbn_actor_db)s?readonly&copy&cache_size=30000&temp_store=MEMORY
use_normalized_sqlite_geonames_tables: false

OVERRIDE modules: English,BasicCipherStream

use_feature_module_BasicCipherStream: true
xor_always_decrypt: false
cipher_stream_always_decrypt: true
'''

serif_queue_manager.KEYBOARD_INTERRUPT_MSG = ''

serif_queue_manager.Worker.QUIET = True

USAGE='''\
%prog [options] QUEUE_DIR

QUEUE_DIR: A temporary directory that SERIF processes will use to
communicate with one another.  This directory will be created by
serif_icews.py, and deleted once processing is complete.'''

class ICEWS_CLI(object):
    def __init__(self, pipeline):
        self.pipeline = pipeline
        self.parfile = None
        self.root = None
        self.build_parser()
        self.parse_args()

    def build_parser(self):
        self.parser = optparse.OptionParser(usage=USAGE)
        self.parser.add_option("-v", action='count', dest='verbose', default=0,
                               help="Generate more verbose output")
        self.parser.add_option("-q", action='count', dest='quiet', default=0,
                               help="Generate less verbose output")
        self.parser.add_option("--mode", dest="mode", default='ICEWS',
                               help="determines PAR template to use")
        self.parser.add_option("--no-cleanup", action='store_false',
                               dest='cleanup',
                               default=True, help="Do not remove worker "
                               "directories when stopping or killing workers")
        self.parser.add_option("--serif-bin", dest='serif_bin', metavar='PATH',
                               help="Path to Serif binary")
        self.parser.add_option('--input-dir',
                         dest='input_dir', metavar='DIRECTORY',
                         help="Read input files from the given directory")
        self.parser.add_option('-s', '--story-source',
                               dest='icews_story_source',
                               metavar='SOURCE_NAME')
        self.parser.add_option('-i', '--story-id-range',
                               dest='icews_story_ids', metavar='IDS'),
        self.parser.add_option('-d', '--story-date-range',
                               dest='icews_story_dates', metavar='DATES',
                               help="Range of PublicationDate values: "
                               "mm/dd/yyyy-mm/dd/yyyy")
        self.parser.add_option('--ingest-date-range',
                               dest='icews_ingest_dates', metavar='DATES',
                               help="Range of IngestDate values: "
                               "mm/dd/yyyy-mm/dd/yyyy")
        self.parser.add_option('--icews-db', metavar='DATABASE',
                               dest='icews_db', help='ICEWS input database')
        self.parser.add_option('--icews-stories-db', metavar='DATABASE',
                               dest='icews_stories_db',
                               help='ICEWS stories database')
        self.parser.add_option('--icews-gazetteer-db', metavar='DATABASE',
                               dest='icews_gazetteer_db',
                               help='ICEWS gazetteer database')
        self.parser.add_option('--output-db', metavar='DATABASE',
                               dest='output_db')
        self.parser.add_option('--output-table', metavar='DATABASE',
                               dest='output_table', default='serif_events')
        self.parser.add_option('--beam1', metavar='N', type=int,
                               dest='beam1', default=4)
        self.parser.add_option('--beam2', metavar='N', type=int,
                               dest='beam2', default=4)
        self.parser.add_option('--temp-dir', metavar='DIR',
                               dest='temp_dir', default='/tmp',
                               help='Directory for temporary files')
        self.parser.add_option('--single-writer', action='store_true',
                               default=False, dest='single_writer')
        self.parser.add_option('--bbn-actor-db', dest='bbn_actor_db',
                               help='For ADU mode, location of sqlite actor DB')

    def parse_args(self):
        (self.options, args) = self.parser.parse_args()
        if len(args) != 1: self.parser.error('Expected a QUEUE_DIR')
        self.verbosity = (self.options.verbose - self.options.quiet)
        self.root = os.path.abspath(args[0])
        if os.path.exists(self.root):
            self.parser.error('QUEUE_DIR %r already exists' % self.root)
        if self.options.serif_bin:
            serif_queue_manager.Worker.set_serif_bin(self.options.serif_bin)
        if len([o for o in [self.options.input_dir,
                            self.options.icews_story_source,
                            self.options.icews_story_ids,
                            self.options.icews_story_dates,
                            self.options.icews_ingest_dates]
                if o is not None]) != 1:
            sys.stderr.write("Warning: you are running on all stories in the database!\n")            

        self.find_serif_par_dir()
        self.find_serif_data_dir()
        self.find_icews_lib_dir()

    def copy_input_dir_files(self):
        assert self.root
        src = self.options.input_dir
        if not os.path.isdir(src):
            self.parser.error('%r is not a directory' % src)
        start_dir = serif_queue_manager.Worker.get_queue_dir(
            self.root, serif_queue_manager.PIPELINE_START)
        if os.path.exists(start_dir):
            print '%r already exists -- not copying files' % start_dir
            return
        os.makedirs(start_dir)
        src_files = os.listdir(src)
        print 'Copying %d input files %r -> %r...' % (
            len(src_files), src, start_dir)
        for filename in src_files:
            tmp_name = os.path.join(start_dir, filename)+'.tmp'
            ready_name = (os.path.join(start_dir, filename)+
                          serif_queue_manager.READY_EXTENSION)
            shutil.copy(os.path.join(src, filename), tmp_name)
            os.rename(tmp_name, ready_name)
        # Create 'done' file to signal that we're done once these
        # have all been processed.
        done_file = os.path.join(
            start_dir, serif_queue_manager.DONE_FILENAME)
        open(done_file, 'wb').close()

    _SERIF_PAR_DIR_PATH = ['../par']
    def find_serif_par_dir(self):
        here = os.path.abspath(os.path.split(__file__)[0])
        for path in self._SERIF_PAR_DIR_PATH:
            p = os.path.join(here, path, 'master.icews.english.par')
            if os.path.exists(p):
                self._par_dir = os.path.join(here, path)
                return
        self.parser.error('Unable to find SERIF parameter directory')

    _ICEWS_LIB_DIR_PATH = ['../icews']
    def find_icews_lib_dir(self):
        here = os.path.abspath(os.path.split(__file__)[0])
        for path in self._ICEWS_LIB_DIR_PATH:
            p = os.path.join(here, path, 'event-patterns.txt')
            if os.path.exists(p):
                self._icews_lib_dir = os.path.join(here, path)
                return
        self.parser.error('Unable to find SERIF icews_lib directory')

    _SERIF_DATA_DIR_PATH = ['../data']
    def find_serif_data_dir(self):
        here = os.path.abspath(os.path.split(__file__)[0])
        for path in self._SERIF_DATA_DIR_PATH:
            p = os.path.join(here, path,
                             'ace/ace_2004_entity_types.txt')
            if os.path.exists(p):
                self._data_dir = os.path.join(here, path)
                return
        self.parser.error('Unable to find SERIF data directory')

    def make_icews_parfile(self):
        self.parfile = os.path.join(self.root, 'icews.par')
        # Add default options to sqlite databases.
        if (self.options.icews_db and self.options.icews_db.startswith('sqlite://') and
            '?' not in self.options.icews_db):
            self.options.icews_db += (
                '?readonly&copy&cache_size=30000&temp_store=MEMORY')

        # [xx] we can't have each worker create the database --
        # they'll delete one another's work!!
        #if (self.options.output_db.startswith('sqlite://') and
        #    '?' not in self.options.output_db):
        #    self.options.output_db += '?create'

        
        # Construct the parameter file.
        par = ICEWS_PAR_TEMPLATE % dict(
            par_dir=self._par_dir,
            serif_data=self._data_dir,
            icews_lib_dir=self._icews_lib_dir,
            icews_output_db=self.options.output_db,
            icews_output_table=self.options.output_table,
            temp_dir=self.options.temp_dir)
        
        # Set specialized DBs if specified (OVERRIDE necessary for icews_db)
        if self.options.icews_db:
            par += '\nOVERRIDE icews_db: %s\n' % (
                self.options.icews_db)
        if self.options.icews_gazetteer_db:
            par += '\nicews_gazetteer_db: %s\n' % (
                self.options.icews_gazetteer_db)
        if self.options.icews_stories_db:
            par += '\nicews_stories_db: %s\n' % (
                self.options.icews_stories_db)

        # Read from the database 
        if self.options.input_dir:
            par += "OVERRIDE icews_read_stories_from_database: false"

        # Add story source, id range, publication range.
        if self.options.icews_story_source:
            par += '\nicews_story_source: %s\n' % (
                self.options.icews_story_source)
        if self.options.icews_story_ids:
            m = re.match(r'(\d+)-(\d+)', self.options.icews_story_ids)
            if not m:
                self.parser.error("Badly formatted story_ids.  "
                                  "Expected nnn-nnn")
            par += ('\nicews_min_storyid: %s'
                    '\nicews_max_storyid: %s\n' % (m.group(1), m.group(2)))
        if self.options.icews_story_dates:
            m = re.match(r'(\d+/\d+/\d+)-(\d+/\d+/\d+)',
                         self.options.icews_story_dates)
            if not m:
                self.parser.error("Badly formatted story_dates.  "
                                  "Expected mm/dd/yyyy-mm/dd/yyyy")
            par += ('\nicews_min_story_publication_date: %s'
                    '\nicews_max_story_publication_date: %s\n' %
                    (m.group(1), m.group(2)))
        if self.options.icews_ingest_dates:
            m = re.match(r'(\d+/\d+/\d+)-(\d+/\d+/\d+)',
                         self.options.icews_ingest_dates)
            if not m:
                self.parser.error("Badly formatted ingest_dates.  "
                                  "Expected mm/dd/yyyy-mm/dd/yyyy")
            par += ('\nicews_min_ingest_publication_date: %s'
                    '\nicews_max_ingest_publication_date: %s\n' %
                    (m.group(1), m.group(2)))

        # Add any necessary feature modules
        dbs = [self.options.icews_db, self.options.icews_stories_db,
               self.options.icews_gazetteer_db, self.options.output_db]
        if any(db.startswith('oracle://') for db in dbs if db):
            par += '\nuse_feature_module_Oracle: true\n'

        # Write the parameter file.
        out = open(self.parfile, 'wb')
        out.write(par)
        out.close()


    def make_adu_parfile(self):
        self.parfile = os.path.join(self.root, 'serif_adu.par')
        
        # Construct the parameter file.
        par = ADU_PAR_TEMPLATE % dict(
            par_dir=self._par_dir,
            serif_data=self._data_dir,
            icews_lib_dir=self._icews_lib_dir,
            bbn_actor_db=self.options.bbn_actor_db)
        
        # Write the parameter file.
        out = open(self.parfile, 'wb')
        out.write(par)
        out.close()

    def get_icews_pipeline(self):
        if self.pipeline:
            return

        if self.options.input_dir:
            self.pipeline = [
                ('values', 1),
                ('parse', self.options.beam1),
                ('icews-output', self.options.beam2),
                ]
        else:
            self.pipeline = [
                ('icews-feeder', 1),
                ('values', 1),
                ('parse', self.options.beam1),
                ('icews-output', self.options.beam2),
                ]
        if self.options.single_writer:
            self.pipeline[-1] = ('icews-events', self.options.beam2)
            self.pipeline.append( ('icews-output', 1) )

    def create_output_sqlite_db(self):
        m = re.match('sqlite://([^\?]+)', self.options.output_db)
        if not m: return
        output_db = m.group(1)
        if not os.path.exists(output_db):
            try:
                import sqlite3
                print 'Creating output database %r' % output_db
                sqlite3.connect(output_db).close()
            except:
                print '(python sqlite3 not found)'

    def run(self):
        if self.root is None:
            self.parser.error("--queue-dir required")        

        if self.options.input_dir:
            self.copy_input_dir_files()

        if not os.path.exists(self.root):
            os.makedirs(self.root)

        if self.options.mode == 'ICEWS':
            serif_queue_manager.Worker.SOURCE_FORMAT = 'ICEWS_XMLText'

            self.get_icews_pipeline()

            if not self.options.output_db:
                self.parser.error('--output-db is required')
            if not self.options.output_table:
                self.parser.error('--output-table is required')
            
            if self.options.input_dir and (self.options.icews_story_source or self.options.icews_story_ids or self.options.icews_story_dates or self.options.icews_ingest_dates):
                self.parser.error("Cannot specify which stories to pull from the database as well as an input directory")

            if self.options.output_db.startswith('sqlite://'):
                self.create_output_sqlite_db()
                
            self.make_icews_parfile()
        
        elif self.options.mode == 'ADU':
            serif_queue_manager.Worker.SOURCE_FORMAT = 'sgm' # We are actually processing rawtext here, but this ensures a DefaultDocumentReader will be created

            self.pipeline = [
                ('parse', 6),
                ('actor-match', 6),
                ('output', 4),
                ]

            if not self.options.bbn_actor_db:
                self.parser.error('--bbn-actor-db is required')
            
            self.make_adu_parfile()

        else:
            self.parser.error('Invalid mode: ' + self.options.mode)

        # Build & run the queue monitor.
        monitor = serif_queue_manager.QueueMonitor(
            self.root, self.parfile, self.verbosity, self.pipeline)
        pipeline_rv = False
        try:
            pipeline_rv = monitor.watch_pipeline()
        finally:
            if any(w.is_alive() for w in monitor.workers):
                print 'Shutting down all jobs'
                monitor.kill_all(self.options.cleanup)
            if self.options.cleanup:
                print 'Deleting queue directory %r' % self.root
                try: shutil.rmtree(self.root)
                except Exception, e:
                    print 'Error during cleanup: %s' % e

        if pipeline_rv:
            return 0
        else:
            return 1

if __name__ == '__main__':
    print "Copyright 2015 Raytheon BBN Technologies Corp."
    print 'All Rights Reserved.'
    print
    cli = ICEWS_CLI(None)
    sys.exit(cli.run())
