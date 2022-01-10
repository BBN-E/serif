# Copyright 2008 by BBN Technologies Corp.     All Rights Reserved.

package main;

############################################################################
# Global Variables
# 
# Global variables are expanded in the templates by variables 
# with the same name, but surrounded by plusses (e.g. +VAR_NAME+)
############################################################################

# The top level experiment dir for this experiment number
$expbase = "$BBN_CONTROL";
$expdir = "$expbase/$exp/expts";

# Directory must contain the following scripts, used by serif.train.pl:
# create-lists-of-files.pl, collect-idf-training.pl, break-up-training.pl
# run_csh_script
$TRAINING_SCRIPT_DIR = "$expbase/$exp/bin";

# Location of Serif data and model files
$SERIF_DIR = "***SERIF-DIR***";
$SERIF_BUILD_DIR = "***SERIF-BUILD-DIR***";
$CA_SERIF_BUILD_DIR = "***CA-SERIF-BUILD-DIR***";
$SERIF_DATA_DIR = "$SERIF_DIR/data";

# Training data language
$SERIF_LANG = "***LANGUAGE***";
$SERIF_LANG_LOWER = lc($SERIF_LANG);

# Location of training input and output. File names must have a 
# one-to-one correspondence in the sgmdir and the apfdir.
$sgmdir = "***SGM-DIR***";
$sgmsuffix = ".sgm";
$apfdir = "***APF-DIR***";
$apfsuffix = ".apf.xml";
$outdir = "$expdir/" . $SERIF_LANG_LOWER;

$tagsdir = "$expdir/params";
$entity_types_file = "$tagsdir/entity_types.txt";
$entity_tags_file = "$tagsdir/entity_tags.txt";
$entity_subtypes_file = "$tagsdir/entity_subtypes.txt";
$value_types_file = "$tagsdir/value_types.txt";
$value_tags_file = "$tagsdir/value_tags.txt";
$relation_types_file = "$tagsdir/relation_types.txt";
$event_triggers_file = "$tagsdir/event_triggers.txt";
$event_args_file = "$tagsdir/event_args.txt";

$standard_entity_types = "$SERIF_DATA_DIR/ace/ace_2004_entity_types.txt";
$standard_entity_subtypes = "$SERIF_DATA_DIR/ace/entity-subtypes-2005.txt";

$regions_to_process = "TEXT,TURN,SPEAKER,POST,POSTER,POSTDATE,SUBJECT";
$speaker_tags = "SPEAKER";
$receiver_tags = "RECEIVER";

# Apf DTD for XML validation. Set $dtd to empty string ("") 
# to skip validation step
$dtd = "$apfdir/apf.v5.1.1.dtd";
$XMLLINT = "/usr/bin/xmllint";

# Number of files for each pre-training batch (CorrectAnswerSerif run)
$NUM_FILES_PER_CA_BATCH = 20;

# "pidf" or "idf", used for names, values, timex, and other-values
$name_training_type = "pidf";

# Descriptor Classification
$desc_out_dir = "$outdir/desc";
$desc_model_name = "$exp.desc-classify";

# DT Desc Coref
$dt_desc_coref_out_dir = "$outdir/dt-desc-coref";
$dt_desc_coref_model_name = "$exp.dt-desc-coref";

# DT Name Coref
$dt_name_coref_out_dir = "$outdir/dt-name-coref";
$dt_name_coref_model_name = "$exp.dt-name-coref";

# DT Pron Coref
$dt_pron_coref_out_dir = "$outdir/dt-pron-coref";
$dt_pron_coref_model_name = "$exp.dt-pron-coref";

# Event Training AA
$event_aa_out_dir = "$outdir/events";
$event_aa_model_name = "$exp.event-aa";

# Event Training Link
$event_link_out_dir = "$outdir/events";
$event_link_model_name = "$exp.event-link";

# Event Training Trigger
$event_trigger_out_dir = "$outdir/events";
$event_trigger_model_name = "$exp.event-trigger";

# Nominal Premod Classification
$nom_premod_out_dir = "$outdir/nom-premods";
$nom_premod_model_name = "$exp.nom-premod";

# PM Pronoun Coref Training
$pm_pronoun_coref_out_dir = "$outdir/pm-pron-coref";
$pm_pronoun_coref_model_name = "$exp.pm-pron-coref";

