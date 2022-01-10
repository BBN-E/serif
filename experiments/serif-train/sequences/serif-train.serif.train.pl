eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}' && eval 'exec perl -w -S $0 $argv:q'
  if 0;

# Copyright 2009 by BBN Technologies Corp.     All Rights Reserved.

use File::Path;
use File::Basename;
use runjobs3000;

$exp = 'serif-train';
$BBN_CONTROL = 'PLEASE-EDIT';    
$OTHER_QUEUE = 'ears'; 
$QUEUE_PRIO = '5';                  
                             
require "$exp.serif.train.defines.pl";

set_language_specific_variables($SERIF_LANG);


startjobs(); 

my @preprocessing_jobs = ();
my @training_jobs = ();
my %local_params = ();


##############################
# Check training consistency
@sgm_files = glob("$sgmdir/*$sgmsuffix");
$num_files = scalar(@sgm_files);
@apf_files = glob("$apfdir/*$apfsuffix");
$num_apf_files = scalar(@apf_files);

if($runjobs3000::major_mode == runjobs3000::MM_RUN) {
    die "No sgm files found." if $num_files == 0;
    die "No apf files found." if $num_apf_files == 0;
    die "Different number of sgm files than apf files." 
        if $num_files != $num_apf_files;
    
    unless (-f $dtd || length($dtd) == 0) {
        die "Could not find DTD: $dtd";   
    }
}

# Make sure that $BBN_CONTROL was set up properly
if ( ! -d "$BBN_CONTROL/$exp/templates") {
    die('Please set up $BBN_CONTROL correctly.');
}

foreach $sgm_file (@sgm_files) {
    $basename = basename($sgm_file, ($sgmsuffix));
    $apf_file_check = "$apfdir/$basename$apfsuffix";
    die "Could not find apf file: $apf_file_check" unless -f $apf_file_check;
    $result = 0;
    if (length($dtd) > 0) {
	$result = system ("$XMLLINT --valid --noout --dtdvalid $dtd $apf_file_check");
    }
    
    if ($result != 0) {
	die "Problem XML validating $apf_file_check";
    }
}

$num_batches = int($num_files / $NUM_FILES_PER_CA_BATCH);
$num_batches++ if $num_files % $NUM_FILES_PER_CA_BATCH != 0;

##############################
# Collect tag types
serif_train_mkpath($tagsdir);
my $collect_tags = runjobs(\@preprocessing_jobs,
			 "COLLECT_TRAINING_TAGS",
			 \%local_params,
			 $CSH, "tmpl.run_collect_training_tags.csh");
push(@preprocessing_jobs, $collect_tags);

##############################
# Training into batches
my $batches_run = runjobs(\@preprocessing_jobs,
			 "BREAK_UP_TRAINING",
			 \%local_params,
			 $CSH, "tmpl.run_break_up_training.csh");
push(@preprocessing_jobs, $batches_run);

##############################
# Run CA Serif
my @prev_jobs = @preprocessing_jobs;

for ($i = 0; $i < $num_batches; $i++) {
    $current_dir = $i;
    $current_dir = "000" . $current_dir if length($current_dir) == 1;
    $current_dir = "00" . $current_dir if length($current_dir) == 2;
    $current_dir = "0" . $current_dir if length($current_dir) == 3;
    $padded = $current_dir;
    $current_dir = "$outdir/batches/$current_dir";
    
    my $ca_run = runjobs(\@prev_jobs,
			 "RUN_CA_SERIF-$padded",
			 \%local_params,
			 $CA_SERIF, "tmpl.$SERIF_LANG_LOWER.ca_${SERIF_LANG_LOWER}.par");
    push(@preprocessing_jobs, $ca_run);
}

$intermediate_dir = "$outdir/intermediate-files";
serif_train_mkpath($intermediate_dir);

