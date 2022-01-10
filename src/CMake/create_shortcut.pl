# Copyright 2008 by BBN Technologies Corp.
# All Rights Reserved.

# TB: this script creates a Windows shortcut
# It is created to be used with the CMake utility
use Win32::Shortcut;

use strict;
use warnings;

die "Usage: $0 file working_dir link_location" unless @ARGV==3;

my($file, $working_dir, $link_location) = @ARGV;
my $link = Win32::Shortcut->new();
$link->{'Path'} = "$file";
$link->{'WorkingDirectory'} = "$working_dir";
$link->{'ShowCmd'} = SW_SHOWNORMAL;
$link->Save("$link_location");
$link->Close();
