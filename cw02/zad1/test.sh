#!/bin/bash

for record_count in 2000 4000; do
	echo " === $record_count Records === "
	echo ""
	for record_size in 1 4 512 1024 4096 8196; do
		./main generate records "$record_count" "$record_size" > /dev/null 2>&1

		echo "== LIB == Copying $record_count records of size $record_size"
		./main copy records records_lib "$record_count" "$record_size" lib
		echo "== SYS == Copying $record_count records of size $record_size"
		./main copy records records_sys "$record_count" "$record_size" sys

		echo "== LIB == Sorting $record_count records of size $record_size"
		./main sort records_lib "$record_count" "$record_size" lib
		echo "== SYS == Sorting $record_count records of size $record_size"
		./main sort records_sys "$record_count" "$record_size" sys

		echo ""
	done
done

rm -f records records_lib records_sys
