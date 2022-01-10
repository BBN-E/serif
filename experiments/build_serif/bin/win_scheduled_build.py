###
# SERIF Windows Scheduled Build Script
#
# This script runs cmake to build Visual Studio project files, and
# then uses Visual Studio to compile the standard build of Serif.
#
# Based on the win_build.py template from the build-serif sequence.
# Expected to be run by Windows Task Scheduler.
###

import os
import sys
import subprocess
import re
import shutil

# Static parameters for build on Windows Server 2012
cmake_generator = 'Visual Studio 9 2008 Win64'
cmake_options = '-DSYMBOL_REF_COUNT=ON -DSYMBOL_THREADSAFE=OFF -DFEATUREMODULE_Spanish_INCLUDED=ON'
daily_build = '\\\\raid68\\u17\\serif\\daily_build'
build_drive = 'Q:'
boost_version = '1_59_0'
release_name = 'Release'
project_names = {
    'CMake\\Solutions\\Serif\\Serif.sln': ['Serif', ],
}

# Determine the most recent daily build
daily_build_name = sorted([d for d in os.listdir(daily_build) if d.startswith("SerifDaily") and not d.endswith("gz")])[-1]
daily_build_dir = os.path.join(daily_build, daily_build_name)
rev_name = os.listdir(os.path.join(daily_build_dir, 'expts', 'build'))[0]

# Determine build and source paths
build_dir = os.path.join(daily_build, daily_build_name,
                         'expts', 'build', rev_name, 'Win64',
                         'NoThread', 'Classic', release_name)
src_dir = os.path.join(daily_build_dir, 'expts', 'Serif', rev_name, 'src')
xerces_root = os.path.join(src_dir, '..', 'external', 'XercesLib')
openssl_root = os.path.join(src_dir, '..', 'external', 'openssl-1.0.1g')

# If we're re-run, then always build from scratch.
if os.path.exists(build_dir):
    print "Warning: build_dir (%s) already exists; erasing it" % build_dir
    shutil.rmtree(build_dir)
os.makedirs(build_dir)

# Determine where Boost is installed
boost_root = 'C:\\boost_%s' % boost_version
if not os.path.isdir(boost_root):
    boost_root = boost_root.replace('C:', 'D:')
assert os.path.isdir(boost_root), ('Boost %s not found on C: or D: drives' % boost_version)

# Set environment variables appropriately.  (Don't run as admin!)
os.environ['BOOST_ROOT'] = boost_root
os.environ['XERCESCROOT'] = xerces_root
os.environ['OPENSSL_ROOT'] = openssl_root
os.environ.setdefault('ProgramFiles', 'C:\\Program Files')

print 'Mounting %s on %s' % (build_dir, build_drive)
sys.stdout.flush()
m = re.match(r'(\\\\[^\\]+\\[^\\]+)(.*)', build_dir)
assert m is not None, ('Invalid winshare path: %s' % build_dir)
(share_name, rest_of_path) = m.groups()
subprocess.check_call(['net', 'use', build_drive,
                      share_name, '/persistent:no'])

# Run cmake
cmake = "C:\\Program Files (x86)\\CMake 2.8\\bin\\cmake.exe"
working_dir = build_drive + rest_of_path
print 'Running %s in %s' % (cmake, working_dir)
sys.stdout.flush()
log_filename = os.path.join(build_dir, 'cmake.log')
log = open(log_filename, 'wb')
err_filename = os.path.join(build_dir, 'cmake.err')
err = open(err_filename, 'wb')
subprocess.check_call([cmake, '-G', cmake_generator, src_dir] +
                      cmake_options.split(), stdout=log, stderr=err,
                      cwd=working_dir)
log.close()
err.close()
print open(err_filename, 'rb').read()
print open(log_filename, 'rb').read()

# Run Visual Studio
visual_studio = "C:\\Program Files (x86)\\Microsoft Visual Studio 9.0\\Common7\\IDE\\devenv.com"
print 'Running Visual Studio %s' % visual_studio
sys.stdout.flush()
build_filename = os.path.join(build_dir, 'build.log')
for solution, projects in project_names.iteritems():
    for project in projects:
        subprocess.check_call([visual_studio,
                               os.path.join(build_dir, solution),
                               '/rebuild', release_name,
                               '/project', project,
                               '/out', build_filename])

print 'Unmounting %s' % build_drive
subprocess.check_call(['net', 'use', build_drive, '/delete', '/yes'])
