#!/bin/env python

# Copyright 2012 by BBN Technologies Corp.
# All Rights Reserved.


import optparse, re, sys, os, shutil, textwrap, subprocess, time
import stat
import trim_serif_data
import assert_serif_rights

USAGE="""\
Usage: %prog [options] release_dir deliverable_dir

  release_dir: A directory containing a built release
  deliverable_dir: The directory where output should be written."""

# Languages:
ARABIC = 'Arabic'
CHINESE = 'Chinese'
ENGLISH = 'English'
SPANISH = 'Spanish'

#----------------------------------------------------------------------
# File & Directory lists.
#
# It would be fairly easy to compute most of these automatically; but
# we would include some files that we might not want to have in the
# release.  E.g., we would include extra par files.  So for now, we'll
# just use a hand-maintained list.


PAR_FILES = {
    ARABIC: ['all.best-arabic.par', 'all.speed.best-arabic.par', 'master.arabic.par', 'master.arabic-speed.par'],
    CHINESE: ['all.best-chinese.par', 'all.speed.best-chinese.par', 'master.chinese.par', 'master.chinese-speed.par'],
    ENGLISH: ['all.best-english.par', 'all.speed.best-english.par', 'master.english.par', 'master.english-speed.par'],
    SPANISH: ['all.best-spanish.par', 'master.spanish.par'],
    }

SOURCE_DIRS = [
    'Arabic',
    'CMake', 'Chinese', 'DTCorefTrainer',
    'DescriptorClassifierTrainer', 'DescriptorLinkerTrainer',
    'English', 'EventFinder', 'Generic', 'IdfTrainer',
    'IdfTrainerPreprocessor', 'MaxEntRelationTrainer',
    'MentionMapper', 'MorphAnalyzer', 'MorphologyTrainer',
    'NameLinkerTrainer', 'P1DescTrainer', 'P1RelationTrainer',
    'PIdFTrainer', 'PNPChunkTrainer', 'PPartOfSpeechTrainer',
    'ParserTrainer', 'Preprocessor', 'PronounLinkerTrainer',
    'RelationTimexArgFinder', 'RelationTrainer', 'SerifHTTPServer',
    'SerifHTTPServer', 'SerifSocketServer', 'StandaloneParser',
    'StandaloneSentenceBreaker', 'StandaloneTokenizer',
    'StatSentBreakerTrainer',
    'SerifMain', 'Spanish',
    'StaticFeatureModules',
    # Should any of these be included:?
    #     'ActiveLearning',
    #     'LearnIt',
    #     'Temporal',
    #     'PredFinder',
    #     'MySQL',
    #     'Oracle',
    #     'ICEWS',
    #     'SevenZip',
    #     'OpenSSLCipherStream',
    #     'BasicCipherStream',
    #     'KbaStreamCorpus', #use option --include-kba-source
    ]

# (release_name, deliverable_name) tuples:
RELEASE_BINARIES = [
    # command-line binary:
    ('../install/%(ARCH_SUFFIX)s/NoThread/Classic/Release/SerifMain/Serif',
     '%(ARCH_SUFFIX)s/Serif'),
    ]

WINDOWS_BINARIES = [
    ('../install/%(ARCH_SUFFIX)s/NoThread/Classic/Release/SerifMain/Release/Serif.exe',
     '%(ARCH_SUFFIX)s/Serif.exe'),
    ('../build/%(ARCH_SUFFIX)s/NoThread/Classic/Release/SerifMain/Release/xerces-c_3_1-%(XERCES_SUFFIX)s.dll',
     '%(ARCH_SUFFIX)s/xerces-c_3_1-%(XERCES_SUFFIX)s.dll')
    ]

SHARED_LIBRARIES = [
    ('../install/%(LANGUAGE)s/%(ARCH_SUFFIX)s/NoThread/Server/Release/SerifJNI/'+
     'libSerifJNI_%(LANGUAGE)s.so',
     '%(ARCH_SUFFIX)s/libSerifJNI_%(LANGUAGE)s.so'),
    ]

TRAINING_BINARIES = [
    'DTCorefTrainer/DTCorefTrainer',
    'EventFinder/EventFinder',
    'IdfTrainer/IdfTrainer',
    'MaxEntRelationTrainer/MaxEntRelationTrainer',
    'NameLinkerTrainer/NameLinkerTrainer',
    'P1DescTrainer/P1DescTrainer',
    'P1RelationTrainer/P1RelationTrainer',
    'PIdFTrainer/PIdFTrainer',
    'PNPChunkTrainer/PNPChunkTrainer',
    'PPartOfSpeechTrainer/PPartOfSpeechTrainer',
    'PronounLinkerTrainer/PronounLinkerTrainer',
    'RelationTimexArgFinder/RelationTimexArgFinder',
    'RelationTrainer/RelationTrainer',
    ]

ARCH_DICT = {'i686': '32-bit (x86)',
             'x86_64': '64-bit (x86-64)',
             'x86_64_glibc27': '64-bit Linux with glibc 2.7 (Redhat 6.2)',
             'Win32': '32-bit Windows',
             'Win64': '64-bit Windows'}

PYTHON_SCRIPTS = [
    'SerifHTTPServer.py',
    'SerifHTTPClient.py',
    'SerifXMLModifier.py',
    'serifxml.py',
    'serifxml_viewer.py',
    'serifxml_viewer_resources/serifxml_viewer.css',
    'serifxml_viewer_resources/serifxml_viewer.js',
    ]


ENCRYPT_BINARY = 'install/x86_64/NoThread/Classic/Release/BasicCipherStream/BasicCipherStreamEncryptFile'

JAVA_FILES = ['serif.jar', 'serif_test.jar', 'serifxml.xsd']
JAVA_SUBDIRS = ['doc', 'src', 'demo', 'test']

BOOST_LIBDIR='/opt/boost_1_40_0-gcc-4.1.2%(ARCH_SUFFIX2)s/lib'
MTDECODER_LIBDIR='/d4m/serif/External/MTDecoder/%(ARCH_SUFFIX)s/'
SHARED_LIB_DEPENDENCIES = [
    BOOST_LIBDIR+'/libboost_date_time.so.1.40.0',
    BOOST_LIBDIR+'/libboost_filesystem.so.1.40.0',
    BOOST_LIBDIR+'/libboost_iostreams.so.1.40.0',
    BOOST_LIBDIR+'/libboost_program_options.so.1.40.0',
    BOOST_LIBDIR+'/libboost_regex.so.1.40.0',
    BOOST_LIBDIR+'/libboost_serialization.so.1.40.0',
    BOOST_LIBDIR+'/libboost_system.so.1.40.0',
    BOOST_LIBDIR+'/libboost_thread.so.1.40.0',
    MTDECODER_LIBDIR+'libDecoder.so',
]

ICEWS_BINARIES = [
    # command-line binary:
    #('../install/%(ARCH_SUFFIX)s/NoThread/ICEWS/Release/SerifMain/Serif',
    ('../install/%(ARCH_SUFFIX)s/NoThread/Classic/Release/SerifMain/Serif',
     '%(ARCH_SUFFIX)s/Serif'),
    ]

