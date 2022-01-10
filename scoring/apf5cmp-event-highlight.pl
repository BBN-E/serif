# Copyright BBN Technologies Corp. 2007
# All rights reserved

# Compare apf key to apf responce, show with original source text.
# show cross-linked chains in an apf file 

# Also, attempt cross-linking entity chains based on head intersections.

# CHANGE: try to compensate for incorrect nesting

# CHANGE: print info about cross-entity links for individual mentions in make_link_table

use FindBin;
use lib $FindBin::Bin;

use strict vars;
use XMLDOMWrapper;
use open ':utf8';

require "getopts.pl";

my $USE_RELATIONS = 1;
my %key_mention_start_cache = ();
my %key_mention_end_cache = ();
my %resp_mention_start_cache = ();
my %resp_mention_end_cache = ();

my %key_value_start_cache = ();
my %key_value_end_cache = ();
my %resp_value_start_cache = ();
my %resp_value_end_cache = ();


my %key_mismatch_cache = ();
my %resp_mismatch_cache = ();

my %r2k_rel_cache = ();
my %k2r_rel_cache = ();

my %r2k_eve_cache = ();
my %k2r_eve_cache = ();

# all file names start with the same prefix, but the source, the response and the key file
# differ by standard extension

my( $SRC_EXT, $KEY_EXT1, $KEY_EXT2, $KEY_EXT3, $RES_EXT ) = ( '.sgm', '.sgm.apf.xml', '.sgm.tmx.rdc.xml', '.apf.xml', '.sgm.apf' );

die "Usage $0 [options] name+ \n" .
    "Options: \n" .
    "         -v       verbose mode, all key and response mentions printed\n" .
    "         -s DIR   source dir   [default ., assumed to contain $SRC_EXT files]\n" .
    "         -k DIR   key dir      [default ., assumed to contain $KEY_EXT1 or $KEY_EXT2 files]\n" .
    "         -r DIR   response dir [default ., assumed to contain $RES_EXT files]\n" .
    "         -d DIR   common parent of source and key dirs, \n" .
    "                    which then become DIR/source, DIR/key\n" .
    "         -o DIR   output dir   [default .]\n" .
    "         -i FILE  output from RDC scorer [optional but needed for relation mapping]\n" 
    unless @ARGV >= 1;

use vars qw{ $opt_s $opt_r $opt_k $opt_d $opt_o $opt_v $opt_i }; 

&Getopts('vd:s:r:k:o:i:');

my( $srcDir, $keyDir, $resDir, $src_and_keyDir, $outDir, $scorer_output ) = ($opt_s, $opt_k, $opt_r, $opt_d, $opt_o, $opt_i );

# if the common root of source and key dirs is specified, fill srcDir and keyDir
if( $opt_d ){
    die "Options -s and -d are incompatible" if $opt_s;
    die "Options -k and -d are incompatible" if $opt_k;
    $src_and_keyDir = &normalize_dir( $src_and_keyDir );
    $srcDir = $src_and_keyDir . 'source/';
    $keyDir = $src_and_keyDir . 'key/';
}

if( $opt_i ) {
    &read_scorer_output( $scorer_output );
}


my( @names ) = @ARGV;

# if no names are given, the script expects the source, key and response directories
# to be specified (via -s, -k, -r options), and deduces the names from the source directory

