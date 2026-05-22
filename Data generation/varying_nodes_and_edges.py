import networkx as nx
import numpy as np
import os

DATASET_DIR = "varying_nodes_datasets"
os.makedirs(DATASET_DIR, exist_ok=True)

n_communities = 20


p_in = 0.6
p_out = 0.01
p_noise = 0.02
noise_prob = 0.01

node_counts = list(range(1000, 26000, 1000))
n_runs_per_size = 20

summary_file = os.path.join(DATASET_DIR, "dataset_summary.txt")
with open(summary_file, "w") as summary:
    summary.write("node_size_id run_id target_total_nodes actual_total_nodes nodes_per_community extra_noise_nodes generated_edges edge_file label_file ground_truth_file\n")

    for size_id, total_nodes in enumerate(node_counts, start=1):
        nodes_per_comm = total_nodes // 22
        extra_nodes = total_nodes - (n_communities * nodes_per_comm)
        sizes = ([nodes_per_comm] * n_communities) + [extra_nodes]
        actual_total_nodes = sum(sizes)
        n_blocks = len(sizes)
        num_noiselabels = int(total_nodes * 0.10)
        probs = np.full((n_blocks, n_blocks), p_out)
        for i in range(n_communities):
            probs[i, i] = p_in
        probs[n_blocks - 1, :] = p_noise
        probs[:, n_blocks - 1] = p_noise

        for run_id in range(1, n_runs_per_size + 1):

            print("-" * 70)
            print(f"Node size ID      : {size_id}")
            print(f"Run ID            : {run_id}")
            print(f"Target nodes      : {total_nodes}")
            print(f"Actual nodes      : {actual_total_nodes}")
            print(f"Nodes/community   : {nodes_per_comm}")
            print(f"Extra/noise nodes : {extra_nodes}")

            G = nx.stochastic_block_model(sizes,probs)
            labels = {n: set() for n in G.nodes}
            for n in G.nodes:
                block_id = G.nodes[n]["block"]
                if block_id < n_communities:
                    labels[n].add(block_id)

                noise_indices = np.where(np.random.rand(num_noiselabels) < noise_prob)[0]
                for idx in noise_indices:
                    labels[n].add(100 + idx)

            edge_file_name = os.path.join(DATASET_DIR,f"unsigned_edges_nodes_{actual_total_nodes}_run_{run_id:02d}.txt")
            label_file_name = os.path.join(DATASET_DIR,f"unsigned_labels_nodes_{actual_total_nodes}_run_{run_id:02d}.txt")
            with open(edge_file_name, "w") as f:
                for u, v in G.edges():
                    f.write(f"{u} {v}\n")
            with open(label_file_name, "w") as f:
                for node_id, label_set in labels.items():
                    for l in sorted(label_set):
                        f.write(f"{node_id} {l}\n")
            generated_edges = G.number_of_edges()
            summary.write(f"{size_id} {run_id} {total_nodes} {actual_total_nodes} {nodes_per_comm} {extra_nodes} {generated_edges} {edge_file_name} {label_file_name}\n")
            print(f"Generated edges   : {generated_edges}")
            print(f"Generated nodes   : {G.number_of_nodes()}")



print("\nAll datasets.")
print(f"Summary saved to: {summary_file}")