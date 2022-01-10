#!/usr/bin/env bash
#
# Subversion checkout script for Serif build experiment.
# 
#   SVN_URL: +SVN_URL+
#     The base URL from which Serif should be checked out.  This will
#     be either the base URL of the text repository's Active
#     directory, or the URL of a tagged version of the text
#     repository.
#
#   SVN_REVISION: +SVN_REVISION+
#     The SVN revision which should be checked out.  Use "HEAD" for
#     the current revision.
#
#   TARGET: +TARGET+
#     The directory that the SVN repository should be checked out to.
#     If it already exists, it will be deleted before it is checked out.
#
#   SUBDIRS: +SUBDIRS+
#     The list of subdirectories that should be checked out from the 
#     repository.  This should typically include at least 'src', 'par',
#     and 'scoring'.  Include 'data' as well to make sure that the 
#     svn revision of the data matches that of the code.
#
# To link to an existing repository, set SVN_REVISION to "LOCAL" and
# set SVN_URL to the path to the repository.  Symlinks will be created
# that link into the existing repository.

set -e # Exit immediately if any simple command fails.
set -u # Treat unset variables as an error.

if [ -e "+TARGET+" ]; then
    echo "Warning: +TARGET+ exists; erasing it."
    rm -rf "+TARGET+"
fi
mkdir -p "+TARGET+"

SVN_BIN="/opt/subversion-1.8.13-x86_64/bin/svn"
SVN_URL="+SVN_URL+"
IDX_SVN_URL=`dirname "$SVN_URL"`/Private/Active

if [ "+SVN_REVISION+" != "local" ]; then
	for SUBDIR in +SUBDIRS+; do
		case $SUBDIR in
			data) $SVN_BIN export $SVN_URL/Data/SERIF/@+SVN_REVISION+ \
				+TARGET+/data ;;
			src) $SVN_BIN export $SVN_URL/Core/SERIF/@+SVN_REVISION+ \
				+TARGET+/src
				if [ "+SERIF_VERSION_STRING+" != "version" ]; then
					sed -i 's,SERIF_VERSION_STRING "[\.0-9]\+",SERIF_VERSION_STRING "+SERIF_VERSION_STRING+",' \
					+TARGET+/src/Generic/common/version.cpp
				fi
				;;
			external) $SVN_BIN export $SVN_URL/External/@+SVN_REVISION+ \
				+TARGET+/external ;;
			idx) $SVN_BIN export $IDX_SVN_URL/Core/SERIF/@+SVN_REVISION+ \
				+TARGET+/idx ;;
			icews) $SVN_BIN export $SVN_URL/Projects/W-ICEWS/lib/@+SVN_REVISION+ \
				+TARGET+/icews/lib
				$SVN_BIN export $SVN_URL/Projects/W-ICEWS/scripts/@+SVN_REVISION+ \
				+TARGET+/icews/scripts
				$SVN_BIN export $SVN_URL/Projects/W-ICEWS/annotation/tool/@+SVN_REVISION+ \
				+TARGET+/icews/annotation-tool
                                ;;
			awake) $SVN_BIN export $SVN_URL/Projects/AWAKE/@+SVN_REVISION+ \
				+TARGET+/awake ;;
			spectrum) $SVN_BIN export $SVN_URL/Projects/Spectrum/ActorDictionaryUI/@+SVN_REVISION+ \
				+TARGET+/spectrum/ActorDictionaryUI
 				$SVN_BIN export $SVN_URL/Projects/Spectrum/annotation/@+SVN_REVISION+ \
				+TARGET+/spectrum/annotation
				;;
			marathon) $SVN_BIN export $SVN_URL/Projects/Marathon/@+SVN_REVISION+ \
				+TARGET+/marathon ;;
			kba-streamcorpus) $SVN_BIN export $SVN_URL/Projects/kba-streamcorpus/@+SVN_REVISION+ \
				+TARGET+/kba-streamcorpus ;;
			accent) $SVN_BIN export $SVN_URL/Projects/ACCENT/@+SVN_REVISION+ \
				+TARGET+/accent ;;
			*) $SVN_BIN export $SVN_URL/Projects/SERIF/$SUBDIR@+SVN_REVISION+ \
				+TARGET+/$SUBDIR;;
		esac
	done
else
	for SUBDIR in +SUBDIRS+; do
		case $SUBDIR in
      data) ln -s $SVN_URL/Data/SERIF/ +TARGET+/data ;;
       idx) ln -s $IDX_SVN_URL/Core/SERIF/ +TARGET+/idx ;;
     icews) ln -s $SVN_URL/Projects/W-ICEWS/ +TARGET+/icews ;;
     awake) ln -s $SVN_URL/Projects/AWAKE/ +TARGET+/awake ;;
  spectrum) ln -s $SVN_URL/Projects/Spectrum/ +TARGET+/spectrum ;;
  marathon) ln -s $SVN_URL/Projects/Marathon/ +TARGET+/marathon ;;
    accent) ln -s $SVN_URL/Projects/ACCENT/ +TARGET+/accent ;;
       src) ln -s $SVN_URL/src/ +TARGET+/src ;;
  external) ln -s $SVN_URL/external/ +TARGET+/external ;;
         *) ln -s $SVN_URL/$SUBDIR +TARGET+/$SUBDIR;;
		esac
	done
fi

# make sure the target directory is group writeable in case a
# different user needs to erase it.
chmod -R 775 +TARGET+
