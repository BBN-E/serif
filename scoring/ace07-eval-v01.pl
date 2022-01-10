#!/usr/bin/perl -w
use strict;
#################################
# History:
#
# version 01
#    Derives from ace05-eval-vx16d.
#
#################################

my %entity_attributes =
  (ID => {},
   CLASS => {GEN => 1, NEG => 1, SPC => 1, USP => 1},
   LEVEL => {},
   SUBTYPE => {Address => 1, Air => 1, Airport => 1, Biological => 1, Blunt => 1, Boundary => 1,
	       "Building-Grounds" => 1, Celestial => 1, Chemical => 1, Commercial => 1, Continent => 1,
	       "County-or-District" => 1, Educational => 1, Entertainment => 1, Exploding => 1,
	       "GPE-Cluster" => 1, Government => 1, Group => 1, Indeterminate => 1, Individual => 1,
	       Land => 1, "Land-Region-Natural" => 1, Media => 1, "Medical-Science" => 1, Nation => 1,
	       "Non-Governmental" => 1, Nuclear => 1, Path => 1, Plant => 1, "Population-Center" => 1,
	       Projectile => 1, "Region-General" => 1, "Region-International" => 1, Religious => 1, Sharp => 1,
	       Shooting => 1, Special => 1, Sports => 1, "State-or-Province" => 1, "Subarea-Facility" => 1,
	       "Subarea-Vehicle" => 1, Underspecified => 1, Water => 1, "Water-Body" => 1},
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
my @entity_attributes = sort keys %entity_attributes;

my %quantity_attributes =
  (ID => {},
   SUBTYPE => {"E-Mail" => 1, Money => 1, Percent => 1, "Phone-Number" => 1, URL => 1},
   TYPE => {"Contact-Info" => {"E-Mail" => 1, "Phone-Number" => 1, URL => 1},
	    Crime => {},
	    "Job-Title" => {},
	    Numeric => {Money => 1, Percent => 1},
	    Sentence => {}});
my @quantity_attributes = sort keys %quantity_attributes;

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
   SUBTYPE => {Artifact => 1, Business => 1, "Citizen-Resident-Religion-Ethnicity" => 1, Employment => 1, Family => 1,
	       Founder => 1, Geographical => 1, "Investor-Shareholder" => 1, "Lasting-Personal" => 1, Located => 1,
	       Membership => 1, Near => 1, "Org-Location" => 1, Ownership => 1, "Sports-Affiliation" => 1,
	       "Student-Alum" => 1, Subsidiary => 1, "User-Owner-Inventor-Manufacturer" => 1},
   TENSE => {Future => 1, Past => 1, Present => 1, Unspecified => 1},
   TYPE => {ART => {"User-Owner-Inventor-Manufacturer" => 1},
	    "GEN-AFF" => {"Citizen-Resident-Religion-Ethnicity" => 1, "Org-Location" => 1},
	    METONYMY => {},
	    "ORG-AFF" => {Employment => 1, Founder => 1, "Investor-Shareholder" => 1, Membership => 1,
			  Ownership => 1, "Sports-Affiliation" => 1, "Student-Alum" => 1},
	    "PART-WHOLE" => {Artifact => 1, Geographical => 1, Subsidiary => 1},
	    "PER-SOC" => {Business => 1, Family => 1, "Lasting-Personal" => 1},
	    PHYS => {Located => 1, Near => 1}});
my @relation_attributes = sort keys %relation_attributes;

my %relation_argument_roles =
  ("Arg-1" => 1, "Arg-2" => 1, "Time-After" => 1, "Time-At-Beginning" => 1, "Time-At-End" => 1, "Time-Before" => 1,
   "Time-Ending" => 1, "Time-Holds" => 1, "Time-Starting" => 1, "Time-Within" => 1);
my @relation_argument_roles = sort keys %relation_argument_roles;

my %relation_symmetry =
  ("PER-SOC" => 1,
   METONYMY => 1,
   PHYS => 1);

my %event_attributes =
  (ID => {},
   GENERICITY => {Generic => 1, Specific => 1},
   MODALITY => {Asserted => 1, Other => 1},
   POLARITY => {Negative => 1, Positive => 1},
   SUBTYPE => {Acquit => 1, Appeal => 1, "Arrest-Jail" => 1, Attack => 1, "Be-Born" => 1, "Charge-Indict" => 1,
	       Convict => 1, Date => 1, "Declare-Bankruptcy" => 1, Demonstrate => 1, Die => 1, Divorce => 1,
	       Elect => 1, "End-Org" => 1, "End-Position" => 1, Execute => 1, Extradite => 1, Fine => 1,
	       Injure => 1, Marry => 1, Meet => 1, "Merge-Org" => 1, Nominate => 1, Pardon => 1, "Phone-Write" => 1,
	       "Release-Parole" => 1, Sentence => 1, "Start-Org" => 1, "Start-Position" => 1, Sue => 1,
	       "Transfer-Money" => 1, "Transfer-Ownership" => 1, Transport => 1, "Trial-Hearing" => 1},
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

use vars qw ($opt_R $opt_r $opt_T $opt_t $opt_W $opt_M $opt_N $opt_S $opt_P $opt_V);
use vars qw ($opt_c $opt_m $opt_e $opt_a $opt_s $opt_d $opt_h);

$opt_W = "level";
$opt_M = "arguments";
$opt_N = "both";
$opt_S = "overlapping";
$opt_P = "default_parameters";
$opt_V = "both";

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
   GEN => 0.00,
   NEG => 0.00,
   USP => 0.00);
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

#Quantity scoring parameters
my %quantity_type_wgt =
  ("Contact-Info" => 1.00,
   Crime          => 1.00,
   Illness        => 1.00,
   "Job-Title"    => 1.00,
   Numeric        => 1.00,
   Sentence       => 1.00);
my %quantity_err_wgt = 
  (TYPE    => 0.50,
   SUBTYPE => 0.90);
my $quantity_fa_wgt = 0.75;
my $quantity_mention_coref_fa_wgt = 1.00;

#################################
# SCORING PARAMETER SCHEMES

my %parameter_set =
  (default_parameters => {},
   original_entity_type_weights => {entity_mention_type_wgt => {NAM => 1.00,
								NOM => 0.20,
								PRO => 0.04}},
   );

#################################
# MENTION MAPPING PARAMETERS:

# min_overlap is the minimum mutual fractional overlap allowed
#                for a mention to be declared as successfully detected.
# min_text_match is the minimum fractional contiguous matching string length
#                for a mention to be declared as successfully recognized.
my $min_overlap = 0.3;
my $min_text_match = 0.3;

# max_diff is the maximum extent difference allowed mentions to be declared a "match".
# max_diff_chars is the maximum extent difference in characters for text sources.
# max_diff_time is the maximum extent difference in seconds for audio sources.
# max_diff_xy is the maximum extent difference in centimeters for image mentions.
my $max_diff_chars = 4;
my $max_diff_time = 0.4;
my $max_diff_xy = 0.4;

#################################
# GLOBAL DATA

my (%ref_database, %tst_database, %eval_docs, %sys_docs);
my ($input_file, $input_doc, $input_element, $fatal_input_error_header);
my (%mention_detection_statistics);
my (%name_statistics);
my (%mapped_values, %mapped_document_values, %mention_map, %argument_map);
my %arg_document_values;
my (%source_types, $source_type, $data_type);
my %unique_pages;

my (@entity_mention_types, @entity_types, @entity_classes);
my (@relation_types, @relation_modalities);
my (@event_types, @event_modalities, @quantity_types);

my @error_types = ("correct", "miss", "fa", "error");
my @xdoc_types = ("1", ">1");
my @entity_value_types = ("<=0.2", "0.2-0.5", "0.5-1.0", "1-2", "2-4", ">4");
my @entity_style_types = ("LITERAL", "METONYMIC");
my @entity_mention_count_types = ("1", "2", "3-4", "5-8", ">8");
my @relation_mention_count_types = ("1", ">1");
my @event_mention_count_types = ("1", ">1");
my @timex2_mention_count_types = ("1", ">1");
my @quantity_mention_count_types = ("1", ">1");
my @relation_arg_count_types = ("0", "1", "2", "3", "4", ">4");
my @event_arg_count_types = ("0", "1", "2", "3", "4", ">4");
my @relation_arg_err_count_types = ("0", "1", ">1");
my @event_arg_err_count_types = ("0", "1", ">1");

my $max_string_length_to_print = 40;

my ($score_bound, $max_delta, $print_data);

my ($level_weighting, $mention_overlap_required, $argscore_required, $all_args_required, $nargs_required);
my ($allow_wrong_mapping);

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
  "  -W = \"level\" or \"mention\".  This controls how entities are weighted.\n".
  "         The default is \"$opt_W\" weighting.\n".
  "  -M = \"arguments\" or \"extents\" or \"both\" or \"either\" or \"all\".  This controls\n".
  "         which ref-sys pairs of relations and events are candidates for mapping.\n".
  "         The default is \"$opt_M\".\n".
  "  -N = \"0\" or \"1\" or \"both\" to require, for relation mapping, that 0, 1, or both\n".
  "         Arg-1 and Arg-2 arguments of the sys relation overlap with the corresponding\n".
  "         arguments of the ref relation.  The default is \"$opt_N\".\n".
  "  -S = \"mapped\" or \"overlapping\".  This controls how relation/event arguments\n".
  "         are scored.  The default is \"$opt_S\".  (When \"mapped\" is chosen,\n".
  "         arguments must be correctly mapped at the element level to contribute\n".
  "         value at the relation/event level.)\n".
  "  -P = <parameter_set_name>.  This controls the scoring mode by providing a selection\n".
  "         of different parameters (pre)defined in named parameter sets.\n".
  "  -V = \"both\" or \"attributes\" or \"mentions\" or \"none\".  This controls how arguments\n".
  "         are valued.  For \"attributes\", only attribute errors are considered.\n".
  "         For \"mentions\", only mention errors are considered.  For \"none\", any\n".
  "         candidate is considered to be a perfect match.  The default is \"$opt_V\".\n".
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
  getopts ('R:r:T:t:W:M:N:S:P:V:cmeasdh');
  die $usage if defined $opt_h;

  $level_weighting = $opt_W =~ /level/i;
  $opt_W =~ /^(level|mention)$/i or die
    "\n\nPARAMETER ERROR:  weighting (-W) was specified as '$opt_W' but must be 'level' or 'mention'$usage";

  $mention_overlap_required = ($opt_M =~ /extents/i or $opt_M =~ /both/i);
  $argscore_required = ($opt_M =~ /arguments/i or $opt_M =~ /both/i);
  $all_args_required = $opt_M =~ /all/i;
  $opt_M =~ /^(arguments|extents|both|either|all|all\+extents)$/i or die
    "\n\nPARAMETER ERROR:  relation/event mapping (-M) was specified as '$opt_M' but must be 'arguments', 'extents', 'both', 'either', 'all', or 'all+extents'$usage";

  $opt_N =~ /^(0|1|both)$/i or die
    "\n\nPARAMETER ERROR:  relation argument mapping requirement (-N) was specified as '$opt_N' but must be '0', '1', or 'both'$usage";
  $nargs_required = $opt_N;
  $nargs_required = 2 if $opt_N =~ /both/i;

  $allow_wrong_mapping = $opt_S =~ /overlapping/i;
  $opt_S =~ /^(mapped|overlapping)$/i or die
    "\n\nPARAMETER ERROR:  argument scoring restriction (-S) was specified as '$opt_S' but must be 'mapped' or 'overlapping'$usage";

  $opt_V =~ /^(both|attributes|mentions|none)$/i or die
    "\n\nPARAMETER ERROR:  argument valuation (-V) was specified as '$opt_V' but must be 'full' or 'attributes' or 'mentions' or 'none'$usage";
  
  select_parameter_set ();

  not $opt_c or ($opt_R and $opt_R eq $opt_T) or ($opt_r and $opt_r eq $opt_t) or die
    "\n\nPARAMETER ERROR:  ref and sys input must be the same when checking for duplicate elements (-c)$usage";

  $opt_R xor $opt_r or die
    "\n\nPARAMETER ERROR:  error in specifying data to process (one of -R or -r, but not both)$usage";
  $opt_T xor $opt_t or die
    "\n\nPARAMETER ERROR:  error in specifying data to process (one of -T or -t, but not both)$usage";
  print_parameters ();

#read in the data
  my ($t0, $t1, $t2, $t3);
  $t0 = (times)[0];
  get_data (\%ref_database, \%eval_docs, $opt_R, $opt_r);
  get_data (\%tst_database, \%sys_docs, $opt_T, $opt_t);
  check_docs ();
  print_documents ("REF", \%eval_docs) if $opt_s;
  print_documents ("TST", \%sys_docs) if $opt_s;
  foreach my $type ("entities", "quantities", "timex2s", "relations", "events") {
    copy_elements (\%ref_database, $type, "saved_$type") if $opt_m;
    copy_elements (\%tst_database, $type, "saved_$type") if $opt_m;
  }
  $t1 = (times)[0];
  printf STDERR "\ndata input:           %8.2f secs to load data\n", $t1-$t0;

