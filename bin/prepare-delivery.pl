use strict;
use File::Copy;
die "usage: $0 source_dir target_dir\n" unless @ARGV == 2;
my ($sdir, $tdir) = @ARGV;

## This script will create a replica of a SERIF directory,
##   but with copyrights updated and commercial code removed
##   (unless otherwise specified)
##
## It also removes .vssscc and .dsp files, plus
##   a few other strays.

my $allow_commercial_code = 0;

sub process_dir {
    my $path = shift;
    my $dir = shift;
    my $full_input_dir = "$sdir/$path$dir";
    my $full_output_dir = "$tdir/$path$dir";

    mkdir($full_output_dir);
    opendir(DIR, $full_input_dir) || die "Cannot opendir $full_input_dir";
    my @stuff = grep { /\w/ } readdir(DIR);

    foreach my $thing (@stuff) {
	if (-d "$full_input_dir/$thing") {
	    if ($thing =~ /\.svn/ || $thing =~ /Release$/ || $thing =~ /Debug$/ || $thing =~ /\.dir$/ || $thing =~ /CMakeFiles/) {
		next;
	    }
	    my $new_path = $path;
	    if ($dir ne "") {
		$new_path .= $dir . "/";
	    }
	    process_dir($new_path, $thing);
	} elsif (-f "$full_input_dir/$thing") {
	    if ($thing =~ /\.vssscc/ || $thing =~ /\.vspscc/  || $thing =~ /\.vsscc/ || $thing =~ /\.dsp$/ || $thing eq "Changelog.xls" || $thing eq "fix_vcproj_guids.py") {
		next;
	    } elsif ($thing =~ /\.vcproj$/) {
		copy("$full_input_dir/$thing", "$full_output_dir/$thing");
		next;
	    }
	    open(IN, "$full_input_dir/$thing");
	    open(OUT, ">$full_output_dir/$thing");
	    my $printable = 1;
	    my $has_copyright = 0;
	    my $has_rights = 0;
	    my $commercial_if = 0;
	    my $if_count = 0;
	    while (<IN>) {
		if (/^\s*\#if/) {
		    $if_count++;
		}

		if (/COMMERCIAL_AND_DEVELOPED_EXCLUSIVELY_UNDER_PRIVATE_FUNDING/ && !$allow_commercial_code) {
		    $commercial_if = $if_count;
		}
		
		if ($commercial_if == 0) {
		    if (/\# Copyright.*BBN/) {
			    print OUT 
"############################################################################
# Copyright 2011 by Raytheon BBN Technologies Corp.                        #
#                                                                          #
# Use, duplication, or disclosure by the Government is subject to          #
# restrictions as set forth in the Rights in Noncommercial Computer        #
# Software and Noncommercial Computer Software Documentation clause at     #
# DFARS 252.227-7014.                                                      #
#                                                                          #
#      Raytheon BBN Technologies Corp.                                     #
#      10 Moulton St.                                                      #
#      Cambridge, MA 02138                                                 #
#      617-873-3411                                                        #
#                                                                          #
############################################################################

";
			$_ = "";
			$has_copyright = 1;
		    } elsif (/Copyright.*BBN/ && !/\#define /) {
			    print OUT 
"/****************************************************************************/
/* Copyright 2011 by Raytheon BBN Technologies Corp.                        */
/*                                                                          */
/* Use, duplication, or disclosure by the Government is subject to          */
/* restrictions as set forth in the Rights in Noncommercial Computer        */
/* Software and Noncommercial Computer Software Documentation clause at     */
/* DFARS 252.227-7014.                                                      */
/*                                                                          */
/*      Raytheon BBN Technologies Corp.                                     */
/*      10 Moulton St.                                                      */
/*      Cambridge, MA 02138                                                 */
/*      617-873-3411                                                        */
/*                                                                          */
/****************************************************************************/

";
			$_ = "";
			$has_copyright = 1;
		    } elsif (/Copyright/ && $_ !~ /displayVersionAndCopyright/  &&
			     $thing !~ /.*\.cmake$/ && 
			     $thing !~ /rijndael/ &&
			     $thing !~ /.*\.c$/ && $thing !~ /.*\.h$/) 
		    {
			print "Warning: copyright mentioned in unexpected setting ($path$dir/$thing): $_\n";
		    }
		    print OUT;
		}
		if (/all rights reserved/i) {
		    $has_rights = 1;
		}
		if (/\#endif/) {
		    if ($commercial_if == $if_count) {
			$commercial_if = 0;
		    }
		    $if_count--;
		}
	    }
	    if ($thing !~ /\.cmake$/ && $thing !~ /.*\.c$/ && $thing !~ /.*\.h$/) {
		if ($has_copyright && !$has_rights) {
		    print "Copyright but no rights reserved: $path$dir/$thing\n";
		}
		if (!$has_copyright) {
		    print "No copyright: $path$dir/$thing\n";
		}
	    }
	    close IN;
	    close OUT;
	}
    }
}

process_dir("","");
close DEBUG;
