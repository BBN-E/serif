eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}' && eval 'exec perl -w -S $0 $argv:q'
  if 0;

# Copyright 2008 by BBN Technologies Corp.     All Rights Reserved.

use File::Copy;
use File::Basename;
use File::Path;

die "Usage: $0 sgm-dir sgm-suffix apf-dir apf-suffix output-dir num-files-per-batch ca-sexp-script [dtd]" 
    unless @ARGV == 7 || @ARGV == 8;

$sgm_dir = shift;
$sgm_suffix = shift;
$apf_dir = shift;
$apf_suffix = shift;
$output_dir = shift;
$num_files = shift;
$script = shift;

if (@ARGV == 1) {
    $dtd = shift;
}

die unless $num_files > 0;

@sgm_files = glob("$sgm_dir/*$sgm_suffix");

$file_count = 0;
$dir_count = 0;

mkpath($output_dir);

foreach $sgm_file (@sgm_files) 
{
    $sgm_file =~ /^(.*)$sgm_suffix$/;
    $base = basename($1);
    $apf_file = "$apf_dir/$base$apf_suffix";
    die "Could not find $apf_file" unless -f $apf_file;
    
    if ($file_count % $num_files == 0) {
	unless ($file_count == 0) {
	    close SRC;
            close KEY;
            if ($dtd ne "") {
                copy($dtd, "$current_dir/apf/" . basename($dtd));
            }
	    system("perl $script $current_dir/apf $apf_suffix $current_dir/sgm $sgm_suffix $current_dir/ca.sexp");
	}

	$current_dir = $dir_count;
	$current_dir = "000" . $current_dir if length($current_dir) == 1;
	$current_dir = "00" . $current_dir if length($current_dir) == 2;
	$current_dir = "0" . $current_dir if length($current_dir) == 3;
	$current_dir = $output_dir . "/" . $current_dir;
	mkdir $current_dir; 
	mkdir "$current_dir/sgm"; 
	mkdir "$current_dir/apf"; 

	die "Could not make directories under $current_dir"
	    unless -d $current_dir &&
	           -d "$current_dir/sgm" &&
		   -d "$current_dir/apf"; 

	$dir_count++;
	open SRC, ">$current_dir/source_files.txt";
        open KEY, ">$current_dir/key_files.txt";
    }
    
    copy($sgm_file, "$current_dir/sgm/" . basename($sgm_file));
    copy($apf_file, "$current_dir/apf/" . basename($apf_file));
    
    print SRC "$current_dir/sgm/" . basename($sgm_file) . "\n";
    print KEY "$current_dir/apf/" . basename($apf_file) . "\n";

    $file_count++;
}

close SRC;
close KEY;
if ($dtd ne "") {
    copy($dtd, "$current_dir/apf/" . basename($dtd));
}

my $status = `perl $script $current_dir/apf $apf_suffix $current_dir/sgm $sgm_suffix $current_dir/ca.sexp`;
exit $status;