ICEWS_WINDOWS_BINARIES =  [
    #('../install/%(ARCH_SUFFIX)s/NoThread/ICEWS/Release/SerifMain/Release/Serif.exe',
    ('../install/%(ARCH_SUFFIX)s/NoThread/Classic/Release/SerifMain/Release/Serif.exe',
     '%(ARCH_SUFFIX)s/Serif.exe'),
    ('../build/%(ARCH_SUFFIX)s/NoThread/ICEWS/Release/SerifMain/Release/xerces-c_3_1-%(XERCES_SUFFIX)s.dll',
     '%(ARCH_SUFFIX)s/xerces-c_3_1-%(XERCES_SUFFIX)s.dll')
    ]
KBA_STREAMCORPUS_BINARIES = [
    # command-line binary:
    # ('../install/%(ARCH_SUFFIX)s/NoThread/KbaStreamCorpus_Serif/Release/SerifMain/Serif',
    #  '%(ARCH_SUFFIX)s/Serif'),
    ('../install/%(ARCH_SUFFIX)s/NoThread/KbaStreamCorpus_IDX/Release/IDX/IDX',
    '%(ARCH_SUFFIX)s/Serif'),
    ]
IDX_BINARIES = [
    ('../install/%(ARCH_SUFFIX)s/NoThread/IDX/Release/IDX/IDX',
     '%(ARCH_SUFFIX)s/IDX'),
    ]
IDX_WINDOWS_BINARIES = [
    ('../install/%(ARCH_SUFFIX)s/NoThread/IDX/Release/IDX/Release/IDX.exe',
     '%(ARCH_SUFFIX)s/IDX.exe'),
    ('../build/%(ARCH_SUFFIX)s/NoThread/IDX/Release/IDX/Release/xerces-c_3_1-%(XERCES_SUFFIX)s.dll',
     '%(ARCH_SUFFIX)s/xerces-c_3_1-%(XERCES_SUFFIX)s.dll')
    ]
IDX_ENCRYPT_BINARY = 'install/x86_64/NoThread/IDX/Release/SERIF_IMPORT/BasicCipherStream/BasicCipherStreamEncryptFile'
ICEWS_PYTHON_SCRIPTS = [
    'serif_queue_manager.py',
    'serif_icews.py',
    'compat.py',
    'serifxml.py',
    'serifxml_viewer.py',
    'serifxml_viewer_resources/serifxml_viewer.css',
    'serifxml_viewer_resources/serifxml_viewer.js',
    ]
ICEWS_EXCLUDES = [
    'database',
    'demo_scripts',
    'scripts',
    'annotation-tool',
    ]

#ICEWS_ENCRYPT_BINARY = 'install/x86_64/NoThread/ICEWS/Release/BasicCipherStream/BasicCipherStreamEncryptFile'
ICEWS_ENCRYPT_BINARY = 'install/x86_64/NoThread/Classic/Release/BasicCipherStream/BasicCipherStreamEncryptFile'
ICEWS_STORY_SAMPLE_DIR = '/nfs/raid61/u10/ICEWS/STORIES/20090101'
AWAKE_UTILS_DIR = '/nfs/raid63/u10/awake_utils_20150529'
WKDB = '/nfs/raid63/u11/Active/Data/SERIF/english/wkdb/WKDB.db'

# Should we put these in a central location? Build DocSim from scratch?
DOCSIM = '/nfs/raid63/u10/users/azamania/TextGroup/Active/Projects/AWAKE/experiments/run_awake_pipeline/bin/x86_64/DocSim'
DJANGO_DIR = '/nfs/raid63/u10/users/azamania/TextGroup/Active/Projects/Spectrum/ActorDictionaryUI/Hypothesis_Validation/django'
DIJIT_DIR = '/nfs/raid63/u10/users/azamania/TextGroup/Active/Projects/Spectrum/ActorDictionaryUI/Hypothesis_Validation/static/js/dijit'
DOJO_DIR = '/nfs/raid63/u10/users/azamania/TextGroup/Active/Projects/Spectrum/ActorDictionaryUI/Hypothesis_Validation/static/js/dojo'
# Must have been pg_dump'ed with the option --no-owner!
INITIAL_AWAKE_DB_DUMP = '/nfs/raid63/u10/users/azamania/TextGroup/Active/Projects/AWAKE/lib/database/wicews_20150622_read_only.dump.sql'

AWAKE_PYTHON_SCRIPTS = [
    'add_indices.py',
    'adu_driver.py',
    'awake_db_import_utilities.py',
    'awake_m3s_converter.py',
    'convert_actor_db_from_postgres_to_sqlite.py',
    'drop_indices.py',
    'pull_stories.py',
    'WKDB.py',
    'AwakeDB.py',
    'prune_batch_file_for_duplicates.py'
]

#----------------------------------------------------------------------
# Process Command-line arguments.

parser = optparse.OptionParser(usage=USAGE)
parser.add_option("-v", action='count', dest='verbose', default=0,
                  help="Generate more verbose output")
parser.add_option("-q", action='count', dest='quiet', default=0,
                  help="Generate less verbose output")
parser.add_option('--include-retraining', dest='include_retraining',
                  action='store_true', default=False)
parser.add_option('--include-source', dest='include_source',
                  action='store_true', default=False)
parser.add_option('--include-english', action='append_const', dest='languages',
                  const=ENGLISH)
parser.add_option('--include-arabic', action='append_const', dest='languages',
                  const=ARABIC)
parser.add_option('--include-chinese', action='append_const', dest='languages',
                  const=CHINESE)
parser.add_option('--include-spanish', action='append_const', dest='languages',
                  const=SPANISH)
parser.add_option('--include-shared-libs', dest='include_shared_libs',
                  action='store_true', default=False)
parser.add_option('--include-java', dest='include_java',
                  action='store_true', default=False)
parser.add_option('--exclude-doc', dest='include_doc',
                  action='store_false', default=True)
parser.add_option('--exclude-scoring', dest='include_scoring',
                  action='store_false', default=True)
parser.add_option('--exclude-python', dest='include_python',
                  action='store_false', default=True)
parser.add_option('--icews', dest='icews',
                  action='store_true', default=False,
                  help="Include ICEWS files.")
parser.add_option('--awake-basic', dest='awake_basic',
                  action='store_true', default=False,
                  help="Include AWAKE SERIF parameter file and actor DB (but not AWAKE pipeline files)")
parser.add_option('--accent', dest='accent',
                  action='store_true', default=False,
                  help="Include ACCENT files.")
parser.add_option('--remove-manifest', dest='remove_manifest',
                  action='store_true', default=False,
                  help="Don't include a MANIFEST file in deliverable.")
parser.add_option('--msa', dest='msa',
                  action='store_true', default=False,
                  help="Collect MSA Serif data files for use in MSA delivery/installer")
parser.add_option('--icews-on-site', dest='icews_on_site',
                  action='store_true', default=False)
