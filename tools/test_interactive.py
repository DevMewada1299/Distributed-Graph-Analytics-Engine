import requests
import json
import os

BASE_URL = "http://127.0.0.1:5001"

def test_upload():
    print("Testing /api/upload...")
    # Create a dummy graph file
    with open("test_graph.txt", "w") as f:
        f.write("3\n")
        f.write("0 1\n")
        f.write("1 2\n")
        f.write("2 0\n")
    
    files = {'file': open('test_graph.txt', 'rb')}
    try:
        r = requests.post(f"{BASE_URL}/api/upload", files=files)
        print("Status:", r.status_code)
        print("Response:", r.json())
        if r.status_code == 200 and r.json().get('success'):
            print("PASS: Upload successful")
        else:
            print("FAIL: Upload failed")
    except Exception as e:
        print(f"FAIL: Exception {e}")
    finally:
        files['file'].close()
        os.remove("test_graph.txt")

def test_interactive_run():
    print("\nTesting /api/run with dynamic data...")
    # Dynamic graph data: 0->1, 1->2 (Line graph)
    payload = {
        "nodes": [{"id": 0}, {"id": 1}, {"id": 2}],
        "edges": [
            {"from": 0, "to": 1},
            {"from": 1, "to": 2}
        ]
    }
    
    try:
        r = requests.post(f"{BASE_URL}/api/run", json=payload)
        print("Status:", r.status_code)
        print("Response:", r.json())
        
        # Check if results contain expected keys
        data = r.json()
        if '0' in data and '1' in data and '2' in data:
            print("PASS: Analysis ran on dynamic data")
        else:
            print("FAIL: Missing nodes in result")
            
    except Exception as e:
        print(f"FAIL: Exception {e}")

if __name__ == "__main__":
    test_upload()
    test_interactive_run()
