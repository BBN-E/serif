#!/usr/bin/perl -w
use strict;
binmode (STDOUT, ":utf8");
binmode (STDERR, ":utf8");
#################################
# History:
#
# version 08
#    * bug fix in RMD and VMD evaluation
#
# version 07
#    * added language condition (Arabic, Chinese, English)
#    * improved robustness of diagnostic output
#
# version 06
#    * correct handling of unicode apf data
#
# version 05
#    * added minimum requirement for name similarity 
#      - names with less than the minimum similarity are judged as
#            non-matching and are assigned a similarity score of zero
#    * modified name normalization
#      - discard whitespace adjacent to apostrophes and dashes
#      - discard periods
#    * improved name score reporting in entity mapping diagnostic output
#
# version 04
#    * entity name entropy added as an evaluation condition
#
# version 03
#    * warning added to flag entities that are mentioned both literally and metonymically
#      using the same word(s).
#
# version 02
#    * improved name mapping
#    * additional parameter set:
#      - "global_2008_with_mentions" (the same as "global_2008" except that
#            mentions are used in mapping and valuation)
#
# version 01
#    * derives from ace07-eval-v06
#    * B-cubed scoring changed to match Bagga and Baldwin's algorithm
#      - as interpreted by BBN (Lance Ramshaw) at the 1997 ACE workshop
#      - with weighting of precision and recall according to mention type
#    * debugged for global (cross-document) entities and relations
#    * with three parameter sets:
#      - "local_2007" (the 2007 evaluation parameters)
#      - "local_2008" (the 2008 evaluation parameters for document-level evaluation)
#            The only difference between 2007 and 2008 is the use of mention-weighted
#            valuation of entities for mapping - followed by level-weighted valuation
#            for scoring.
#      - "global_2008" (the 2008 evaluation parameters for cross-document evaluation)
#            global_2008 parameters are the same as local_2007 parameters except for
#            the following differences:
#            - mentionless (document-level) mapping and valuation
#            - entity valuation includes the similarity between ref and sys names
#            - relation arguments must map in order to count
#
#################################

my %entity_attributes =
  (ID => {},
   CLASS => {GEN => 1, NEG => 1, SPC => 1, USP => 1},
   LEVEL => {},
   TYPE => {FAC => {Airport => 1, "Building-Grounds" => 1, Path => 1, Plant => 1, "Subarea-Facility" => 1},
	    GPE => {Continent => 1, "County-or-District" => 1, "GPE-Cluster" => 1, Nation => 1,
		    "Population-Center" => 1, Special => 1, "State-or-Province" => 1},
	    LOC => {Address => 1, Boundary => 1, Celestial => 1, "Land-Region-Natural" => 1,
		    "Region-General" => 1, "Region-International" => 1, "Water-Body" => 1},
	    ORG => {Commercial => 1, Educational => 1, Entertainment => 1, Government => 1, Media => 1,
		    "Medical-Science" => 1, "Non-Governmental" => 1, Religious => 1, Sports => 1},
	    PER => {Group => 1, Indeterminate => 1, Individual => 1},
	    VEH => {Air => 1, Land => 1, "Subarea-Vehicle" => 1, Underspecified => 1, Water => 1},
	    WEA => {Biological => 1, Blunt => 1, Chemical => 1, Exploding => 1, Nuclear => 1, Projectile => 1,
		    Sharp => 1, Shooting => 1, Underspecified => 1}});
foreach my $type (keys %{$entity_attributes{TYPE}}) {
  map {$entity_attributes{SUBTYPE}{$_} = 1} keys %{$entity_attributes{TYPE}{$type}};
}
my @entity_attributes = sort keys %entity_attributes;

my %value_attributes =
  (ID => {},
   TYPE => {"Contact-Info" => {"E-Mail" => 1, "Phone-Number" => 1, URL => 1},
	    Crime => {},
	    "Job-Title" => {},
	    Numeric => {Money => 1, Percent => 1},
	    Sentence => {}});
foreach my $type (keys %{$value_attributes{TYPE}}) {
  map {$value_attributes{SUBTYPE}{$_} = 1} keys %{$value_attributes{TYPE}{$type}};
}
my @value_attributes = sort keys %value_attributes;

my %timex2_attributes =
  (ID => {},
   ANCHOR_DIR => {AFTER => 1, AS_OF => 1, BEFORE => 1, ENDING => 1, STARTING => 1, WITHIN => 1},
   ANCHOR_VAL => {},
   COMMENT => {},
   MOD => {AFTER => 1, APPROX => 1, BEFORE => 1, END => 1, EQUAL_OR_LESS => 1, EQUAL_OR_MORE => 1,
	   LESS_THAN => 1, MID => 1, MORE_THAN => 1, ON_OR_AFTER => 1, ON_OR_BEFORE => 1, START => 1},
   NON_SPECIFIC => {YES => 1},
   SET => {YES => 1},
   SUBTYPE => {},
   TYPE => {},
   VAL => {});
my @timex2_attributes = sort keys %timex2_attributes;

my %relation_attributes =
  (ID => {},
   MODALITY => {Asserted => 1, Other => 1},
#   TENSE => {Future => 1, Past => 1, Present => 1, Unspecified => 1},
   TYPE => {ART => {"User-Owner-Inventor-Manufacturer" => 1},
	    "GEN-AFF" => {"Citizen-Resident-Religion-Ethnicity" => 1, "Org-Location" => 1},
	    METONYMY => {},
	    "ORG-AFF" => {Employment => 1, Founder => 1, "Investor-Shareholder" => 1, Membership => 1,
			  Ownership => 1, "Sports-Affiliation" => 1, "Student-Alum" => 1},
	    "PART-WHOLE" => {Artifact => 1, Geographical => 1, Subsidiary => 1},
	    "PER-SOC" => {Business => 1, Family => 1, "Lasting-Personal" => 1},
	    PHYS => {Located => 1, Near => 1}});
foreach my $type (keys %{$relation_attributes{TYPE}}) {
  map {$relation_attributes{SUBTYPE}{$_} = 1} keys %{$relation_attributes{TYPE}{$type}};
}
my @relation_attributes = sort keys %relation_attributes;

my %relation_argument_roles =
  ("Arg-1" => 1, "Arg-2" => 1, "Time-After" => 1, "Time-At-Beginning" => 1, "Time-At-End" => 1, "Time-Before" => 1,
   "Time-Ending" => 1, "Time-Holds" => 1, "Time-Starting" => 1, "Time-Within" => 1);

my %relation_symmetry =
  ("PER-SOC" => 1,
   METONYMY => 1,
   PHYS => 1);

my %event_attributes =
  (ID => {},
   GENERICITY => {Generic => 1, Specific => 1},
   MODALITY => {Asserted => 1, Other => 1},
   POLARITY => {Negative => 1, Positive => 1},
   TENSE => {Future => 1, Past => 1, Present => 1, Unspecified => 1},
   TYPE => {Business => {"Declare-Bankruptcy" => 1, "End-Org" => 1, "Merge-Org" => 1, "Start-Org" => 1},
	    Conflict => {Attack => 1, Demonstrate => 1},
	    Contact => {Meet => 1, "Phone-Write" => 1},
	    Justice => {Acquit => 1, Appeal => 1, "Arrest-Jail" => 1, "Charge-Indict" => 1, Convict => 1,
			Execute => 1, Extradite => 1, Fine => 1, Pardon => 1, "Release-Parole" => 1,
			Sentence => 1, Sue => 1, "Trial-Hearing" => 1},
	    Life => {"Be-Born" => 1, Date => 1, Die => 1, Divorce => 1, Injure => 1, Marry => 1},
	    Movement => {Transport => 1},
	    Personnel => {Elect => 1, "End-Position" => 1, Nominate => 1, "Start-Position" => 1},
	    Transaction => {"Transfer-Money" => 1, "Transfer-Ownership" => 1}});
foreach my $type (keys %{$event_attributes{TYPE}}) {
  map {$event_attributes{SUBTYPE}{$_} = 1} keys %{$event_attributes{TYPE}{$type}};
}
my @event_attributes = sort keys %event_attributes;

my %event_argument_roles = 
  (Adjudicator => 1, Agent => 1, Artifact => 1, Attacker => 1, Beneficiary => 1, Buyer => 1, Crime => 1,
   Defendant => 1, Destination => 1, Entity => 1, Giver => 1, Instrument => 1, Money => 1, Org => 1,
   Origin => 1, Person => 1, Place => 1, Plaintiff => 1, Position => 1, Price => 1, Prosecutor => 1,
   Recipient => 1, Seller => 1, Sentence => 1, Target => 1, "Time-After" => 1, "Time-At-Beginning" => 1,
   "Time-At-End" => 1, "Time-Before" => 1, "Time-Ending" => 1, "Time-Holds" => 1, "Time-Starting" => 1,
   "Time-Within" => 1, Vehicle => 1, Victim => 1);
my @event_argument_roles = sort keys %event_argument_roles;

my %event_mention_attributes =
  (ID => {},
   LEVEL => {SEN => 1});

#################################
# DEFAULT SCORING PARAMETERS:

use vars qw ($opt_R $opt_r $opt_T $opt_t $opt_P);
use vars qw ($opt_c $opt_m $opt_e $opt_a $opt_s $opt_d $opt_h);

my $map_weighting = "MENTION";     # available options are LEVEL, MENTION, and FMEASURE
my $eval_weighting = "LEVEL";      # available options are LEVEL and MENTION
my $mention_overlap_required = 0;  # determines whether mention overlap is required in order to allow relations/events to map
my $argscore_required = 1;         # determines how many non-null argument matches are required in order to allow relations/events to map - options are 0, 1, or ALL
my $nargs_required = 2;            # determines how many "Arg-*" relation arguments correspondences are required to map relations - options are 0, 1, or 2
my $allow_wrong_mapping = 1;       # determines whether relation/event arguments must map in order to contribute to the argument score
my $arg_valuation = "BOTH";        # determines how to (de)value arguments - options are ATTRIBUTES, MENTIONS, BOTH, or NONE
my $use_name_similarity = 0;       # determines whether name (de)similarity is used in computing entity scores
my $use_mentions = 1;              # determines whether mentions are used in computing 
my $score_non_named_entities = 1;  # determines whether to use NAM level document entities in mapping and scoring
my $allow_non_named_entities = 1;  # determines whether to allow non-NAM level document entities in mapping and scoring
my $score_relation_times = 0;      # determines whether time arguments of relations are scored or ignored

$opt_P = "local_2008";

my $epsilon = 1E-8;

#Entity scoring parameters
my %entity_type_wgt =
  (PER => 1.00,
   ORG => 1.00,
   LOC => 1.00,
   GPE => 1.00,
   FAC => 1.00,
   VEH => 1.00,
   WEA => 1.00);
my %entity_class_wgt = 
  (SPC => 1.00,
   GEN => $epsilon,
   NEG => $epsilon,
   USP => $epsilon);
my %entity_err_wgt = 
  (TYPE    => 0.50,
   SUBTYPE => 0.90,
   CLASS   => 0.75);
my $entity_fa_wgt = 0.75;
my %entity_mention_type_wgt =
  (NAM => 1.00,
   NOM => 0.50,
   PRO => 0.10);
my %entity_mention_err_wgt =
  (TYPE  => 0.90,
   ROLE  => 0.90,
   STYLE => 0.90);
my $entity_mention_coref_fa_wgt = 1.00;

#Relation scoring parameters
my %relation_type_wgt;
foreach my $type (keys %{$relation_attributes{TYPE}}) {
  $relation_type_wgt{$type} = 1.00;
}
my %relation_modality_wgt;
foreach my $mode (keys %{$relation_attributes{MODALITY}}) {
  $relation_modality_wgt{$mode} = 1.00;
}
$relation_modality_wgt{Unspecified} = 1.00;
my %relation_err_wgt =
  (TYPE       => 1.00,
   SUBTYPE    => 0.70,
   MODALITY   => 0.75,
   TENSE      => 1.00-$epsilon);
my $relation_fa_wgt = 0.75;
my $relation_argument_role_err_wgt = 0.75;
my $relation_asymmetry_err_wgt = 0.70;

#Event scoring parameters
my %event_type_wgt;
foreach my $type (keys %{$event_attributes{TYPE}}) {
  $event_type_wgt{$type} = 1.00;
}
my %event_modality_wgt;
foreach my $mode (keys %{$event_attributes{MODALITY}}) {
  $event_modality_wgt{$mode} = 1.00;
}
my %event_err_wgt =
  (TYPE       => 0.50,
   MODALITY   => 0.75,
   SUBTYPE    => 0.90,
   GENERICITY => 1.00-$epsilon,
   POLARITY   => 1.00-$epsilon,
   TENSE      => 1.00-$epsilon);
my $event_fa_wgt = 0.75;
my $event_argument_role_err_wgt = 0.75;

#Timex2 scoring parameters
my %timex2_attribute_wgt =
  (ANCHOR_DIR => 0.25,
   ANCHOR_VAL => 0.50,
   MOD        => 0.10,
   SET        => 0.10,
   TYPE       => 0.10,
   VAL        => 1.00);
my $timex2_fa_wgt = 0.75;
my $timex2_mention_coref_fa_wgt = 1.00;

#Value scoring parameters
my %value_type_wgt =
  ("Contact-Info" => 1.00,
   Crime          => 1.00,
   Illness        => 1.00,
   "Job-Title"    => 1.00,
   Numeric        => 1.00,
   Sentence       => 1.00);
my %value_err_wgt = 
  (TYPE    => 0.50,
   SUBTYPE => 0.90);
my $value_fa_wgt = 0.75;
my $value_mention_coref_fa_wgt = 1.00;

#################################
# SCORING PARAMETER SCHEMES

my %parameter_set =
  (local_2008 => {allow_wrong_mapping => 0},
   global_2008 => {use_mentions => 0,
		   use_name_similarity => 1,
		   allow_wrong_mapping => 0,
		   entity_err_wgt => {TYPE    => 0.50,
				      SUBTYPE => 0.90,
				      CLASS   => 0.75,
				      LEVEL   => 0.50}},
   global_2008_with_mentions => {
		   use_name_similarity => 1,
		   allow_wrong_mapping => 0,
		   entity_err_wgt => {TYPE    => 0.50,
				      SUBTYPE => 0.90,
				      CLASS   => 0.75,
				      LEVEL   => 0.50}},
   local_2007 => {map_weighting => "LEVEL"},
   );

#################################
# MENTION MAPPING PARAMETERS:

# min_overlap is the minimum mutual fractional overlap allowed
#                for a mention to be declared as successfully detected.
# min_text_match is the minimum fractional contiguous matching string length
#                for a mention to be declared as successfully recognized.
my $min_overlap = 0.3;
my $min_text_match = 0.0;

# max_diff is the maximum extent difference allowed mentions to be declared a "match".
# max_diff_chars is the maximum extent difference in characters for text sources.
# max_diff_time is the maximum extent difference in seconds for audio sources.
# max_diff_xy is the maximum extent difference in centimeters for image mentions.
my $max_diff_chars = 4;
my $max_diff_time = 0.4;
my $max_diff_xy = 0.4;

#################################
# NAME SCORING PARAMETERS:

my $min_name_similarity = 0.5;

#################################
# GLOBAL DATA

my (%ref_database, %tst_database, %eval_docs, %sys_docs, %mapped_refs);
my ($input_file, $input_doc, $input_element, $fatal_input_error_header);
my (%mention_detection_statistics, %ref_mention_scores, %tst_mention_scores);
my (%name_statistics);
my (%mapped_values, %mapped_document_values, %mention_map, %name_map, %argument_map);
my %mapped_document_costs;
my %arg_document_values;
my (%source_types, $source_type, $data_type);
my %unique_pages;
my %b3_data;
my %discarded_entities;

my (@entity_mention_types, @entity_types, @entity_classes);
my (@relation_types, @relation_modalities);
my (@event_types, @event_modalities, @value_types);

my @languages = ("Arabic", "Chinese", "English", "MIXED", "UNKNOWN");
my @error_types = ("correct", "miss", "fa", "error");
my @xdoc_types = ("1", ">1");
my @entropy_types = ("<=0.3", "0.3-0.6", "0.6-1.0", ">1.0");
my @entity_value_types = ("<=0.2", "0.2-0.5", "0.5-1.0", "1-2", "2-4", ">4");
my @relation_value_types = ("<=0.2", "0.2-0.5", "0.5-1.0", "1-2", "2-4", ">4");
my @event_value_types = ("<=0.2", "0.2-0.5", "0.5-1.0", "1-2", "2-4", ">4");
my @entity_style_types = ("LITERAL", "METONYMIC");
my @entity_mention_count_types = ("1", "2", "3-4", "5-8", ">8");
my @relation_mention_count_types = ("1", ">1");
my @event_mention_count_types = ("1", ">1");
my @timex2_mention_count_types = ("1", ">1");
my @value_mention_count_types = ("1", ">1");
my @relation_arg_count_types = ("0", "1", "2", "3", "4", ">4");
my @event_arg_count_types = ("0", "1", "2", "3", "4", ">4");
my @relation_arg_err_count_types = ("0", "1", ">1");
my @event_arg_err_count_types = ("0", "1", ">1");

my $max_string_length_to_print = 40;

my ($score_bound, $max_delta, $print_data);

my ($level_weighting, $use_Fmeasure);

my $usage = "\n\n$0 -R <ref_file> -T <tst_file>\n\n".
  "Description:  This Perl program evaluates ACE system performance.\n".
  "\n".
  "Required arguments:\n".
  "  -R <ref_file> or -r <ref_list>\n".
  "     <ref_file> is a file containing ACE reference data in apf format\n".
  "     <ref_list> is a file containing a list of files containing ACE\n".
  "         reference data in apf format\n".
  "  -T <tst_file> or -t <tst_list>\n".
  "     <tst_file> is a file containing ACE system output data in apf format\n".
  "     <ref_list> is a file containing a list of files containing ACE\n".
  "         system output data in apf format\n".
  "\n".
  "Optional arguments:\n".
  "  -P = <parameter_set_name>.  This controls the scoring mode by providing a selection\n".
  "         of different parameters (pre)defined in named parameter sets.\n".
  "  -c to check for duplicate elements - ref and sys data files must be the same\n".
  "         when performing this check.\n".
  "  -m to evaluate EMD, RMD and VMD\n".
  "  -e prints ref/sys comparisons for errorful data\n".
  "  -a prints ref/sys comparisons for all data\n".
  "  -s prints a summary of annotation data\n".
  "  -d prints document-level scores for all tasks\n".
  "  -h prints this help message\n".
  "\n";

#################################
# MAIN

{
  my ($date, $time) = date_time_stamp();
  print "command line (run on $date at $time):  ", $0, " ", join(" ", @ARGV), "\n";
  use Getopt::Std;
  getopts ('R:r:T:t:P:cmeasdh');
  die $usage if defined $opt_h;

  not $opt_c or ($opt_R and $opt_R eq $opt_T) or ($opt_r and $opt_r eq $opt_t) or die
    "\n\nPARAMETER ERROR:  ref and sys input must be the same when checking for duplicate elements (-c)$usage";

  $opt_R xor $opt_r or die
    "\n\nPARAMETER ERROR:  error in specifying data to process (one of -R or -r, but not both)$usage";
  $opt_T xor $opt_t or die
    "\n\nPARAMETER ERROR:  error in specifying data to process (one of -T or -t, but not both)$usage";

  select_parameter_set ();
  print_parameters ();

#read in the data
  my ($t0, $t1, $t2, $t3);
  $t0 = (times)[0];
  get_data (\%ref_database, \%eval_docs, $opt_R, $opt_r);
  check_metonymic_mentions (\%ref_database, "REF", \%eval_docs);
  get_data (\%tst_database, \%sys_docs, $opt_T, $opt_t);
  check_metonymic_mentions (\%tst_database, "SYS", \%sys_docs);
  check_docs ();
  print_documents ("REF", \%eval_docs) if $opt_s;
  print_documents ("TST", \%sys_docs) if $opt_s;
  foreach my $type ("entities", "values", "timex2s", "relations", "events") {
    $ref_database{"saved_$type"} = copy_structure ($ref_database{$type}) if $opt_m;
    $tst_database{"saved_$type"} = copy_structure ($tst_database{$type}) if $opt_m;
  }
  $t1 = (times)[0];
  printf STDERR "\ndata input:           %8.2f secs to load data\n", $t1-$t0;

#evaluate entities
  $t0 = (times)[0];
  $level_weighting = $map_weighting !~ /MENTION|FMEASURE/;
  $use_Fmeasure = $map_weighting =~ /FMEASURE/;
  compute_element_values ("entities");
  undef $use_Fmeasure;
  if ((keys %{$ref_database{entities}})>0) {
    $t1 = (times)[0];
    map_elements ($ref_database{entities}, $tst_database{entities}, \&map_doc_entities);
    $level_weighting = $eval_weighting =~ /LEVEL/;
    compute_element_values ("entities") if $map_weighting ne $eval_weighting;
    $t2 = (times)[0];
    print_entities ("REF", "entities", $ref_database{entities});
    if ((keys %{$tst_database{entities}})>0) {
      print_entities ("TST", "entities", $tst_database{entities});
      print_entity_mapping ("entity");
      print "\n======== entity scoring ========\n\nEntity Detection and Recognition statistics:\n";
      compute_b3 () if $use_mentions;
      score_entity_detection ();
      undef %b3_data;
      score_entity_attribute_recognition ();
      if ($use_mentions) {
	(my $detection_stats, my $role_stats, my $style_stats) = mention_recognition_stats ("entities");
	score_entity_mention_detection ($detection_stats);
	score_entity_mention_attribute_recognition ($role_stats, $style_stats);
      }
      document_level_scores ("entities") if $opt_d;
    }
    $t3 = (times)[0];
    printf STDERR "entity eval:          %8.2f secs to compute values,%8.2f secs to map,%8.2f secs to score\n", $t1-$t0, $t2-$t1, $t3-$t2;
  }

#evaluate value expressions
  $t0 = (times)[0];
  compute_element_values ("values");
  $t1 = (times)[0];
  if ((keys %{$ref_database{values}})>0) {
    map_elements ($ref_database{values}, $tst_database{values}, \&map_doc_entities);
    $t2 = (times)[0];
    print_values ("REF", "values", $ref_database{values}, \@value_attributes);
    if ((keys %{$tst_database{values}})>0) {
      print_values ("TST", "values", $tst_database{values}, \@value_attributes);
      print_value_mapping ($ref_database{values}, $tst_database{values}, "value", \@value_attributes);
      print "\n======== value scoring ========\n\nValue Detection and Recognition statistics:\n";
      score_value_detection ();
      my $attribute_stats = attribute_confusion_stats ("values", \@value_attributes);
      foreach my $attribute (@value_attributes) {
	next if $attribute eq "ID";
	score_confusion_stats ($attribute_stats->{$attribute}, "attribute $attribute");
      }
      document_level_scores ("values") if $opt_d;
    }
    $t3 = (times)[0];
    printf STDERR "value eval:           %8.2f secs to compute values,%8.2f secs to map,%8.2f secs to score\n", $t1-$t0, $t2-$t1, $t3-$t2;
  }

#evaluate timex2 expressions
  $t0 = (times)[0];
  compute_element_values ("timex2s");
  $t1 = (times)[0];
  if ((keys %{$ref_database{timex2s}})>0) {
    map_elements ($ref_database{timex2s}, $tst_database{timex2s}, \&map_doc_entities);
    $t2 = (times)[0];
    print_values ("REF", "timexs", $ref_database{timex2s}, \@timex2_attributes);
    if ((keys %{$tst_database{timex2s}})>0) {
      print_values ("TST", "timexs", $tst_database{timex2s}, \@timex2_attributes);
      print_value_mapping ($ref_database{timex2s}, $tst_database{timex2s}, "timex2", \@timex2_attributes);
      print "\n======== timex2 scoring ========\n\nTimex2 Detection and Recognition statistics:\n";
      score_timex2_detection ();
      my $attribute_stats = attribute_confusion_stats ("timex2s", \@timex2_attributes);
      foreach my $attribute (@timex2_attributes) {
	next if $attribute =~ /^(ID|TYPE)$/;
	score_confusion_stats ($attribute_stats->{$attribute}, "attribute $attribute");
      }
      document_level_scores ("timex2s") if $opt_d;
    }
    $t3 = (times)[0];
    printf STDERR "timex2 eval:          %8.2f secs to compute values,%8.2f secs to map,%8.2f secs to score\n", $t1-$t0, $t2-$t1, $t3-$t2;
  }

#evaluate relations
  $t0 = (times)[0];
  if ((keys %{$ref_database{relations}})>0 and (keys %{$tst_database{relations}})>0) {
    $use_Fmeasure = $map_weighting =~ /FMEASURE/;
    compute_releve_values ($ref_database{relations}, $tst_database{relations});
    undef $use_Fmeasure;
    $t1 = (times)[0];
    map_elements ($ref_database{relations}, $tst_database{relations}, \&map_releve_arguments);
    compute_releve_values ($ref_database{relations}, $tst_database{relations}) if $map_weighting =~ /FMEASURE/;
    $t2 = (times)[0];
    print_releves ("REF", "relations", $ref_database{relations}, \@relation_attributes);
    print_releves ("TST", "relations", $tst_database{relations}, \@relation_attributes);
    print_releve_mapping ($ref_database{relations}, $tst_database{relations}, "relation", \@relation_attributes);
    print "\n======== relation scoring ========\n\nRelation Detection and Recognition statistics:\n";
    score_relation_detection ();
    score_releve_attribute_recognition ("relations", \@relation_types, \%relation_attributes, \@relation_modalities);
    document_level_scores ("relations") if $opt_d;
    $t3 = (times)[0];
    printf STDERR "relation eval:        %8.2f secs to compute values,%8.2f secs to map,%8.2f secs to score\n", $t1-$t0, $t2-$t1, $t3-$t2;
  }

#evaluate events
  $t0 = (times)[0];
  if ((keys %{$ref_database{events}})>0 and (keys %{$tst_database{events}})>0) {
    $use_Fmeasure = $map_weighting =~ /FMEASURE/;
    compute_releve_values ($ref_database{events}, $tst_database{events});
    undef $use_Fmeasure;
    $t1 = (times)[0];
    map_elements ($ref_database{events}, $tst_database{events}, \&map_releve_arguments);
    compute_releve_values ($ref_database{events}, $tst_database{events}) if $map_weighting =~ /FMEASURE/;
    $t2 = (times)[0];
    print_releves ("REF", "events", $ref_database{events}, \@event_attributes);
    print_releves ("TST", "events", $tst_database{events}, \@event_attributes);
    print_releve_mapping ($ref_database{events}, $tst_database{events}, "event", \@event_attributes);
    print "\n======== event scoring ========\n\nEvent Detection and Recognition statistics:\n";
    score_event_detection ();
    score_releve_attribute_recognition ("events", \@event_types, \%event_attributes, \@event_modalities);
    document_level_scores ("events") if $opt_d;
    $t3 = (times)[0];
    printf STDERR "event eval:           %8.2f secs to compute values,%8.2f secs to map,%8.2f secs to score\n", $t1-$t0, $t2-$t1, $t3-$t2;
  }
  exit unless $opt_m and $use_mentions;

#evaluate entity mentions
  $t0 = (times)[0];
  if ((keys %{$ref_database{entities}})>0 and (keys %{$tst_database{entities}})>0) {
    undef %mapped_values, undef %mapped_document_values, undef %mention_map;
    for my $db (\%ref_database, \%tst_database) {
      promote_mentions ($db, "saved_entities", "mention_entities");
      replace_elements ($db, "mention_entities", "entities");
    }
    $level_weighting = $map_weighting !~ /MENTION|FMEASURE/;
    $use_Fmeasure = $map_weighting =~ /FMEASURE/;
    compute_element_values ("entities");
    undef $use_Fmeasure;
    $t1 = (times)[0];
    map_elements ($ref_database{entities}, $tst_database{entities}, \&map_doc_entities);
    $level_weighting = $eval_weighting =~ /LEVEL/;
    compute_element_values ("entities") if $map_weighting ne $eval_weighting;
    $t2 = (times)[0];
    print_entities ("REF", "mention_entities", $ref_database{entities});
    print_entities ("TST", "mention_entities", $tst_database{entities});
    print_entity_mapping ("mention_entity");
    print "\n======== mention_entity scoring ========\n\nEntity Mention Detection statistics:\n";
    score_entity_detection ();
    score_entity_attribute_recognition ();
    (my $detection_stats, my $role_stats, my $style_stats) = mention_recognition_stats ("entities");
    score_entity_mention_detection ($detection_stats);
    score_entity_mention_attribute_recognition ($role_stats, $style_stats);
    document_level_scores ("mention_entities") if $opt_d;
    $t3 = (times)[0];
    printf STDERR "entity mention eval:  %8.2f secs to compute values,%8.2f secs to map,%8.2f secs to score\n", $t1-$t0, $t2-$t1, $t3-$t2;
  }

#releve mention evaluation preparation
  $t0 = (times)[0];
  undef %mapped_values, undef %mapped_document_values, undef %mention_map;
  undef %arg_document_values, undef %argument_map;
  $allow_wrong_mapping = 1;
  if ((keys %{$ref_database{relations}})>0 and (keys %{$tst_database{relations}})>0 or
      (keys %{$ref_database{events}})   >0 and (keys %{$tst_database{events}})   >0) {
    replace_elements (\%ref_database, "saved_entities", "entities");
    replace_elements (\%tst_database, "mention_entities", "entities");
    $level_weighting = $eval_weighting =~ /LEVEL/;
    compute_element_values ("entities");
    foreach my $type ("values", "timex2s") {
      replace_elements (\%ref_database, "saved_$type", "$type");
      promote_mentions (\%tst_database, "saved_$type", "mention_$type");
      replace_elements (\%tst_database, "mention_$type", "$type");
      compute_element_values ($type);
    }
    $t1 = (times)[0];
    printf STDERR "mention preparation:  %8.2f secs to compute and map entity/value/timex2 data\n", $t1-$t0;
  }

#evaluate relation mentions
  $t0 = (times)[0];
  if ((keys %{$ref_database{relations}})>0 and (keys %{$tst_database{relations}})>0) {
    foreach my $db (\%ref_database, \%tst_database) {
      promote_mentions ($db, "saved_relations", "mention_relations", $db eq \%ref_database);
      replace_elements ($db, "mention_relations", "relations");
      check_arguments ("relations", $db);
    }
    $use_Fmeasure = $map_weighting =~ /FMEASURE/;
    compute_releve_values ($ref_database{relations}, $tst_database{relations});
    undef $use_Fmeasure;
    $t1 = (times)[0];
    map_elements ($ref_database{relations}, $tst_database{relations}, \&map_releve_arguments);
    compute_releve_values ($ref_database{relations}, $tst_database{relations}) if $map_weighting =~ /FMEASURE/;
    $t2 = (times)[0];
    print_releves ("REF", "mention_relations", $ref_database{relations}, \@relation_attributes);
    print_releves ("TST", "mention_relations", $tst_database{relations}, \@relation_attributes);
    print_releve_mapping ($ref_database{relations}, $tst_database{relations}, "mention_relation", \@relation_attributes);
    print "\n======== mention_relation scoring ========\n\nRelation Mention Detection and Recognition statistics:\n";
    score_relation_detection ();
    score_releve_attribute_recognition ("mention_relations", \@relation_types, \%relation_attributes, \@relation_modalities);
    document_level_scores ("mention_relations") if $opt_d;
    $t3 = (times)[0];
    printf STDERR "relation mention eval:%8.2f secs to compute values,%8.2f secs to map,%8.2f secs to score\n", $t1-$t0, $t2-$t1, $t3-$t2;
  }

#evaluate event mentions
  $t0 = (times)[0];
  if ((keys %{$ref_database{events}})>0 and (keys %{$tst_database{events}})>0) {
    foreach my $db (\%ref_database, \%tst_database) {
      promote_mentions ($db, "saved_events", "mention_events", $db eq \%ref_database);
      replace_elements ($db, "mention_events", "events");
      check_arguments ("events", $db);
    }
    $use_Fmeasure = $map_weighting =~ /FMEASURE/;
    compute_releve_values ($ref_database{events}, $tst_database{events});
    undef $use_Fmeasure;
    $t1 = (times)[0];
    map_elements ($ref_database{events}, $tst_database{events}, \&map_releve_arguments);
    compute_releve_values ($ref_database{events}, $tst_database{events}) if $map_weighting =~ /FMEASURE/;
    $t2 = (times)[0];
    print_releves ("REF", "mention_events", $ref_database{events}, \@event_attributes);
    print_releves ("TST", "mention_events", $tst_database{events}, \@event_attributes);
    print_releve_mapping ($ref_database{events}, $tst_database{events}, "mention_event", \@event_attributes);
    print "\n======== mention_event scoring ========\n\nEvent Mention Detection and Recognition statistics:\n";
    score_event_detection ();
    score_releve_attribute_recognition ("mention_events", \@event_types, \%event_attributes, \@event_modalities);
    document_level_scores ("mention_events") if $opt_d;
    $t3 = (times)[0];
    printf STDERR "event mention eval:   %8.2f secs to compute values,%8.2f secs to map,%8.2f secs to score\n", $t1-$t0, $t2-$t1, $t3-$t2;
  }
}