# Relation Vector Tree
$relation_vector_tree_out_dir = "$outdir/relations";
$relation_vector_tree_model_name = "$exp.relation-vt";

# Relation P1
$relation_p1_out_dir = "$outdir/relations";
$relation_p1_model_name = "$exp.relation-p1";

# Relation MaxEnt
$relation_maxent_out_dir = "$outdir/relations";
$relation_maxent_model_name = "$exp.relation-maxent";

# Relation Timex Attachment
$relation_timex_out_dir = "$outdir/relations";
$relation_timex_model_name = "$exp.relation-ta";

# Stat Name Linker (Coref)
$stat_name_coref_out_dir = "$outdir/stat-name-coref";
$stat_name_coref_model_name = "$exp.stat-name-coref";

# Mixed Case Name Finder 
$mixed_case_name_out_dir = "$outdir/names";
$mixed_case_name_model_name = "$exp.names.mc";

# Lower Case Name Finder 
$lower_case_name_out_dir = "$outdir/names";
$lower_case_name_model_name = "$exp.names.lc";

# Mixed Case Value Finder 
$mixed_case_value_out_dir = "$outdir/values";
$mixed_case_value_model_name = "$exp.values.mc";

# Lower Case Value Finder 
$lower_case_value_out_dir = "$outdir/values";
$lower_case_value_model_name = "$exp.values.lc";

# Mixed Case Other Value Finder 
$mixed_case_other_value_out_dir = "$outdir/values";
$mixed_case_other_value_model_name = "$exp.other-values.mc";

# Lower Case Other Value Finder 
$lower_case_other_value_out_dir = "$outdir/values";
$lower_case_other_value_model_name = "$exp.other-values.lc";

# Mixed Case Timex Finder 
$mixed_case_timex_out_dir = "$outdir/values";
$mixed_case_timex_model_name = "$exp.timex.mc";

# Lower Case Timex Finder 
$lower_case_timex_out_dir = "$outdir/values";
$lower_case_timex_model_name = "$exp.timex.lc";

$SERIF = $SERIF_BUILD_DIR . "/Serif_" . $SERIF_LANG . "/LinuxSerif" . $SERIF_LANG;

$CA_SEXP_SCRIPT = $SERIF_DIR . "/bin/convert-2005-apf-to-sexp-with-ids-v1.6.pl";
$CA_SERIF = $CA_SERIF_BUILD_DIR . "/CASerif_" . $SERIF_LANG . "/LinuxCASerif_" . $SERIF_LANG;

$CREATE_LISTS_SCRIPT = "$TRAINING_SCRIPT_DIR/create-lists-of-files.pl";
$COLLECT_IDF_TRAINING_SCRIPT = "$TRAINING_SCRIPT_DIR/collect-idf-training.pl";
$BREAK_UP_TRAINING_SCRIPT = "$TRAINING_SCRIPT_DIR/break-up-training.pl";
$COLLECT_TRAINING_TAGS_SCRIPT = "$TRAINING_SCRIPT_DIR/collect-training-tags.pl";
$CSH = "$TRAINING_SCRIPT_DIR/run_csh_script";

$DT_COREF_TRAINER = $SERIF_BUILD_DIR . "/DTCorefTrainer/LinuxDTCorefTrainer";
$EVENT_FINDER = $SERIF_BUILD_DIR . "/EventFinder/LinuxEventFinder";
$IDF_TRAINER = $SERIF_BUILD_DIR . "/IdfTrainer/LinuxIdfTrainer";
$PIDF_TRAINER = $SERIF_BUILD_DIR . "/PIdFTrainer/LinuxPIdFTrainer";
$MAXENT_RELATION_TRAINER = $SERIF_BUILD_DIR . "/MaxEntRelationTrainer/LinuxMaxEntRelationTrainer";
$NAME_LINKER_TRAINER = $SERIF_BUILD_DIR . "/NameLinkerTrainer/LinuxNameLinkerTrainer";
$P1_DESC_TRAINER = $SERIF_BUILD_DIR . "/P1DescTrainer/LinuxP1DescTrainer";
$P1_RELATION_TRAINER = $SERIF_BUILD_DIR . "/P1RelationTrainer/LinuxP1RelationTrainer";
$PRONOUN_LINKER_TRAINER = $SERIF_BUILD_DIR . "/PronounLinkerTrainer/LinuxPronounLinkerTrainer";
$RELATION_TIMEX_ARG_FINDER = $SERIF_BUILD_DIR . "/RelationTimexArgFinder/LinuxRelationTimexArgFinder";
$RELATION_TRAINER = $SERIF_BUILD_DIR . "/RelationTrainer/LinuxRelationTrainer";


