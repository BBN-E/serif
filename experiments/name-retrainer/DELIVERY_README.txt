Before delivery:

1) The executables in bin/ should be updated from source.

2) The data in data/ should be updated by running the train-namefinding.pl
   script in Active/Training/train-namefinding/sequences. Run with the
   following parameters:
   	     $expt_name = 'BBN';
	     $train_regtest_models = 0;
	     $create_training_files_only = 0; 
   Run once for each language. This will generate the .all.sexp files needed 
   for this delivery (without actually training the models from it). 
   Then encrypt them using PIdFTrainer and copy them into these directories. 
   (Look in the PIdFTrainer source code to discover the correct command to 
   run.) 
