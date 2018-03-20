#!/bin/bash

# fix.sh
#
# Scripts to go from UIUC format to something simpler

for SIZE in small medium large; do
    python fix-graph.py \
        --graph-path ./_data/synthetic/graphs/GT_$SIZE.txt \
        --type-path ./_data/synthetic/graphs/types_$SIZE.txt \
        --outpath ./_data/synthetic/graphs/edges_$SIZE.tsv
done

python fix-queries.py --indir ../_data/synthetic/queries