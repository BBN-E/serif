#
# This essentially does the same thing as trim_serif_data.py --
# copy just the files from a Serif data repository that were
# used in a Serif run, but unlike trim_serif_data.py, this
# doesn't need to be run in the context of a Serif
# deliverable creation process. 
# 
# This script gets the list of files from a files_read.log
# file which is created by Serif when the parameter
# track_files_read is set to true. 
#
# Works only with Linux paths.
#

import sys, os, shutil, argparse, subprocess

def make_parent_dir(d):
    parentdir = os.path.split(d)[0]
    if not os.path.exists(parentdir):
        os.makedirs(parentdir)

def copy(src, dst):
    make_parent_dir(dst)

    if os.path.exists(dst) and os.path.isdir(dst):
        shutil.rmtree(dst)

    if os.path.isdir(src):
        shutil.copytree(src, dst)
    else:
        shutil.copy(src, dst)
    
parser = argparse.ArgumentParser(description="Prune Serif data repository")
parser.add_argument("files_read_log", help="A files_read.log file output by Serif")
parser.add_argument("output_serif_data_repo", help="Where the pruned Serif data repository will be created")
parser.add_argument("--orig_serif_data_repo", default="/d4m/serif/data", help="The Serif data repo that was used in the Serif run which created files_read.log")
parser.add_argument("--encrypt_binary", help="A BasicCipherStreamEncryptFile binary that will encrypt and copy files")
args = parser.parse_args()

i = open(args.files_read_log, 'r')
for line in i:
    if not line.startswith(args.orig_serif_data_repo):
        continue
    line = line.strip()
    src = line
    subpath = src[len(args.orig_serif_data_repo):]

    if subpath.startswith("/"):
        subpath = subpath[1:]
    dst = os.path.join(args.output_serif_data_repo, subpath)
    
    if args.encrypt_binary:
        make_parent_dir(dst)
        subprocess.check_call([args.encrypt_binary, src, dst])
        #print args.encrypt_binary + " " + src + " " + dst
    else:
        copy(src, dst)
    
i.close()

# Extras that don't show up in files_read.log, but
# We'll probably want for CauseEx and WorldModelers
for subpath in ("english/Software", "ace", "english/tokenization", "english/misc", "english/values/pidf/no-timex-plus-nums.tags"):
    src = os.path.join(args.orig_serif_data_repo, subpath)
    dst = os.path.join(args.output_serif_data_repo, subpath)
    copy(src, dst)

