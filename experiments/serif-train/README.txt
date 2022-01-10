First, make sure the $BBN_CONTROL variable in the main sequence file
(serif-train.serif.train.pl) is set to the directory where this experiment 
resides ($SERIF_DIR/experiments).

Then use the runjobs copy command on the sequence file 
sequences/serif-train.serif.train.pl to move the serif-train directory to a 
new location.

perl serif-train.serif.train.pl -copy *jobnumber* *directory*

Check the four variables: $exp, $BBN_CONTROL, $OTHER_QUEUE, and $QUEUE_PRIO in 
in *jobnumber*.serif.train.pl to make sure they are set appropriately for the 
runjobs setup. $BBN_CONTROL should be set to the directory that the 
serif-train directory was copied to. (Though "serif-train" was likely renamed 
to a job number.)

Organize training data into directories of sgm files and apf files. Training 
files should be called DOCID.<sgmsuffix> and DOCID.<apfsuffix>. <sgmsuffix> 
and <apfsuffix> can be specified in sequences/*jobnumber*.serif.train.defines.pl

Modify the ***SERIF-DIR***, ***SERIF-BUILD-DIR***,
***CA-SERIF-BUILD-DIR***, ***SGM-DIR***, ***APF-DIR*** and
***LANGUAGE*** variables in *jobnumber*.serif.train.defines.pl to
specify the top level parameters of the training sequence. Parameters
for the individual training runs may be adjusted by modifying the .par
files in the templates directory. 

Before running the sequence, make sure the file permissions are set correctly.
All experiment scripts should have 755 permissions settings and templates 
should have 644 permissions.

Any file referred to by the .par files must not contain relative paths. Often, 
the .par files with "pidf" in their names refer to a features file that 
contains relative paths. Also the alternative-relation-model-file parameter, 
found in relation training .par files will often contain relative paths. 

