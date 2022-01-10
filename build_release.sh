#!/bin/sh

src_dir=$1
output_dir=$2
name=$3

echo "Building release for tag $name..."

release_dir="$output_dir/$name"

if [ -d $release_dir ]; then
    echo "Release directory $release_dir already exists!  Please use a different name."
    exit 1
fi

mkdir -p $release_dir
cp -r $src_dir/* $release_dir

# make files group-writeable by "d4m" group
chgrp -R d4m $release_dir
chmod -R g+w $release_dir

echo "Created SERIF release under $release_dir"
