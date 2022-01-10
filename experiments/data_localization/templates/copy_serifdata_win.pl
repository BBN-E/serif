#!/opt/perl-5.8.6/bin/perl

eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}' && eval 'exec perl -w -S $0 $argv:q'
if 0;

# Copy a set of files or directories to a set of machines, using robocopy
#
use strict;
use POSIX "sys_wait_h";
use lib "+SERIFDataFunctions_location+";
use SERIFDataFunctions;
use File::Path qw(mkpath rmtree);
use Cwd;

my $history_file = "+history_file+";
my $shared_dir = "+share_name+";
my $date_and_rev_name = "+daterev_folder+";
my $serif_data = "+serif_data+";
my $machines_file = "+machines_file+";

my $folder_name;
my $root_path;
if ($serif_data =~ /(.*)(\/|\\)([^\/\\]+)$/) {
	$root_path = $1;
	$folder_name = $3;
	print "Using source root path: $root_path\nUsing folder name: $folder_name\n";
} else {
	die ("Bad filename (remember to use the full path) $serif_data");
}

# read in the list of machines
my @machines = ();
my %number_of_copies = ();
open(MACH, $machines_file) || die "Cannot open file $machines_file for input\n";
while (<MACH>) {
	chomp;
	if (!($_ =~ /^\#/)) { # ignore comments
		my ($machine, $copies) = split(/\s+/, $_);
		push(@machines, $machine);
		$number_of_copies{$machine} = $copies;
	}
}
close MACH;    

my $copy = "robocopy";
my $program_files_x86 = "c:\\Program Files\ (x86)";
my $program_files = "c:\\Program Files";
if (-d $program_files_x86) {
	$copy = "\"${program_files_x86}\\Windows Resource Kits\\Tools\\robocopy.exe\"";
} else {
	$copy = "\"${program_files}\\Windows Resource Kits\\Tools\\robocopy.exe\"";
}
# /E: Everything -- including subdirectories
# /PURGE: remove files that no longer exist
# /SEC: copy files with SECurity -- including permissions
# /MIR: MIRror the directory trees == /E & /PURGE
# /XD: eXclude Directory -- by name pattern
# my $options = "/SEC /MIR /XD .svn";
my $options = "/MIR /XD .svn /R:1";

# copy files to first machine from server location
my $numMachines = @machines;
print "About to copy file(s) to $numMachines machines.\n";
my @done_machines = ();
my $m = shift(@machines);
print "Sending to machine: $m\n";
my $src_file = $folder_name;
my $src_path = $root_path;

print "Sending $src_path\\$src_file\n";

copy_new_version_directory($copy, $src_file, $date_and_rev_name, $src_path, 
	"\\\\$m\\$shared_dir", $options, $history_file);

my $status = `echo %ERRORLEVEL%`;
# $status =~ s/\n//;
chomp $status;
if ($status) {
	die "Failed to copy $src_file from $src_path to $shared_dir (status $status)\n";
}
# change the permissions
# $cmd = "$call $m chmod -R guoa+w $shared_file";
# print "$cmd\n";
# system $cmd;

push(@done_machines, $m);
print("Copied first set of files to machine $m\n");

# copy files to the rest of the machines 
# from "already done" machines (iteratively)
my $num_mach_to_copy = scalar(@machines);
my $num_machines_done = 0;
my %pid2dest_machine = ();
my %pid2src_machine = ();
while ($num_machines_done < $num_mach_to_copy) {
	if (@done_machines && @machines) {
		my $dest_mach = shift(@machines);
		my $src_mach = shift(@done_machines);
		
		my $pid = fork;
		if ($pid == 0) {
			# child
			copy_version_directory($copy, $date_and_rev_name,
				"\\\\$src_mach\\$shared_dir", 
				"\\\\$dest_mach\\$shared_dir", $options, $history_file);
			my $status |= `echo %ERRORLEVEL%`;
			$status =~ s/\n//;
			exit $status;
		}
		else {
			# parent
			$pid2dest_machine{$pid} = "$dest_mach";
			$pid2src_machine{$pid} = "$src_mach";
		}
	}

	if (my $child = waitpid(-1, WNOHANG)) {
		if ($child == -1) {
			die "Error in waitpid\n";
		}
		my $status = $?;
		if ($status) {
			warn ">>> Copying on machine $pid2dest_machine{$child} failed!\n";
		}
		else {
			print "Finished copying to machine $pid2dest_machine{$child}\n";
		}
		push(@done_machines, $pid2dest_machine{$child}, $pid2src_machine{$child});
		$num_machines_done++;
	}
}

sub copy {
	my $copy = shift;
	my $file = shift; 
	my $src_location = shift;
	my $dest_location = shift;
	my $options = shift;
	
	print "Copying file: $file\n";
	
	my $cmd = "$copy $src_location\\$file $dest_location $options";
	print "Running command: $cmd\n";
	return system $cmd;
}

# copy from one queue computer to another
# date_and_rev_name: mm.dd.yyyy-rxxxx
sub copy_version_directory {
	my $copyexe = shift; # "robocopy" or "copy"
	my $date_and_rev_name = shift; # e.g., "03.25.2010-r3"
	my $src_location = shift; # e.g., "\\traid01\speed\serif-data"
	my $dest_location = shift; # e.g., "\\machine\serif-data"
	my $options = shift;
	my $history_file = shift;
	
	create_next_version_dir($copyexe, $dest_location, $date_and_rev_name, $history_file, $options);
	copy($copyexe, $date_and_rev_name, $src_location, "$dest_location\\$date_and_rev_name", $options);
}

# copy from default location to a queue computer
# file: data
# date_and_rev_name: mm.dd.yyyy-rxxxx
sub copy_new_version_directory {
	my $copyexe = shift; # "robocopy" or "copy"
	my $src_file = shift; # e.g., "data"
	my $date_and_rev_name = shift; # e.g., "03.25.2010-r3"
	my $src_location = shift; # e.g., "\\traid01\speed\serif-data"
	my $dest_location = shift; # e.g., "\\machine\serif-data"
	my $options = shift;
	my $history_file = shift;
	
	create_next_version_dir($copyexe, $dest_location, $date_and_rev_name, $history_file);
	
	# synchronize the source with the new destination
	copy($copyexe, $src_file, $src_location, "$dest_location\\$date_and_rev_name", $options);
}

sub create_next_version_dir {
	my $copyexe = shift;
	my $location = shift;
	my $date_and_rev_name = shift;
	my $history_file = shift;
	my $options = shift;
	
	my @path_parts = grep(/\S/, split(/[\\|\/]/, $location));
	my $machine = $path_parts[0];
	
	# if it doesn't exist already
	# then create the new directory
	if (!(-d "$location\\$date_and_rev_name")) {
		print "Creating a SERIF data version directory: $date_and_rev_name\n";
		
		# copy previous version of the data to new day directory
		# OR just create new directory (if it's the first)
		my ($date, $rev) = split(/-/, $date_and_rev_name);
		$date = SERIFDataFunctions::reduce_one_day($date);
		my $previous_dir_name = SERIFDataFunctions::nearest_date($date, $history_file);
		if ($previous_dir_name ne '') {
			my $cmd = "$copyexe $location\\$previous_dir_name $location\\$date_and_rev_name $options";
			print "Running command: $cmd\n";
			system $cmd;
		} else {
			print "Creating directory: $location\\$date_and_rev_name\n";
			mkpath "$location\\$date_and_rev_name" || die("Failure! Could not create $location\\$date_and_rev_name: $!\n");
		}
	}
	
	# remove old directories
	my @dirs = get_dirs($location);
	if (scalar(@dirs) > $number_of_copies{$machine}) {
		remove_old_dirs($location, $number_of_copies{$machine}, \@dirs);
	}
}

sub get_dirs {
	my $location = shift;
	opendir(DIR, $location) || die("Unable to open SERIFData share: $location\n");
	my @dirs = grep(/[0-9]{4}\.[0-9]{2}\.[0-9]{2}\-r[0-9]+/, readdir(DIR));
	closedir(DIR);
	return sort(@dirs);
}

sub remove_old_dirs {
	my $loc = shift;
	my $max_copies = shift;
	my $dirs_ref = shift;
	my @dirs = @$dirs_ref;
	
	print "Removing old SERIF data versions...\n";
	my $current_copies = scalar(@dirs);
	if ($current_copies <= $max_copies) {
		die "Invalid input: number of copies ($current_copies) <= max copies ($max_copies) in $loc\n";
	}
	while (scalar(@dirs) > $max_copies) {
		my $dir = shift @dirs;
		print "Removing copy: $dir\n";
		system "net use x: $loc /persistent:no /yes";
		my $cwd = cwd();
		chdir "x:/" or die("Cannot chdir into $loc ('x:/'): $!\n");
		rmtree "x:\\$dir" or die("Failed to delete $loc\\$dir: $!\n");
		chdir $cwd or die("Cannot chdir into $cwd: $!\n");
		system "net use x: /delete /yes";
	}
}
