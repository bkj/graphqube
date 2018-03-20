#!/bin/bash

# run.sh

# --
# Prep graph and query

SIZE="medium"
PROBLEM="Subgraph.4.4"
python prep-query.py \
    --query-path ./_data/synthetic/queries/query.$PROBLEM.txt \
    --edge-path ./_data/synthetic/graphs/edges_$SIZE.tsv \
    --outdir ./_results/synthetic/$SIZE

# --
# Run

# python version (serial)
time python main.py \
    --query-path ./_results/synthetic/$SIZE/query.tsv \
    --edge-path ./_results/synthetic/$SIZE/edgelist.tsv

# c++ version (parallel)
time ./cpp/main \
    ./_results/synthetic/$SIZE/query.tsv \
    ./_results/synthetic/$SIZE/edgelist.tsv \
    20

g++ -std=c++11 -fopenmp main.cpp   -o main
valgrind --leak-check=full --track-origins=yes ./main \
    ../_results/synthetic/$SIZE/query.tsv \
    ../_results/synthetic/$SIZE/edgelist.tsv \
    20

./main \
    ../_results/synthetic/$SIZE/query.tsv \
    ../_results/synthetic/$SIZE/edgelist.tsv \
    20