sub serif_train_mkpath
{
    my $dir = shift;

    if($runjobs3000::major_mode == runjobs3000::MM_RUN) {
        printf("SERIF train creating dir: %s\n", $dir);
        mkpath($dir);
    }
}

sub set_language_specific_variables
{
    my $lang = shift;

    if ($lang eq "English") {
        $name_training_unit = "token";
        $RELATION_ALT_MODEL_FILE = "$SERIF_DATA_DIR/english/relations/alt-models/ace2007-alt-model-params";

        $do_desc_training = "run";
        $do_dt_desc_coref_training = "run";
        $do_dt_name_coref_training = "run";
        $do_dt_pron_coref_training = "run";
        $do_event_aa_training = "run";
        $do_event_link_training = "run";
        $do_event_trigger_training = "run";
        $do_nom_premod_training = "run";
        $do_pm_pronoun_coref_training = "skip";
        $do_relation_vector_tree_training = "run";
        $do_relation_p1_training = "run";
        $do_relation_maxent_training = "run";
        $do_relation_timex_training = "run";
        $do_stat_name_coref_training = "skip";
        $do_mixed_case_name_training = "run";
        $do_lower_case_name_training = "run";
        $do_mixed_case_value_training = "skip";
        $do_lower_case_value_training = "skip";
        $do_mixed_case_other_value_training = "run";
        $do_lower_case_other_value_training = "run";
        $do_mixed_case_timex_training = "run";
        $do_lower_case_timex_training = "run";
    }
    if ($lang eq "Chinese") {
        $name_training_unit = "char";
        $RELATION_ALT_MODEL_FILE = "$SERIF_DATA_DIR/chinese/relations/alt-models/1201.giga9804-tdt4bn.2004-alt-models.txt";
        $do_desc_training = "run";
        $do_dt_desc_coref_training = "run";
        $do_dt_name_coref_training = "skip";
        $do_dt_pron_coref_training = "run";
        $do_event_aa_training = "run";
        $do_event_link_training = "run";
        $do_event_trigger_training = "run";
        $do_nom_premod_training = "skip";
        $do_pm_pronoun_coref_training = "run";
        $do_relation_vector_tree_training = "run";
        $do_relation_p1_training = "run";
        $do_relation_maxent_training = "run";
        $do_relation_timex_training = "run";
        $do_stat_name_coref_training = "run";
        $do_mixed_case_name_training = "skip";
        $do_lower_case_name_training = "run";
        $do_mixed_case_value_training = "skip";
        $do_lower_case_value_training = "run";
        $do_mixed_case_other_value_training = "skip";
        $do_lower_case_other_value_training = "skip";
        $do_mixed_case_timex_training = "skip";
        $do_lower_case_timex_training = "skip";
    }
    if ($lang eq "Arabic") {
        $name_training_unit = "token";
        $RELATION_ALT_MODEL_FILE = "$SERIF_DATA_DIR/arabic/relations/alt-models/altRelationFile.txt";
        $do_desc_training = "run";
        $do_dt_desc_coref_training = "run";
        $do_dt_name_coref_training = "skip";
        $do_dt_pron_coref_training = "skip";
        $do_event_aa_training = "skip";
        $do_event_link_training = "skip";
        $do_event_trigger_training = "skip";
        $do_nom_premod_training = "skip";
        $do_pm_pronoun_coref_training = "skip";
        $do_relation_vector_tree_training = "run";
        $do_relation_p1_training = "run";
        $do_relation_maxent_training = "run";
        $do_relation_timex_training = "skip";
        $do_stat_name_coref_training = "skip";
        $do_mixed_case_name_training = "skip";
        $do_lower_case_name_training = "run";
        $do_mixed_case_value_training = "skip";
        $do_lower_case_value_training = "run";
        $do_mixed_case_other_value_training = "skip";
        $do_lower_case_other_value_training = "skip";
        $do_mixed_case_timex_training = "skip";
        $do_lower_case_timex_training = "skip";
    }
}

1;
