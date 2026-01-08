import os
import subprocess
import re
from flask import Flask, render_template, jsonify

app = Flask(__name__)

DATA_FILE = os.path.abspath("data/social_network.txt")
ENGINE_BIN = os.path.abspath("build/dgraph_engine")

@app.route('/')
def index():
    return render_template('index.html')

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

@app.route('/api/run')
def run_engine():
    try:
        # Run the C++ engine
        result = subprocess.run([ENGINE_BIN, DATA_FILE], capture_output=True, text=True)
        if result.returncode != 0:
            return jsonify({"error": result.stderr}), 500
        
        output = result.stdout
        
        # Parse output
        # Expected format: "  V[0]: PR=2.1070, Community=1"
        results = {}
        
        for line in output.split('\n'):
            match = re.search(r'V\[(\d+)\]: PR=([0-9.]+), Community=(\d+)', line)
            if match:
                vid = match.group(1)
                pr = float(match.group(2))
                comm = int(match.group(3))
                results[vid] = {"pr": pr, "community": comm}
                
        return jsonify(results)
        
    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True, port=5000)
