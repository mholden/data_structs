#!/bin/bash

RESULTS_FILE="data_struct_results.txt"

function test_structs(){
	local SIZE=$1

	./key_val "${SIZE}" >> "${RESULTS_FILE}"
}

function clear_file(){
	cat /dev/null > "${RESULTS_FILE}"
}

clear_file

test_structs "1024"
test_structs "2048"
test_structs "4096"
test_structs "8192"
test_structs "16384"
test_structs "32768"
test_structs "65536"

exit 0