parser.add_option('--adu', dest='adu',
                  action='store_true', default=False,
                  help="Include ICEWS actor dictionary update files")
parser.add_option('--adu-ui', dest='adu_ui',
                  action='store_true', default=False,
                  help="When --adu is on, include UI as well")
parser.add_option('--distillation', dest='distillation',
                  action='store_true', default=False,
                  help="Include Distillation files.")
parser.add_option('--idx-names', dest='idx_names',
                  action='store_true', default=False,
                  help="Include IDX Names files.")
parser.add_option('--idx-relations', dest='idx_relations',
                  action='store_true', default=False,
                  help="Include IDX Relations files.")
parser.add_option('--license-file', dest='license_file',
                  default=None, help="When dry-running Serif, use this license file.")
parser.add_option('--dryrun', '--noop', dest='dryrun', action='store_true',
                  default=False)
parser.add_option('-f', '--force', dest='force', action='store_true',
                  default=False)
parser.add_option('--x64', action='append_const', dest='arch_suffixes',
                  const='x86_64', help="Install 64 bit binaries")
parser.add_option('--glibc27', action='append_const', dest='arch_suffixes',
                  const='x86_64_glibc27', help="Install 64 bit binaries")
parser.add_option('--x86', action='append_const', dest='arch_suffixes',
                  const='i686', help="Install 32 bit binaries")
parser.add_option('--win32',action='append_const', dest='arch_suffixes',
                  const='Win32',help="Install windows 32-bit binaries with dll")
parser.add_option('--win64',action='append_const', dest='arch_suffixes',
                  const='Win64',help="Install windows 64-bit binaries with dll")
parser.add_option('--coe-copyright', dest='copyright', action='store_const',
                  const='coe', help='Use the COE copyright notice.')
parser.add_option('--contract-4100426631-copyright', dest='copyright', action='store_const',
                  const='contract_4100426631', help='Use the Contract 4100426631 copyright notice.')
parser.add_option('--computable-insights-llc-copyright', dest='copyright', action='store_const',
                  const='computable_insights_llc',
                  help='Use the Computable Insights LLC copyright notice.')
parser.add_option('--generic-contract-copyright', dest='copyright', action='store_const',
                  const='generic_contract', help='Use the generic contract copyright notice.')
parser.add_option('--simple-copyright', dest='copyright', action='store_const',
                  const='simple_copyright', help='Use the simple copyright notice.')
parser.add_option('--ear99-copyright', dest='copyright', action='store_const',
                  const='ear99_copyright', help='Use the EAR99 copyright notice.')
parser.add_option('--contract-number', dest='contract_num',
                  help='Number to use in generic contract copyright notice.')
parser.add_option('--contract-date', dest='contract_date',
                  help='Date to use in generic contract copyright notice.')
parser.add_option('--contract-parties', dest='contract_parties',
                  help='Parties to use in generic contract copyright notice.')


parser.add_option('--xor-encrypt-data', action='store_true', dest='xor_encrypt',
                  default=False, help='Encrypt all SERIF data files using '
                  'BasicCipherStream.')
parser.add_option('--kba-streamcorpus', dest='kba_streamcorpus',
                  action='store_true', default=False,
                  help="Include KBA StreamCorpus files.")
parser.add_option('--include-kba-source', dest='include_kba_source',
                  action='store_true', default=False,
                  help="Include KBA stream corpus source, not binaries")

# We no longer symlink data, since we use trim_serif_data now.
#parser.add_option('--link-data', dest='link_data', action='store_true',
#                  default=False, help="Don't copy the data dir; just make a "
#                  "symlink.")

(options, args) = parser.parse_args()
if len(args) != 2:
    parser.error('Expected a release_dir and a deliverable_dir')
release_dir, deliverable_dir = args
if not os.path.exists(release_dir):
    parser.error('Release dir %r not found' % release_dir)
if not os.path.isdir(release_dir):
    parser.error('Release dir %r is not a directory' % release_dir)
if os.path.exists(deliverable_dir) and not options.force:
    parser.error('Deliverable dir %r already exists' % deliverable_dir)
if not options.languages:
    print 'No languages specified; including just English.'
    options.languages = [ENGLISH]
if not options.arch_suffixes:
    print 'No architecture specified; including 64 bit binaries.'
    options.arch_suffixes = ['x86_64']
verbosity = (options.verbose - options.quiet)
release_dir = os.path.abspath(release_dir)
deliverable_dir = os.path.abspath(deliverable_dir)
if options.copyright is None:
    copyright_notice = assert_serif_rights.DEFAULT_COPYRIGHT_NOTICE
elif options.copyright == 'coe':
    copyright_notice = assert_serif_rights.COE_COPYRIGHT_NOTICE
elif options.copyright == 'contract_4100426631':
    copyright_notice = assert_serif_rights.CONTRACT_4100426631_COPYRIGHT_NOTICE
elif options.copyright == 'generic_contract':
    copyright_notice = assert_serif_rights.GENERIC_CONTRACT_COPYRIGHT_NOTICE % {
        'contract_num' : options.contract_num,
        'contract_date' : options.contract_date,
        'contract_parties' : options.contract_parties,
        }
elif options.copyright == 'computable_insights_llc':
    copyright_notice = assert_serif_rights.COMPUTABLE_INSIGHTS_LLC_COPYRIGHT_NOTICE
elif options.copyright == 'simple_copyright':
    copyright_notice = assert_serif_rights.SIMPLE_COPYRIGHT_NOTICE
elif options.copyright == 'ear99_copyright':
    copyright_notice = assert_serif_rights.EAR99_COPYRIGHT_NOTICE
else:
    parser.error("Unknown copyright %s" % options.copyright)
if verbosity > 1:
    print '\nOptions:'
    print '  %-20s %r' % ('Release dir', release_dir)
    print '  %-20s %r' % ('Deliverable dir', deliverable_dir)
    for (k,v) in options.__dict__.items():
        if not k.startswith('_'):
            print '  %-20s %r' % (k,v)
    print

#----------------------------------------------------------------------
# ICEWS Deliverable Extras

if options.icews:
    RELEASE_BINARIES = ICEWS_BINARIES
    WINDOWS_BINARIES = ICEWS_WINDOWS_BINARIES
    PAR_FILES[ENGLISH] = ['master.english.par', 'master.icews.english.par', 'icews.english.xmltext.par', 'regtest.icews.english.xmltext.par', 'regtest.icews.english.sgm.par']
    PYTHON_SCRIPTS = ICEWS_PYTHON_SCRIPTS
    ENCRYPT_BINARY = ICEWS_ENCRYPT_BINARY
    assert ARABIC not in options.languages, 'ICEWS does not support arabic'
    assert CHINESE not in options.languages, 'ICEWS does not support chinese'

#----------------------------------------------------------------------
# ACCENT Deliverable Extras

