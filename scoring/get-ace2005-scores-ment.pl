# Copyright BBN Technologies Corp. 2007
# All rights reserved

my ($olddir, $newdir, $str) = @ARGV;
die "usage $0: OLDDIR NEWDIR condition\n" if (@ARGV != 2 && @ARGV != 1 && @ARGV != 3);

opendir(DIR, $olddir) || die "Cannot opendir $olddir";
my @names = grep { /-/ } readdir(DIR);

print "ENTITY MENTIONS:\n";
$count = 0;
$old_sum = 0;
$new_sum = 0;
foreach $name (sort @names) {
    if ($str ne "" && !($name =~ /$str/)) {
	next;
    }
    open(IN, "$olddir/$name/ace2005-eval.txt");
    print "$name: \t";

    $ready = 0;
    $old = 0;
    $new = 0;
    while (<IN>) {
	if (/\= entity mention scoring \=/) {
	    $ready = 1;
	} 
	elsif (/\= mention_entity scoring \=/){
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
    open(IN, "$newdir/$name/ace2005-eval.txt");
    print "\t";
    $ready = 0;
    while (<IN>) {
	if (/\= entity mention scoring \=/) {
	    $ready = 1;
	}
	elsif (/\= mention_entity scoring \=/){
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



print "\n              ";
print"\t\tCNT \tFA \tMI \tR_ER \tVALUE \t\tCNT \tFA \tMI \tR_ER \tVALUE  \tFA \tMI \tR_ER \tVALUE\n";
foreach $name (sort @names) {
    if ($str ne "" && !($name =~ /$str/)) {
	next;
    }
    open(IN, "$olddir/$name/ace2005-eval.txt");
    print "\n$name:";

    $ready = 0;
    $old = 0;
    $new = 0;
    @old_nam_vals;
    @old_nom_vals;
    @old_pro_vals;
    @new_nam_vals;
    @new_nom_vals;
    @new_pro_vals;
    
    while (<IN>) {
	if (/\= entity mention scoring \=/) {
	    $ready = 1;
	} 
	elsif (/\= mention_entity scoring \=/){
	    $ready = 1;
	}
	if ($ready && /total/) {
	    <IN>; #empty
	    <IN>; #ref...
	    <IN>; #entity...
	    <IN>; #level ...
	    $line = <IN>;
	    @old_nam_vals = getMatch($line);
	    $line = <IN>;
	    @old_nom_vals = getMatch($line);
	    $line = <IN>;
	    @old_pro_vals = getMatch($line);
	    last;
	}
    }
    close(IN);
    open(IN, "$newdir/$name/ace2005-eval.txt");
    print "\t";
    $ready = 0;
    
    while (<IN>) {
	if (/\= entity mention scoring \=/) {
	    $ready = 1;
	}
	elsif (/\= mention_entity scoring \=/){
	    $ready = 1;
	}
	if ($ready && /total/) {
	    <IN>; #empty
	    <IN>; #ref...
	    <IN>; #entity...
	    <IN>; #level ...
	    $line = <IN>;
	    @new_nam_vals = getMatch($line);
	    $line = <IN>;
	    @new_nom_vals = getMatch($line);
	    $line = <IN>;
	    @new_pro_vals = getMatch($line);
	    last;
	}
    }
    close(IN);
    
    print "\nNAMES: ";
    printValueComp(\@old_nam_vals, \@new_nam_vals);
    print "\nNOMINAL: ";
    printValueComp(\@old_nom_vals, \@new_nom_vals);
    print "\nPRONOUN: ";
    printValueComp(\@old_pro_vals, \@new_pro_vals);
    print "\n";
}

sub getMatch{
    my $line = shift;
    $line=~/^\s*\S+\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+[0-9.]+\s+([\-0-9.]+)\s+/;
    my $tot = $1;
    my $fa  =$2;
    my $mi = $3;
    my $rec = $4;
    my $va = $5;
    return ($tot, $fa, $mi, $rec, $va);
}
sub printValueComp{
    $oldValA = shift;
    $newValA = shift;
    print " \t\t$oldValA->[0] \t$oldValA->[1] \t$oldValA->[2] \t$oldValA->[3] \t$oldValA->[4] \t";
    if(scalar(@$newValA) !=0 ){
	print "   \t$newValA->[0] \t$newValA->[1] \t$newValA->[2] \t$newValA->[3] \t$newValA->[4] \t";
	print "   ";
	for($i = 1; $i <=4; $i++){
	    if($oldValA->[$i] > $newValA->[$i]){
		print " - \t";
	    }
	    elsif($oldValA->[$i] < $newValA->[$i]){
		print " + \t";
	    }
	    else{
		print " ==\t";
	    }
	}
	#print " \t";
    }
}

