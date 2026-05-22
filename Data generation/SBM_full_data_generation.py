import networkx as nx
import numpy as np
import random
import os

DATASET_DIR = "p_in_datasets"
os.makedirs(DATASET_DIR, exist_ok=True)

n_communities = 20
nodes_per_comm = 50
n_extra_nodes = 100 
num_noiselabels = 100

p_out = 0.01   
p_noise = 0.02 
noise_prob = 0.01

n_datasets_per_pin = 20


sizes = ([nodes_per_comm] * n_communities) + [n_extra_nodes]

ground_truth_file = os.path.join(DATASET_DIR, "ground_truth_assignments.txt")
with open(ground_truth_file, "w") as f:
    current_node = 0
    for block_id, size in enumerate(sizes):
        for _ in range(size):
            f.write(f"{current_node} {block_id}\n")
            current_node += 1

p_in_values = np.concatenate([np.arange(0.9, 0.09, -0.1),np.arange(0.09, 0.00, -0.01)])
for p_in in p_in_values:
    p_in = round(p_in,2)

    for j in range(n_datasets_per_pin):
        n_blocks = len(sizes)
        probs = np.full((n_blocks, n_blocks), p_out)
        for i in range(n_communities):
            probs[i, i] = p_in

        probs[n_blocks-1, :] = p_noise
        probs[:, n_blocks-1] = p_noise

        G = nx.stochastic_block_model(sizes, probs)
        
        labels ={n: set () for n in G.nodes}
        for n in G.nodes:
            block_id = G.nodes[n]['block']
            if block_id <n_communities:
                labels[n].add(block_id)
            noise_indices = np.where(np.random.rand(num_noiselabels) < noise_prob)[0]
            for idx in noise_indices:
                labels[n].add(100 + idx)

        edge_file_name = os.path.join(DATASET_DIR, f"unsigned_edges_pin_{p_in:.2f}_{j+1}.txt")
        label_file_name = os.path.join(DATASET_DIR, f"unsigned_labels_pin_{p_in:.2f}_{j+1}.txt")

        with open(edge_file_name, "w") as f:
            for u, v in G.edges():
                f.write(f"{u} {v}\n")

        with open(label_file_name, "w") as f:
            for node_id, label_set in labels.items():
                for l in sorted(list(label_set)):
                    f.write(f"{node_id} {l}\n")

    print(f"Generated: p_in = {p_in:.2f}")