if options.accent:
    RELEASE_BINARIES = []
    WINDOWS_BINARIES = []
    PAR_FILES[ENGLISH] = ['master.english.par', 'master.icews.english.par', 'icews.english.xmltext.par', 'regtest.accent.english.xmltext.par']
    PYTHON_SCRIPTS = []
    ENCRYPT_BINARY = ICEWS_ENCRYPT_BINARY
    assert ARABIC not in options.languages, 'ACCENT does not support arabic'
    assert CHINESE not in options.languages, 'ACCENT does not support chinese'

#----------------------------------------------------------------------
# MSA Deliverable Extras
if options.msa:
    WINDOWS_BINARIES = []
    PAR_FILES[ENGLISH] = ['master.english.par', 'regtest.msa.english.sgm.par', 'master.english-speed.par', 'master.awake.english.par']
    PYTHON_SCRIPTS = []
    ENCRYPT_BINARY = ICEWS_ENCRYPT_BINARY
    assert ARABIC not in options.languages, 'MSA does not support arabic'
    assert CHINESE not in options.languages, 'MSA does not support chinese'
 
#----------------------------------------------------------------------
# Actor Dictionary Update Deliverable Extras

if options.adu:
    assert options.icews, 'icews option must be on to use adu'
    assert not options.awake_basic, '--adu and --awake_basic cannot both be specified'
    PAR_FILES[ENGLISH].extend(['master.awake.english.par', 'regtest.awake.english.sgm.par'])
 
#----------------------------------------------------------------------
# Actor Dictionary Update Deliverable Extras

if options.awake_basic:
    PAR_FILES[ENGLISH].extend(['master.awake.english.par', 'regtest.awake.english.sgm.par'])

#----------------------------------------------------------------------
# Distillation Deliverable Extras

if options.distillation:
    PAR_FILES[ENGLISH].append('distill.best-english.par')
    assert ARABIC not in options.languages, 'Disillation does not support arabic'
    assert CHINESE not in options.languages, 'Distillation does not support chinese'
    assert SPANISH not in options.languages, 'Distillation does not support spanish'

#----------------------------------------------------------------------
# KbaStreamCorpus Deliverable Extras

if options.kba_streamcorpus:
    # Set binary location.
    RELEASE_BINARIES = KBA_STREAMCORPUS_BINARIES
    # Exclude python scripts.
    PYTHON_SCRIPTS = []
    SOURCE_DIRS.append('KbaStreamCorpus')
    assert ARABIC not in options.languages, 'KBA Streamcorpus does not support arabic'
    assert CHINESE not in options.languages, 'KBA Streamcorpus does not support chinese'
    assert copyright_notice==assert_serif_rights.COMPUTABLE_INSIGHTS_LLC_COPYRIGHT_NOTICE

if options.include_kba_source:
    SOURCE_DIRS.append('KbaStreamCorpus')

#----------------------------------------------------------------------
# IDX Deliverable Extras

if options.idx_names:
    # Set binary location.
    RELEASE_BINARIES = IDX_BINARIES
    WINDOWS_BINARIES = IDX_WINDOWS_BINARIES
    # Set parameter location
    PAR_FILES = { ENGLISH : ['cicero.names.best-english.par', 'master.english.par'] }
    ENCRYPT_BINARY = IDX_ENCRYPT_BINARY
    assert ARABIC not in options.languages, 'IDX does not support arabic'
    assert CHINESE not in options.languages, 'IDX does not support chinese'

if options.idx_relations:
    # Set binary location.
    RELEASE_BINARIES = IDX_BINARIES
    WINDOWS_BINARIES = IDX_WINDOWS_BINARIES
    # Set parameter location
    PAR_FILES = { ENGLISH : ['cicero.relations.fast-english.par', 'relations.fast-english.par', 'master.english.par'] }
    ENCRYPT_BINARY = IDX_ENCRYPT_BINARY
    assert ARABIC not in options.languages, 'IDX does not support arabic'
    assert CHINESE not in options.languages, 'IDX does not support chinese'

#----------------------------------------------------------------------
# Helper functions

def copy_dir(dirname, dirname2=None):
    src = os.path.join(release_dir, 'expts', 'Serif', dirname)
    dst = os.path.join(deliverable_dir, dirname2 or dirname)
    if not os.path.isdir(src):
        raise ValueError('Directory %r not found!' % src)
    _make_parent(dst)
    if verbosity > 0:
        log('copy dir %s -> %s' % (src, dst))
    if not options.dryrun:
        shutil.copytree(src, dst)

def copy_source_dir(dirname, dirname2=None, extensions=None):
    src = os.path.join(release_dir, 'expts', 'Serif', dirname)
    dst = os.path.join(deliverable_dir, dirname2 or dirname)
    if not os.path.isdir(src):
        raise ValueError('Directory %r not found!' % src)
    _make_parent(dst)
    if verbosity > 0:
        log('copy source dir %s -> %s' % (src, dst))
    if not options.dryrun:
        for warning in assert_serif_rights.copy_and_cleanup_source_dir(src, dst, copyright_notice, verbose=(verbosity>0), extensions=extensions):
            warn(warning)

def copy_dir_encrypted(dirname, dirname2=None, exclude=()):
    if isinstance(exclude, basestring): exclude=[exclude]
    src = os.path.join(release_dir, 'expts', 'Serif', dirname)
    dst = os.path.join(deliverable_dir, dirname2 or dirname)
    if not os.path.isdir(src):
        raise ValueError('Directory %r not found!' % src)
    _make_parent(dst)
    if verbosity > 0:
        log('copy dir %s -> %s (encrypted)' % (src, dst))
    if not options.dryrun:
        for src_subdir, subdirs, files in os.walk(src):
            assert src_subdir.startswith(src)
            relpath = src_subdir[len(src):].lstrip('/\\')
            dst_subdir = dst+'/'+relpath
            if not os.path.exists(dst_subdir):
                os.makedirs(dst_subdir)
            for filename in files:
                relfile = os.path.join(relpath, filename)
                if any(re.match(e, relfile) for e in exclude):
                    log('Excluding: %r' % relfile)
                else:
                    encrypt_file(os.path.join(src_subdir, filename),
                                 os.path.join(dst_subdir, filename))
            for subdir in subdirs[:]:
                if any(re.match(e, subdir) for e in exclude):
                    log('Excluding directory: %r' % subdir)
                    subdirs.remove(subdir)

BASIC_CIPHER_STREAM_ENCRYPT_BINARY=os.path.join(release_dir, 'expts', ENCRYPT_BINARY)
print BASIC_CIPHER_STREAM_ENCRYPT_BINARY
def encrypt_file(src, dst):
    assert os.path.exists(BASIC_CIPHER_STREAM_ENCRYPT_BINARY)
    devnull=open('/dev/null', 'wb')
    subprocess.check_call([BASIC_CIPHER_STREAM_ENCRYPT_BINARY, src, dst])

def make_dir(dirname):
    dst = os.path.join(deliverable_dir, dirname)
    if verbosity > 0:
        log('make dir %s' % dst)
    if not options.dryrun:
        os.makedirs(dst)