#################################

sub get_data {

  my ($db, $docs, $file, $list) = @_;

  open (LIST, $list) or die "\nUnable to open file list '$list'", $usage if $list;
  while ($input_file = $list ? <LIST> : $file) {
    $input_file =~ s/\n//;
    get_document_data ($db, $docs, $input_file);
    undef $file;
  }
  close (LIST);

#create reference pointers for all element types
  foreach my $type ("entity", "value", "timex2", "relation", "event",
		    "mention_entity", "mention_relation", "mention_event") {
    my $types = $type."s";
    $types =~ s/ys$/ies/;
    while (my ($id, $ref) = each %{$db->{$types}}) {
      $ref->{NAME_ENTROPY} = entity_name_entropy ($ref) if $type eq "entity";
      not defined $db->{refs}{$id} or die
	"\n\nFATAL INPUT ERROR:  duplicate ID ($id) ".
	"for elements of type '$types' and '$db->{refs}{$id}{ELEMENT_TYPE}'\n";
      $db->{refs}{$id} = $ref;
      foreach my $doc (keys %{$ref->{documents}}) {
	foreach my $mention (@{$ref->{documents}{$doc}{mentions}}) {
	  not defined $db->{refs}{$mention->{ID}} or die
	    "\n\nFATAL INPUT ERROR:  duplicate ID ($mention->{ID}) ".
	    "for mentions of elements of type '$type' ".
	    "and '$db->{refs}{$mention->{ID}}{ELEMENT_TYPE}'\n";
	  $db->{refs}{$mention->{ID}} = $mention;
	  $mention->{ELEMENT_TYPE} = "$type mention";
	}
      }
    }
  }

  label_element_language ($db);
  check_arguments ("relations", $db);
  check_arguments ("events", $db);
}

#################################

sub check_metonymic_mentions {

  my ($db, $source, $docs) = @_;

  foreach my $entity (values %{$db->{entities}}) {
    my %names;
    foreach my $doc_entity (values %{$entity->{documents}}) {
      foreach my $name (@{$doc_entity->{names}}) {
	$names{$name}++;
      }
    }
    while ( my ($doc, $doc_entity) = each %{$entity->{documents}}) {
      foreach my $mention (@{$doc_entity->{mentions}}) {
	next if $mention->{STYLE} eq "LITERAL";
	my $name = normalize_name($mention->{head}{locator}{text});
	next if !$names{$name};
	warn "\nWARNING:  metonymic mention ($name) in $source document '$doc' is the same as a name for entity '$entity->{ID}'\n";
	delete $names{$name};
      }
    }
  }
}

#################################

sub label_element_language {

  my ($db) = @_;

  my $lang;
  foreach my $type ("entities", "values", "timex2s", "relations", "events") {
    foreach my $element (values %{$db->{$type}}) {
      while (my ($doc, $doc_element) = each %{$element->{documents}}) {
	foreach my $mention (@{$doc_element->{mentions}}) {
	  my $lang = text_language ($mention->{head} ? $mention->{head}{text} : $mention->{extent}{text});
	  $doc_element->{LANGUAGE} = $lang if not $doc_element->{LANGUAGE};
	  $doc_element->{LANGUAGE} = "MIXED" if $lang ne $doc_element->{LANGUAGE};
	}
	foreach my $name (@{$doc_element->{names}}) {
	  my $lang = text_language ($name);
	  $doc_element->{LANGUAGE} = $lang if not $doc_element->{LANGUAGE};
	  $doc_element->{LANGUAGE} = "MIXED" if $lang ne $doc_element->{LANGUAGE};
	}
	foreach my $arg (values %{$doc_element->{arguments}}) {
	  foreach my $id (keys %$arg) {
	    my $lang = $db->{refs}{$id}{documents}{$doc}{LANGUAGE};
	    $doc_element->{LANGUAGE} = $lang if not $doc_element->{LANGUAGE};
	    $doc_element->{LANGUAGE} = "MIXED" if $lang ne $doc_element->{LANGUAGE};
	  }
	}
      }
    }
  }
}

#################################

sub text_language {

  ($_) = @_;
  my $arabic = /\p{Arabic}/;
  my $chinese = /\p{Han}/;
  my $english = /\p{Latin}/;
  my $language = ($arabic ? ($chinese || $english ? "MIXED" : "Arabic") :
		  $chinese ? ($english ? "MIXED" : "Chinese") :
		  $english ? "English" : "UNKNOWN");
  return $language
}

#################################

sub check_arguments {

  my ($elements, $db) = @_;

  my $type;
  while (my ($id, $element) = each %{$db->{$elements}}) {
    $type = $element->{ELEMENT_TYPE};
    my %valid_arg_mention_ids;
    while (my ($role, $arg_ids) = each %{$element->{arguments}}) {
      foreach my $arg_id (keys %$arg_ids) {
	defined $db->{refs}{$arg_id} or die
	  "\n\nFATAL INPUT ERROR:  $type '$id' references argument '$arg_id' in role '$role'\n".
	  "    but this argument has not been loaded\n";
	foreach my $mention_id (element_mention_ids ($db, $arg_id)) {
	  $valid_arg_mention_ids{$role}{$mention_id} = 1;
	}
      }
    }
    while (my ($doc, $doc_element) = each %{$element->{documents}}) {
      foreach my $mention (@{$doc_element->{mentions}}) {
	while (my ($role, $arg_ids) = each %{$mention->{arguments}}) {
	  foreach my $arg_id (keys %$arg_ids) {
	    defined $db->{refs}{$arg_id} or die
	      "\n\nFATAL INPUT ERROR:  $type mention '$mention->{ID}' references argument mention '$arg_id' in role '$role'\n".
	      "    but this argument mention has not been loaded\n";
	    $valid_arg_mention_ids{$role}{$arg_id} or die
	      "\n\nFATAL INPUT ERROR:  $type mention '$mention->{ID}' references argument mention '$arg_id' in role '$role'\n".
	      "    but this argument mention is not a mention of any argument of $type '$id' in this role\n";
	  }
	}
      }
    }
  }
}

#################################

sub element_mention_ids {

  my ($db, $id) = @_;

  my @mention_ids;
  while (my ($doc, $doc_element) = each %{$db->{refs}{$id}{documents}}) {
    foreach my $mention (@{$doc_element->{mentions}}) {
      push @mention_ids, $mention->{ID};
    }
  }
  return @mention_ids;
}

#################################

sub set_params {
  my ($prms, $data) = @_;

  my ($ptype, $dtype) = (ref $prms, ref $data);
  $ptype eq $dtype or die
    "\n\nFATAL ERROR:  data type ($dtype) not equal to parameter type ($ptype) in call to set_params\n"
    if $data;
  if ($ptype eq "SCALAR") {
    $prms = $data if $data;
    return;
  } elsif ($ptype eq "ARRAY") {
    @$prms = @$data if $data;
    return;
  } elsif ($ptype eq "HASH") {
    while ((my $key, my $value) = each %$data) {
      $prms->{$key} = $value if defined $value;
    }
    my @sorted_names = sort {$prms->{$b} <=> $prms->{$a} ? $prms->{$b} <=> $prms->{$a} : $a cmp $b;} keys %$prms;
    return @sorted_names;
  }
}

#################################

sub select_parameter_set {

  $opt_P = "" unless $opt_P;
  my $parms = $parameter_set{$opt_P};
  if (not $parms) {
    print STDERR "\n\nPARAMETER ERROR:  An unknown parameter set '$opt_P' specified (-P), The available parameter sets are:";
    foreach my $name (sort keys %parameter_set) {
      printf STDERR "\n    %s", $name;
    }
    die $usage;
  }

  my $p;
#Mapping parameters
  $min_overlap = $p if defined ($p = $parms->{min_overlap});
  $min_text_match = $p if defined ($p = $parms->{min_text_match});
  $max_diff_chars = $p if defined ($p = $parms->{max_diff_chars});
  $max_diff_time = $p if defined ($p = $parms->{max_diff_time});
  $max_diff_xy = $p if defined ($p = $parms->{max_diff_xy});
  $use_mentions = $p if defined ($p = $parms->{use_mentions});
  $score_non_named_entities = $p if defined ($p = $parms->{score_non_named_entities});
  $allow_non_named_entities = $p if defined ($p = $parms->{allow_non_named_entities});
  $score_relation_times = $p if defined ($p = $parms->{score_relation_times});
  $use_name_similarity = $p if defined ($p = $parms->{use_name_similarity});
  $min_name_similarity = $p if defined ($p = $parms->{min_name_similarity});
  $map_weighting = $p if defined ($p = $parms->{map_weighting});
  $map_weighting =~ /^(LEVEL|MENTION|FMEASURE)$/ or die
    "\n\nPARAMETER ERROR:  '$map_weighting' is an unknown entity mapping weighting option.  (The parameter is 'map_weighting')".
        "                  The evailable options are LEVEL, MENTION, and FMEASURE$usage";
  $eval_weighting = $p if defined ($p = $parms->{eval_weighting});
  $eval_weighting =~ /^(LEVEL|MENTION)$/ or die
    "\n\nPARAMETER ERROR:  '$eval_weighting' is an unknown entity evaluation weighting option.  (The parameter is 'eval_weighting')".
        "                  The evailable options are LEVEL and MENTION$usage";
  $mention_overlap_required = $p if defined ($p = $parms->{mention_overlap_required});
  $argscore_required = $p if defined ($p = $parms->{argscore_required});
  $argscore_required =~ /^(0|1|ALL)$/ or die
    "\n\nPARAMETER ERROR:  '$argscore_required' is an unknown argument scoring requirement option.  (The parameter is 'argscore_required')".
        "                  The evailable options are 0, 1 and ALL$usage";
  $nargs_required = $p if defined ($p = $parms->{nargs_required});
  $nargs_required =~ /^[012]$/ or die
    "\n\nPARAMETER ERROR:  '$nargs_required' is an illegal number of required relation Arg-* arguments.  (The parameter is 'nargs_required')".
    "                  The evailable options are 0, 1 or 2$usage";
  $allow_wrong_mapping = $p if defined ($p = $parms->{allow_wrong_mapping});
  $arg_valuation = $p if defined ($p = $parms->{arg_valuation});
  $arg_valuation =~ /^(ATTRIBUTES|MENTIONS|BOTH|NONE)$/ or die
    "\n\nPARAMETER ERROR:  '$arg_valuation' is an unknown argument valuation option.  (The parameter is 'arg_valuation')".
        "                  The evailable options are ATTRIBUTES, MENTIONS, BOTH, and NONE$usage";

#Entity parameters
  @entity_mention_types = set_params (\%entity_mention_type_wgt, $parms->{entity_mention_type_wgt});
  @entity_types = set_params (\%entity_type_wgt, $parms->{entity_type_wgt});
  @entity_classes = set_params (\%entity_class_wgt, $parms->{entity_class_wgt});
  set_params (\%entity_err_wgt, $parms->{entity_err_wgt});
  set_params (\%entity_mention_err_wgt, $parms->{entity_mention_err_wgt});
  $entity_fa_wgt = $p if defined ($p = $parms->{entity_fa_wgt});
  $entity_mention_coref_fa_wgt = $p if defined ($p = $parms->{entity_mention_coref_fa_wgt});

#Relation parameters
  @relation_types = set_params (\%relation_type_wgt, $parms->{relation_type_wgt});
  @relation_modalities = set_params (\%relation_modality_wgt, $parms->{relation_modality_wgt});
  set_params (\%relation_err_wgt, $parms->{relation_err_wgt});
  $relation_fa_wgt = $p if defined ($p = $parms->{relation_fa_wgt});
  $relation_argument_role_err_wgt = $p if defined ($p = $parms->{relation_argument_role_err_wgt});
  $relation_asymmetry_err_wgt = $p if defined ($p = $parms->{relation_asymmetry_err_wgt});

#Event parameters
  @event_types = set_params (\%event_type_wgt, $parms->{event_type_wgt});
  @event_modalities = set_params (\%event_modality_wgt, $parms->{event_modality_wgt});
  set_params (\%event_err_wgt, $parms->{event_err_wgt});
  $event_fa_wgt = $p if defined ($p = $parms->{event_fa_wgt});
  $event_argument_role_err_wgt = $p if defined ($p = $parms->{event_argument_role_err_wgt});

#Timex2 parameters
  set_params (\%timex2_attribute_wgt, $parms->{timex2_attribute_wgt});
  $timex2_fa_wgt = $p if defined ($p = $parms->{timex2_fa_wgt});
  $timex2_mention_coref_fa_wgt = $p if defined ($p = $parms->{timex2_mention_coref_fa_wgt});

#Value parameters
  @value_types = set_params (\%value_type_wgt, $parms->{value_type_wgt});
  set_params (\%value_err_wgt, $parms->{value_err_wgt});
  $value_fa_wgt = $p if defined ($p = $parms->{value_fa_wgt});
  $value_mention_coref_fa_wgt = $p if defined ($p = $parms->{value_mention_coref_fa_wgt});
}

#################################

sub print_parameters {

  printf "\nThe scoring mode parameter set is \"$opt_P\":\n".
    " Scoring %s mentions for mapping and to compute value\n".
    " %s document entities are allowed\n".
    " %s document entities are mapped and scored\n".
    " Entity mapping is $map_weighting weighted\n".
    " Entity scoring is $eval_weighting weighted\n".
    " Entity scoring %s name (dis)similarity to penalize entity values%s\n".
    " Time arguments of relations are %s\n".
    " Relation/Event mapping %s overlap of Relation/Event mentions\n".
    " Relation/Event mapping %s have a non-null argument score\n".
    " Relation mapping requires that %s Arg-* relation arguments have a non-null argument score\n".
    " Relation/Event arguments %s required to map correctly in order to contribute to the argument score\n".
    "\n".
    "  min acceptable overlap of matching mention heads or names:\n".
    "%11.1f percent\n".
    "  min acceptable run of matching characters in mention heads:\n".
    "%11.1f percent\n".
    "  max acceptable extent difference for names and mentions to match:\n".
    "%11d chars for text sources\n".
    "%11.3f sec for audio sources\n".
    "%11.3f cm for image sources\n",
    $use_mentions ? "USES" : "DOES NOT use",
    $allow_non_named_entities ? "All" : "Only NAM-level",
    $score_non_named_entities ? "All" : "Only NAM-level",
    $use_name_similarity ? ("USES", "\n   (minimum acceptable similarity is $min_name_similarity)") : ("DOES NOT use", ""),
    $score_relation_times ? "SCORED" : "NOT scored",
    $mention_overlap_required ? "REQUIRES" : "DOES NOT require",
    $argscore_required == 0 ? "does NOT require that arguments" :
    $argscore_required == 1 ? "requires that AT LEAST ONE argument" : "requires that ALL arguments",
    $nargs_required == 2 ? "BOTH" : "at least $nargs_required",
    $allow_wrong_mapping ? "are NOT" : "ARE",
    100*$min_overlap, 100*$min_text_match, $max_diff_chars, $max_diff_time, $max_diff_xy;

#Entity parameters
  print "\n";
  print "  Entity mention values:\n";
  foreach my $type (@entity_mention_types) {
    printf "%11.3f for type %s\n", $entity_mention_type_wgt{$type}, $type;
  }
  print "  Entity value weights for entity types:\n";
  foreach my $type (@entity_types) {
    printf "%11.3f for type %s\n", $entity_type_wgt{$type}, $type;
  }
  print "  Entity value weights for entity classes:\n";
  foreach my $class (@entity_classes) {
    printf "%11.3f for class %s\n", $entity_class_wgt{$class}, $class;
  }
  print "  Entity value discounts for entity attribute recognition errors:\n";
  foreach my $type (sort keys %entity_err_wgt) {
    printf "%11.3f for $type errors\n", $entity_err_wgt{$type};
  }
  print "  Entity mention value discounts for mention attribute recognition errors:\n";
  foreach my $type (sort keys %entity_mention_err_wgt) {
    printf "%11.3f for $type errors\n", $entity_mention_err_wgt{$type};
  }
  printf "  Entity mention value (cost) for spurious entity mentions:%6.3f\n", $entity_fa_wgt;
  printf "  Entity mention value (cost) discount for incorrect coreference:%6.3f\n", $entity_mention_coref_fa_wgt;

#Relation parameters
  print "\n";
  print "  Relation value weights for relation types:\n";
  foreach my $type (@relation_types) {
    printf "%11.3f for type %s\n", $relation_type_wgt{$type}, $type;
  }
  print "  Relation value weights for relation modalities:\n";
  foreach my $type (@relation_modalities) {
    printf "%11.3f for modality %s\n", $relation_modality_wgt{$type}, $type;
  }
  print "  Relation value discounts for relation attribute recognition errors:\n";
  foreach my $type (sort keys %relation_err_wgt) {
    printf "%11.3f for $type errors\n", $relation_err_wgt{$type};
  }
  printf "  Relation argument value (cost) for spurious relation arguments:%6.3f\n", $relation_fa_wgt;
  printf "  Relation argument value (cost) discount for argument role errors:%6.3f\n", $relation_argument_role_err_wgt;
  printf "  Relation argument value (cost) discount for Arg-[12] asymmetry errors:%6.3f\n", $relation_asymmetry_err_wgt;

#Event parameters
  print "\n";
  print "  Event value weights for event types:\n";
  foreach my $type (@event_types) {
    printf "%11.3f for type %s\n", $event_type_wgt{$type}, $type;
  }
  print "  Event value weights for event modalities:\n";
  foreach my $type (@event_modalities) {
    printf "%11.3f for modality %s\n", $event_modality_wgt{$type}, $type;
  }
  print "  Event value discounts for event attribute recognition errors:\n";
  foreach my $type (sort keys %event_err_wgt) {
    printf "%11.3f for $type errors\n", $event_err_wgt{$type};
  }
  printf "  Event argument value (cost) for spurious event arguments:%6.3f\n", $event_fa_wgt;
  printf "  Event argument value (cost) discount for argument role errors:%6.3f\n", $event_argument_role_err_wgt;

#Timex2 parameters
  print "\n";
  print "  Timex2 attribute value weights for timex2 attributes:\n";
  foreach my $type (sort keys %timex2_attribute_wgt) {
    printf "%11.3f for type %s\n", $timex2_attribute_wgt{$type}, $type;
  }
  printf "  Timex2 mention value (cost) for spurious timex2 mentions:%6.3f\n", $timex2_fa_wgt;
  printf "  Timex2 mention value (cost) discount for incorrect coreference:%6.3f\n", $timex2_mention_coref_fa_wgt;

#Value parameters
  print "\n";
  print "  Value value weights for value types:\n";
  foreach my $type (@value_types) {
    printf "%11.3f for type %s\n", $value_type_wgt{$type}, $type;
  }
  print "  Value value discounts for value attribute recognition errors:\n";
  foreach my $type (sort keys %value_err_wgt) {
    printf "%11.3f for $type errors\n", $value_err_wgt{$type};
  }
  printf "  Value mention value (cost) for spurious value mentions:%6.3f\n", $value_fa_wgt;
  printf "  Value mention value (cost) discount for incorrect coreference:%6.3f\n", $value_mention_coref_fa_wgt;
}

#################################

sub check_docs {

  my ($doc_id, $eval_doc, $sys_doc);

  foreach $doc_id (keys %eval_docs) {
    $eval_doc = $eval_docs{$doc_id};
    $sys_doc = $sys_docs{$doc_id};
    $sys_doc or warn
      "\nWARNING:  ref doc '$doc_id' has no corresponding tst doc\n";
    next unless $sys_doc;
    $sys_doc->{TYPE} eq $eval_doc->{TYPE} or die
      "\n\nFATAL ERROR:  different reference and system output data types for document '$doc_id'\n".
	"    ref type is '$eval_doc->{TYPE}' but system output type is '$sys_doc->{TYPE}'\n";
  }
}


#################################

sub check_for_duplicates {

  my ($elements) = @_;

  my @ids = sort keys %$elements;
  while (my $id1 = shift @ids) {
    next unless $mapped_values{$id1};
    foreach my $id2 (sort keys %{$mapped_values{$id1}}) {
      next if $id2 le $id1 or $mapped_values{$id1}{$id2} ne $elements->{$id1}{VALUE};
      warn "\nWARNING:  equivalent $elements->{$id1}{ELEMENT_TYPE} elements: $id1 and $id2\n";
    }
  }
}

#################################

sub replace_elements {
#This subroutine deletes all elements in the database of type "$srcname"
#and replaces them with copies of all elements in the database of type "$dstname".
#The database list of references is updated to account for this replacement.
  my ($db, $srcname, $dstname) = @_;

  my $dbsrc = $db->{$srcname};
  my $dbdst = $db->{$dstname};
  foreach my $element (values %$dbdst) {
    delete $db->{refs}{$element->{ID}};
    foreach my $doc_element (values %{$dbdst->{$element->{ID}}{documents}}) {
      foreach my $mention (@{$doc_element->{mentions}}) {
	delete $db->{refs}{$mention->{ID}};
      }
    }
  }

  $dbdst = $db->{$dstname} = {};
  foreach my $id (keys %$dbsrc) {
    not defined $db->{refs}{$id} or die
      "\n\nFATAL ERROR:  duplicate ID ($id) in replace_elements\n";
    $db->{refs}{$id} = $dbdst->{$id} = copy_structure ($dbsrc->{$id});
    foreach my $doc_element (values %{$dbsrc->{$id}{documents}}) {
      foreach my $mention (@{$doc_element->{mentions}}) {
	not defined $db->{refs}{$mention->{ID}} or die
	  "\n\nFATAL ERROR:  duplicate mention ID ($mention->{ID}) in replace_elements\n";
	$db->{refs}{$mention->{ID}} = $mention;
      }
      foreach my $arg_ids (values %{$doc_element->{arguments}}) {
	foreach my $arg_id (keys %$arg_ids) {
	  defined $db->{refs}{$arg_id} or die
	    "\n\nFATAL ERROR:  undefined argument ID ($arg_id) in replace_elements\n";
	}
      }
    }
  }
}

#################################

sub copy_structure {

  my ($element) = @_;

  my $ref = ref $element;
  return $element if not $ref;
  return [map copy_structure($_), @$element] if $ref eq "ARRAY";
  die "\n\nFATAL ERROR in copy_structure - can't copy element of type '$ref'\n" if $ref ne "HASH";
  my %hash;
  while (my ($key, $value) = each %$element) {
    $hash{$key} = copy_structure ($value) if $key ne "MAP";
  }
  return \%hash;
}

#################################

sub score_entity_detection {

  conditional_performance ("entities", "source", "language", "LANGUAGE", \@languages);
  conditional_performance ("entities", "entity", "type", "TYPE", \@entity_types);
  conditional_performance ("entities", "entity", "level", "LEVEL", \@entity_mention_types);
  conditional_performance ("entities", "entity", "value", "VALUE", \@entity_value_types);
  conditional_performance ("entities", "mention", "count", "MENTION COUNT", \@entity_mention_count_types);
  conditional_performance ("entities", "entity", "class", "CLASS", \@entity_classes);
  my @source_types = sort keys %source_types;
  conditional_performance ("entities", "document", "source", "SOURCE", \@source_types);
  conditional_performance ("entities", "entity", "num-docs", "CROSS-DOC", \@xdoc_types);
  conditional_performance ("entities", "entity", "name-ent", "NAME_ENTROPY", \@entropy_types);
  conditional_performance ("entities", "entity", "type", "TYPE", \@entity_types, "LEVEL", \@entity_mention_types);
  conditional_performance ("entities", "entity", "num-docs", "CROSS-DOC", \@xdoc_types, "TYPE", \@entity_types);
#  conditional_performance ("entities", "entity", "type", "TYPE", \@entity_types, undef, undef, 1);
#  conditional_performance ("entities", "entity", "value", "VALUE", \@entity_value_types, undef, undef, 1);
  my @subtypes = sort keys %{$entity_attributes{SUBTYPE}};
  conditional_performance ("entities", "entity", "subtype", "SUBTYPE", \@subtypes, "TYPE", \@entity_types);
}

#################################

sub document_level_scores {

  my ($type) = @_;
  (my $elements, my $task) = ($type =~ /entities/ ? ("entities", "EDR")  :
			      ($type =~ /values/ ? ("values", "VAL") :
			       ($type =~ /timex2s/ ? ("timex2s", "TIM") :
				($type =~ /relations/ ? ("relations", "RDR") :
				 ($type =~ /events/ ? ("events", "VDR") :
				  die "unknown element type ($type) in document_level_scores")))));
  $task =~ s/DR/MD/ if $type =~ /mention/i;
  (my $count, my $cost, my $nrm_cost, my $doc_costs) = conditional_error_stats ($elements, "SOURCE");
  print "\n";
  foreach my $doc (sort keys %$doc_costs) {
    my $ref_value = defined $doc_costs->{$doc}{REF} ? $doc_costs->{$doc}{REF} : 0;
    my $sys_value = $ref_value - (defined $doc_costs->{$doc}{SYS} ? $doc_costs->{$doc}{SYS} : 0);
    my $score = $ref_value ? $sys_value/$ref_value : ($sys_value ? -99.9999 : 1);
    printf "%10.2f %% (%.5f out of %.5f) $task score for %s\n", 100*max($score,-99.9999), $sys_value, $ref_value, $doc;
  }
}

#################################

sub conditional_performance {

  my ($elements, $label1, $label2, $cond1, $c1s, $cond2, $c2s, $external_reconciliation) = @_;

  my ($count, $cost, $nrm_cost) = conditional_error_stats ($elements, $cond1, $cond2, $external_reconciliation);
  my $b3_scores = conditional_b3_stats (\%b3_data, $cond1, $cond2, $external_reconciliation) if $elements eq "entities";

  my $args_mentions = $elements =~ /relations|events/ ? "__Arguments___" : "___Mentions___";
  my $count_weight  = $b3_scores ? "B3 Unweighted"  : " Unweighted  ";
  my $value_weights = $b3_scores ? "B3 Value_based" : " Value-based  ";
  my $elements_label = ($elements =~ /entit/ ? "_Entity_Count__" :
			$elements =~ /relat/ ? "Relation_Count_" :
			$elements =~ /event/ ? "__Event_Count__" :
			$elements =~ /timex/ ? "_Timex2_Count__" :
			$elements =~ /value/ ? "__Value_Count__" :
			die "unknown elements in conditional_performance");
  my $hdr1 = "$elements_label     ___Document_Count____     _____Document_Count_(%)______        ________________________Cost_(%)________________________       ____________Unconditioned_Cost_(%)____________";
  my $hdr2 = "Ref   Detection     Ref   Detection   Rec     Detection   Rec   $count_weight      Detection  Attr    $args_mentions   Value   $value_weights     Max      Detection   Attr     _$args_mentions\_";
  my $hdr3 = "Tot    FA  Miss     Tot    FA  Miss   Err      FA  Miss   Err    Pre--Rec--F        FA  Miss   Err    FA  Miss   Err     (%)    Pre--Rec--F      Value     FA   Miss    Err     FA   Miss    Err";

  if ($cond2) {
    foreach my $cond ("ALL", @$c2s) {
      next unless $count and $count->{doc}{$cond};
      print "\nEvaluation of externally reconciled elements:" if $external_reconciliation;
      print "\nPerformance statistics for $cond2 = $cond:";
      printf "\n %-8s %s\n %-8s %s\n          %s\n", $label1, $hdr1, uc $label2, $hdr2, $hdr3;
      foreach my $type (@$c1s, "(multi)", "total") {
	print_eval ($type, $count->{element}{$cond}{$type}, $count->{doc}{$cond}{$type}, $cost->{$cond}{$type}, $nrm_cost->{$cond}{$type}, $nrm_cost->{ALL}{total}, $b3_scores ? $b3_scores->{$cond}{$type} : undef);
      }
    }
  } else {
    return unless %$count;
    print "\nEvaluation of externally reconciled elements:" if $external_reconciliation;
    printf "\n %-8s %s\n %-8s %s\n          %s\n", $label1, $hdr1, uc $label2, $hdr2, $hdr3;
    foreach my $type (@$c1s, "(multi)", "total") {
      print_eval ($type, $count->{element}{$type}, $count->{doc}{$type}, $cost->{$type}, $nrm_cost->{$type}, $nrm_cost->{total}, $b3_scores ? $b3_scores->{$type} : undef);
    }
  }
}

#################################

sub print_eval {

  my ($type, $count, $doc_count, $cost, $ref_value, $total_value, $b3_scores) = @_;

  return unless (defined $count->{correct} or defined $doc_count->{correct} or
		 defined $count->{error}   or defined $doc_count->{error}   or
		 defined $count->{miss}    or defined $doc_count->{miss}    or
		 defined $count->{fa}      or defined $doc_count->{fa}      or
		 defined $count->{mapped}  or defined $doc_count->{mapped});
  my $format = "%7.7s%6d%6d%6d%8d%6d%6d%6d%8.1f%6.1f%6.1f%7.1f%5.1f%5.1f%8.1f%6.1f%6.1f%6.1f%6.1f%6.1f%8.1f%7.1f%5.1f%5.1f%9.2f%7.2f%7.2f%7.2f%7.2f%7.2f%7.2f\n";

  foreach my $category ("correct", "error", "miss", "fa", "mapped", "ATTR_ERR", "FA_SUB", "MISS_SUB", "ERR_SUB") {
    $count->{$category} = 0 unless defined $count->{$category};
    $doc_count->{$category} = 0 unless defined $doc_count->{$category};
    $cost->{$category} = 0 unless defined $cost->{$category};
  }
  $ref_value = 0 unless defined $ref_value;
  my $nref = $count->{mapped}+$count->{miss};
  my $nref_doc = $doc_count->{correct}+$doc_count->{error}+$doc_count->{miss};
  my $nsys = $doc_count->{correct}+$doc_count->{error}+$doc_count->{fa};
  my $pn = 100/max(1E-30, $nref_doc);
  my $cn = 100/max(1E-30, $ref_value);
  my ($recall, $precision) = $b3_scores ? ($b3_scores->{COUNT}{recall}, $b3_scores->{COUNT}{precision}) :
      ($doc_count->{correct}/max($nref_doc,1E-30),
       $doc_count->{correct}/max($nsys,1E-30));
  my $fmeasure = 2*$precision*$recall/max($precision+$recall, 1E-30);

  my $value_correct = $ref_value-$cost->{miss}-$cost->{error}-$cost->{correct};
  my $sys_value = $ref_value-$cost->{miss}+$cost->{fa};
  my ($value_recall, $value_precision) = $b3_scores ? ($b3_scores->{VALUE}{recall}, $b3_scores->{VALUE}{precision}) :
      (max($value_correct,0)/max(1E-30, $ref_value), max($value_correct,0)/max(1E-30, $sys_value));
  my $value_fmeasure = 2*$value_precision*$value_recall/max($value_precision+$value_recall, 1E-30);

  my $un = 100/max($total_value,1E-30);
  printf $format, $type, $nref, $count->{fa}, $count->{miss}, $nref_doc, $doc_count->{fa}, $doc_count->{miss}, $doc_count->{error},
  min(999.9,$pn*$doc_count->{fa}), $pn*$doc_count->{miss}, $pn*$doc_count->{error}, 100*$precision, 100*$recall, 100*$fmeasure,
  min(999.9,$cn*$cost->{fa}), $cn*$cost->{miss},
  min(999.9,$cn*$cost->{ATTR_ERR}),
  min(999.9,$cn*$cost->{FA_SUB}),
  min(999.9,$cn*$cost->{MISS_SUB}),
  min(999.9,$cn*$cost->{ERR_SUB}),
  max(-999.9,$cn*($value_correct-$cost->{fa})), 100*$value_precision, 100*$value_recall, 100*$value_fmeasure,
  $un*$ref_value, min(999.99,$un*$cost->{fa}), $un*$cost->{miss},
  $un*$cost->{ATTR_ERR},
  $un*$cost->{FA_SUB},
  $un*$cost->{MISS_SUB},
  $un*$cost->{ERR_SUB};
}

