import os
import subprocess
import re
import json
from flask import Flask, render_template, jsonify, request

app = Flask(__name__)

DATA_FILE = os.path.abspath("data/social_network.txt")
ENGINE_BIN = os.path.abspath("build/dgraph_engine")

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/upload', methods=['POST'])
def upload_file():
    if 'file' not in request.files:
        return jsonify({"success": False, "error": "No file part"})
    
    file = request.files['file']
    if file.filename == '':
        return jsonify({"success": False, "error": "No selected file"})
        
    if file:
        try:
            # Save uploaded file
            filepath = os.path.join("data", "uploaded_graph.txt")
            file.save(filepath)
            
            # Update global DATA_FILE to point to uploaded file for subsequent runs
            # Note: In a real multi-user app, this should be session-based.
            global DATA_FILE
            DATA_FILE = os.path.abspath(filepath)
            
            # Parse it to return to UI
            nodes = set()
            edges = []
            with open(filepath, 'r') as f:
                lines = f.readlines()
                start_idx = 0
                if len(lines) > 0 and len(lines[0].split()) == 1:
                    start_idx = 1
                
                for line in lines[start_idx:]:
                    parts = line.strip().split()
                    if len(parts) >= 2:
                        src, dst = parts[0], parts[1]
                        nodes.add(src)
                        nodes.add(dst)
                        edges.append({"from": src, "to": dst})
            
            node_list = [{"id": n, "label": str(n)} for n in sorted(list(nodes))]
            return jsonify({"success": True, "nodes": node_list, "edges": edges})
        except Exception as e:
            return jsonify({"success": False, "error": str(e)})

@app.route('/api/graph')
def get_graph():
    nodes = set()
    edges = []
    
    if os.path.exists(DATA_FILE):
        with open(DATA_FILE, 'r') as f:
            # Skip first line if it's vertex count (simple heuristic)
            lines = f.readlines()
            start_idx = 0
            if len(lines) > 0 and len(lines[0].split()) == 1:
                start_idx = 1
            
            for line in lines[start_idx:]:
                parts = line.strip().split()
                if len(parts) >= 2:
                    src, dst = parts[0], parts[1]
                    nodes.add(src)
                    nodes.add(dst)
                    edges.append({"from": src, "to": dst})
    
    node_list = [{"id": n, "label": str(n)} for n in sorted(list(nodes))]
    return jsonify({"nodes": node_list, "edges": edges})

@app.route('/api/run', methods=['GET', 'POST'])
def run_engine():
    target_file = DATA_FILE
    
    try:
        # Check if we received dynamic graph data
        if request.method == 'POST':
            data = request.json
            if data and 'nodes' in data and 'edges' in data:
                # Write to temporary file
                target_file = os.path.abspath("data/interactive_graph.txt")
                
                unique_nodes = set()
                edge_lines = []
                for e in data['edges']:
                    src, dst = str(e['from']), str(e['to'])
                    unique_nodes.add(src)
                    unique_nodes.add(dst)
                    edge_lines.append(f"{src} {dst}")
                
                # Also ensure isolated nodes are accounted for in ID range
                for n in data['nodes']:
                    unique_nodes.add(str(n['id']))

                # Determine NumVertices (MaxID + 1)
                max_id = 0
                for n in unique_nodes:
                    try:
                        val = int(n)
                        if val > max_id: max_id = val
                    except:
                        pass # Ignore non-integer IDs
                
                with open(target_file, 'w') as f:
                    f.write(f"{max_id + 1}\n") # NumVertices
                    for line in edge_lines:
                        f.write(line + "\n")
        
        # Prepare Command
        algo = 'default'
        cmd_args = [ENGINE_BIN, target_file]
        
        if request.method == 'POST':
            data = request.json
            if data and 'algorithm' in data:
                algo = data['algorithm']
                cmd_args.append(algo)
                
                if algo == 'bfs':
                    source = data.get('sourceNode', '0')
                    cmd_args.append(str(source))
                elif algo == 'rw':
                    # Default: len=10, num=5
                    cmd_args.append("10")
                    cmd_args.append("5")

        # Run the C++ engine
        print(f"Executing: {' '.join(cmd_args)}")
        result = subprocess.run(cmd_args, capture_output=True, text=True)
        if result.returncode != 0:
            return jsonify({"error": result.stderr}), 500
        
        output = result.stdout
        
        # Parse output
        results = {}
        
        for line in output.split('\n'):
            # Default: V[ID]: PR=..., Community=...
            match_default = re.search(r'V\[(\d+)\]: PR=([0-9.]+), Community=(\d+)', line)
            if match_default:
                vid = match_default.group(1)
                results[vid] = {"pr": float(match_default.group(2)), "community": int(match_default.group(3))}
                continue

            # BFS: V[ID]: BFS_Dist=...
            match_bfs = re.search(r'V\[(\d+)\]: BFS_Dist=(INF|\d+)', line)
            if match_bfs:
                vid = match_bfs.group(1)
                dist_str = match_bfs.group(2)
                dist = -1 if dist_str == 'INF' else int(dist_str)
                results[vid] = {"bfs_dist": dist}
                continue

            # CC: V[ID]: CC_ID=...
            match_cc = re.search(r'V\[(\d+)\]: CC_ID=(\d+)', line)
            if match_cc:
                vid = match_cc.group(1)
                results[vid] = {"cc_id": int(match_cc.group(2))}
                continue
                
            # PR only
            match_pr = re.search(r'V\[(\d+)\]: PR=([0-9.]+)', line)
            if match_pr:
                vid = match_pr.group(1)
                if vid not in results: results[vid] = {}
                results[vid]["pr"] = float(match_pr.group(2))
                continue
            
            # LPA only
            match_lpa = re.search(r'V\[(\d+)\]: Community=(\d+)', line)
            if match_lpa:
                vid = match_lpa.group(1)
                if vid not in results: results[vid] = {}
                results[vid]["community"] = int(match_lpa.group(2))
                continue

        return jsonify(results)
        
    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True, port=5001)
