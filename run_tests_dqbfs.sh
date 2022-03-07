#!/bin/bash
./t1.py -p test-benchmarks -o test-results -t 180 -P -x -S dqbfs -f 10 -W 1 -X count
./process_results.py -i test-results -o test-summary.csv