# Output files for storing lists of training data
$local_params{STATE_FILE_LIST} = "$intermediate_dir/$exp.state-15-files.list";
$local_params{MC_NAME_TRAINING} = "$intermediate_dir/$exp.name-training.mc.list";
$local_params{LC_NAME_TRAINING} = "$intermediate_dir/$exp.name-training.lc.list";
$local_params{MC_VALUE_TRAINING} = "$intermediate_dir/$exp.value-training.mc.list";
$local_params{LC_VALUE_TRAINING} = "$intermediate_dir/$exp.value-training.lc.list";
$local_params{MC_OTHER_VALUE_TRAINING} = "$intermediate_dir/$exp.other-value-training.mc.list";
$local_params{LC_OTHER_VALUE_TRAINING} = "$intermediate_dir/$exp.other-value-training.lc.list";
$local_params{MC_TIMEX_TRAINING} = "$intermediate_dir/$exp.timex-training.mc.list";
$local_params{LC_TIMEX_TRAINING} = "$intermediate_dir/$exp.timex-training.lc.list";
 
##############################
# Create list of state-files
my $create_lists_run = runjobs(\@preprocessing_jobs,
				"COLLECT_FILES",
				\%local_params,
				 $CSH, "tmpl.run_create_lists_script.csh");
push(@preprocessing_jobs, $create_lists_run);

##############################
# Descriptor Classification
if ($do_desc_training eq "run") {
    serif_train_mkpath($desc_out_dir);
    my $desc_training_run = runjobs(\@preprocessing_jobs,
                                    "DESC_CLASSIFY",
                                    \%local_params,
                                    $P1_DESC_TRAINER, "tmpl.$SERIF_LANG_LOWER.descriptor_training.par");
    push(@training_jobs, $desc_training_run);
}

##############################
# DT Desc Coref
if ($do_dt_desc_coref_training eq "run") {
    serif_train_mkpath($dt_desc_coref_out_dir);
    my $dt_desc_coref_run = runjobs(\@preprocessing_jobs,
                               "DT_DESC_COREF_TRAIN",
                               \%local_params,
                               $DT_COREF_TRAINER, "tmpl.$SERIF_LANG_LOWER.dt_desc_coref_training.par");
    push(@training_jobs, $dt_desc_coref_run);
}

##############################
# DT Name Coref
if ($do_dt_name_coref_training eq "run") {
    serif_train_mkpath($dt_name_coref_out_dir);
    my $dt_name_coref_run = runjobs(\@preprocessing_jobs,
                                    "DT_NAME_COREF_TRAIN",
                                    \%local_params,
                                    $DT_COREF_TRAINER, "tmpl.$SERIF_LANG_LOWER.dt_name_coref_training.par");
    push(@training_jobs, $dt_name_coref_run);
}

##############################
# DT Pronoun Coref
if ($do_dt_pron_coref_training eq "run") {
    serif_train_mkpath($dt_pron_coref_out_dir);
    my $dt_pron_coref_run = runjobs(\@preprocessing_jobs,
                                    "DT_PRON_COREF_TRAIN",
                                    \%local_params,
                                    $DT_COREF_TRAINER, "tmpl.$SERIF_LANG_LOWER.dt_pron_coref_training.par");
    push(@training_jobs, $dt_pron_coref_run);
}

##############################
# Event Training AA
if ($do_event_aa_training eq "run") {
    serif_train_mkpath($event_aa_out_dir);
    my $event_aa_run = runjobs(\@preprocessing_jobs,
                               "EVENT_FINDER_AA",
                               \%local_params,
                               $EVENT_FINDER, "tmpl.$SERIF_LANG_LOWER.event_training_aa.par");
    push(@training_jobs, $event_aa_run);

}

##############################
# Event Training Link
if ($do_event_link_training eq "run") {
    serif_train_mkpath($event_link_out_dir);
    my $event_link_run = runjobs(\@preprocessing_jobs,
                                 "EVENT_FINDER_LINK",
                                 \%local_params,
                                 $EVENT_FINDER, "tmpl.$SERIF_LANG_LOWER.event_training_link.par");
    push(@training_jobs, $event_link_run);
}

