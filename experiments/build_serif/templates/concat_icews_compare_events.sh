#!/bin/env bash

set -e # Exit immediately if any simple command fails.
set -u # Treat unset variables as an error.
set -x # Print commands before executing

cat +test_dir+/*/compare_events/* > +out_file+



