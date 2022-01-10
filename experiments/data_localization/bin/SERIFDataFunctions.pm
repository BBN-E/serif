package SERIFDataFunctions;

use strict;
use POSIX;
use Time::Local;

my $DATE_FORMAT = "%Y.%m.%d";
my $DATA_DIRECTORY = "SERIFData";

# main routines
sub get_latest {
	my $history_file = shift;
	my $date = '';
	if ($#_ > 0) {
		$date = shift;
	} else {
		$date = POSIX::strftime($DATE_FORMAT, localtime);
	}
	
	return get_by_date($date, $history_file);
}
sub get_by_date {
	my $date = shift;
	my $history_file = shift;
	return "\$DATA_DRIVE\$/$DATA_DIRECTORY/" . nearest_date($date, $history_file);
}
sub get_by_revision_num {
	my $rev = shift;
	# if rev contains an "r", remove it
	if ($rev =~ /r([0-9]+)/) {
		$rev = $1;
	}
	my $history_file = shift;
	return "\$DATA_DRIVE\$/$DATA_DIRECTORY/" . nearest_revision($rev, $history_file);
}

sub get_dirs {
	my $history_file = shift;
	open(FILE, $history_file);
	my @items = ();
	# verify items
	foreach my $item (<FILE>) {
		# e.g., 02.26.2010-r300
		if ($item =~ /[0-9]{4}\.[0-9]{2}\.[0-9]{2}\-r[0-9]+/) {
			push(@items, $item);
		}
	}
	close(FILE);
	
	return @items;
}
# find a data dir on the current date, 
# OR the most recent one before it
sub nearest_date {
	my $date = shift;
	my $history_file = shift;
	my @dirs = get_dirs($history_file);
	my $dir = '';
	while (scalar(@dirs) > 0) {
		$dir = find_dir($date, \@dirs);
		# if a date match can't be found
		# go back one day and try again
		if ($dir eq '') {
			$date = reduce_one_day($date);
		} else {
			$dir =~ s/\n//;
			return $dir;
		}
	}
	return '';
}
sub reduce_one_day {
	my $date = shift;
	my ($year, $month, $day) = split(/\./, $date);
	my $day_time = timelocal(0, 0, 0, $day, $month-1, $year);
	$day_time = $day_time - (60*60*24);
	$date = POSIX::strftime($DATE_FORMAT, localtime($day_time));
	return $date;
}
sub nearest_revision {
	my $rev = shift;
	my $history_file = shift;
	
	my @dirs = get_dirs($history_file);
	my $dir = '';
	while (1) {
		$dir = find_dir("r$rev", \@dirs);
		# if a rev match can't be found
		# go back one and try again
		if ($dir eq '') {
			$rev--;
		} else {
			return $dir;
		}
	}
	return '';
}

sub find_dir {
	my $str = shift;
	my $dirs_ref = shift;
	my @dirs = @$dirs_ref;
	
	foreach my $dir (@dirs) {
		#my ($datestr, $revstr) = split(/-/, $dir);
		#if ($datestr eq $str || $revstr eq $str) {
		if ($dir =~ /.*$str.*/) {
			return $dir;
		}
	}
	
	return '';
}

1;