##############################
# Event Training Trigger
if ($do_event_trigger_training eq "run") {
    serif_train_mkpath($event_trigger_out_dir);
    my $event_trigger_run = runjobs(\@preprocessing_jobs,
                                    "EVENT_FINDER_TRIGGER",
                                    \%local_params,
                                    $EVENT_FINDER, "tmpl.$SERIF_LANG_LOWER.event_training_trigger.par");
    push(@training_jobs, $event_trigger_run);
}

##############################
# Nominal Premods
if ($do_nom_premod_training eq "run") {
    serif_train_mkpath($nom_premod_out_dir);
    my $nom_premod_run = runjobs(\@preprocessing_jobs,
                                 "NOM_PREMODS",
                                 \%local_params,
                                 $P1_DESC_TRAINER, "tmpl.$SERIF_LANG_LOWER.nominal_premod_training.par");
    push(@training_jobs, $nom_premod_run);
}


##############################
# PM Pronoun Link
if ($do_pm_pronoun_coref_training eq "run") {
    serif_train_mkpath($pm_pronoun_coref_out_dir);
    my $pm_pron_run = runjobs(\@preprocessing_jobs,
                              "PRONOUN_LINK", 
                              \%local_params,
                              $PRONOUN_LINKER_TRAINER, "tmpl.$SERIF_LANG_LOWER.pronoun_link_training.par");
    push(@training_jobs, $pm_pron_run);
}

##############################
# Relations Vector Tree
if ($do_relation_vector_tree_training eq "run") {
    serif_train_mkpath($relation_vector_tree_out_dir);
    my $relation_vt_run = runjobs(\@preprocessing_jobs,
                                  "RELATION_VT", 
                                  \%local_params,
                                  $RELATION_TRAINER, "tmpl.$SERIF_LANG_LOWER.relation_training_vector_tree.par");
    push(@training_jobs, $relation_vt_run);
}

##############################
# Relations P1
if ($do_relation_p1_training eq "run") {
    serif_train_mkpath($relation_p1_out_dir);
    my $relation_p1_run = runjobs(\@preprocessing_jobs,
                                  "RELATION_P1", 
                                  \%local_params,
                                  $P1_RELATION_TRAINER, "tmpl.$SERIF_LANG_LOWER.relation_training_p1.par");
    push(@training_jobs, $relation_p1_run);
}

##############################
# Relations MaxEnt
if ($do_relation_maxent_training eq "run") {
    serif_train_mkpath($relation_maxent_out_dir);
    my $relation_maxent_run = runjobs(\@preprocessing_jobs,
                                      "RELATION_MAXENT", 
                                      \%local_params,
                                      $MAXENT_RELATION_TRAINER, "tmpl.$SERIF_LANG_LOWER.relation_training_maxent.par");
    push(@training_jobs, $relation_maxent_run);
}

##############################
# Relations Timex Attachement
if ($do_relation_timex_training eq "run") {
    serif_train_mkpath($relation_timex_out_dir);
    my $relation_timex_run = runjobs(\@preprocessing_jobs,
                                     "RELATION_TIMEX",
                                     \%local_params,
                                     $RELATION_TIMEX_ARG_FINDER, "tmpl.$SERIF_LANG_LOWER.relation_training_timex.par");
    push(@training_jobs, $relation_timex_run);
}

##############################
# Stat Name Linker (Coref)
if ($do_stat_name_coref_training eq "run") {
    serif_train_mkpath($stat_name_coref_out_dir);
    my $stat_name_coref_run = runjobs(\@preprocessing_jobs,
                                      "STAT_NAME_COREF",
                                      \%local_params,
                                      $NAME_LINKER_TRAINER, "tmpl.$SERIF_LANG_LOWER.stat_name_coref_training.par");
    push(@training_jobs, $stat_name_coref_run);
}

