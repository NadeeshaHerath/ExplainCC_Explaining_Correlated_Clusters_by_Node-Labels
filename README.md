This repository contains the implementation of EXPAND algorthm and scripts for synthetic data generation.

# Algorithms

## SBM_full

This is for graphs with no negative edges.This takes an edge file and a label file as input and outputs the cluster assignments and its tree.

Usage:

    g++ SBM_full.cpp -o SBMfulltree
    
    ./SBMfulltree -e <edge file> -l <label file> -o <node output file> -t <tree output file> -b <lambda>
    
    -h    print help
    -e    edge input file
    -l    label input file
    -o    node output file
    -t    tree output file
    -b    weight lambda value

Input file formatis:

  Edge file: Each line contains one edge.

 ` <node u> <node v>`
 
  Label file: Each line contains one node and one label.
  
  `<node id> <label id>`

## SBM_sparse

This is for graphs with positive and negative edges.This takes an positive edge file, negative edge file and a label file as input and outputs the cluster assignments and its tree.

Usage:

    g++ SBM_sparse.cpp -o SBMsparsetree
    
    ./SBMsparsetree -p <positive edge file> -n <negative edge file> -l <label file> -o <node output file> -t <tree output file> -b <lambda>
    
    -h    print help
    -p    positive edge input file
    -n    negative edge input file
    -l    label input file
    -o    node output file
    -t    tree output file
    -b    weight lambda value

Input file formatis:
    
Both positive and negative edges are stored in separate files and each line in both contains one edge.

` <node u> <node v>`
 
Label file: Each line contains one node and one label.
  
`<node id> <label id>`

In both SBM_full, SBM_sparse,
    
Output file formats:

Node output file: Each line represents one cluster.

`<cluster id> <node 1> <node 2>`

Tree output file: Each line represents one split in the generated tree.

`<parent node> <split label> <score> <left child> <right child>`


# Data Generation

## Synthetic datasets generation of SBM_full

Usage:

python SBM_full_data_generation.py

This generates edge files and label files.

## Synthetic datasets generation of SBM_sparse

Usage:

python SBM_sparse_data_generation.py

This generates positive edge files, negative edge files and label files.



