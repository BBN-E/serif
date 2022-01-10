eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}' && eval 'exec perl -w -S $0 $argv:q'
  if 0;

# Copyright 2008 by BBN Technologies Corp.
# All Rights Reserved.

use File::Basename;
use XML::DOM;

%entity_types = 
    ( 'PER' => 1,
      'ORG' => 1,
      'GPE' => 1,
      'LOC' => 1,
      'WEA' => 1,
      'VEH' => 1,
      'FAC' => 1 );

%value_types = 
    ( 'Numeric' => 1,
      'Sentence' => 1,
      'Job-Title' => 1,
      'Crime' => 1,
      'Contact-Info' => 1 );


$parser = new XML::DOM::Parser; 

die "Usage key-file-dir key-file-suffix source-file-dir source-file-suffix output-file" unless @ARGV == 5;

$input = shift;
$key_suffix = shift;
$source = shift;
$source_suffix = shift;
$output = shift;

$input =~ s/\\/\//g;
open OUT, ">$output";

print OUT "(\n";
@files = glob( "$input/*$key_suffix" );
foreach $file ( @files ) {
    next unless -f $file;
    $basename = basename( $file, ( $key_suffix ) );

    $source_file = "$source/$basename$source_suffix";
    
    $docname = "NOTFOUND";
    # get actual docname from source file
    open SOU, "$source_file" or die "Could not open $source_file";
    while ( <SOU> ) {
	if( /<DOCNO>(.*)<\/DOCNO>/ ) {
	    $docname = $1;
	    next;
	} elsif( /<DOCID>(.*)<\/DOCID>/ ) {
	    $docname = $1;
	    next;
	}
    }

    $docname =~ s/^\s+//;
    $docname =~ s/\s+$//;
    $docname =~ tr/ /_/;

    close SOU;
    die "Could not find docname for $file\n" unless $docname;
    
    print "Working on document $docname\n";

    print OUT "($docname\n";
    $xml = $parser->parsefile( $file ) or die "Could not parse $file\n";
    
    print OUT "  (Entities\n";
    $ent_list = $xml->getElementsByTagName( "entity" );
    for( $i = 0; $i < $ent_list->getLength; $i++ ) {
	$ent = $ent_list->item( $i );
	$type = $ent->getAttribute( "TYPE" );
	$subtype = $ent->getAttribute( "SUBTYPE" );
	$subtype = "NONE" unless $subtype;
	$class = $ent->getAttribute( "CLASS" );
	$class = "NONE" unless $class;
	
	$entity_id = $ent->getAttribute( "ID" );
	if ($type =~ /^(...).*/) {
	    $type = $1;
	}
	if ($type eq "GSP") {
	    $type = "GPE";
	}
	if( !$entity_types{ $type } && !$event_types{ $type } ) {
	    #print "Warning: Unknown Entity Type: $type\n";
	    next;
	}

	print OUT "    ($type $subtype $class FALSE $entity_id\n";
	
	$entity_mention_list = $ent->getElementsByTagName( "entity_mention" );
	for( $j = 0; $j < $entity_mention_list->getLength; $j++ ) {
	    $entity_mention = $entity_mention_list->item( $j );
	    $mention_type = $entity_mention->getAttribute( "TYPE" );
	    $mention_id = $entity_mention->getAttribute( "ID" );
	    $mention_role = $entity_mention->getAttribute( "ROLE" );
	    $mention_role = "NONE" unless $mention_role;
	    if ($mention_type =~ /^NAM/) {
		$mention_type = "NAME";
	    } elsif ($mention_type =~ /^NOM/) {
		if ($entity_mention->getAttribute( "LDCTYPE" ) eq "PRE") {
		    $mention_type = "NOM_PRE";
		} elsif ($entity_mention->getAttribute( "LDCTYPE" ) eq "NOMPRE") {
		    $mention_type = "NOM_PRE";
		} elsif ($mention_type eq "NOM_PRE") {
		    $mention_type = "NOM_PRE";
		} else {
		    $mention_type = "NOMINAL";
		}
	    } elsif ($mention_type =~ /^PRO/) {
		$mention_type = "PRONOUN";
	    } elsif ($mention_type =~ /^PRE/) {
		$mention_type = "PRE";
	    } else {
		$mention_type = "";
	    }
	    
	    unless ($mention_type) {
		#print "Warning missing valid mention type for mention $mention_id, assuming NOM\n";
		$mention_type = "NOM";
	    }

	    $extent_list = $entity_mention->getElementsByTagName( "extent" );
	    if ($extent_list->getLength != 1) {
		print "Bad extent: $mention_id\n";
		next;
	    }
	    $extent = $extent_list->item( 0 );
	    $charseq_list = $extent->getElementsByTagName( "charseq" );
	    die "Bad charseq" unless $charseq_list->getLength == 1;
	    $charseq = $charseq_list->item( 0 );
	    $start = $charseq->getAttribute( "START" );
	    $end = $charseq->getAttribute( "END" );

	    $head_list = $entity_mention->getElementsByTagName( "head" );
	    die "Bad head: ID = $mention_id" unless $head_list->getLength == 1;
	    $head = $head_list->item( 0 );
	    $charseq_list = $head->getElementsByTagName( "charseq" );
	    die "Bad charseq" unless $charseq_list->getLength == 1;
	    $charseq = $charseq_list->item( 0 );
	    $head_start = $charseq->getAttribute( "START" );
	    $head_end = $charseq->getAttribute( "END" );
	    
	    print OUT "      ($mention_type $mention_role $start $end $head_start $head_end $mention_id)\n";
	}
	# close individual entity
	print OUT "    )\n";
    }
    # close entities list
    print OUT "  )\n";

    print OUT "  (Values\n";
    $val_list = $xml->getElementsByTagName( "value" );
    for( $i = 0; $i < $val_list->getLength; $i++ ) {
	$val = $val_list->item( $i );
	$type = $val->getAttribute( "TYPE" );
	$subtype = $val->getAttribute( "SUBTYPE" );
	$subtype = "NONE" unless $subtype;
	
	$value_id = $val->getAttribute( "ID" );

	if( !$value_types{ $type } ) {
	    print "Warning: Unknown Value Type $type\n";
	    next;
	}

	if ($subtype ne "NONE") {
	    print OUT "    ($type.$subtype $value_id\n";
	}
	else {
	    print OUT "    ($type $value_id\n";
	}

	$value_mention_list = $val->getElementsByTagName( "value_mention" );
	for( $j = 0; $j < $value_mention_list->getLength; $j++ ) {
	    $value_mention = $value_mention_list->item( $j );
	    $mention_id = $value_mention->getAttribute( "ID" );

	    $extent_list = $value_mention->getElementsByTagName( "extent" );
	    if ($extent_list->getLength != 1) {
		print "Bad extent: $value_id\n";
		next;
	    }
	    $extent = $extent_list->item( 0 );
	    $charseq_list = $extent->getElementsByTagName( "charseq" );
	    die "Bad charseq" unless $charseq_list->getLength == 1;
	    $charseq = $charseq_list->item( 0 );
	    $start = $charseq->getAttribute( "START" );
	    $end = $charseq->getAttribute( "END" );

	    print OUT "      ($mention_id $start $end)\n";
	}
	# close individual value
	print OUT "    )\n";
    }
     ## check for quantities too (they may occur in Callisto annotation)
    $val_list = $xml->getElementsByTagName( "quantity" );
    for( $i = 0; $i < $val_list->getLength; $i++ ) {
	$val = $val_list->item( $i );
	$type = $val->getAttribute( "TYPE" );
	$subtype = $val->getAttribute( "SUBTYPE" );
	$subtype = "NONE" unless $subtype;
	
	$value_id = $val->getAttribute( "ID" );

	if( !$value_types{ $type } ) {
	    print "Warning: Unknown Value Type $type\n";
	    next;
	}

	if ($subtype ne "NONE") {
	    print OUT "    ($type.$subtype $value_id\n";
	}
	else {
	    print OUT "    ($type $value_id\n";
	}

	$value_mention_list = $val->getElementsByTagName( "quantity_mention" );
	for( $j = 0; $j < $value_mention_list->getLength; $j++ ) {
	    $value_mention = $value_mention_list->item( $j );
	    $mention_id = $value_mention->getAttribute( "ID" );

	    $extent_list = $value_mention->getElementsByTagName( "extent" );
	    if ($extent_list->getLength != 1) {
		print "Bad extent: $value_id\n";
		next;
	    }
	    $extent = $extent_list->item( 0 );
	    $charseq_list = $extent->getElementsByTagName( "charseq" );
	    die "Bad charseq" unless $charseq_list->getLength == 1;
	    $charseq = $charseq_list->item( 0 );
	    $start = $charseq->getAttribute( "START" );
	    $end = $charseq->getAttribute( "END" );

	    print OUT "      ($mention_id $start $end)\n";
	}
	# close individual value
	print OUT "    )\n";
    }
    $timex_list = $xml->getElementsByTagName( "timex2" );
    for( $i = 0; $i < $timex_list->getLength; $i++ ) {
	$timex = $timex_list->item( $i );
	$timex_val = $timex->getAttribute( "VAL" );
	$timex_anchor_val = $timex->getAttribute( "ANCHOR_VAL" );
	$timex_anchor_dir = $timex->getAttribute( "ANCHOR_DIR" );
	$timex_set = $timex->getAttribute( "SET" );
	$timex_mod = $timex->getAttribute( "MOD" );
	$timex_non_specific = $timex->getAttribute( "NON_SPECIFIC" );
	$timex_id = $timex->getAttribute( "ID" );
	$timex_id =~ tr/ /_/;

	$timex_val =~ tr/ /_/;
	$timex_anchor_val =~ tr/ /_/;
	$timex_anchor_dir =~ tr/ /_/;
	$timex_set =~ tr/ /_/;
	$timex_mod =~ tr/ /_/;
	$timex_non_specific =~ tr/ /_/;

	if (!$timex_val || $timex_val eq "") { $timex_val = "NULL"; }
	if (!$timex_anchor_val || $timex_anchor_val eq "") { $timex_anchor_val = "NULL"; }
	if (!$timex_anchor_dir || $timex_anchor_dir eq "") { $timex_anchor_dir = "NULL"; }
	if (!timex_set || $timex_set eq "") { $timex_set = "NULL"; }
	if (!timex_mod || $timex_mod eq "") { $timex_mod = "NULL"; }
	if (!timex_non_specific || $timex_non_specific eq "") { $timex_non_specific = "NULL"; }

	print OUT "    (TIMEX2 $timex_val $timex_anchor_val $timex_anchor_dir $timex_set $timex_mod $timex_non_specific $timex_id\n";
	
	$timex_mention_list = $timex->getElementsByTagName( "timex2_mention" );
	for( $j = 0; $j < $timex_mention_list->getLength; $j++ ) {
	    $timex_mention = $timex_mention_list->item( $j );
	    $mention_id = $timex_mention->getAttribute( "ID" );

	    $extent_list = $timex_mention->getElementsByTagName( "extent" );
	    if ($extent_list->getLength != 1) {
		print "Bad extent: $timex_id";
		next;
	    }
	    $extent = $extent_list->item( 0 );
	    $charseq_list = $extent->getElementsByTagName( "charseq" );
	    die "Bad charseq" unless $charseq_list->getLength == 1;
	    $charseq = $charseq_list->item( 0 );
	    $start = $charseq->getAttribute( "START" );
	    $end = $charseq->getAttribute( "END" );

	    print OUT "      ($mention_id $start $end)\n";
	}
	# close individual timex2 value
	print OUT "    )\n";
    }
    # close values list
    print OUT "  )\n";

    print OUT "  (Relations\n";

    $rel_list = $xml->getElementsByTagName( "relation" );
    for( $i = 0; $i < $rel_list->getLength; $i++ ) {
	$rel = $rel_list->item( $i );
	$id = $rel->getAttribute( "ID" );
	$type = $rel->getAttribute( "TYPE" );
	$subtype = $rel->getAttribute( "SUBTYPE" );

	## some day we should add these
	$tense = $rel->getAttribute( "TENSE" );
	$modality = $rel->getAttribute( "MODALITY" );

	if ($subtype ne "" && $subtype ne "NONE") {
	    print OUT "    ($type.$subtype EXPLICIT $id\n";
	}
	else {
	    print OUT "    ($type EXPLICIT $id\n";
	}

	$relation_mention_list = $rel->getElementsByTagName( "relation_mention" );
	for( $j = 0; $j < $relation_mention_list->getLength; $j++ ) {
	    $relation_mention = $relation_mention_list->item( $j );
	    $rel_mention_id = $relation_mention->getAttribute( "ID" );
	    $rel_arg_list = $relation_mention->getElementsByTagName( "rel_mention_arg" );
	    my $arg1;
	    my $arg2;
	    my $time_arg = "NULL";
	    my $time_role = "NULL";
	    for( $k = 0; $k < $rel_arg_list->getLength; $k++ ) {
		$arg = $rel_arg_list->item( $k );
		$arg_id = $arg->getAttribute( "ENTITYMENTIONID" );
		$argnum = $arg->getAttribute( "ARGNUM" );
		if ($argnum eq "1") {
		    $arg1 = $arg_id;
		} elsif ($argnum eq "2") {
		    $arg2 = $arg_id;
		} else {
		    $time_arg = $arg_id;
		    $time_role = $argnum;
		}
	    }
	    print OUT "      ($arg1 $arg2  $time_role $time_arg $rel_mention_id)\n";
	}
	print OUT "    )\n";
    }   

    # close relations list
    print OUT "  )\n";

    print OUT "  (Events\n";

    $event_list = $xml->getElementsByTagName( "event" );
    for( $i = 0; $i < $event_list->getLength; $i++ ) {
	$event = $event_list->item( $i );
	$id = $event->getAttribute( "ID" );
	$type = $event->getAttribute( "TYPE" );
	$subtype = $event->getAttribute( "SUBTYPE" );
	$modality = $event->getAttribute( "MODALITY" );
	$genericity = $event->getAttribute( "GENERICITY" );
	$polarity = $event->getAttribute( "POLARITY" );
	$tense = $event->getAttribute( "TENSE" );
	$modality = "Asserted" unless $modality;
	$genericity = "Specific" unless $genericity;
	$tense = "Unspecified" unless $tense;
	$polarity = "Positive" unless $polarity;
	
	print OUT "    ($type.$subtype $modality $genericity $tense $polarity $id\n";
	$event_mention_list = $event->getElementsByTagName( "event_mention" );
	for( $j = 0; $j < $event_mention_list->getLength; $j++ ) {
	    $event_mention = $event_mention_list->item( $j );
	    $event_mention_id = $event_mention->getAttribute( "ID" );
	    $event_extent_list = $event_mention->getElementsByTagName( "extent" );
	    if ($event_extent_list->getLength != 1) {
		print "Bad extent: $id\n";
		next;
	    }
	    $extent = $event_extent_list->item(0);
	    $charseq_list = $extent->getElementsByTagName( "charseq" );
	    die "Bad charseq" unless $charseq_list->getLength == 1;
	    $charseq = $charseq_list->item( 0 );
	    $extent_start = $charseq->getAttribute( "START" );
	    $extent_end = $charseq->getAttribute( "END" );

	    $event_anchor_list = $event_mention->getElementsByTagName( "anchor" );
	    die "Bad anchor: $event_mention_id" unless $event_anchor_list->getLength == 1;
	    $anchor = $event_anchor_list->item(0);
	    $charseq_list = $anchor->getElementsByTagName( "charseq" );
	    die "Bad charseq" unless $charseq_list->getLength == 1;
	    $charseq = $charseq_list->item( 0 );
	    $anchor_start = $charseq->getAttribute( "START" );
	    $anchor_end = $charseq->getAttribute( "END" );
	    
	    print OUT "      ($event_mention_id $anchor_start $anchor_end $extent_start $extent_end\n";
	    $event_arg_list = $event_mention->getElementsByTagName( "event_mention_argument" );
	    my $arg1;
	    my $arg2;
	    for( $k = 0; $k < $event_arg_list->getLength; $k++ ) {
		$arg = $event_arg_list->item( $k );
		$arg_id = $arg->getAttribute( "REFID" );
		$argrole = $arg->getAttribute( "ROLE" );
		print OUT "        ($arg_id $argrole)\n";	    
	    }
	    print OUT "      )\n";
	}
	print OUT "    )\n";
    }   

    # close events list
    print OUT "  )\n";

    # close document
    print OUT ")\n\n";
}

# close file
print OUT ")\n";
close OUT;