##############################
# IdF Mixed Case Name
if ($do_mixed_case_name_training eq "run") {
    serif_train_mkpath($mixed_case_name_out_dir);
    
    $names_mc_ct_job = 
	runjobs(\@preprocessing_jobs,
		"COLLECT_MC_NAME_TRAINING",
		{ IDF_TRAINING_FILE_LIST => $local_params{MC_NAME_TRAINING},
		  TRAINING_PREFIX => "nametraining",
		  OUTPUT_PREFIX => "$exp.nametraining",
		  MIXED_CASE => 1,
                  TRAINING_UNIT => $name_training_unit,
	          IDF_TYPE => "names"},
		$CSH, "tmpl.run_collect_idf_training.csh");
    my @prev_jobs = @preprocessing_jobs;
    push(@prev_jobs, $names_mc_ct_job);
    
    if ($name_training_type eq "idf") {
	my $idf_mc_run = runjobs(\@prev_jobs,
                                 "NAME_MC_IDF", 
                                 { IDF_TAGS_FILE => "$SERIF_DATA_DIR/$SERIF_LANG_LOWER/names/idf/ace2004.tags",
                                   IDF_TRAINING_FILE_LIST => $local_params{MC_NAME_TRAINING},
                                   IDF_MODEL => "$mixed_case_name_out_dir/$mixed_case_name_model_name" },
                                 $CSH, "tmpl.run_idf.csh");
        push(@training_jobs, $idf_mc_run);
    } elsif ($name_training_type eq "pidf") {
	my $pidf_mc_run = runjobs(\@prev_jobs,
                                  "NAME_MC_PIDF",
                                  { PIDF_TRAINING_FILE_LIST => $local_params{MC_NAME_TRAINING}},
                                  $PIDF_TRAINER, "tmpl.$SERIF_LANG_LOWER.pidf_mc_name_training.par");
        push(@training_jobs, $pidf_mc_run);
    }
}

##############################
# IdF Lower Case Name
if ($do_lower_case_name_training eq "run") {
    serif_train_mkpath($lower_case_name_out_dir);
    
    $names_lc_ct_job = 
	runjobs(\@preprocessing_jobs,
		"COLLECT_LC_NAME_TRAINING",
		{ IDF_TRAINING_FILE_LIST => $local_params{LC_NAME_TRAINING},
		  TRAINING_PREFIX => "nametraining",
		  OUTPUT_PREFIX => "$exp.nametraining",
		  MIXED_CASE => 0, 
                  TRAINING_UNIT => $name_training_unit,
	          IDF_TYPE => "names" },
		$CSH, "tmpl.run_collect_idf_training.csh");
    my @prev_jobs = @preprocessing_jobs;
    push(@prev_jobs, $names_lc_ct_job);
    
    if ($name_training_type eq "idf") {
	my $idf_lc_run = runjobs(\@prev_jobs,
                                 "NAME_LC_IDF", 
                                 { IDF_TAGS_FILE => "$SERIF_DATA_DIR/$SERIF_LANG_LOWER/names/idf/ace2004.tags",
                                   IDF_TRAINING_FILE_LIST => $local_params{LC_NAME_TRAINING},
                                   IDF_MODEL => "$lower_case_name_out_dir/$lower_case_name_model_name" },
                                 $CSH, "tmpl.run_idf.csh");
        push(@training_jobs, $idf_lc_run);
    } elsif ($name_training_type eq "pidf") {
	my $pidf_lc_run = runjobs(\@prev_jobs,
                                  "NAME_LC_PIDF",
                                  { PIDF_TRAINING_FILE_LIST => $local_params{LC_NAME_TRAINING}},
                                  $PIDF_TRAINER, "tmpl.$SERIF_LANG_LOWER.pidf_lc_name_training.par");
        push(@training_jobs, $pidf_lc_run);
    }
}

