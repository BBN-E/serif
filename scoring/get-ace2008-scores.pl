#!/usr/bin/perl
#
# Copyright BBN Technologies Corp. 2007
# All rights reserved


my ($olddir, $newdir, $str, $oldxx, $newxx) = @ARGV;
die "usage $0: OLDDIR NEWDIR [condition] [oldyear] [newyear]\n" if (@ARGV < 2);

if (@ARGV <= 3) {
  $oldxx = "08";
}
if (@ARGV <= 4) {
  $newxx = "08";
}

opendir(DIR, $olddir) || die "Cannot opendir $olddir";
#my @names = glob("$olddir/adj-?");
my @names = grep { /\w/ } readdir(DIR);
#my @names = grep { /adj.+/ } readdir(DIR);
#print STDERR "names=@names\n"; exit(0);

foreach $name (sort @names) {
    if ($name =~ /debug/) {
	next;
    }
    if ($str ne "" && !($name =~ /$str/)) {
	next;
    }
    open($INOLD{$name}, "$olddir/$name/ace20$oldxx-eval.txt");
    open($INNEW{$name}, "$newdir/$name/ace20$newxx-eval.txt");
}

@task = (entity, "B3-Unweighted", "B3-Value", relation, event, value, timex2);
@taskname = (ENTITIES, "B3-Unweighted", "B3-Value", RELATIONS, EVENTS, VALUES, TIME);

foreach $task (@task) {
    $count{$task} = 0;
    $old_sum{$task} = 0;
    $new_sum{$task} = 0;
}

foreach $name (sort @names) {
    if ($name =~ /debug/) {
	next;
    }
    if ($str ne "" && !($name =~ /$str/)) {
	next;
    }
    #print "$name: \t\t";

    $ready = 0;
    $old = 0;
    $new = 0;
    $in = $INOLD{$name};
    while (<$in>) {
	if (/(\S+) scoring =/) {
	    $ready = 1;
            $task = $1;
            #if ($task eq "entity") {
        	#if (/B-cubed score = ([\-0-9.]+)/) {
		    #$old{"B-cubed"}{$name} = $1 * 100;
		    #$old_sum{"B-cubed"} += $old{"B-cubed"}{$name};
		    #$count{"B-cubed"}++;
        	#}
            #}
	} 
	if ($ready && /total/) {
	    #if(/^  total\s+\d+\s+\d+\s+\d+\s+\d+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+([\-0-9.]+)\s+/) {
	    if(/^  total\s+\d+\s+\d+\s+\d+\s+\d+\s+\d+\s+\d+\s+\d+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+([\-0-9.]+)\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+([\-0-9.]+)\s+[\-0-9.]+\s+[\-0-9.]+\s+([\-0-9.]+)\s+/) {
		#print "$1";
		$old{$task}{$name} = $2;
		$old_sum{$task} += $old{$task}{$name};
		$count{$task}++;
		if ($task eq "entity") {
		    $old{"B3-Unweighted"}{$name} = $1;
		    $old_sum{"B3-Unweighted"} += $old{"B3-Unweighted"}{$name};
		    $count{"B3-Unweighted"}++;

		    $old{"B3-Value"}{$name} = $3;
		    $old_sum{"B3-Value"} += $old{"B3-Value"}{$name};
		    $count{"B3-Value"}++;
        	}
	    }
	    $ready = 0;
	    #last;
	}
    }
    if (@ARGV == 1) {
	#print "\n";
	next;
    }
    #print "\t";
    $ready = 0;
    $in = $INNEW{$name};
    while (<$in>) {
	if (/(\S+) scoring =/) {
	    $ready = 1;
            $task = $1;
            if ($task eq "entity") {
        	if (/B-cubed score = ([\-0-9.]+)/) {
		    $new{"B-cubed"}{$name} = $1;
		    $new_sum{"B-cubed"} += $new{"B-cubed"}{$name};
        	}
            }
	} 
	if ($ready && /total/) {
	    #if(/^  total\s+\d+\s+\d+\s+\d+\s+\d+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+([\-0-9.]+)\s+/) {
	    #if(/^  total\s+\d+\s+\d+\s+\d+\s+\d+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+([\-0-9.]+)\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+([\-0-9.]+)\s+[\-0-9.]+\s+[\-0-9.]+\s+([\-0-9.]+)\s+/) {
	    if(/^  total\s+\d+\s+\d+\s+\d+\s+\d+\s+\d+\s+\d+\s+\d+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+([\-0-9.]+)\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+[\-0-9.]+\s+([\-0-9.]+)\s+[\-0-9.]+\s+[\-0-9.]+\s+([\-0-9.]+)\s+/) {
		#print "$1";
		$new{$task}{$name} = $2;
		$new_sum{$task} += $new{$task}{$name};
		if ($task eq "entity") {
		    $new{"B3-Unweighted"}{$name} = $1;
		    $new_sum{"B3-Unweighted"} += $new{"B3-Unweighted"}{$name};

		    $new{"B3-Value"}{$name} = $3;
		    $new_sum{"B3-Value"} += $new{"B3-Value"}{$name};
        	}
	    }
	    $ready = 0;
	    #last;
	}
    }
    foreach $task (@task) {
	if ($new{$task}{$name} != 0) {
	    if ($new{$task}{$name} - $old{$task}{$name} > 0) {
		$diff{$task}{$name} = "+";
	    } elsif ($new{$task}{$name} - $old{$task}{$name} == 0) {
		$diff{$task}{$name} = "";
	    } elsif ($new{$task}{$name} - $old{$task}{$name} < 0) {
		$diff{$task}{$name} = "-";
	    }
	} 
    }
}

for ($t = 0; $t < scalar(@task); $t++) {
    $task = $task[$t];
    print $taskname[$t], ":\n";
    foreach $name (sort @names) {
	if ($name =~ /debug/) {
	    next;
	}
	if ($str ne "" && !($name =~ /$str/)) {
	    next;
	}
	printf "%-16s", $name;
	printf " %6.2f ", $old{$task}{$name};
	if (@ARGV != 1) {
	    printf " %6.2f%s%s", $new{$task}{$name}, "  ", $diff{$task}{$name};
	}
	printf "\n";
    }
    printf "\n%-16s", "ALL AVG:";
    if ($count{$task} != 0) {
	$old_average = $old_sum{$task} / $count{$task};
	$new_average = $new_sum{$task} / $count{$task};
	printf " %6.2f ", $old_average;
	if (@ARGV != 1) {
	    printf " %6.2f  ", $new_average;
	    if ($new_average - $old_average > 0) {
		print "+";
	    } elsif ($new_average - $old_average == 0) {
		print "";
	    } elsif ($new_average - $old_average < 0) {
		print "-";
	    } 
	}
    }
    print "\n\n";
}


