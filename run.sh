#!/bin/bash

# run.sh

# --
# Prep graph and query

SIZE="medium"
PROBLEM="Subgraph.4.5"
python prep-query.py \
    --query-path ./_data/synthetic/queries/query.$PROBLEM.txt \
    --edge-path ./_data/synthetic/graphs/edges_$SIZE.tsv \
    --outdir ./_results/synthetic/$SIZE

# --
# Run

# Python
time python main.py \
    --query-path ./_results/synthetic/$SIZE/query.tsv \
    --edge-path ./_results/synthetic/$SIZE/edgelist.tsv

# C++
time ./cpp/main \
    ./_results/synthetic/$SIZE/query.tsv \
    ./_results/synthetic/$SIZE/edgelist.tsv \
    20