#################################

sub score_relation_detection {

  conditional_performance ("relations", "source", "language", "LANGUAGE", \@languages);
  conditional_performance ("relations", "relation", "type", "TYPE", \@relation_types);
  conditional_performance ("relations", "modality", "type", "MODALITY", \@relation_modalities);
  conditional_performance ("relations", "relation", "value", "VALUE", \@relation_value_types);
#  conditional_performance ("relations", "mention", "count", "MENTION COUNT", \@relation_mention_count_types);
  my @source_types = sort keys %source_types;
  conditional_performance ("relations", "source", "type", "SOURCE", \@source_types);
  conditional_performance ("relations", "argument", "errors", "ARGUMENT ERRORS", \@relation_arg_err_count_types);
#  conditional_performance ("relations", "relation", "type", "TYPE", \@relation_types, "ARGUMENT ERRORS", \@relation_arg_err_count_types);
  conditional_performance ("relations", "num", "args", "NUM ARGUMENTS", \@relation_arg_count_types);
  my @subtypes = sort keys %{$relation_attributes{SUBTYPE}};
  conditional_performance ("relations", "relation", "subtype", "SUBTYPE", \@subtypes, "TYPE", \@relation_types);
}

#################################

sub score_event_detection {

  conditional_performance ("events", "source", "language", "LANGUAGE", \@languages);
  conditional_performance ("events", "event", "type", "TYPE", \@event_types);
  conditional_performance ("events", "modality", "type", "MODALITY", \@event_modalities);
  conditional_performance ("events", "event", "value", "VALUE", \@event_value_types);
#  conditional_performance ("events", "mention", "count", "MENTION COUNT", \@event_mention_count_types);
  my @source_types = sort keys %source_types;
  conditional_performance ("events", "source", "type", "SOURCE", \@source_types);
  conditional_performance ("events", "argument", "errors", "ARGUMENT ERRORS", \@event_arg_err_count_types);
#  conditional_performance ("events", "event", "type", "TYPE", \@event_types, "ARGUMENT ERRORS", \@event_arg_err_count_types);
  conditional_performance ("events", "num", "args", "NUM ARGUMENTS", \@event_arg_count_types);
  my @subtypes = sort keys %{$event_attributes{SUBTYPE}};
  conditional_performance ("events", "event", "subtype", "SUBTYPE", \@subtypes, "TYPE", \@event_types);
}

#################################

sub score_timex2_detection {

  my @types = sort keys %source_types;
  conditional_performance ("timex2s", "source", "language", "LANGUAGE", \@languages);
  conditional_performance ("timex2s", "source", "type", "SOURCE", \@types);
}

#################################

sub score_value_detection {

  my @types = sort keys %source_types;
  conditional_performance ("values", "source", "language", "LANGUAGE", \@languages);
  conditional_performance ("values", "source", "type", "SOURCE", \@types);
}

#################################

sub score_confusion_stats {

  my ($stats, $label) = @_;
  my $maxdisplay = 10;

#display dominant confusion statistics
  return unless $stats;
  my (%ref_count, %tst_count, %sort_count, %correct_count);
  my $ntot = my $nref = my $ncor = my $nfa = my $nmiss = 0;
#select attribute values that contribute the most confusions
  while (my ($ref_value, $tst_stats) = each %$stats) {
    while (my ($tst_value, $count) = each %$tst_stats) {
      $ref_count{$ref_value} += $count;
      $tst_count{$tst_value} += $count;
      $ntot += $count;
      $nref += $count unless $ref_value eq "<undef>";
      if ($tst_value eq $ref_value) {
	$correct_count{$ref_value} += $count;;
	$sort_count{$ref_value} -= $epsilon*$count;
	$ncor += $count unless $ref_value eq "<undef>";
      } else {
	$sort_count{$tst_value} += $count;
	$sort_count{$ref_value} += $count;
	$nfa += $count if $ref_value eq "<undef>";
	$nmiss += $count if $tst_value eq "<undef>";
      }
    }
  }
  my @display_values = sort {$sort_count{$b} <=> $sort_count{$a}} keys %sort_count;
  my $ndisplay = min($maxdisplay, scalar @display_values);
  splice (@display_values, $ndisplay);

  #tabulate confusion statistics for "other" attribute values
  my $others = "all others";
  foreach my $value (@display_values) {
    $stats->{$value}{$others} = $ref_count{$value};
    $stats->{$others}{$value} = $tst_count{$value};
  }
  $stats->{$others}{$others} = $ntot;
  my $display_count = 0;
  foreach my $ref_value (@display_values) {
    foreach my $tst_value (@display_values) {
      my $count = $stats->{$ref_value}{$tst_value};
      next unless $count;
      $stats->{$ref_value}{$others} -= $count;
      $stats->{$others}{$tst_value} -= $count;
      $stats->{$others}{$others} -= $count;
      $display_count += $count;
    }
  }

  #output results
  my $nerr = $nfa+$nref-$ncor;
  my $nsub = $nref-$nmiss-$ncor;
  my $nsys = $ncor+$nsub+$nfa;
  return unless $nref or $nsys;
  print "\nRecognition statistics for $label (for mapped elements):\n";
  $nref = max($nref,$epsilon);
  $nsys = max($nsys,$epsilon);
  my $pfa = $nfa/$nref;
  my $psub = $nsub/$nref;
  my $pmiss = $nmiss/$nref;
  my $perror = $nerr/$nref;
  my $recall = ($ncor+$nsub)/$nref;
  my $precision = ($ncor+$nsub)/$nsys;
  my $f = 2*$precision*$recall/max($precision+$recall,$epsilon);
  printf "    Summary (count/percent):  Nref=%d/%.1f%%, Nfa=%d/%.1f%%, Nmiss=%d/%.1f%%, Nsub=%d/%.1f%%, Nerr=%d/%.1f%%"
    .", P/R/F=%.1f%%/%.1f%%/%.1f%%\n",
    $nref, 100, $nfa, min(999.9,100*$pfa), $nmiss, 100*$pmiss, $nsub, min(999.9,100*$psub), $nerr, min(999.9,100*$perror),
    100*$precision, 100*$recall, 100*$f;
  print "    Confusion matrix for major error contributors (count/percent):\n        ref\\tst:";
  push @display_values, $others if $display_count != $ntot;
  foreach my $tst_value ("(recall)", @display_values) {
    printf "%11.11s ", $tst_value;
  }
  print "\n";
  printf "  %14.14s%s", "(precision)", " "x12;
  foreach my $tst_value (@display_values) { #precision
    my $count = $correct_count{$tst_value} ? $correct_count{$tst_value} : 0;
    $tst_value =~ /^<undef>$/ ? print "     N/A    " :
      $tst_count{$tst_value} ? (printf "%6d/%4.1f%%", $count, min(99.9,100*$count/$tst_count{$tst_value})) :
      print "      -     ";
  }    
  print "\n";
  foreach my $ref_value (@display_values) {
    printf "  %14.14s", $ref_value;
    my $count = $correct_count{$ref_value} ? $correct_count{$ref_value} : 0;
    $ref_value =~ /^<undef>$/ ? print "     N/A    " :
      $ref_count{$ref_value} ? (printf "%6d/%4.1f%%", $count, min(99.9,100*$count/$ref_count{$ref_value})) :
      print "      -     ";
    foreach my $tst_value (@display_values) {
      my $count = $stats->{$ref_value}{$tst_value};
      printf "%s", $count ? 
	(sprintf "%6d/%4.1f%%", $count, min(99.9,100*$count/max($nref,$epsilon))) :
	"      -     ";
    }
    print "\n";
  }
}

#################################

sub score_entity_attribute_recognition {

#type attributes
  my ($type, $ref_type, %type_stats);
  my (%entity_type_total);
  my $attribute_stats = attribute_confusion_stats ("entities", \@entity_attributes);
  my $type_stats = $attribute_stats->{TYPE};
  foreach $ref_type (@entity_types) {
    foreach $type (@entity_types) {
      $type_stats{$ref_type}{$type} = $attribute_stats->{TYPE}{$ref_type}{$type} ?
	$attribute_stats->{TYPE}{$ref_type}{$type} : 0;
      $entity_type_total{$ref_type} += $type_stats{$ref_type}{$type};
    }
  }
  print "\nEntity Type confusion matrix for \"$source_type\" sources (for mapped entities):\n",
    "               ___________count___________        __________percent__________\n",
      "    ref\\tst:  ";
  foreach $type (@entity_types) {
    printf " %3.3s  ", $type;
  }
  print "   ";
  foreach $type (@entity_types) {
    printf "   %3.3s", $type;
  }
  print "\n";
  foreach $ref_type (@entity_types) {
    printf "  %3.3s       ", $ref_type;
    foreach $type (@entity_types) {
      printf "%6d", $type_stats{$ref_type}{$type};
    }
    print "     ";
    foreach $type (@entity_types) {
      printf "%6.1f", 100*$type_stats{$ref_type}{$type} /
	max($entity_type_total{$ref_type},1);
    }
    print "\n";
  }

#attribute confusion statistics
  foreach my $attribute (@entity_attributes) {
    next if $attribute eq "ID";
    score_confusion_stats ($attribute_stats->{$attribute}, "attribute $attribute");
  }

#class attributes
  my ($class, $ref_class);
  my (%entity_class_total);
  my $class_stats = $attribute_stats->{CLASS};
  foreach $ref_class (@entity_classes) {
    foreach $class (@entity_classes) {
      $class_stats->{$ref_class}{$class} = 0 unless defined $class_stats->{$ref_class}{$class};
      $entity_class_total{$ref_class} += $class_stats->{$ref_class}{$class};
    }
  }
  print "\nEntity Class confusion matrix for \"$source_type\" sources (for mapped entities):\n",
    "               __count__        _percent_\n",
      "    ref\\tst:  ";
  foreach $class (@entity_classes) {
    printf " %3.3s  ", $class;
  }
  print "   ";
  foreach $class (@entity_classes) {
    printf "   %3.3s", $class;
  }
  print "\n";
  foreach $ref_class (@entity_classes) {
    printf "%5.5s       ", $ref_class;
    foreach $class (@entity_classes) {
      printf "%6d", $class_stats->{$ref_class}{$class};
    }
    print "     ";
    foreach $class (@entity_classes) {
      printf "%6.1f", 100*$class_stats->{$ref_class}{$class} /
	max($entity_class_total{$ref_class},1);
    }
    print "\n";
  }

#name attributes
  my $name_stats = name_recognition_stats ("entities");
  foreach $type (@entity_types) {
    foreach my $err (@error_types) {
      $name_stats->{$type}{$err} = 0 unless defined $name_stats-> {$type}{$err};
    }
    $name_stats->{total}{miss} += $name_stats->{$type}{miss};
    $name_stats->{total}{fa} += $name_stats->{$type}{fa};
    $name_stats->{total}{correct} += $name_stats->{$type}{correct};
    $name_stats->{total}{error} += $name_stats->{$type}{error};
  }
  print "\nName Detection and Extent Recognition statistics (for mapped entities):\n",
  " ref      ____________count____________      ________percent________\n",
  " entity   Ref    Detection   Extent_Rec      Detection    Extent_Rec\n",
  " type     Tot     FA  Miss    Err  Corr       FA  Miss     Err  Corr\n";
  foreach $type (@entity_types, "total") {
    my $total = ($name_stats->{$type}{miss} +
		 $name_stats->{$type}{error} +
		 $name_stats->{$type}{correct});
    printf "%5.5s%8d%7d%6d%7d%6d%9.1f%6.1f%8.1f%6.1f\n", $type, $total,
    $name_stats->{$type}{fa}, $name_stats->{$type}{miss}, $name_stats->{$type}{error}, $name_stats->{$type}{correct},
    100*$name_stats->{$type}{fa}/max($total,1), 100*$name_stats->{$type}{miss}/max($total,1),
    100*$name_stats->{$type}{error}/max($total,1), 100*$name_stats->{$type}{correct}/max($total,1);
  }
}

#################################

sub score_entity_mention_detection {

  my ($mention_stats) = @_;

  my (%men_count, $type, $men_type, $rol_type, $sty_type, $err_type);
  my ($pn, $total);

  #scoring conditioned on mention type
  undef %men_count;
  foreach $men_type (@entity_mention_types) {
    foreach $rol_type (@entity_types) {
      foreach $sty_type (@entity_style_types) {
	foreach $err_type (@error_types) {
	  my $nent = $mention_stats->{$men_type}{$rol_type}{$sty_type}{$err_type};
	  $men_count{$men_type}{$err_type} += $nent ? $nent : 0;
	  $men_count    {total}{$err_type} += $nent ? $nent : 0;
	}
      }
    }
  }
  print "\nMention Detection and Extent Recognition statistics (for mapped entities):\n",
  " ref      ____________count____________      ________percent________\n",
  " mention  Ref    Detection   Extent_Rec      Detection    Extent_Rec\n",
  " type     Tot     FA  Miss    Err  Corr       FA  Miss     Err  Corr\n";
  foreach $type (@entity_mention_types, "total") {
    $total = $men_count{$type}{miss}+$men_count{$type}{error}+$men_count{$type}{correct};
    $pn = 100/max($epsilon, $total);
    printf "%5.5s%8d%7d%6d%7d%6d%9.1f%6.1f%8.1f%6.1f\n", $type, $total,
    $men_count{$type}{fa}, $men_count{$type}{miss}, $men_count{$type}{error}, $men_count{$type}{correct},
    min(999.9,$pn*$men_count{$type}{fa}), $pn*$men_count{$type}{miss}, $pn*$men_count{$type}{error}, $pn*$men_count{$type}{correct};
  }

  #scoring conditioned on mention style
  undef %men_count;
  foreach $men_type (@entity_mention_types) {
    foreach $rol_type (@entity_types) {
      foreach $sty_type (@entity_style_types) {
	foreach $err_type (@error_types) {
	  my $nent = $mention_stats->{$men_type}{$rol_type}{$sty_type}{$err_type};
	  $men_count{$sty_type}{$err_type} += $nent ? $nent : 0;
	  $men_count    {total}{$err_type} += $nent ? $nent : 0;
	}
      }
    }
  }
  print "\nMention Detection and Extent Recognition statistics (for mapped entities):\n",
  " ref      ____________count____________      ________percent________\n",
  " mention  Ref    Detection   Extent_Rec      Detection    Extent_Rec\n",
  " style    Tot     FA  Miss    Err  Corr       FA  Miss     Err  Corr\n";
  foreach $type (@entity_style_types, "total") {
    $total = $men_count{$type}{miss}+$men_count{$type}{error}+$men_count{$type}{correct};
    $pn = 100/max($epsilon, $total);
    printf "%5.5s%8d%7d%6d%7d%6d%9.1f%6.1f%8.1f%6.1f\n", $type, $total,
    $men_count{$type}{fa}, $men_count{$type}{miss}, $men_count{$type}{error}, $men_count{$type}{correct},
    min(999.9,$pn*$men_count{$type}{fa}), $pn*$men_count{$type}{miss}, $pn*$men_count{$type}{error}, $pn*$men_count{$type}{correct};
  }

  #scoring conditioned on mention role
  undef %men_count;
  foreach $men_type (@entity_mention_types) {
    foreach $rol_type (@entity_types) {
      foreach $sty_type (@entity_style_types) {
	foreach $err_type (@error_types) {
	  my $nent = $mention_stats->{$men_type}{$rol_type}{$sty_type}{$err_type};
	  $men_count{$rol_type}{$err_type} += $nent ? $nent : 0;
	  $men_count    {total}{$err_type} += $nent ? $nent : 0;
	}
      }
    }
  }
  print "\nMention Detection and Extent Recognition statistics (for mapped entities):\n",
  " ref      ____________count____________      ________percent________\n",
  " mention  Ref    Detection   Extent_Rec      Detection    Extent_Rec\n",
  " role     Tot     FA  Miss    Err  Corr       FA  Miss     Err  Corr\n";
  foreach $type (@entity_types, "total") {
    $total = $men_count{$type}{miss}+$men_count{$type}{error}+$men_count{$type}{correct};
    $pn = 100/max($epsilon, $total);
    printf "%5.5s%8d%7d%6d%7d%6d%9.1f%6.1f%8.1f%6.1f\n", $type, $total,
    $men_count{$type}{fa}, $men_count{$type}{miss}, $men_count{$type}{error}, $men_count{$type}{correct},
    min(999.9,$pn*$men_count{$type}{fa}), $pn*$men_count{$type}{miss}, $pn*$men_count{$type}{error}, $pn*$men_count{$type}{correct};
  }
}

#################################

sub score_entity_mention_attribute_recognition {

  my ($role_stats, $style_stats) = @_;

  # role attributes
  my ($role, $ref_role, $ent_type);
  my (%mention_role_count, %mention_role_total);

  print "\nMention Role confusion matrix for \"$source_type\" sources (for mapped mentions)\n",
    "  For all mapped mentions:\n",
      "               ___________count___________        __________percent__________\n",
	"    ref\\tst:  ";
  foreach $role (@entity_types) {
    printf " %3.3s  ", $role;
  }
  print "   ";
  foreach $role (@entity_types) {
    printf "   %3.3s", $role;
  }
  print "\n";
  foreach $ref_role (@entity_types) {
    foreach $role (@entity_types) {
      foreach $ent_type (@entity_types) {
	$role_stats->{ROLE}{$ent_type}{$ref_role}{$role} = 0 unless defined $role_stats->{ROLE}{$ent_type}{$ref_role}{$role};
	$mention_role_count{$ref_role}{$role} += $role_stats->{ROLE}{$ent_type}{$ref_role}{$role};
      }
      $mention_role_total{$ref_role} += $mention_role_count{$ref_role}{$role};
    }
    printf "%5.5s       ", $ref_role;
    foreach $role (@entity_types) {
      printf "%6d", $mention_role_count{$ref_role}{$role};
    }
    print "     ";
    foreach $role (@entity_types) {
      printf "%6.1f", 100*$mention_role_count{$ref_role}{$role} /
	max($mention_role_total{$ref_role},1);
    }
    print "\n";
  }

  foreach $ent_type (()) {         #(@entity_types) {
    print "  For mapped mentions whose entity is of type \"$ent_type\":\n",
      "               ___________count___________        __________percent__________\n",
	"    ref\\tst:  ";
    foreach $role (@entity_types) {
      printf " %3.3s  ", $role;
    }
    print "   ";
    foreach $role (@entity_types) {
      printf "   %3.3s", $role;
    }
    print "\n";
    foreach $ref_role (@entity_types) {
      $mention_role_total{$ref_role} = 0;
      foreach $role (@entity_types) {
	$mention_role_total{$ref_role} += $role_stats->{ROLE}{$ent_type}{$ref_role}{$role};
      }
      printf "%5.5s       ", $ref_role;
      foreach $role (@entity_types) {
	printf "%6d", $role_stats->{ROLE}{$ent_type}{$ref_role}{$role};
      }
      print "     ";
      foreach $role (@entity_types) {
	printf "%6.1f", 100*$role_stats->{ROLE}{$ent_type}{$ref_role}{$role} /
	  max($mention_role_total{$ref_role},1);
      }
      print "\n";
    }
  }

  # style attributes
  my ($style, $ref_style);
  my (%mention_style_total);
  foreach $ref_style (@entity_style_types) {
    foreach $style (@entity_style_types) {
      $style_stats->{STYLE}{$ref_style}{$style} = 0 unless defined $style_stats->{STYLE}{$ref_style}{$style};
      $mention_style_total{$ref_style} += $style_stats->{STYLE}{$ref_style}{$style};
    }
  }
  print "\nMention Style confusion matrix for \"$source_type\" sources (for mapped mentions):\n",
    "               __count__        _percent_\n",
      "    ref\\tst:  ";
  foreach $style (@entity_style_types) {
    printf " %3.3s  ", $style;
  }
  print "   ";
  foreach $style (@entity_style_types) {
    printf "   %3.3s", $style;
  }
  print "\n";
  foreach $ref_style (@entity_style_types) {
    printf "%5.5s       ", $ref_style;
    foreach $style (@entity_style_types) {
      printf "%6d", $style_stats->{STYLE}{$ref_style}{$style};
    }
    print "     ";
    foreach $style (@entity_style_types) {
      printf "%6.1f", 100*$style_stats->{STYLE}{$ref_style}{$style} /
	max($mention_style_total{$ref_style},1);
    }
    print "\n";
  }
}

#################################

sub score_releve_attribute_recognition {

  my ($element_types, $releve_types, $attributes, $modalities) = @_;

  my $element_type = my $element_key = $element_types;
  $element_type =~ s/s$//;
  $element_key =~ s/^mention_//;
  my $attribute_stats = attribute_confusion_stats ($element_key, [keys %$attributes]);

  # type attributes
  my ($ref_type, %type_stats);
  my (%type_total);
  foreach $ref_type (@$releve_types) {
    foreach my $type (@$releve_types) {
      $type_stats{$ref_type}{$type} = defined $attribute_stats->{TYPE}{$ref_type}{$type} ?
	$attribute_stats->{TYPE}{$ref_type}{$type} : 0;
      $type_total{$ref_type} += $type_stats{$ref_type}{$type};
    }
  }
  print "\n", ucfirst "$element_type Type confusion matrix for \"$source_type\" sources (for mapped $element_types):\n",
  "             COUNT", "." x (6*(@$releve_types-1)), "    PERCENT", "." x (6*(@$releve_types-1)), "\n    ref\\tst:";
  foreach my $type (@$releve_types) {
    printf " %5.5s", $type;
  }
  print "     ";
  foreach my $type (@$releve_types) {
    printf " %5.5s", $type;
  }
  print "\n";
  foreach $ref_type (@$releve_types) {
    printf "%11.11s ", $ref_type;
    foreach my $type (@$releve_types) {
      printf "%6d", $type_stats{$ref_type}{$type};
    }
    print "     ";
    foreach my $type (@$releve_types) {
      printf "%6.1f", 100*$type_stats{$ref_type}{$type} /
	max($type_total{$ref_type},1);
    }
    print "\n";
  }

  # subtype attributes
  my $subtype_stats = subtype_confusion_stats ($element_key);
  foreach $ref_type (@$releve_types) {
    my $stats = $subtype_stats->{$ref_type};
    next unless $stats;
    my (@subtypes, %subtype_total);
    @subtypes = sort keys %{$attributes->{TYPE}{$ref_type}};
    foreach my $subtype (@subtypes) {
      foreach my $type (@subtypes) {
	$stats->{$subtype}{$type} = 0 unless
	  defined $stats->{$subtype} and defined $stats->{$subtype}{$type};
	$subtype_total{$subtype} += $stats->{$subtype}{$type};
      }
    }
    print "\n", ucfirst "$element_type Subtype confusion matrix for \"$source_type\" sources (for mapped $element_types):\n";
    printf "  type=%-5.5s", $ref_type;
    print " COUNT", "." x (6*(@subtypes-1)), "    PERCENT", "." x (6*(@subtypes-1)), "\n    ref\\tst: ";
    foreach my $subtype (@subtypes) {
      printf "%5.5s ", $subtype;
    }
    print "     ";
    foreach my $subtype (@subtypes) {
      printf "%5.5s ", $subtype;
    }
    print "\n";
    foreach my $subtype (@subtypes) {
      printf "%11.11s ", $subtype;
      foreach my $type (@subtypes) {
	printf "%6d", $stats->{$subtype}{$type};
      }
      print "     ";
      foreach my $type (@subtypes) {
	printf "%6.1f", 100*$stats->{$subtype}{$type} /
	  max($subtype_total{$subtype},1);
      }
      print "\n";
    }
  }

  # modality attributes
  my ($modality, $ref_modality, %modality_stats);
  my (%modality_total);
  foreach $ref_modality (@$modalities) {
    foreach $modality (@$modalities) {
      $modality_stats{$ref_modality}{$modality} = defined $attribute_stats->{MODALITY}{$ref_modality}{$modality} ?
	$attribute_stats->{MODALITY}{$ref_modality}{$modality} : 0;
      $modality_total{$ref_modality} += $modality_stats{$ref_modality}{$modality};
    }
  }
  print "\n", ucfirst "$element_type Modality confusion matrix for \"$source_type\" sources (for mapped $element_types):\n",
  "             COUNT", "." x (6*(@$modalities-1)), "    PERCENT", "." x (6*(@$modalities-1)), "\n    ref\\tst:";
  foreach $modality (@$modalities) {
    printf " %5.5s", $modality;
  }
  print "     ";
  foreach $modality (@$modalities) {
    printf " %5.5s", $modality;
  }
  print "\n";
  foreach $ref_modality (@$modalities) {
    printf "%11.11s ", $ref_modality;
    foreach $modality (@$modalities) {
      printf "%6d", $modality_stats{$ref_modality}{$modality};
    }
    print "     ";
    foreach $modality (@$modalities) {
      printf "%6.1f", 100*$modality_stats{$ref_modality}{$modality} /
	max($modality_total{$ref_modality},1);
    }
    print "\n";
  }

  foreach my $attribute (sort keys %$attributes) {
    next if $attribute eq "ID";
    score_confusion_stats ($attribute_stats->{$attribute}, "attribute $attribute");
  }

  my $role_stats = role_confusion_stats ($ref_database{$element_key});
  foreach my $type ("", sort keys %$role_stats) {
    next if $type eq "ALL_TYPES";
    my $type_label = "argument ROLE for ".((not $type) ? "all types" : "type $type");
    foreach my $subtype ("", sort keys %{$role_stats->{(not $type) ? "ALL_TYPES" : $type}}) {
      next if $subtype eq "ALL_SUBTYPES";
      my $subtype_label = (not $subtype) ? "all subtypes" : "subtype $subtype";
      foreach my $level ("MAPPED", "MATCHING_TYPE", "MATCHING_SUBTYPE") {
	my $level_label = $level eq "MAPPED" ? "" :
	  $level eq "MATCHING_TYPE" ? "for matching TYPE" : "for matching TYPE and SUBTYPE";
	next if $type or $subtype;
	score_confusion_stats ($role_stats->{(not $type) ? "ALL_TYPES" : $type}
			       {(not $subtype) ? "ALL_SUBTYPES" : $subtype}
			       {$level},
			       "$type_label and $subtype_label $level_label");
      }
    }
  }
}

#################################

sub value_type {

  my ($value, $value_types) = @_;

  foreach my $value_type (@$value_types) {
    return $value_type if $value_type =~ /^>/;
    my $upper_value = $value_type;
    $upper_value =~ s/.*[^0-9\.]//;
    return $value_type if not defined $value or $value < $upper_value+0.000005;
  }
  die "\n\nFATAL ERROR:  problem in value_type\n";
}

#################################

sub condition_value {

  my ($element, $document, $condition) = @_;

  my $docs = $element->{documents};
  my ($value, $count_types, $narg_errs);
  if ($condition eq "MENTION COUNT") {
    $count_types = ($element->{ELEMENT_TYPE} =~ /entity/ ?   \@entity_mention_count_types :
		    $element->{ELEMENT_TYPE} =~ /relation/ ? \@relation_mention_count_types :
		    $element->{ELEMENT_TYPE} =~ /event/ ?    \@event_mention_count_types :
		    $element->{ELEMENT_TYPE} =~ /timex2/ ?   \@timex2_mention_count_types :
		    $element->{ELEMENT_TYPE} =~ /value/ ? \@value_mention_count_types :
		    die "\n\nFATAL ERROR:  unknown element type ($element->{ELEMENT_TYPE}) in condition_type\n");
    my $nmentions = 0;
    foreach my $doc ($document ? $document : keys %{$element->{documents}}) {
      $nmentions += scalar @{$element->{documents}{$doc}{mentions}};
    }
    $value = value_type ($nmentions, $count_types);
  } elsif ($condition eq "ARGUMENT ERRORS") {
    $count_types = ($element->{ELEMENT_TYPE} =~ /relations/ ? \@relation_arg_err_count_types :
		                                              \@event_arg_err_count_types);
    $narg_errs = $element->{MAP} ? num_argument_mapping_errors ($element) : 0;
    $value = value_type ($narg_errs, $count_types);
  } elsif ($condition eq "NUM ARGUMENTS") {
    $count_types = ($element->{ELEMENT_TYPE} =~ /relations/ ? \@relation_arg_count_types :
		                                              \@event_arg_count_types);
    my $num_args = 0;
    while (my ($ref_role, $ref_ids) = each %{$element->{arguments}}) {
      $num_args += keys %$ref_ids;
    }
    $value = value_type ($num_args, $count_types);
  } elsif ($condition eq "SOURCE") {
    foreach my $doc ($document ? $document : keys %{$element->{documents}}) {
      $value = !$value ? $docs->{$doc}{SOURCE} : $value eq $docs->{$doc}{SOURCE} ? $value : "(multi)";
    }
  } elsif ($condition eq "LEVEL") {
    foreach my $doc ($document ? $document : keys %{$element->{documents}}) {
      $value = !$value ? $docs->{$doc}{LEVEL} : $value eq $docs->{$doc}{LEVEL} ? $value : "(multi)";
    }
  } elsif ($condition eq "LANGUAGE") {
    foreach my $doc ($document ? $document : keys %{$element->{documents}}) {
      $value = $element->{documents}{$doc}{LANGUAGE};
    }
  } else {
    $value = 
      $condition =~ /^(TYPE|SUBTYPE|CLASS|MODALITY)$/ ? $element->{$condition} :
      $condition eq "VALUE" ?     value_type ($document ? $docs->{$document}{VALUE} : $element->{VALUE}, \@entity_value_types) :
      $condition eq "CROSS-DOC" ? value_type (scalar keys %{$element->{documents}}, \@xdoc_types) :
      $condition eq "NAME_ENTROPY" ? value_type ($element->{NAME_ENTROPY}, \@entropy_types) :
      die "\n\nFATAL ERROR:  unknown condition ($condition) in call to condition_value\n";
  }
  return $value eq "" ? "<null>" : $value;
}

#################################

