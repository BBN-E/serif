eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}' && eval 'exec perl -w -S $0 $argv:q'
  if 0;

# Copyright 2009 by BBN Technologies Corp.     All Rights Reserved.

use strict;
use XML::DOM;

die "Usage: $0 apf-dir apf-suffix entity-type-file entity-tag-file entity-subtype-file value-type-file value-tag-file relation-type-file event-trigger-file event-arg-file standard-types-file standard-subtypes-file"
    unless @ARGV == 12;

my $apf_dir = shift;
my $apf_suffix = shift;
my $entity_type_file = shift;
my $entity_tag_file = shift;
my $entity_subtype_file = shift;
my $value_type_file = shift;
my $value_tag_file = shift;
my $relation_type_file = shift;
my $event_trigger_file = shift;
my $event_arg_file = shift;
my $standard_types_file = shift;
my $standard_subtypes_file = shift;

my $parser = new XML::DOM::Parser

my %entity_types;
my %entity_subtypes;
my %value_types;
my %value_tags;
my %relation_types;
my %event_trigger_types;
my %event_arg_types;


## Load the standard set of entity types
open T, "<$standard_types_file" or die "Could not open $standard_types_file\n";
while (<T>) {
    next if /^\s*\#/ || /^\s*$/;
    /^\s*(\S+)\s*/;
    $entity_types{ $1 } = 1;
}
close T;

## Load the standard set of entity subtypes
open S, "<$standard_subtypes_file" or die "Could not open $standard_subtypes_file\n";
while (<S>) {
    next if /^\s*\#/ || /^\s*$/;
    /^\s*(\S+)\.(\S+)\s*/;
    $entity_subtypes{ $1 }{ $2 } = 1;
}
close S;


my @apf_files = glob("$apf_dir/*$apf_suffix");

foreach my $apf_file (@apf_files) 
{
    
    my $xml = $parser->parsefile( $apf_file ) or die "Could not parse $apf_file\n";
    
    my $ent_list = $xml->getElementsByTagName( "entity" );
    for( my $i = 0; $i < $ent_list->getLength; $i++ ) {
        my $ent = $ent_list->item( $i );
	my $type = $ent->getAttribute( "TYPE" );
	my $subtype = $ent->getAttribute( "SUBTYPE" );
	$subtype = "NONE" unless $subtype;
        $entity_types{ $type } = 1;
        $entity_subtypes{ $type }{ $subtype } += 1;
    }

    my $value_list = $xml->getElementsByTagName( "value" );
    for ( my $i = 0; $i < $value_list->getLength; $i++ ) {
        my $value = $value_list->item( $i );
        my $type = $value->getAttribute( "TYPE" );
        my $subtype = $value->getAttribute( "SUBTYPE" );
        if ($subtype ne "") {
            $value_types{ "$type.$subtype" } = 1;
            $value_tags{ $subtype } = 1;
        }
        else {
            $value_types{ $type } = 1;
            $value_tags{ $type } = 1;
        }
        $value_tags{ "TIMEX" } = 1;
    }

    my $timex_list = $xml->getElementsByTagName( "timex2" );
    if ( $timex_list->getLength > 0 ) {
        $value_types{ "TIMEX2 -nickname=TIMEX" } = 1;
        $value_types{ "TIMEX2.TIME -nickname=TIME" } = 1;
        $value_types{ "TIMEX2.DATE -nickname=DATE" } = 1;
    }

    my $rel_list = $xml->getElementsByTagName( "relation" );
    for ( my $i = 0; $i < $rel_list->getLength; $i++ ) {
        my $rel = $rel_list->item( $i );
        my $type = $rel->getAttribute( "TYPE" );
        my $subtype = $rel->getAttribute( "SUBTYPE" );
        $relation_types{ "$type.$subtype" } = 1;
    }

    my $event_list = $xml->getElementsByTagName( "event" );
    for ( my $i = 0; $i < $event_list->getLength; $i++ ) {
        my $event = $event_list->item( $i );
        my $type = $event->getAttribute( "TYPE" );
        my $subtype = $event->getAttribute( "SUBTYPE" );
        $event_trigger_types{ "$type.$subtype" } = 1;
    }
    
    my $arg_list = $xml->getElementsByTagName( "event_argument" );
    for ( my $j = 0; $j < $arg_list->getLength; $j++ ) {
        my $arg = $arg_list->item( $j );
        my $role = $arg->getAttribute( "ROLE" );
        $event_arg_types{ $role } = 1;
    }
}


open TAGS, ">$entity_tag_file" or die "Could not open $entity_tag_file\n";
open TYPES, ">$entity_type_file" or die "Could not open $entity_type_file\n";
my @entities = keys %entity_types;
my $n_entity_types = $#entities + 1;
print TAGS $n_entity_types . "\n";
foreach my $t (keys %entity_types) {
    print TAGS $t . "\n";
    print TYPES $t;
    if ($t =~ /^PER/ && $t !~ /PERCENT/) {
        print TYPES " -MatchesPER";
    } 
    if ($t =~ /^ORG/) {
        print TYPES " -MatchesORG";
    }
    if ($t =~ /^GPE/) {
        print TYPES " -MatchesGPE";
        print TYPES " -IsNat";
    }
    if ($t =~ /^FAC/) {
        print TYPES " -MatchesFAC";
    }
    if ($t =~ /^LOC/) {
        print TYPES " -MatchesLOC";
    }
    print TYPES "\n";
}
close TYPES;

open SUB, ">$entity_subtype_file" or die "Could not open $entity_subtype_file\n";
foreach my $t (keys %entity_subtypes) {
    my @sorted_subtypes = sort{$entity_subtypes{$t}{$b} <=> $entity_subtypes{$t}{$a}} (keys %{$entity_subtypes{$t}});
    my $first = shift @sorted_subtypes;
    print SUB $t . "." . $first . " DEFAULT\n";
    foreach my $s (@sorted_subtypes) {
        print SUB $t . "." . $s . "\n";
    }
}
close SUB;


open VAL, ">$value_type_file" or die "Could not open $value_type_file\n";
foreach my $v (keys %value_types) {
    print VAL $v;
    if ($v =~ /\.(\S*)$/) {
        print VAL " -nickname=" . $1;
    }
    print VAL "\n";
}
close VAL;


open VTAGS, ">$value_tag_file" or die "Could not open $value_tag_file\n";
my @values = keys %value_tags;
my $n_value_tags = $#values + 1;
print VTAGS $n_value_tags . "\n";
foreach my $v (@values) {
    print VTAGS $v . "\n";
}
close VTAGS;

open REL, ">$relation_type_file" or die "Could not open $relation_type_file\n";
my @relations = keys %relation_types;
my $n_relation_types = $#relations + 1;
print REL $n_relation_types . "\n";
foreach my $r (@relations) {
    print REL $r . "\n";
}
close TRIG;

open TRIG, ">$event_trigger_file" or die "Could not open $event_trigger_file\n";
my @triggers = keys %event_trigger_types;
my $n_trigger_types = $#triggers + 1;
print TRIG $n_trigger_types . "\n";
foreach my $t (@triggers) {
    print TRIG $t . "\n";
}
close TRIG;

open ARG, ">$event_arg_file" or die "Could not open $event_arg_file\n";
my @args = keys %event_arg_types;
my $n_arg_types = $#args + 1;
print ARG $n_arg_types . "\n";
foreach my $a (keys %event_arg_types) {
    print ARG $a . "\n";
}
close ARG;
