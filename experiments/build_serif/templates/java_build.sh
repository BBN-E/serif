#!/usr/bin/env bash
set -e # Exit immediately if any simple command fails.
set -u # Treat unset variables as an error.

src_dir="+src_dir+"
doc_dir="+doc_dir+"
dst_dir="+dst_dir+"

# Sanity check
if [ -e $dst_dir ]; then
    echo "Destination directory already exists!"
    exit -1
fi
    
mkdir -p `dirname $dst_dir`
cp -a $src_dir $dst_dir
cd $dst_dir
make "SERIFXML_DTD=$doc_dir/serifxml/serifxml.dtd" \
     "LANGDETECT_HOME=/d4m/serif/External/langdetect" \
     serif.jar serif_test.jar docs
