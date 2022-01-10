eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}' && eval 'exec perl -w -S $0 $argv:q'
  if 0;

# Copyright 2008 by BBN Technologies Corp.     All Rights Reserved.

use utf8;

die "Usage: $0 run-directory intermediate-dir output-file prefix output-prefix mixed-case-flag training-unit type(names|values|other-values|times) num-batches" unless @ARGV == 9;

$dir = shift;
$intdir = shift;
$output = shift;
$prefix = shift;
$output_prefix = shift;
$mixed = shift;
$unit = shift;
$type = shift;
$num_batches = shift;

if ($unit ne "token" && $unit ne "char") {
    die "Bad unit type.  Unit must be set to token or char";
}

if ($type ne "names" && $type ne "values" && $type ne "other-values" && $type ne "times") {
    die "Bad type. Type must be set to names, values, other-values, or times";
}

@files = ();
for ($current_dir = 0; $current_dir < $num_batches; $current_dir++) {
    $current_dir = "000" . $current_dir if length($current_dir) == 1;
    $current_dir = "00" . $current_dir if length($current_dir) == 2;
    $current_dir = "0" . $current_dir if length($current_dir) == 3;
    
    push(@files, glob("$dir/batches/$current_dir/dumps/$prefix*"));
}

# training files contain the training sexps
# output file will point to training files
$training1 = "$intdir/$output_prefix";
$training2 = "$intdir/$output_prefix";

if ($mixed) {
    $training1 .= ".mc";
    $training2 .= ".mc";
} else {
    $training1 .= ".lc";
    $training2 .= ".lc";
}

$training1 .= ".1";
$training2 .= ".2";

open OUT1, ">:utf8", "$training1" or die;
open OUT2, ">:utf8", "$training2" or die;

open OUT, ">$output" or die;
print OUT "2\n$training1\n$training2\n";
close OUT;

print "Found " . scalar(@files) . " files\n";

$first_file = 1;
foreach $file (@files) {
    if ($first_file) {
	$stream = OUT1;
    } else {
	$stream = OUT2;
    }

    next if $mixed && !containsMixedCaseTraining($file);

    open IN, "<:utf8", $file or die;
    while (<IN>) {
        $_ = modifyTraining($_, $mixed, $type);
	
	print $stream $_;
    }
    close IN;
    $first_file = !$first_file;
}

close OUT1;
close OUT2;

sub containsMixedCaseTraining
{
    my $file = shift @_;
    
    open FI, $file;
    while (<FI>) {
	while (s/^.*?\(\s*([^\(]*?)\s+//) {
	    $word = $1;
	    #print "Checking $word for case\n";
	    return 1 if $word ne lc($word);
	}
	
    }
    close FI;
    return 0;
}

sub modifyTraining
{
    my $line = shift @_;
    my $mixed_flag = shift @_;
    my $type = shift @_;

    $result = "";
    
    # remove outer parens and whitespace
    $line =~ s/^\s*\((.*)\)\s*$/$1/;
    while ($line =~ s/^\s*\(\s*(\S+)\s+(\S+)-(\S\S)\s*\)//) {
	$word = $1;
	$tag = $2;
	$st_or_co = $3;
	
	$word = lc($word) unless $mixed_flag;

	if ($type eq "values") {
            $tag = "TIMEX" if ($tag eq "TIMEX2" || $tag eq "TIME");
        }
        
        if ($type eq "times") {
	    $tag = "TIME" if ($tag eq "TIMEX2" || $tag eq "TIMEX");
	    $tag = "NONE" unless $tag eq "TIME";
	}
	
	if ($type eq "other-values") {
	    $tag = "NONE" if ($tag eq "TIMEX2" || $tag eq "TIMEX");
	}

	$result .= "($word $tag-$st_or_co) ";
    }
    if ($unit eq "char") {
        $result = splitTokensIntoChars($result);
    }
    $result = "(" . $result . ")\n";
    return $result;
}

sub splitTokensIntoChars
{
    my $line = shift @_;

    $result = "";

    while ($line =~ s/^\s*\(\s*(\S+)\s+(\S+)-(\S\S)\s*\)//) { 
        $word = $1;
        $tag = $2;
        $st_or_co = $3;

        @chars = split(/(\w|-LRB-|-RRB-|-LCB-|-RCB-|--)/, $word);
        my $i = 0;
        while ($chars[$i] =~ /^\s*$/) { $i++; }
        $result .= "($chars[$i] $tag-$st_or_co) ";
        for ($i = $i + 1; $i < @chars; $i++) {
            if ($chars[$i] !~ /^\s*$/) {
                $result .= "($chars[$i] $tag-CO) ";
            }
        }
    }

    return $result;
}
