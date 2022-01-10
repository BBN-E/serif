#!/bin/sh
set -e
set -u

#####################################################################
# IDX Release Build Script
# Nicolas Ward
# nward@bbn.com
#
# Copyright Raython BBN Technologies 2014
# All Rights Reserved
#####################################################################

#####################################################################
# Release Configuration
#####################################################################
SERIF_REVISION=48154
VERSION=3.5.13
RELEASE_NAME=bbn_idx-$VERSION-r$SERIF_REVISION
EXPIRATION_DATE=2015-10-31
LAST_VALID_STAGE=doc-relations-events

#####################################################################
# Derived Release Paths
#####################################################################
PROJECT_DIR=/nfs/raid58/u30
RELEASES_DIR=$PROJECT_DIR/releases
DELIVERABLE_DIR=$RELEASES_DIR/$RELEASE_NAME
RELEASE_DIR=$RELEASES_DIR/$RELEASE_NAME-release
EXPT_DIR=$RELEASES_DIR/$RELEASE_NAME-experiment
BUILD_DIR=$EXPT_DIR/build_serif
PY_DIR=$BUILD_DIR/expts/Serif/rev-$SERIF_REVISION/python
SVNROOT=svn+ssh://svn.d4m.bbn.com/export/svn-repository/text/trunk

#####################################################################
# Build Serif
#####################################################################

# Check out the build_serif experiment & the python scripts we'll
# be using from svn, into a temp directory. Create the build
# experiment runjobs directories so we don't get symlinks.
echo "Checking out Serif build files from svn..."
echo "  $EXPT_DIR"
rm -rf $EXPT_DIR
mkdir -p $EXPT_DIR
svn co -q $SVNROOT/Active/Projects/SERIF/experiments/build_serif $BUILD_DIR
mkdir -p $BUILD_DIR/ckpts $BUILD_DIR/etemplates $BUILD_DIR/expts $BUILD_DIR/logfiles
svn co -q -r $SERIF_REVISION $SVNROOT/Private/Active/Projects/SERIF/java $EXPT_DIR/java
svn co -q -r $SERIF_REVISION $SVNROOT/Private/Active/Projects/SERIF/python $EXPT_DIR/private_python
svn co -q -r $SERIF_REVISION $SVNROOT/Private/Active/Projects/Cicero $EXPT_DIR/Cicero
svn co -q -r $SERIF_REVISION $SVNROOT/Active/External/langdetect $EXPT_DIR/langdetect

# Use the build experiment to build serif
echo "Building Serif binary..."
$BUILD_DIR/sequences/build_serif.pl \
    -sge \
    --build-only \
    --build IDX/NoThread/Release/x86_64+Win64/English/rev-$SERIF_REVISION \
    --checkout-data \
    --assemble-release \
    --serif-version $VERSION \
    </dev/null

#####################################################################
# Prepare Deliverable
#####################################################################

echo "Copying release..."
rm -rf $RELEASE_DIR
python $PY_DIR/copy_serif_release.py \
    $BUILD_DIR rev-$SERIF_REVISION $RELEASE_DIR

echo "Creating license..."
$RELEASE_DIR/expts/install/x86_64/NoThread/IDX/Release/SERIF_IMPORT/License/create_license -o $RELEASE_DIR/expts/Serif/data/unspec/misc -s `date +%F` -e $EXPIRATION_DATE -c Serif -u Cicero -r "last_valid_stage:$LAST_VALID_STAGE"
mv $RELEASE_DIR/expts/Serif/data/unspec/misc/Cicero*.lic $RELEASE_DIR/expts/Serif/data/unspec/misc/cicero.lic

echo "Building deliverable..."
rm -rf $DELIVERABLE_DIR
python $PY_DIR/build_serif_deliverable.py \
    --idx-relations \
    --xor-encrypt-data \
    --include-english \
    --exclude-scoring \
    --exclude-doc \
    --x64 --win64 \
    $RELEASE_DIR \
    $DELIVERABLE_DIR
rm $DELIVERABLE_DIR/MANIFEST
rm $DELIVERABLE_DIR/README.txt

#####################################################################
# Build Client
#####################################################################

# Build the IDX Java client
echo "Building Java client..."
pushd $EXPT_DIR/java > /dev/null
svn revert SerifManifest.in
cat SerifManifest.in | sed 's/serif\.Serif/serif\.SerifRelations/' > SerifManifest.tmp
mv SerifManifest.tmp SerifManifest.in
make \
    SERIFXML_DTD=$RELEASE_DIR/expts/Serif/doc/serifxml/serifxml.dtd \
    LANGDETECT_HOME=$EXPT_DIR/langdetect \
    clean \
    build \
    docs
patch < $EXPT_DIR/Cicero/deliverables/serifxml.patch
popd > /dev/null

# Copy the client into the deliverable
echo "Copying Java client..."
JAVA_DIR=$DELIVERABLE_DIR/java
mkdir -p $JAVA_DIR
cp $EXPT_DIR/java/idx.jar $JAVA_DIR
mkdir -p $DELIVERABLE_DIR/doc/serifxml
cp $EXPT_DIR/java/serifxml.xsd $DELIVERABLE_DIR/doc/serifxml/
rsync -a --exclude $EXPT_DIR/java/doc/api/private $EXPT_DIR/java/doc/api/ $DELIVERABLE_DIR/doc/javadoc

#####################################################################
# Additional Files
#####################################################################

# Copy in other files manually
cp $EXPT_DIR/Cicero/deliverables/README.txt $DELIVERABLE_DIR
cp -r $EXPT_DIR/Cicero/testing $DELIVERABLE_DIR/test
cp $EXPT_DIR/private_python/*.py $DELIVERABLE_DIR/python

# Regenerate the manifest
echo "Generating manifest..."
pushd $DELIVERABLE_DIR > /dev/null
find . -type f -exec md5sum "{}" \; > MANIFEST.txt
popd > /dev/null

#####################################################################
# Build Packages
#####################################################################

# Zip install dirs
ZIP_FILE=$RELEASE_NAME.zip
echo "Zipping packages..."
echo "  $ZIP_FILE"
pushd $RELEASES_DIR > /dev/null
rm -rf $ZIP_FILE
zip -qr $ZIP_FILE $RELEASE_NAME
md5sum $ZIP_FILE > $ZIP_FILE.md5
md5sum -c $ZIP_FILE.md5
popd > /dev/null
