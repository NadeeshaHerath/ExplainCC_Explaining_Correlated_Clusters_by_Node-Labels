import networkx as nx
import numpy as np
import random
import os

DATASET_DIR = "SBM_sparse_datasets"
os.makedirs(DATASET_DIR, exist_ok=True)

n_communities = 20
nodes_per_comm = 50
n_extra_nodes = 100

p_in = p_out = 0.6  
p_noise = 0.02 
q_out=0.2
num_noiselabels = 100  
noise_prob = 0.01

n_datasets_per_q_in = 20
sizes = ([nodes_per_comm] * n_communities) + [n_extra_nodes]

ground_truth_file = os.path.join(DATASET_DIR, "ground_truth_assignments.txt")
with open(ground_truth_file, "w") as f:
    current_node = 0
    for block_id, size in enumerate(sizes):
        for _ in range(size):
            f.write(f"{current_node} {block_id}\n")
            current_node += 1

q_in_values = np.concatenate([np.arange(0.9, 0.09, -0.1),np.arange(0.09, 0.00, -0.01)])
for q_in in q_in_values:
    q_in = round(q_in,2)
    
    for j in range(n_datasets_per_q_in):
        n_blocks = len(sizes)
        probs = np.full((n_blocks, n_blocks), p_out)
        for i in range(n_communities):
            probs[i, i] = p_in

        probs[n_blocks-1, :] = p_noise
        probs[:, n_blocks-1] = p_noise

        G = nx.stochastic_block_model(sizes, probs)

        labels = {n: set() for n in G.nodes}
        for n in G.nodes:
            block_id = G.nodes[n]['block']
            if block_id <n_communities:
                labels[n].add(block_id)
            noise_indices = np.where(np.random.rand(num_noiselabels) < noise_prob)[0]
            for idx in noise_indices:
                labels[n].add(100 + idx)

        pos_edges = []
        neg_edges = []

        for u, v in G.edges():
            block_u = G.nodes[u]['block']
            block_v = G.nodes[v]['block']
        
            is_internal = (block_u == block_v and block_u < n_communities)
        
            rand_num = random.random()
            if is_internal:
                sign = 1 if rand_num < q_in else -1
            else:
                sign = 1 if rand_num < q_out else -1
        
            if sign == 1:
                pos_edges.append((u, v))
            else:
                neg_edges.append((u, v))
    
        pos_edge_file_name = os.path.join(DATASET_DIR,f"pos_edges_q_in_{q_in:.2f}_{j+1}.txt")
        neg_edge_file_name = os.path.join(DATASET_DIR, f"neg_edges_q_in_{q_in:.2f}_{j+1}.txt")
        signed_label_file_name = os.path.join(DATASET_DIR, f"signed_labels_q_in_{q_in:.2f}_{j+1}.txt")

        with open(pos_edge_file_name, "w") as f:
            for u, v in pos_edges:
                f.write(f"{u} {v}\n")

        with open(neg_edge_file_name , "w") as f:
            for u, v in neg_edges:
                f.write(f"{u} {v}\n")

        with open(signed_label_file_name, "w") as f:
            for node_id, label_set in labels.items():
                for l in sorted(list(label_set)):
                    f.write(f"{node_id} {l}\n")

    print(f"Generated files for q_in = {q_in:.2f}")