##############################
# IdF Mixed Case Values
if ($do_mixed_case_value_training eq "run") {
    serif_train_mkpath($mixed_case_value_out_dir);

    $values_mc_ct_job = 
	runjobs(\@preprocessing_jobs,
		"COLLECT_MC_VALUE_TRAINING",
		{ IDF_TRAINING_FILE_LIST => $local_params{MC_VALUE_TRAINING},
		  TRAINING_PREFIX => "valuetraining",
		  OUTPUT_PREFIX => "$exp.valuetraining",
		  MIXED_CASE => 1, 
                  TRAINING_UNIT => $name_training_unit,
	          IDF_TYPE => "values" },
		$CSH, "tmpl.run_collect_idf_training.csh");

    my @prev_jobs = @preprocessing_jobs;
    push(@prev_jobs, $values_mc_ct_job);
    
    if ($name_training_type eq "idf") {
	my $idf_val_mc_run = runjobs(\@prev_jobs,
                                     "VALUE_MC_IDF", 
                                     { IDF_TAGS_FILE => "$SERIF_DATA_DIR/$SERIF_LANG_LOWER/values/idf/ace2005.tags",
                                       IDF_TRAINING_FILE_LIST => $local_params{MC_OTHER_VALUE_TRAINING},
                                       IDF_MODEL => "$mixed_case_value_out_dir/$mixed_case_value_model_name" },
                                     $CSH, "tmpl.run_idf.csh");
        push(@training_jobs, $idf_val_mc_run);
    } elsif ($name_training_type eq "pidf") {
	my $pidf_val_mc_run = runjobs(\@prev_jobs,
                                      "VALUE_MC_PIDF",
                                      { PIDF_TRAINING_FILE_LIST => $local_params{MC_VALUE_TRAINING}},
                                      $PIDF_TRAINER, "tmpl.$SERIF_LANG_LOWER.pidf_mc_values_training.par");
        push(@training_jobs, $pidf_val_mc_run);
    }
}

##############################
# IdF Lower Case Values
if ($do_lower_case_value_training eq "run") {
    serif_train_mkpath($lower_case_value_out_dir);

    $values_lc_ct_job = 
	runjobs(\@preprocessing_jobs,
		"COLLECT_LC_VALUE_TRAINING",
		{ IDF_TRAINING_FILE_LIST => $local_params{LC_VALUE_TRAINING},
		  TRAINING_PREFIX => "valuetraining",
		  OUTPUT_PREFIX => "$exp.valuetraining",
		  MIXED_CASE => 0, 
                  TRAINING_UNIT => $name_training_unit,
	          IDF_TYPE => "values" },
		$CSH, "tmpl.run_collect_idf_training.csh");


    my @prev_jobs = @preprocessing_jobs;
    push(@prev_jobs, $values_lc_ct_job);
    
    if ($name_training_type eq "idf") {
	my $idf_val_lc_run = runjobs(\@prev_jobs,
                                     "VALUE_LC_IDF", 
                                     { IDF_TAGS_FILE => "$SERIF_DATA_DIR/$SERIF_LANG_LOWER/values/idf/ace2005.tags",
                                       IDF_TRAINING_FILE_LIST => $local_params{LC_VALUE_TRAINING},
                                       IDF_MODEL => "$lower_case_value_out_dir/$lower_case_value_model_name" },
                                     $CSH, "tmpl.run_idf.csh");
        push(@training_jobs, $idf_val_lc_run);
    } elsif ($name_training_type eq "pidf") {
	my $pidf_val_lc_run = runjobs(\@prev_jobs,
                                      "VALUE_LC_PIDF",
                                      { PIDF_TRAINING_FILE_LIST => $local_params{LC_VALUE_TRAINING}},
                                      $PIDF_TRAINER, "tmpl.$SERIF_LANG_LOWER.pidf_lc_values_training.par");
        push(@training_jobs, $pidf_val_lc_run);
    }
}