#evaluate entities
  $t0 = $t1;
  compute_element_values ("entities");
  $t3 = $t2 = $t1 = (times)[0];
  if ((keys %{$ref_database{entities}})>0) {
    map_elements ($ref_database{entities}, $tst_database{entities}, \&map_entity_mentions);
    $t3 = $t2 = (times)[0];
    if ($opt_m) {
      copy_elements (\%ref_database, "entities", "mapped_refs");
      copy_elements (\%tst_database, "entities", "mapped_refs");
    }
    print_entities ("REF", "entities", $ref_database{entities});
    if ((keys %{$tst_database{entities}})>0) {
      print_entities ("TST", "entities", $tst_database{entities});
      print_entity_mapping ("entity");
      print "\n======== entity scoring ========\n";
      print "\nEntity Detection and Recognition statistics:\n";
      score_entity_detection ();
      score_entity_attribute_recognition ();
      (my $detection_stats, my $role_stats, my $style_stats) = mention_recognition_stats ("entities");
      score_entity_mention_detection ($detection_stats);
      score_entity_mention_attribute_recognition ($role_stats, $style_stats);
      document_level_scores ("entities") if $opt_d;
    }
    $t3 = (times)[0];
    printf STDERR "entity eval:          %8.2f secs to compute values,%8.2f secs to map,%8.2f secs to score\n", $t1-$t0, $t2-$t1, $t3-$t2;
  }

#evaluate quantity expressions
  $t0 = $t3;
  compute_element_values ("quantities");
  $t3 = $t2 = $t1 = (times)[0];
  if ((keys %{$ref_database{quantities}})>0) {
    map_elements ($ref_database{quantities}, $tst_database{quantities}, \&map_entity_mentions);
    $t3 = $t2 = (times)[0];
    if ($opt_m) {
      copy_elements (\%ref_database, "quantities", "mapped_refs");
      copy_elements (\%tst_database, "quantities", "mapped_refs");
    }
    print_quantities ("REF", "values", $ref_database{quantities}, \@quantity_attributes);
    if ((keys %{$tst_database{quantities}})>0) {
      print_quantities ("TST", "values", $tst_database{quantities}, \@quantity_attributes);
      print_quantity_mapping ($ref_database{quantities}, $tst_database{quantities}, "value", \@quantity_attributes);
      print "\n======== value scoring ========\n";
      print "\nValue Detection and Recognition statistics:\n";
      score_quantity_detection ();
      my $attribute_stats = attribute_confusion_stats ("quantities", \@quantity_attributes);
      foreach my $attribute (@quantity_attributes) {
	next if $attribute eq "ID";
	score_confusion_stats ($attribute_stats->{$attribute}, "attribute $attribute");
      }
      document_level_scores ("quantities") if $opt_d;
    }
    $t3 = (times)[0];
    printf STDERR "value eval:           %8.2f secs to compute values,%8.2f secs to map,%8.2f secs to score\n", $t1-$t0, $t2-$t1, $t3-$t2;
  }

