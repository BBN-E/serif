#!/usr/bin/env bash
set -e # Exit immediately if any simple command fails.

if [ -e "+dir+" ]; then
    echo "Warning: '+dir+' is not empty; deleting it."
    rm -rf "+dir+"
fi
mkdir -p "+dir+"
