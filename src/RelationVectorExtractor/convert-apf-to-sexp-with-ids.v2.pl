# Copyright 2008 by BBN Technologies Corp.
# All Rights Reserved.

use File::Basename;
use XML::DOM;

%entity_types = 
    ( 'PER' => 1,
      'ORG' => 1,
      'GPE' => 1,
      'LOC' => 1,
      'FAC' => 1 );

%relation_types =
    ( 'AT.BASED-IN' => 1,
      'AT.LOCATED' => 1,
      'AT.RESIDENCE' => 1,
      'NEAR.RELATIVE-LOCATION' => 1,
      'PART.AFFILIATE-PARTNER' => 1,
      'PART.MEMBER' => 1,
      'PART.OTHER' => 1,
      'PART.PART-OF' => 1,
      'PART.SUBSIDIARY' => 1,
      'ROLE.AFFILIATE-PARTNER' => 1,
      'ROLE.CITIZEN-OF' => 1,
      'ROLE.CLIENT' => 1,
      'ROLE.FOUNDER' => 1,
      'ROLE.GENERAL-STAFF' => 1,
      'ROLE.MANAGEMENT' => 1,
      'ROLE.MEMBER' => 1,
      'ROLE.OTHER' => 1,
      'ROLE.OWNER' => 1,
      'SOC.ASSOCIATE' => 1,
      'SOC.GRANDPARENT' => 1,
      'SOC.OTHER-PERSONAL' => 1,
      'SOC.OTHER-PROFESSIONAL' => 1,
      'SOC.OTHER-RELATIVE' => 1,
      'SOC.PARENT' => 1,
      'SOC.SIBLING' => 1,
      'SOC.SPOUSE' => 1 );
      