if( ! @names ){
    die "Source directory must be specified\n" unless $opt_s || $opt_d; 

    # get the storynames from the source directory, based on $SRC_EXT.
    opendir(SRCDIR, $srcDir) || die "Can't opendir $srcDir: $!";

    # assume that the files in srcDir are named according to the scheme 
    #           /^(storyname)$SRC_EXT$/
    my @src_files = grep { /$SRC_EXT$/ } readdir(SRCDIR);
    
    # break off the extensions
    @names = map { s/$SRC_EXT$//; $_ } @src_files;

    closedir SRCDIR;
}

print "Names: " , join( ' ', @names ), "\n";

# Test to determine which extension ($KEY_EXT1 or $KEY_EXT2) is found in $keyDir 
my $KEY_EXT = &get_key_ext();

sub normalize_dir {
    my( $dirname ) = @_;
    return './' if $dirname eq '';
    
    die "Dir $dirname does not exist!\n" unless -d $dirname;

    if( $dirname !~ m|/$| ){
	return $dirname .'/' ;
    }
    else {
	return $dirname;
    }
}


sub get_key_ext {
   
    my $result = '';    
    opendir(KEYDIR, $keyDir) || die "Can't opendir $keyDir: $!";
    my @key_files = readdir(KEYDIR);
    my @tmp = grep {/$KEY_EXT1$/ } @key_files; 
    
    if ($#tmp > -1) {
        $result = $KEY_EXT1;
    }
    else {
        @tmp = grep { /$KEY_EXT2$/ } @key_files;
        if ($#tmp > -1) {
            $result = $KEY_EXT2;
        } else {
	    @tmp = grep { /$KEY_EXT3$/ } @key_files;
	    if ($#tmp > -1) {
		$result = $KEY_EXT3;
	    } 
	}
    }
    closedir KEYDIR;
    $result;
}

## check if the dirs end in '/', add it if not
$srcDir = &normalize_dir( $srcDir );
$keyDir = &normalize_dir( $keyDir );
$resDir = &normalize_dir( $resDir );

$outDir = &normalize_dir( $outDir );

print "Source dir: $srcDir\n";
print "Key dir:    $keyDir\n";
print "Resp dir:   $resDir\n";
print "Output dir: $outDir\n";


for my $name (@names){
    %key_mention_start_cache = ();
    %key_mention_end_cache = ();
    %resp_mention_start_cache = ();
    %resp_mention_end_cache = ();

    %key_value_start_cache = ();
    %key_value_end_cache = ();
    %resp_value_start_cache = ();
    %resp_value_end_cache = ();

    print $name, "\n";

    ## ---------------- open output files --------------------------------------------------

    # keep output files, except the top level _view file in an aux dir
    my $auxdir = 'dat';
    if( ! -d "$outDir$auxdir" ){
        mkdir( "$outDir$auxdir", 0777 );
    }
    

    my( $topfile, $key_botfile, $resp_botfile, $key_relfile, $resp_relfile ) = 
	( "${name}_top.html", 
	  "${name}_kbot.html",
	  "${name}_rbot.html", 
	  "${name}_krel.html", 
	  "${name}_rrel.html" );

    open( TOP,  ">$outDir$auxdir/$topfile" ) || die "Cannot open $outDir$auxdir/$topfile" ;
    open( BOTK, ">$outDir$auxdir/$key_botfile" ) || die "Cannot open $outDir$auxdir/$key_botfile" ;
    open( BOTR, ">$outDir$auxdir/$resp_botfile" ) || die "Cannot open $outDir$auxdir/$resp_botfile" ;

    if( $USE_RELATIONS ) {
	open( RELK, ">$outDir$auxdir/$key_relfile" ) || die "Cannot open $outDir$auxdir/$key_relfile" ;
	open( RELR, ">$outDir$auxdir/$resp_relfile" ) || die "Cannot open $outDir$auxdir/$resp_relfile" ;
    }
    
    # print TOP "<HTML><BASE TARGET=\"BOT\">\n";
    print TOP "<HTML>\n<BODY BGCOLOR=white LINK=purple VLINK=purple>\n";
    print BOTK "<HTML><BASE TARGET=\"TOP\">\n";
    print BOTR "<HTML><BASE TARGET=\"TOP\">\n";
    
    if( $USE_RELATIONS ) {
	print RELK "<HTML><BASE TARGET=\"TOP\">\n";
	print RELR "<HTML><BASE TARGET=\"TOP\">\n";
    }
    
    # print TOP "<FONT FACE=\"Courier New\">";
    print TOP "<FONT size=+1>";

    &make_frames_file( "$outDir${name}_view.html", 
		       "$auxdir/$topfile", 
		       "$auxdir/$key_botfile", 
		       "$auxdir/$resp_botfile", 
		       "$auxdir/$key_relfile", 
		       "$auxdir/$resp_relfile" );
    
    ## ---------------- read the key and extract info and offsets for entities' mentions ---

    my $apfKey = APFDocument->new(  "$keyDir$name$KEY_EXT", "k" );
    my $apfRes = APFDocument->new(  "$resDir$name$RES_EXT", "r" );


    ## ------------- get the source text and make the offeset reference text ---------

    # get the entire source file in one string 
    undef $/;
    open( SRC, "<$srcDir$name$SRC_EXT" ) || die "Cannot open $name$SRC_EXT"; 

    my $src_text = <SRC>;

    # Remove all sgml tags. This should give the reference text for source

    my $ref_src_text = $src_text;  # make a copy of source text
    $ref_src_text =~ s/<[^>]+>//sg; 

    # DEBUG {
    # print "------------------------\n";
    # print $ref_src_text , "\n";
    # print "------------------------\n";
    # }

    ## -------- print mentions grouped by entity and collect them for sorting ------

    # To form the inserts with proper bracketing, we must sort the extents
    # of all mentions together, not just within each entity. Thus we must create
    # a list if all mentions, regardless of entity, and sort it.
    
    my( $linkTable12, $mentionLinkTable12 ) = &make_link_table( $apfRes, $apfKey );
    my( $linkTable21, $mentionLinkTable21 ) = &make_link_table( $apfKey, $apfRes );
    
    &make_mentions_frame( \*BOTK, $apfKey, $ref_src_text, 'k', $linkTable21, $mentionLinkTable21,
			  'BOTR', $resp_botfile, $topfile ); 
    &make_mentions_frame( \*BOTR, $apfRes, $ref_src_text, 'r', $linkTable12, $mentionLinkTable12,
			  'BOTK', $key_botfile, $topfile ); 

    if( $USE_RELATIONS ) {
	&make_relations_frame( \*RELK, $apfKey, $ref_src_text, 'k', 'BOTK', 'RELR', $resp_relfile, $key_botfile, $topfile );
	&make_relations_frame( \*RELR, $apfRes, $ref_src_text, 'r', 'BOTR', 'RELK', $key_relfile, $resp_botfile, $topfile );
    }
    
    # k for key, r for resp

    ## ------------------ insert brackets for mentions ---------------------------------


    # insert bracketing or any other text into the ref_src_text
    my %inserts = ();

     # Sort mentions of all entities in one bin, so that the mentions are ordered
     # by their start offset, and when one mention covers another, the covering
     # one comes up after the covered one. That way the closing bracket for the covering
     # outer mention can be correctly placed behind the inner covered mention's bracket.

    # mix mentions from key and response, and tag them as coming from such

    my @key_mentions  = $apfKey->mentions();
    my @resp_mentions = $apfRes->mentions();
    my @key_events = $apfKey->events();
    my @resp_events = $apfRes->events();
    my @key_values = $apfKey->values();
    my @resp_values = $apfRes->values();

    my @mentions = ();
    for my $mention (@key_mentions) {
	push @mentions, $mention . ' k';
	#print $mention . "\n";
    }
    for my $mention (@resp_mentions){
	push @mentions, $mention . ' r';
    }
    
    # make a "mention" out of each event mention;
    for my $event (@key_events) {
	my $id = $event->getAttribute( "ID" );
	my $type = $event->getAttribute( "TYPE" );
	my $ment_type = "NOM";

	my $event_mention_list = $event->getElementsByTagName( "event_mention" );
	for( my $i=0; $i < $event_mention_list->getLength; $i++ ) {
	    my $event_mention = $event_mention_list->item( $i );
	    
	    my @anchors = $event_mention->getElementsByTagName( 'anchor' );
	    my $anchor = $anchors[0];
	    my @charseqs = $anchor->getElementsByTagName( 'charseq' );
	    my $charseq = $charseqs[0];
	    my $start     = $charseq->getAttribute( "START" );
	    my $end       = $charseq->getAttribute( "END" );
	    
	    my $event_mention_id = $event_mention->getAttribute( "ID" );
	    push @mentions, "$start $end $id $type $ment_type $event_mention_id k 1";
	}
    }

    for my $event (@resp_events) {
	my $id = $event->getAttribute( "ID" );
	my $type = $event->getAttribute( "TYPE" );
	my $ment_type = "NOM";

	my $event_mention_list = $event->getElementsByTagName( "event_mention" );
	for( my $i=0; $i < $event_mention_list->getLength; $i++ ) {
	    my $event_mention = $event_mention_list->item( $i );

	    my @anchors = $event_mention->getElementsByTagName( 'anchor' );
	    my $anchor = $anchors[0];
	    my @charseqs = $anchor->getElementsByTagName( 'charseq' );
	    my $charseq = $charseqs[0];
	    my $start     = $charseq->getAttribute( "START" );
	    my $end       = $charseq->getAttribute( "END" );
	    
	    my $event_mention_id = $event_mention->getAttribute( "ID" );
	    push @mentions, "$start $end $id $type $ment_type $event_mention_id r 1";
	}
    }
    
    # make a "mention" out of each value
    for my $value ( @key_values ) {
	my $id = $value->getAttribute( 'ID' );
	my $value_mention = ${$value->getElementsByTagName( 'value_mention' )}[0];
	my $is_timex = 0;
	if (!$value_mention) {
	    $is_timex = 1;
	    $value_mention = ${$value->getElementsByTagName( 'timex2_mention' )}[0];
	}
	
	my $value_mention_id = $value_mention->getAttribute( 'ID' );
	
	my $type = $value->getAttribute( 'SUBTYPE' );
	if (!$type) {
	    $type = $value->getAttribute( 'TYPE' );
	}
	if ($is_timex) {
	    $type = 'Time';
	}
	if (!$type) {
	    $type = 'Unknown';
	}
	my $ment_type = "NOM";
	
	my $extent = ${$value_mention->getElementsByTagName( 'extent' )}[0];
	my $charseq = ${$extent->getElementsByTagName( 'charseq' )}[0];
	my $start     = $charseq->getAttribute( "START" );
	my $end       = $charseq->getAttribute( "END" );
	    
	push @mentions, "$start $end $id $type $ment_type $value_mention_id k 2";
    }
    
    for my $value ( @resp_values ) {
	my $id = $value->getAttribute( 'ID' );
	my $value_mention = ${$value->getElementsByTagName( 'value_mention' )}[0];
	my $is_timex = 0;
	if (!$value_mention) {
	    $is_timex = 1;
	    $value_mention = ${$value->getElementsByTagName( 'timex2_mention' )}[0];
	}
	
	my $value_mention_id = $value_mention->getAttribute( 'ID' );
	
	my $type = $value->getAttribute( 'SUBTYPE' );
	if (!$type) {
	    $type = $value->getAttribute( 'TYPE' );
	}
	if ($is_timex) {
	    $type = 'Time';
	}
	if (!$type) {
	    $type = 'Unknown';
	}
	my $ment_type = "NOM";
	
	my $extent = ${$value_mention->getElementsByTagName( 'extent' )}[0];
	my $charseq = ${$extent->getElementsByTagName( 'charseq' )}[0];
	my $start     = $charseq->getAttribute( "START" );
	my $end       = $charseq->getAttribute( "END" );
	
	push @mentions, "$start $end $id $type $ment_type $value_mention_id r 2";
    }
    
    my @sorted_mentions = sort cover_order @mentions;

    for my $mention (@sorted_mentions){
	my( $start, $end, $id, $type,  $mention_type, $mention_id, $docmark, $rel_file_flag ) = split( ' ', $mention );
	
	## print out the info about the mention if VERBOSE is set
	if( $opt_v ){
	    my $mention_text =  substr($ref_src_text, $start, $end - $start + 1);
	    $mention_text =~ s/\n/ /gs;
	    $mention_text =~ s/\&AMP;/\&/gs;
	    print "$start, $end, $id, $type,  $mention_type, $mention_id, $docmark, \"$mention_text\"\n"; 
	}
	# end printting mention text

	if( $docmark eq 'k' ){  
	    my $short_id = &shorten_id( $id );
	    ## ------------------------ key mention ----------------------------------
	    my $file = $key_botfile;
	    $file = $key_relfile if $rel_file_flag == 1;
	    my $target = "BOTK";
	    $target = "RELK" if $rel_file_flag == 1;
	    my $ins_start_text =  '(' . uc( $type ) . "-$short_id " ;
	    my $ins_end_text   =    " $short_id)" ;  

	    my $color = '<FONT color=blue>';
	    my $end_color = '</FONT>';
	    # for values don't print out text
	    #if( $rel_file_flag == 2) {
	#	$ins_start_text = '';
	#	$ins_end_text == '';
	#	$color = '';
	#	$end_color = '';
	#    }
	    
	    $inserts{ $start } .= "<A NAME=\"$docmark-$mention_id\">";
	    if( $rel_file_flag == 1) {
		$inserts{ $start } .= "<A TARGET=\"$target\" HREF=\"$file#$docmark-$mention_id\">" .
		    $ins_start_text . "</A>$color";
		$inserts{ $end + 1 } = "$end_color<A TARGET=\"$target\" HREF=\"$file#$docmark-$mention_id\">" .
		    $ins_end_text . 
		    '</A>' . 
		    $inserts{ $end + 1 } ;  # NOTE the order of appending!
	    }
	}
	elsif( $docmark eq 'r' ){
	    my $short_id = &shorten_id( $id );
	    ## ------------------------ response mention ----------------------------------
	    my $file = $resp_botfile;
	    $file = $resp_relfile if $rel_file_flag == 1;
	    my $target = "BOTR";
	    $target = "RELR" if $rel_file_flag == 1;

	    my $ins_start_text =  '&lt;' . lc( $type ) . "-<FONT SIZE=\"-1\">$short_id</FONT> " ;
	    my $ins_end_text   =    " <FONT SIZE=\"-1\">$short_id</FONT>&gt;" ;  

	    my $color = '<FONT color=blue>';
	    my $end_color = '</FONT>';
	    # for values don't print out text
	    #if( $rel_file_flag == 2) {
	#	$ins_start_text = '';
	#	$ins_end_text == '';
	#	$color = '';
	#	$end_color = '';
	#    }
	    
	    $inserts{ $start }   .= "<A NAME=\"$docmark-$mention_id\">";
	    if( $rel_file_flag == 1 ) {
		$inserts{ $start }  .= "<A TARGET=\"$target\" HREF=\"$file#$docmark-$mention_id\">" .
		    $ins_start_text .
		    "</A>$color";
		$inserts{ $end + 1 } = "$end_color<A TARGET=\"$target\" HREF=\"$file#$docmark-$mention_id\">" .
		    $ins_end_text . 
		    '</A>' .
		    $inserts{ $end + 1 } ;
	    }
	}
	else {
	    die "Expected: 'r' or 'k', not $docmark" ;
	}
    }   

    my $decorated_text = &apply_inserts( $ref_src_text, \%inserts );
    
    # clean up decorated text for HTML display
    $decorated_text =~ s/\n/<br>\n\n/sg;

    print TOP $decorated_text , "\n";
    
    # pad the frame files for scrolling
    for( my $i=0; $i < 10; $i++ ){
	print TOP "<P>&nbsp;\n";
	print BOTR "<P>&nbsp;\n";
	print BOTK "<P>&nbsp;\n";
	if( $USE_RELATIONS ) {
	    print RELR "<P>&nbsp;\n";
	    print RELK "<P>&nbsp;\n";
	}
    }
    
    close( TOP );
    close( BOTK );
    close( BOTR );
    if( $USE_RELATIONS ) {
	close( RELK );
	close( RELR );
    }
} # end processing file

#############################################################################
#
#  Helper functions
#

# put the inserts into the string
sub apply_inserts {
    my( $string, $inserts ) = @_;

    my @sorted_offsets = sort { $a <=> $b } keys %$inserts;
    
    my $curr_string = $string;
    my $total_displacement = 0; 

    for my $key (@sorted_offsets){
	my $val = $inserts->{ $key };
	
	my $val_len = length( $val );
	substr( $curr_string, $key + $total_displacement, 0, $val );
	$total_displacement += $val_len;
    }

    return $curr_string;
}

sub make_frames_file {
    my( $fname, $top, $key_bot, $resp_bot, $key_rel, $resp_rel ) = @_;
    open( FRA, ">$fname" ) || die "Cannot open $fname"; 

    if( !$USE_RELATIONS ) {
    print FRA <<"END_OF_FRAMES";
<HTML> 
    <HEAD> 
    </HEAD> 
    <FRAMESET ROWS="50%,50%" > 
	<FRAME NAME="TOP" SRC="$top" SCROLLING="yes" >
	<FRAMESET COLS="50%,50%" > 
	    <FRAME NAME="BOTK" SRC="$key_bot" SCROLLING="yes" >
	    <FRAME NAME="BOTR" SRC="$resp_bot" SCROLLING="yes" >
	</FRAMESET> 
    </FRAMESET> 
</HTML> 
END_OF_FRAMES


} else {
    print FRA <<"END_OF_FRAMES";
<HTML> 
    <HEAD> 
    </HEAD> 
    <FRAMESET ROWS="35%,35%,30%" > 
	<FRAME NAME="TOP" SRC="$top" SCROLLING="yes" >
	<FRAMESET COLS="50%,50%" > 
	    <FRAME NAME="BOTK" SRC="$key_bot" SCROLLING="yes" >
	    <FRAME NAME="BOTR" SRC="$resp_bot" SCROLLING="yes" >
	</FRAMESET>
	<FRAMESET COLS="50%,50%" > 
	    <FRAME NAME="RELK" SRC="$key_rel" SCROLLING="yes" >
	    <FRAME NAME="RELR" SRC="$resp_rel" SCROLLING="yes" >
	</FRAMESET>
    </FRAMESET> 
</HTML> 
END_OF_FRAMES
}
    close( FRA );
}

#############################################################################
#
# Make a table of links between two documents (i.e. their sets of entities)
#

#
# record and return the table of mentions that introduce a new entity into the chain
#

# the link table is ent_id1 --> ( ent_id2* )

sub make_link_table
{
    my( $doc1, $doc2 ) = @_;
    
    my @extents1   = $doc1->extents();
    my @extents2   = $doc2->extents();

    my %links = ();

    my %mention_links = ();

    for my $ext_list1 (@extents1){
	for my $ment1 ( @$ext_list1 ){
	    my( $start1, $end1, $ent_id1, $ent_type1, $mention_type1, 
		$mention_id1, $head_start1, $head_end1 ) = split( ' ', $ment1 );    

	    for my $ext_list2 (@extents2){
		for my $ment2 ( @$ext_list2 ){
		    my( $start2, $end2, $ent_id2, $ent_type2, $mention_type2, 
			$mention_id2, $head_start2, $head_end2 ) = split( ' ', $ment2 );    

		    # if the link between entities has already been established, no need to loop
		    # over the mentions. Bail out of this loop.
		    next if $links{ "$ent_id1 $ent_id2" };
		    
		    #if heads overlap, link the chains
		    if( &overlaps( $head_start1, $head_end1, $head_start2, $head_end2 ) ){  

			#DEBUG{ print "$head_start1, $head_end1, $head_start2, $head_end2, $ent_id1, $ent_id2 \n"; }
			$links{ "$ent_id1 $ent_id2" } = 1;

			# record which mention introduced this link
			$mention_links{ "$ent_id1 $mention_id1" } .= " $ent_id2";

			# no need to check other mentions once the connection is established, so:
			next;
		    }
		} # end ment2
	    } # end doc2 entities
	} # end ment1
    } # end doc1 entities

    # re-index the links table
    my %edges = ();

    # DEBUG{ print join( ', ', keys %links ); }

    for my $edge (keys %links){
	my( $v1, $v2 ) = split( ' ', $edge );
	if( exists $edges{$v1} ){
	    push @{$edges{$v1}}, $v2;
	}
	else {
	    $edges{$v1} = [ $v2 ];
	}
    }

    return ( \%edges, \%mention_links );
}

# is x between y and z?
sub is_between {
    my( $x, $y, $z ) = @_;
    return ( $z >= $x && $y <= $x );
} 


sub overlaps {
    my( $x1, $y1, $x2, $y2 ) = @_;

    return 1 if &is_between( $x1, $x2, $y2 );
    return 1 if &is_between( $y1, $x2, $y2 );
    return 1 if &is_between( $x2, $x1, $y1 );
    return 1 if &is_between( $y2, $x1, $y1 );

    return 0;
}

sub make_relations_frame
{
    my( $FH, $apfDoc, $ref_src_text, $docmark, $target_file, $rel_target_file, $other_rel_file, $botfile, $topfile ) = @_;
    
    my @relations = $apfDoc->relations();
    my @events = $apfDoc->events();

    # foreach my $relation ( @relations ) {
    	
# 	my $rel_id = $relation->getAttribute( 'ID' );
# 	my $type = $relation->getAttribute( 'TYPE' );
# 	my $subtype = $relation->getAttribute( 'SUBTYPE' );
# 	my $argument_list = $relation->getElementsByTagName( 'relation_argument' );
# 	my $arg1id = "";
# 	my $arg2id = "";
	
# 	for ( my $i = 0; $i < $argument_list->getLength; $i++ ) {
# 	    my $argument = $argument_list->item( $i );
# 	    if ( $argument->getAttribute( 'ROLE' ) eq 'Arg-1' ) {
# 		$arg1id = $argument->getAttribute( 'REFID' );
# 	    } elsif ( $argument->getAttribute( 'ROLE' ) eq 'Arg-2' ) {
# 		$arg2id = $argument->getAttribute( 'REFID' );
# 	    } 
# 	}

# 	my $relation_mentions = $relation->getElementsByTagName( 'relation_mention' );

# 	my $modality = $relation->getAttribute( "MODALITY" );
# 	my $genericity = $relation->getAttribute( "GENERICITY" );
# 	my $polarity = $relation->getAttribute( "POLARITY" );
# 	my $tense = $relation->getAttribute( "TENSE" );

# 	my %cache;
# 	my %mismatch_cache;
	
# 	my $short_rel_id = &shorten_id( $rel_id );
	
# 	print $FH "<A NAME=\"$rel_id\"> ";
# 	print $FH "<FONT COLOR=darkgreen>R$short_rel_id</FONT>/<b>$type:$subtype&nbsp;&nbsp;</b>";
	
# 	if( $modality || $genericity || $polarity || $tense ) {
# 	    my $started = 0;
# 	    my $info = "";
# 	    if( $modality ) {
# 		$started = 1;
# 		$info .= $modality;
# 	    }
# 	    if( $genericity ) {
# 		$info .= ", " if $started;
# 		$started = 1;
# 		$info .= $genericity;
# 	    }
# 	    if( $polarity ) {
# 		$info .= ", " if $started;
# 		$started = 1;
# 		$info .= $polarity;
# 	    }
# 	    if( $tense ) {
# 		$info .= ", " if $started;
# 		$started = 1;
# 		$info .= $tense;
# 	    }
# 	    print $FH "<FONT FACE=\"garamond\" SIZE=-1>($info)</FONT>" 
# 	}

# 	if( $docmark eq 'k' ) {
# 	    %cache = %k2r_rel_cache;
# 	    %mismatch_cache = %key_mismatch_cache;
# 	} elsif( $docmark eq 'r' ) {
# 	    %cache = %r2k_rel_cache;
# 	    %mismatch_cache = %resp_mismatch_cache;
# 	} else {
# 	    die "docmark must be 'r' or 'k'";
# 	}
# 	my $mapped_rel_id = $cache{ $rel_id } ;
# 	if( $mapped_rel_id ) {
# 	    $mapped_rel_id =~ /-R(\d+)$/;
# 	    print $FH "&nbsp;&nbsp;(<A TARGET=\"$rel_target_file\"; HREF=\"$other_rel_file#$mapped_rel_id\">&gt;R$1</A>)";
	    
# 	    my $mismatch;
# 	    # print out whether there were any errors reported in scorer output file
# 	    #if( ( $mismatch = $mismatch_cache{ $rel_id } ) ) {
# 	#	print $FH "&nbsp;&nbsp;(<FONT COLOR=red SIZE=-1>$mismatch</FONT>)";
# 	#    }   
	    
# 	} else {
# 	    if( $docmark eq 'k' ) {
# 		print $FH "&nbsp;&nbsp;(<FONT COLOR=red SIZE=-1>MISS</FONT>)";
# 	    } elsif( $docmark eq 'r' ) {
# 		print $FH "&nbsp;&nbsp;(<FONT COLOR=red SIZE=-1>SPURIOUS</FONT>)";
# 	    }
# 	}
       

# 	print $FH "<br>\n";
	
# 	# implicit relations may not have any mentions
# 	if( $relation_mentions->getLength == 0 ) {
# 	    my $short_a1id = &shorten_id( $arg1id );
# 	    my $short_a2id = &shorten_id( $arg2id );
# 	    print $FH "Arg1: <A TARGET=\"$target_file\"; HREF=\"$botfile#$arg1id\">&gt;$short_a1id</A><br>";
# 	    print $FH "Arg2: <A TARGET=\"$target_file\"; HREF=\"$botfile#$arg2id\">&gt;$short_a2id</A>";
# 	    print $FH "<P>\n\n";
# 	}
	    

# 	for( my $i = 0; $i < $relation_mentions->getLength; $i++ ) {
# 	    my $rel_mention = $relation_mentions->item( $i );
	    
# 	    my $arguments = $rel_mention->getElementsByTagName( 'relation_mention_argument' );
	    
# 	    my $arg1 = "";
# 	    my $arg2 = "";
# 	    my @additional_args = ();
# 	    for( my $j = 0; $j < $arguments->getLength; $j++ ) {
# 		my $arg = $arguments->item( $j );
# 		if( $arg->getAttribute( 'ROLE' ) eq 'Arg-1' ) {
# 		    $arg1 = $arg->getAttribute( 'REFID' );
# 		} elsif( $arg->getAttribute( 'ROLE' ) eq 'Arg-2' ) {
# 		    $arg2 = $arg->getAttribute( 'REFID' );
# 		} else {
# 		    push @additional_args, $arg;
# 		}
# 	    }
	   
# 	    my ( $start1, $end1, $start2, $end2 );
	   
# 	    if( $docmark eq 'k' ) {
# 		$start1 = $key_mention_start_cache{ $arg1 };
# 		$end1 = $key_mention_end_cache{ $arg1 };
		
# 		$start2 = $key_mention_start_cache{ $arg2 };
# 		$end2 = $key_mention_end_cache{ $arg2 };
# 	    } elsif ( $docmark eq 'r' ) {
# 		$start1 = $resp_mention_start_cache{ $arg1 };
# 		$end1 = $resp_mention_end_cache{ $arg1 };
		
# 		$start2 = $resp_mention_start_cache{ $arg2 };
# 		$end2 = $resp_mention_end_cache{ $arg2 };
# 	    }

# 	    #if ($arg1 eq 'AGGRESSIVEVOICEDAILY_20050224.2252-T1-M1') {
# 	#	print "found it start1 is $start1\n";
# 	#    }
	    
# 	    my $short_a1id = &shorten_id( $arg1id );
# 	    my $short_a2id = &shorten_id( $arg2id );

# 	    my $mention_str = substr( $ref_src_text, $start1, $end1 - $start1 + 1 );
# 	    print $FH "Arg1: ";
# 	    print $FH "<A TARGET=\"$target_file\"; HREF=\"$botfile#$arg1id\">&gt;E$short_a1id</A>&nbsp;&nbsp;";
# 	    print $FH "<A HREF=\"$topfile#$docmark-$arg1\">$mention_str</A><br>";
	    
# 	    my $mention_str = substr( $ref_src_text, $start2, $end2 - $start2 + 1 );
# 	    print $FH "Arg2: ";
# 	    print $FH "<A TARGET=\"$target_file\"; HREF=\"$botfile#$arg2id\">&gt;E$short_a2id</A>&nbsp;&nbsp;";
# 	    print $FH "<A HREF=\"$topfile#$docmark-$arg2\">$mention_str</A><br>";
	    

# 	    foreach my $arg ( @additional_args ) {
# 		my $id = $arg->getAttribute( 'REFID' );
# 		my $role = $arg->getAttribute( 'ROLE' );
		
# 		my( $start, $end );
# 		if( $docmark eq 'k' ) {
# 		    $start = $key_value_start_cache{ $id };
# 		    $end = $key_value_end_cache{ $id };
# 		} elsif ( $docmark eq 'r' ) {
# 		    $start = $resp_value_start_cache{ $id };
# 		    $end = $resp_value_end_cache{ $id };
# 		}
	       
# 		my $value_str = substr( $ref_src_text, $start, $end - $start + 1 );
# 		print $FH "$role: ";
# 		print $FH "<A HREF=\"$topfile#$docmark-$id\">$value_str</A><br>";
# 	    }
# 	    print $FH "<P>\n\n";
# 	}	   
#     }

    foreach my $event ( @events ) {
	my $id = $event->getAttribute( "ID" );
	my $type = $event->getAttribute( "TYPE" );
	my $subtype = $event->getAttribute( "SUBTYPE" );
	my $modality = $event->getAttribute( "MODALITY" );
	my $genericity = $event->getAttribute( "GENERICITY" );
	my $polarity = $event->getAttribute( "POLARITY" );
	my $tense = $event->getAttribute( "TENSE" );

	my $short_id = &shorten_id( $id );
	print $FH "<A NAME=\"$id\"> ";
	print $FH "<FONT COLOR=darkgreen>V$short_id</FONT>/<b>$type:$subtype&nbsp;&nbsp;</b>";
	if( $modality || $genericity || $polarity || $tense ) {
	    my $started = 0;
	    my $info = "";
	    if( $modality ) {
		$started = 1;
		$info .= $modality;
	    }
	    if( $genericity ) {
		$info .= ", " if $started;
		$started = 1;
		$info .= $genericity;
	    }
	    if( $polarity ) {
		$info .= ", " if $started;
		$started = 1;
		$info .= $polarity;
	    }
	    if( $tense ) {
		$info .= ", " if $started;
		$started = 1;
		$info .= $tense;
	    }
	    print $FH "<FONT FACE=\"garamond\" SIZE=-1>($info)</FONT>" 
	}
	
	my %cache;
	my %mismatch_cache;
	if( $docmark eq 'k' ) {
	    %cache = %k2r_eve_cache;
	    %mismatch_cache = %key_mismatch_cache;
	} elsif( $docmark eq 'r' ) {
	    %cache = %r2k_eve_cache;
	    %mismatch_cache = %resp_mismatch_cache;
	} else {
	    die "docmark must be 'r' or 'k'";
	}
	my $mapped_eve_id = $cache{ $id } ;
	if( $mapped_eve_id ) {
	    my $short_eve_id = &shorten_id( $mapped_eve_id );
	    print $FH "&nbsp;&nbsp;(<A TARGET=\"$rel_target_file\"; HREF=\"$other_rel_file#$mapped_eve_id\">&gt;V$short_eve_id</A>)";
	    my $mismatch;
	    # print out whether there were any errors reported in scorer output file
	    #if( ( $mismatch = $mismatch_cache{ $id } ) ) {
		#print $FH "&nbsp;&nbsp;(<FONT COLOR=red SIZE=-1>$mismatch</FONT>)";
	    #}   
	}
	else { 
	    if( $docmark eq 'k' ) {
		print $FH "&nbsp;&nbsp;(<FONT COLOR=red SIZE=-1>MISS</FONT>)";
	    } elsif( $docmark eq 'r' ) {
		print $FH "&nbsp;&nbsp;(<FONT COLOR=red SIZE=-1>SPURIOUS</FONT>)";
	    }
	}
            

	print $FH "<br>\n";
    
	my $event_mention_list = $event->getElementsByTagName( 'event_mention' );
	for( my $i = 0; $i < $event_mention_list->getLength; $i++ ) {
    
	    my $event_mention = $event_mention_list->item( $i );	
	    my $emid = $event_mention->getAttribute( "ID" );
	    
	    my $extent = ${$event_mention->getElementsByTagName( 'anchor' )}[0];
	    my $charseq = ${$extent->getElementsByTagName( 'charseq' )}[0];
	    my $start     = $charseq->getAttribute( "START" );
	    my $end       = $charseq->getAttribute( "END" );

	    my $anchor_str = substr( $ref_src_text, $start, $end - $start + 1 );
	    print $FH "<A NAME=\"$docmark-$emid\">Anchor:";
	    print $FH "&nbsp;<A HREF=\"$topfile#$docmark-$emid\">$anchor_str</A><br>";
	    
	    #print "Found EMID $emid\n";
	    my $event_mention_arg_list = $event_mention->getElementsByTagName( 'event_mention_argument' );
	    for( my $j = 0; $j < $event_mention_arg_list->getLength; $j++ ) {
		#print "Foudn event mention arg\n";
		my $event_mention_arg = $event_mention_arg_list->item( $j );
		my $mention_id = $event_mention_arg->getAttribute( "REFID" );
		my $role = $event_mention_arg->getAttribute( "ROLE" );
		
		my $entity_id = "";
		$entity_id = $1 if $mention_id =~ /(.*)-[^-]+$/;
		my $short_entity_id = &shorten_id( $entity_id );
		
		my( $start, $end );
		if( $docmark eq 'k' ) {
		    $start = $key_mention_start_cache{ $mention_id };
		    $end = $key_mention_end_cache{ $mention_id };
		} elsif ( $docmark eq 'r' ) {
		    $start = $resp_mention_start_cache{ $mention_id };
		    $end = $resp_mention_end_cache{ $mention_id };
		}
		my $is_value = 0;
		if ($start eq "") {
		    $is_value = 1;
		    if( $docmark eq 'k' ) {
			$start = $key_value_start_cache{ $mention_id };
			$end = $key_value_end_cache{ $mention_id };
		    } elsif ( $docmark eq 'r' ) {
			$start = $resp_value_start_cache{ $mention_id };
			$end = $resp_value_end_cache{ $mention_id };
		    }
		}

		my $mention_str = substr( $ref_src_text, $start, $end - $start + 1 );
		print $FH "$role: ";
		if (!$is_value) {
		    print $FH "<A TARGET=\"$target_file\"; HREF=\"$botfile#$entity_id\">&gt;E$short_entity_id</A>&nbsp;&nbsp;";
		}
		print $FH "<A HREF=\"$topfile#$docmark-$mention_id\">$mention_str</A><br>";
		
	    }
	    print $FH "<br>\n";
	}
	
    }
}
	
    

#############################################################################
#
# Fill a bottom frame with mentions, grouped by entity. Take another document,
# and use it to collate and cross-link mention heads.
#

sub make_mentions_frame 
{
    my( $FH, $apfDoc, $ref_src_text, $docmark, $linkTable, $mentionLinkTable, 
	$target, $targetFile, $topfile ) = @_;  
    
    my @extents   = $apfDoc->extents();
    my @ent_ids   = $apfDoc->ent_ids();
    my @ent_types = $apfDoc->ent_types();
    my @ent_classes = $apfDoc->ent_classes();
    my @ent_subtypes = $apfDoc->ent_subtypes();


    # print all mentions, grouped by entity
    for my $ext_list (@extents){
	my( @exts ) =  @$ext_list ;
    
	my $id   = shift @ent_ids;
	my $type = shift @ent_types;
	my $subtype = shift @ent_subtypes;
	my $class = shift @ent_classes;
	
	my $short_id = &shorten_id( $id );
	
	print $FH "<A NAME=\"$id\">";
	print $FH "<FONT COLOR=darkgreen>E$short_id</FONT>/" . "<b>" . substr( $type, 0, 3 );
	print $FH ":$subtype" if $subtype;
	print $FH '</b> ';
	print $FH "<FONT FACE=\"garamond\" SIZE=-1>($class)</FONT>" if $class;
	print $FH "<br>\n";
	
	# Let us re-order the pairs of offsets so that of the two spans
	# with the same starting point, the outermost span comes last.
	# Note that spans are not allowed to intersect (i.e. cross like [(]) ). 
	
	my @sorted_exts = sort cover_order @exts;

	my $entity_id;

	while( @sorted_exts ){
	    my $data = shift @sorted_exts;
	    #print "Sorted exts:\n";
	    #print $data . "\n";
	    
	    # get the mention, and its info
	    my @data = split( ' ', $data );
	    die "Mention data missing element(s)" unless scalar( @data ) == 8;
	    my( $start, $end, $ent_id, $ent_type, 
		$mention_type, $mention_id, $head_start, $head_end ) = @data;
	    
	    $entity_id = $ent_id;

	    my $mention_str = substr( $ref_src_text, $start, $end - $start + 1 );

	    # clean mention_str for the show
	    $mention_str =~ s/\n/ /gs;
	    $mention_str =~ s/\&AMP;/\&/gs;
	    

	    my $link_start =  " <A NAME=\"$docmark-$mention_id\"> " . 
		              "<A HREF=\"$topfile#$docmark-$mention_id\">";
	    my $link_end   =  '</A>&nbsp; &nbsp;';
	    
	    my $link_text = $mention_str;

	    # print NOMINALs in boldface
	    if( $mention_type eq 'NOM' || $mention_type eq 'PRE' ){
		$link_text = '<B>' . $mention_str . '</B>';
	    }

	    # print PRONOUNs in italic
	    if( $mention_type eq 'PRO') {
		$link_text = '<I>' . $mention_str . '</I>';
	    }

	    # If this mention introduced a cross-link to a new entity, add the cross-link,
	    # UNLESS the entire chain points to a single entity 
	    if( 
		exists $mentionLinkTable->{ "$ent_id $mention_id" } &&
		scalar @{$linkTable->{$ent_id}} > 1  # this is the total number of cross-linked ents
		){
		my $cross_ent_ids = $mentionLinkTable->{ "$ent_id $mention_id" } ;
		
		# split the list of cross-linked ids
		my @cross_ent_ids = split( ' ', $cross_ent_ids );

		print $FH '(';
		for my $cross_ent_id (@cross_ent_ids){
		    my $short_ceid = $cross_ent_id;
		    if( $cross_ent_id =~ /(\d+)$/ ) {
			$short_ceid = $1;
		    }
		    my $link = "<A TARGET=\"$target\"; HREF=\"$targetFile#$cross_ent_id\">"; 
		    print $FH  $link , "&gt;E$short_ceid", '</A> ';
		}
		print $FH ')&nbsp; ';
	    }

	    # print the actual mention
	    print $FH $link_start . $link_text . $link_end;

	}

	print $FH "&nbsp; ";

	# add the link to the crossing chain from another document, if linkTable is given
	if( $linkTable ){
	    if( exists $linkTable->{$entity_id} ){
		my $linkedChains = $linkTable->{$entity_id};
		for my $linked_ent_id ( @$linkedChains ){
		    my $short_leid = $linked_ent_id;
		    if( $linked_ent_id =~ /(\d+)$/ ) {
			$short_leid = $1;
		    }
		    my $link = "<A TARGET=\"$target\"; HREF=\"$targetFile#$linked_ent_id\">"; 
		    print $FH $link , "&gt;E$short_leid", '</A>&nbsp; ';
		} 
	    }
	}

	print $FH "<P>\n\n";
    }
}

# this code is duplicated in the package below! This is BAD!
sub cover_order {
    my( $x1, $y1 ) = split( ' ', $a );
    my( $x2, $y2 ) = split( ' ', $b );
	return  $x1 <=> $x2 unless $x1 == $x2;
    return  $y2 <=> $y1;
}

sub read_scorer_output {
    my $file = shift @_;
    my( $resp, $key );
    
    open FILE, $file;
    while( <FILE> ) {
	
	if( /^--------/ ) {
	    $resp = 0;
	    $key = 0;
	    #print "------------\n";
	}
	$key = $1 if /ref relation (\S+?) /;
	$resp = $1 if /tst relation (\S+?) /;
	
	$key_mismatch_cache{ $key } = $1 if /ref relation .*--.* (\S* ERRORS)/;
	$resp_mismatch_cache{ $resp } = $1 if /tst relation .*--.* (\S* ERRORS)/;
			    
	#print $key . "\n" if /ref relation (\S+?) /;
	#print $resp . "\n" if /tst relation (\S+?) /;

	if( $resp && $key ) {
	    $r2k_rel_cache{ $resp } = $key;
	    $k2r_rel_cache{ $key } = $resp;
	}
	last if /= event mapping =/;
    }
    
    while( <FILE> ) {
	if( /^--------/ ) {
	    $resp = 0;
	    $key = 0;
	    #print "------------\n";
	}
	
	$key = $1 if /ref event (\S+?) /;
	$resp = $1 if /tst event (\S+?) /;

	$key_mismatch_cache{ $key } = $1 if /ref event .*--.* (\S* ERRORS)/;
	$resp_mismatch_cache{ $resp } = $1 if /tst event .*--.* (\S* ERRORS)/;

	if( $resp && $key ) {
	    $r2k_eve_cache{ $resp } = $key;
	    $k2r_eve_cache{ $key } = $resp;
	}

    }
    
}

sub shorten_id {
    my $id = shift @_;
    my $short_id = $id;
    
    if( $id =~ /(\d+)$/ ) {
	$short_id = $1;
    }
    return $short_id;
}


#############################################################################
#
#  Access subroutines for elts of DOM tree
#

package APFDocument;

@APFDocument::ISA = qw( Exporter );

sub new {
    shift; # shift away the class name

    my( $infile, $source ) = @_;
    
    # get the key
    my $DOMkey = &XMLDOMWrapper::load( $infile ) || die "Cannot open $infile"; 

    # Index mentions of entities
    my @entities = ();
    my @extents  = ();
    my @ent_ids  = ();
    my @ent_types = ();
    my @ent_classes = ();
    my @ent_subtypes = ();
    
    my @relations = ();
    my @events = ();
    my @values = ();
        
    my $eve_list = $DOMkey->getElementsByTagName( 'event' );
    for( my $i=0; $i < $eve_list->getLength; $i++ ) {
	my $event = $eve_list->item($i);
	push @events, $event;
    }

    my $timex_list = $DOMkey->getElementsByTagName( 'timex2' );
    for (my $i = 0; $i < $timex_list->getLength; $i++ ) {
	my $timex = $timex_list->item($i);
	
	my $timex_mention = ${$timex->getElementsByTagName( 'timex2_mention' )}[0];
	my $id = $timex_mention->getAttribute( 'ID' );
	my $extent = ${$timex_mention->getElementsByTagName( 'extent' )}[0];
	my $charseq = ${$extent->getElementsByTagName( 'charseq' )}[0];
	my $start = $charseq->getAttribute( 'START' );
	my $end = $charseq->getAttribute( 'END' );
	
	if( $source eq 'k' ) {
	    
	    $key_value_start_cache{ $id } = $start;
	    $key_value_end_cache{ $id } = $end;
	} elsif ( $source eq 'r' ) {
	    
	    $resp_value_start_cache{ $id } = $start;
	    $resp_value_end_cache{ $id } = $end;
	} else {
	    die "source must be key or test";
	}

	push @values, $timex;
    }

    my $value_list = $DOMkey->getElementsByTagName( 'value' );
    for (my $i = 0; $i < $value_list->getLength; $i++ ) {
	my $value = $value_list->item($i);
	
	my $value_mention = ${$value->getElementsByTagName( 'value_mention' )}[0];
	my $id = $value_mention->getAttribute( 'ID' );
	my $extent = ${$value_mention->getElementsByTagName( 'extent' )}[0];
	my $charseq = ${$value_mention->getElementsByTagName( 'charseq' )}[0];
	my $start = $charseq->getAttribute( 'START' );
	my $end = $charseq->getAttribute( 'END' );
	
	if( $source eq 'k' ) {
	    
	    $key_value_start_cache{ $id } = $start;
	    $key_value_end_cache{ $id } = $end;
	} elsif ( $source eq 'r' ) {
	    
	    $resp_value_start_cache{ $id } = $start;
	    $resp_value_end_cache{ $id } = $end;
	} else {
	    die "source must be key or test";
	}

	push @values, $value;
    }

    

    my $ent_list = $DOMkey->getElementsByTagName( 'entity' );
    for( my $i=0; $i < $ent_list->getLength; $i++ ){
	my $entity = $ent_list->item($i);

        # SRS: filter out generics
        # next if &is_generic($entity);

        push @entities, $entity;
    
        my( @mention_extents ) = &get_all_extents( $entity, $source );
        my( $id )              = &get_ent_id( $entity );
        my( $type )            = &get_ent_type( $entity );  
	my( $class )           = &get_ent_class( $entity );
	my( $subtype )         = &get_ent_subtype( $entity );
        #DEBUG{ print "EXTENTS: " , join( ' ', @mention_extents ), "\n"; }

        push @extents, \@mention_extents;
        push @ent_ids, $id;
        push @ent_types, $type;
	push @ent_classes, $class;
	push @ent_subtypes, $subtype;
 
    }

    my $rel_list = $DOMkey->getElementsByTagName( 'relation' );
    for( my $i=0; $i < $rel_list->getLength; $i++ ) {
	my $relation = $rel_list->item($i);
	push @relations, $relation;
    }
    
    # create the object
    my $self = {};
    $self->{extents} = \@extents;
    $self->{entities} = \@entities;
    $self->{ent_ids}  = \@ent_ids;
    $self->{ent_types} = \@ent_types;
    $self->{ent_subtypes} = \@ent_subtypes;
    $self->{ent_classes} = \@ent_classes;
    
    $self->{relations} = \@relations;
    $self->{events} = \@events;
    $self->{values} = \@values;
         
    bless $self;

    return $self;
}

# Return all mention records (not sorted by group, i.e. flattened)
sub mentions 
{
    my $self = shift;

    # return it right away if cached
    return @{$self->{_mentions}} if exists $self->{_sorted_mentions};

    my @mentions = ();
    my @extents = $self->extents();

    for my $ext_list (@extents){
	for my $mention_record (@$ext_list){
	    my( $start, $end, $ent_id, $ent_type, $mention_type, $mention_id ) = 
		split( ' ', $mention_record );
	    push @mentions, "$start $end $ent_id $ent_type $mention_type $mention_id";
	}
    }
    $self->{_mentions} = \@mentions;
    return  @{$self->{_mentions}};
}

sub values {
    my $self = shift;
    return @{$self->{values}};
}

sub events {
    my $self = shift;
    return @{$self->{events}};
}

sub relations {
    my $self = shift;
    return @{$self->{relations}};
}

sub extents {
    my $self = shift;
    return @{$self->{extents}};
}

sub entities {
    my $self = shift;
    return @{$self->{entities}};
}

sub ent_ids {
    my $self = shift;
    return @{$self->{ent_ids}};
}

sub ent_types {
    my $self = shift;
    return @{$self->{ent_types}};
}

sub ent_subtypes {
    my $self = shift;
    return @{$self->{ent_subtypes}};
}

sub ent_classes {
    my $self = shift;
    return @{$self->{ent_classes}};
}

# Return the extent of an entity_mention. Include the type of mention with the extent.
sub get_extent 
{
    my( $mention ) = @_;
    die "entity_mention expected" unless $mention->getTagName eq 'entity_mention' ;
    
    my $mention_extent = ${$mention->getElementsByTagName( 'extent' )}[0];
    my $mention_charseq = ${$mention_extent->getElementsByTagName( 'charseq' )}[0];
    my $head = ${$mention->getElementsByTagName( 'head' )}[0];
    my $head_charseq = ${$head->getElementsByTagName( 'charseq' )}[0];

    my $start     = $mention_charseq->getAttribute( "START" );
    my $end       = $mention_charseq->getAttribute( "END" );
    my $head_start = $head_charseq->getAttribute( "START" );
    my $head_end   = $head_charseq->getAttribute( "END" );
    my $ment_type = $mention->getAttribute( 'TYPE' );

    # DEBUG{ print $start->text . ", " . $end->text . "\n"; }
   
    return ( $start, $end, $ment_type, $head_start, $head_end );
}
    
# Return the list of all extents of an entity. Extents are represented
# as pairs "$start $stop $ent_id $ent_type $mention_type".
sub get_all_extents 
{
    my( $entity, $source ) = @_;
    die "entity expected" unless $entity->getTagName eq 'entity' ;

    my $ent_type = &get_ent_type( $entity );
    my $ent_id   = &get_ent_id( $entity ); 

    my @extents = ();

    my $mention_list = $entity->getElementsByTagName( 'entity_mention' );
    for( my $i=0; $i < $mention_list->getLength; $i++ ){
	my $mention = $mention_list->item($i);
	
	my $id = $mention->getAttribute( "ID" );
		
	my ($start, $end, $ment_type, $h_start, $h_end) = &get_extent( $mention );
	if( $source eq 'k' ) {
	    $key_mention_start_cache{ $id } = $start;
	    $key_mention_end_cache{ $id } = $end;
	} elsif ( $source eq 'r' ) {
	    $resp_mention_start_cache{ $id } = $start;
	    $resp_mention_end_cache{ $id } = $end;
	} else {
	    die "source must be key or test";
	}
	
	push @extents, "$start $end $ent_id $ent_type $ment_type $id $h_start $h_end";
       
    }

    return @extents;
}

# retrun the entity's id
sub get_ent_id 
{
    my( $entity ) = @_;
    die "entity expected" unless $entity->getTagName eq 'entity' ;
    
    my( $full_id ) = $entity->getAttribute( "ID" );
    
    #my ( $id );
    #if( $full_id =~ /-E?(\d+)$/ ) {
#	$id = $1;
#    } else {
#	die "Could not get ID from $full_id\n";
#    }

    return $full_id;
    
}

# retrun the entity's type
sub get_ent_type 
{
    my( $entity ) = @_;
    die "entity expected" unless $entity->getTagName eq 'entity' ;
    
    return $entity->getAttribute( "TYPE" );
}

sub get_ent_subtype
{
    my( $entity ) = @_;
    die "entity expected" unless $entity->getTagName eq 'entity' ;
    
    return $entity->getAttribute( "SUBTYPE" );

}

sub get_ent_class
{
    my( $entity ) = @_;
    die "entity expected" unless $entity->getTagName eq 'entity' ;
    
    return $entity->getAttribute( "CLASS" );
}


sub get_rel_type 
{
    my( $relation ) = @_;
    die "relation expected" unless $relation->getTagName eq 'relation';
    
    return $relation->getAttribute( "TYPE" );
}

 sub get_rel_subtype
 {
     my( $relation ) = @_;
     die "relation expected" unless $relation->getTagName eq 'relation' ;
     return $relation->getAttribute( "SUBTYPE" );
 }

 sub get_rel_id
 {
     my( $relation ) = @_;
     die "relation expected" unless $relation->getTagName eq 'relation' ;
     my( $id ) = $relation->getAttribute( "ID" );
     return $id;
 }


 # SRS: figure out if an entity is generic or not
 sub is_generic
 {
     my ($entity) = @_;
     die "entity expected" unless $entity->getTagName eq 'entity' ;

     return $entity->getAttribute("CLASS") eq "GEN";
 }



# this code is duplicated! Look for it in a different package in this file!!
sub cover_order {
    my( $x1, $y1 ) = split( ' ', $a );
    my( $x2, $y2 ) = split( ' ', $b );
	return  $x1 <=> $x2 unless $x1 == $x2;
    return  $y2 <=> $y1;
}






=for none


    ### ---------- key mentions ---------------
    
    my $docmark = 'k';
    my $botfile = $key_botfile;
    my @mentions = $apfKey->mentions();
    
    for my $mention (@mentions){
	my( $start, $end, $id, $type,  $mention_type, $mention_id ) = split( ' ', $mention );
	
	# NOW with hypelinks
	my $ins_start_text =  '(' . substr( $type, 0, 3 ) . "-$id " ;
	my $ins_end_text   =    " $id)" ;  
	
	$inserts{ $start }   .= "<A NAME=\"$docmark-$mention_id\">" .
	    "<A TARGET=\"BOTK\" HREF=\"$botfile#$docmark-$mention_id\">" .
		$ins_start_text .
		    '</A><FONT color=blue>';
	$inserts{ $end + 1 } = "</FONT><A TARGET=\"BOTK\" HREF=\"$botfile#$docmark-$mention_id\">" .
	    $ins_end_text . 
		'</A>' .
		    $inserts{ $end + 1 } ;
    }
    
    ### ---------- response mentions ---------------

    # Make response's brackets show lowecase and in smaller font

    my $docmark = 'r';
    my $botfile = $resp_botfile;
    my @resp_mentions = $apfRes->mentions();
    
    for my $mention (@resp_mentions){
	my( $start, $end, $id, $type,  $mention_type, $mention_id ) = split( ' ', $mention );
	
	# NOW with hypelinks
	my $ins_start_text =  '&lt;' . lc(substr( $type, 0, 3 )) . "-<FONT SIZE=\"-1\">$id</FONT> " ;
	my $ins_end_text   =    " <FONT SIZE=\"-1\">$id</FONT>&gt;" ;  

	$inserts{ $start }   .= "<A NAME=\"$docmark-$mention_id\">" .
	                        "<A TARGET=\"BOTR\" HREF=\"$botfile#$docmark-$mention_id\">" .
	                        $ins_start_text .
	                        '</A><FONT color=blue>';
	$inserts{ $end + 1 } = "</FONT><A TARGET=\"BOTR\" HREF=\"$botfile#$docmark-$mention_id\">" .
	                        $ins_end_text . 
	                        '</A>' .
			        $inserts{ $end + 1 } ;
    }

    ### ------ end response mentions -------------------


=cut

