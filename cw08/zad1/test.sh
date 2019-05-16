#!/bin/bash

test_thread() {
    echo "=="
    echo "== $2 threads =="
    ./gimp "$2" "$1" images/lena512.pgm filters/17 test-output.pgm
}

test_variant() {
    echo "==="
    echo "=== $1 ==="
    test_thread "$1" 1
    test_thread "$1" 2
    test_thread "$1" 4
    test_thread "$1" 8
}

test_variant "block"
test_variant "interleaved"