sub conditional_b3_stats {

  my ($b3_data, $cond1, $cond2, $external_reconciliation) = @_;

  return undef unless $b3_data;
  foreach my $cond ($cond1, $cond2) {
    return undef if $cond and $cond !~ /^(TYPE|SUBTYPE|LEVEL|CLASS|SOURCE|VALUE|MENTION COUNT|CROSS-DOC)$/;
  }
  my %cum_b3_scores;
  foreach my $type ("REF", "SYS") {
    my ($cum_coref_score, $cum_mention_score);
    my $entities = $type eq "REF" ? $ref_database{entities} : $tst_database{entities};
    foreach my $hash (values %{$b3_data->{$type}}) {
      my $mention = $hash->{MENTION};
      my $entity = $entities->{$mention->{host_id}};
      next if ($external_reconciliation and not @{$entity->{external_links}});
      my $c1_value = b3_condition_value ($mention, $entity, $cond1);
      my $c2_value = b3_condition_value ($mention, $entity, $cond2) if $cond2;
      my ($coref_count_score, $coref_value_score) = $hash->{COUNT} && $entity->{mentions_count} ?
	  (max (values %{$hash->{COUNT}})/$entity->{mentions_count},
	   max (values %{$hash->{VALUE}})/$entity->{mentions_value}) : (0,0);
      if ($cond2) {
	foreach my $k2 ($c2_value, "ALL") {
	  foreach my $k1 ($c1_value, "total") {
	    $cum_b3_scores{$k2}{$k1}{$type}{COUNT}{coref} += $coref_count_score;
	    $cum_b3_scores{$k2}{$k1}{$type}{COUNT}{mention}++;
	    $cum_b3_scores{$k2}{$k1}{$type}{VALUE}{coref} += $mention->{self_score}*$coref_value_score;
	    $cum_b3_scores{$k2}{$k1}{$type}{VALUE}{mention} += $mention->{self_score};
	  }
	}
      } else {
	foreach my $k1 ($c1_value, "total") {
	  $cum_b3_scores{$k1}{$type}{COUNT}{coref} += $coref_count_score;
	  $cum_b3_scores{$k1}{$type}{COUNT}{mention}++;
	  $cum_b3_scores{$k1}{$type}{VALUE}{coref} += $mention->{self_score}*$coref_value_score;
	  $cum_b3_scores{$k1}{$type}{VALUE}{mention} += $mention->{self_score};
	}
      }
    }
  }
  my %b3_scores;
  if ($cond2) {
    while (my ($k2, $k1_scores) = each %cum_b3_scores) {
      while (my ($k1, $scores) = each %$k1_scores) {
	foreach my $type ("COUNT", "VALUE") {
	  $b3_scores{$k2}{$k1}{$type}{recall} = $scores->{REF}{$type}{mention} ?
	    $scores->{REF}{$type}{coref}/$scores->{REF}{$type}{mention} : 0;
	  $b3_scores{$k2}{$k1}{$type}{precision} = $scores->{SYS}{$type}{mention} ? 
	    $scores->{SYS}{$type}{coref}/$scores->{SYS}{$type}{mention} : 0;						    
	}
      }
    }
  } else {
    while (my ($k1, $scores) = each %cum_b3_scores) {
      foreach my $type ("COUNT", "VALUE") {
	$b3_scores{$k1}{$type}{recall} = $scores->{REF}{$type}{mention} ?
	  $scores->{REF}{$type}{coref}/$scores->{REF}{$type}{mention} : 0;
	$b3_scores{$k1}{$type}{precision} = $scores->{SYS}{$type}{mention} ?
	  $scores->{SYS}{$type}{coref}/$scores->{SYS}{$type}{mention} : 0;
      }
    }
  }
  return {%b3_scores};
}

#################################

sub b3_condition_value {

  my ($mention, $entity, $condition) = @_;

  my $value =
    $condition =~ /^(TYPE|SUBTYPE|CLASS)$/ ? $entity->{$condition} :
    $condition eq "LEVEL" ? $mention->{TYPE} :
    $condition eq "SOURCE" ? $eval_docs{$mention->{document}}{SOURCE} :
    $condition eq "CROSS-DOC" ? value_type (scalar keys %{$entity->{documents}}, \@xdoc_types) :
    $condition eq "MENTION COUNT" ? value_type ($entity->{mentions_count}, \@entity_mention_count_types) :
    $condition eq "VALUE" ? value_type ($entity->{VALUE}, \@entity_value_types) :
    die "\n\nFATAL ERROR:  unknown condition ($condition) in call to b3_condition_value\n";
}

#################################

sub conditional_error_stats {

  my ($elements, $cond1, $cond2, $external_reconciliation) = @_;

  my $attributes =
    $elements eq "entities" ?  \@entity_attributes :
    $elements eq "relations" ? \@relation_attributes :
    $elements eq "events" ?    \@event_attributes :
    $elements eq "timex2s" ?   \@timex2_attributes :
                               \@value_attributes;
    
#accumulate statistics over all documents
  my (%error_count, %cumulative_cost, %normalizing_cost, %document_costs);
  while (my ($id, $element) = each %{$ref_database{$elements}}) {
    next if ($external_reconciliation and not @{$element->{external_links}});
    foreach my $doc (keys %{$element->{documents}}) {
      my $doc_ref = $element->{documents}{$doc};
      my $doc_tst=$doc_ref->{MAP};
      my $cost = my $norm_cost = $doc_ref->{VALUE};
      my ($err_type, $att_errs);
      my $doc_data = $doc_tst ? $mapped_document_costs{$doc_ref->{ID}}{$doc_tst->{ID}}{$doc} : undef;
      if ($doc_data) {
	$cost = $doc_ref->{VALUE} - $mapped_document_values{$doc_ref->{ID}}{$doc_tst->{ID}}{$doc};
	$cost = 0 if $cost < $epsilon;
	foreach my $attribute (@$attributes) {
	  next if $attribute eq "ID";
	  next if not defined $doc_ref->{$attribute} and not defined $doc_tst->{$attribute};
	  $att_errs++ unless (defined $doc_ref->{$attribute} and defined $doc_tst->{$attribute}
			      and $doc_ref->{$attribute} eq $doc_tst->{$attribute});
	}
	my $arg_errs = num_argument_mapping_errors ($element)
	  if $element->{ELEMENT_TYPE} =~ /relation|event/;
	$err_type = ($att_errs or $arg_errs) ? "error" : "correct";
      } else {
	$err_type = "miss";
      }
      $err_type = match_external_links ($element, $element->{MAP}) if $external_reconciliation;
      my $c1_value = condition_value ($element, $doc, $cond1);
      if ($cond2) {
	my $c2_value = condition_value ($element, $doc, $cond2);
	foreach my $k2 ($c2_value, "ALL") {
	  foreach my $k1 ($c1_value, "total") {
	    $error_count{doc}{$k2}{$k1}{$err_type}++;
	    $cumulative_cost{$k2}{$k1}{$err_type} += $cost;
	    $normalizing_cost{$k2}{$k1} += $norm_cost;
	    if ($doc_data) {
	      foreach my $cost_type (keys %$doc_data) {
		$cumulative_cost{$k2}{$k1}{$cost_type} += $doc_data->{$cost_type};
	      }
	    }
	  }
	}
      } else {
	foreach my $k1 ($c1_value, "total") {
	  $error_count{doc}{$k1}{$err_type}++;
	  $cumulative_cost{$k1}{$err_type} += $cost;
	  $normalizing_cost{$k1} += $norm_cost;
	  if ($doc_data) {
	    foreach my $cost_type (keys %$doc_data) {
	      $cumulative_cost{$k1}{$cost_type} += $doc_data->{$cost_type};
	    }
	  }
	}
      }
      $document_costs{$doc}{REF} += $norm_cost;
      $document_costs{$doc}{SYS} += $cost;
    }
    my $element_err = $element->{MAP} ? "mapped" : "miss";
    my $c1_value = condition_value ($element, undef, $cond1);
    if ($cond2) {
      my $c2_value = condition_value ($element, undef, $cond2);
      foreach my $k2 ($c2_value, "ALL") {
	foreach my $k1 ($c1_value, "total") {
	  $error_count{element}{$k2}{$k1}{$element_err}++;
        }
      }
    } else {
      foreach my $k1 ($c1_value, "total") {
	$error_count{element}{$k1}{$element_err}++;
      }
    }
  }

#update entity false alarm statistics
  while (my ($id, $element) = each %{$tst_database{$elements}}) {
    next if ($external_reconciliation and not @{$element->{external_links}});
    foreach my $doc (keys %{$element->{documents}}) {
      my $doc_tst = $element->{documents}{$doc};
      next if $doc_tst->{MAP};
      my $norm_cost = 0;
      my $cost = -$doc_tst->{FA_VALUE};
      my $err_type = "fa";
      my $c1_value = condition_value ($element, $doc, $cond1);
      if ($cond2) {
	my $c2_value = condition_value ($element, $doc, $cond2);
	foreach my $k2 ($c2_value, "ALL") {
	  foreach my $k1 ($c1_value, "total") {
	    $error_count{doc}{$k2}{$k1}{$err_type}++;
	    $cumulative_cost{$k2}{$k1}{$err_type} += $cost;
	    $normalizing_cost{$k2}{$k1} += $norm_cost;
	  }
        }
      } else {
	foreach my $k1 ($c1_value, "total") {
	  $error_count{doc}{$k1}{$err_type}++;
	  $cumulative_cost{$k1}{$err_type} += $cost;
	  $normalizing_cost{$k1} += $norm_cost;
	}
      }
      $document_costs{$doc}{REF} += $norm_cost;
      $document_costs{$doc}{SYS} += $cost;
    }
    next if $element->{MAP};
    my $err_type = "fa";
    my $c1_value = condition_value ($element, undef, $cond1);
    if ($cond2) {
      my $c2_value = condition_value ($element, undef, $cond2);
      foreach my $k2 ($c2_value, "ALL") {
	foreach my $k1 ($c1_value, "total") {
	  $error_count{element}{$k2}{$k1}{$err_type}++;
        }
      }
    } else {
      foreach my $k1 ($c1_value, "total") {
	$error_count{element}{$k1}{$err_type}++;
      }
    }
  }
  return ({%error_count}, {%cumulative_cost}, {%normalizing_cost}, {%document_costs});
}

#################################

sub match_external_links { #return "correct" if any ref link matches any tst link

  my ($ref, $tst) = @_;

  if (@{$ref->{external_links}}) {
    return "miss" unless $tst and @{$tst->{external_links}};
  } else {
    return @{$tst->{external_links}} ? "fa" : undef;
  }
  my %tst_links;
  foreach my $link (@{$tst->{external_links}}) {
    $tst_links{$link->{RESOURCE}} = $link->{ID};
  }
  foreach my $link (@{$ref->{external_links}}) {
    return "correct" if (defined $tst_links{$link->{RESOURCE}} and
			 $tst_links{$link->{RESOURCE}} eq $link->{ID});
  }
  return "error";
}

#################################

sub name_recognition_stats {

  my ($elements) = @_;

#accumulate statistics over all documents
  my %name_statistics;
  while (my ($id, $element) = each %{$ref_database{$elements}}) {
    while (my ($doc, $doc_ref) = each %{$element->{documents}}) {
      next unless my $doc_tst=$doc_ref->{MAP};
      my $num_ref_names = @{$doc_ref->{names}};
      my $num_tst_names = @{$doc_tst->{names}};
      my $name_map = $name_map{$doc_ref->{ID}}{$doc_tst->{ID}}{$doc};
      my $num_matched_names = keys %$name_map;
      while (my ($i, $j) = each %$name_map) {
	my $err_type = $doc_ref->{names}[$i] eq $doc_tst->{names}[$j] ? "correct" : "error";
	$name_statistics{$element->{TYPE}}{$err_type}++;
      }	  
      $name_statistics{$element->{TYPE}}{miss} += $num_ref_names - $num_matched_names;
      $name_statistics{$element->{TYPE}}{fa} += $num_tst_names - $num_matched_names;
    }
  }
  return {%name_statistics};
}

#################################

sub mention_recognition_stats {

  my ($elements) = @_;

#accumulate statistics over all documents
  my (%detection_stats, %role_stats, %style_stats);
  while (my ($id, $element) = each %{$ref_database{$elements}}) {
    while (my ($doc, $doc_ref) = each %{$element->{documents}}) {
      next unless my $doc_tst=$doc_ref->{MAP};
      my $ref_mentions = $doc_ref->{mentions};
      my $tst_mentions = $doc_tst->{mentions};
      foreach my $ref_mention (@$ref_mentions) {
	my $err_type = "miss";
	my $ref_type = defined $ref_mention->{TYPE} ? $ref_mention->{TYPE} : $doc_ref->{TYPE};
	my $ref_role = defined $ref_mention->{ROLE} ? $ref_mention->{ROLE} : $doc_ref->{TYPE};
	my $ref_style = defined $ref_mention->{STYLE} ? $ref_mention->{STYLE} : "LITERAL";
	my $tst_mention = $ref_mention->{MAP};
	if ($tst_mention) {
	  my $tst_role = defined $tst_mention->{ROLE} ? $tst_mention->{ROLE} : $doc_tst->{TYPE};
	  my $tst_style = defined $tst_mention->{STYLE} ? $tst_mention->{STYLE} : "LITERAL";
	  $role_stats{ROLE}{$element->{TYPE}}{$ref_role}{$tst_role}++;
	  $style_stats{STYLE}{$ref_style}{$tst_style}++;
	  $err_type = extent_mismatch ($ref_mention->{extent}{locator}, $tst_mention->{extent}{locator}) <= 1 ?
	    "correct" : "error";
	}
	$detection_stats{$ref_type}{$ref_role}{$ref_style}{$err_type}++;
      }
      foreach my $tst_mention (@$tst_mentions) {
	next if $tst_mention->{MAP};
	my $tst_role = defined $tst_mention->{ROLE} ? $tst_mention->{ROLE} : $doc_tst->{TYPE};
	my $tst_style = defined $tst_mention->{STYLE} ? $tst_mention->{STYLE} : "LITERAL";
	$detection_stats{$tst_mention->{TYPE}}{$tst_role}{$tst_style}{fa}++;
      }
    }
  }
  return ({%detection_stats}, {%role_stats}, {%style_stats});
}

#################################

sub attribute_confusion_stats {

  my ($elements, $attributes) = @_;

#accumulate statistics over all documents
  my %attribute_stats;
  while (my ($id, $element) = each %{$ref_database{$elements}}) {
    while (my ($doc, $doc_ref) = each %{$element->{documents}}) {
      next unless my $doc_tst=$doc_ref->{MAP};
      foreach my $attribute (@$attributes) {
	next if $attribute eq "ID";
	next if not defined $doc_ref->{$attribute} and not defined $doc_tst->{$attribute};
	my $ref_att = $doc_ref->{$attribute} ? $doc_ref->{$attribute} : "<undef>";
	my $tst_att = $doc_tst->{$attribute} ? $doc_tst->{$attribute} : "<undef>";
	$attribute_stats{$attribute}{$ref_att}{$tst_att}++;
      }
    }
  }
  return {%attribute_stats};
}

#################################

sub subtype_confusion_stats {

  my ($elements) = @_;

#accumulate statistics over all documents
  my %stats;
  while (my ($id, $element) = each %{$ref_database{$elements}}) {
    while (my ($doc, $doc_ref) = each %{$element->{documents}}) {
      next unless my $doc_tst=$doc_ref->{MAP};
      my $ref_subtype = $doc_ref->{SUBTYPE} ? $doc_ref->{SUBTYPE} : "<undef>";
      my $tst_subtype = $doc_tst->{SUBTYPE} ? $doc_tst->{SUBTYPE} : "<undef>";
      $stats{$element->{TYPE}}{$ref_subtype}{$tst_subtype}++;
    }
  }
  return {%stats};
}

#################################

sub role_confusion_stats {

  my ($db) = @_;

#accumulate statistics over all documents
  my %role_stats;
  while (my ($id, $ref) = each %$db) {
    next unless my $tst = $ref->{MAP};
    while (my ($doc, $doc_ref) = each %{$ref->{documents}}) {
      next unless my $doc_tst=$doc_ref->{MAP};
      while (my ($ref_role, $ref_ids) = each %{$ref->{arguments}}) {
	foreach my $ref_arg (values %$ref_ids) {
	  my $tst_arg = $ref_arg->{MAP};
	  my $tst_role = $tst_arg ? $tst_arg->{ROLE} : "<undef>";
	  foreach my $level ("MAPPED", "MATCHING_TYPE", "MATCHING_SUBTYPE") {
	    next if $level eq "MATCHING_TYPE" and $doc_ref->{TYPE} ne $doc_tst->{TYPE};
	    next if $level eq "MATCHING_SUBTYPE" and $doc_ref->{SUBTYPE} ne $doc_tst->{SUBTYPE};
	    $role_stats{$doc_tst->{TYPE}}{$doc_tst->{SUBTYPE}}{$level}{$ref_role}{$tst_role}++;
	    $role_stats{$doc_tst->{TYPE}}{ALL_SUBTYPES}{$level}{$ref_role}{$tst_role}++;
	    $role_stats{ALL_TYPES}{$doc_tst->{SUBTYPE}}{$level}{$ref_role}{$tst_role}++;
	    $role_stats{ALL_TYPES}{ALL_SUBTYPES}{$level}{$ref_role}{$tst_role}++;
	  }
	}
      }
      while (my ($tst_role, $tst_ids) = each %{$tst->{arguments}}) {
	foreach my $tst_arg (values %$tst_ids) {
	  next if my $ref_arg = $tst_arg->{MAP};
	  my $ref_role = "<undef>";
	  foreach my $level ("MAPPED", "MATCHING_TYPE", "MATCHING_SUBTYPE") {
	    next if $level eq "MATCHING_TYPE" and $doc_ref->{TYPE} ne $doc_tst->{TYPE};
	    next if $level eq "MATCHING_SUBTYPE" and $doc_ref->{SUBTYPE} ne $doc_tst->{SUBTYPE};
	    $role_stats{$doc_tst->{TYPE}}{$doc_tst->{SUBTYPE}}{$level}{$ref_role}{$tst_role}++;
	    $role_stats{$doc_tst->{TYPE}}{ALL_SUBTYPES}{$level}{$ref_role}{$tst_role}++;
	    $role_stats{ALL_TYPES}{$doc_tst->{SUBTYPE}}{$level}{$ref_role}{$tst_role}++;
	    $role_stats{ALL_TYPES}{ALL_SUBTYPES}{$level}{$ref_role}{$tst_role}++;
	  }
	}
      }
    }
  }
  return {%role_stats};
}

#################################

sub num_argument_mapping_errors {

  my ($ref) = @_;

  my $num_ref_args = 0;
  while (my ($ref_role, $ref_ids) = each %{$ref->{arguments}}) {
    $num_ref_args += keys %$ref_ids;
  }
  return $num_ref_args unless (defined $ref->{MAP} and
			       defined $argument_map{$ref->{ID}} and
			       defined $argument_map{$ref->{ID}}{$ref->{MAP}{ID}});

  my $num_mapped = my $num_correctly_mapped = 0;
  while (my ($tst_role, $tst_ids) = each %{$argument_map{$ref->{ID}}{$ref->{MAP}{ID}}}) {
    while (my ($tst_id, $ref_arg) = each %$tst_ids) {
      my $ref_map = $mapped_refs{$ref_arg->{ID}};
      next unless $ref_map;
      $num_mapped++;
      $num_correctly_mapped++ if (($ref_map->{ID} eq $tst_id or
				   (defined $tst_database{refs}{$tst_id}{HOST_ELEMENT_ID} and
				    $ref_map->{ID} eq $tst_database{refs}{$tst_id}{HOST_ELEMENT_ID})) and
				  ($ref_arg->{ROLE} eq $tst_role));
    }
  }

  my $num_sys_args = 0;
  while (my ($sys_role, $sys_ids) = each %{$ref->{MAP}{arguments}}) {
    $num_sys_args += keys %$sys_ids;
  }
  return $num_ref_args + $num_sys_args - $num_mapped - $num_correctly_mapped;
}

#################################

sub print_entity_mapping {

  my ($type) = @_;

  return unless $opt_a or $opt_e;
  print "\n======== $type mapping ========\n";

  my $ref_entities = $ref_database{entities};
  my $tst_entities = $tst_database{entities};
  foreach my $ref_id (sort keys %$ref_entities) {
    $print_data = $opt_a;
    my $output;
    my $ref = $ref_entities->{$ref_id};
    my $tst = $ref->{MAP};
    if ($tst) {
      my $tst_id = $tst->{ID};
      my $err_type = "";
      foreach my $attr ("TYPE", "SUBTYPE", "CLASS", "LEVEL") {
	$err_type .= ($err_type ? "/" : "").$attr if $ref->{$attr} ne $tst->{$attr};
      }
      $err_type = " -- ENTITY $err_type MISMATCH" if $err_type;
      my $external_match = match_external_links ($ref, $tst);
      $err_type .= (defined $err_type ? ", " : " -- ")."EXTERNAL ID MISMATCH" if $external_match and $external_match ne "correct";
      $print_data ||= $opt_e if $err_type;
      $output .= ($err_type ? ">>> " : "    ")."ref entity ".id_plus_external_ids($ref)." ".
	list_element_attributes($ref, \@entity_attributes)."$err_type\n";
      $output .= ($err_type ? ">>> " : "    ")."tst entity ".id_plus_external_ids($tst)." ".
	list_element_attributes($tst, \@entity_attributes)."$err_type\n";
      $output .= sprintf ("      entity score:  %.5f out of %.5f\n",
			  $mapped_values{$ref_id}{$tst_id}, $ref->{VALUE});
    } else {
      $print_data ||= $opt_e;
      $output .= ">>> ref entity ".id_plus_external_ids($ref)." ".
	list_element_attributes($ref, \@entity_attributes)." -- NO MATCHING TST ENTITY\n";
      $output .= sprintf "      entity score:  0.00000 out of %.5f\n", $ref->{VALUE};
    }
    my %docs;
    map $docs{$_}=1, keys %{$ref->{documents}} if $ref;
    map $docs{$_}=1, keys %{$tst->{documents}} if $tst;
    foreach my $doc (sort keys %docs) {
      $output .= entity_mapping_details ($ref, $ref->{MAP}, $doc) if $eval_docs{$doc};
    }
    print $output, "--------\n" if $print_data;
  }

  return unless $opt_a or $opt_e;
  foreach my $tst_id (sort keys %$tst_entities) {
    my $tst = $tst_entities->{$tst_id};
    next if $tst->{MAP};
    print ">>> tst entity ".id_plus_external_ids($tst)." ".
      list_element_attributes($tst, \@entity_attributes)." -- NO MATCHING REF ENTITY\n";
    printf "      entity score:  %.5f out of 0.00000\n", $tst->{FA_VALUE};
    foreach my $doc (sort keys %{$tst->{documents}}) {
      print STDOUT entity_mapping_details (undef, $tst, $doc) if $eval_docs{$doc};
    }
    print "--------\n";
  }
}

#################################

sub entity_mapping_details {

  my ($ref, $tst, $doc) = @_;

  $ref = $ref->{documents}{$doc} if $ref;
  $tst = $tst->{documents}{$doc} if $tst;
  (my $entity, my $max_value, my $value) =
    ($ref and $tst) ? ($ref, $ref->{VALUE}, $mapped_document_values{$ref->{ID}}{$tst->{ID}}{$doc}) :
      ($ref ? ($ref, $ref->{VALUE}, 0) :
       ($tst, 0, $tst->{FA_VALUE}));
  my $output = sprintf "- in document %s:  score:  %.5f out of %.5f\n", $doc, $value, $max_value;
  $output .= entity_name_mapping_details ($ref, $tst) if $use_name_similarity;
  $output .= entity_mention_mapping_details ($ref, $tst) if $use_mentions;
  return $output;
}

#################################

sub entity_name_mapping_details {

  my ($doc_ref, $doc_tst) = @_;

  my (@ref_data, @tst_data);
  if ($doc_ref) { # names
    foreach my $name (@{$doc_ref->{names}}) {
      push @ref_data, {NAME=>$name, TYPE=>"REF"};
    }
  }
  if ($doc_tst) {
    foreach my $name (@{$doc_tst->{names}}) {
      push @tst_data, {NAME=>$name, TYPE=>"TST"};
    }
  }
  my $map;
  if (@ref_data and @tst_data and
      $map = $name_map{$doc_ref->{ID}} and
      $map = $map->{$doc_tst->{ID}} and
      $map = $map->{$doc_ref->{document}}) {
    while (my ($i, $j) = each %$map) {
      $ref_data[$i]{MAP} = $tst_data[$j];
      $tst_data[$j]{MAP} = $ref_data[$i];
    }
    my ($score, $map, $best_ref_score, $best_tst_score) = entity_name_similarity ($doc_ref, $doc_tst);
    foreach my $name (@ref_data) {
      $name->{SCORE} = @$best_ref_score ? shift @$best_ref_score : 0;
    }
    foreach my $name (@tst_data) {
      $name->{SCORE} = @$best_tst_score ? shift @$best_tst_score : 0;
    }
  }
  my $output = "";
  foreach my $data (sort {$_ = $a->{NAME} cmp $b->{NAME};
			  $_ = $a->{TYPE} cmp $b->{TYPE} unless $_;} (@ref_data, @tst_data)) {
    my $type = $data->{TYPE};
    if ($data->{MAP}) {
      next if $type eq "TST";
      my $ref_name = $data->{NAME};
      my $tst_data = $data->{MAP};
      my $tst_name = $tst_data->{NAME};
      if ($ref_name eq $tst_name) {
	$output .= sprintf "         ref name score:%8.5f, \"%s\"\n", $data->{SCORE}, $ref_name;
      } else {
	$print_data ||= $opt_e;
	$output .= sprintf ">>>      ref name score:%8.5f, \"%s\"\n", $data->{SCORE}, $ref_name;
	$output .= sprintf ">>>      tst name score:%8.5f, \"%s\" -- NAME MISMATCH\n", $tst_data->{SCORE}, $tst_name;
      }	
    } else {
      $print_data ||= $opt_e;
      $output .= $doc_ref && $doc_tst ?
	sprintf ">>>      ".(lc$type)." name score:%8.5f, \"%s\" -- NO CORRESPONDING "
	  .($type eq "REF"?"TST":"REF")." NAME\n", $data->{SCORE}, $data->{NAME} :
	sprintf "         ".(lc$type)." name                 \"%s\"\n", $data->{NAME};
    }
  }
  return $output;
}

#################################

sub entity_mention_mapping_details {

  my ($doc_ref, $doc_tst) = @_;

  my @data;
  if ($doc_ref) { # mentions
    foreach my $mention (@{$doc_ref->{mentions}}) {
      push @data, {DATA=>$mention, TYPE=>"REF"};
    }
  }
  if ($doc_tst) {
    foreach my $mention (@{$doc_tst->{mentions}}) {
      push @data, {DATA=>$mention, TYPE=>"TST"};
    }
  }
  my $output = "";
  foreach my $mention (sort compare_locators @data) {
    my $type = $mention->{TYPE};
    $mention = $mention->{DATA};
    if ($mention->{MAP}) {
      next if $type eq "TST";
      my ($ref_mention, $tst_mention) = ($mention, $mention->{MAP});
      my $error_type = $ref_mention->{TYPE} eq $tst_mention->{TYPE} ? "" : "TYPE";
      $error_type .= $error_type ? "/ROLE" : "ROLE" if ($ref_mention->{ROLE} ne $tst_mention->{ROLE});
      $error_type .= $error_type ? "/STYLE" : "STYLE" if ($ref_mention->{STYLE} ne $tst_mention->{STYLE});
      $print_data ||= $opt_e if $error_type;
      foreach $mention (["REF", $ref_mention], ["TST", $tst_mention]) {
	$type = $mention->[0], $mention = $mention->[1];
	$output .= sprintf ("%s   %s mention", $error_type ? ">>>" : "   ", lc $type);
	$output .= $type eq "REF" ? ":"." "x31 : sprintf (" score: %.5f out of %.5f, ", 
							  $ref_mention->{tst_scores}{$tst_mention},
							  $ref_mention->{self_score});
	$output .= defined $mention->{head}{text} ? "\"$mention->{head}{text}\"" : "\"???\"";
	$output .= sprintf " (%3.3s/%3.3s/%3.3s) ID=%s", $mention->{TYPE}, $mention->{ROLE}, $mention->{STYLE}, $mention->{ID};
	$output .= sprintf " -- MENTION $error_type MISMATCH" if $error_type;
	$output .= "\n";
      }
      if (extent_mismatch ($ref_mention->{extent}{locator}, $tst_mention->{extent}{locator}) > 1) {
	$print_data ||= $opt_e;
	$output .= sprintf (">>>    ref mention extent = \"%s\"\n",
			    defined $ref_mention->{extent}{text} ? $ref_mention->{extent}{text} : "???");
	$output .= sprintf (">>>    tst mention extent = \"%s\" -- MENTION EXTENT MISMATCH\n",
			    defined $tst_mention->{extent}{text} ? $tst_mention->{extent}{text} : "???");
      }
    } else {
      $print_data ||= $opt_e;
      my $fa_score = -$entity_fa_wgt*entity_mention_score ($mention, $mention) *
	$entity_type_wgt{($type eq "REF") ? $doc_ref->{TYPE} : $doc_tst->{TYPE}} *
	$entity_class_wgt{($type eq "REF") ? $doc_ref->{CLASS} : $doc_tst->{CLASS}};
      $output .= sprintf (">>>   %s mention score: %.5f out of %.5f, \"%s\"", lc $type,
			  ($type eq "TST" ? ($fa_score, 0) : (0, $mention->{self_score})),
			  defined $mention->{head}{text} ? $mention->{head}{text} : "???");
      $output .= sprintf (" (%3.3s/%3.3s/%3.3s) ID=$mention->{ID} -- NO MATCHING %s MENTION\n",
			  $mention->{TYPE}, $mention->{ROLE}, $mention->{STYLE}, ($type eq "REF"?"TST":"REF"));
    }
  }
  return $output;
}

#################################

sub compute_b3 {

  $b3_data{REF} = mention_refs ($ref_database{entities});
  $b3_data{SYS} = mention_refs ($tst_database{entities});

  my %b3_coref;
  foreach my $ref_data (values %{$b3_data{REF}}) {
    my $ref_entity = $ref_data->{MENTION}{host_id};
    my %score;
    while (my ($tst, $tst_score) = each %{$ref_data->{MENTION}{tst_scores}}) {
      my $tst_data = $b3_data{SYS}{$tst};
      my $tst_entity = $tst_data->{MENTION}{host_id};
      $score{$tst_entity} = $tst_score if (not defined $score{$tst_entity} or
					   $tst_score > $score{$tst_entity});
    }
    while (my ($tst_entity, $score) = each %score) {
      $b3_coref{COUNT}{$ref_entity}{$tst_entity}++;
      $b3_coref{VALUE}{$ref_entity}{$tst_entity} += $score;
    }
  }
  foreach my $ref_data (values %{$b3_data{REF}}) {
    my $ref_entity = $ref_data->{MENTION}{host_id};
    while (my ($tst, $tst_score) = each %{$ref_data->{MENTION}{tst_scores}}) {
      my $tst_data = $b3_data{SYS}{$tst};
      my $tst_entity = $tst_data->{MENTION}{host_id};
      foreach my $type ("COUNT", "VALUE") {
	$ref_data->{$type}{$tst_entity} = max ($b3_coref{$type}{$ref_entity}{$tst_entity},
					       $ref_data->{$type}{$tst_entity});
	$tst_data->{$type}{$ref_entity} = max ($b3_coref{$type}{$ref_entity}{$tst_entity},
					       $tst_data->{$type}{$ref_entity});
      }
    }
  }
}

#################################

sub mention_refs {

  my ($entities) = @_;

  my $mention_data;
  foreach my $entity (values %$entities) {
    $entity->{mentions_value} = $entity->{mentions_count} = 0;
    foreach my $doc_entity (values %{$entity->{documents}}) {
      foreach my $mention (@{$doc_entity->{mentions}}) {
	$mention_data->{$mention} = {MENTION => $mention};
	$entity->{mentions_value} += $mention->{self_score};
	$entity->{mentions_count}++;
      }
    }
  }
  return $mention_data;
}

#################################

