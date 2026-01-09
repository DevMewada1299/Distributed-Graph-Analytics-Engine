import argparse
import os
from gensim.models import Word2Vec
import glob

def train_node2vec(walks_files, output_file, dimensions=128, window=5, min_count=1, workers=4):
    """
    Train Node2Vec embeddings using Gensim Word2Vec.
    """
    class WalkIterator:
        def __init__(self, file_patterns):
            self.files = glob.glob(file_patterns)
            
        def __iter__(self):
            for fname in self.files:
                with open(fname, 'r') as f:
                    for line in f:
                        yield line.strip().split()

    print(f"Reading walks from {walks_files}...")
    walks = WalkIterator(walks_files)
    
    print("Training Word2Vec model...")
    model = Word2Vec(sentences=walks, vector_size=dimensions, window=window, min_count=min_count, workers=workers, sg=1)
    
    print(f"Saving embeddings to {output_file}...")
    model.wv.save_word2vec_format(output_file)
    print("Done.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Train Node2Vec embeddings from random walks")
    parser.add_argument("--walks", type=str, default="walks_out_*.txt", help="Glob pattern for walk files")
    parser.add_argument("--output", type=str, default="embeddings.txt", help="Output file for embeddings")
    parser.add_argument("--dim", type=int, default=128, help="Embedding dimensions")
    
    args = parser.parse_args()
    
    train_node2vec(args.walks, args.output, args.dim)