#evaluate timex2 expressions
  $t0 = $t3;
  compute_element_values ("timex2s");
  $t3 = $t2 = $t1 = (times)[0];
  if ((keys %{$ref_database{timex2s}})>0) {
    map_elements ($ref_database{timex2s}, $tst_database{timex2s}, \&map_entity_mentions);
    $t3 = $t2 = (times)[0];
    if ($opt_m) {
      copy_elements (\%ref_database, "timex2s", "mapped_refs");
      copy_elements (\%tst_database, "timex2s", "mapped_refs");
    }
    print_quantities ("REF", "timexs", $ref_database{timex2s}, \@timex2_attributes);
    if ((keys %{$tst_database{timex2s}})>0) {
      print_quantities ("TST", "timexs", $tst_database{timex2s}, \@timex2_attributes);
      print_quantity_mapping ($ref_database{timex2s}, $tst_database{timex2s}, "timex2", \@timex2_attributes);
      print "\n======== timex2 scoring ========\n";
      print "\nTimex2 Detection and Recognition statistics:\n";
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
  $t0 = $t3;
  if ((keys %{$ref_database{relations}})>0 and (keys %{$tst_database{relations}})>0) {
    compute_releve_values ($ref_database{relations}, $tst_database{relations});
    $t1 = (times)[0];
    map_elements ($ref_database{relations}, $tst_database{relations}, \&map_releve_arguments);
    $t2 = (times)[0];
    print_releves ("REF", "relations", $ref_database{relations}, \@relation_attributes);
    print_releves ("TST", "relations", $tst_database{relations}, \@relation_attributes);
    print_releve_mapping ($ref_database{relations}, $tst_database{relations}, "relation", \@relation_attributes);
    print "\n======== relation scoring ========\n";
    print "\nRelation Detection and Recognition statistics:\n";
    score_relation_detection ();
    score_releve_attribute_recognition ("relations", \@relation_types, \%relation_attributes, \@relation_modalities);
    document_level_scores ("relations") if $opt_d;
    $t3 = (times)[0];
    printf STDERR "relation eval:        %8.2f secs to compute values,%8.2f secs to map,%8.2f secs to score\n", $t1-$t0, $t2-$t1, $t3-$t2;
    $t0 = $t3;
  }

#evaluate events
  if ((keys %{$ref_database{events}})>0 and (keys %{$tst_database{events}})>0) {
    compute_releve_values ($ref_database{events}, $tst_database{events});
    $t1 = (times)[0];
    map_elements ($ref_database{events}, $tst_database{events}, \&map_releve_arguments);
    $t2 = (times)[0];
    print_releves ("REF", "events", $ref_database{events}, \@event_attributes);
    print_releves ("TST", "events", $tst_database{events}, \@event_attributes);
    print_releve_mapping ($ref_database{events}, $tst_database{events}, "event", \@event_attributes);
    print "\n======== event scoring ========\n";
    print "\nEvent Detection and Recognition statistics:\n";
    score_event_detection ();
    score_releve_attribute_recognition ("events", \@event_types, \%event_attributes, \@event_modalities);
    document_level_scores ("events") if $opt_d;
    $t3 = (times)[0];
    printf STDERR "event eval:           %8.2f secs to compute values,%8.2f secs to map,%8.2f secs to score\n", $t1-$t0, $t2-$t1, $t3-$t2;
    $t0 = $t3;
  }
  exit unless $opt_m;

#evaluate entity mentions
  if ((keys %{$ref_database{entities}})>0 and (keys %{$tst_database{entities}})>0) {
    undef %mapped_values, undef %mapped_document_values, undef %mention_map;
    for my $db (\%ref_database, \%tst_database) {
      promote_mentions ($db, "saved_entities", "mention_entities");
      replace_elements ($db, "mention_entities", "entities");
    }
    compute_element_values ("entities");
    $t1 = (times)[0];
    map_elements ($ref_database{entities}, $tst_database{entities}, \&map_entity_mentions);
    $t2 = (times)[0];
    print_entities ("REF", "mention_entities", $ref_database{entities});
    print_entities ("TST", "mention_entities", $tst_database{entities});
    print_entity_mapping ("mention_entity");
    print "\n======== mention_entity scoring ========\n";
    print "\nEntity Mention Detection statistics:\n";
    score_entity_detection ();
    score_entity_attribute_recognition ();
    document_level_scores ("mention_entities") if $opt_d;
    $t3 = (times)[0];
    printf STDERR "entity mention eval:  %8.2f secs to compute values,%8.2f secs to map,%8.2f secs to score\n", $t1-$t0, $t2-$t1, $t3-$t2;
    $t0 = $t3;
  }

#releve mention evaluation preparation
  undef %mapped_values, undef %mapped_document_values, undef %mention_map;
  undef %arg_document_values;
  if ((keys %{$ref_database{relations}})>0 and (keys %{$tst_database{relations}})>0 or
      (keys %{$ref_database{events}})   >0 and (keys %{$tst_database{events}})   >0) {
    replace_elements (\%ref_database, "saved_entities", "entities");
    replace_elements (\%tst_database, "mention_entities", "entities");
    compute_element_values ("entities");
    foreach my $type ("quantities", "timex2s") {
      replace_elements (\%ref_database, "saved_$type", "$type");
      promote_mentions (\%tst_database, "saved_$type", "mention_$type");
      replace_elements (\%tst_database, "mention_$type", "$type");
      compute_element_values ($type);
    }
    $t1 = (times)[0];
    printf STDERR "mention preparation:  %8.2f secs to compute and map entity/value/timex2 data\n", $t1-$t0;
    $t0 = $t1;
  }

#evaluate relation mentions
  if ((keys %{$ref_database{relations}})>0 and (keys %{$tst_database{relations}})>0) {
    foreach my $db (\%ref_database, \%tst_database) {
      promote_mentions ($db, "saved_relations", "mention_relations", $db eq \%ref_database);
      replace_elements ($db, "mention_relations", "relations");
      check_arguments ("relations", $db);
    }
    compute_releve_values ($ref_database{relations}, $tst_database{relations});
    $t1 = (times)[0];
    map_elements ($ref_database{relations}, $tst_database{relations}, \&map_releve_arguments);
    $t2 = (times)[0];
    print_releves ("REF", "mention_relations", $ref_database{relations}, \@relation_attributes);
    print_releves ("TST", "mention_relations", $tst_database{relations}, \@relation_attributes);
    print_releve_mapping ($ref_database{relations}, $tst_database{relations}, "mention_relation", \@relation_attributes);
    print "\n======== mention_relation scoring ========\n";
    print "\nRelation Mention Detection and Recognition statistics:\n";
    score_relation_detection ();
    score_releve_attribute_recognition ("mention_relations", \@relation_types, \%relation_attributes, \@relation_modalities);
    document_level_scores ("mention_relations") if $opt_d;
    $t3 = (times)[0];
    printf STDERR "relation mention eval:%8.2f secs to compute values,%8.2f secs to map,%8.2f secs to score\n", $t1-$t0, $t2-$t1, $t3-$t2;
    $t0 = $t3;
  }

#evaluate event mentions
  if ((keys %{$ref_database{events}})>0 and (keys %{$tst_database{events}})>0) {
    foreach my $db (\%ref_database, \%tst_database) {
      promote_mentions ($db, "saved_events", "mention_events", $db eq \%ref_database);
      replace_elements ($db, "mention_events", "events");
      check_arguments ("events", $db);
    }
    compute_releve_values ($ref_database{events}, $tst_database{events});
    $t1 = (times)[0];
    map_elements ($ref_database{events}, $tst_database{events}, \&map_releve_arguments);
    $t2 = (times)[0];
    print_releves ("REF", "mention_events", $ref_database{events}, \@event_attributes);
    print_releves ("TST", "mention_events", $tst_database{events}, \@event_attributes);
    print_releve_mapping ($ref_database{events}, $tst_database{events}, "mention_event", \@event_attributes);
    print "\n======== mention_event scoring ========\n";
    print "\nEvent Mention Detection and Recognition statistics:\n";
    score_event_detection ();
    score_releve_attribute_recognition ("mention_events", \@event_types, \%event_attributes, \@event_modalities);
    document_level_scores ("mention_events") if $opt_d;
    $t3 = (times)[0];
    printf STDERR "event mention eval:   %8.2f secs to compute values,%8.2f secs to map,%8.2f secs to score\n", $t1-$t0, $t2-$t1, $t3-$t2;
    $t0 = $t3;
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
  foreach my $type ("entity", "quantity", "timex2", "relation", "event",
		    "mention_entity", "mention_relation", "mention_event") {
    my $types = $type."s";
    $types =~ s/ys$/ies/;
    while ((my $id, my $ref) = each %{$db->{$types}}) {
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

  check_arguments ("relations", $db);
  check_arguments ("events", $db);
}

#################################

sub check_arguments {

  my ($elements, $db) = @_;

  my $type;
  while ((my $id, my $element) = each %{$db->{$elements}}) {
    $type = $element->{ELEMENT_TYPE};
    my %valid_arg_mention_ids;
    while ((my $role, my $arg_ids) = each %{$element->{arguments}}) {
      foreach my $arg_id (keys %$arg_ids) {
	defined $db->{refs}{$arg_id} or die
	  "\n\nFATAL INPUT ERROR:  $type '$id' references argument '$arg_id' in role '$role'\n".
	  "    but this argument has not been loaded\n";
	foreach my $mention_id (element_mention_ids ($db, $arg_id)) {
	  $valid_arg_mention_ids{$role}{$mention_id} = 1;
	}
      }
    }
    while ((my $doc, my $doc_element) = each %{$element->{documents}}) {
      foreach my $mention (@{$doc_element->{mentions}}) {
	while ((my $role, my $arg_ids) = each %{$mention->{arguments}}) {
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
  while ((my $doc, my $doc_element) = each %{$db->{refs}{$id}{documents}}) {
    foreach my $mention (@{$doc_element->{mentions}}) {
      push @mention_ids, $mention->{ID};
    }
  }
  return @mention_ids;
}

#################################

sub set_params {
  my ($prms, $data) = @_;

  while ((my $key, my $value) = each %$data) {
    $prms->{$key} = $value if defined $value;
  }
  my @sorted_names = sort {$prms->{$b} <=> $prms->{$a} ? $prms->{$b} <=> $prms->{$a} : $a cmp $b;} keys %$prms;
  return @sorted_names;
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

#Quantity parameters
  @quantity_types = set_params (\%quantity_type_wgt, $parms->{quantity_type_wgt});
  $quantity_fa_wgt = $p if defined ($p = $parms->{quantity_fa_wgt});
  $quantity_mention_coref_fa_wgt = $p if defined ($p = $parms->{quantity_mention_coref_fa_wgt});
}

#################################

sub print_parameters {

  printf "\n Entity scoring is %s weighted\n".
    " Relation/Event mapping is allowed when %s criterion is satisfied\n".
    " Overlap of %s of Arg-* relation arguments is required for mapping\n".
    " Arguments contribute to scoring when arguments are %s\n".
    " Scoring mode parameter set is \"$opt_P\"\n", uc $opt_W, uc $opt_M, uc $opt_N, uc $opt_S;

  printf "\n".
    "  min acceptable overlap of matching mention heads or names:\n".
    "%11.1f percent\n".
    "  min acceptable run of matching characters in mention heads:\n".
    "%11.1f percent\n".
    "  max acceptable extent difference for names and mentions to match:\n".
    "%11d chars for text sources\n".
    "%11.3f sec for audio sources\n".
    "%11.3f cm for image sources\n",
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

#Quantity parameters
  print "\n";
  print "  Value value weights for value types:\n";
  foreach my $type (@quantity_types) {
    printf "%11.3f for type %s\n", $quantity_type_wgt{$type}, $type;
  }
  print "  Value value discounts for value attribute recognition errors:\n";
  foreach my $type (sort keys %quantity_err_wgt) {
    printf "%11.3f for $type errors\n", $quantity_err_wgt{$type};
  }
  printf "  Value mention value (cost) for spurious value mentions:%6.3f\n", $quantity_fa_wgt;
  printf "  Value mention value (cost) discount for incorrect coreference:%6.3f\n", $quantity_mention_coref_fa_wgt;
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
    $db->{refs}{$id} = $dbdst->{$id} = copy_element ($dbsrc->{$id});
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

sub copy_elements {
#This subroutine copies all elements in the database of type "$srcname"
#to a new type called "$dstname".  Any existing elements of type "$dstname"
#are discarded.  The database list of references is NOT updated.
  my ($db, $srcname, $dstname) = @_;

  $db->{$dstname} = {} unless $db->{$dstname};
  while (my($key, $value) = each %{$db->{$srcname}}) {
    $db->{$dstname}{$key} = copy_element ($value);
  }
}

#################################

sub copy_element {
#This subroutine creates a separate and independent copy of an element,
#at least down through all components that may be updated in the evaluation
#process, including mentions and arguments.
  my ($src) = @_;

  my $dst = {%$src};
  foreach my $type ("names", "mentions", "external_links", "arguments") {
    delete $dst->{$type} if $dst->{$type};
    if ($type eq "arguments") {
      while ((my $role, my $ids) = each %{$src->{arguments}}) {
	while ((my $id, my $arg) = each %$ids) {
	  $dst->{arguments}{$role}{$id} = {%$arg};
	}
      }
    } else {
      $dst->{$type} = [];
      foreach my $value (@{$src->{$type}}) {
	push @{$dst->{$type}}, {%$value};
      }
    }
  }

  delete $dst->{documents};
  return $dst unless $src->{documents};
  while ((my $doc, my $doc_src) = each %{$src->{documents}}) {
    my $doc_dst = $dst->{documents}{$doc} = {%$doc_src};
    foreach my $type ("names", "mentions", "external_links", "arguments") {
      delete $doc_dst->{$type} if $doc_dst->{$type};
      if ($type eq "arguments") {
	while ((my $role, my $ids) = each %{$doc_src->{arguments}}) {
	  while ((my $id, my $arg) = each %$ids) {
	    $doc_dst->{arguments}{$role}{$id} = {%$arg};
	  }
	}
      } else {
	$doc_dst->{$type} = [];
	foreach my $value (@{$doc_src->{$type}}) {
	  push @{$doc_dst->{$type}}, {%$value};
	}
      }
    }
  }
  return $dst;
}

#################################

sub score_entity_detection {

  conditional_performance ("entities", "entity", "type", "TYPE", \@entity_types);
  conditional_performance ("entities", "entity", "level", "LEVEL", \@entity_mention_types);
  conditional_performance ("entities", "entity", "value", "VALUE", \@entity_value_types);
  conditional_performance ("entities", "mention", "count", "MENTION COUNT", \@entity_mention_count_types);
  conditional_performance ("entities", "entity", "class", "CLASS", \@entity_classes);
  my @source_types = sort keys %source_types;
  conditional_performance ("entities", "source", "type", "SOURCE", \@source_types);
#  conditional_performance ("entities", "Ndoc", "type", "CROSS-DOC", \@xdoc_types, "TYPE", @entity_types);
  conditional_performance ("entities", "entity", "type", "TYPE", \@entity_types, undef, undef, 1);
  conditional_performance ("entities", "entity", "value", "VALUE", \@entity_value_types, undef, undef, 1);
  my @subtypes = sort keys %{$entity_attributes{SUBTYPE}};
  conditional_performance ("entities", "entity", "subtype", "SUBTYPE", \@subtypes, "TYPE", \@entity_types);
}

#################################

sub document_level_scores {

  my ($type) = @_;
  (my $elements, my $task) = ($type =~ /entities/ ? ("entities", "EDR")  :
			      ($type =~ /quantities/ ? ("quantities", "VAL") :
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
    printf "%10.2f %% (%.5f out of %.5f) $task score for $doc\n", 100*max($score,-99.9999), $sys_value, $ref_value;
  }
}

#################################

sub conditional_performance {

  my ($elements, $label1, $label2, $cond1, $c1s, $cond2, $c2s, $external_reconciliation) = @_;

  my $hdr1 = "________Count________     __________Count_(%)__________        ______________Cost_(%)________________       __Unconditioned_Cost_(%)_";
  my $hdr2 = "Ent   Detection   Rec     Detection   Rec    Unweighted        Detection   Rec   Value    Value-based       Max      Detection    Rec";
  my $hdr3 = "Tot    FA  Miss   Err      FA  Miss   Err    Pre--Rec--F        FA  Miss   Err     (%)    Pre--Rec--F      Value     FA   Miss    Err";

  (my $count, my $cost, my $nrm_cost) = conditional_error_stats ($elements, $cond1, $cond2, $external_reconciliation);

  if ($cond2) {
    foreach my $cond ("ALL", @$c2s) {
      next unless $count and $count->{$cond};
      print "\nEvaluation of externally reconciled elements:" if $external_reconciliation;
      print "\nPerformance statistics for $cond2 = $cond:";
      printf "\n ref      %s\n %-8s %s\n %-8s %s\n", $hdr1, $label1, $hdr2, $label2, $hdr3;
      foreach my $type (@$c1s, "total") {
	print_eval ($type, $count->{$cond}{$type}, $cost->{$cond}{$type}, $nrm_cost->{$cond}{$type}, $nrm_cost->{ALL}{total});
      }
    }
  } else {
    return unless %$count;
    print "\nEvaluation of externally reconciled elements:" if $external_reconciliation;
    printf "\n ref      %s\n %-8s %s\n %-8s %s\n", $hdr1, $label1, $hdr2, $label2, $hdr3;
    foreach my $type (@$c1s, "total") {
      print_eval ($type, $count->{$type}, $cost->{$type}, $nrm_cost->{$type}, $nrm_cost->{total});
    }
  }
}

#################################

sub print_eval {

  my ($type, $count, $cost, $ref_value, $total_value) = @_;

  return unless (defined $count->{correct} or
		 defined $count->{error} or
		 defined $count->{miss} or
		 defined $count->{fa});
  my $format = "%7.7s%6d%6d%6d%6d%8.1f%6.1f%6.1f%7.1f%5.1f%5.1f%8.1f%6.1f%6.1f%8.1f%7.1f%5.1f%5.1f%9.2f%7.2f%7.2f%7.2f\n";

  foreach my $category ("correct", "error", "miss", "fa") {
    $count->{$category} = 0 unless defined $count->{$category};
    $cost->{$category} = 0 unless defined $cost->{$category};
  }
  $ref_value = 0 unless defined $ref_value;
  my $nref = $count->{correct}+$count->{error}+$count->{miss};
  my $nsys = $count->{correct}+$count->{error}+$count->{fa};
  my $pn = 100/max(1E-30, $nref);
  my $cn = 100/max(1E-30, $ref_value);

  my $recall = $count->{correct}/max($nref,1E-30);
  my $precision = $count->{correct}/max($nsys,1E-30);
  my $fmeasure = 2*$precision*$recall/max($precision+$recall, 1E-30);

  my $value_correct = $ref_value-$cost->{miss}-$cost->{error}-$cost->{correct};
  my $value_recall = max($value_correct,0)/max(1E-30, $ref_value);
  my $sys_value = $ref_value-$cost->{miss}+$cost->{fa};
  my $value_precision = max($value_correct,0)/max(1E-30, $sys_value);
  my $value_fmeasure = 2*$value_precision*$value_recall/max($value_precision+$value_recall, 1E-30);

  my $un = 100/max($total_value,1E-30);
  printf $format, $type, $nref, $count->{fa}, $count->{miss}, $count->{error},
  min(999.9,$pn*$count->{fa}), $pn*$count->{miss}, $pn*$count->{error}, 100*$precision, 100*$recall, 100*$fmeasure,
  min(999.9,$cn*$cost->{fa}), $cn*$cost->{miss}, $cn*($cost->{error}+$cost->{correct}),
  max(-999.9,$cn*($value_correct-$cost->{fa})), 100*$value_precision, 100*$value_recall, 100*$value_fmeasure,
  $un*$ref_value, min(999.99,$un*$cost->{fa}), $un*$cost->{miss}, $un*($cost->{error}+$cost->{correct});
}

#################################

sub score_relation_detection {

  conditional_performance ("relations", "relation", "type", "TYPE", \@relation_types);
  conditional_performance ("relations", "modality", "type", "MODALITY", \@relation_modalities);
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

  conditional_performance ("events", "event", "type", "TYPE", \@event_types);
  conditional_performance ("events", "modality", "type", "MODALITY", \@event_modalities);
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
  conditional_performance ("timex2s", "source", "type", "SOURCE", \@types);
}

#################################

sub score_quantity_detection {

  my @types = sort keys %source_types;
  conditional_performance ("quantities", "source", "type", "SOURCE", \@types);
}

#################################

sub score_confusion_stats {

  my ($stats, $label) = @_;
  my $maxdisplay = 8;

#display dominant confusion statistics
  return unless $stats;
  my (%ref_count, %tst_count, %sort_count);
  my $ntot = my $nref = my $ncor = my $nfa = my $nmiss = 0;
#select attribute values that contribute the most confusions
  while ((my $ref_value, my $tst_stats) = each %$stats) {
    while ((my $tst_value, my $count) = each %$tst_stats) {
      $ref_count{$ref_value} += $count;
      $tst_count{$tst_value} += $count;
      $ntot += $count;
      $nref += $count unless $ref_value eq "<undef>";
      if ($tst_value eq $ref_value) {
	$sort_count{$ref_value} += $epsilon*$count;
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
  printf "    Summary (count/percent):  Nref=%d/%.1f%s, Nfa=%d/%.1f%s, Nmiss=%d/%.1f%s, Nsub=%d/%.1f%s, Nerr=%d/%.1f%s"
    .", P/R/F=%.1f%s/%.1f%s/%.1f%s\n",
    $nref, 100, "%", $nfa, min(999.9,100*$pfa), "%", $nmiss, 100*$pmiss, "%",
    $nsub, min(999.9,100*$psub), "%", $nerr, min(999.9,100*$perror), "%",
    100*$precision, "%", 100*$recall, "%", 100*$f, "%";
  print "    Confusion matrix for major error contributors (count/percent):\n        ref\\tst:";
  push @display_values, $others if $display_count != $ntot;
  foreach my $tst_value (@display_values) {
    printf "%11.11s ", $tst_value;
  }
  print "\n";
  foreach my $ref_value (@display_values) {
    printf "  %14.14s", $ref_value;
    foreach my $tst_value (@display_values) {
      my $count = $stats->{$ref_value}{$tst_value};
      printf "%s", $count ? 
	(sprintf "%6d/%4.1f%s", $count, min(99.9,100*$count/max($nref,$epsilon)), "%") :
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
  " ref     ____________count______________       ____________percent____________\n",
  " entity  Detection     Extent_Recognition      Detection     Extent_Recognition\n",
  " type    miss    fa     miss   err  corr       miss    fa     miss   err  corr\n";
  foreach $type (@entity_types, "total") {
    my $total = ($name_stats->{$type}{miss} +
		 $name_stats->{$type}{error} +
		 $name_stats->{$type}{correct});
    printf "%5.5s%8d%6d%9d%6d%6d%11.1f%6.1f%9.1f%6.1f%6.1f\n", $type,
    $name_stats->{$type}{miss}, $name_stats->{$type}{fa}, $name_stats->{$type}{miss},
    $name_stats->{$type}{error}, $name_stats->{$type}{correct},
    100*$name_stats->{$type}{miss}/max($total,1), 100*$name_stats->{$type}{fa}/max($total,1),
    100*$name_stats->{$type}{miss}/max($total,1), 100*$name_stats->{$type}{error}/max($total,1),
    100*$name_stats->{$type}{correct}/max($total,1);
  }
}

#################################

sub score_entity_mention_detection {

  my ($mention_stats) = @_;

  my (%men_count, $type, $men_type, $rol_type, $sty_type, $err_type);
  my ($pn);

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
    " ref     ____________count______________       ____________percent____________\n",
      " mention Detection     Extent_Recognition      Detection     Extent_Recognition\n",
	" type    miss    fa     miss   err  corr       miss    fa     miss   err  corr\n";
  foreach $type (@entity_mention_types, "total") {
    $pn = 100/max($epsilon, $men_count{$type}{miss}+$men_count{$type}{error}+$men_count{$type}{correct});
    printf "%5.5s%8d%6d%9d%6d%6d%11.1f%6.1f%9.1f%6.1f%6.1f\n", $type,
    $men_count{$type}{miss}, $men_count{$type}{fa},
    $men_count{$type}{miss}, $men_count{$type}{error}, $men_count{$type}{correct},
    $pn*$men_count{$type}{miss}, min(999.9,$pn*$men_count{$type}{fa}),
    $pn*$men_count{$type}{miss}, $pn*$men_count{$type}{error}, $pn*$men_count{$type}{correct};
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
    " ref     ____________count______________       ____________percent____________\n",
      " mention Detection     Extent_Recognition      Detection     Extent_Recognition\n",
	" style   miss    fa     miss   err  corr       miss    fa     miss   err  corr\n";
  foreach $type (@entity_style_types, "total") {
    $pn = 100/max($epsilon, $men_count{$type}{miss}+$men_count{$type}{error}+$men_count{$type}{correct});
    printf "%5.5s%8d%6d%9d%6d%6d%11.1f%6.1f%9.1f%6.1f%6.1f\n", $type,
    $men_count{$type}{miss}, $men_count{$type}{fa},
    $men_count{$type}{miss}, $men_count{$type}{error}, $men_count{$type}{correct},
    $pn*$men_count{$type}{miss}, min(999.9,$pn*$men_count{$type}{fa}),
    $pn*$men_count{$type}{miss}, $pn*$men_count{$type}{error}, $pn*$men_count{$type}{correct};
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
  " ref     ____________count______________       ____________percent____________\n",
  " mention Detection     Extent_Recognition      Detection     Extent_Recognition\n",
  " role    miss    fa     miss   err  corr       miss    fa     miss   err  corr\n";
  foreach $type (@entity_types, "total") {
    $pn = 100/max($epsilon, $men_count{$type}{miss}+$men_count{$type}{error}+$men_count{$type}{correct});
    printf "%5.5s%8d%6d%9d%6d%6d%11.1f%6.1f%9.1f%6.1f%6.1f\n", $type,
    $men_count{$type}{miss}, $men_count{$type}{fa},
    $men_count{$type}{miss}, $men_count{$type}{error}, $men_count{$type}{correct},
    $pn*$men_count{$type}{miss}, min(999.9,$pn*$men_count{$type}{fa}),
    $pn*$men_count{$type}{miss}, $pn*$men_count{$type}{error}, $pn*$men_count{$type}{correct};
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
  my ($type, $ref_type, %type_stats);
  my (%type_total);
  foreach $ref_type (@$releve_types) {
    foreach $type (@$releve_types) {
      $type_stats{$ref_type}{$type} = defined $attribute_stats->{TYPE}{$ref_type}{$type} ?
	$attribute_stats->{TYPE}{$ref_type}{$type} : 0;
      $type_total{$ref_type} += $type_stats{$ref_type}{$type};
    }
  }
  print "\n", ucfirst "$element_type Type confusion matrix for \"$source_type\" sources (for mapped $element_types):\n",
  "             COUNT", "." x (6*(@$releve_types-1)), "    PERCENT", "." x (6*(@$releve_types-1)), "\n    ref\\tst:";
  foreach $type (@$releve_types) {
    printf " %5.5s", $type;
  }
  print "     ";
  foreach $type (@$releve_types) {
    printf " %5.5s", $type;
  }
  print "\n";
  foreach $ref_type (@$releve_types) {
    printf "%11.11s ", $ref_type;
    foreach $type (@$releve_types) {
      printf "%6d", $type_stats{$ref_type}{$type};
    }
    print "     ";
    foreach $type (@$releve_types) {
      printf "%6.1f", 100*$type_stats{$ref_type}{$type} /
	max($type_total{$ref_type},1);
    }
    print "\n";
  }

  # subtype attributes
  my $subtype_stats = subtype_confusion_stats ($element_key);
  my ($subtype);
  foreach $ref_type (@$releve_types) {
    my $stats = $subtype_stats->{$ref_type};
    next unless $stats;
    my (@subtypes, %subtype_total);
    @subtypes = sort keys %{$attributes->{TYPE}{$ref_type}};
    foreach $subtype (@subtypes) {
      foreach $type (@subtypes) {
	$stats->{$subtype}{$type} = 0 unless
	  defined $stats->{$subtype} and defined $stats->{$subtype}{$type};
	$subtype_total{$subtype} += $stats->{$subtype}{$type};
      }
    }
    print "\n", ucfirst "$element_type Subtype confusion matrix for \"$source_type\" sources (for mapped $element_types):\n";
    printf "  type=%-5.5s", $ref_type;
    print " COUNT", "." x (6*(@subtypes-1)), "    PERCENT", "." x (6*(@subtypes-1)), "\n    ref\\tst: ";
    foreach $subtype (@subtypes) {
      printf "%5.5s ", $subtype;
    }
    print "     ";
    foreach $subtype (@subtypes) {
      printf "%5.5s ", $subtype;
    }
    print "\n";
    foreach $subtype (@subtypes) {
      printf "%11.11s ", $subtype;
      foreach $type (@subtypes) {
	printf "%6d", $stats->{$subtype}{$type};
      }
      print "     ";
      foreach $type (@subtypes) {
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
  score_confusion_stats ($role_stats, "argument ROLE");
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

  my ($element, $doc, $condition) = @_;

  my $doc_element = $element->{documents}{$doc};
  my ($value, $count_types, $narg_errs);
  if ($condition eq "MENTION COUNT") {
    $count_types = ($element->{ELEMENT_TYPE} =~ /entity/ ?   \@entity_mention_count_types :
		    $element->{ELEMENT_TYPE} =~ /relation/ ? \@relation_mention_count_types :
		    $element->{ELEMENT_TYPE} =~ /event/ ?    \@event_mention_count_types :
		    $element->{ELEMENT_TYPE} =~ /timex2/ ?   \@timex2_mention_count_types :
		    $element->{ELEMENT_TYPE} =~ /quantity/ ? \@quantity_mention_count_types :
		    die "\n\nFATAL ERROR:  unknown element type ($element->{ELEMENT_TYPE}) in condition_type\n");
    $value = value_type (scalar @{$doc_element->{mentions}}, $count_types);
  } elsif ($condition eq "ARGUMENT ERRORS") {
    $count_types = ($element->{ELEMENT_TYPE} =~ /relations/ ? \@relation_arg_err_count_types :
		                                              \@event_arg_err_count_types);
    $narg_errs = $element->{MAP} ? num_argument_mapping_errors ($element) : 0;
    $value = value_type ($narg_errs, $count_types);
  } elsif ($condition eq "NUM ARGUMENTS") {
    $count_types = ($element->{ELEMENT_TYPE} =~ /relations/ ? \@relation_arg_count_types :
		                                              \@event_arg_count_types);
    my $num_args = 0;
    while ((my $ref_role, my $ref_ids) = each %{$element->{arguments}}) {
      $num_args += keys %$ref_ids;
    }
    $value = value_type ($num_args, $count_types);
  } else {
    $value = 
      $condition =~ /^(TYPE|SUBTYPE|CLASS|MODALITY)$/ ? $element->{$condition} :
      $condition =~ /^(SOURCE|LEVEL)$/ ?                $doc_element->{$condition} :
      $condition eq "VALUE" ?     value_type ($doc_element->{VALUE}, \@entity_value_types) :
      $condition eq "CROSS-DOC" ? value_type (scalar @{$element->{documents}}, \@xdoc_types) :
      undef;
  }
  defined $value or die
    "\n\nFATAL ERROR:  unknown condition ($condition) in call to condition_value\n";
  return $value eq "" ? "<null>" : $value;
}

#################################

sub conditional_error_stats {

  my ($elements, $cond1, $cond2, $external_reconciliation) = @_;

  my $attributes =
    $elements eq "entities" ?  \@entity_attributes :
    $elements eq "relations" ? \@relation_attributes :
    $elements eq "events" ?    \@event_attributes :
    $elements eq "timex2s" ?   \@timex2_attributes :
                               \@quantity_attributes;
    
#accumulate statistics over all documents
  my (%error_count, %cumulative_cost, %normalizing_cost, %document_costs);
  while ((my $id, my $element) = each %{$ref_database{$elements}}) {
    next if ($external_reconciliation and not @{$element->{external_links}});
    while ((my $doc, my $doc_ref) = each %{$element->{documents}}) {
      my $doc_tst=$doc_ref->{MAP};
      my $cost = my $norm_cost = $doc_ref->{VALUE};
      my ($err_type, $att_errs);
      if ($doc_tst) {
	my $mapped_value = $mapped_document_values{$doc_ref->{ID}}{$doc_tst->{ID}}{$doc};
	$cost = $doc_ref->{VALUE} - $mapped_value;
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
	    $error_count{$k2}{$k1}{$err_type}++;
	    $cumulative_cost{$k2}{$k1}{$err_type} += $cost;
	    $normalizing_cost{$k2}{$k1} += $norm_cost;
	  }
	}
      } else {
	foreach my $k1 ($c1_value, "total") {
	  $error_count{$k1}{$err_type}++;
	  $cumulative_cost{$k1}{$err_type} += $cost;
	  $normalizing_cost{$k1} += $norm_cost;
	}
      }
      $document_costs{$doc}{REF} += $norm_cost;
      $document_costs{$doc}{SYS} += $cost;
    }
  }

#update entity false alarm statistics
  while ((my $id, my $element) = each %{$tst_database{$elements}}) {
    next if ($external_reconciliation and not @{$element->{external_links}});
    while ((my $doc, my $doc_tst) = each %{$element->{documents}}) {
      next if $doc_tst->{MAP};
      my $norm_cost = 0;
      my $cost = -$doc_tst->{FA_VALUE};
      my $err_type = "fa";
      my $c1_value = condition_value ($element, $doc, $cond1);
      if ($cond2) {
	my $c2_value = condition_value ($element, $doc, $cond2);
	foreach my $k2 ($c2_value, "ALL") {
	  foreach my $k1 ($c1_value, "total") {
	    $error_count{$k2}{$k1}{$err_type}++;
	    $cumulative_cost{$k2}{$k1}{$err_type} += $cost;
	    $normalizing_cost{$k2}{$k1} += $norm_cost;
	  }
	}
      } else {
	foreach my $k1 ($c1_value, "total") {
	  $error_count{$k1}{$err_type}++;
	  $cumulative_cost{$k1}{$err_type} += $cost;
	  $normalizing_cost{$k1} += $norm_cost;
	}
      }
      $document_costs{$doc}{REF} += $norm_cost;
      $document_costs{$doc}{SYS} += $cost;
    }
  }
  return ({%error_count}, {%cumulative_cost}, {%normalizing_cost}, {%document_costs});
}

#################################

sub match_external_links { #return "correct" if any ref link matches any tst link

  my ($ref, $tst) = @_;

  if (@{$ref->{external_links}}) {
    return "miss" if not @{$tst->{external_links}};
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
  while ((my $id, my $element) = each %{$ref_database{$elements}}) {
    while ((my $doc, my $doc_ref) = each %{$element->{documents}}) {
      next unless my $doc_tst=$doc_ref->{MAP};
      my $ref_names = $doc_ref->{names};
      foreach my $ref_name (@$ref_names) {
	my $tst_name = $ref_name->{MAP};
	my $err_type = $tst_name ?
	  (extent_mismatch($ref_name->{locator}, $tst_name->{locator}) <= 1 ?
	   "correct" : "error") : "miss";
	$name_statistics{$element->{TYPE}}{$err_type}++;
      }
      my $tst_names = $doc_tst->{names};
      foreach my $tst_name (@$tst_names) {
	$name_statistics{$element->{TYPE}}{fa}++ unless $tst_name->{MAP};
      }
    }
  }
  return {%name_statistics};
}

#################################

sub mention_recognition_stats {

  my ($elements) = @_;

#accumulate statistics over all documents
  my (%detection_stats, %role_stats, %style_stats);
  while ((my $id, my $element) = each %{$ref_database{$elements}}) {
    while ((my $doc, my $doc_ref) = each %{$element->{documents}}) {
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
  while ((my $id, my $element) = each %{$ref_database{$elements}}) {
    while ((my $doc, my $doc_ref) = each %{$element->{documents}}) {
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
	  $role_stats{$ref_role}{$tst_role}++;
	}
      }
      while (my ($tst_role, $tst_ids) = each %{$tst->{arguments}}) {
	foreach my $tst_arg (values %$tst_ids) {
	  next if my $ref_arg = $tst_arg->{MAP};
	  $role_stats{"<undef>"}{$tst_role}++;
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
  while ((my $ref_role, my $ref_ids) = each %{$ref->{arguments}}) {
    $num_ref_args += keys %$ref_ids;
  }
  return $num_ref_args unless (defined $ref->{MAP} and
			       defined $argument_map{$ref->{ID}} and
			       defined $argument_map{$ref->{ID}}{$ref->{MAP}{ID}});

  my $num_mapped = my $num_correctly_mapped = 0;
  while ((my $tst_role, my $tst_ids) = each %{$argument_map{$ref->{ID}}{$ref->{MAP}{ID}}}) {
    while ((my $tst_id, my $ref_arg) = each %$tst_ids) {
      my $ref_map = $ref_database{mapped_refs}{$ref_arg->{ID}}{MAP};
      next unless $ref_map;
      $num_mapped++;
      $num_correctly_mapped++ if (($ref_map->{ID} eq $tst_id or
				   (defined $tst_database{refs}{$tst_id}{HOST_ELEMENT_ID} and
				    $ref_map->{ID} eq $tst_database{refs}{$tst_id}{HOST_ELEMENT_ID})) and
				  ($ref_arg->{ROLE} eq $tst_role or
				   ($relation_symmetry{$ref->{TYPE}} and $tst_role =~ /^Arg-[12]$/)))
    }
  }

  my $num_sys_args = 0;
  while ((my $sys_role, my $sys_ids) = each %{$ref->{MAP}{arguments}}) {
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
    my $ref_entity = $ref_entities->{$ref_id};
    if (my $tst_entity = $ref_entity->{MAP}) {
      my $tst_id = $tst_entity->{ID};
      my $err_type = "";
      foreach my $attr ("TYPE", "SUBTYPE", "CLASS", "LEVEL") {
	$err_type .= ($err_type ? "/" : "").$attr if $ref_entity->{$attr} ne $tst_entity->{$attr};
      }
      $err_type = " -- ENTITY $err_type MISMATCH" if $err_type;
      my $external_match = match_external_links ($ref_entity, $tst_entity);
      $err_type .= (defined $err_type ? ", " : " -- ")."EXTERNAL ID MISMATCH" if $external_match and $external_match ne "correct";
      $print_data ||= $opt_e if $err_type;
      $output .= ($err_type ? ">>> " : "    ")."ref entity ".id_plus_external_ids($ref_entity)." ".
	list_element_attributes($ref_entity, \@entity_attributes)."$err_type\n";
      $output .= ($err_type ? ">>> " : "    ")."tst entity ".id_plus_external_ids($tst_entity)." ".
	list_element_attributes($tst_entity, \@entity_attributes)."$err_type\n";
      $output .= sprintf ("      entity score:  %.5f out of %.5f\n",
			  $mapped_values{$ref_id}{$tst_id}, $ref_entity->{VALUE});
    } else {
      $print_data ||= $opt_e;
      $output .= ">>> ref entity ".id_plus_external_ids($ref_entity)." ".
	list_element_attributes($ref_entity, \@entity_attributes)." -- NO MATCHING TST ENTITY\n";
      $output .= sprintf "      entity score:  0.00000 out of %.5f\n", $ref_entity->{VALUE};
    }
    foreach my $doc (sort keys %{$ref_entity->{documents}}) {
      next unless defined $eval_docs{$doc};
      my $doc_ref = $ref_entity->{documents}{$doc};
      $output .= entity_mapping_details ($doc_ref, $doc_ref->{MAP}, $doc);
    }
    print $output, "--------\n" if $print_data;
  }

  return unless $opt_a or $opt_e;
  foreach my $tst_id (sort keys %$tst_entities) {
    my $tst_entity = $tst_entities->{$tst_id};
    next if $tst_entity->{MAP};
    print ">>> tst entity ".id_plus_external_ids($tst_entity)." ".
      list_element_attributes($tst_entity, \@entity_attributes)." -- NO MATCHING REF ENTITY\n";
    printf "      entity score:  %.5f out of 0.00000\n", $tst_entity->{FA_VALUE};
    foreach my $doc (sort keys %{$tst_entity->{documents}}) {
      next unless defined $eval_docs{$doc};
      my $doc_tst = $tst_entity->{documents}{$doc};
      next if $doc_tst->{MAP};
      print STDOUT entity_mapping_details (undef, $doc_tst, $doc);
    }
    print "--------\n";
  }
}

#################################

sub entity_mapping_details {

  my ($ref, $tst, $doc) = @_;

  (my $entity, my $max_value, my $value) =
    ($ref and $tst) ? ($ref, $ref->{VALUE}, $mapped_values{$ref->{ID}}{$tst->{ID}}) :
      ($ref ? ($ref, $ref->{VALUE}, 0) :
       ($tst, 0, $tst->{FA_VALUE}));
  my $output = sprintf "- in document $doc:  score:  %.5f out of %.5f\n", $value, $max_value;
  my (@data, @names);
  if ($ref) {
    foreach my $mention (@{$ref->{mentions}}) {
      push @data, {DATA=>$mention, TYPE=>"REF"};
    }
    foreach my $name (@{$ref->{names}}) {
      push @names, {DATA=>$name, TYPE=>"REF"};
    }
  }
  if ($tst) {
    foreach my $mention (@{$tst->{mentions}}) {
      push @data, {DATA=>$mention, TYPE=>"TST"};
    }
    foreach my $name (@{$tst->{names}}) {
      push @names, {DATA=>$name, TYPE=>"TST"};
    }
  }
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
	$output .= sprintf " (%3.3s/%3.3s/%3.3s) ID=$mention->{ID}",
        $mention->{TYPE}, $mention->{ROLE}, $mention->{STYLE};
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
	$entity_type_wgt{($type eq "REF") ? $ref->{TYPE} : $tst->{TYPE}} *
	$entity_class_wgt{($type eq "REF") ? $ref->{CLASS} : $tst->{CLASS}};
      $output .= sprintf (">>>   %s mention score: %.5f out of %.5f, \"%s\"", lc $type,
			  ($type eq "TST" ? ($fa_score, 0) : (0, $mention->{self_score})),
			  defined $mention->{head}{text} ? $mention->{head}{text} : "???");
      $output .= sprintf (" (%3.3s/%3.3s/%3.3s) ID=$mention->{ID} -- NO MATCHING %s MENTION\n",
			  $mention->{TYPE}, $mention->{ROLE}, $mention->{STYLE}, ($type eq "REF"?"TST":"REF"));
    }
  }
  foreach my $name (sort compare_locators @names) {
    my $type = $name->{TYPE};
    $name = $name->{DATA};
    next if $type eq "TST" and $name->{MAP};
    if ($name->{MAP}) {
      my $ref_name = $name;
      my $tst_name = $name->{MAP};
      if (extent_mismatch($ref_name->{locator}, $tst_name->{locator}) <= 1) {
	$output .= "         ref name=\"" . (defined $ref_name->{text} ? $ref_name->{text} : "???") . "\"\n";
      } else {
	$print_data ||= $opt_e;
	my $text = defined $ref_name->{text} ? $ref_name->{text} : "???";
	$output .= ">>>    ref name extent = \"$text\"\n";
	$text = defined $tst_name->{text} ? $tst_name->{text} : "???";
	$output .= ">>>    tst name extent = \"$text\" -- NAME EXTENT MISMATCH\n";
      }
    } else {
      $print_data ||= $opt_e;
      $output .= ">>>      ".(lc$type)." name=\"" . (defined $name->{text} ? $name->{text} : "???") . "\"";
      $output .= " -- NO MATCHING ".($type eq "REF"?"TST":"REF")." NAME\n";
    }
  }
  return $output;
}

#################################

sub compute_element_values {

  my ($type) = @_;

  my $refs = $ref_database{$type};
  my $tsts = $tst_database{$type};
  my $mention_scorer = $type =~ /^entities$/ ? \&entity_mention_score : \&quantity_mention_score;

#select putative REF-TST element pairs for mapping
  my (%doc_refs, %doc_tsts);
  while ((my $id, my $element) = each %$refs) {
    while ((my $doc, my $doc_element) = each %{$element->{documents}}) {
      push @{$doc_refs{$doc}}, $doc_element;
    }
  }
  while ((my $id, my $element) = each %$tsts) {
    while ((my $doc, my $doc_element) = each %{$element->{documents}}) {
      push @{$doc_tsts{$doc}}, $doc_element if defined $eval_docs{$doc};
    }
  }

#compute ref values
  foreach my $element (values %$refs) {
    while ((my $doc, my $doc_element) = each %{$element->{documents}}) {
      ($doc_element->{VALUE}) = element_document_value ($element, undef, $doc);
      $element->{VALUE} += $doc_element->{VALUE};
    }
  }

#compute mapped element values
  foreach my $doc (keys %doc_refs) {
    my @candidate_pairs = candidate_element_pairs ($doc_refs{$doc}, $doc_tsts{$doc}, $mention_scorer);
    foreach my $pair (@candidate_pairs) {
      my $ref_id = $pair->[0]{ID};
      my $tst_id = $pair->[1]{ID};
      (my $value, my $map) = element_document_value ($refs->{$ref_id}, $tsts->{$tst_id}, $doc);
      next unless defined $value;
      $mapped_document_values{$ref_id}{$tst_id}{$doc} = $value;
      $mapped_values{$ref_id}{$tst_id} += $value;
      $mention_map{$ref_id}{$tst_id}{$doc} = $map;
    }
  }

#compute tst values
  foreach my $element (values %$tsts) {
    while ((my $doc, my $doc_element) = each %{$element->{documents}}) {
      next unless defined $eval_docs{$doc};
      ($doc_element->{VALUE}) = element_document_value ($element, undef, $doc);
      $element->{VALUE} += $doc_element->{VALUE};
      ($doc_element->{FA_VALUE}) = element_document_value (undef, $element, $doc);
      $element->{FA_VALUE} += $doc_element->{FA_VALUE};
    }
  }

  check_for_duplicates ($tsts) if $opt_c;
}

#################################

sub candidate_element_pairs {

  my ($ref_doc_elements, $tst_doc_elements, $scorer) = @_;

  my @events;
  foreach my $element (@$ref_doc_elements) {
    foreach my $mention (@{$element->{mentions}}) {
      undef $mention->{tst_scores};
      my $locator = $mention->{head} ? $mention->{head}{locator} : $mention->{extent}{locator};
      push @events, {TYPE => "REF", EVENT => "BEG", MENTION => $mention, ELEMENT => $element, LOCATOR => $locator};
      push @events, {TYPE => "REF", EVENT => "END", MENTION => $mention, ELEMENT => $element, LOCATOR => $locator};
    }
  }

  foreach my $element (@$tst_doc_elements) {
    foreach my $mention (@{$element->{mentions}}) {
      undef $mention->{is_ref_mention};
      my $locator = $mention->{head} ? $mention->{head}{locator} : $mention->{extent}{locator};
      push @events, {TYPE => "TST", EVENT => "BEG", MENTION => $mention, ELEMENT => $element, LOCATOR => $locator};
      push @events, {TYPE => "TST", EVENT => "END", MENTION => $mention, ELEMENT => $element, LOCATOR => $locator};
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
      my $ref_element = $ref_event->{ELEMENT};
      my $tst_element = $tst_event->{ELEMENT};
      next if defined $overlapping_elements{$ref_element}{$tst_element};
      $overlapping_elements{$ref_element}{$tst_element} = 1;
      push @output_pairs, [$ref_element, $tst_element];
    }
  }
  return @output_pairs;
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
    while ((my $tst, my $score) = each %{$mapped_values{$ref}}) {
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
    "\n\nFATAL ERROR:  Cohort mapping through BGM FAILED\n";

  foreach $i (keys %$map) {
    $j = $map->{$i};
    $ref_cohorts->[$i]{MAP} = $tst_cohorts->[$j];
    $tst_cohorts->[$j]{MAP} = $ref_cohorts->[$i];
  }
}

#################################

sub map_entity_mentions {

  my ($ref_entity, $tst_entity) = @_;

  foreach my $doc (keys %{$ref_entity->{documents}}) {
    my $ref_occ = $ref_entity->{documents}{$doc};
    my $tst_occ = $tst_entity->{documents}{$doc};
    next unless $tst_occ;
    $ref_occ->{MAP} = $tst_occ;
    $tst_occ->{MAP} = $ref_occ;

    #map mentions	    
    my $map = $mention_map{$ref_occ->{ID}}{$tst_occ->{ID}}{$doc};
    my $ref_mentions = $ref_occ->{mentions};
    my $tst_mentions = $tst_occ->{mentions};
    foreach my $i (keys %$map) {
      my $j = $map->{$i};
      $ref_mentions->[$i]{MAP} = $tst_mentions->[$j];
      $tst_mentions->[$j]{MAP} = $ref_mentions->[$i];
    }
	
    #map names
    my ($ref_names, $tst_names, $ref_name, $tst_name, $overlap, $max_overlap);
    $ref_names = $ref_occ->{names};
    $tst_names = $ref_occ->{MAP}{names};
    foreach $ref_name (@$ref_names) {
      $max_overlap = $min_overlap;
      foreach $tst_name (@$tst_names) {
	$overlap = span_overlap($ref_name->{locator}, $tst_name->{locator});
	if ($overlap > $max_overlap) {
	  $max_overlap = $overlap;
	  $ref_name->{MAP} = $tst_name;
	}
      }
      $tst_name = $ref_name->{MAP};
      $tst_name->{MAP} = $ref_name if $tst_name;
    }
  }
}

#################################

sub element_document_value {
  
  my ($ref, $tst, $doc) = @_;

  my $compute_fa = not $ref; #calculate FA score if ref is null
  $ref = $tst if not $ref;
  $tst = $ref if not $tst;
  my $doc_ref = $ref->{documents}{$doc};
  my $doc_tst = $tst->{documents}{$doc};

# compute mentions value
  my $type = $ref->{ELEMENT_TYPE};
  my ($fa_wgt, $coref_fa_wgt) =
    ($type =~ /entity/ ? ($entity_fa_wgt, $entity_mention_coref_fa_wgt) :
     ($type =~ /quantity/ ? ($quantity_fa_wgt, $quantity_mention_coref_fa_wgt) :
      ($type =~ /timex2/ ? ($timex2_fa_wgt, $timex2_mention_coref_fa_wgt) :
       die "FATAL ERROR in element_document_value:  unknown element type '$type'")));
  my $mention_scorer = $type =~ /entity/ ? \&entity_mention_score : \&quantity_mention_score;
  $doc_ref->{element_value} = element_value ($doc_ref, $doc_ref)
    unless defined $doc_ref->{element_value};
  $doc_tst->{element_value} = element_value ($doc_tst, $doc_tst)
    unless defined $doc_tst->{element_value};
  my $element_value = element_value ($doc_ref, $doc_tst);
  my $norm = $element_value;
  my $fa_norm = $doc_tst->{element_value};
  if ($type =~ /entity/ and $level_weighting) {
    foreach my $doc_element ($doc_ref, $doc_tst) {
      if (not defined $doc_element->{mentions_value}) {
	my $mentions = $doc_element->{mentions};
	for (my $j=0; $j<@$mentions; $j++) {
	  $mentions->[$j]{self_score} = &$mention_scorer ($mentions->[$j], $mentions->[$j])
	    unless defined $mentions->[$j]{self_score};
	  $doc_element->{mentions_value} += $mentions->[$j]{self_score};
	}
      }
    }
    $norm /= $doc_ref->{mentions_value};
    $norm *= $entity_mention_type_wgt{$doc_ref->{LEVEL}};
    $fa_norm /= $doc_tst->{mentions_value};
    $fa_norm *= $entity_mention_type_wgt{$doc_tst->{LEVEL}};
  }
    
  my ($total_value, $mentions_map);
  my $tst_mentions = $doc_tst->{mentions};
  if ($doc_ref eq $doc_tst) { #compute self-score
    for (my $j=0; $j<@$tst_mentions; $j++) {
      $tst_mentions->[$j]{self_score} = &$mention_scorer ($tst_mentions->[$j], $tst_mentions->[$j])
	unless defined $tst_mentions->[$j]{self_score};
      $total_value += $tst_mentions->[$j]{self_score};
      $mentions_map->{$j} = $j;
    }
    $total_value *= -$fa_wgt if $compute_fa;
    return ($norm*$total_value, $mentions_map);
  }

  if (not $doc_tst->{fa_scores}) {
    foreach my $mention (@$tst_mentions) {
      $mention->{self_score} = &$mention_scorer ($mention, $mention)
	unless defined $mention->{self_score};
      my $fa_score = -$fa_wgt*$fa_norm*$mention->{self_score};
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
      $mapping_costs{$j}{$i} = $fa_scores->[$j] - $norm*$tst_score;
    }
  }
  return undef unless %mapping_costs;
  my $map = weighted_bipartite_graph_matching(\%mapping_costs) or die
    "\n\nFATAL ERROR:  Document level mention mapping through BGM FAILED\n";
  for (my $j=0; $j<@$tst_mentions; $j++) {
    $total_value += $fa_scores->[$j];
    next unless defined (my $i = $map->{$j});
    $mentions_map->{$i} = $j;
    $total_value -= $mapping_costs{$j}{$i};
  }
      
  $arg_document_values{$ref->{ID}}{$tst->{ID}}{$doc} = 
    $type !~ /entity/ ? $total_value :
    ($opt_V =~ /both/i ? $total_value :
     ($opt_V =~ /attributes/i ? $element_value*
      ($level_weighting ? $entity_mention_type_wgt{$doc_ref->{LEVEL}} : $doc_ref->{mentions_score}) :
      ($opt_V =~ /mentions/i ? $doc_ref->{element_value}*$total_value/$element_value :
       $doc_ref->{VALUE})));
  return ($total_value, $mentions_map);
}

#################################

sub element_value {

  my ($doc_ref, $doc_tst) = @_;

  my $element_value = $epsilon;
  my $type = $doc_ref->{ELEMENT_TYPE};
  if ($type =~ /timex2/) {
    while ((my $attribute, my $weight) = each %timex2_attribute_wgt) {
      next unless defined $doc_ref->{$attribute} and defined $doc_tst->{$attribute};
      $element_value += $weight * ($doc_ref->{$attribute} eq $doc_tst->{$attribute} ? 1 : $epsilon)
    }
  } else {
    if ($doc_ref eq $doc_tst) {
      $element_value += $type =~ /entity/ ?
	$entity_type_wgt{$doc_ref->{TYPE}} * $entity_class_wgt{$doc_ref->{CLASS}} :
	$quantity_type_wgt{$doc_ref->{TYPE}};
    } else {
      $element_value += $type =~ /entity/ ?
	min($entity_type_wgt{$doc_ref->{TYPE}},$entity_type_wgt{$doc_tst->{TYPE}})*
	min($entity_class_wgt{$doc_ref->{CLASS}},$entity_class_wgt{$doc_tst->{CLASS}}) :
	min($quantity_type_wgt{$doc_ref->{TYPE}},$quantity_type_wgt{$doc_tst->{TYPE}});
      while ((my $attribute, my $weight) = each %{$type =~ /entity/ ? \%entity_err_wgt : \%quantity_err_wgt}) {
	$element_value *= $weight if (($doc_ref->{$attribute} xor $doc_tst->{$attribute}) or
				      ($doc_ref->{$attribute} and $doc_ref->{$attribute} ne $doc_tst->{$attribute}));
      }
    }
  }
  return $element_value;
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
  while ((my $attribute, my $weight) = each %entity_mention_err_wgt) {
    $score *= $weight if $ref_mention->{$attribute} ne $tst_mention->{$attribute};
  }
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

  my $overlap;
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
    if ($x > $max) {
      $max = $x;
    }
  }
  return $max;
}

#################################

sub min {

  my $min = shift @_;
  foreach my $x (@_) {
    if ($x < $min) {
      $min = $x;
    }
  }
  return $min;
}

#################################

sub get_document_data {

  my ($db, $docs, $file) = @_;
  my $data;

  open (FILE, $file) or die "\nUnable to open ACE data file '$input_file'", $usage;
  while (<FILE>) {
    $data .= $_;
  }
  close (FILE);

  my $ndocs;
  my $input_objects = extract_sgml_structure ($data);
  my %get_objects = (entity=>\&get_entities, relation=>\&get_releves, event=>\&get_releves,
		     timex2=>\&get_timex2s, quantity=>\&get_quantities);
  my %attributes = (entity=>\@entity_attributes, relation=>\@relation_attributes, event=>\@event_attributes,
		    timex2=>\@timex2_attributes, quantity=>\@quantity_attributes);
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
      foreach my $type ("entity", "relation", "event", "timex2", "quantity") {
	my @elements = &{$get_objects{$type}} ($doc->{subelements}{$type}, $type);
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
	    $element->{$attribute} eq $db_element->{$attribute} or die
	      "\n\nFATAL INPUT ERROR:  attribute value conflict for attribute '$attribute'"
	      ." for $type '$element->{ID}' in document '$doc_id'\n"
	      ."    database value is '$db_element->{$attribute}'\n"
	      ."    document value is '$element->{$attribute}'\n";
	  }
	  promote_external_links ($db_element, $element);
	  if ($type eq "entity") {
	    $db_element->{LEVEL} = $element->{LEVEL} if not defined $db_element->{LEVEL} or
	      $entity_mention_type_wgt{$element->{LEVEL}} > $entity_mention_type_wgt{$db_element->{LEVEL}};
	  } elsif ($type =~ /^(event|relation)$/) {
	    $db_element->{arguments} = {} if not defined $db_element->{arguments};
	    while ((my $role, my $ids) = each %{$element->{arguments}}) {
	      while ((my $id, my $arg) = each %$ids) {
		if (my $db_arg = $db_element->{arguments}{$role}{$id}) {
		  $element->{arguments}{$role}{$id} = $db_arg;
		} else {
		  $db_element->{arguments}{$role}{$id} = $arg;
		}
	      }
	    }
	    delete $element->{arguments};
	    if ($type eq "relation") {
	      while ((my $role, my $ids) = each %{$db_element->{arguments}}) {
		(keys %$ids) == 1 or $role !~ /^Arg-[12]$/ or die
		  "\n\nFATAL INPUT ERROR:  argument conflict:  '$role' arg is multiply defined in relation '$element->{ID}'\n";
	      }
	      foreach my $role ("Arg-1", "Arg-2") {
		defined $db_element->{arguments}{$role} or die
		  "\n\nFATAL INPUT ERROR:  relation '$element->{ID}' does not have a '$role' argument, which is required\n";
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
      $new_element->{ID} = $new_doc_element->{ID} = $mention->{ID};
      $new_element->{ELEMENT_TYPE} = "mention_$type";
      $new_element->{mentions} = $new_doc_element->{mentions} = [$new_mention];
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
	my $new_name;
	my $max_overlap = $min_overlap;
	foreach my $name (@{$doc_element->{names}}) {
	  next unless defined $mention->{head};
	  my $overlap = span_overlap ($name->{locator}, $mention->{head}{locator});
	  next if $overlap < $max_overlap;
	  $new_name = $name;
	  $max_overlap = $overlap;
	}
	if ($new_name) {
	  $mention->{TYPE} eq "NAM" or warn
	    "\nWARNING:  NAME found for un-NAME mention $mention->{ID} (name = '$new_name->{text}')\n";
	} else {
	  $mention->{TYPE} ne "NAM" or $mention->{STYLE} eq "METONYMIC" or warn
	    "\nWARNING:  no NAME found for NAME mention $mention->{ID}\n";
	  $new_name = $mention->{head} if
	    $mention->{TYPE} eq "NAM" and $mention->{STYLE} eq "METONYMIC";
	}
	$new_doc_element->{names} = $new_name ? [{%$new_name}] : [];
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

  my ($releves, $type) = @_;
  (my $attributes, my $arg_roles, my $get_attr) = $type eq "relation" ?
    (\%relation_attributes, \%relation_argument_roles, \&get_attribute) :
    (\%event_attributes, \%event_argument_roles, \&demand_attribute);

  my (@releves, %ids);
  foreach my $releve (@$releves) {
    my %releve;
    $releve{ELEMENT_TYPE} = $type;

#get relation/event ID
    $releve{ID} = $input_element = demand_attribute ($type, "ID", $releve->{attributes});
    $fatal_input_error_header =
      "\n\nFATAL INPUT ERROR for $type '$input_element'\n	in document '$input_doc'\n	in file '$input_file'\n";
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
    my @mentions = get_releve_mentions ($releve, $type, $arg_roles) or $releve{TYPE} eq "METONYMY" or
      die $fatal_input_error_header."    no mentions found\n";
    $releve{mentions} = [@mentions];
    $releve{external_links} = [get_external_links ($releve)];
    push @releves, {%releve};
  }
  return @releves;
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

  my ($releve, $type, $argument_roles) = @_;
    
  my @mentions;
  foreach my $mention (@{$releve->{subelements}{$type."_mention"}}) {
    my %mention;
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
    printf "ID=$releve->{ID}, VALUE=%.5f, TYPE=%s, SUBTYPE=%s",
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
  (my $tag, $data) = (($data=longest_string($ref, "name")) ? ("name", $data) :
		      (($data=longest_string($ref, "mention", "head")) ? ("head", $data) :
		       (($data=longest_string($ref, "mention", "extent")) ? ("extent", $data) : "")));
  $out .= ", $tag=\"$data\"" if $data;
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

  while ((my $doc, my $doc_element) = each %{$element->{documents}}) {
    print "      -- in document $doc\n";
    foreach my $mention (@{$doc_element->{mentions}}) {
      printf "         mention ID=$mention->{ID}";
      print ", anchor=\"$mention->{anchor}{text}\"" if $mention->{anchor};
      print $mention->{extent}{text} ? ", extent=\"$mention->{extent}{text}\"\n" : "\n";
      foreach my $role (sort keys %{$mention->{arguments}}) {
	foreach my $id (sort keys %{$mention->{arguments}{$role}}) {
	  my $ref = $ref_refs->{$mention->{arguments}{$role}{$id}{ID}};
	  printf "%21s: ID=$id%s", $role, ($ref->{head} ? ", head=\"$ref->{head}{text}\"\n" :
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
    while ((my $ref_id, my $ref) = each %$refs) {
      $doc_refs{$ref_id} = 1 if defined $ref->{documents}{$doc};
    }
    next unless %doc_refs;
    while ((my $tst_id, my $tst) = each %$tsts) {
      next unless defined $tst->{documents}{$doc};
      foreach my $ref_id (keys %doc_refs) {
	$putative_pairs{$ref_id}{$tst_id} = $tst;
      }
    }
  }

#compute mapped element values
  while ((my $ref_id, my $putative_tsts) = each %putative_pairs) {
    my $ref = $refs->{$ref_id};
    while ((my $tst_id, my $tst) = each %$putative_tsts) {
      my $arg_map = compute_argument_map ($ref, $tst);
      $argument_map{$ref_id}{$tst_id} = $arg_map if $arg_map;
      my $value;
      foreach my $doc (keys %{$ref->{documents}}) {
	(my $value, my $map) = releve_document_value ($ref, $tst, $doc, $arg_map);
	next unless defined $value;
	$mapped_document_values{$ref_id}{$tst_id}{$doc} = $value;
	$mapped_values{$ref_id}{$tst_id} += $value;
	$mention_map{$ref_id}{$tst_id}{$doc} = $map;
      }
    }
  }

#compute self-values
  foreach my $elements ($refs, $tsts) {
    foreach my $element (values %$elements) {
      while ((my $doc, my $doc_element) = each %{$element->{documents}}) {
	next unless defined $eval_docs{$doc};
	($doc_element->{VALUE}, $doc_element->{FA_VALUE}) = $elements eq $refs ?
	  releve_document_value ($element, undef, $doc) :
	  releve_document_value (undef, $element, $doc);
	$element->{VALUE} += $doc_element->{VALUE};
	$element->{FA_VALUE} += $doc_element->{FA_VALUE};
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
  while ((my $tst_role, my $tst_ids) = each %$arg_map) {
    while ((my $tst_id, my $ref_map) = each %$tst_ids) {
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
    while ((my $tst_role, my $tst_ids) = each %$arg_map) {
      while ((my $tst_id, my $ref_map) = each %$tst_ids) {
	(my $ref_arg, my $tst_arg) = ($ref->{arguments}{$ref_map->{ROLE}}{$ref_map->{ID}},
				      $tst->{arguments}{$tst_role}{$tst_id});
	next unless $ref_arg and $tst_arg;
	$ref_arg->{MAP} = $tst_arg;
	$tst_arg->{MAP} = $ref_arg;
      }
    }
    my $map = $mention_map{$ref->{ID}}{$tst->{ID}}{$doc};
    next unless $map;
    while ((my $i, my $j) = each %$map) {
      $doc_ref->{mentions}[$i]{MAP} = $doc_tst->{mentions}[$j];
      $doc_tst->{mentions}[$j]{MAP} = $doc_ref->{mentions}[$i];
    }
  }
}

#################################

sub releve_document_value {

  my ($ref, $tst, $doc, $arg_map) = @_;

  my $arg_db;
  ($ref, $arg_db) = ($tst, $tst_database{refs}) if not $ref;
  ($tst, $arg_db) = ($ref, $ref_database{refs}) if not $tst;
  my $ref_mentions = $ref->{documents}{$doc}{mentions};
  my $tst_mentions = $tst->{documents}{$doc}{mentions};
  $ref_mentions = [] unless $ref_mentions;
  $tst_mentions = [] unless $tst_mentions;
  my $relation_type = $ref->{ELEMENT_TYPE} =~ /relation/;
  my $mention_type = $ref->{ELEMENT_TYPE} =~ /mention/;
  (my $type_wgt, my $modality_wgt, my $fa_wgt, my $role_err_wgt, my $asymmetry_wgt) =
    $relation_type ? (\%relation_type_wgt, \%relation_modality_wgt, $relation_fa_wgt,
		      $relation_argument_role_err_wgt, $relation_asymmetry_err_wgt) :
		      (\%event_type_wgt, \%event_modality_wgt, $event_fa_wgt,
		       $event_argument_role_err_wgt, 1);
  if ($arg_db) {
    my $arg_score;
    while ((my $role, my $ids) = each %{$ref->{arguments}}) {
      foreach my $id (keys %$ids) {
	$arg_score += $arg_db->{$id}{documents}{$doc}{VALUE};
      }
    }
    $arg_score += $epsilon * @$ref_mentions if $ref_mentions;
    my $value = releve_value($ref,$ref) * $arg_score;
    return ($value, -$fa_wgt*$value);
  }
  my $nargs_ref = 0;
  while ((my $role, my $ids) = each %{$ref->{arguments}}) {
    $nargs_ref += keys %$ids;
  }
  return undef if (($argscore_required and $nargs_ref and not $arg_map) or
		   ($relation_type and
		    (($nargs_required eq "2" and (not $arg_map->{"Arg-1"} or not $arg_map->{"Arg-2"})) or
		     ($nargs_required eq "1" and (not $arg_map->{"Arg-1"} and not $arg_map->{"Arg-2"})))));
  (my $mentions_overlap, my $mentions_map) = optimum_mentions_mapping ($ref_mentions, $tst_mentions);
  return undef unless $arg_map or $mentions_overlap;
  return undef if $mention_overlap_required and not $mentions_overlap;
  return undef if $mention_type and not ($relation_type ?
					 relation_mention_args_overlap ($ref_mentions, $tst_mentions) :
					 span_overlap ($ref_mentions->[0]{extent}{locator},
						       $tst_mentions->[0]{extent}{locator}));
  my $args_value = my $faargs_value = my $nargs_mapped = 0;
  while ((my $tst_role, my $tst_ids) = each %{$tst->{arguments}}) {
    foreach my $tst_id (keys %$tst_ids) {
      $faargs_value += $tst_database{refs}{$tst_id}{documents}{$doc}{VALUE};
      next unless $arg_map and $arg_map->{$tst_role} and my $arg = $arg_map->{$tst_role}{$tst_id};
      (my $ref_id, my $ref_role) = ($arg->{ID}, $arg->{ROLE});
      my $values = $arg_document_values{$ref_id}{$tst_id} if $ref_id and $arg_document_values{$ref_id};
      my $map_score = $values->{$doc} if $values and defined $values->{$doc} and
	($allow_wrong_mapping or ($ref_database{refs}{$ref_id}{MAP} and
				  $ref_database{refs}{$ref_id}{MAP} eq $tst_database{refs}{$tst_id}));
      next unless $map_score;
      $map_score = $ref_database{refs}{$ref_id}{documents}{$doc}{VALUE} if $mention_type;
      $map_score *= (not $relation_type) ? $role_err_wgt :
	($relation_symmetry{$ref->{TYPE}} ? 1 : ($ref_role =~ /^Arg-[12]$/ ? $asymmetry_wgt : $role_err_wgt))
	if $ref_role ne $tst_role;
      $args_value += $map_score;
      $nargs_mapped++;
      $faargs_value -= $tst_database{refs}{$tst_id}{documents}{$doc}{VALUE};
    }
  }
  return undef if $all_args_required and $nargs_mapped != $nargs_ref;
  return undef unless $args_value or $mentions_overlap;
  $args_value += $mentions_overlap * $epsilon if $mentions_overlap;
  return (releve_value($ref,$tst)*$args_value - $fa_wgt*releve_value($tst,$tst)*$faargs_value,
	  $mentions_map);
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
  while ((my $attribute, my $weight) = each %$err_wgt) {
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
  while ((my $role, my $ids) = each %{$ref->{arguments}}) {
    foreach my $id (keys %$ids) {
      push @ref_args, {ID => $id, ROLE => $role};
    }
  }
  return undef unless @ref_args;
  while ((my $role, my $ids) = each %{$tst->{arguments}}) {
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
    "\n\nFATAL ERROR:  argument mapping through BGM FAILED\n";

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

  my $id;
  my $ref_min = my $tst_min = 99999999999999;
  my $ref_max = my $tst_max = -99999999999999;
  foreach my $role ("Arg-1", "Arg-2") {
    ($id) = keys %{$ref_mentions->[0]{arguments}{$role}};
    my $locator = $ref_database{refs}{$id}{head}{locator};
    $locator->{data_type} ne "image" or die
      "\n\nFATAL ERROR in relation_mention_overlap - attempted use of image data not accommodated\n";
    $ref_min = min($ref_min, $locator->{start});
    $ref_max = max($ref_max, $locator->{end});
    ($id) = keys %{$tst_mentions->[0]{arguments}{$role}};
    $locator = $tst_database{refs}{$id}{head}{locator};
    $locator->{data_type} ne "image" or die
      "\n\nFATAL ERROR in relation_mention_overlap - attempted use of image data not accommodated\n";
    $tst_min = min($tst_min, $locator->{start});
    $tst_max = max($tst_max, $locator->{end});
  }
  my $overlap = min($ref_max,$tst_max) - max($ref_min,$tst_min);
  return $overlap >= 0 ? 1 : 0;
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
    my $arg_out = print_argument_mapping ($ref, $tst);
    $output .= $arg_out if $arg_out;
    foreach my $doc (sort keys %{$ref->{documents}}) {
      $output .= releve_mention_mapping ($ref, $tst, $doc) if $eval_docs{$doc};
    }
    print $output, "--------\n" if $print_data;
  }

#print unmapped test elements
  return unless $opt_a or $opt_e;
  foreach my $tst_id (sort keys %$tst_data) {
    my $tst = $tst_data->{$tst_id};
    next if $tst->{MAP};
    printf ">>> tst $type $tst->{ID} ".
      list_element_attributes($tst, $attributes)." -- NO MATCHING REF\n";
    printf "      $type score:  %.5f out of 0.00000\n", $tst->{FA_VALUE};
    my $arg_out = print_argument_mapping (undef, $tst);
    print STDOUT $arg_out if $arg_out;
    foreach my $doc (sort keys %{$tst->{documents}}) {
      print STDOUT releve_mention_mapping (undef, $tst, $doc) if $eval_docs{$doc};
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

  my ($ref, $tst) = @_;

  my %args;
  if ($ref and $ref->{arguments}) {
    while ((my $role, my $ids) = each %{$ref->{arguments}}) {
      foreach my $arg (values %$ids) {
	$args{$arg}{REF} = $arg;
      }
    }
  }
  if ($tst and $tst->{arguments}) {
    my $reversed_order;
    while ((my $role, my $ids) = each %{$tst->{arguments}}) {
      foreach my $arg (values %$ids) {
	$args{$arg->{MAP}?$arg->{MAP}:$arg}{TST} = {%$arg};
	$reversed_order = 1 if ($tst->{ELEMENT_TYPE} =~ /relation/ and
				$arg->{MAP} and
				$arg->{MAP}{ROLE} ne $role);
      }
    }
    if ($reversed_order and $relation_symmetry{$ref->{TYPE}}) {
      foreach my $arg (values %args) {
	next unless $arg->{TST};
	my $role = $arg->{TST}{ROLE};
	$arg->{TST}{ROLE} = ($role eq "Arg-1" ? "Arg-2" :
			     ($role eq "Arg-2" ? "Arg-1" : $role));
      }
    }
  }
  my @args = sort {my $cmp;
		   return $cmp if $cmp = ($a->{REF}?$a->{REF}{ROLE}:$a->{TST}{ROLE}) cmp ($b->{REF}?$b->{REF}{ROLE}:$b->{TST}{ROLE});
		   return $cmp if $cmp = (defined $b->{REF} and defined $b->{TST}) cmp (defined $a->{REF} and defined $a->{TST});
		   return $cmp if $cmp = defined $b->{REF} cmp defined $a->{REF};
		   return ($a->{REF}?$a->{REF}{ID}:$a->{TST}{ID}) cmp ($b->{REF}?$b->{REF}{ID}:$b->{TST}{ID});
		 } values %args;
  my $output;
  foreach my $arg (@args) {
    my $err_text;
    my $ref_arg = $arg->{REF};
    my $tst_arg = $arg->{TST};
    if ($ref_arg) {
      $err_text = $tst_arg ? "" : "NO CORRESPONDING TST ARGUMENT";
      if ($ref->{ELEMENT_TYPE}!~/mention/) {
	my $a_ref = $ref_database{refs}{$ref_arg->{ID}};
	$err_text .= ($err_text ? ", " : "")."REF ARGUMENT NOT MAPPED" if not $a_ref->{MAP};
	$err_text .= ($err_text ? ", " : "")."REF ARGUMENT MISMAPPED" if ($a_ref->{MAP} and $tst_arg and
									  $a_ref->{MAP}{ID} ne $tst_arg->{ID});
      }
      $err_text .= ($err_text ? ", " : "")."ARGUMENT ROLE MISMATCH" if ($tst_arg and $ref_arg->{ROLE} ne $tst_arg->{ROLE} and
									($ref_arg->{ROLE} !~ /^Arg-[12]$/ or not $relation_symmetry{$ref->{TYPE}}));
      $print_data ||= $err_text;
      undef $err_text unless $tst;
      $output .= ($err_text ? ">>>   ":"      ").mapped_argument_description ("ref", $ref_arg, $tst_arg, $ref, $tst, $err_text);
    }
    if ($tst_arg) {
      $err_text = $ref_arg ? "" : "NO CORRESPONDING REF ARGUMENT";
      if ($tst->{ELEMENT_TYPE}!~/mention/) {
	my $a_ref = $tst_database{refs}{$tst_arg->{ID}};
	$err_text .= ($err_text ? ", " : "")."TST ARGUMENT NOT MAPPED" if not $a_ref->{MAP};
	$err_text .= ($err_text ? ", " : "")."TST ARGUMENT MISMAPPED" if ($a_ref->{MAP} and $ref_arg and
									  $a_ref->{MAP}{ID} ne $ref_arg->{ID});
      }
      $err_text .= ($err_text ? ", " : "")."ARGUMENT ROLE MISMATCH" if ($ref_arg and $tst_arg->{ROLE} ne $ref_arg->{ROLE} and
									($tst_arg->{ROLE} !~ /^Arg-[12]$/ or not $relation_symmetry{$ref->{TYPE}}));
      $print_data ||= $err_text;
      undef $err_text unless $ref;
      $output .= ($err_text ? ">>>   ":"      ").mapped_argument_description ("tst", $ref_arg, $tst_arg, $ref, $tst, $err_text);
    }
  }
  return $output;
}

#################################

sub mapped_argument_description {

  my ($src, $refarg, $tstarg, $ref, $tst, $text) = @_;

  my $refarg_id = $refarg->{ID} if $refarg;
  my $tstarg_id = $tstarg->{ID} if $tstarg;
  (my $arg, my $role, my $id) = $src eq "ref" ?
    ($ref_database{refs}{$refarg_id}, $refarg->{ROLE}, $refarg_id) :
    ($tst_database{refs}{$tstarg_id}, $tstarg->{ROLE}, $tstarg_id);
  my $role_err_wgt = (($ref and $ref->{ELEMENT_TYPE} =~ /relation/) or
		      ($tst and $tst->{ELEMENT_TYPE} =~ /relation/)) ?
		      $relation_argument_role_err_wgt : $event_argument_role_err_wgt;
  my $out = sprintf ("%11.11s $src arg", $role);
  if ($src eq "ref" and $refarg and $tstarg) {
    $out .= ":" . " "x31;
  } else {
    my %docs;
    foreach my $doc (keys %{$ref->{documents}}, keys %{$tst->{documents}}) {
      $docs{$doc} = 1;
    }
    my $cumscore = my $refscore = my $fa_score = 0;
    foreach my $doc (keys %docs) {
      if ($refarg and $tstarg) {
	$cumscore += $arg_document_values{$refarg_id}{$tstarg_id}{$doc} if
	  $arg_document_values{$refarg_id} and $arg_document_values{$refarg_id}{$tstarg_id};
	$refscore += $ref_database{refs}{$refarg_id}{documents}{$doc}{VALUE};
      } elsif ($refarg) {
	$refscore += $ref_database{refs}{$refarg_id}{documents}{$doc}{VALUE};
      } else {
	$fa_score += $tst_database{refs}{$tstarg_id}{documents}{$doc}{VALUE};
      }
    }
    $cumscore *= $role_err_wgt if $refarg and $tstarg and $tstarg->{ROLE} ne $refarg->{ROLE};
    $out .= sprintf (" score: %.5f out of %.5f, ", $cumscore - $fa_score, $refscore);
  }
  $out .= sprintf ("ID=$id (%3.3s/%3.3s", $arg->{TYPE},
		   $arg->{SUBTYPE} ? (substr $arg->{SUBTYPE}, 0, 7) : "---");
  $out .= $arg->{ELEMENT_TYPE} =~ /entity/ ? sprintf ("/%3.3s/%3.3s)", $arg->{LEVEL}, $arg->{CLASS}) : ")";
  my $data;
  (my $tag, $data) = (($data=longest_string($arg, "name")) ? ("name", $data) :
		      (($data=longest_string($arg, "mention", "head")) ? ("head", $data) :
		       (($data=longest_string($arg, "mention", "extent")) ? ("extent", $data) : "")));
  $out .= ", $tag=\"$data\"" if $data;
  $out .= $text ? " -- $text\n" : "\n";
  return $out;
}

#################################

sub releve_mention_mapping {

  my ($ref, $tst, $doc) = @_;

  my $doc_ref = $ref ? $ref->{documents}{$doc} : undef;
  my $doc_tst = $tst ? $tst->{documents}{$doc} : undef;
  my $ref_value = $doc_ref ? $doc_ref->{VALUE} : 0;
  my $tst_value = ($doc_ref ? ($doc_tst ? (defined $mapped_document_values{$ref->{ID}}{$tst->{ID}}{$doc} ? 
					   $mapped_document_values{$ref->{ID}}{$tst->{ID}}{$doc}
					   : 0)
			       : 0)
		   : $doc_tst->{FA_VALUE});
  my $output = sprintf "- in document $doc:  score:  %.5f out of %.5f\n", $tst_value, $ref_value;
			      
  my @data;
  if ($ref) {
    foreach my $mention (@{$ref->{documents}{$doc}{mentions}}) {
      push @data, {DATA=>$mention, TYPE=>"ref"};
    }
  }
  if ($tst) {
    foreach my $mention (@{$tst->{documents}{$doc}{mentions}}) {
      push @data, {DATA=>$mention, TYPE=>"tst"};
    }
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
  return $output;
}

#################################

sub get_timex2s { #extract information for document-level timex2s

  my ($timex2s) = @_;

  my (@timex2s, %ids);
  foreach my $timex2 (@$timex2s) {
    my %timex2;
    $timex2{ELEMENT_TYPE} = "timex2";

#get timex2 ID
    $timex2{ID} = $input_element = demand_attribute ("TIMEX2", "ID", $timex2->{attributes});
    $fatal_input_error_header =
      "\n\nFATAL INPUT ERROR for timex2 '$input_element'\n	in document '$input_doc'\n	in file '$input_file'\n";
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

    my @mentions = get_timex2_mentions ($timex2) or die
      $fatal_input_error_header."    timex2 data contains no mentions\n";
    $timex2{mentions} = [@mentions];
    $timex2{external_links} = [get_external_links ($timex2)];
    push @timex2s, {%timex2};
  }
  return @timex2s;
}

#################################

sub get_timex2_mentions {

  my ($timex2) = @_;

  my @mentions;
  foreach my $mention (@{$timex2->{subelements}{timex2_mention}}) {
    my %mention;
    $mention{host_id} = $timex2->{attributes}{ID};
    $mention{ID} = demand_attribute ("TIMEX2_mention", "ID", $mention->{attributes});
    ($mention{extent}) = get_locators ("extent", $mention) or die
      $fatal_input_error_header."    no mention extent found\n";
    push @mentions, {%mention};
  }
  return @mentions;
}

#################################

sub get_quantities { #extract information for document-level quantities

  my ($quantities) = @_;

  my (@quantities, %ids);
  foreach my $quantity (@$quantities) {
    my %quantity;
    $quantity{ELEMENT_TYPE} = "quantity";

#get quantity ID
    $quantity{ID} = $input_element = demand_attribute ("value", "ID", $quantity->{attributes});
    $fatal_input_error_header =
      "\n\nFATAL INPUT ERROR for quantity '$input_element'\n	in document '$input_doc'\n	in file '$input_file'\n";
    not defined $ids{$input_element} or die $fatal_input_error_header.
      "    multiple quantity definitions (only one definition allowed per document)\n";
    $ids{$input_element} = 1;

#get quantity attributes
    $quantity{TYPE} = demand_attribute ("value", "TYPE", $quantity->{attributes}, $quantity_attributes{TYPE});
    $quantity{SUBTYPE} = demand_attribute ("value", "SUBTYPE", $quantity->{attributes}, $quantity_attributes{TYPE}{$quantity{TYPE}});
    foreach my $attribute (@quantity_attributes) {
      next if $attribute =~ /^(ID|TYPE|SUBTYPE)$/;
      my $value = get_attribute ("VALUE", $attribute, $quantity->{attributes});
      $quantity{$attribute} = $value if defined $value;
    }

    my @mentions = get_quantity_mentions ($quantity) or die
      $fatal_input_error_header."    quantity contains no mentions\n";
    $quantity{mentions} = [@mentions];
    $quantity{external_links} = [get_external_links ($quantity)];
    push @quantities, {%quantity};
  }
  return @quantities;
}

#################################

sub get_quantity_mentions {

  my ($quantity) = @_;

  my @mentions;
  foreach my $mention (@{$quantity->{subelements}{quantity_mention}}) {
    my %mention;
    $mention{host_id} = $quantity->{attributes}{ID};
    $mention{ID} = demand_attribute ("quantity_mention", "ID", $mention->{attributes});
    ($mention{extent}) = get_locators ("extent", $mention);
    $mention{extent} or die $fatal_input_error_header.
      "    no mention extent found\n";
    push @mentions, {%mention};
  }
  return @mentions;
}

#################################

sub quantity_mention_score {

  my ($ref_mention, $tst_mention) = @_;

  return undef unless ($ref_mention->{extent} and
		       $tst_mention->{extent} and
		       (my $overlap = span_overlap($ref_mention->{extent}{locator},
						   $tst_mention->{extent}{locator})) >= $min_overlap);
  return 1 + $overlap*$epsilon;;
}
      
#################################

sub print_quantities {

  my ($source, $type, $quantities, $attributes) = @_;

  return unless $opt_s;
  print "\n======== $source $type ========\n";
  foreach my $id (sort keys %$quantities) {
    my $quantity = $quantities->{$id};
    printf "ID=$quantity->{ID}, VALUE=%.5f, TYPE=$quantity->{TYPE}", $quantity->{VALUE};
    foreach my $attribute (@$attributes) {
      print ", $attribute=$quantity->{$attribute}" unless
	$attribute =~ /^(ID|TYPE)$/ or not $quantity->{$attribute};
    }
    print "\n";
    foreach my $doc (sort keys %{$quantity->{documents}}) {
      my $doc_info = $quantity->{documents}{$doc};
      print "    -- in document $doc\n";
      foreach my $mention (sort compare_locators @{$doc_info->{mentions}}) {
	printf "      mention extent=\"%s\"\n", $mention->{extent}{text} ? $mention->{extent}{text} : "???";
      }
    }
  }
}

#################################

sub print_quantity_mapping {

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
    foreach $doc (keys %{$ref->{documents}}) {
      next unless defined $eval_docs{$doc};
      $ref_occ = $ref->{documents}{$doc};
      $output .= quantity_mapping_details ($ref_occ, $ref_occ->{MAP}, $doc);
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
      next unless defined $eval_docs{$doc};
      $tst_occ = $tst->{documents}{$doc};
      print STDOUT quantity_mapping_details (undef, $tst_occ, $doc);
    }
    print "--------\n";
  }
}

#################################

sub quantity_mapping_details {

  my ($ref, $tst, $doc) = @_;
  my ($type, $ref_mention, $tst_mention, $mention, @mentions);

  my $output = "- in document $doc:\n";
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

  my ($entities) = @_;

  my (@entities, %ids);
  foreach my $entity (@$entities) {
    my %entity;
    $entity{ELEMENT_TYPE} = "entity";

#get entity ID
    $entity{ID} = $input_element = demand_attribute ("entity", "ID", $entity->{attributes});
    $fatal_input_error_header =
      "\n\nFATAL INPUT ERROR for entity '$input_element'\n	in document '$input_doc'\n	in file '$input_file'\n";
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

    my @mentions = get_entity_mentions ($entity) or die
      $fatal_input_error_header."    entity contains no mentions\n";
    $entity{mentions} = [@mentions];
    $entity{names} = [get_entity_names ($entity)];
    $entity{external_links} = [get_external_links ($entity)];
    $entity{LEVEL} = doc_entity_level (\%entity);
    push @entities, {%entity};
  }
  return @entities;
}

#################################

sub get_entity_mentions {

  my ($entity) = @_;

  my @mentions;
  foreach my $mention (@{$entity->{subelements}{entity_mention}}) {
    my %mention;
    my $attributes = $mention->{attributes};
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

sub doc_entity_level {

  my ($entity) = @_;
  my $level;
  my $weight = -9E9;
  foreach my $mention (@{$entity->{mentions}}) {
    my $cmp = $entity_mention_type_wgt{$mention->{TYPE}} <=> $weight;
    next if $cmp < 0;
    if ($cmp) {
      $weight = $entity_mention_type_wgt{$mention->{TYPE}};
      $level = $mention->{TYPE};
    } elsif ($level eq "PRO") {
      $level = $mention->{TYPE};
    } elsif ($mention->{TYPE} eq "NAM") {
      $level = "NAM";
    }
  }
  return $level;
}

#################################

sub longest_string {

  my ($element, $kind, $type) = @_;

# $kind is either "mention" or "name"
# $type is either "head" or "extent" (for $kind eq "mention") or undef (for $kind eq "name")

  my $longest_string="";

  my $kinds = $kind."s";
  if (defined $element->{$kinds}) {
    foreach my $kind (@{$element->{$kinds}}) {
      my $text = ($type and $kind->{$type}) ? $kind->{$type}{text} : $kind->{text};
      $longest_string = $text if defined $text and length($text) > length($longest_string);
    }
  }

  if (defined $element->{documents}) {
    foreach my $doc (keys %{$element->{documents}}) {
      if (defined $element->{documents}{$doc}{$kinds}) {
	foreach my $kind (@{$element->{documents}{$doc}{$kinds}}) {
	  my $text = ($type and $kind->{$type}) ? $kind->{$type}{text} : $kind->{text};
	  $longest_string = $text if defined $text and length($text) > length($longest_string);
	}
      }
    }
  }
  
  return $longest_string;
}

#################################
    
sub get_entity_names {

  my ($entity) = @_;
    
  my @names;
  foreach my $attr (@{$entity->{subelements}{entity_attributes}}) {
    push @names, get_locators ("name", $attr);
  }
  foreach my $name (@names) {
    if (my $text = $name->{name}) {
      $text =~ s/\r\n/\n/sg;
      $text =~ s/-\n//sg;
      $text =~ s/\n/ /sg;
      $text =~ s/\s+/ /sg;
      $name->{text} = limit_string $text if $text;
    }
  }
  return @names;
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

  while ((my $resource, my $id) = each %new_links) {
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
      $text =~ s/\r\n/\n/sg;
      $text =~ s/-\n//sg;
      $text =~ s/\n/ /sg;
      $text =~ s/\s+/ /sg;
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

  my $objects;
  while ($data and $data =~ /<([a-zA-Z_][a-zA-Z0-9_+-]*)[>\s]/) {
    my $name = $1;
    (my $tag, my $span, $data) = 
      ($data =~ /<$name\s*(((\s([^>]*?[^\/]))?>(.*?)<\/$name\s*>(.*))|((\s([^>]*?))?\/>(.*)))/si) ?
      (($4 or $5 or $6) ? ($4, $5, $6) : ($9, undef, $10)) : ();
    $name =~ s/value/quantity/; #substitute ace-eval name for apf DTD name
    my $subelements = extract_sgml_structure ($span);
    my $attributes = {};
    push @{$objects->{$name}}, {attributes=>$attributes, subelements=>$subelements, span=>$span};
    next unless $tag;
    $attributes->{uc$1} = $2 while $tag =~ s/\s*([^\s]+)\s*=\s*\"\s*([^\"]*?)\s*\"//si;
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
    foreach my $name (@{$entity->{names}}) {
      print "  name=\"$name->{name}\"\n";
    }
    foreach my $title (@{$entity->{titles}}) {
      print "  title=\"$title->{title}\"\n";
    }
    print_entity_mentions ($entity);
  }
}

#################################

sub print_entity_mentions {

  my ($entity) = @_;

  foreach my $doc (sort keys %{$entity->{documents}}) {
    my $doc_entity = $entity->{documents}{$doc};
    printf "    -- in document $doc VALUE=%.5f\n", $doc_entity->{VALUE};
    foreach my $mention (sort compare_locators @{$doc_entity->{mentions}}) {
      print "      mention TYPE=$mention->{TYPE}, ROLE=$mention->{ROLE}, STYLE=$mention->{STYLE}, ";
      printf "head=\"%s\", ", defined $mention->{head}{text} ? $mention->{head}{text} : "???";
      printf "extent=\"%s\"\n", defined $mention->{extent}{text} ? $mention->{extent}{text} : "???";
    }
    foreach my $name (sort compare_locators @{$doc_entity->{names}}) {
      printf "      name extent=\"%s\"\n", defined $name->{text} ? $name->{text} : "???";
    }
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
