#!/bin/bash
./t1.py -p test-benchmarks -o test-mqbfs-results -t 30 -P -x -S mqbfs -f 10 -W 1 -X count
./process_results.py -i test-mqbfs-results -o test-mqbfs-summary.csv