def copy_source_file(filename, filename2=None, exectuable=False):
    src = os.path.join(release_dir, 'expts', 'Serif', filename)
    dst = os.path.join(deliverable_dir, filename2 or filename)
    if not os.path.isfile(src):
        raise ValueError('File %r not found!' % src)
    _make_parent(dst)
    if verbosity > 0:
        log('copy source file %s -> %s' % (src, dst))
    if not options.dryrun:
        for warning in assert_serif_rights.copy_and_cleanup_source_file(src, dst, copyright_notice):
            warn(warning)
        if exectuable:
            os.chmod(dst, (stat.S_IXUSR|stat.S_IRUSR|stat.S_IWUSR|
                           stat.S_IXGRP|stat.S_IRGRP|
                           stat.S_IXOTH|stat.S_IROTH))

def copy_binary(filename, filename2=None):
    src = os.path.join(release_dir, 'expts', 'Serif', filename)
    dst = os.path.join(deliverable_dir, filename2 or filename)
    if not os.path.isfile(src):
        raise ValueError('File %r not found!' % src)
    _make_parent(dst)
    if verbosity > 0:
        log('copy binary %s -> %s' % (src, dst))
    if not options.dryrun:
        shutil.copy(src, dst)

def copy_template_file(filename, filename2=None):
    src = os.path.join(release_dir, 'expts', 'Serif', filename)
    dst = os.path.join(deliverable_dir, filename2 or filename)
    par_file_directory = os.path.dirname(os.path.realpath(src))
    if not os.path.isfile(src):
        raise ValueError('File %r not found!' % src)
    _make_parent(dst)
    if verbosity > 0:
        log('copy template file %s -> %s' % (src, dst))
    if not options.dryrun:
        shutil.copy(src, dst)

def copy_file(filename, filename2=None):
    src = os.path.join(release_dir, 'expts', 'Serif', filename)
    dst = os.path.join(deliverable_dir, filename2 or filename)
    if not os.path.isfile(src):
        raise ValueError('File %r not found!' % src)
    _make_parent(dst)
    if verbosity > 0:
        log('copy file %s -> %s' % (src, dst))
    if not options.dryrun:
        shutil.copy(src, dst)

def copy_param_file(filename, filename2=None):
    src = os.path.join(release_dir, 'expts', 'Serif', filename)
    dst = os.path.join(deliverable_dir, filename2 or filename)
    par_file_directory = os.path.dirname(os.path.realpath(src))
    actor_db = os.path.realpath(os.path.join(par_file_directory, '..', 'awake', 'lib', 'database', 'actor_db.icews.sqlite'))
    if not os.path.isfile(src):
        raise ValueError('File %r not found!' % src)
    _make_parent(dst)
    if verbosity > 0:
        log('copy and adjust param file %s -> %s' % (src, dst))
    if not options.dryrun:
        s = open(src).read()
        # Set variables.
        s = s.replace('+parallel+', '000')
        s = s.replace('+start_stage+', 'START')
        s = s.replace('+end_stage+', 'output')
        s = s.replace('+state_saver_stages+', 'NONE')
        s = s.replace('+par_dir+', '.')
        s = s.replace('+icews_lib_dir+', '../icews/lib')
        s = s.replace('+bbn_actor_db+', 'sqlite://' + actor_db + '?readonly&copy&cache_size=30000&temp_store=MEMORY')
        s = s.replace('+awake_lib_dir+', '../awake/lib')

        # Use an include to get serif data & serif score.
        s = re.sub('serif_data:\s+\+serif_data\+',
               'INCLUDE ./config.par', s)
        s = re.sub('serif_score:\s+\+serif_score\+', '', s)

        # Comment out any batch file or experiment dir definitions.
        s = re.sub(r'(?m)^((batch_file|experiment_dir).*)', r'#\1', s)

        # Sanity check: make sure we removed all variables.
        for lineno, line in enumerate(s.split('\n')):
            line = line.split('#',1)[0]
            if '+' in line:
                warn('"+" found on line %d of %s:\n  %s' %
                     (lineno, src, line))
        out = open(dst, 'w')
        out.write(s)
        out.close()

def make_symlink(filename, filename2=None):
    src = os.path.join(release_dir, 'expts', 'Serif', filename)
    dst = os.path.join(deliverable_dir, filename2 or filename)
    if not os.path.exists(src):
        raise ValueError('File or directory %r not found!' % src)
    _make_parent(dst)
    if verbosity > 0:
        log('make symlink %s -> %s' % (src, dst))
    if not options.dryrun:
        os.symlink(src, dst) #@UndefinedVariable

def create_file(filename, contents):
    path = os.path.join(deliverable_dir, filename)
    if verbosity > 0:
        log('create %s' % path)
    if not options.dryrun:
        parent, child = os.path.split(path)
        if not os.path.isdir(parent):
            os.makedirs(parent)
        out = open(path, 'wb')
        out.write(contents)
        out.close()

def create_manifest():
    path = os.path.join(deliverable_dir, 'MANIFEST')
    if verbosity > 0:
        log('create manifest %s' % path)
    if not options.dryrun:
        out = open(path, 'wb')
        cmd = ['find', deliverable_dir, '-printf', '%P\\n']
        subprocess.check_call(cmd, stdout=out)
        out.close()

def log(msg):
    if verbosity <= 0: return
    if options.dryrun:
        print textwrap.fill(msg, initial_indent='[DRYRUN] ',
                            subsequent_indent='              ')
    else:
        print textwrap.fill(msg, subsequent_indent='        ')

warnings = []
def warn(warning):
    print textwrap.fill(warning, initial_indent='WARNING: ',
                        subsequent_indent='         ')
    warnings.append(warning)

def show_warnings():
    if warnings:
        print 'WARNINGS:'
        for warning in warnings:
            print textwrap.fill(warning, initial_indent='  - ',
                                subsequent_indent='    ')
        print

def _make_parent(dst):
    parent_dir = os.path.split(dst)[0]
    if not os.path.exists(parent_dir):
        make_dir(os.path.abspath(parent_dir))

#----------------------------------------------------------------------
# Copy relevant sub-directories.

if not options.accent and not options.msa:
    make_dir('bin')
if options.include_doc:
    copy_dir('doc')
if options.include_scoring:
    copy_source_dir('scoring')
if options.include_retraining:
    copy_source_dir('experiments')

#----------------------------------------------------------------------
# Copy primary release binaries:
for arch_suffix in options.arch_suffixes:
    if 'Win' in arch_suffix:
        if 'Win32' in arch_suffix:
            xerces_suffix = 'win32'
        else:
            xerces_suffix = 'x64'
        for (src, dst) in WINDOWS_BINARIES:
            kw = {'ARCH_SUFFIX': arch_suffix,
                  'XERCES_SUFFIX' : xerces_suffix }
            src = src % kw
            dst = os.path.join('bin', dst % kw)
            copy_binary(src, dst)
    else:
        for (src, dst) in RELEASE_BINARIES:
            kw = {'ARCH_SUFFIX': arch_suffix}
            src = src % kw
            dst = os.path.join('bin', dst % kw)
            copy_binary(src, dst)

