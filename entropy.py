import math

files = [
    "benchmark/cb.raw",
    "benchmark/cb2.raw",
    "benchmark/df1h.raw",
    "benchmark/df1v.raw",
    "benchmark/df1hvx.raw",
    "benchmark/nk01.raw",
    "benchmark/shp.raw",
    "benchmark/shp1.raw",
    "benchmark/shp2.raw"
]

for file in files:
    with open(file, 'rb') as f:
        data = f.read()
        
        counts = [0] * 256
        for byte in data:
            counts[byte] += 1

        entropy = 0
        for count in counts:
            if count > 0:
                pi = count / len(data)
                log2pi = math.log2(pi)
                entropy += pi * log2pi
        entropy = -entropy
        print(f"Entropy of {file}: {entropy:.4f} bits per byte")
        