use strict;
use File::Copy;

if (@ARGV != 5) {
    print "Usage: language delivery_root new_data_dir experiment_dir model_name\n";
    print "\n";
    print " ******USE FULL PATH NAMES******\n";
    print "\n";
    print " language: 'english', 'arabic', or 'chinese'\n";
    print " delivery_root: the location of the SERIF delivery package\n";
    print " new_data_directory: a directory containing all non-BBN-annotated\n";
    print "    files to be included in this version of the model\n";
    print " experiment_directory: the directory where all work will be done\n";
    print " model_name: the name of the models to be created, e.g. 'english-010411'\n";
    print "\n";
    exit(1);
}

my ($language, $delivery_root, $new_data, $expt, $model) = @ARGV;

if ($language ne 'english' && $language ne 'chinese' && $language ne 'arabic') {
    print "\nFirst parameter must be 'english', 'arabic', or 'chinese'\n\n";
    exit(1);
}

mkdir($expt);
mkdir("$expt/processing");
mkdir("$expt/new-data");


# Set up simple parameter files
copy("$delivery_root/par/config.par", "$expt/processing/config.par");
copy("$delivery_root/par/names-and-values.best-$language.par", "$expt/processing/process.par");
my $all_new_data = "$expt/all-new-data.sexp";
if (-f $all_new_data) {
    print "Removing pre-existing file $all_new_data\n";
    system("rm $all_new_data");
}
system("echo experiment_dir: $expt/processing >> $expt/processing/process.par");

# Process each file & store in $expt/new-data
opendir(ND, $new_data) || die "Could not open directory $new_data\n";
my @files = grep { /\w/ } readdir(ND);
closedir(ND);
for my $file (@files) {
    if (!(-f "$new_data/$file")) {
	next;
    }
    open(IN, "$new_data/$file") || die "Could not open $new_data/$file\n";
    open(OUT, ">$expt/new-data/$file") || die "Could not open $expt/new-data/$file\n";
    while (<IN>) {
	s/<Person>/<ENAMEX TYPE=\"PER\">/g;
	s/<Organization>/<ENAMEX TYPE=\"ORG\">/g;
	s/<Location>/<ENAMEX TYPE=\"LOC\">/g;
	s/<GPE>/<ENAMEX TYPE=\"GPE\">/g;
	s/<Facility>/<ENAMEX TYPE=\"FAC\">/g;
	s/<Vehicle>/<ENAMEX TYPE=\"VEH\">/g;
	s/<Weapon>/<ENAMEX TYPE=\"WEA\">/g;
	s/<\/Person\>/<\/ENAMEX>/g;
	s/<\/Organization\>/<\/ENAMEX>/g;
	s/<\/Location\>/<\/ENAMEX>/g;
	s/<\/GPE\>/<\/ENAMEX>/g;
	s/<\/Facility\>/<\/ENAMEX>/g;
	s/<\/Vehicle\>/<\/ENAMEX>/g;
	s/<\/Weapon\>/<\/ENAMEX>/g;
	print OUT;
    }
    close(IN);
    close(OUT);
    my $cl = "$delivery_root/name-retrainer/bin/$language/IdfTrainerPreprocessor $expt/processing/process.par -d $expt/new-data/$file";
    if (system($cl)) {
	print "$cl\n";
	print "Problem preprocessing new data; failing.\n";
	exit();
    }
    system("cat $expt/new-data/$file.sexp >> $all_new_data");
}