#----------------------------------------------------------------------
# Copy shared libraries and dependencies:
if options.include_shared_libs:

    for language in options.languages:
        for arch_suffix in options.arch_suffixes:
            for (src, dst) in SHARED_LIBRARIES:
                kw = {'LANGUAGE': language, 'ARCH_SUFFIX': arch_suffix}
                src = src % kw
                dst = os.path.join('bin', dst % kw)
                copy_binary(src, dst)

    for arch_suffix in options.arch_suffixes:
        if arch_suffix == 'i686': arch_suffix2 = ''
        else: arch_suffix2 = '-'+arch_suffix
        kw = {'ARCH_SUFFIX': arch_suffix, 'ARCH_SUFFIX2': arch_suffix2}
        for lib in SHARED_LIB_DEPENDENCIES:
            src = lib % kw
            dst = os.path.join('bin', arch_suffix, os.path.split(src)[1])
            copy_binary(src, dst)

#----------------------------------------------------------------------
# Copy training binaries

if options.include_retraining:
    for langauge in options.languages:
        for arch_suffix in options.arch_suffixes:
            for binary in TRAINING_BINARIES:
                src = os.path.join('../install/%s/%s/NoThread/Classic/Release/' %
                                   (language, arch_suffix),
                                   binary)
                dst = os.path.join('bin/%s/%s' % (arch_suffix, language), binary)
                copy_binary(src, dst)

#----------------------------------------------------------------------
# Copy parser directory

if options.include_retraining:
    copy_source_file('src/ParserTrainer/serif-one-step-k-train.pl')
    copy_source_file('src/ParserTrainer/serif-one-step-train.pl')
    copy_source_file('src/ParserTrainer/serif-one-step-train-no-lambdas.pl')
    for language in options.languages:
        copy_source_file('src/ParserTrainer/serif-step1-collect-prune.pl',
                  'parser/%s/serif-step1-collect-prune.pl' % language.lower())
        copy_source_file('src/ParserTrainer/serif-step2-smooth.pl',
                  'parser/%s/serif-step2-smooth.pl' % language.lower())
        copy_source_file('src/ParserTrainer/serif-step3-derive-tables.pl',
                  'parser/%s/serif-step3-derive-tables.pl' % language.lower())
        copy_source_file('src/ParserTrainer/serif-step3-derive-tables-no-lambda.pl',
                  'parser/%s/serif-step3-derive-tables-no-lambda.pl' %
                  language.lower())
        copy_binary('../install/%s/%s/NoThread/Classic/Release/ParserTrainer/DeriveTables' %
                  (language, options.arch_suffix),
                  'parser/%s/DeriveTables' % language.lower())
        copy_binary('../install/%s/%s/NoThread/Classic/Release/ParserTrainer/Headify' %
                  (language, options.arch_suffix),
                  'parser/%s/Headify' % language.lower())
        copy_binary('../install/%s/%s/NoThread/Classic/Release/ParserTrainer/K_Estimator' %
                  (language, options.arch_suffix),
                  'parser/%s/K_Estimator' % language.lower())
        copy_binary('../install/%s/%s/NoThread/Classic/Release/ParserTrainer/StatsCollector' %
                  (language, options.arch_suffix),
                  'parser/%s/StatsCollector' % language.lower())
        copy_binary('../install/%s/%s/NoThread/Classic/Release/ParserTrainer/VocabCollector' %
                  (language, options.arch_suffix),
                  'parser/%s/VocabCollector' % language.lower())
        copy_binary('../install/%s/%s/NoThread/Classic/Release/ParserTrainer/VocabPruner' %
                  (language, options.arch_suffix),
                  'parser/%s/VocabPruner' % language.lower())

#----------------------------------------------------------------------
# Copy parameter files

deliverable_par_file_dir = 'par'
if options.accent:
    deliverable_par_file_dir = os.path.join('installation', 'par')
for par_file in sum([PAR_FILES[l] for l in options.languages], []):
    copy_param_file(os.path.join('par', par_file), os.path.join(deliverable_par_file_dir, par_file))

#----------------------------------------------------------------------
# Create config file

CONFIG_FILE = """\
#######################################################################
## Serif Global Configuration
#######################################################################
##
## This file tells Serif where it can find trained model files and the
## scripts that are used for scoring.  It is imported by each of the
## other parameter files (such as all.best-english.par).  It is not
## intended to be used directly.
##
## This file needs to be updated if Serif is moved or copied to a new
## location.  No other files should need to be changed.
##
#######################################################################

serif_home: ..

serif_data: %%serif_home%%/data
serif_score: %%serif_home%%/scoring
server_docs_root: %%serif_home%%/doc/http_server
"""
if options.xor_encrypt:
    CONFIG_FILE += 'use_feature_module_BasicCipherStream: true\n'
    CONFIG_FILE += 'cipher_stream_always_decrypt: true\n'
if options.icews:
    CONFIG_FILE += 'log_threshold: ERROR\n'
if options.kba_streamcorpus:
    CONFIG_FILE += 'use_feature_module_KbaStreamCorpus: true\n'
    CONFIG_FILE += 'use_feature_module_HTMLDocumentReader: true\n'
    CONFIG_FILE += ('html_character_entities: %%serif_data%%/unspec/misc/'
                    'html-character-entities.listmap\n')
    CONFIG_FILE += 'log_threshold: ERROR\n'
config_file = CONFIG_FILE % dict(
    deliverable=deliverable_dir,
    )
create_file(os.path.join(deliverable_par_file_dir, 'config.par'), config_file)

#----------------------------------------------------------------------
# Copy source files

if options.include_source:
    copy_source_file(os.path.join('src', 'CMakeLists.txt'))
    copy_source_file(os.path.join('src', 'export.cmake.in'))
    for source_dir in SOURCE_DIRS:
        copy_source_dir(os.path.join('src', source_dir))

#----------------------------------------------------------------------
# Copy python scripts
if options.include_python:

    for script in PYTHON_SCRIPTS:
        copy_source_file(os.path.join('python', script))

#----------------------------------------------------------------------
# Copy java files
if options.include_java:

    for java_file in JAVA_FILES:
        copy_binary(os.path.join('..', 'build', 'java', java_file),
                    os.path.join('java', java_file))
    for java_dir in JAVA_SUBDIRS:
        copy_source_dir(os.path.join('..', 'build', 'java', java_dir),
                        os.path.join('java', java_dir))

    # Put a copy of the java interface docs in the main doc dir.
    copy_dir(os.path.join('..', 'build', 'java', 'doc'),
             os.path.join('doc', 'java'))
    
#----------------------------------------------------------------------
# Copy ICEWS only lib files

