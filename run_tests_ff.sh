#!/bin/bash
./t1.py -p test-benchmarks -o test-ff-results -t 30 -P
./process_results.py -i test-ff-results -o test-ff-summary.csv
