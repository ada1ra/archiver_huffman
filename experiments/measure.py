import os
import subprocess
import time
import statistics
import csv
import glob

HUFFMAN = './build/src/huffman'
FLAG_C = '-c'
FLAG_D = '-d'
N_RUNS = 10
WARMUP = 1

TMP_COMPRESSED = 'experiments/output/tmp.bin'
TMP_RESTORED   = 'experiments/output/tmp.restored'

def run(cmd):
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def measure_one(path):
    orig_size = os.path.getsize(path)

    for _ in range(WARMUP):
        run([HUFFMAN, FLAG_C, path, TMP_COMPRESSED])
        run([HUFFMAN, FLAG_D, TMP_COMPRESSED, TMP_RESTORED])

    compress_times = []
    decompress_times = []

    for _ in range(N_RUNS):
        t0 = time.perf_counter()
        run([HUFFMAN, FLAG_C, path, TMP_COMPRESSED])
        t1 = time.perf_counter()
        run([HUFFMAN, FLAG_D, TMP_COMPRESSED, TMP_RESTORED])
        t2 = time.perf_counter()

        compress_times.append(t1 - t0)
        decompress_times.append(t2 - t1)

    compressed_size = os.path.getsize(TMP_COMPRESSED)

    return {
        'file': path,
        'orig_size': orig_size,
        'compressed_size': compressed_size,
        'ratio_percent': 100.0 * (1 - compressed_size / orig_size) if orig_size else 0.0,
        'compress_mean_ms':   1000 * statistics.mean(compress_times),
        'compress_stdev_ms':  1000 * statistics.stdev(compress_times) if len(compress_times) > 1 else 0.0,
        'decompress_mean_ms': 1000 * statistics.mean(decompress_times),
        'decompress_stdev_ms':1000 * statistics.stdev(decompress_times) if len(decompress_times) > 1 else 0.0,
    }

def main():
    os.makedirs('experiments/output', exist_ok=True)
    files = sorted(glob.glob('experiments/input/*'))
    results = []

    for path in files:
        row = measure_one(path)
        results.append(row)
        print(f"{os.path.basename(path):30s} "
              f"{row['orig_size']:>9} → {row['compressed_size']:>9}  "
              f"({row['ratio_percent']:>6.2f}%)  "
              f"c: {row['compress_mean_ms']:>8.2f}±{row['compress_stdev_ms']:>6.2f} мс  "
              f"d: {row['decompress_mean_ms']:>8.2f}±{row['decompress_stdev_ms']:>6.2f} мс")

    with open('experiments/results.csv', 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=results[0].keys())
        writer.writeheader()
        writer.writerows(results)

    print('\nСохранено в experiments/results.csv')

if __name__ == '__main__':
    main()