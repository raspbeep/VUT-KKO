import statistics
import sys
data = sys.stdin.read()


data = data.strip().split('\n')
data = [line.split() for line in data]


for line in data:
    for i in range(len(line)):
        if line[i] == '(-a':
            line[i] = '(-a, -m)'
            del line[i+1]
            break

class TestResult:
    def __init__(self, file_name, original_size, compressed_size, bpp, compression_time, decompression_time):
        self.file_name = file_name
        self.original_size = original_size
        self.compressed_size = compressed_size
        self.bpp = bpp
        self.compression_time = compression_time
        self.decompression_time = decompression_time
    def __repr__(self):
        return f"TestResult({self.file_name}, {self.original_size}, {self.compressed_size}, {self.bpp}, {self.compression_time}, {self.decompression_time})"

results = {
    "baseline": [],
    "adaptive": [],
    "model": [],
    "adaptive_model": []
}

for line in data:
    if line[1] == 'Baseline':
        results['baseline'].append(TestResult(line[0], line[2], line[3], line[4], line[5], line[6]))
    elif line[1] == "Adaptive":
        results['adaptive'].append(TestResult(line[0], line[3], line[4], line[5], line[6], line[7]))
    elif line[1] == "Model":
        results['model'].append(TestResult(line[0], line[3], line[4], line[5], line[6], line[7]))
    elif line[1] == "Adaptive+Model":
        results['adaptive_model'].append(TestResult(line[0], line[3], line[4], line[5], line[6], line[7]))


table_1_header = """
%% table 1
\\begin{table}[h!]
\\centering
\\caption{Benchmark Results: Baseline vs. Adaptive (-a)}
\\label{tab:baseline_adaptive_script}
\\begin{tabular}{lrrrrrr}
\\toprule
          & \\multicolumn{3}{c}{Baseline} & \\multicolumn{3}{c}{Adaptive (-a)} \\\\
\\cmidrule(lr){2-4} \\cmidrule(lr){5-7}
File        & Comp. (s) & Decomp (s) & bpp  & Comp. (s) & Decomp (s) & bpp  \\\\
\\midrule
"""

table_1_footer = """
\\bottomrule
\\end{tabular}
\\end{table}
"""

file_names = sorted([x.file_name.split('-')[1] for x in results['baseline']])

print(table_1_header, end='')
for idx, file_name in enumerate(file_names):
    # find the restresult in the list of results
    baseline_result = next((x for x in results['baseline'] if x.file_name.split('-')[1] == file_name), None)
    adaptive_result = next((x for x in results['adaptive'] if x.file_name.split('-')[1] == file_name), None)

    print(f"{file_name} &  {baseline_result.compression_time} &  {baseline_result.decompression_time}  & {baseline_result.bpp} &  {adaptive_result.compression_time} & {adaptive_result.decompression_time} & {adaptive_result.bpp} \\\\", end='' if idx==len(file_names)-1 else '\n')
print(table_1_footer)


table_2_header = """
%% table 2
\\begin{table}[h!]
\\centering
\\caption{Benchmark Results: Model (-m) vs. Adaptive\\,+\\,Model (-a -m)}
\\label{tab:model_adaptive_model_script}
\\begin{tabular}{lrrrrrr}
\\toprule
          & \\multicolumn{3}{c}{Model (-m)} & \\multicolumn{3}{c}{Adaptive\\,+\\,Model (-a -m)} \\\\
\\cmidrule(lr){2-4} \\cmidrule(lr){5-7}
File        & Comp. (s) & Decomp (s) & bpp  & Comp. (s) & Decomp (s) & bpp  \\\\
\\midrule
"""

table_2_footer = """
\\bottomrule
\\end{tabular}
\\end{table}
"""

print(table_2_header, end='')
for idx, file_name in enumerate(file_names):
    # find the restresult in the list of results
    model_result = next((x for x in results['model'] if x.file_name.split('-')[1] == file_name), None)
    adaptive_model_result = next((x for x in results['adaptive_model'] if x.file_name.split('-')[1] == file_name), None)
    print(f"{file_name} &  {model_result.compression_time} &  {model_result.decompression_time}  & {model_result.bpp} &  {adaptive_model_result.compression_time} & {adaptive_model_result.decompression_time} & {adaptive_model_result.bpp} \\\\", end='' if idx==len(file_names)-1 else '\n')