%event_types = 
    ( 'Accusing' => 1,
      'ArrestingSomeone' => 1,
      'ArrivingAtAPlace' => 1,
      'AttackOnTangible' => 1,
      'CoercingAnAgent' => 1,
      'Dying' => 1,
      'EmployeeHiring' => 1,
      'HarmingAnAgent' => 1,
      'KidnappingSomebody' => 1,
      'LeavingAPlace' => 1,
      'MakingAPromise' => 1,
      'MakingAnAgreement' => 1,
      'MonetaryExchangeOfUserRights' => 1,
      'Murder' => 1,
      'Paying' => 1 );
		

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
    
    # get actual docname from source file
    open SOU, "$source_file" or die "Could not open $source_file";
    while ( <SOU> ) {
	if( /<DOCNO>\s*(\S+)\s*<\/DOCNO>/ ) {
	    $docname = $1;
	    next;
	}
    }
    close SOU;
    die "Could not find docname for $file\n" unless $docname;
    
    print "Working on document $docname\n";

    print OUT "($docname\n";
    $xml = $parser->parsefile( $file ) or die "Could not parse $file\n";
    
    print OUT "  (Entities\n";
    $ent_list = $xml->getElementsByTagName( "entity" );
    for( $i = 0; $i < $ent_list->getLength; $i++ ) {
	$ent = $ent_list->item( $i );
	$entity_type_list = $ent->getElementsByTagName( "entity_type" );
	die "Bad entity_type" if $entity_type_list->getLength != 1;
	$entity_type = $entity_type_list->item( 0 );
	$generic = $entity_type->getAttribute( "GENERIC" );
	$generic = "FALSE" unless $generic;
	$group_fn = $entity_type->getAttribute( "GROUP_FN" );
	$group_fn = "FALSE" unless $group_fn;
	$type_element = $entity_type->getFirstChild();
	$entity_id = $ent->getAttribute( "ID" );
	unless( $type_element ) {
	    print "Warning: missing entity type for entity $entity_id, skipping.\n";
	    next;
	}
	$type = $type_element->getNodeValue();
	if( !$entity_types{ $type } && !$event_types{ $type } ) {
	    print "Warning: Unknown Entity Type $type\n";
	    next;
	}

#	next if $event_types{ $type };

	print OUT "    ($type $generic $group_fn $entity_id\n";
	
	$entity_mention_list = $ent->getElementsByTagName( "entity_mention" );
	for( $j = 0; $j < $entity_mention_list->getLength; $j++ ) {
	    $entity_mention = $entity_mention_list->item( $j );
	    $mention_type = $entity_mention->getAttribute( "TYPE" );
	    $mention_id = $entity_mention->getAttribute( "ID" );
	    unless ($mention_type) {
		print "Warning missing mention type for mention $mention_id, assuming Nominal\n";
		$mention_type = "Nominal";
	    }

	    $extent_list = $entity_mention->getElementsByTagName( "extent" );
	    die "Bad extent" unless $extent_list->getLength == 1;
	    $extent = $extent_list->item( 0 );
	    $start_list = $extent->getElementsByTagName( "start" );
	    die "Bad start" unless $start_list->getLength == 1;
	    $start = $start_list->item( 0 )->getFirstChild()->getNodeValue();
	    $end_list = $extent->getElementsByTagName( "end" );
	    die "Bad end" unless $end_list->getLength == 1;
	    $end = $end_list->item( 0 )->getFirstChild()->getNodeValue();

	    $head_list = $entity_mention->getElementsByTagName( "head" );
	    die "Bad head" unless $head_list->getLength == 1;
	    $head = $head_list->item( 0 );
	    $head_start_list = $head->getElementsByTagName( "start" );
	    die "Bad head start" unless $head_start_list->getLength == 1;
	    $head_start = $head_start_list->item( 0 )->getFirstChild()->getNodeValue();
	    $head_end_list = $head->getElementsByTagName( "end" );
	    die "Bad head head_end" unless $head_end_list->getLength == 1;
	    $head_end = $head_end_list->item( 0 )->getFirstChild()->getNodeValue();
	    
	    print OUT "      ($mention_type $start $end $head_start $head_end $mention_id)\n";
	}
	print OUT "    )\n";
    }
    print OUT "  )\n";
    print OUT "  (Relations\n";
    $rel_list = $xml->getElementsByTagName( "relation" );
    for( $i = 0; $i < $rel_list->getLength; $i++ ) {
	$rel = $rel_list->item( $i );
	$type = $rel->getAttribute( "TYPE" );
	$subtype = $rel->getAttribute( "SUBTYPE" );
	$class = $rel->getAttribute( "CLASS" );
	$relation_id = $rel->getAttribute( "ID" );
	$full_type = $type . "." . $subtype;
	if( !$relation_types{ $full_type } ) {
	    print "Warning: Unknown Relation Type $full_type\n";
	    next;
	}

	print OUT "    (${type}.${subtype} $class $relation_id\n";
	
	$relation_mentions_list = $rel->getElementsByTagName( "relation_mentions" );
	die "Bad Relation Mentions" if ($relation_mentions_list->getLength != 1);
	$relation_mentions = $relation_mentions_list->item( 0 );
	$relation_mention_list = $relation_mentions->getElementsByTagName( "relation_mention" );
	for( $j = 0; $j < $relation_mention_list->getLength; $j++ ) {
	    $relation_mention = $relation_mention_list->item( $j );
	    $mention_id = $relation_mention->getAttribute( "ID" );
	    $relation_mention_arg_list = $relation_mention->getElementsByTagName( "rel_mention_arg" );
	    unless ($relation_mention_arg_list->getLength == 2) {
		print "Warning number of relation mention args does not exactly equal 2\n";
            }
	    $arg1 = $relation_mention_arg_list->item( 0 );
	    $arg2 = $relation_mention_arg_list->item( 1 );
	    if ( $arg1 == 0 || $arg2 == 0 ) {
		print "Warning: Missing Mention Arg for $relation_id\n";
		next;
	    }
	    $arg1_num = $arg1->getAttribute( "ARGNUM" );
	    $arg2_num = $arg2->getAttribute( "ARGNUM" );
	    if ($arg1_num == 2 && $arg2_num == 1) {
		$temp = $arg1;
		$arg1 = $arg2;
		$arg2 = temp;
	    }
	    $arg1_id = $arg1->getAttribute( "MENTIONID" );
	    $arg2_id = $arg2->getAttribute( "MENTIONID" );

	    print OUT "      ($arg1_id $arg2_id $mention_id)\n";
	}
	print OUT "    )\n";
    }
    print OUT "  )\n";
    print OUT ")\n\n";
}

print OUT ")\n";
close OUT;
