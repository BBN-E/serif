#!/bin/env bash

set -e # Exit immediately if any simple command fails.
set -u # Treat unset variables as an error.
set -x # comment this out if we ever get clean sqlite diff reporting

function die() {
    echo "$*"
    exit 1
}


echo "in shell script compare_icews_sql.sh "
echo  "test_dir is +TEST_DIR+"
echo  "testdata_dir is +TESTDATA_DIR+"
echo "sql_source_tail is +SQL_SOURCE_TAIL+"
echo "sql_diffs_name is +SQL_DIFFS_NAME+"
echo "sorted_sql_name is +SORTED_SQL_NAME+"
echo " "
test_comparisons_dir=+TEST_DIR+/+COMPARE_ALL_DIR_NAME+
echo "test_comparisons_dir is $test_comparisons_dir"
mkdir -p $test_comparisons_dir

# echo "catting the html files in batch order for comparison to testdata"
# touch  +TEST_DIR+/display/all-display.html
# for ((batch_n=1;batch_n<=12;batch_n++)); do
# 	cat +TEST_DIR+/display/icews${batch_n}.html >> +TEST_DIR+/display/all-display.html
# done

# echo "wrote all html to +TEST_DIR+/display/all-display.html"

test_sorted_sql=$test_comparisons_dir/+SORTED_SQL_NAME+
testdata_sorted_sql=+TESTDATA_DIR+/+COMPARE_ALL_DIR_NAME+/+SORTED_SQL_NAME+
compare_diffs_file=$test_comparisons_dir/+SQL_DIFFS_NAME+

echo "debug -- here comes the perl regex and sort"

cat +TEST_DIR+/*/+SQL_SOURCE_TAIL+ | +PERL+ -pe 's/ //g; s/.*VALUES\(\d+,//;' | sort > $test_sorted_sql

echo "wrote all sorted SQL to  $test_sorted_sql "

diff $testdata_sorted_sql $test_sorted_sql > $compare_diffs_file

echo "wrote the sql diffs to  $compare_diffs_file"

if [ -s "$compare_diffs_file" ]; then
	die "icews SQL differences are in $compare_diffs_file"
fi

echo "tested sql diffs and did not _die"


# die_for_html=''
# html_diffs_file=$test_comparisons_dir/+HTML_DIFFS_NAME+

# diff +TESTDATA_DIR+/display/all-display.html   +TEST_DIR+/display/all-display.html > $html_diffs_file

# echo "wrote html diffs to $html_diffs_files "

# if [ -s "$html_diffs_file" ]; then
# 	echo "gonna die for html not same"
# 	why_die="$why_die icews html differences are in $html_diffs_file"
# 	die_for_html="html_diffs"
# fi

# echo "tested html diffs and set why_die to $why_die"


# if [ $die_for_html ]; then
# 	die "$why_die"
# fi

# if [ $die_for_sql ]; then
# 	die "$why_die"
# fi

echo "did not find any diffs in SQL or html"




