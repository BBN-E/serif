my $output_dir = "+OUTPUT_DIR+";
my $summary_file = "+SCORE_SUMMARY_FILE+";
my $expt_name = "+EXPT_NAME+";
my $language = "+LANGUAGE+";


print "\n\n\nSCORES:\n";



my @reports = glob("$output_dir/$expt_name-$language*.scores");
my %scores;

foreach my $r (@reports) {
    $r =~ /$expt_name-$language-(.*)k\.scores/;
    my $config = $1;
    open(SCORE, "$r");
    while (<SCORE>) {
	if (/F-MEASURES\s+([0-9\.]+)/) {
            $scores{$config} = $1;
	    last;
	}
    }
    close(SCORE);
    
}


open(SUMMARY, ">$summary_file");
for my $c (sort {$a <=> $b} (keys %scores)) {
    my $line = sprintf("%8dK words:%8.2f", $c, $scores{$c});
    print SUMMARY "$line\n";
}
close SUMMARY;

