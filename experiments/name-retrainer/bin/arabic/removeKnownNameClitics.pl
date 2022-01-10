#!/usr/local/bin/perl -w

#use utf8;
use strict;
die"Usage: idfSplitPOSClitics.pl dict_file in_file out_file" unless @ARGV >=3;
my ($dict, $infile, $outfile) = @ARGV;
print "$infile\n";

open(INF, "$infile") or die "can't open input- $infile";
open(OUTF, ">$outfile") or die "can't open output- $outfile";
#open(DEBUGF, ">$outfile.debug") or die;
open(DICT, "$dict")or die "can open dict - $dict";

my %clitic_names;
my $name;
my @parts;
my @value;
my $valuestr;
#read the dictionary into a hash 
while(<DICT>){
    my $line = $_;
    #$line =~s/^\s+(\S.*?\S)\s+$/$1/;
    $line =~s/^(.*)\n$/$1/;
    @parts = split(/\t/,$line);
    if($parts[2]=~/\S\s\S/ || $parts[2]=~/[()]/ ){
	print "Ignore $parts[1] $parts[2]\n";
	next;
    }
    $name = $parts[0];
    $value[0] = $parts[1];
    $value[1] = $parts[2];
    $value[0]=~s/\s*$//gs;
    $value[1]=~s/\s*$//gs;
    $valuestr = "$value[0],$value[1]";
    $clitic_names{$valuestr}=[ @value ];
#    $clitic_names{$valuestr}= @value ;
    #print "$line ... $value[0] ... $value[1]\n"
    
}
close(DICT);
my @value2;
my $line;
my $key;
my $w1;
my $w2;
my $linecount =0;
while(<INF>){
    $line =$_;
    $linecount ++;
    foreach $key (keys %clitic_names){
	my @value2 = @{$clitic_names{$key}};
	$w1 = $value2[0];
	$w2 = $value2[1];
	#print "$w1\n$w2\n";
	#( WORD1 TAG1 )( WORD2 TAG2 )
# 	 if($line=~/(\( $w1 )(\S+)( \)\( $w2 )(\S+ \))/){
	 if($line=~/\( ($w1) (\S+) \)\( ($w2) (\S+) \)/){    
	    print "line- $linecount: replace ($1 $2)($3 $4) \n";
	    my $firsttag = $2;
	    my $secondtag = $4;
	    if(($firsttag=~/\-ST/) && ($secondtag=~/\-CO/)){
		$line=~s/(\( $w1) \S+ (\)\( $w2) (\S+)\-\S+ \)/$1 NONE-ST $2 $3\-ST \)/gsi;
		$line=~/(\( $w1 \S+ \)\( $w2 \S+ \))/;
		print "with $1 \n";
	    }
	}
	
    }
    print OUTF $line;
}
close(INF); 
close(OUTF);


	