sub compute_element_values {

  my ($type) = @_;

  my (%doc_refs, %doc_tsts);

  undef %mapped_values;
  undef %mapped_document_values;
  foreach my $db ($ref_database{$type}, $tst_database{$type}) {
    foreach my $element (values %$db) {
      reset_element_values ($element);
    }
  }

  my $refs = $ref_database{$type};
  my $tsts = $tst_database{$type};
  my $mention_scorer = $type =~ /^entities$/ ? \&entity_mention_score : \&value_mention_score;

#select putative REF-TST element pairs for mapping
  while (my ($id, $element) = each %$refs) {
    foreach my $doc (keys %{$element->{documents}}) {
      push @{$doc_refs{$doc}}, $element;
    }
  }
  while (my ($id, $element) = each %$tsts) {
    foreach my $doc (keys %{$element->{documents}}) {
      push @{$doc_tsts{$doc}}, $element if defined $eval_docs{$doc};
    }
  }

#compute ref values
  foreach my $element (values %$refs) {
    while (my ($doc, $doc_element) = each %{$element->{documents}}) {
      ($doc_element->{VALUE}) = element_document_value ($element, $element, $doc);
      $element->{VALUE} += $doc_element->{VALUE};
    }
  }

#compute tst values
  foreach my $element (values %$tsts) {
    while (my ($doc, $doc_element) = each %{$element->{documents}}) {
      next unless defined $eval_docs{$doc};
      ($doc_element->{VALUE}) = element_document_value ($element, $element, $doc);
      $element->{VALUE} += $doc_element->{VALUE};
      ($doc_element->{FA_VALUE}) = element_document_value (undef, $element, $doc);
      $element->{FA_VALUE} += $doc_element->{FA_VALUE};
    }
  }

  my %candidate_tst_ref_pairs;
  if ($use_mentions) {
    my %candidate_pairs;
    foreach my $doc (keys %doc_refs) { #find candidate pairs
      my $pairs = candidate_element_pairs ($doc, $doc_refs{$doc}, $doc_tsts{$doc}, $mention_scorer);
      map $candidate_pairs{$_->[0]}{$_->[1]}=1, @$pairs;
    }
    while (my ($ref_id, $tst_ids) = each %candidate_pairs) { #compute ref-tst values
      foreach my $tst_id (keys %$tst_ids) {
	my %docs;
	map $docs{$_}=1, keys %{$refs->{$ref_id}{documents}};
	map $docs{$_}=1, keys %{$tsts->{$tst_id}{documents}};
	foreach my $doc (keys %docs) {
	  next unless defined $tsts->{$tst_id}{documents}{$doc};
	  my ($value, $mention_map, $name_map, $c1, $c2, $c3, $c4) = element_document_value ($refs->{$ref_id}, $tsts->{$tst_id}, $doc);
	  next unless defined $value;
	  $mapped_document_costs{$ref_id}{$tst_id}{$doc} = {ATTR_ERR => $c1,
							    MISS_SUB => $c2,
							    ERR_SUB => $c3,
							    FA_SUB => $c4};
	  $mapped_document_values{$ref_id}{$tst_id}{$doc} = $value;
	  $mapped_values{$ref_id}{$tst_id} += $value;
	  $mention_map{$ref_id}{$tst_id}{$doc} = $mention_map;
	  $name_map{$ref_id}{$tst_id}{$doc} = $name_map if $name_map;
	  $candidate_tst_ref_pairs{$tst_id}{$ref_id} = 1;
	}
      }
    }
  } else { #compute mapped element values
    while (my ($doc, $ref_doc_elements) = each %doc_refs) {
      foreach my $ref_doc_element (@$ref_doc_elements) {
	my $ref_id = $ref_doc_element->{ID};
	foreach my $tst_doc_element (@{$doc_tsts{$doc}}) {
	  my $tst_id = $tst_doc_element->{ID};
	  my ($value, $mention_map, $name_map, $c1, $c2, $c3, $c4) = element_document_value ($refs->{$ref_id}, $tsts->{$tst_id}, $doc);
	  next unless defined $value;
	  $mapped_document_costs{$ref_id}{$tst_id}{$doc} = {ATTR_ERR => $c1,
							    MISS_SUB => $c2,
							    ERR_SUB => $c3,
							    FA_SUB => $c4};
	  $mapped_document_values{$ref_id}{$tst_id}{$doc} = $value;
	  $mapped_values{$ref_id}{$tst_id} += $value;
	  $mention_map{$ref_id}{$tst_id}{$doc} = $mention_map;
	  $name_map{$ref_id}{$tst_id}{$doc} = $name_map if $name_map;
	  $candidate_tst_ref_pairs{$tst_id}{$ref_id} = 1;
	}
      }
    }
  }

# add in ref-sys pair costs due to document-level false alarms
  while (my ($tst_id, $ref_ids) = each %candidate_tst_ref_pairs) {
    foreach my $doc (keys %{$tsts->{$tst_id}{documents}}) {
      foreach my $ref_id (keys %$ref_ids) {
	next if defined $mapped_document_values{$ref_id}{$tst_id}{$doc};
	my $fa_value = $tsts->{$tst_id}{documents}{$doc}{FA_VALUE};
	$mapped_document_values{$ref_id}{$tst_id}{$doc} = $fa_value;
	$mapped_values{$ref_id}{$tst_id} += $fa_value;
      }
    }
  }

  check_for_duplicates ($tsts) if $opt_c;
}

#################################

sub reset_element_values {

  my ($element) = @_;

  delete $element->{VALUE};
  delete $element->{FA_VALUE};
  while (my ($doc, $doc_element) = each %{$element->{documents}}) {
    delete $doc_element->{element_value};
    delete $doc_element->{mentions_value};
    delete $doc_element->{VALUE};
    delete $doc_element->{FA_VALUE};
    delete $doc_element->{fa_scores};
    foreach my $mention (@{$doc_element->{mentions}}) {
      delete $mention->{self_score};
      delete $mention->{is_ref_mention};
      delete $mention->{tst_scores};
    }
  }
}

#################################

sub candidate_element_pairs {

  my ($doc, $ref_elements, $tst_elements, $scorer) = @_;

  my @events;
  foreach my $ref (@$ref_elements) {
    my $doc_ref = $ref->{documents}{$doc};
    foreach my $mention (@{$doc_ref->{mentions}}) {
      undef $mention->{tst_scores};
      my $locator = $mention->{head} ? $mention->{head}{locator} : $mention->{extent}{locator};
      push @events, {TYPE => "REF", EVENT => "BEG", MENTION => $mention, ID => $ref->{ID}, LOCATOR => $locator};
      push @events, {TYPE => "REF", EVENT => "END", MENTION => $mention, ID => $ref->{ID}, LOCATOR => $locator};
    }
  }

  foreach my $tst (@$tst_elements) {
    my $doc_tst = $tst->{documents}{$doc};
    foreach my $mention (@{$doc_tst->{mentions}}) {
      undef $mention->{is_ref_mention};
      my $locator = $mention->{head} ? $mention->{head}{locator} : $mention->{extent}{locator};
      push @events, {TYPE => "TST", EVENT => "BEG", MENTION => $mention, ID => $tst->{ID}, LOCATOR => $locator};
      push @events, {TYPE => "TST", EVENT => "END", MENTION => $mention, ID => $tst->{ID}, LOCATOR => $locator};
    }
  }
  @events = sort compare_locator_events @events;

  my (%active_ref_events, %active_tst_events, %overlapping_mentions, %overlapping_elements, @output_pairs);
  foreach my $event (@events) {
    my ($ref_event, $tst_event, $ref_mention, $tst_mention);
    ($event->{TYPE} eq "REF" ? ($ref_event, $ref_mention) :
                               ($tst_event, $tst_mention)) = ($event, $event->{MENTION});
    my $active_events = $ref_event ? \%active_ref_events : \%active_tst_events;
    $event->{EVENT} eq "BEG" ? ($active_events->{$event->{MENTION}} = $event) :
                        (delete $active_events->{$event->{MENTION}});
    foreach my $new_event ($ref_event ? (values %active_tst_events) : (values %active_ref_events)) {
      ($event->{TYPE} eq "REF" ? ($tst_event, $tst_mention) :
	                         ($ref_event, $ref_mention)) = ($new_event, $new_event->{MENTION});
      next if defined $overlapping_mentions{$ref_mention}{$tst_mention};
      $overlapping_mentions{$ref_mention}{$tst_mention} = 1;
      my $score  = &$scorer ($ref_mention, $tst_mention);
      next unless $score;
      $ref_mention->{tst_scores}{$tst_mention} = $score;
      $tst_mention->{is_ref_mention} = 1;
      my $ref_id = $ref_event->{ID};
      my $tst_id = $tst_event->{ID};
      next if $overlapping_elements{$ref_id}{$tst_id};
      $overlapping_elements{$ref_id}{$tst_id} = 1;
      push @output_pairs, [$ref_id, $tst_id];
    }
  }
  return [@output_pairs];
}

#################################

sub compare_locator_events {

  my $alocator = $a->{LOCATOR};
  my $blocator = $b->{LOCATOR};
  my $abeg = $a->{EVENT} eq "BEG";
  my $bbeg = $b->{EVENT} eq "BEG";

  my $cmp;
  if (($alocator->{data_type} eq "text" and $blocator->{data_type} eq "text") or
      ($alocator->{data_type} eq "audio" and $blocator->{data_type} eq "audio")) {
    return $cmp if $cmp = (($abeg ? $alocator->{start} : $alocator->{end}) <=>
			   ($bbeg ? $blocator->{start} : $blocator->{end}));
  } elsif ($alocator->{data_type} eq "image" and $blocator->{data_type} eq "image") {
    my $minmax = $abeg ? \&min : \&max;
    my $apage = &$minmax (keys %{$alocator->{pages}});
    $minmax = $bbeg ? \&min : \&max;
    my $bpage = &$minmax (keys %{$blocator->{pages}});
    return $cmp if $cmp = $apage <=> $bpage;
    $apage = $alocator->{pages}{$apage};
    $bpage = $blocator->{pages}{$bpage};
    return $cmp if $cmp = (($abeg ? $apage->{xmin} : $apage->{xmax}) <=>
			   ($bbeg ? $bpage->{xmin} : $bpage->{xmax}));
  } else {
    die "\n\nFATAL ERROR in compare_locator_events\n";
  }
  return $cmp if $cmp = $a->{EVENT} cmp $b->{EVENT};
  return $cmp = $a->{TYPE} cmp $b->{TYPE};
}

#################################

sub map_elements {

  my ($ref_objects, $tst_objects, $map_internals) = @_;

  my %reversed_scores;
  foreach my $ref (keys %$ref_objects) {
    while (my ($tst, $score) = each %{$mapped_values{$ref}}) {
      $reversed_scores{$tst}{$ref} = $score;
    }
  }

#group objects into cohort sets and map each set independently
  foreach my $object (values %$ref_objects) {
    next if exists $object->{cohort};
    my (@ref_cohorts, @tst_cohorts);
    get_cohorts ($object, \%mapped_values, \%reversed_scores, \@ref_cohorts, \@tst_cohorts, $ref_objects, $tst_objects);
    map_cohorts (\@ref_cohorts, \@tst_cohorts, \%mapped_values);

    foreach my $ref (@ref_cohorts) {
      my $tst = $ref->{MAP};
      &$map_internals ($ref, $tst) if $tst;
      $mapped_refs{$ref->{ID}} = $tst;
    }
  }
}

#################################

sub get_cohorts {
  my ($ref, $scores, $reversed_scores, $ref_cohorts, $tst_cohorts, $ref_db, $tst_db) = @_;

  my (%tst_map, %ref_map, @queue);
  @queue = ($ref->{ID}, 1);
  $ref->{cohort} = 1;
  $ref->{mapped} = 1;
  push @$ref_cohorts, $ref;
  $ref_map{$ref->{ID}} = 1;

  while (@queue > 0) {
    (my $id, my $ref_type) = splice @queue, 0, 2;
    if ($ref_type) { #find tst cohorts for this ref
      foreach my $tst_id (keys %{$scores->{$id}}) {
	next if defined $tst_map{$tst_id} or not defined $scores->{$id}{$tst_id};
	$tst_map{$tst_id} = 1;
	my $tst = $tst_db->{$tst_id};
	$tst->{cohort} = 1;
	$tst->{mapped} = 1;
	push @$tst_cohorts, $tst;
	splice @queue, scalar @queue, 0, $tst_id, 0;
      }
    } else { #find ref cohorts for this tst
      foreach my $ref_id (keys %{$reversed_scores->{$id}}) {
	next if defined $ref_map{$ref_id} or not defined $reversed_scores->{$id}{$ref_id};
	$ref_map{$ref_id} = 1;
	my $ref = $ref_db->{$ref_id};
	$ref->{cohort} = 1;
	$ref->{mapped} = 1;
	push @$ref_cohorts, $ref;
	splice @queue, scalar @queue, 0, $ref_id, 1;
      }
    }
  }
}

#################################

sub map_cohorts {

  my ($ref_cohorts, $tst_cohorts, $scores) = @_;

  my ($i, $j, $ref_id, $tst_id, %costs, $fa_value);

  #compute mapping costs
  for ($i=0; $i<@$ref_cohorts; $i++) {
    $ref_id = $ref_cohorts->[$i]{ID};
    for ($j=0; $j<@$tst_cohorts; $j++) {
      $tst_id = $tst_cohorts->[$j]{ID};
      $costs{$i}{$j} = $tst_cohorts->[$j]{FA_VALUE} - $scores->{$ref_id}{$tst_id} if
	defined $scores->{$ref_id}{$tst_id};
    }
  }
	    
  my $map = weighted_bipartite_graph_matching(\%costs) or die
    "\n\nFATAL ERROR:  BGM FAILED in map_cohorts\n";

  foreach $i (keys %$map) {
    $j = $map->{$i};
    $ref_cohorts->[$i]{MAP} = $tst_cohorts->[$j];
    $tst_cohorts->[$j]{MAP} = $ref_cohorts->[$i];
  }
}

#################################

sub map_doc_entities {

  my ($ref_entity, $tst_entity) = @_;

  foreach my $doc (keys %{$ref_entity->{documents}}) {
    my $ref_occ = $ref_entity->{documents}{$doc};
    my $tst_occ = $tst_entity->{documents}{$doc};
    next unless $tst_occ;
    $ref_occ->{MAP} = $tst_occ;
    $tst_occ->{MAP} = $ref_occ;

    my %mapping_costs;
    if ($use_mentions) { #map mentions
      my $map = $mention_map{$ref_occ->{ID}}{$tst_occ->{ID}}{$doc};
      my $ref_mentions = $ref_occ->{mentions};
      my $tst_mentions = $tst_occ->{mentions};
      foreach my $i (keys %$map) {
	my $j = $map->{$i};
	$ref_mentions->[$i]{MAP} = $tst_mentions->[$j];
	$tst_mentions->[$j]{MAP} = $ref_mentions->[$i];
      }
    }
  }
}

#################################

sub element_document_value {
  
  my ($ref, $tst, $doc) = @_;

  my $compute_fa = not $ref;
  my $doc_ref = $ref && $ref->{documents}{$doc} ? $ref->{documents}{$doc} : undef;
  my $doc_tst = $tst && $tst->{documents}{$doc} ? $tst->{documents}{$doc} : undef;

  return undef if not $doc_tst;
  $doc_ref = $doc_tst if $compute_fa;
  return undef if not $doc_ref;
  my ($element_value, $name_map) = element_value ($doc_ref, $doc_tst);
  $doc_ref->{element_value} = $element_value if $doc_ref eq $doc_tst;
  my ($mentions_value, $mentions_map, @costs) = element_mentions_value ($doc_ref, $doc_tst, $element_value);
  return undef unless $element_value;
  my $type = $doc_ref->{ELEMENT_TYPE};
  my $fa_wgt = ($type =~ /entity/ ? $entity_fa_wgt :
		$type =~ /value/ ? $value_fa_wgt : 
		$type =~ /timex2/ ? $timex2_fa_wgt :
		die "FATAL ERROR in element_document_value:  unknown element type '$type'");
  $mentions_value *= -$fa_wgt if $compute_fa;
  my $norm = $doc_ref->{ELEMENT_TYPE} =~ /entity/ && $level_weighting ?
      $entity_mention_type_wgt{$doc_ref->{LEVEL}}/$doc_ref->{mentions_value} : 1;
  my $value = $norm*$element_value*$mentions_value;
  return $value if $doc_ref eq $doc_tst;
  
  $arg_document_values{$ref->{ID}}{$tst->{ID}}{$doc} = 
    $arg_valuation eq "BOTH" || $doc_ref->{ELEMENT_TYPE} !~ /entity/ ? $value :
    ($arg_valuation eq "ATTRIBUTES" ? $element_value*
     ($level_weighting ? $entity_mention_type_wgt{$doc_ref->{LEVEL}} : $doc_ref->{mentions_value}) :
     ($arg_valuation eq "MENTIONS" ? $doc_ref->{element_value}*$value/$element_value :
      $doc_ref->{VALUE}));
  my $return_value = $value;
  if ($use_Fmeasure) {
    my $precision = $value/$doc_tst->{VALUE};
    my $recall = $value/$doc_ref->{VALUE};
    $return_value = 2*$precision*$recall/($precision+$recall);
  }
  return ($return_value, $mentions_map, $name_map, @costs);
 }

#################################

sub element_value {

  my ($doc_ref, $doc_tst) = @_;

  $doc_ref = $doc_tst unless $doc_ref;
  my $type = $doc_ref->{ELEMENT_TYPE};
  my $element_value = $epsilon;
  my ($name_similarity, $name_map);
  if ($type =~ /timex2/) {
    while (my ($attribute, $weight) = each %timex2_attribute_wgt) {
      next unless defined $doc_ref->{$attribute} and defined $doc_tst->{$attribute};
      $element_value += $weight * ($doc_ref->{$attribute} eq $doc_tst->{$attribute} ? 1 : $epsilon)
    }
  } elsif ($type =~ /entity/) {
    my $value = (min($entity_type_wgt{$doc_ref->{TYPE}},$entity_type_wgt{$doc_tst->{TYPE}}) *
		 min($entity_class_wgt{$doc_ref->{CLASS}},$entity_class_wgt{$doc_tst->{CLASS}}));
    $element_value += $value;
    while (my ($attribute, $weight) = each %entity_err_wgt) {
      $element_value *= $weight if (($doc_ref->{$attribute} xor $doc_tst->{$attribute}) or
				    ($doc_ref->{$attribute} and $doc_ref->{$attribute} ne $doc_tst->{$attribute}));
    }
    return undef unless $element_value;
    ($name_similarity, $name_map) = entity_name_similarity ($doc_ref, $doc_tst);
    $element_value *= $name_similarity if $use_name_similarity;
  } elsif ($type =~ /value/) {
    my $value = min($value_type_wgt{$doc_ref->{TYPE}},$value_type_wgt{$doc_tst->{TYPE}});
    $element_value += $value;
    while (my ($attribute, $weight) = each %value_err_wgt) {
      $element_value *= $weight if (($doc_ref->{$attribute} xor $doc_tst->{$attribute}) or
				    ($doc_ref->{$attribute} and $doc_ref->{$attribute} ne $doc_tst->{$attribute}));
    }
  } else {
    die "\n\nFATAL ERROR in element_value:  unknown element type '$type'";
  }
  return ($element_value, $name_map);
}

#################################

sub element_mentions_value {

  my ($doc_ref, $doc_tst, $element_value) = @_;

  if (not $use_mentions) {
    $doc_ref->{mentions_value} = $doc_tst->{mentions_value} = 1;
    return (1, undef, $doc_ref->{element_value}-$element_value, 0, 0, 0, 0);
  }
  my $type = $doc_ref->{ELEMENT_TYPE};
  my ($fa_wgt, $coref_fa_wgt) =
    ($type =~ /entity/ ? ($entity_fa_wgt, $entity_mention_coref_fa_wgt) :
     ($type =~ /value/ ? ($value_fa_wgt, $value_mention_coref_fa_wgt) :
      ($type =~ /timex2/ ? ($timex2_fa_wgt, $timex2_mention_coref_fa_wgt) :
       die "FATAL ERROR in element_mentions_value:  unknown element type '$type'")));

  $fa_wgt = 0 if $use_Fmeasure;
  my $mention_scorer = $type =~ /entity/ ? \&entity_mention_score : \&value_mention_score;
  foreach my $doc_element ($doc_ref, $doc_tst) {
    next if defined $doc_element->{mentions_value};
    my $mentions = $doc_element->{mentions};
    for (my $j=0; $j<@$mentions; $j++) {
      $mentions->[$j]{self_score} = &$mention_scorer ($mentions->[$j], $mentions->[$j])
	unless defined $mentions->[$j]{self_score};
      $doc_element->{mentions_value} += $mentions->[$j]{self_score};
    }
  }
    
  my $norm = 1;
  my $fa_norm = $element_value > 0 ? $doc_tst->{element_value}/$element_value : 9E9;
  if ($type =~ /entity/ and $level_weighting) {
    $norm *= $entity_mention_type_wgt{$doc_ref->{LEVEL}}/$doc_ref->{mentions_value};
    $fa_norm *= $norm > 0 ? ($entity_mention_type_wgt{$doc_tst->{LEVEL}}/$doc_tst->{mentions_value}/
			     ($entity_mention_type_wgt{$doc_ref->{LEVEL}}/$doc_ref->{mentions_value})) : 9E9;
  }
  my ($mentions_value, $mentions_map);
  my $tst_mentions = $doc_tst->{mentions};
  if ($doc_ref eq $doc_tst) { #compute self-score
    for (my $j=0; $j<@$tst_mentions; $j++) {
      $tst_mentions->[$j]{self_score} = &$mention_scorer ($tst_mentions->[$j], $tst_mentions->[$j])
	unless defined $tst_mentions->[$j]{self_score};
      $mentions_value += $tst_mentions->[$j]{self_score};
      $mentions_map->{$j} = $j;
    }
    return ($mentions_value, $mentions_map);
  }

  if (not defined $doc_tst->{fa_scores}) {
    foreach my $mention (@$tst_mentions) {
      my $fa_score = -$fa_wgt*$mention->{self_score};
      $fa_score *= $coref_fa_wgt if $mention->{is_ref_mention};
      push @{$doc_tst->{fa_scores}}, $fa_score;
    }
  }
  my %mapping_costs;
  my $fa_scores = $doc_tst->{fa_scores};
  my $ref_mentions = $doc_ref->{mentions};
  for (my $j=0; $j<@$tst_mentions; $j++) {
    for (my $i=0; $i<@$ref_mentions; $i++) {
      next unless defined (my $tst_scores = $ref_mentions->[$i]{tst_scores});
      next unless defined (my $tst_score = $tst_scores->{$tst_mentions->[$j]});
      $mapping_costs{$j}{$i} = $fa_norm*$fa_scores->[$j] - $tst_score;
    }
  }
  return undef unless %mapping_costs;
  my $map = weighted_bipartite_graph_matching(\%mapping_costs) or die
    "\n\nFATAL ERROR:  BGM FAILED in element_mentions_value\n";
  my ($ref_mentions_value, $tst_mentions_value);
  for (my $j=0; $j<@$tst_mentions; $j++) {
    $mentions_value += $fa_norm*$fa_scores->[$j];
    next unless defined (my $i = $map->{$j});
    $mentions_map->{$i} = $j;
    $mentions_value -= $mapping_costs{$j}{$i};
    $ref_mentions_value += $ref_mentions->[$i]{self_score};
    $tst_mentions_value += $ref_mentions->[$i]{tst_scores}{$tst_mentions->[$j]};
  }
  my $value = $norm*$element_value*$mentions_value;
  return ($mentions_value, $mentions_map,
	  max(0,$norm*($doc_ref->{element_value} - $element_value)*$doc_ref->{mentions_value}),
	  max(0,$norm*$element_value*($doc_ref->{mentions_value} - $ref_mentions_value)),
	  max(0,$norm*$element_value*($ref_mentions_value - $tst_mentions_value)),
	  max(0,$norm*$element_value*$tst_mentions_value - $value));
}

#################################

sub entity_name_similarity {

  my ($doc_ref, $doc_tst) = @_;

  return 1 if ($doc_ref->{LEVEL} ne "NAM" and
	       $doc_tst->{LEVEL} ne "NAM");
  return 0 if ($doc_ref->{LEVEL} ne "NAM" or
	       $doc_tst->{LEVEL} ne "NAM");

  my (%mapping_costs, @best_ref_score, @best_tst_score, $cum_score);
  my ($ref_names, $tst_names) = ($doc_ref->{names}, $doc_tst->{names}); 
  for (my $i=0; $i<@$ref_names; $i++) {
    for (my $j=0; $j<@$tst_names; $j++) {
      my $score = name_similarity ($ref_names->[$i], $tst_names->[$j]);
      $score = 0 if $score < $min_name_similarity;
      $mapping_costs{$j}{$i} = -$score if $score;
      $best_ref_score[$i] = $score if !$best_ref_score[$i] || $score > $best_ref_score[$i];
      $best_tst_score[$j] = $score if !$best_tst_score[$j] || $score > $best_tst_score[$j];
    }
    $cum_score += $best_ref_score[$i] if defined $best_ref_score[$i];
  }
  return 0 unless %mapping_costs;
  for (my $j=0; $j<@$tst_names; $j++) {
      $cum_score += $best_tst_score[$j] if defined $best_tst_score[$j];
  }
  $cum_score /= (@$ref_names + @$tst_names);

  my $map = weighted_bipartite_graph_matching(\%mapping_costs) or die
    "\n\nFATAL ERROR:  BGM FAILED in entity_name_similarity\n";
  my $name_map;
  for (my $j=0; $j<@$tst_names; $j++) {
    $name_map->{$map->{$j}} = $j if defined $map->{$j};
  }
  return ($cum_score, $name_map, \@best_ref_score, \@best_tst_score);
}

#################################

sub name_similarity {

  my ($name1, $name2) = @_;

  my @chrs = split //, $name1;
  my $nc = @chrs;
  my @chrs1 = split //, $name2; 
  my $nc1 = @chrs1;
  my $cost = 1;
  my @c;
  $c[0][0] = 0;
  foreach my $ic (1 .. $nc)  {$c[$ic][0]=$ic*$cost}
  foreach my $jc (1 .. $nc1) {$c[0][$jc]=$jc*$cost}
  foreach my $ic (1 .. $nc) {
    foreach my $jc (1 .. $nc1) {
      my $c = $c[$ic-1][$jc-1];
      $c += $cost if defined $c and $chrs[$ic-1] ne $chrs1[$jc-1];
      my $c_ins = $c[$ic-1][$jc];
      if (defined $c_ins) {
	$c_ins += $cost;
	$c = $c_ins if not defined $c or $c_ins < $c;
      }
      my $c_del = $c[$ic][$jc-1];
      if (defined $c_del) {
	$c_del += $cost;
	$c = $c_del if not defined $c or $c_del < $c;
      }
      $c[$ic][$jc] = $c;
    }
  }
  my $similarity = 1 - $c[$nc][$nc1]/max($nc,$nc1);
  return $similarity;
}

#################################

sub entity_mention_score {

#N.B.  The mention mapping score must be undef if tst doesn't match ref.

  my ($ref_mention, $tst_mention) = @_;

  return $entity_mention_type_wgt{$ref_mention->{TYPE}} + $epsilon if $ref_mention eq $tst_mention;
  my $ref_head = $ref_mention->{head};
  my $tst_head = $tst_mention->{head};
  return undef unless $ref_head and $tst_head;
  return undef if (my $overlap = span_overlap($ref_head->{locator}, $tst_head->{locator})) < $min_overlap;
  return undef if text_match($ref_head->{locator}{text}, $tst_head->{locator}{text}) < $min_text_match;
  my $score = min($entity_mention_type_wgt{$ref_mention->{TYPE}},
		  $entity_mention_type_wgt{$tst_mention->{TYPE}});
  $score += $overlap * $epsilon;

#reduce value for errors in mention attributes
  while (my ($attribute, $weight) = each %entity_mention_err_wgt) {
    $score *= $weight if $ref_mention->{$attribute} ne $tst_mention->{$attribute};
  }
  $tst_mention_scores{$tst_mention}{$ref_mention} = 
  $ref_mention_scores{$ref_mention}{$tst_mention} = [$ref_mention, $tst_mention, $score];
  return $score;
}

#################################

sub text_match {
#This subroutine returns the maximum number of contiguous matching characters
#between two strings, expressed as a fraction of the maximum string length
  my $s1 = uc $_[0];
  my $s2 = uc $_[1];
    
#count only alphanumerics (because of legitimate variation in spaces and punctuation)
  if ($s1 !~ /([^\x{00}-\x{7f}])/ and $s2 !~ /([^\x{00}-\x{7f}])/) { #do this only for ASCII
    $s1 =~ s/[^a-z0-9]//ig;
    $s2 =~ s/[^a-z0-9]//ig;
  }
  return 0 if $s1 xor $s2;
  return 1 if $s1 eq $s2;

  ($s1, $s2) = ($s2, $s1) if length $s1 > length $s2;
  my @s1 = split //, $s1;
  my @s2 = split //, $s2;
  my $max_match = 0;
  while (@s1) {
    last if @s1 <= $max_match;
    for (my $n2=0; $n2<@s2; $n2++) {
      last if @s2-$n2 <= $max_match;
      my $n = 0;
      while ($s1[$n] eq $s2[$n2+$n]) {
	$n++;
	last if @s1 == $n or @s2 == $n2+$n;
      }
      $max_match = $n if $n > $max_match;
    }
    shift @s1;
  }
  return $max_match/max(length($s1),length($s2));
}
      
#################################

sub optimum_mentions_mapping { #find the mapping that maximizes extent overlap

  my ($ref_mentions, $tst_mentions) = @_;

  my (%mapping_costs, $total_overlap, $mentions_map);
  for (my $i=0; $i<@$ref_mentions; $i++) {
    for (my $j=0; $j<@$tst_mentions; $j++) {
      my $score = span_overlap($ref_mentions->[$i]{extent}{locator},
			       $tst_mentions->[$j]{extent}{locator});
      $mapping_costs{$j}{$i} = -$score if $score;
    }
  }
  return undef unless %mapping_costs;

  my $map = weighted_bipartite_graph_matching(\%mapping_costs) or die
    "\n\nFATAL ERROR:  BGM FAILED in optimum_mentions_mapping\n";
  for (my $j=0; $j<@$tst_mentions; $j++) {
    next unless defined (my $i = $map->{$j});
    $mentions_map->{$i} = $j;
    $total_overlap -= $mapping_costs{$j}{$i};
  }
  return $total_overlap, $mentions_map;
}

#################################

sub span_overlap { #This subroutine returns the minimum mutual overlap between two spans.

  my ($ref_locator, $tst_locator) = @_;

  if ($ref_locator->{data_type} eq "text" and $tst_locator->{data_type} eq "text") {
    return text_span_overlap ($ref_locator, $tst_locator);
  } elsif ($ref_locator->{data_type} eq "audio" and $tst_locator->{data_type} eq "audio") {
    return audio_span_overlap ($ref_locator, $tst_locator);
  } elsif ($ref_locator->{data_type} eq "image" and $tst_locator->{data_type} eq "image") {
    return image_span_overlap ($ref_locator, $tst_locator);
  } else {
    die "\n\nFATAL ERROR in span_overlap\n"
      ."    unknown or nonexistent or incompatible ref/tst locator data types\n";
  }
}

#################################

sub text_span_overlap { #This subroutine returns the minimum mutual overlap between two spans.

  my ($ref_locator, $tst_locator) = @_;
    
  my $ref_start = $ref_locator->{start};
  my $ref_end = $ref_locator->{end};
  my $tst_start = $tst_locator->{start};
  my $tst_end = $tst_locator->{end};

  my $overlap = 1 + (min($ref_end,$tst_end) -
		     max($ref_start,$tst_start));
  return $overlap <= 0 ? 0 : $overlap/max($ref_end-$ref_start+1,
					  $tst_end-$tst_start+1);
}

#################################

