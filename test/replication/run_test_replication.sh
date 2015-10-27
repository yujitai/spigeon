#!/bin/bash
#make && log_to_stderr=1 ./test_db 2>&1 | grep -v -P '(--|==|RUN|OK|PASSED)' | logql -c 'select *'

make && log_to_stderr=1 ./test_replication

#tmp_file="tmp_result"
#mv $tmp_file $tmp_file.bak
#make && log_to_stderr=1 ./test_db &> $tmp_file | grep -v -P '^[A-Z]'
#patten="^[A-Z]"
#cat $tmp_file | grep -P "$pattern" | logql -c 'select *'
#cat $tmp_file | grep -v -P "$pattern"

