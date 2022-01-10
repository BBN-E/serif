#!/bin/bash
#
# Draw a bargraph (using bargraph.pl) from the given source file
# (+src+), and write the resulting image to the given output 
# file (+dst+).
#
# Use the PLOT_PATH substitution to allow switching between /d4m/serif/bin
#  for older runs and globally installed version for modern SL6

set -e # Exit immediately if any simple command fails.

cat +src+ \
    |perl +bindir+/bargraph.pl \
       -gnuplot-path +PLOT_HOME+/gnuplot \
       -convert-path +PLOT_HOME+/convert \
       -fig2dev-path +PLOT_HOME+/fig2dev -fig - \
    |+PLOT_HOME+/fig2dev -L png -m 6 \
    |+PLOT_HOME+/convert - -resize 20% +dst+