print(table_2_footer)


table_3_header = """
%% table 3
\\begin{table}[h!]
\\centering
\\caption{Comparison of different configurations}
\\label{tab:compression_comparison_script}
\\begin{tabular}{lrrrr}
\\toprule
& Baseline & Adaptive & Model  & \\text{Adaptive\\,+\\,Model} \\\\
\\midrule
"""

table_3_footer = """
\\bottomrule
\\end{tabular}
\\end{table}
"""

print(table_3_header, end='')

compression_times_baseline = [float(x.compression_time.strip('s')) for x in results['baseline']]
compression_times_adaptive = [float(x.compression_time.strip('s')) for x in results['adaptive']]
compression_times_model = [float(x.compression_time.strip('s')) for x in results['model']]
compression_times_adaptive_model = [float(x.compression_time.strip('s')) for x in results['adaptive_model']]


compression_times_baseline_avg = statistics.mean(compression_times_baseline)
compression_times_adaptive_avg = statistics.mean(compression_times_adaptive)
compression_times_model_avg = statistics.mean(compression_times_model)
compression_times_adaptive_model_avg = statistics.mean(compression_times_adaptive_model)


decompression_times_baseline = [float(x.decompression_time.strip('s')) for x in results['baseline']]
decompression_times_adaptive = [float(x.decompression_time.strip('s')) for x in results['adaptive']]
decompression_times_model = [float(x.decompression_time.strip('s')) for x in results['model']]
decompression_times_adaptive_model = [float(x.decompression_time.strip('s')) for x in results['adaptive_model']]

decompression_times_baseline_avg = statistics.mean(decompression_times_baseline)
decompression_times_adaptive_avg = statistics.mean(decompression_times_adaptive)
decompression_times_model_avg = statistics.mean(decompression_times_model)
decompression_times_adaptive_model_avg = statistics.mean(decompression_times_adaptive_model)

bpp_baseline = statistics.mean([float(x.bpp.strip('s')) for x in results['baseline']])
bpp_adaptive = statistics.mean([float(x.bpp.strip('s')) for x in results['adaptive']])
bpp_model = statistics.mean([float(x.bpp.strip('s')) for x in results['model']])
bpp_adaptive_model = statistics.mean([float(x.bpp.strip('s')) for x in results['adaptive_model']])

print(f"Compression time average (s)           & {compression_times_baseline_avg:.3f} & {compression_times_adaptive_avg:.3f} & {compression_times_model_avg:.3f} & {compression_times_adaptive_model_avg:.3f} \\\\")
print(f"Decompression time average (s)         & {decompression_times_baseline_avg:.3f} & {decompression_times_adaptive_avg:.3f} & {decompression_times_model_avg:.3f} & {decompression_times_adaptive_model_avg:.3f} \\\\")
print(f"Bits per pixel average (bpp)       & {bpp_baseline:.3f} & {bpp_adaptive:.3f} & {bpp_model:.3f} & {bpp_adaptive_model:.3f} \\\\", end='')

print(table_3_footer)

average_bpp = (bpp_baseline + bpp_adaptive + bpp_model + bpp_adaptive_model) / 4
cumtime = sum(compression_times_baseline) + sum(compression_times_adaptive) + sum(compression_times_model) + sum(compression_times_adaptive_model)
average_comp_time = (compression_times_baseline_avg + compression_times_adaptive_avg + compression_times_model_avg + compression_times_adaptive_model_avg) / 4
print(f"average bpp: {average_bpp:.3f}")
print(f"average compression time: {average_comp_time:.3f}s")
print(f"total time: {cumtime:.3f}s")


total_original_size_B = sum([int(x.original_size) for x in results['baseline']]) + sum([int(x.original_size) for x in results['adaptive']]) + sum([int(x.original_size) for x in results['model']]) + sum([int(x.original_size) for x in results['adaptive_model']])
total_original_size_MB = total_original_size_B / 1000_000
thoughput = total_original_size_MB / cumtime
print(f"thoughput: {thoughput:.3f}MB/s")