sub audio_span_overlap { #This subroutine returns the minimum mutual overlap between two spans.

  my ($ref_locator, $tst_locator) = @_;
    
  my $ref_start = $ref_locator->{start};
  my $ref_end = $ref_locator->{end};
  my $tst_start = $tst_locator->{start};
  my $tst_end = $tst_locator->{end};

  my $overlap = (min($ref_end,$tst_end) -
		 max($ref_start,$tst_start));
  return $overlap <= 0 ? 0 : $overlap/max($ref_end-$ref_start,
					  $tst_end-$tst_start);
}

#################################

sub image_span_overlap { #This subroutine returns the minimum mutual overlap between two image spans.

  my ($ref_locator, $tst_locator) = @_;
  my $ref_pages = $ref_locator->{pages};
  my $tst_pages = $tst_locator->{pages};

  my ($ref_area, $tst_area);
  foreach my $ref_page (values %$ref_pages) {
    if ($ref_page->{area}) {
      $ref_area += $ref_page->{area};
    } else {
      my $area;
      foreach my $ref_box (@{$ref_page->{boxes}}) {
	$area += $ref_box->{width} * $ref_box->{height};
      }
      $ref_area += $ref_page->{area} = $area;
    }
  }
  foreach my $tst_page (values %$tst_pages) {
    if ($tst_page->{area}) {
      $tst_area += $tst_page->{area};
    } else {
      my $area;
      foreach my $tst_box (@{$tst_page->{boxes}}) {
	$area += $tst_box->{width} * $tst_box->{height};
      }
      $tst_area += $tst_page->{area} = $area;
    }
  }

  my ($mutual_area, $joint_ref_area, $joint_tst_area);
  while (my ($page_num, $ref_page) = each %$ref_pages) {
    next unless my $tst_page = $tst_pages->{$page_num};
    my $rboxes = $ref_page->{boxes};
    my $tboxes = $tst_page->{boxes};
    $joint_ref_area += $ref_page->{area};
    $joint_tst_area += $tst_page->{area};
    if ($ref_page->{$tst_page}) {
      $mutual_area += $ref_page->{$tst_page}{area};
    } else {
      my %mapping_costs;
      for (my $i=0; $i<@$rboxes; $i++) {
	for (my $j=0; $j<@$tboxes; $j++) {
	  my $x1 = max($rboxes->[$i]{xstart},
		       $tboxes->[$j]{xstart});
	  my $x2 = min($rboxes->[$i]{xend},
		       $tboxes->[$j]{xend});
	  next if ($x1 > $x2);
	  my $y1 = max($rboxes->[$i]{ystart},
		       $tboxes->[$j]{ystart});
	  my $y2 = min($rboxes->[$i]{yend},
		       $tboxes->[$j]{yend});
	  next if ($y1 > $y2);
	  my $overlap_area = ($x2 - $x1) * ($y2 - $y1);
	  $mapping_costs{$j}{$i} = -$overlap_area;
	}
      }
      my $area=0;
      if (%mapping_costs) { #use the mapping that maximizes overlap on the page
	my $map = weighted_bipartite_graph_matching(\%mapping_costs) or die
	  "\n\nFATAL ERROR:  BGM FAILED in image_span_overlap\n";
	foreach my $j (keys %$map) {
	  $area -= $mapping_costs{$j}{$map->{$j}};
	}
      }
      $mutual_area += $ref_page->{$tst_page}{area} = $area;
    }
  }
  return $mutual_area ? $mutual_area/max($ref_area,$tst_area) : 0;
}

#################################

sub extent_mismatch { #This subroutine returns the maximum mismatch in the extent of two locators

  my ($ref_locator, $tst_locator) = @_;
  if ($ref_locator->{data_type} eq "text" and $ref_locator->{data_type} eq "text") {
    return text_extent_mismatch ($ref_locator, $tst_locator);
  } elsif ($ref_locator->{data_type} eq "audio" and $ref_locator->{data_type} eq "audio") {
    return audio_extent_mismatch ($ref_locator, $tst_locator);
  } elsif ($ref_locator->{data_type} eq "image" and $ref_locator->{data_type} eq "image") {
    return image_extent_mismatch ($ref_locator, $tst_locator);
  } else {
    die "\n\nFATAL ERROR in extent_mismatch\n";
  }
}

#################################

sub text_extent_mismatch { #This subroutine returns the maximum mismatch in the character extent of two text streams

  my ($ref_locator, $tst_locator) = @_;
  my $extent_mismatch = 0;

  $extent_mismatch =
    max(abs($ref_locator->{start} - $tst_locator->{start}),
	abs($ref_locator->{end} - $tst_locator->{end}))/$max_diff_chars;
}

#################################

sub audio_extent_mismatch { #This subroutine returns the maximum mismatch in the time extent of two audio signals

  my ($ref_locator, $tst_locator) = @_;
  my $extent_mismatch = 0;

  $extent_mismatch =
    max(abs($ref_locator->{start} - $tst_locator->{start}),
	abs($ref_locator->{end} - $tst_locator->{end}))/$max_diff_time;
}

#################################

sub image_extent_mismatch { #This subroutine returns the maximum mismatch in the spatial extent of two images.

  my ($ref_locator, $tst_locator) = @_;
  my $ref_pages = $ref_locator->{pages};
  my $tst_pages = $tst_locator->{pages};

  my $max_mismatch = 9999999999;
  my $extent_mismatch = 0;
  foreach my $ref_page (keys %$ref_pages) {
    return $max_mismatch unless $tst_pages->{$ref_page};
    my $rboxes = $ref_pages->{$ref_page}{boxes};
    my $tboxes = $tst_pages->{$ref_page}{boxes};
    return $max_mismatch unless @$rboxes == @$tboxes;

#find best mapping of boxes on this page
    my %mapping_costs;
    for (my $i=0; $i<@$rboxes; $i++) {
      for (my $j=0; $j<@$tboxes; $j++) {
	my $x1 = max($rboxes->[$i]{xstart},
		     $tboxes->[$j]{xstart});
	my $x2 = min($rboxes->[$i]{xend},
		     $tboxes->[$j]{xend});
	next if ($x1 > $x2);
	my $y1 = max($rboxes->[$i]{ystart},
		     $tboxes->[$j]{ystart});
	my $y2 = min($rboxes->[$i]{yend},
		     $tboxes->[$j]{yend});
	next if ($y1 > $y2);
	my $overlap_area = (($x2 - $x1) *
			    ($y2 - $y1));
	$mapping_costs{$j}{$i} = -$overlap_area;
      }
    }
    return $max_mismatch unless %mapping_costs;
    my $map = weighted_bipartite_graph_matching(\%mapping_costs) or die
      "\n\nFATAL ERROR:  BGM FAILED in image_extent_mismatch\n";

#compute extent mismatch if there is an optimum mapping for all boxes
    return $max_mismatch unless keys %$map == @$rboxes;
    foreach my $j (keys %$map) {
      my $i = $map->{$j};
      $extent_mismatch = max($extent_mismatch,
			     abs(($rboxes->[$i]{xstart}) -
				 ($tboxes->[$j]{xstart})),
			     abs(($rboxes->[$i]{xend}) -
				 ($tboxes->[$j]{xend})),
			     abs(($rboxes->[$i]{ystart}) -
				 ($tboxes->[$j]{ystart})),
			     abs(($rboxes->[$i]{yend}) -
				 ($tboxes->[$j]{yend})));
    }
  }
  return $extent_mismatch/$max_diff_xy;
}

#################################

sub max {

  my $max = shift @_;
  foreach my $x (@_) {
    $max = $x if defined $x and defined $max and $x > $max;
  }
  return $max;
}

#################################

sub min {

  my $min = shift @_;
  foreach my $x (@_) {
    $min = $x if defined $x and defined $min and $x < $min;
  }
  return $min;
}

#################################

sub get_document_data {

  my ($db, $docs, $file) = @_;
  my $data;

  open (FILE, $file) or die "\nUnable to open ACE data file '$input_file'", $usage;
  binmode (FILE, ":utf8");
  while (<FILE>) {
    $data .= $_;
  }
  close (FILE);

  my $ndocs;
  my $input_objects = extract_sgml_structure ($data);
  my %get_objects = (entity=>\&get_entities, relation=>\&get_releves, event=>\&get_releves,
		     timex2=>\&get_timex2s, value=>\&get_values);
  my %attributes = (entity=>\@entity_attributes, relation=>\@relation_attributes, event=>\@event_attributes,
		    timex2=>\@timex2_attributes, value=>\@value_attributes);
  foreach my $src (@{$input_objects->{source_file}}) { #get data for all source files
    $fatal_input_error_header = "\n\nFATAL INPUT ERROR in file '$input_file'\n";
    $data_type = lc demand_attribute ("source_file", "TYPE", $src->{attributes}, {text=>1, audio=>1, image=>1});
    my $src_file = demand_attribute ("source_file", "URI", $src->{attributes});
    my $source = demand_attribute ("source_file", "SOURCE", $src->{attributes});
    $source_type = $source if not defined $source_type;
    $source_type = "MIXED" if $source ne $source_type;
    $source_types{$source} = 1;
    foreach my $doc (@{$src->{subelements}{document}}) { #get data for all documents in the source file
      my $doc_id = demand_attribute ("document", "DOCID", $doc->{attributes});
      $input_doc = $doc_id;
      $fatal_input_error_header = "\n\nFATAL INPUT ERROR in document '$input_doc' in file '$input_file'\n";
      not defined $docs->{$doc_id} or die $fatal_input_error_header.
	"        document ID '$doc->{name}' has already been defined\n";
      $docs->{$doc_id}{SOURCE} = $source;
      $docs->{$doc_id}{FILE} = $src_file;
      $docs->{$doc_id}{TYPE} = $data_type unless exists $docs->{$doc_id}{TYPE};
      $data_type eq $docs->{$doc_id}{TYPE} or die
	"\n\nFATAL INPUT ERROR:  all data for a given document must be of the same type\n".
	"        data of type '$docs->{$doc_id}{TYPE} was previously loaded for document '$doc_id'\n".
	"        but now file '$input_file' contains data of type '$data_type'\n";
      if ($db eq \%tst_database and not $eval_docs{$doc_id}) {
	(my $warning = $fatal_input_error_header) =~ s/FATAL/WARNING: /;
	warn $warning."    system output for unevaluated document\n";
	next;
      }
      $source eq $eval_docs{$doc_id}{SOURCE} or die $fatal_input_error_header.
	"        system source ($source) disagrees with reference source ($eval_docs{$doc_id}{SOURCE})\n";
      $ndocs++;
	    
#load data into database
      foreach my $type ("entity", "relation", "event", "timex2", "value") {
	my @elements = &{$get_objects{$type}} ($doc->{subelements}{$type}, $type, $doc_id);
	my $attributes = $attributes{$type};
	my $types = $type."s";
	$types =~ s/ys$/ies/;
	foreach my $element (@elements) {
	  $db->{$types}{$element->{ID}} = {} if not defined $db->{$types}{$element->{ID}};
	  my $db_element = $db->{$types}{$element->{ID}};
	  not $db_element->{documents}{$doc_id} or die
	    "\n\nFATAL INPUT ERROR:  $type '$element->{ID}' multiply defined in document '$doc_id'\n";
	  $db_element->{documents}{$doc_id} = {%$element};
	  $db_element->{documents}{$doc_id}{SOURCE} = $source;
	  foreach my $attribute (@$attributes, "ELEMENT_TYPE") {
	    next unless defined $element->{$attribute};
	    $db_element->{$attribute} = $element->{$attribute} unless defined $db_element->{$attribute};
	    next if $element->{$attribute} eq $db_element->{$attribute};
	    if ($attribute eq "LEVEL") {
	      $db_element->{$attribute} = $element->{$attribute} if
		$entity_mention_type_wgt{$element->{LEVEL}} > $entity_mention_type_wgt{$db_element->{LEVEL}};
	    } else {
	      $element->{$attribute} eq $db_element->{$attribute} or die
		"\n\nFATAL INPUT ERROR:  attribute value conflict for attribute '$attribute'"
		." for $type '$element->{ID}' in document '$doc_id'\n"
		."    database value is '$db_element->{$attribute}'\n"
		."    document value is '$element->{$attribute}'\n";
	    }
	  }
	  promote_external_links ($db_element, $element);
	  if ($type eq "entity") {
	    $db_element->{LEVEL} = $element->{LEVEL} if not defined $db_element->{LEVEL} or
	      $entity_mention_type_wgt{$element->{LEVEL}} > $entity_mention_type_wgt{$db_element->{LEVEL}};
	  } elsif ($type =~ /^(event|relation)$/) {
	    $db_element->{arguments} = {} if not defined $db_element->{arguments};
	    while (my ($role, $ids) = each %{$element->{arguments}}) {
	      while (my ($id, $arg) = each %$ids) {
		$db_element->{arguments}{$role}{$id} = $arg;
	      }
	    }
	    delete $element->{arguments};
	    if ($type eq "relation") {
	      if ($relation_symmetry{$db_element->{TYPE}}) {
		my $argids = $db_element->{arguments}{Arg} or die
		  "\n\nFATAL INPUT ERROR:  symmetric relation '$element->{ID}' has no 'Arg' arguments\n";
		my @argids = keys %$argids;
		$argids = join "' and '", @argids;
		@argids == 2 or die
		  "\n\nFATAL INPUT ERROR:  symmetric relation '$element->{ID}' must have exactly 2 'Arg' arguments, but:\n".
		  "                            Arg arguments: '$argids'";
	      } else {
		for my $arg ("Arg-1", "Arg-2") {
		  my $argids = $db_element->{arguments}{$arg} or die
		    "\n\nFATAL INPUT ERROR:  non-symmetric relation '$element->{ID}' has no '$arg' arguments\n";
		  my @argids = keys %$argids;
		  $argids = join "' and '", @argids;
		  @argids == 1 or die
		    "\n\nFATAL INPUT ERROR:  non-symmetric relation '$element->{ID}' must have exactly 1 '$arg' arguments, but:\n".
		    "                            $arg arguments: '$argids'";
		}
	      }
	    }
	  }
	}
      }
    }
  }
  $ndocs or warn
    "\nWARNING:  file '$input_file' contains no evaluable documents\n";
}

#################################

sub promote_mentions {
#This subroutine promotes all mentions of all elements in the database of type $srcname"
#to element status and assigns the promoted mentions to a new type called "$dstname".
#Any existing elements of type "$dstname" are discarded.
#The database list of references is NOT updated.
  my ($db, $srcname, $dstname, $keep_arg_ids) = @_;

  my $db_mentions = $db->{$dstname} = {};
  foreach my $element (values %{$db->{$srcname}}) {
    my $mention_elements = promote_mentions_of_element ($db, $element, $keep_arg_ids);
    foreach my $mention_element (@$mention_elements) {
      $mention_element->{HOST_ELEMENT_ID} = $element->{ID};
      $db_mentions->{$mention_element->{ID}} = $mention_element;
    }
  }
}

#################################

sub promote_mentions_of_element {
#This subroutine promotes all mentions of an element to element status
#and creates a separate and independent copy of that element, at least down
#through all components that may be updated in the evaluation process.
#The new mention elements assigned ID's by appending " (mention)" to the
#original mention ID's and the mention itself is assigned this same ID
#but with " [mention]" appended in addition.
  my ($db, $element, $keep_arg_ids) = @_;

  my $type = $element->{ELEMENT_TYPE};
  my $new_elements;
  foreach my $doc (keys %{$element->{documents}}) {
    my $doc_element = $element->{documents}{$doc};
    my $num;
    foreach my $mention (@{$doc_element->{mentions}}) {
      my $new_mention = {%$mention};
      my $new_element = {%$element};
      my $new_doc_element = {%$doc_element};
      $new_doc_element->{mentions} = [$new_mention];
      $new_element->{ID} = $new_doc_element->{ID} = $mention->{ID};
      $new_element->{ELEMENT_TYPE} = "mention_$type";
      $new_element->{documents} = {$doc => $new_doc_element};
      $new_mention->{ID} = "$mention->{ID} mention";
      $new_mention->{ELEMENT_TYPE} = "$new_element->{ELEMENT_TYPE} mention";
      $new_mention->{host_id} = $mention->{ID};
      delete $new_mention->{arguments};
      delete $new_element->{arguments};
      delete $new_doc_element->{arguments};
      $new_element->{external_links} = $new_doc_element->{external_links} = [];
      foreach my $role (keys %{$mention->{arguments}}) {
	foreach my $id (keys %{$mention->{arguments}{$role}}) {
	  my $new_arg_mention_id = $keep_arg_ids ? $id : "$id mention";
	  my $new_arg_id = $db->{refs}{$new_arg_mention_id}{host_id};
	  $new_arg_id or die "\n\nFATAL ERROR in promote_mentions_of_element:  '$new_arg_mention_id' is not registered\n";
	  $new_element->{arguments}{$role}{$new_arg_id} = {ID=>$new_arg_id, ROLE=>$role};
	  $new_mention->{arguments}{$role}{$new_arg_mention_id} = {ID=>$new_arg_mention_id, ROLE=>$role};
	}
      }
      if ($type =~ /entity/) {
	$new_element->{LEVEL} = $new_doc_element->{LEVEL} = doc_entity_level ($new_doc_element);
	$new_doc_element->{names} = [$mention->{head}{text}] if ($mention->{TYPE} eq "NAM" and $mention->{STYLE} ne "METONYMIC");
      }
      push @$new_elements, $new_element;
      $num++;
    }
    $num or warn
      "\nWARNING:  no mentions found for '$element->{ID}' in document '$doc' in promote_mentions_of_element\n";
  }
  return $new_elements;
}

#################################

sub get_releves { #extract information for document-level relations/events

  my ($releves, $type, $doc_id) = @_;
  (my $attributes, my $arg_roles, my $get_attr) = $type eq "relation" ?
    (\%relation_attributes, \%relation_argument_roles, \&get_attribute) :
    (\%event_attributes, \%event_argument_roles, \&demand_attribute);

  my (@releves, %ids);
  foreach my $releve (@$releves) {
    my %releve;
    $releve{ELEMENT_TYPE} = $type;
    $releve{document} = $doc_id;

#get relation/event ID
    $releve{ID} = $input_element = demand_attribute ($type, "ID", $releve->{attributes});
    $fatal_input_error_header =
      "\n\nFATAL INPUT ERROR for $type '$input_element'\n	in document '$doc_id'\n	in file '$input_file'\n";
    not defined $ids{$input_element} or die $fatal_input_error_header.
      "    multiple $type definitions (only one definition allowed per document)\n";
    $ids{$input_element} = 1;

#get relation/event attributes
    $releve{TYPE} = demand_attribute ($type, "TYPE", $releve->{attributes}, $attributes->{TYPE});
    $releve{SUBTYPE} = $releve{TYPE} eq "METONYMY" ? "Unspecified" :
      demand_attribute ($type, "SUBTYPE", $releve->{attributes}, $attributes->{TYPE}{$releve{TYPE}});
    foreach my $attribute (keys %$attributes) {
      next if $attribute =~ /^(ID|TYPE|SUBTYPE)$/;
      my $value = &$get_attr ($type, $attribute, $releve->{attributes}, $attributes->{$attribute});
      $releve{$attribute} = $value ? $value : "Unspecified";
    }

    $releve{arguments} = get_arguments ($releve, $type."_argument", $arg_roles);
    next if args_have_been_discarded ($releve{arguments});
    my @mentions = get_releve_mentions ($releve, $type, $arg_roles, $doc_id);
    @mentions or $releve{TYPE} eq "METONYMY" or not $use_mentions or die
      $fatal_input_error_header."    no mentions found\n";
    $releve{mentions} = [@mentions];
    make_doc_relation_symmetric (\%releve) if $type eq "relation" and $relation_symmetry{$releve{TYPE}};
    $releve{external_links} = [get_external_links ($releve)];
    push @releves, {%releve};
  }
  return @releves;
}

#################################

sub make_doc_relation_symmetric {

  my ($doc_relation) = @_;

# replace Arg-[12] argument roles with role = "Arg"
  while (my ($role, $ids) = each %{$doc_relation->{arguments}}) {
    next unless $role =~ /^Arg-[12]$/;
    foreach my $id (keys %$ids) {
      $doc_relation->{arguments}{Arg}{$id} = {ROLE=>"Arg", ID=>$id};
    }
    delete $doc_relation->{arguments}{$role};
  }

# replace Arg-[12] mention argument roles with role = "Arg"
  foreach my $mention (@{$doc_relation->{mentions}}) {
    while (my ($role, $ids) = each %{$mention->{arguments}}) {
      next unless $role =~ /^Arg-[12]$/;
      foreach my $id (keys %$ids) {
	$mention->{arguments}{Arg}{$id} = {ROLE=>"Arg", ID=>$id};
      }
      delete $mention->{arguments}{$role};
    }
  }
}

#################################

sub args_have_been_discarded {

  my ($args) = @_;

  foreach my $role (values %$args) {
    foreach my $id (keys %$role) {
      return 1 if defined $discarded_entities{$id};
    }
    next;
  }
}

#################################

sub get_attribute {

  my ($element, $name, $attributes, $required_values) = @_;

  my $value = $attributes->{$name};
  return $value if
    not defined $value or not defined $required_values or defined $required_values->{$value};
  die $fatal_input_error_header.
    "    Unrecognized $element $name attribute (= '$value')\n";
}

#################################

sub demand_attribute {

  my ($element, $name, $attributes, $required_values) = @_;

  my $value = $attributes->{$name};
  return $value if
    (defined $value and (not defined $required_values or defined $required_values->{$value})) or
    (not defined $value and defined $required_values and keys %$required_values == 0);
  die $fatal_input_error_header.(defined $value ?
				 "    Illegal $element $name attribute (= '$value')\n" :
				 "    Missing $element $name attribute\n");
}

#################################

sub get_releve_mentions {

  my ($releve, $type, $argument_roles, $doc_id) = @_;
    
  my @mentions = ();
  foreach my $mention (@{$releve->{subelements}{$type."_mention"}}) {
    my %mention;
    $mention{document} = $doc_id;
    $mention{host_id} = $releve->{attributes}{ID};
    $mention{ID} = demand_attribute ($type."_mention", "ID", $mention->{attributes});
    $mention{arguments} = get_arguments ($mention, $type."_mention_argument", $argument_roles);
    ($mention{extent}) = get_locators ("extent", $mention) or die
      $fatal_input_error_header."    No extent found\n";
    ($mention{anchor}) = get_locators ("anchor", $mention) or die
      $fatal_input_error_header."    No anchor found\n" if $type eq "event";

    foreach my $mention (@mentions) { # warn if mention appears to be redundant
      my $different = ($type =~ /event/ and span_overlap ($mention{extent}{locator},
							  $mention->{extent}{locator}) < $min_overlap);
      next if $different;
      foreach my $role (keys %{$mention->{arguments}}) {
	foreach my $id (keys %{$mention->{arguments}{$role}}) {
	  $different = 1 unless $mention{arguments}{$role} and $mention{arguments}{$role}{$id};
	}
	last if $different;
      }
      next if $different;
      foreach my $role (keys %{$mention{arguments}}) {
	foreach my $id (keys %{$mention{arguments}{$role}}) {
	  $different = 1 unless $mention->{arguments}{$role} and $mention->{arguments}{$role}{$id};
	}
	last if $different;
      }
      next if $different;
      (my $warning = $fatal_input_error_header) =~ s/FATAL/WARNING: /;
      warn $warning."    $mention{ID} and $mention->{ID} appear to be redundant mentions\n";
      last;
    }
    push @mentions, {%mention};
  }
  return @mentions;
}

#################################

sub print_releves {

  my ($source, $type, $releves, $attributes) = @_;

  return unless $opt_s;
  print "\n======== $source $type  ========\n";
  my $db = $source eq "REF" ? \%ref_database : \%tst_database;
  foreach my $id (sort keys %$releves) {
    my $releve = $releves->{$id};
    printf "ID=%s, VALUE=%.5f, TYPE=%s, SUBTYPE=%s", $releve->{ID},
    $releve->{VALUE}, $releve->{TYPE}, $releve->{SUBTYPE};
    foreach my $attribute (@$attributes) {
      print ", $attribute=$releve->{$attribute}" unless
	$attribute =~ /^(ID|TYPE|SUBTYPE)$/ or not defined $releve->{$attribute};
    }
    print "\n";
    my @argument_info;
    foreach my $role (sort keys %{$releve->{arguments}}) {
      foreach my $id (sort keys %{$releve->{arguments}{$role}}) {
	print "  ".argument_description ($releve->{arguments}{$role}{$id}, $db);
      }
    }
    print_releve_mentions ($releve, $db->{refs});
  }
}

#################################

sub argument_description {

  my ($argument, $db) = @_;

  my $ref = $db->{refs}{$argument->{ID}};
  my $out = (sprintf ("%11.11s (%3.3s/%3.3s/%3.3s/%3.3s): ID=%s", $argument->{ROLE},
		      $ref->{TYPE}, $ref->{SUBTYPE} ? $ref->{SUBTYPE} : "---",
		      $ref->{LEVEL} ? $ref->{LEVEL} : "---", $ref->{CLASS} ? $ref->{CLASS} : "---", $ref->{ID}));
  my $data;
  my ($tag, $value) = (($data=longest_name($ref)) ? ("name", $data) :
		      (($data=longest_mention($ref, "head")) ? ("head", $data) :
		       (($data=longest_mention($ref, "extent")) ? ("extent", $data) : ("???", ""))));
  $out .= ", $tag=\"$data\"" if $value;
  return "$out\n";
}	

#################################

sub limit_string {

  my ($string, $max_length) = @_;

  my $max = $max_length ? $max_length : $max_string_length_to_print;
  return length $string <= $max ? $string : substr ($string, 0, $max-3)."...";
}

#################################

sub get_arguments {

  my ($releve, $name, $roles) = @_;

  my (%arguments, %arg_ids);
  foreach my $arg (@{$releve->{subelements}{$name}}) {
    my %arg;
    $arg{ID} = demand_attribute ($name, "REFID", $arg->{attributes});
    $arg{ROLE} = demand_attribute ($name, "ROLE", $arg->{attributes}, $roles);
    next unless $score_relation_times or $name !~ /relation/i or $arg{ROLE} !~ /time/i;
    not $arguments{$arg{ROLE}}{$arg{ID}} or die $fatal_input_error_header.
      "    multiple declarations of argument $arg{ID} in role $arg{ROLE}\n";
    $arguments{$arg{ROLE}}{$arg{ID}} = {%arg};
    (my $warning = $fatal_input_error_header) =~ s/FATAL/WARNING: /;
    $name !~ /relation/ or not $arg_ids{$arg{ID}} or warn $warning.
      "    $arg{ID} appears as a $name multiple times\n";
    $arg_ids{$arg{ID}} = 1;
  }
  return {%arguments};
}

#################################

sub print_releve_mentions {

  my ($element, $ref_refs) = @_;

  while (my ($doc, $doc_element) = each %{$element->{documents}}) {
    print "      -- in document $doc\n";
    foreach my $mention (@{$doc_element->{mentions}}) {
      printf "         mention ID=%s", $mention->{ID};
      print ", anchor=\"$mention->{anchor}{text}\"" if $mention->{anchor};
      print $mention->{extent}{text} ? ", extent=\"$mention->{extent}{text}\"\n" : "\n";
      foreach my $role (sort keys %{$mention->{arguments}}) {
	foreach my $id (sort keys %{$mention->{arguments}{$role}}) {
	  my $ref = $ref_refs->{$mention->{arguments}{$role}{$id}{ID}};
	  printf "%21s: ID=%s%s", $role, $id, ($ref->{head} ? ", head=\"$ref->{head}{text}\"\n" :
					       ($ref->{extent} ? ", extent=\"$ref->{extent}{text}\"\n" : "\n"));
	}
      }
    }
  }
}

#################################

sub compute_releve_values {

  my ($refs, $tsts) = @_;

#select putative REF-TST element pairs for mapping
  my %putative_pairs;
  foreach my $doc (keys %eval_docs) {
    my %doc_refs;
    while (my ($ref_id, $ref) = each %$refs) {
      $doc_refs{$ref_id} = 1 if defined $ref->{documents}{$doc};
    }
    next unless %doc_refs;
    while (my ($tst_id, $tst) = each %$tsts) {
      next unless defined $tst->{documents}{$doc};
      foreach my $ref_id (keys %doc_refs) {
	$putative_pairs{$ref_id}{$tst_id} = $tst;
      }
    }
  }

#compute self-values
  foreach my $elements ($refs, $tsts) {
    foreach my $element (values %$elements) {
      while (my ($doc, $doc_element) = each %{$element->{documents}}) {
	next unless defined $eval_docs{$doc};
	$doc_element->{VALUE} = releve_document_value ($element, undef, $doc);
	$element->{VALUE} += $doc_element->{VALUE};
	next if $elements eq $refs;
	$doc_element->{FA_VALUE} = releve_document_value (undef, $element, $doc);
	$element->{FA_VALUE} += $doc_element->{FA_VALUE};
      }
    }
  }

#compute mapped element values
  while (my ($ref_id, $putative_tsts) = each %putative_pairs) {
    my $ref = $refs->{$ref_id};
    while (my ($tst_id, $tst) = each %$putative_tsts) {
      my $arg_map = compute_argument_map ($ref, $tst);
      $argument_map{$ref_id}{$tst_id} = $arg_map if $arg_map;
      my %docs;
      map $docs{$_}=1, keys %{$ref->{documents}};
      map $docs{$_}=1, keys %{$tst->{documents}};
      foreach my $doc (keys %docs) {
	next unless defined $tst->{documents}{$doc};
	my ($value, $map, $c1, $c2, $c3, $c4) = releve_document_value ($ref, $tst, $doc, $arg_map);
	next unless defined $value;
	$mapped_document_costs{$ref_id}{$tst_id}{$doc} = {ATTR_ERR => $c1,
							  MISS_SUB => $c2,
							  ERR_SUB => $c3,
							  FA_SUB => $c4};
	$mapped_document_values{$ref_id}{$tst_id}{$doc} = $value;
	$mapped_values{$ref_id}{$tst_id} += $value;
	$mention_map{$ref_id}{$tst_id}{$doc} = $map;
      }
    }
  }

  check_for_duplicates ($tsts) if $opt_c;
}

#################################

sub map_releve_arguments {

  my ($ref, $tst) = @_;

  my $arg_map = $argument_map{$ref->{ID}}{$tst->{ID}};

#map arguments
  while (my ($tst_role, $tst_ids) = each %$arg_map) {
    while (my ($tst_id, $ref_map) = each %$tst_ids) {
      (my $ref_arg, my $tst_arg) = ($ref->{arguments}{$ref_map->{ROLE}}{$ref_map->{ID}},
				    $tst->{arguments}{$tst_role}{$tst_id});
      next unless $ref_arg and $tst_arg;
      $ref_arg->{MAP} = $tst_arg;
      $tst_arg->{MAP} = $ref_arg;
    }
  }

#map document level info
  foreach my $doc (keys %{$ref->{documents}}) {
    my $doc_ref = $ref->{documents}{$doc};
    my $doc_tst = $tst->{documents}{$doc};
    next unless $doc_tst;
    $doc_ref->{MAP} = $doc_tst;
    $doc_tst->{MAP} = $doc_ref;
    while (my ($tst_role, $tst_ids) = each %$arg_map) {
      while (my ($tst_id, $ref_map) = each %$tst_ids) {
	(my $ref_arg, my $tst_arg) = ($ref->{arguments}{$ref_map->{ROLE}}{$ref_map->{ID}},
				      $tst->{arguments}{$tst_role}{$tst_id});
	next unless $ref_arg and $tst_arg;
	$ref_arg->{MAP} = $tst_arg;
	$tst_arg->{MAP} = $ref_arg;
      }
    }
    my $map = $mention_map{$ref->{ID}}{$tst->{ID}}{$doc};
    next unless $map;
    while (my ($i, $j) = each %$map) {
      $doc_ref->{mentions}[$i]{MAP} = $doc_tst->{mentions}[$j];
      $doc_tst->{mentions}[$j]{MAP} = $doc_ref->{mentions}[$i];
    }
  }
}

#################################

