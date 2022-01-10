##
## SERIF Windows Build Script
##
## This script runs cmake to build Visual Studio project files, and
## then uses Visual Studio to compile a specified version of SERIF.
## The build directory is assumed to be on a Windows share drive
## (e.g. "\\titan\u70"); that drive is mounted as a local disk
## (usually T:) for the duration of the build.

#----------------------------------------------------------------------
# Parameters from runjobs

build_dir       = r'+build_dir+'
src_dir         = r'+src_dir+'
cmake_generator = r'+cmake_generator+'
cmake_options   = r'+cmake_options+'
build_drive     = r'+build_drive+'
boost_version   = r'+boost_version+'
xerces_root     = r'+xerces_root+'
openssl_root    = r'+openssl_root+'
yamcha_root     = r'+yamcha_root+'
release_name    = r'+release_name+'
project_names   = +project_names+

#----------------------------------------------------------------------
# Script

import os, os.path, sys, subprocess, re, shutil

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
os.environ['YAMCHA_ROOT'] = yamcha_root
for v in ['USERNAME', 'USERPROFILE']:
    os.environ[v] = os.environ[v].replace('Administrator', os.environ['USER'])
os.environ.setdefault('ProgramFiles', 'C:\Program Files')

print 'Mounting %s on %s' % (build_dir, build_drive)
sys.stdout.flush()
m = re.match(r'(\\\\[^\\]+\\[^\\]+)(.*)', build_dir)
assert m is not None, ('Invalid winshare path: %s' % build_dir)
(share_name, rest_of_path) = m.groups()
subprocess.check_call(['net', 'use', build_drive, share_name,'/persistent:no'])

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
print 'Running Visual Studio'; sys.stdout.flush()
for solution, projects in project_names.iteritems():
    for project in projects:
        subprocess.check_call(['devenv.com', os.path.join(build_dir, solution),
                               '/rebuild', release_name,
                               '/project', project,
                               '/out', '%s\\build.log' % build_dir])

print 'Unmounting %s' % build_drive
subprocess.check_call(['net', 'use', build_drive, '/delete'])