# If the language is English, create a lowercase version as well
if ($language eq 'english') {
    open(MC, $all_new_data);
    copy($all_new_data, "$expt/all-new-data.sexp.mc");
    open(LC, ">$expt/all-new-data.sexp.lc");
    while (<MC>) {
	my @tokens = split(/ /, $_);
	my $first_token = 1;
	foreach my $token (@tokens) {
	    if ($token !~ /-ST/ && $token !~ /-CO/) {
		$token =~ tr/A-Z/a-z/;
	    }
	    if (!$first_token) {
		print LC " ";
	    }
	    print LC $token;
	    $first_token = 0;
	}
    }
    close(MC);
    close(LC);
} elsif ($language eq 'arabic') {

    ## Do morphology and clitic transformations
    open(MORPH_PAR, ">$expt/morphologyTrainer.par");
    print MORPH_PAR "serif_data:          $delivery_root/data\n";
    print MORPH_PAR "tokenizer_subst:     %serif_data%/arabic/tokenization/token-subst.data\n";
    print MORPH_PAR "MorphModel:          %serif_data%/arabic/morphology/morph_model_from_text8\n";
    print MORPH_PAR "BWMorphDict:         %serif_data%/arabic/morphology/serif-dict-all.txt\n";
    print MORPH_PAR "BWRuleDict:          %serif_data%/arabic/morphology/ruledict.txt\n";
    print MORPH_PAR "entity_type_set:     %serif_data%/ace/ace_2004_entity_types.txt\n";
    print MORPH_PAR "do_mt_normalization: false\n";
    close(MORPH_PAR);

    copy("$expt/all-new-data.sexp", "$expt/all-new-data.raw.sexp");
    
    my $cl = "$delivery_root/name-retrainer/bin/arabic/MorphologyTrainer -dsexp $expt/morphologyTrainer.par $expt/all-new-data.raw.sexp $expt/all-new-data.morphology.sexp";
    print "Tokenizing data. This may take a few minutes.\n";
    if (system($cl)) {
	print "Problem tokenizing data; failing.\n";
	exit(1);
    }    

    $cl = "perl $delivery_root/name-retrainer/bin/arabic/removeKnownNameClitics.pl $delivery_root/name-retrainer/bin/arabic/known_name_clitics.txt $expt/all-new-data.morphology.sexp $expt/all-new-data.clitics.sexp > $expt/clitic-removal.log";
    print "Removing known name clitics. This may take a few minutes.\n";
    if (system($cl)) {
	print "Problem removing known name clitics; failing.\n";
	exit(1);
    }    

    copy("$expt/all-new-data.clitics.sexp", "$expt/all-new-data.sexp");

}

# Create training parameter file(s)
my @versions;
if ($language eq 'english') {
    push @versions, '.mc';
    push @versions, '.lc';
} else {
    push @versions, '';
}
    
foreach my $version (@versions) {
    open(PAR, ">$expt/training$version.par");
    print PAR "parallel: 000\n";
    print PAR "serif_data: $delivery_root/data\n";
    print PAR "pidf_trainer_output_mode: taciturn\n";
    print PAR "pidf_standalone_mode: train\n";
    print PAR "pidf_print_model_every_epoch: true\n";
    print PAR "pidf_trainer_seed_features: true\n";
    print PAR "pidf_trainer_add_hyp_features: true\n";
    print PAR "pidf_trainer_weightsum_granularity: 16\n";
    print PAR "pidf_trainer_epochs: 10\n";
    print PAR "pidf_learn_transitions: false\n";
    print PAR "pidf_training_file: $expt/all-new-data.sexp$version\n";
    print PAR "pidf_model_file: $expt/$model$version\n";
    print PAR "pidf_encrypted_training_file: $delivery_root/name-retrainer/data/$language/BBN$version.all.encrypted.sexp\n";

    if ($language eq 'english') {
	print PAR "pidf_interleave_tags:         false\n";
	print PAR "pidf_tag_set_file:            %serif_data%/english/names/pidf/ace2004.tags\n";
	print PAR "pidf_features_file:           %serif_data%/english/names/pidf/ace2011.features\n";
	if ($version eq '.mc') {
	    print PAR "word_cluster_bits_file:   %serif_data%/english/clusters/ace2005.hBits\n";
	} else {
	    print PAR "word_cluster_bits_file:   %serif_data%/english/clusters/ace2005.lowercase.hBits\n";
	}
    } elsif ($language eq 'arabic') {
	print PAR "pidf_interleave_tags:         true\n";
	print PAR "pidf_features_file:           %serif_data%/arabic/names/pidf/ace2007-with-lists.features\n";
	print PAR "pidf_tag_set_file:            %serif_data%/arabic/names/pidf/7Types-names.txt\n";
	print PAR "word_cluster_bits_file:       %serif_data%/arabic/clusters/cl2_tdt4_wl.hBits\n";
    } elsif ($language eq 'chinese') {
	print PAR "pidf_interleave_tags:         true\n";
	print PAR "pidf_tag_set_file:            %serif_data%/chinese/names/pidf/ace2004.tags\n";
	print PAR "pidf_features_file:           %serif_data%/chinese/names/pidf/bi_and_tri_wc.features\n";
	print PAR "word_cluster_bits_file:       %serif_data%/chinese/clusters/giga9804-tdt4bn.hBits\n";
    } 
    close(PAR);
}

print "\n\n";
print "******************************************************************\n";
print "******************************************************************\n";
print "Training preparation is finished. To train the models, please run:\n\n";
foreach my $version (@versions) {
    print "$delivery_root/name-retrainer/bin/$language/PIdFTrainer $expt/training$version.par\n";
}
print "\n\n";
print "Please ignore any \"truncating sentence\" messages.\n\n";
