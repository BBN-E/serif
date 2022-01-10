#!/bin/tcsh -vx

# this is a daily cron job to check out the data
# from SVN on every Windows queue server used for SERIF

set exp_root = /nfs/raid58/u14/serif/localization_exp_rmg

set NETWORK_SHARE = SERIFData

set MACHINES_FILE = $exp_root/bin/win64.list

set HISTORY_FILE = $exp_root/bin/history_file
set NUM_HISTORY_LINES = 10

set SERIF_DATA = /nfs/raid58/u14/serif/data

# clean up last experiment
rm -rf $exp_root/bin/log.txt
rm -rf $exp_root/etemplates/*
rm -rf $exp_root/expts/*
rm -rf $exp_root/ckpts/*
rm -rf $exp_root/logfiles/*

cd $SERIF_DATA

# check for changes in the past day
set svn_status = `svn status`

# only synchronize if there have been changes
if ( ${%svn_status} != 0 ) then

	# update data working copy
	svn update
	
	# get name of recent svn revision and construct data folder name
	set svn_rev = `svn info | grep "Revision:" | grep -E -o "[0-9]+"`
	set date_str = `/bin/date +"%Y.%m.%d"`
	set folder_name = $date_str-r$svn_rev

	# queue perl script
	perl $exp_root/sequences/run.pl -sge -die_on_error $SERIF_DATA $MACHINES_FILE $HISTORY_FILE $NETWORK_SHARE $folder_name
	
	# add folder to the history file
	$exp_root/bin/append_history_copy.csh $folder_name $HISTORY_FILE $NUM_HISTORY_LINES
endif

