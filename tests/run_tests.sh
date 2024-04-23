#!/bin/bash

# Description: Run all tests
make clean
make ppcbc
make ppcbs

rm -rf ./results
rm -rf ./logs
mkdir results
mkdir logs
mkdir logs/server
mkdir logs/client

#python3 ./tests/generateFiles.py  1000 tests/files/1K.txt
#python3 ./tests/generateFiles.py  10000 tests/files/10K.txt
#python3 ./tests/generateFiles.py  100000 tests/files/100K.txt
#python3 ./tests/generateFiles.py  1000000 tests/files/1M.txt
#python3 ./tests/generateFiles.py  10000000 tests/files/10M.txt
#python3 ./tests/generateFiles.py  100000000 tests/files/100M.txt
#python3 ./tests/generateFiles.py  1000000000 tests/files/1G.txt

killall ppcbs

#./tests/basic/basic_test.sh
./tests/concurrency/concurrent_basic.sh