if options.icews and not options.accent:
    copy_dir_encrypted('icews/lib', exclude=ICEWS_EXCLUDES)
    
    # We can't just copy the database/ folder as part of copy_dir_encrypted, because
    #  we don't want it encrypted.
    copy_binary('icews/lib/database/icews.current.sqlite',
                'icews/lib/database/icews.current.sqlite')
    
    # ICEWS test scripts & supporting files:
    copy_source_file('icews/lib/demo_scripts/test_delivery.py', 'test_delivery.py', True)

    copy_dir(ICEWS_STORY_SAMPLE_DIR, 'icews_test/documents')
    
    if options.icews_on_site:
        # Files to support on-site environment under BBN control
        copy_source_dir(os.path.join('icews', 'scripts'), 'scripts')
        copy_binary(os.path.join('icews', 'annotation-tool', 'ICEWS-annotation-tool.jar'),
                    os.path.join('tool', 'ICEWS-annotation-tool.jar'))
        copy_template_file(os.path.join('icews', 'annotation-tool', 'sample-param.par'),
                           os.path.join('tool', 'sample-param.par'))
        copy_source_file(os.path.join('icews', 'annotation-tool', 'event-code-descriptions.txt'),
                         os.path.join('tool', 'event-code-descriptions.txt'))

#----------------------------------------------------------------------
# Copy ACCENT only lib files

if options.accent:
    copy_dir_encrypted('icews/lib', 'installation/icews/lib', exclude=ICEWS_EXCLUDES)
    
    # We can't just copy the database/ folder as part of copy_dir_encrypted, because
    #  we don't want it encrypted.
    copy_binary('icews/lib/database/icews.current.sqlite',
                'installation/icews/lib/database/icews.current.sqlite')

    # Put in top directory for easy access
    binary_loc = None
    if len(options.arch_suffixes) != 1:
        raise ValueError('Must have one arch_suffix for ACCENT' % src)
    binary = os.path.join('../install', options.arch_suffixes[0], 'NoThread/Classic/Release/SerifMain/Serif')

    copy_binary(binary, 'installation/bin/BBN_ACCENT')
    copy_file('accent/lib/event-code-descriptions.txt', 'installation/icews/event_code_descriptions.txt')
    copy_source_file('accent/lib/scripts/cameoxml.py', 'installation/cameoxml/cameoxml.py')
    copy_source_file('accent/lib/scripts/cameoxml_viewer.py', 'installation/cameoxml/cameoxml_viewer.py')

    # Sample input and output files
    copy_dir('accent/sample_files', 'sample_test')
    
    # Additional par files
    copy_param_file('accent/par/master.accent.english.par', 'installation/par/master.accent.english.par')
    copy_file('accent/par/accent.text.par', 'sample_test/par/accent.text.par') # Use copy_file so as not to comment out experiment_dir
    copy_file('accent/par/accent.sgml.par', 'sample_test/par/accent.sgml.par') 

    # Parameter file generator
    copy_source_file('accent/lib/parameter_generator/parameter_tool_builder_generalized.py', 'installation/parameter_generator/parameter_file_builder.py')
    copy_file('accent/lib/parameter_generator/parameter_queries.xml', 'installation/parameter_generator/parameter_queries.xml')

    # Put guide at top level
    copy_file('accent/doc/BBN_ACCENT_Installation_and_Invocation.docx', 'BBN_ACCENT_Installation_and_Invocation.docx')

#----------------------------------------------------------------------
# Copy MSA only lib files

if options.msa:
    copy_file('awake/lib/database/actor_db.freebase.sqlite', 'awake/lib/database/actor_db.freebase.sqlite')
        
#----------------------------------------------------------------------
# Copy ADU files
if options.adu:
    copy_dir(AWAKE_UTILS_DIR, os.path.join('bin', 'awake_utils'))

    copy_source_dir(os.path.join('awake', 'lib', 'actor_dictionary_update'))
    copy_source_dir(os.path.join('awake', 'lib', 'novel_actor_discovery'))
    copy_dir(os.path.join('awake', 'lib', 'actors'))
    copy_dir(os.path.join('awake', 'lib', 'ontologies'))
    copy_source_dir(os.path.join('awake', 'lib', 'single_machine_sequence'))

    copy_source_file(os.path.join('marathon', 'IO', 'wbq_params.py'), os.path.join('awake', 'lib', 'IO', 'wbq_params.py'))
    copy_source_file(os.path.join('marathon', 'IO', '__init__.py'), os.path.join('awake', 'lib', 'IO', '__init__.py'))
    copy_template_file(os.path.join('awake', 'experiments', 'run_awake_pipeline', 'templates', 'make_batch_files.sh'), os.path.join('awake', 'lib', 'scripts', 'make_batch_files.sh'))

    for script in AWAKE_PYTHON_SCRIPTS:
        copy_source_file(os.path.join('awake', 'lib', 'scripts', script), os.path.join('awake', 'lib', 'scripts', script))

    copy_template_file(os.path.join('awake', 'experiments', 'run_awake_pipeline', 'templates', 'uploader_config.xml'), os.path.join('awake', 'lib', 'templates', 'uploader_config.xml'))
    copy_template_file(os.path.join('awake', 'lib', 'templates', 'pull_stories.par'), os.path.join('awake', 'lib', 'templates', 'pull_stories.par'))
    copy_template_file(os.path.join('awake', 'experiments', 'run_awake_pipeline', 'templates', 'dedup.par'), os.path.join('par', 'dedup.par'))
    copy_template_file(os.path.join('awake', 'lib', 'database', 'postgres_kb_indexes.txt'), os.path.join('awake', 'lib', 'database', 'postgres_kb_indexes.txt'))

    copy_binary(DOCSIM, os.path.join('bin', 'x86_64', 'DocSim'))
    copy_binary(WKDB, os.path.join('data', 'english', 'wkdb', 'WKDB.db'))

    shutil.copy(INITIAL_AWAKE_DB_DUMP, os.path.join(deliverable_dir, 'awake', 'lib', 'database', 'initial_awake_db.dump.sql'))

    if options.adu_ui:
        copy_template_file(os.path.join('spectrum', 'ActorDictionaryUI', 'Hypothesis_Validation', 'Hypothesis_Validation', 'settings.py'), os.path.join('awake', 'lib', 'templates', 'settings.py'))
        copy_source_dir(os.path.join('spectrum', 'ActorDictionaryUI'), os.path.join('awake', 'lib', 'ActorDictionaryUI'), extensions=['.py'])
        copy_dir(DJANGO_DIR, os.path.join('awake', 'lib', 'ActorDictionaryUI', 'Hypothesis_Validation', 'django'))
        copy_dir(DIJIT_DIR, os.path.join('awake', 'lib', 'ActorDictionaryUI', 'Hypothesis_Validation', 'static', 'js', 'dijit'))
        copy_dir(DOJO_DIR, os.path.join('awake', 'lib', 'ActorDictionaryUI', 'Hypothesis_Validation', 'static', 'js', 'dojo'))
        shutil.copy(os.path.join(release_dir, 'expts', 'Serif', 'spectrum', 'annotation', 'Actor-Dictionary-Update-annotation-guidelines.docx'),
                    os.path.join(deliverable_dir, 'Actor-Dictionary-Update-annotation-guidelines.docx'))