sub releve_document_value {

  my ($ref, $tst, $doc, $arg_map) = @_;

  my $compute_fa = not $ref; #calculate FA score if ref is null
  $ref = $tst if not $ref;
  $tst = $ref if not $tst;
  my $doc_ref = $ref->{documents}{$doc};
  my $doc_tst = $tst->{documents}{$doc};
  return undef if not $doc_tst;
  $compute_fa |= not $doc_ref; #calculate FA score if doc_ref is null
  $doc_ref = $doc_tst if not $doc_ref;

  my $ref_mentions = $doc_ref->{mentions};
  my $tst_mentions = $doc_tst->{mentions};
  $ref_mentions = [] unless $ref_mentions;
  $tst_mentions = [] unless $tst_mentions;
  my $relation_type = $ref->{ELEMENT_TYPE} =~ /relation/;
  my $mention_type = $ref->{ELEMENT_TYPE} =~ /mention/;
  (my $type_wgt, my $modality_wgt, my $fa_wgt, my $role_err_wgt, my $asymmetry_wgt) =
    $relation_type ? (\%relation_type_wgt, \%relation_modality_wgt, $relation_fa_wgt,
		      $relation_argument_role_err_wgt, $relation_asymmetry_err_wgt) :
		      (\%event_type_wgt, \%event_modality_wgt, $event_fa_wgt,
		       $event_argument_role_err_wgt, 1);
  $fa_wgt = 0 if $use_Fmeasure;
  
  my ($ref_args, $tst_args) = ($ref_database{refs}, $tst_database{refs});
  if ($ref eq $tst) { #compute self-score
    my $arg_db = $compute_fa ? $tst_args : $ref_args;
    my $arg_score;
    while (my ($role, $ids) = each %{$ref->{arguments}}) {
      foreach my $id (keys %$ids) {
	$arg_score += $arg_db->{$id}{documents}{$doc}{VALUE} if
	  $arg_db->{$id}{documents}{$doc};
      }
    }
    $arg_score += $epsilon * @$ref_mentions if $ref_mentions;
    my $value = releve_value($ref,$ref) * $arg_score;
    return $compute_fa ? -$fa_wgt*$value : $value;
  }

  return undef unless $tst->{documents}{$doc};
  my $nargs_ref = 0;
  while (my ($role, $ids) = each %{$ref->{arguments}}) {
    $nargs_ref += keys %$ids;
  }
  return undef if (($argscore_required and $nargs_ref and not $arg_map) or
		   ($relation_type and
		    (($nargs_required eq "2" and (not $arg_map->{"Arg-1"} or not $arg_map->{"Arg-2"}) and
		      (not $arg_map->{"Arg"} or not keys %{$arg_map->{"Arg"}} == 2)) or
		     ($nargs_required eq "1" and (not $arg_map->{"Arg-1"} and not $arg_map->{"Arg-2"}) and
		      not $arg_map->{"Arg"}))));
  (my $mentions_overlap, my $mentions_map) = optimum_mentions_mapping ($ref_mentions, $tst_mentions);
  return undef unless $arg_map or $mentions_overlap;
  return undef if $mention_overlap_required and not $mentions_overlap;
  return undef if $mention_type and not ($relation_type ?
					 relation_mention_args_overlap ($ref_mentions, $tst_mentions) :
					 span_overlap ($ref_mentions->[0]{extent}{locator},
						       $tst_mentions->[0]{extent}{locator}));
  my ($args_value, $faargs_value, $nargs_mapped, $refargs_value, $used_refargs_value) = (0,0,0,0,0);
  while (my ($tst_role, $tst_ids) = each %{$tst->{arguments}}) {
    foreach my $tst_id (keys %$tst_ids) {
      $faargs_value += $tst_args->{$tst_id}{documents}{$doc}{VALUE} if
	$tst_args->{$tst_id}{documents}{$doc};
      next unless $ref->{documents}{$doc};
      next unless $arg_map and $arg_map->{$tst_role} and my $arg = $arg_map->{$tst_role}{$tst_id};
      (my $ref_id, my $ref_role) = ($arg->{ID}, $arg->{ROLE});
      my $values = $arg_document_values{$ref_id}{$tst_id} if $ref_id and $arg_document_values{$ref_id};
      my $map_score = $values->{$doc} if $values and defined $values->{$doc} and
	($allow_wrong_mapping or ($ref_args->{$ref_id}{MAP} and
				  $ref_args->{$ref_id}{MAP} eq $tst_args->{$tst_id}));
      next unless $map_score;
      $map_score = $ref_args->{$ref_id}{documents}{$doc}{VALUE} if
	$mention_type and $ref_args->{$ref_id}{documents}{$doc};
      $map_score *= !$relation_type ? $role_err_wgt : $ref_role =~ /^Arg-[12]$/ ? $asymmetry_wgt : $role_err_wgt
	if $ref_role ne $tst_role;
      $args_value += $map_score;
      $nargs_mapped++;
      $faargs_value -= $tst_args->{$tst_id}{documents}{$doc}{VALUE} if
	$tst_args->{$tst_id}{documents}{$doc};
      $used_refargs_value += $ref_args->{$ref_id}{documents}{$doc}{VALUE} if
	$ref_args->{$ref_id}{documents}{$doc};
    }
  }
  return undef if $argscore_required eq "ALL" and $nargs_mapped != $nargs_ref;
  return undef unless $args_value or $mentions_overlap;
  $args_value += $mentions_overlap * $epsilon if $mentions_overlap;
  my $releve_value = releve_value($ref,$tst);
  return undef unless $releve_value;
  while (my($ref_role, $ref_ids) = each %{$ref->{arguments}}) {
    foreach my $ref_id (keys %$ref_ids) {
      $refargs_value += $ref_args->{$ref_id}{documents}{$doc}{VALUE} if
	$ref_args->{$ref_id}{documents}{$doc};
    }
  }
  my $v1 = releve_value($ref,$ref)*$refargs_value;
  my $v2 = $releve_value*$refargs_value;
  my $v3 = $releve_value*$used_refargs_value;
  my $v4 = $releve_value*$args_value;
  my $v5 = $v4 - $fa_wgt*releve_value($tst,$tst)*$faargs_value;
  my $return_value = $v5;
  if ($use_Fmeasure) {
    my $precision = $v5/$doc_tst->{VALUE};
    my $recall = $v5/$doc_ref->{VALUE};
    $return_value = 2*$precision*$recall/($precision+$recall);
  }
  return ($return_value, $mentions_map, max(0,$v1-$v2), max(0,$v2-$v3), max(0,$v3-$v4), max(0,$v4-$v5));
}

#################################

sub releve_value {

  my ($ref, $tst) = @_;

  (my $type_wgt, my $modality_wgt, my $err_wgt) = $ref->{ELEMENT_TYPE} =~ /relation/ ?
    (\%relation_type_wgt, \%relation_modality_wgt, \%relation_err_wgt) :
    (\%event_type_wgt, \%event_modality_wgt, \%event_err_wgt);

  my $value = (min($type_wgt->{$ref->{TYPE}},$type_wgt->{$tst->{TYPE}})*
	       min($modality_wgt->{$ref->{MODALITY}},$modality_wgt->{$tst->{MODALITY}}));
  return $value if $ref eq $tst;

#reduce value for errors in attributes
  while (my ($attribute, $weight) = each %$err_wgt) {
    $value *= $weight if (($ref->{$attribute} xor $tst->{$attribute}) or
			  ($tst->{$attribute} and $ref->{$attribute} ne $tst->{$attribute}));
  }
  return $value
}

#################################

sub compute_argument_map {

  my ($ref, $tst) = @_;

#collect arguments
  my (@ref_args, @tst_args);
  while (my ($role, $ids) = each %{$ref->{arguments}}) {
    foreach my $id (keys %$ids) {
      push @ref_args, {ID => $id, ROLE => $role};
    }
  }
  return undef unless @ref_args;
  while (my ($role, $ids) = each %{$tst->{arguments}}) {
    foreach my $id (keys %$ids) {
      push @tst_args, {ID => $id, ROLE => $role};
    }
  }
  return undef unless @tst_args;

#compute argument mapping scores
  my $sysref_value = releve_value ($ref, $tst);
  my $sys_value = releve_value ($tst, $tst);
  my %mapping_costs;
  (my $role_err_wgt, my $fa_wgt, my $asymmetry_wgt) = $ref->{ELEMENT_TYPE} =~ /relation/ ?
    ($relation_argument_role_err_wgt, $relation_fa_wgt, $relation_asymmetry_err_wgt) :
    ($event_argument_role_err_wgt, $event_fa_wgt, 1);
  for (my $i=0; $i<@ref_args; $i++) {
    my $ref_id = $ref_args[$i]->{ID};
    for (my $j=0; $j<@tst_args; $j++) {
      my $tst_id = $tst_args[$j]->{ID};
      my $values = $arg_document_values{$ref_id}{$tst_id} if $arg_document_values{$ref_id};
      next unless $values and 
	(($allow_wrong_mapping or ($ref_database{refs}{$ref_id}{MAP} and
				   $ref_database{refs}{$ref_id}{MAP} eq $tst_database{refs}{$tst_id})));
      my ($map_score, $sys_score);
      foreach my $doc (keys %{$tst->{documents}}) {
	$map_score += $values->{$doc} if $values->{$doc};
	$sys_score += $tst_database{refs}{$tst_id}{documents}{$doc}{VALUE};
      }
      next unless defined $map_score;
      $map_score *= $ref_args[$i]->{ROLE} =~ /^Arg-[12]$/ ? $asymmetry_wgt : $role_err_wgt
	if $ref_args[$i]->{ROLE} ne $tst_args[$j]->{ROLE};
      my $fa_score = -$sys_score*$fa_wgt;
      $mapping_costs{$j}{$i} = $fa_score*$sys_value - $map_score*$sysref_value;
    }
  }
  return undef unless %mapping_costs;
    
#find optimum mapping
  my $map = weighted_bipartite_graph_matching(\%mapping_costs) or die
    "\n\nFATAL ERROR:  BGM FAILED in compute_argument_map\n";

  my %arg_map;
  for (my $j=0; $j<@tst_args; $j++) {
    next unless defined (my $i = $map->{$j});
    $arg_map{$tst_args[$j]->{ROLE}}{$tst_args[$j]->{ID}} = {ROLE => $ref_args[$i]->{ROLE},
							    ID => $ref_args[$i]->{ID}};
  }
  return {%arg_map};
}

#################################

sub relation_mention_args_overlap {

  my ($ref_mentions, $tst_mentions) = @_;

  my ($id, $ids);
  my $max_overlap = 0;
  foreach my $role ("Arg", "Arg-1", "Arg-2") {
    my $ref_ids = $ref_mentions->[0]{arguments}{$role};
    next unless $ref_ids;
    foreach my $ref_id (keys %$ref_ids) {
      my $tst_ids = $tst_mentions->[0]{arguments}{$role};
      next unless $tst_ids;
      foreach my $tst_id (keys %$tst_ids) {
	my $overlap = span_overlap ($ref_database{refs}{$ref_id}{head}{locator},
				    $tst_database{refs}{$tst_id}{head}{locator});
	$max_overlap = max($overlap, $max_overlap);
      }
    }
  }
  return $max_overlap >= 0 ? 1 : 0;
}

#################################

sub print_releve_mapping {

  my ($ref_data, $tst_data, $type, $attributes) = @_;

  return unless $opt_a or $opt_e;
  print "\n======== $type mapping ========\n";

  foreach my $ref_id (sort keys %$ref_data) {
    $print_data = $opt_a;
    my $output;
    my $ref = $ref_data->{$ref_id};
    my $tst = $ref->{MAP};
    if ($tst) {
      my $tst_id = $tst->{ID};
      my $attribute_errors;
      foreach my $attribute (@$attributes) {
	next if $attribute eq "ID";
	$attribute_errors = ($ref->{$attribute} xor $tst->{$attribute} or
			     defined $tst->{$attribute} and $tst->{$attribute} ne $ref->{$attribute});
	last if $attribute_errors;
      }
      $print_data ||= $opt_e if $attribute_errors;
      $output .= ($attribute_errors ? ">>> " : "    ").
	"ref $type $ref->{ID} ".list_element_attributes($ref, $attributes);
      $output .= ($attribute_errors ? " -- ATTRIBUTE ERRORS\n" : "\n");
      $output .= ($attribute_errors ? ">>> " : "    ").
	"tst $type $tst->{ID} ".list_element_attributes($tst, $attributes);
      $output .= ($attribute_errors ? " -- ATTRIBUTE ERRORS\n" : "\n");
      $output .= sprintf ("      $type score:  %.5f out of %.5f\n",
			  $mapped_values{$ref_id}{$tst_id}, $ref->{VALUE});
    } else {
      $print_data ||= $opt_e;
      $output .= ">>> ref $type $ref->{ID} ".
	list_element_attributes($ref, $attributes)." -- NO MATCHING TST\n";
      $output .= sprintf "      $type score:  0.00000 out of %.5f\n", $ref->{VALUE};
    }
    $output .= print_argument_mapping ($ref, $tst); 

    my %docs;
    map $docs{$_}=1, keys %{$ref->{documents}} if $ref;
    map $docs{$_}=1, keys %{$tst->{documents}} if $tst;
    foreach my $doc (sort keys %docs) {
      $output .= print_releve_mention_mapping ($ref, $tst, $doc) if $eval_docs{$doc};
    }
    print $output, "--------\n" if $print_data;
  }

#print unmapped test elements
  return unless $opt_a or $opt_e;
  foreach my $tst_id (sort keys %$tst_data) {
    my $tst = $tst_data->{$tst_id};
    next if $tst->{MAP};
    printf ">>> tst $type %s ", $tst->{ID};
    printf "%s", list_element_attributes($tst, $attributes)." -- NO MATCHING REF\n";
    printf "      $type score:  %.5f out of 0.00000\n", $tst->{FA_VALUE};
    print STDOUT print_argument_mapping (undef, $tst); 

    foreach my $doc (sort keys %{$tst->{documents}}) {
      print STDOUT print_releve_mention_mapping (undef, $tst, $doc) if $eval_docs{$doc};
    }
    print "--------\n";
  }
}

#################################

sub list_element_attributes {

  my ($event, $attributes) = @_;

  my $output;
  for my $attribute ("TYPE", "SUBTYPE") {
    $output .= ($output ? "/" : "(").($event->{$attribute} ? $event->{$attribute} : "---");
  }
  for my $attribute (@$attributes) {
    next if $attribute =~ /^(TYPE|SUBTYPE|ID)$/;
    $output .= "/".($event->{$attribute} ? $event->{$attribute} : "---");
  }
  return "$output)"
}

#################################

sub print_argument_mapping {  

  my ($ref, $tst, $doc) = @_;

  my $output = "";
  my %args;
  if ($ref and $ref->{arguments}) {
    while (my ($role, $ids) = each %{$ref->{arguments}}) {
      foreach my $arg (values %$ids) {
	$args{$arg}{REF} = $arg;
      }
    }
  } elsif (not $doc) {
    $output .= "      no ref args\n" if $ref;
  }
  if ($tst and $tst->{arguments}) {
    my $reversed_order;
    while (my ($role, $ids) = each %{$tst->{arguments}}) {
      foreach my $arg (values %$ids) {
	$args{$arg->{MAP}?$arg->{MAP}:$arg}{TST} = {%$arg};
	$reversed_order = 1 if ($tst->{ELEMENT_TYPE} =~ /relation/ and
				$arg->{MAP} and
				$arg->{MAP}{ROLE} ne $role);
      }
    }
  } elsif (not $doc) {
    $output .= "      no tst args\n" if $tst;
  }
  my @args = sort {my $cmp;
		   return $cmp if $cmp = ($a->{REF}?$a->{REF}{ROLE}:$a->{TST}{ROLE}) cmp ($b->{REF}?$b->{REF}{ROLE}:$b->{TST}{ROLE});
		   return $cmp if $cmp = (defined $b->{REF} and defined $b->{TST}) cmp (defined $a->{REF} and defined $a->{TST});
		   return $cmp if $cmp = defined $b->{REF} cmp defined $a->{REF};
		   return ($a->{REF}?$a->{REF}{ID}:$a->{TST}{ID}) cmp ($b->{REF}?$b->{REF}{ID}:$b->{TST}{ID});
		 } values %args;
  foreach my $arg (@args) {
    my $err_text;
    my $ref_arg = $arg->{REF};
    my $tst_arg = $arg->{TST};
    if ($ref_arg) {
      my $arg_ref = $ref_database{refs}{$ref_arg->{ID}};
      $err_text = $tst_arg ? "" : "NO CORRESPONDING TST ARGUMENT";
      if ($ref->{ELEMENT_TYPE}!~/mention/) {
	$err_text .= ($err_text ? ", " : "")."REF ARGUMENT NOT MAPPED" if not $arg_ref->{MAP};
	$err_text .= ($err_text ? ", " : "")."REF ARGUMENT MISMAPPED" if ($arg_ref->{MAP} and $tst_arg and
									  $arg_ref->{MAP}{ID} ne $tst_arg->{ID});
      }
      $err_text .= ($err_text ? ", " : "")."ARGUMENT ROLE MISMATCH" if ($tst_arg and $ref_arg->{ROLE} ne $tst_arg->{ROLE});
      $print_data ||= $err_text;
      undef $err_text unless $tst;
      my $arg_tst = $tst_arg ? $tst_database{refs}{$tst_arg->{ID}} : undef;
      if (not $doc or
	  ($arg_ref->{documents}{$doc} and 
	   (not $arg_tst or not $arg_tst->{documents}{$doc}))) {
	$output .= ($err_text ? ">>>   ":"      ").mapped_argument_description ("ref", $ref_arg, $tst_arg, $ref, $tst, $doc);
	$output .= ($err_text && not $doc) ? " -- $err_text\n" : "\n";
      }
    }
    if ($tst_arg) {
      my $arg_tst = $tst_database{refs}{$tst_arg->{ID}};
      $err_text = $ref_arg ? "" : "NO CORRESPONDING REF ARGUMENT";
      if ($tst->{ELEMENT_TYPE}!~/mention/) {
	$err_text .= ($err_text ? ", " : "")."TST ARGUMENT NOT MAPPED" if not $arg_tst->{MAP};
	$err_text .= ($err_text ? ", " : "")."TST ARGUMENT MISMAPPED" if ($arg_tst->{MAP} and $ref_arg and
									  $arg_tst->{MAP}{ID} ne $ref_arg->{ID});
      }
      $err_text .= ($err_text ? ", " : "")."ARGUMENT ROLE MISMATCH" if ($ref_arg and $tst_arg->{ROLE} ne $ref_arg->{ROLE});
      $print_data ||= $err_text;
      undef $err_text unless $ref;
      my $arg_ref = $ref_arg ? $ref_database{refs}{$ref_arg->{ID}} : undef;
      if (not $doc or $arg_tst->{documents}{$doc}) {
	$output .= ($err_text ? ">>>   ":"      ").mapped_argument_description ("tst", $ref_arg, $tst_arg, $ref, $tst, $doc);
	$output .= ($err_text && not $doc) ? " -- $err_text\n" : "\n";
      }
    }
  }
  return $output;
}

#################################

sub mapped_argument_description {

  my ($src, $refarg, $tstarg, $ref, $tst, $doc) = @_;

  my $refarg_id = $refarg->{ID} if $refarg;
  my $tstarg_id = $tstarg->{ID} if $tstarg;
  (my $arg, my $role, my $id) = $src eq "ref" ? 
    ($ref_database{refs}{$refarg_id}, $refarg->{ROLE}, $refarg_id) :
    ($tst_database{refs}{$tstarg_id}, $tstarg->{ROLE}, $tstarg_id);
  my $role_err_wgt = (($ref and $ref->{ELEMENT_TYPE} =~ /relation/) or
		      ($tst and $tst->{ELEMENT_TYPE} =~ /relation/)) ?
		      $relation_argument_role_err_wgt : $event_argument_role_err_wgt;
  my $out = sprintf ("%11.11s", $role);
  if ($doc) {
    my $weight = ($refarg && $tstarg && $tstarg->{ROLE} ne $refarg->{ROLE}) ? $role_err_wgt : 1;
    my $score = ($tst->{documents}{$doc} && $ref->{documents}{$doc} && $refarg && $tstarg &&
		 defined $arg_document_values{$refarg_id}{$tstarg_id}{$doc}) ?
		 $arg_document_values{$refarg_id}{$tstarg_id}{$doc}*$weight :
		 ($tst->{documents}{$doc} && $tstarg &&
		  $tst_database{refs}{$tstarg_id}{documents}{$doc}) ?
		  -$tst_database{refs}{$tstarg_id}{documents}{$doc}{VALUE} : 0;
    my $refscore = ($ref->{documents}{$doc} && $refarg &&
		    $ref_database{refs}{$refarg_id}{documents}{$doc}) ?
		    $ref_database{refs}{$refarg_id}{documents}{$doc}{VALUE} : 0;
    $out .= sprintf (" score: %.5f out of %.5f, $src ID=%s", $score, $refscore, $id);
  } else {
    $out .= sprintf (" %s ID=%s (%3.3s/%3.3s", $src, $id, $arg->{TYPE},
		     $arg->{SUBTYPE} ? (substr $arg->{SUBTYPE}, 0, 7) : "---");
    $out .= $arg->{ELEMENT_TYPE} =~ /entity/ ? sprintf ("/%3.3s/%3.3s)", $arg->{LEVEL}, $arg->{CLASS}) : ")";
    my $data;
    my ($tag, $value) = (($data=longest_name($arg)) ? ("name", $data) :
			 (($data=longest_mention($arg, "head")) ? ("head", $data) :
			  (($data=longest_mention($arg, "extent")) ? ("extent", $data) : ("???", ""))));
    $out .= ", $tag=\"$value\"" if $value;
  }
  return $out;
}

#################################

sub print_releve_mention_mapping {

  my ($ref, $tst, $doc) = @_;

  my $doc_ref = $ref ? $ref->{documents}{$doc} : undef;
  my $doc_tst = $tst ? $tst->{documents}{$doc} : undef;
  my $ref_value = $doc_ref ? $doc_ref->{VALUE} : 0;
  my $tst_value = ($doc_ref ? ($doc_tst ? ($mapped_document_values{$ref->{ID}}{$tst->{ID}}{$doc} ? 
					   $mapped_document_values{$ref->{ID}}{$tst->{ID}}{$doc}
					   : 0)
			       : 0)
		   : $doc_tst->{FA_VALUE});
  my $output = sprintf "- in document %s:  score:  %.5f out of %.5f\n", $doc, $tst_value, $ref_value;
  my @data;
  if ($ref and $ref->{documents} and $ref->{documents}{$doc}) {
    foreach my $mention (@{$ref->{documents}{$doc}{mentions}}) {
      push @data, {DATA=>$mention, TYPE=>"ref"};
    }
  } else {
    $output .= ">>>   no ref mentions\n" if $ref;
  }
  if ($tst and $tst->{documents} and $tst->{documents}{$doc}) {
    foreach my $mention (@{$tst->{documents}{$doc}{mentions}}) {
      push @data, {DATA=>$mention, TYPE=>"tst"};
    }
  } else {
    $output .= ">>>   no tst mentions\n" if $tst;
  }
  foreach my $data (sort compare_locators @data) {
    my $type = $data->{TYPE};
    my $mention = $data->{DATA};
    my $mapped = $mention->{MAP};
    next if $mapped and $type eq "tst";
    my $error = (not $mapped and not ($ref xor $tst));
    $print_data ||= $error;
    $output .= sprintf "%s   $type ", $error ? ">>>" : "   ";
    my $anchor = $mention->{anchor}{text} ? $mention->{anchor}{text} : "???";
    my $extent = $mention->{extent}{text} ? $mention->{extent}{text} : "???";
    $output .= $mention->{ELEMENT_TYPE} =~ /event mention/ ? 
      "anchor=\"$anchor\", extent=\"$extent\"" : "extent=\"$extent\"";
    $output .= sprintf $error ? " -- unmatched mention\n" : "\n";
  }

  $output .= print_argument_mapping ($ref, $tst, $doc); 
  return $output;
}

#################################

sub get_timex2s { #extract information for document-level timex2s

  my ($timex2s, $type, $doc_id) = @_;

  my (@timex2s, %ids);
  foreach my $timex2 (@$timex2s) {
    my %timex2;
    $timex2{ELEMENT_TYPE} = "timex2";
    $timex2{document} = $doc_id;

#get timex2 ID
    $timex2{ID} = $input_element = demand_attribute ("TIMEX2", "ID", $timex2->{attributes});
    $fatal_input_error_header =
      "\n\nFATAL INPUT ERROR for timex2 '$input_element'\n	in document '$doc_id'\n	in file '$input_file'\n";
    not defined $ids{$input_element} or die $fatal_input_error_header.
      "    multiple timex2 definitions (only one definition allowed per document)\n";
    $ids{$input_element} = 1;

#get timex2 attributes
    $timex2{TYPE} = "TIMEX2";
    $timex2{SUBTYPE} = "---";
    foreach my $attribute (@timex2_attributes) {
      next if $attribute =~ /^(ID|TYPE|SUBTYPE)$/;
      my $value = get_attribute ("TIMEX2", $attribute, $timex2->{attributes});
      $timex2{$attribute} = $value if defined $value;
    }
    $timex2{SET} = "NO" unless defined $timex2{SET};
    $timex2{NON_SPECIFIC} = "NO" unless defined $timex2{NON_SPECIFIC};

    my @mentions = get_timex2_mentions ($timex2, $doc_id);
    @mentions or not $use_mentions or die
      $fatal_input_error_header."    timex2 data contains no mentions\n";
    $timex2{mentions} = [@mentions];
    $timex2{external_links} = [get_external_links ($timex2)];
    push @timex2s, {%timex2};
  }
  return @timex2s;
}

#################################

sub get_timex2_mentions {

  my ($timex2, $doc_id) = @_;

  my @mentions = ();
  foreach my $mention (@{$timex2->{subelements}{timex2_mention}}) {
    my %mention;
    $mention{document} = $doc_id;
    $mention{host_id} = $timex2->{attributes}{ID};
    $mention{ID} = demand_attribute ("TIMEX2_mention", "ID", $mention->{attributes});
    ($mention{extent}) = get_locators ("extent", $mention) or die
      $fatal_input_error_header."    no mention extent found\n";
    push @mentions, {%mention};
  }
  return @mentions;
}

#################################

sub get_values { #extract information for document-level values

  my ($values, $type, $doc_id) = @_;

  my (@values, %ids);
  foreach my $value (@$values) {
    my %value;
    $value{ELEMENT_TYPE} = "value";
    $value{document} = $doc_id;

#get value ID
    $value{ID} = $input_element = demand_attribute ("value", "ID", $value->{attributes});
    $fatal_input_error_header =
      "\n\nFATAL INPUT ERROR for value '$input_element'\n	in document '$doc_id'\n	in file '$input_file'\n";
    not defined $ids{$input_element} or die $fatal_input_error_header.
      "    multiple value definitions (only one definition allowed per document)\n";
    $ids{$input_element} = 1;

#get value attributes
    $value{TYPE} = demand_attribute ("value", "TYPE", $value->{attributes}, $value_attributes{TYPE});
    $value{SUBTYPE} = demand_attribute ("value", "SUBTYPE", $value->{attributes}, $value_attributes{TYPE}{$value{TYPE}});
    foreach my $attribute (@value_attributes) {
      next if $attribute =~ /^(ID|TYPE|SUBTYPE)$/;
      my $value = get_attribute ("VALUE", $attribute, $value->{attributes});
      $value{$attribute} = $value if defined $value;
    }

    my @mentions = get_value_mentions ($value, $doc_id);
    @mentions or not $use_mentions or die
      $fatal_input_error_header."    value data contains no mentions\n";
    $value{mentions} = [@mentions];
    $value{external_links} = [get_external_links ($value)];
    push @values, {%value};
  }
  return @values;
}

#################################

sub get_value_mentions {

  my ($value, $doc_id) = @_;

  my @mentions = ();
  foreach my $mention (@{$value->{subelements}{value_mention}}) {
    my %mention;
    $mention{document} = $doc_id;
    $mention{host_id} = $value->{attributes}{ID};
    $mention{ID} = demand_attribute ("value_mention", "ID", $mention->{attributes});
    ($mention{extent}) = get_locators ("extent", $mention);
    $mention{extent} or die $fatal_input_error_header.
      "    no mention extent found\n";
    push @mentions, {%mention};
  }
  return @mentions;
}

#################################

sub value_mention_score {

  my ($ref_mention, $tst_mention) = @_;

  return undef unless ($ref_mention->{extent} and
		       $tst_mention->{extent} and
		       (my $overlap = span_overlap($ref_mention->{extent}{locator},
						   $tst_mention->{extent}{locator})) >= $min_overlap);
  return 1 + $overlap*$epsilon;;
}
      
#################################

sub print_values {

  my ($source, $type, $values, $attributes) = @_;

  return unless $opt_s;
  print "\n======== $source $type ========\n";
  foreach my $id (sort keys %$values) {
    my $value = $values->{$id};
    printf "ID=$value->{ID}, VALUE=%.5f, TYPE=$value->{TYPE}", $value->{VALUE};
    foreach my $attribute (@$attributes) {
      print ", $attribute=$value->{$attribute}" unless
	$attribute =~ /^(ID|TYPE)$/ or not $value->{$attribute};
    }
    print "\n";
    foreach my $doc (sort keys %{$value->{documents}}) {
      my $doc_info = $value->{documents}{$doc};
      print "    -- in document $doc\n";
      foreach my $mention (sort compare_locators @{$doc_info->{mentions}}) {
	printf "      mention extent=\"%s\"\n", $mention->{extent}{text} ? $mention->{extent}{text} : "???";
      }
    }
  }
}

#################################

sub print_value_mapping {

  my ($ref_data, $tst_data, $type, $attributes) = @_;

  return unless $opt_a or $opt_e;
  print "\n======== $type mapping ========\n";
  my ($ref, $tst, $ref_id, $tst_id, $doc, $ref_occ, $tst_occ, $attribute, $output);
  foreach $ref_id (sort keys %$ref_data) {
    $print_data = $opt_a;
    my $output;
    $ref = $ref_data->{$ref_id};
    if ($tst = $ref->{MAP}) {
      $tst_id = $tst->{ID};
      my $attribute_error;
      foreach $attribute (@$attributes) {
	$attribute_error .= ($attribute_error ? "/" : "").$attribute if
	  $attribute ne "ID" and
	  (($ref->{$attribute} xor $tst->{$attribute}) or
	   ($ref->{$attribute} and $tst->{$attribute} ne $ref->{$attribute}));
      }
      $print_data ||= $opt_e if $attribute_error;
      $output .= ($attribute_error ? ">>> " : "    ")."ref $type $ref_id";
      foreach $attribute (@$attributes) {
	next if not $ref->{$attribute};
	$output .= ", $attribute=$ref->{$attribute}";
      }
      $output .= "\n";

      $output .= ($attribute_error ? ">>> " : "    ")."tst $type $tst_id";
      foreach $attribute (@$attributes) {
	next if not $tst->{$attribute};
	$output .= ", $attribute=$tst->{$attribute}";
      }
      $output .= $attribute_error ? " -- ATTRIBUTE ERROR ($attribute_error)\n" : "\n";

      $output .= sprintf ("      $type score:  %.5f out of %.5f\n",
			  $mapped_values{$ref_id}{$tst_id}, $ref->{VALUE});
    } else {
      $print_data ||= $opt_e;
      $output .= ">>> ref $type $ref_id -- NO MATCHING TST VALUE\n";
      $output .= sprintf ("      $type score:  0.00000 out of %.5f\n", $ref->{VALUE});
    }
    my %docs;
    map $docs{$_}=1, keys %{$ref->{documents}} if $ref;
    map $docs{$_}=1, keys %{$tst->{documents}} if $tst;
    foreach my $doc (sort keys %docs) {
      $output .= value_mapping_details ($ref, $ref->{MAP}, $doc) if $eval_docs{$doc};
    }
    print $output, "--------\n" if $print_data;
  }

  return unless $opt_a or $opt_e;
  foreach $tst_id (sort keys %$tst_data) {
    $tst = $tst_data->{$tst_id};
    next if $tst->{MAP};
    print ">>> tst $type $tst_id";
    foreach $attribute (@$attributes) {
      next if not $tst->{$attribute};
      print ", $attribute=$tst->{$attribute}";
    }
    print " -- NO MATCHING REF VALUE\n";
    printf "      $type score:  %.5f out of 0.00000\n", $tst->{FA_VALUE};
    foreach $doc (sort keys %{$tst->{documents}}) {
      print STDOUT value_mapping_details (undef, $tst, $doc) if $eval_docs{$doc};
    }
    print "--------\n";
  }
}

