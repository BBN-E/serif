#die "usage $0: sexp_file enamex_file" unless (@ARGV==2);
#my ($sexp, $enamex) = @ARGV;

my $sexp = "+sexp_file+";
my $enamex = "+enamex_file+";

open(IN, "$sexp") or die "Cannot open $sexp: $!";
open(OUT, ">$enamex") or die "Cannot open $enamex: $!";


print OUT "<DOC>\n<DOCID>0</DOCID>\n";

while (<IN>) {
    chomp;

    next if /^\s*$/;
  
    my $prev_type = "NONE";
    my $first = 1;
    while (/\(([^\(\)]*) ([^\(\)]*)-([^\(\)]*)\)/g) {

	my $word = $1;
	my $type = $2;
	my $mode = $3;
    
	$type =~ s/PER/PERSON/;
	$type =~ s/LOC/LOCATION/;
	$type =~ s/ORG/ORGANIZATION/;

	if ($mode eq "ST") {
	    if ($prev_type ne "NONE") {
		print OUT "</ENAMEX>";
	    }
	    if ($first != 1) {
		print OUT " ";
	    }
	    if ($type ne "NONE") {
		print OUT "<ENAMEX TYPE=\"$type\">";
	    }
	}
	elsif ($first != 1) {
	    print OUT " ";
	}
	print OUT $word;
        $prev_type = $type;
	$first = 0;
    }
    if ($prev_type ne "NONE") {
	print OUT "</ENAMEX>";
    }
    print OUT " \n\n";
}

print OUT "</DOC>\n";