#----------------------------------------------------------------------
# Copy AWAKE dictionary
if options.awake_basic:
    # copy both basic AWAKE dictionaries for now...
    copy_file('awake/lib/database/actor_db.freebase.sqlite', 'awake/lib/database/actor_db.freebase.sqlite')
    copy_file('awake/lib/database/actor_db.icews.sqlite', 'awake/lib/database/actor_db.icews.sqlite')

#----------------------------------------------------------------------
# Copy StreamCorpus source files

if options.kba_streamcorpus:
    # Copy parameter files used by streamcorpus.
    for step in [1,2,3]:
        parfile = 'streamcorpus_step%d.par' % step
        copy_param_file(os.path.join('kba-streamcorpus', parfile),
                        os.path.join('par', parfile))
    copy_param_file(os.path.join('kba-streamcorpus', 'streamcorpus_all.par'),
                    os.path.join('par', 'streamcorpus_all.par'))
    # Copy shell scripts
    copy_source_file(os.path.join('kba-streamcorpus',
                                  'serif_streamcorpus.sh'),
                     'idx_streamcorpus.sh', True)
    copy_source_file(os.path.join('kba-streamcorpus',
                                  'serif_streamcorpus_all.sh'),
                     'idx_streamcorpus_all.sh', True)
    # Copy HTML entities file that isn't included in trim_serif_data
    entities_src = os.path.join(release_dir, 'expts', 'Serif', 'data', 'unspec', 'misc',
    	'html-character-entities.listmap')
    entities_dst = os.path.join(deliverable_dir, 'data', 'unspec', 'misc',
    	'html-character-entities.listmap')
    assert os.path.exists(entities_src)
    parentdir = os.path.split(entities_dst)[0]
    if not os.path.exists(parentdir):
        os.makedirs(parentdir)
    if options.xor_encrypt:
    	subprocess.check_call([BASIC_CIPHER_STREAM_ENCRYPT_BINARY, entities_src, entities_dst])
    else:
    	shutil.copy(entities_src, entities_dst)

#----------------------------------------------------------------------
# Copy IDX license

if options.idx_names or options.idx_relations:
    # Copy license file that isn't included in trim_serif_data
    license_src = os.path.join(release_dir, 'expts', 'Serif', 'data', 'unspec', 'misc',
    	'cicero.lic')
    license_dst = os.path.join(deliverable_dir, 'data', 'unspec', 'misc',
    	'cicero.lic')
    assert os.path.exists(license_src)
    parentdir = os.path.split(license_dst)[0]
    if not os.path.exists(parentdir):
        os.makedirs(parentdir)
    shutil.copy(license_src, license_dst)

#----------------------------------------------------------------------
# Collect data files

no_encrypt_files = []
no_encrypt_files.append('SERIF.lic')
no_encrypt_files.append('Serif.lic')
serif_bin_dir = os.path.join(deliverable_dir, 'bin', 'x86_64')
if options.accent:
    serif_bin_dir = os.path.join(deliverable_dir, 'installation', 'bin')
deliverable_data_dir = 'data'
if options.accent:
    deliverable_data_dir = os.path.join('installation', 'data')
    no_encrypt_files.append('BBN_ACCENT.lic')
if options.icews:
    no_encrypt_files.append('ICEWS.lic')
if options.adu:
    no_encrypt_files.append('common_nicknames.txt') # Used by novel actors as well as Serif

for language in options.languages:
    parfiles = PAR_FILES[language]
    config_list = []
    for parfile in parfiles:
        use_parfile = False
        if options.icews or options.accent:
            use_parfile = re.match('^regtest', parfile)
        else:
            use_parfile = not re.match('^master', parfile)
        if use_parfile:
            config_list.append((language, parfile))
            if verbosity > 0:
                log('Including data files for configuration %s' % parfile)
    if options.xor_encrypt:
        copy_binary = BASIC_CIPHER_STREAM_ENCRYPT_BINARY
    else:
        copy_binary = None # use shutil.copy
    if not options.dryrun:
        print 'Copying Files used by SERIF'
        trim_serif_data.copy_files_used_by_serif(config_list,
                             serif_bin_dir,
                             os.path.join(release_dir, 'expts', 'Serif', 'par'),
                             os.path.join(release_dir, 'expts', 'Serif', 'scoring'),
                             os.path.join(release_dir, 'expts', 'Serif', 'data'),
                             os.path.join(deliverable_dir, deliverable_data_dir),
                             options.license_file,
                             copy_binary, verbose=(verbosity>0),
                             # Used by novel actor system as well as Serif
                             copy_binary_exceptions=no_encrypt_files)
        print 'Testing copied files'
        trim_serif_data.test_copy(config_list,
                             serif_bin_dir,
                             os.path.join(deliverable_dir, deliverable_par_file_dir),
                             os.path.join(deliverable_dir, 'scoring'),
                             os.path.join(deliverable_dir, deliverable_data_dir),
                             options.license_file)

#----------------------------------------------------------------------
# Create readme & release notes

README = """\
%(copyright_notice)s

     Raytheon BBN Technologies Corp.
     10 Moulton St.
     Cambridge, MA 02138
     617-873-3411

All Rights Reserved.
--------------------

Serif Release Notes
~~~~~~~~~~~~~~~~~~~
            Release: %(release)s
        Deliverable: %(deliverable)s
           Built on: %(date)s

          Languages: %(languages)s
     Include source: %(include_source)s
 Include retraining: %(include_retraining)s
Include shared libs: %(include_shared_libs)s
       Include java: %(include_java)s
      Architectures: %(architectures)s

"""
readme = README % dict(
    copyright_notice=assert_serif_rights.get_script_copyright_notice(copyright_notice),
    release=os.path.split(release_dir)[1],
    deliverable=os.path.split(deliverable_dir)[1],
    date=time.ctime(),
    languages=', '.join(sorted(options.languages)),
    include_source=('Yes' if options.include_source else 'No'),
    include_retraining=('Yes' if options.include_retraining else 'No'),
    include_shared_libs=('Yes' if options.include_shared_libs else 'No'),
    include_java=('Yes' if options.include_java else 'No'),
    architectures=', '.join(map(ARCH_DICT.get,options.arch_suffixes)),
    )
if os.path.exists(os.path.join(release_dir, 'README.txt')):
    release_notes = open(os.path.join(release_dir, 'README.txt'), 'rb').read()
    readme += re.sub('^.*Release Information\n*', '', release_notes)
if options.accent:
    readme = re.sub('serif', '...', readme, flags=re.IGNORECASE)
create_file('README.txt', readme)

#----------------------------------------------------------------------
# Create manifest

if not options.accent and not options.remove_manifest:
    create_manifest()

#----------------------------------------------------------------------
# All done!

print
if verbosity > 0:
    print readme
if options.dryrun:
    print 'Dryrun complete!  (No files actually copied.)'
else:
    print 'Deliverable complete:\n  %s' % deliverable_dir
    print
    print ('Please edit "README.txt" to include additional information '
           'about this release.')
print
show_warnings()