#################################

sub value_mapping_details {

  my ($ref, $tst, $doc) = @_;

  $ref = $ref->{documents}{$doc} if $ref;
  $tst = $tst->{documents}{$doc} if $tst;
  my ($type, $ref_mention, $tst_mention, $mention, @mentions);
  my $output = "- in document $doc:\n";
  return $output unless $use_mentions;
  if ($ref) {
    foreach $mention (@{$ref->{mentions}}) {
      push @mentions, {DATA=>$mention, TYPE=>"REF"};
    }
  }
  if ($tst) {
    foreach $mention (@{$tst->{mentions}}) {
      push @mentions, {DATA=>$mention, TYPE=>"TST"};
    }
  }
  if ($ref and $tst) {
    foreach $mention (sort compare_locators @mentions) {
      $type = $mention->{TYPE};
      $mention = $mention->{DATA};
      next if $type eq "TST" and $mention->{MAP};
      if ($mention->{MAP}) {
	$ref_mention = $mention;
	$tst_mention = $mention->{MAP};
	my $extent_error = extent_mismatch ($ref_mention->{extent}{locator}, $tst_mention->{extent}{locator}) > 1;
	if (not $extent_error and
	    $ref_mention->{extent}{text} and
	    $tst_mention->{extent}{text} and
	    $ref_mention->{extent}{text} eq $tst_mention->{extent}{text}) {
	  $output .= "          mention=\"" . $ref_mention->{extent}{text} . "\"\n";
	} else {
	  $print_data ||= $opt_e if $extent_error;
	  my $tag = $extent_error ? ">>>   " : "      ";
	  $output .= $tag."ref mention=\"" . ($ref_mention->{extent}{text} ? $ref_mention->{extent}{text} : "???") . "\"\n";
	  $output .= $tag."tst mention=\"" . ($tst_mention->{extent}{text} ? $tst_mention->{extent}{text} : "???") . "\"";
	  $output .= $extent_error ? " -- EXTENT ERROR\n" : "\n";
	}
      } else {
	$print_data ||= $opt_e;
	$output .= ">>>   ".(lc$type)." mention=\"" . ($mention->{extent}{text} ? $mention->{extent}{text} : "???") . "\"";
	$output .= " -- NO MATCHING %s MENTION\n",
      }
    }
  } else {
    $print_data ||= $opt_e;
    foreach $mention (sort compare_locators @mentions) {
      $type = $mention->{TYPE};
      $mention = $mention->{DATA};
      $output .= "      ".(lc$type)." mention=\"" . ($mention->{extent}{text} ? $mention->{extent}{text} : "???") . "\"";
      $output .= "\n";
    }
  }
  return $output;
}

#################################

sub get_entities { #extract information for document-level entities

  my ($entities, $type, $doc_id) = @_;

  my (@entities, %ids);
  foreach my $entity (@$entities) {
    my %entity;
    $entity{ELEMENT_TYPE} = "entity";
    $entity{document} = $doc_id;

#get entity ID
    $entity{ID} = $input_element = demand_attribute ("entity", "ID", $entity->{attributes});
    $fatal_input_error_header =
      "\n\nFATAL INPUT ERROR for entity '$input_element'\n	in document '$doc_id'\n	in file '$input_file'\n";
    not defined $ids{$input_element} or die $fatal_input_error_header.
      "    multiple entity definitions (only one definition allowed per document)\n";
    $ids{$input_element} = 1;

#get entity attributes
    $entity{TYPE} = demand_attribute ("entity", "TYPE", $entity->{attributes}, $entity{TYPE});
    $entity{SUBTYPE} = demand_attribute ("entity", "SUBTYPE", $entity->{attributes}, $entity_attributes{TYPE}{$entity{TYPE}});
    $entity{SUBTYPE} = "---" unless defined $entity{SUBTYPE};
    foreach my $attribute (@entity_attributes) {
      next if $attribute =~ /^(ID|TYPE|SUBTYPE|LEVEL)$/;
      my $value = demand_attribute ("entity", $attribute, $entity->{attributes}, $entity_attributes{$attribute});
      $entity{$attribute} = $value if $value;
    }

    $entity{mentions} = [get_entity_mentions ($entity, $doc_id)];
    @{$entity{mentions}} > 0 or not $use_mentions or die $fatal_input_error_header.
      "    this entity contains no mentions\n";
    $entity{names} = [get_entity_names ($entity)];
    $entity{LEVEL} = doc_entity_level (\%entity);
    my $num_names = @{$entity{names}};
    $num_names == 0 or $entity{LEVEL} eq "NAM" or die $fatal_input_error_header.
      "    this entity is declared to be of level '$entity{LEVEL}', but it has names\n";
    $entity{external_links} = [get_external_links ($entity)];
    $entity{LEVEL} eq "NAM" || $score_non_named_entities ?
      push @entities, {%entity} : ($discarded_entities{$entity{ID}} = 1);
    $entity{LEVEL} eq "NAM" or $allow_non_named_entities or die $fatal_input_error_header.
      "    Only entities with document LEVEL = NAM are allowed.  Document LEVEL = $entity{LEVEL} for this entity\n";
  }
  return @entities;
}

#################################

sub get_entity_mentions {

  my ($entity, $doc_id) = @_;

  my @mentions;
  foreach my $mention (@{$entity->{subelements}{entity_mention}}) {
    my %mention;
    my $attributes = $mention->{attributes};
    $mention{document} = $doc_id;
    $mention{host_id} = $entity->{attributes}{ID};
    $mention{ID} = demand_attribute ("entity_mention", "ID", $attributes);
    $mention{TYPE} = demand_attribute ("entity_mention", "TYPE", $attributes, \%entity_mention_type_wgt);
    my $role = get_attribute ("entity_mention", "ROLE", $attributes, \%entity_type_wgt);
    $mention{ROLE} = defined $role ? $role : $entity->{attributes}{TYPE};
    my $metonymy = get_attribute ("entity_mention", "METONYMY_MENTION", $attributes, {TRUE=>1, FALSE=>1});
    $mention{STYLE} = ($metonymy and $metonymy eq "TRUE") ? "METONYMIC" : "LITERAL";
    ($mention{head}) = get_locators ("head", $mention) or die $fatal_input_error_header.
	"    no mention head found\n";
    ($mention{extent}) = get_locators ("extent", $mention) or die $fatal_input_error_header.
	"    no mention extent found)\n";
    push @mentions, {%mention};
  }
  return @mentions;
}

#################################

sub entity_name_entropy {

  my ($entity) = @_;

  my (%doc_count, $num_docs);
  foreach my $doc_entity (values %{$entity->{documents}}) {
    $num_docs++;
    foreach my $name (@{$doc_entity->{names}}) {
      $doc_count{$name}++;
    }
  }
  return undef unless %doc_count;

  my $name_entropy;
  foreach my $count (values %doc_count) {
    next unless $count < $num_docs;
    my $prob = $count/$num_docs;
    $name_entropy -= $prob*log($prob);
  }
  return $name_entropy;
}

#################################

sub doc_entity_level {

  my ($entity) = @_;
  my $level;
  my $weight = -9E9;
  foreach my $mention (@{$entity->{mentions}}) {
    my $mention_type = $mention->{TYPE};
    $mention_type = "NOM" if ($mention_type eq "NAM" and
			      $mention->{STYLE} eq "METONYMIC");
    my $mention_wgt = $entity_mention_type_wgt{$mention_type};
    my $cmp = $mention_wgt <=> $weight;
    next if $cmp < 0;
    if ($cmp) {
      $weight = $mention_wgt;
      $level = $mention_type;
    } elsif ($level eq "PRO") {
      $level = $mention_type;
    } elsif ($mention_type eq "NAM") {
      $level = "NAM";
    }
  }
  return $level if $use_mentions;
  $level = "NAM" if @{$entity->{names}} > 0;
  $level = "PRO" unless $level;
  return $level;
}

#################################

sub longest_name {

  my ($element) = @_;

  my $longest_name="";
  foreach my $doc_element (values %{$element->{documents}}) {
    foreach my $name (@{$doc_element->{names}}) {
      $longest_name = $name if length $name > length $longest_name;
    }
  }
  return $longest_name;
}


#################################

sub longest_mention {

  my ($element, $type) = @_;

  my $longest_string = "";
  foreach my $doc_element (values %{$element->{documents}}) {
    foreach my $mention (@{$doc_element->{mentions}}) {
      next unless $mention->{$type};
      $longest_string = $mention->{$type}{text} if ($mention->{$type} and
						    length $mention->{$type}{text} > length $longest_string);
    }
  }
  return $longest_string;
}

#################################
    
sub get_entity_names {

  my ($entity) = @_;
    
  my @names;
  return () unless my $attr = $entity->{subelements}{entity_attributes};
  return () unless my $names = $attr->[0]{subelements}{name};
  while (my $name = shift @$names) {
    $name = $name->{attributes}{NAME};
    defined $name or die $fatal_input_error_header.
    "    a name is defined without a NAME attribute\n";
    push @names, normalize_name ($name);
  }
  @names = sort @names;
  my $i = 1;
  $names[$i] eq $names[$i-1] ? splice @names, $i, 1 : $i++ while ($i<@names);
  return @names;
}

#################################

sub normalize_name {

  $_ = lc $_[0];
  s/[\r\n]+/\n/sg;
  s/(\p{Han})\n(\p{Han})/$1$2/sg;
  s/(\p{Latin}|\p{Arabic}|\p{Han})-\n(\p{Latin}|\p{Arabic}|\p{Han})/$1$2/sg;
  s/[\n\s]+/ /sg;
  s/^\s*|\s*$//sg;
  s/\s*(['-])\s*/$1/sg;
  s/\.$//sg;
  return $_;
}    

#################################
    
sub get_external_links {

  my ($entity) = @_;

  my @links;
  foreach my $link (@{$entity->{subelements}{external_link}}) {
    my %link;
    $link{RESOURCE} = demand_attribute ("external_link", "RESOURCE", $link->{attributes});
    $link{ID} = demand_attribute ("external_link", "EID", $link->{attributes});
    push @links, {%link};
  }
  return @links;
}

#################################
    
sub promote_external_links {

  my ($db_element, $element) = @_;

  $db_element->{external_links} = [] unless $db_element->{external_links};
  return unless @{$element->{external_links}};
  my %new_links;
  foreach my $link (@{$element->{external_links}}) {
    not defined $new_links{$link->{RESOURCE}} or die
      "\n\nFATAL INPUT ERROR:  multiple external ID's ('$link->{ID}'/'$new_links{$link->{RESOURCE}}')".
      "given for resource '$link->{RESOURCE}' for element '$element->{ID}'\n";
    $new_links{$link->{RESOURCE}} = $link->{ID};
  }

  foreach my $link (@{$db_element->{external_links}}) {
    next unless defined $new_links{$link->{RESOURCE}};
    $link->{ID} eq $new_links{$link->{RESOURCE}} or die
      "\n\nFATAL INPUT ERROR:  conflicting external ID's ('$link->{ID}'/'$new_links{$link->{RESOURCE}}')".
      "given for resource '$link->{RESOURCE}' for element '$element->{ID}'\n";
    delete $new_links{$link->{RESOURCE}};
  }

  while (my ($resource, $id) = each %new_links) {
    push @{$db_element->{external_links}}, {RESOURCE => $resource, ID => $id};
  }
}

#################################
    
sub get_locators {

  my ($name, $data) = @_;

  my (%info, @info, $text);
  
  foreach my $element (@{$data->{subelements}{$name}}) {
    my $locator = $element->{subelements};
    $info{name} = $element->{attributes}{NAME} if $element->{attributes}{NAME};
    $info{locator} = ($data_type eq "text" ? get_text_locator ($locator) :
		      ($data_type eq "audio" ? get_audio_locator ($locator) :
		       ($data_type eq "image" ? get_image_locator ($locator) : 
			(die $fatal_input_error_header.
			 "    No locator read routine for '$data_type' for $name locator\n"))));
    $info{locator}{data_type} = $data_type;
    if ($text = $info{locator}{text}) {
      $text =~ s/[\r\n]+/\n/sg;
      $text =~ s/([^\s])-\n([^\s])/$1$2/sg;
      $text =~ s/[\n\s]+/ /sg;
      $info{text} = limit_string $text;
    }
    push @info, {%info};
  }
  return @info;
}

#################################
    
sub get_text_locator {

  my ($data) = @_;

  my $locator = $data->{charseq} ? $data->{charseq}[0] : $data->{charspan}[0];
  $locator or die $fatal_input_error_header.
    "    text mention contains no position info (no 'charseq' or 'charspan')\n";
  my %locator;
  $locator{start} = $locator->{attributes}{START};
  $locator{end} = $data->{charseq} ? $locator->{attributes}{END} : $locator->{attributes}{END}-1;
  $locator{text} = $locator->{span};
  defined $locator{start} or die $fatal_input_error_header."    No 'START' attribute found\n";
  defined $locator{end} or die $fatal_input_error_header."    No 'END' attribute found\n";
  $locator{end}-$locator{start} >= 0 or die $fatal_input_error_header.
    "    Non-positive text span (start=$locator{start}, end=$locator{end})\n";
  return {%locator};
}

#################################
    
sub get_audio_locator {

  my ($data) = @_;

  my $locator = $data->{timespan}[0];
  $locator or die $fatal_input_error_header.
    "    audio mention contains no timing info (no 'timespan' element)\n";
  my %locator;
  $locator{start} = $locator->{attributes}{START};
  $locator{end} = $locator->{attributes}{END};
  $locator{text} = $locator->{span};
  defined $locator{start} or die $fatal_input_error_header."    No 'START' attribute found\n";
  defined $locator{end} or die $fatal_input_error_header."    No 'END' attribute found\n";
  $locator{end} > $locator{start} or die $fatal_input_error_header.
    "    Non-positive time span (start=$locator{start}, end=$locator{end})\n";
  return {%locator};
}

#################################
    
sub get_image_locator {

  my ($data) = @_;
    
  my $bblist = $data->{bblist}[0] or die $fatal_input_error_header.
    "    image mention contains no image data (no 'bblist' element)\n";
  my ($nboxes, %pages);
  foreach my $bbox (@{$bblist->{subelements}{bbox}}) {
    my %box;
    defined ($box{page} = $bbox->{attributes}{PAGE}) or die $fatal_input_error_header.
      "    No 'page' attribute found in 'bbox' element\n";
    defined ($box{xstart} = $bbox->{attributes}{X}) or die $fatal_input_error_header.
      "    No 'x' attribute found in 'bbox' element\n";
    defined ($box{ystart} = $bbox->{attributes}{Y}) or die $fatal_input_error_header.
      "    No 'y' attribute found in 'bbox' element\n";
    defined ($box{width} = $bbox->{attributes}{WIDTH}) or die $fatal_input_error_header.
      "    No 'width' attribute found in 'bbox' element\n";
    defined ($box{height} = $bbox->{attributes}{HEIGHT}) or die $fatal_input_error_header.
      "    No 'height' attribute found in 'bbox' element\n";
    $box{width} > 0 or die $fatal_input_error_header."    Non-positive bounding box width\n";
    $box{height} > 0 or die $fatal_input_error_header."    Non-positive bounding box height\n";
    $box{xend} = $box{xstart}+$box{width};
    $box{yend} = $box{ystart}+$box{height};
    $box{text} = $bbox->{span};
    push @{$pages{$box{page}}{boxes}}, {%box};
    $nboxes++;
  }
  $nboxes or die $fatal_input_error_header."    image mention contains no boxes (no 'bbox' data)\n";
  my $text = "";
  foreach my $page_num (sort {$a<=>$b;} keys %pages) {
    my $page = $pages{$page_num};
    my @boxes = sort {my $cmp;
		      return $cmp if $cmp = $b->{yend} <=> $a->{yend};
		      return $cmp if $cmp = $a->{xstart} <=> $b->{xstart};
		      return $cmp if $cmp = $b->{ystart} <=> $a->{ystart};
		      return $a->{xend} <=> $b->{xend};
		    } @{$page->{boxes}};
    my $descriptor;
    foreach my $box (@boxes) {
      ($text .= $box->{text}, delete $box->{text}) if $box->{text};
      my $x = $box->{xstart}, my $w = $box->{width}, my $y = $box->{ystart}, my $h = $box->{height};
      $descriptor .= " $y, $h, $x, $w";
      $page->{xmin} = $x if not defined $page->{xmin} or $x < $page->{xmin};
      $page->{ymin} = $y if not defined $page->{ymin} or $y < $page->{ymin};
      $page->{xmax} = $x+$w if not defined $page->{xmax} or $x+$w > $page->{xmax};
      $page->{ymax} = $y+$h if not defined $page->{ymax} or $y+$h > $page->{ymax};
    }
    $unique_pages{$descriptor} = \@boxes unless $unique_pages{$descriptor};
    $page->{boxes} = $unique_pages{$descriptor};
  }
  return {text => $text, pages => {%pages}};
}

#################################

sub compare_locators {

  my $ax = $a;
  my $bx = $b;

  ($ax, $bx) = ($ax->{DATA}, $bx->{DATA}) if $ax->{DATA} and $bx->{DATA};

  $ax = (defined $ax->{head} and defined $ax->{head}{locator}) ? $ax->{head}{locator} :
    (defined $ax->{extent} and defined $ax->{extent}{locator}) ? $ax->{extent}{locator} :
    defined $ax->{locator} ? $ax->{locator} : die
    "\n\nFATAL ERROR in input to compare_locators\n";

  $bx = (defined $bx->{head} and defined $bx->{head}{locator}) ? $bx->{head}{locator} :
    (defined $bx->{extent} and defined $bx->{extent}{locator}) ? $bx->{extent}{locator} :
    defined $bx->{locator} ? $bx->{locator} : die
    "\n\nFATAL ERROR in input to compare_locators\n";

  if (($ax->{data_type} eq "text" and $bx->{data_type} eq "text") or
      ($ax->{data_type} eq "audio" and $bx->{data_type} eq "audio")) {
    return $ax->{start} <=> $bx->{start};
  } elsif ($ax->{data_type} eq "image" and $bx->{data_type} eq "image") {
    my $apage = min (keys %{$ax->{pages}});
    my $bpage = min (keys %{$bx->{pages}});
    my $cmp;
    return $cmp if $cmp = $apage <=> $bpage;
    return $cmp if $cmp = $bx->{pages}{$apage}{ymax} <=> $ax->{pages}{$bpage}{ymax};
    return $cmp if $cmp = $ax->{pages}{$apage}{xmin} <=> $bx->{pages}{$bpage}{xmin};
    return $ax->{pages}{$apage}{xmax} <=> $bx->{pages}{$bpage}{xmax};
  } else {
    die "\n\nFATAL ERROR in compare_locators\n";
  }
}

#################################

sub extract_sgml_structure {

  my ($data) = @_;

  my $objects = {};
  while ($data =~ s/^.*?<([a-z_][a-z0-9_+-]*)( [^>]*?| *)(\/?)>//is) {
    my ($name, $attributes, $span) = ($1, $2, !$3);
    my $object = {};
    $object->{attributes}{uc$1} = $2 while ($attributes =~ s/\s*([^\s]+)\s*=\s*\"\s*([^\"]*?)\s*\"//si);
    if ($span) {
      $data =~ s/^(.*?)<\/$name *>//is or die
	"\n\nFATAL INPUT ERROR:  unclosed sgml element '$name' in file '$input_file'\n".
	"                    (or element '$name' overlaps with a previous element)\n";
      $object->{span} = $1;
      $object->{subelements} = extract_sgml_structure ($object->{span});
    } else {
      $object->{span} = "";
    }
    push @{$objects->{$name}}, $object;
  }
  return $objects;
}

#################################

sub date_time_stamp {

  my ($sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst) = localtime();
  my @months = qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec);
  my $time = sprintf "%2.2d:%2.2d:%2.2d", $hour, $min, $sec;
  my $date = sprintf "%4.4s %3.3s %s", 1900+$year, $months[$mon], $mday;
  return ($date, $time);
}

#################################

sub print_documents {

  my ($source, $documents) = @_;

  print "\n======== $source documents ========\n";
  foreach my $doc_id (sort keys %$documents) {
    my $doc = $documents->{$doc_id};
    print "ID=$doc_id, TYPE=$doc->{TYPE}, FILE=$doc->{FILE}, SOURCE=$doc->{SOURCE}\n";
  }
}

#################################

sub id_plus_external_ids {

  my ($element) = @_;

  return $element->{ID} unless @{$element->{external_links}};

  my $external_ids;
  foreach my $link (sort {$a->{ID} cmp $b->{ID};} @{$element->{external_links}}) {
    $external_ids .= ($external_ids ? ", " : "")."$link->{ID} in $link->{RESOURCE}";
  }
  return "$element->{ID} ($external_ids)";
}

#################################

sub print_entities {

  my ($source, $type, $entities) = @_;

  return unless $opt_s;
  print "\n======== $source $type ========\n";
  foreach my $id (sort keys %$entities) {
    my $entity = $entities->{$id};
    printf "ID=%s, VALUE=%.5f, TYPE=%s, SUBTYPE=%s, LEVEL=%s, CLASS=%s", id_plus_external_ids($entity),
    $entity->{VALUE}, $entity->{TYPE}, $entity->{SUBTYPE}, $entity->{LEVEL}, $entity->{CLASS};
    foreach my $attribute (@entity_attributes) {
      print ", $attribute=$entity->{$attribute}" unless
	$attribute =~ /^(ID|TYPE|SUBTYPE|LEVEL|CLASS)$/ or not defined $entity->{$attribute};
    }
    print "\n";
    foreach my $doc (sort keys %{$entity->{documents}}) {
      print_document_entity ($doc, $entity);
    }
  }
}

#################################

sub print_document_entity {

  my ($doc, $entity) = @_;

  my $doc_entity = $entity->{documents}{$doc};
  printf "    -- in document $doc VALUE=%.5f\n", $doc_entity->{VALUE};

  foreach my $name (@{$doc_entity->{names}}) {
    print "      name=\"$name\"\n";
  }

  foreach my $mention (sort compare_locators @{$doc_entity->{mentions}}) {
    print "      mention TYPE=$mention->{TYPE}, ROLE=$mention->{ROLE}, STYLE=$mention->{STYLE}, ";
    printf "head=\"%s\", ", defined $mention->{head}{text} ? $mention->{head}{text} : "???";
    printf "extent=\"%s\"\n", defined $mention->{extent}{text} ? $mention->{extent}{text} : "???";
  }
}

#################################

sub weighted_bipartite_graph_matching {
  my ($score) = @_;
    
  unless (defined $score) {
    warn "input to BGM is undefined\n";
    return undef;
  }
  my @keys = keys %$score;
  return {} unless @keys;
  if (@keys == 1) { #skip graph matching an simply pick the minimum cost map
    my $costs = $score->{$keys[0]};
    my (%map, $imin);
    foreach my $i (keys %$costs) {
      $imin = $i if not defined $imin or $costs->{$imin} > $costs->{$i};
    }
    $map{$keys[0]} = $imin;
    return {%map};
  }
    
  my $INF = 1E30;
  my $required_precision = 1E-12;
  my (@row_mate, @col_mate, @row_dec, @col_inc);
  my (@parent_row, @unchosen_row, @slack_row, @slack);
  my ($k, $l, $row, $col, @col_min, $cost, %cost);
  my $t = 0;
    
  my @rows = keys %{$score};
  my $miss = "miss";
  $miss .= "0" while exists $score->{$miss};
  my (@cols, %cols);
  my $min_score = $INF;
  foreach $row (@rows) {
    foreach $col (keys %{$score->{$row}}) {
      $min_score = min($min_score,$score->{$row}{$col});
      $cols{$col} = $col;
    }
  }
  @cols = keys %cols;
  my $fa = "fa";
  $fa .= "0" while exists $cols{$fa};
  my $reverse_search = @rows < @cols; # search is faster when ncols <= nrows
  foreach $row (@rows) {
    foreach $col (keys %{$score->{$row}}) {
      ($reverse_search ? $cost{$col}{$row} : $cost{$row}{$col})
	= $score->{$row}{$col} - $min_score;
    }
  }
  push @rows, $miss;
  push @cols, $fa;
  if ($reverse_search) {
    my @xr = @rows;
    @rows = @cols;
    @cols = @xr;
  }

  my $nrows = @rows;
  my $ncols = @cols;
  my $nmax = max($nrows,$ncols);
  my $no_match_cost = -$min_score*(1+$required_precision);

  # subtract the column minimas
  for ($l=0; $l<$nmax; $l++) {
    $col_min[$l] = $no_match_cost;
    next unless $l < $ncols;
    $col = $cols[$l];
    foreach $row (keys %cost) {
      next unless defined $cost{$row}{$col};
      my $val = $cost{$row}{$col};
      $col_min[$l] = $val if $val < $col_min[$l];
    }
  }
    
  # initial stage
  for ($l=0; $l<$nmax; $l++) {
    $col_inc[$l] = 0;
    $slack[$l] = $INF;
  }
    
 ROW:
  for ($k=0; $k<$nmax; $k++) {
    $row = $k < $nrows ? $rows[$k] : undef;
    my $row_min = $no_match_cost;
    for (my $l=0; $l<$ncols; $l++) {
      my $col = $cols[$l];
      my $val = ((defined $row and defined $cost{$row}{$col}) ? $cost{$row}{$col}: $no_match_cost) - $col_min[$l];
      $row_min = $val if $val < $row_min;
    }
    $row_dec[$k] = $row_min;
    for ($l=0; $l<$nmax; $l++) {
      $col = $l < $ncols ? $cols[$l]: undef;
      $cost = ((defined $row and defined $col and defined $cost{$row}{$col}) ?
	       $cost{$row}{$col} : $no_match_cost) - $col_min[$l];
      if ($cost==$row_min and not defined $row_mate[$l]) {
	$col_mate[$k] = $l;
	$row_mate[$l] = $k;
	# matching row $k with column $l
	next ROW;
      }
    }
    $col_mate[$k] = -1;
    $unchosen_row[$t++] = $k;
  }
    
  goto CHECK_RESULT if $t == 0;
    
  my $s;
  my $unmatched = $t;
  # start stages to get the rest of the matching
  while (1) {
    my $q = 0;
	
    while (1) {
      while ($q < $t) {
	# explore node q of forest; if matching can be increased, update matching
	$k = $unchosen_row[$q];
	$row = $k < $nrows ? $rows[$k] : undef;
	$s = $row_dec[$k];
	for ($l=0; $l<$nmax; $l++) {
	  if ($slack[$l]>0) {
	    $col = $l < $ncols ? $cols[$l]: undef;
	    $cost = ((defined $row and defined $col and defined $cost{$row}{$col}) ?
		     $cost{$row}{$col} : $no_match_cost) - $col_min[$l];
	    my $del = $cost - $s + $col_inc[$l];
	    if ($del < $slack[$l]) {
	      if ($del == 0) {
		goto UPDATE_MATCHING unless defined $row_mate[$l];
		$slack[$l] = 0;
		$parent_row[$l] = $k;
		$unchosen_row[$t++] = $row_mate[$l];
	      } else {
		$slack[$l] = $del;
		$slack_row[$l] = $k;
	      }
	    }
	  }
	}
		
	$q++;
      }
	    
      # introduce a new zero into the matrix by modifying row_dec and col_inc
      # if the matching can be increased update matching
      $s = $INF;
      for ($l=0; $l<$nmax; $l++) {
	if ($slack[$l] and ($slack[$l]<$s)) {
	  $s = $slack[$l];
	}
      }
      for ($q = 0; $q<$t; $q++) {
	$row_dec[$unchosen_row[$q]] += $s;
      }
	    
      for ($l=0; $l<$nmax; $l++) {
	if ($slack[$l]) {
	  $slack[$l] -= $s;
	  if ($slack[$l]==0) {
	    # look at a new zero and update matching with col_inc uptodate if there's a breakthrough
	    $k = $slack_row[$l];
	    unless (defined $row_mate[$l]) {
	      for (my $j=$l+1; $j<$nmax; $j++) {
		if ($slack[$j]==0) {
		  $col_inc[$j] += $s;
		}
	      }
	      goto UPDATE_MATCHING;
	    } else {
	      $parent_row[$l] = $k;
	      $unchosen_row[$t++] = $row_mate[$l];
	    }
	  }
	} else {
	  $col_inc[$l] += $s;
	}
      }
    }
	
   UPDATE_MATCHING:		# update the matching by pairing row k with column l
    while (1) {
      my $j = $col_mate[$k];
      $col_mate[$k] = $l;
      $row_mate[$l] = $k;
      # matching row $k with column $l
      last UPDATE_MATCHING if $j < 0;
      $k = $parent_row[$j];
      $l = $j;
    }
	
    $unmatched--;
    goto CHECK_RESULT if $unmatched == 0;
	
    $t = 0;			# get ready for another stage
    for ($l=0; $l<$nmax; $l++) {
      $parent_row[$l] = -1;
      $slack[$l] = $INF;
    }
    for ($k=0; $k<$nmax; $k++) {
      $unchosen_row[$t++] = $k if $col_mate[$k] < 0;
    }
  }				# next stage

 CHECK_RESULT:			# rigorously check results before handing them back
  for ($k=0; $k<$nmax; $k++) {
    $row = $k < $nrows ? $rows[$k] : undef;
    for ($l=0; $l<$nmax; $l++) {
      $col = $l < $ncols ? $cols[$l]: undef;
      $cost = ((defined $row and defined $col and defined $cost{$row}{$col}) ?
	       $cost{$row}{$col} : $no_match_cost) - $col_min[$l];
      if ($cost < ($row_dec[$k] - $col_inc[$l])) {
	next unless $cost < ($row_dec[$k] - $col_inc[$l]) - $required_precision*max(abs($row_dec[$k]),abs($col_inc[$l]));
	warn "BGM: this cannot happen: cost{$row}{$col} ($cost) cannot be less than row_dec{$row} ($row_dec[$k]) - col_inc{$col} ($col_inc[$l])\n";
	return undef;
      }
    }
  }

  for ($k=0; $k<$nmax; $k++) {
    $row = $k < $nrows ? $rows[$k] : undef;
    $l = $col_mate[$k];
    $col = $l < $ncols ? $cols[$l]: undef;
    $cost = ((defined $row and defined $col and defined $cost{$row}{$col}) ?
	     $cost{$row}{$col} : $no_match_cost) - $col_min[$l];
    if (($l<0) or ($cost != ($row_dec[$k] - $col_inc[$l]))) {
      next unless $l<0 or abs($cost - ($row_dec[$k] - $col_inc[$l])) > $required_precision*max(abs($row_dec[$k]),abs($col_inc[$l]));
      warn "BGM: every row should have a column mate: row $row doesn't, col: $col\n";
      return undef;
    }
  }

  my %map;
  for ($l=0; $l<@row_mate; $l++) {
    $k = $row_mate[$l];
    $row = $k < $nrows ? $rows[$k] : undef;
    $col = $l < $ncols ? $cols[$l]: undef;
    next unless defined $row and defined $col and defined $cost{$row}{$col};
    $reverse_search ? ($map{$col} = $row) : ($map{$row} = $col);
  }
  return {%map};
}
