# Copyright BBN Technologies Corp. 2007
# All rights reserved

my ($olddir, $newdir, $str) = @ARGV;
die "usage $0: OLDDIR NEWDIR condition\n" if (@ARGV != 2 && @ARGV != 1 && @ARGV != 3);

opendir(DIR, $olddir) || die "Cannot opendir $olddir";
my @names = grep { /\w/ } readdir(DIR);

print "VALUES:\n";
$count = 0;
$old_sum = 0;
$new_sum = 0;
foreach $name (sort @names) {
    if ($name =~ /debug/) {
	next;
    }
    if ($str ne "" && !($name =~ /$str/)) {
	next;
    }
    open(IN, "$olddir/$name/ace2007-eval.txt");
    print "$name: \t";

    $ready = 0;
    $old = 0;
    $new = 0;
    while (<IN>) {
	if (/value scoring/) {
	    $ready = 1;
	} 
	if ($ready && /total/) {
	    if(/^  total\s+\d+\s+\d+\s+\d+\s+\d+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+([\-0-9.]+)\s+/) {
		print "$1";
		$old = $1;
		$old_sum += $old;
		$count++;
	    }
	    last;
	}
    }
    close(IN);
    if (@ARGV == 1) {
	print "\n";
	next;
    }
    open(IN, "$newdir/$name/ace2007-eval.txt");
    print "\t";
    $ready = 0;
    while (<IN>) {
	if (/value scoring/) {
	    $ready = 1;
	} 
	if ($ready && /total/) {
	    if(/^  total\s+\d+\s+\d+\s+\d+\s+\d+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+([\-0-9.]+)\s+/) {
		print "$1";
		$new = $1;
		$new_sum += $new;
	    }
	    last;
	}
    }
    if ($new != 0) {
	if ($new - $old > 0) {
	    print "\t+";
	} elsif ($new - $old == 0) {
	    print "\t";
	} elsif ($new - $old < 0) {
	    print "\t-";
	} 
    }
    print "\n";
    close(IN);
}

print "\nALL AVG:\t";
if ($count != 0) {
    $old_average = $old_sum / $count;
    $new_average = $new_sum / $count;
    printf "%5.2f\t%5.2f", $old_average, $new_average;
    if ($new_average - $old_average > 0) {
	print "\t+";
    } elsif ($new_average - $old_average == 0) {
	print "\t";
    } elsif ($new_average - $old_average < 0) {
	print "\t-";
    } 
}
