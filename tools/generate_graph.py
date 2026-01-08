import random
import sys

def generate_sbm_graph(num_communities=4, nodes_per_comm=15, p_intra=0.3, p_inter=0.01):
    total_nodes = num_communities * nodes_per_comm
    edges = []
    
    # Generate edges
    for i in range(total_nodes):
        comm_i = i // nodes_per_comm
        for j in range(i + 1, total_nodes):
            comm_j = j // nodes_per_comm
            
            prob = p_intra if comm_i == comm_j else p_inter
            
            if random.random() < prob:
                edges.append((i, j))
                edges.append((j, i)) # Undirected for simplicity
    
    return total_nodes, edges

def main():
    if len(sys.argv) < 2:
        print("Usage: python generate_graph.py <output_file>")
        sys.exit(1)
        
    output_file = sys.argv[1]
    
    # Parameters for a nice visualization:
    # 4 communities, 15 nodes each = 60 nodes
    # High internal density, low external density
    num_nodes, edges = generate_sbm_graph(num_communities=4, nodes_per_comm=15, p_intra=0.25, p_inter=0.005)
    
    # Add a few "influencers" (hub nodes)
    # Pick one node from each community and connect it to many others in same community
    for c in range(4):
        hub = c * 15
        for i in range(15):
            target = c * 15 + i
            if hub != target and random.random() < 0.5:
                edges.append((hub, target))
                edges.append((target, hub))
                
    # Remove duplicates
    edges = sorted(list(set(edges)))
    
    with open(output_file, 'w') as f:
        f.write(f"{num_nodes}\n")
        for u, v in edges:
            f.write(f"{u} {v}\n")
            
    print(f"Generated graph with {num_nodes} nodes and {len(edges)} edges to {output_file}")

if __name__ == "__main__":
    main()