##############################
# IdF Mixed Case Other Values
if ($do_mixed_case_other_value_training eq "run") {
    serif_train_mkpath($mixed_case_other_value_out_dir);

    $other_values_mc_ct_job = 
	runjobs(\@preprocessing_jobs,
		"COLLECT_MC_OTHER_VALUE_TRAINING",
		{ IDF_TRAINING_FILE_LIST => $local_params{MC_OTHER_VALUE_TRAINING},
		  TRAINING_PREFIX => "valuetraining",
		  OUTPUT_PREFIX => "$exp.valuetraining",
		  MIXED_CASE => 1, 
                  TRAINING_UNIT => $name_training_unit,
	          IDF_TYPE => "other-values" },
		$CSH, "tmpl.run_collect_idf_training.csh");

    my @prev_jobs = @preprocessing_jobs;
    push(@prev_jobs, $other_values_mc_ct_job);
    
    if ($name_training_type eq "idf") {
	my $idf_other_val_mc_run = 
            runjobs(\@prev_jobs,
                    "VALUE_MC_IDF", 
                    { IDF_TAGS_FILE => "$SERIF_DATA_DIR/$SERIF_LANG_LOWER/values/idf/no-timex.tags",
                      IDF_TRAINING_FILE_LIST => $local_params{MC_OTHER_VALUE_TRAINING},
                      IDF_MODEL => "$mixed_case_other_value_out_dir/$mixed_case_other_value_model_name" },
                    $CSH, "tmpl.run_idf.csh");
        push(@training_jobs, $idf_other_val_mc_run);
    } elsif ($name_training_type eq "pidf") {
	my $pidf_other_val_mc_run = 
            runjobs(\@prev_jobs,
                    "VALUE_MC_PIDF",
                    { PIDF_TRAINING_FILE_LIST => $local_params{MC_OTHER_VALUE_TRAINING}},
                    $PIDF_TRAINER, "tmpl.$SERIF_LANG_LOWER.pidf_mc_other_values_training.par");
        push(@training_jobs, $pidf_other_val_mc_run);
    }
}

##############################
# IdF Lower Case Other Values
if ($do_lower_case_other_value_training eq "run") {
    serif_train_mkpath($lower_case_other_value_out_dir);

    $other_values_lc_ct_job = 
	runjobs(\@preprocessing_jobs,
		"COLLECT_LC_OTHER_VALUE_TRAINING",
		{ IDF_TRAINING_FILE_LIST => $local_params{LC_OTHER_VALUE_TRAINING},
		  TRAINING_PREFIX => "valuetraining",
		  OUTPUT_PREFIX => "$exp.valuetraining",
		  MIXED_CASE => 0, 
                  TRAINING_UNIT => $name_training_unit,
	          IDF_TYPE => "other-values" },
		$CSH, "tmpl.run_collect_idf_training.csh");


    my @prev_jobs = @preprocessing_jobs;
    push(@prev_jobs, $other_values_lc_ct_job);
    
    if ($name_training_type eq "idf") {
        my $idf_other_val_lc_run =
            runjobs(\@prev_jobs,
                    "VALUE_LC_IDF", 
                    { IDF_TAGS_FILE => "$SERIF_DATA_DIR/$SERIF_LANG_LOWER/values/idf/no-timex.tags",
                      IDF_TRAINING_FILE_LIST => $local_params{LC_OTHER_VALUE_TRAINING},
                      IDF_MODEL => "$lower_case_other_value_out_dir/$lower_case_other_value_model_name" },
                    $CSH, "tmpl.run_idf.csh");
        push(@training_jobs, $idf_other_val_lc_run);
    } elsif ($name_training_type eq "pidf") {
	my $pidf_other_val_lc_run =
            runjobs(\@prev_jobs,
                    "VALUE_LC_PIDF",
                    { PIDF_TRAINING_FILE_LIST => $local_params{LC_OTHER_VALUE_TRAINING}},
                    $PIDF_TRAINER, "tmpl.$SERIF_LANG_LOWER.pidf_lc_other_values_training.par");
        push(@training_jobs, $pidf_other_val_lc_run);
    }
}

