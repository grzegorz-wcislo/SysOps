#!/bin/bash

function test_suite {
	echo ""
	for i in {1..20}; do printf "-" ; done    
	echo "" && echo "$1"
	for i in {1..20}; do printf "-" ; done    

	echo "" && echo "-- Big search --"
	"$2" 1 s "/" "*" "/tmp/temp" d 0
	echo "" && echo "-- Medium search --"
	"$2" 1 s "/usr" "*" "/tmp/temp" d 0
	echo "" && echo "-- Small search --"
	"$2" 1 s "/usr/share" "*" "/tmp/temp" d 0
	echo "" && echo "-- Alternating alocation and deletion --"
	"$2" 1 a 100000 "/etc/passwd"
}

test_suite "Static" "./main_static"
test_suite "Shared" "./main_shared"
test_suite "Dynamic" "./main_dynamic"
