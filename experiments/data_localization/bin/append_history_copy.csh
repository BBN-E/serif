#!/bin/tcsh -vx

set name = $1
set history_file = $2
set max_history_lines = $3

echo $name >> $history_file
set history_lines = `wc -l $history_file | grep -E -o "[0-9]+"`
#echo "history lines: '$history_lines'\n"
if ($history_lines > $max_history_lines) then
	tail -$max_history_lines $history_file > tmp_file
	chmod 777 tmp_file
	mv tmp_file $history_file
endif