##############################
# IdF Mixed Case Timex
if ($do_mixed_case_timex_training  eq "run") {
    serif_train_mkpath($mixed_case_timex_out_dir);

    $timex_mc_ct_job = 
	runjobs(\@preprocessing_jobs,
		"COLLECT_MC_TIMEX_TRAINING",
		{ IDF_TRAINING_FILE_LIST => $local_params{MC_TIMEX_TRAINING},
		  TRAINING_PREFIX => "valuetraining",
		  OUTPUT_PREFIX => "$exp.timextraining",
		  MIXED_CASE => 1, 
                  TRAINING_UNIT => $name_training_unit,
	          IDF_TYPE => "times" },
		$CSH, "tmpl.run_collect_idf_training.csh");

    my @prev_jobs = @preprocessing_jobs;
    push(@prev_jobs, $timex_mc_ct_job);
    
    if ($name_training_type eq "idf") {
        my $idf_timex_mc_run = 
            runjobs(\@prev_jobs,
                    "TIMEX_MC_IDF", 
                    { IDF_TAGS_FILE => "$SERIF_DATA_DIR/$SERIF_LANG_LOWER/values/idf/just-timex.tags",
                      IDF_TRAINING_FILE_LIST => $local_params{MC_TIMEX_TRAINING},
                      IDF_MODEL => "$mixed_case_timex_out_dir/$mixed_case_timex_model_name" },
                    $CSH, "tmpl.run_idf.csh");
        push(@training_jobs, $idf_timex_mc_run);
    } elsif ($name_training_type eq "pidf") {
        my $pidf_timex_mc_run = 
            runjobs(\@prev_jobs,
                    "TIMEX_MC_PIDF",
                    { PIDF_TRAINING_FILE_LIST => $local_params{MC_TIMEX_TRAINING}},
                    $PIDF_TRAINER, "tmpl.$SERIF_LANG_LOWER.pidf_mc_timex_training.par");
        push(@training_jobs, $pidf_timex_mc_run);
    }
}

##############################
# IdF Lower Case Timex
if ($do_lower_case_timex_training eq "run") {
    serif_train_mkpath($lower_case_timex_out_dir);

    $timex_lc_ct_job = 
	runjobs(\@preprocessing_jobs,
		"COLLECT_LC_TIMEX_TRAINING",
		{ IDF_TRAINING_FILE_LIST => $local_params{LC_TIMEX_TRAINING},
		  TRAINING_PREFIX => "valuetraining",
		  OUTPUT_PREFIX => "$exp.timextraining",
		  MIXED_CASE => 0, 
                  TRAINING_UNIT => $name_training_unit,
	          IDF_TYPE => "times" },
		$CSH, "tmpl.run_collect_idf_training.csh");


    my @prev_jobs = @preprocessing_jobs;
    push(@prev_jobs, $timex_lc_ct_job);
    
    if ($name_training_type eq "idf") {
        my $idf_timex_lc_run =
            runjobs(\@prev_jobs,
                    "TIMEX_LC_IDF", 
                    { IDF_TAGS_FILE => "$SERIF_DATA_DIR/$SERIF_LANG_LOWER/values/idf/just-timex.tags",
                      IDF_TRAINING_FILE_LIST => $local_params{LC_TIMEX_TRAINING},
                      IDF_MODEL => "$lower_case_timex_out_dir/$lower_case_timex_model_name" },
                    $CSH, "tmpl.run_idf.csh");
        push(@training_jobs, $idf_timex_lc_run);
    } elsif ($name_training_type eq "pidf") {
        my $pidf_timex_lc_run = 
            runjobs(\@prev_jobs,
                    "TIMEX_LC_PIDF",
                    { PIDF_TRAINING_FILE_LIST => $local_params{LC_TIMEX_TRAINING}},
                    $PIDF_TRAINER, "tmpl.$SERIF_LANG_LOWER.pidf_lc_timex_training.par");
        push(@training_jobs, $pidf_timex_lc_run);
    }
}


########################################
# Run Serif test with new models 
# (test on train)

for ($i = 0; $i < $num_batches; $i++) {
    $current_dir = $i;
    $current_dir = "000" . $current_dir if length($current_dir) == 1;
    $current_dir = "00" . $current_dir if length($current_dir) == 2;
    $current_dir = "0" . $current_dir if length($current_dir) == 3;
    $padded = $current_dir;
    $test_dir = "$outdir/test/$padded";
    $current_dir = "$outdir/batches/$padded";

    serif_train_mkpath($test_dir);

    runjobs(\@training_jobs,
            "RUN_SERIF_TEST-$padded",
            \%local_params,
            $SERIF, "tmpl.$SERIF_LANG_LOWER.test.par");
}


endjobs();
