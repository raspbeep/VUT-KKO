import statistics

# data = """512-cb2.raw          Baseline                  262144       89486        2.73       0.029s       0.013s      
# 512-cb2.raw          Model (-m)                262144       36013        1.09       0.130s       0.018s      
# 512-cb2.raw          Adaptive (-a)             262144       72464        2.21       0.070s       0.013s      
# 512-cb2.raw          Adaptive+Model (-a -m)    262144       45200        1.37       0.096s       0.015s      
# 512-cb.raw           Baseline                  262144       707          .02        0.227s       0.014s      
# 512-cb.raw           Model (-m)                262144       777          .02        0.477s       0.010s      
# 512-cb.raw           Adaptive (-a)             262144       6160         .18        0.050s       0.017s      
# 512-cb.raw           Adaptive+Model (-a -m)    262144       6736         .20        0.052s       0.026s      
# 512-df1h.raw         Baseline                  262144       262145       8.00       0.027s       0.012s      
# 512-df1h.raw         Model (-m)                262144       911          .02        0.630s       0.020s      
# 512-df1h.raw         Adaptive (-a)             262144       21136        .64        0.062s       0.030s      
# 512-df1h.raw         Adaptive+Model (-a -m)    262144       24520        .74        0.112s       0.015s      
# 512-df1hvx.raw       Baseline                  262144       36257        1.10       0.053s       0.010s      
# 512-df1hvx.raw       Model (-m)                262144       28794        .87        0.068s       0.016s      
# 512-df1hvx.raw       Adaptive (-a)             262144       52321        1.59       0.071s       0.014s      
# 512-df1hvx.raw       Adaptive+Model (-a -m)    262144       45333        1.38       0.089s       0.019s      
# 512-df1v.raw         Baseline                  262144       2958         .09        0.172s       0.013s      
# 512-df1v.raw         Model (-m)                262144       3022         .09        0.281s       0.013s      
# 512-df1v.raw         Adaptive (-a)             262144       21136        .64        0.062s       0.017s      
# 512-df1v.raw         Adaptive+Model (-a -m)    262144       24520        .74        0.075s       0.021s      
# 512-nk01.raw         Baseline                  262144       262145       8.00       0.028s       0.013s      
# 512-nk01.raw         Model (-m)                262144       262145       8.00       0.041s       0.006s      
# 512-nk01.raw         Adaptive (-a)             262144       262145       8.00       0.127s       0.010s      
# 512-nk01.raw         Adaptive+Model (-a -m)    262144       262145       8.00       0.105s       0.006s      
# 512-shp1.raw         Baseline                  262144       67709        2.06       0.065s       0.015s      
# 512-shp1.raw         Model (-m)                262144       39418        1.20       0.067s       0.019s      
# 512-shp1.raw         Adaptive (-a)             262144       66280        2.02       0.069s       0.013s      
# 512-shp1.raw         Adaptive+Model (-a -m)    262144       71560        2.18       0.064s       0.019s      
# 512-shp2.raw         Baseline                  262144       80976        2.47       0.077s       0.037s      
# 512-shp2.raw         Model (-m)                262144       54711        1.66       0.075s       0.020s      
# 512-shp2.raw         Adaptive (-a)             262144       62837        1.91       0.069s       0.019s      
# 512-shp2.raw         Adaptive+Model (-a -m)    262144       68074        2.07       0.085s       0.027s      
# 512-shp.raw          Baseline                  262144       3773         .11        0.131s       0.014s      
# 512-shp.raw          Model (-m)                262144       3962         .12        0.128s       0.011s      
# 512-shp.raw          Adaptive (-a)             262144       70288        2.14       0.078s       0.021s      
# 512-shp.raw          Adaptive+Model (-a -m)    262144       75920        2.31       0.071s       0.023s
# """

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
compression_times_baseline = statistics.mean([float(x.compression_time.strip('s')) for x in results['baseline']])
compression_times_adaptive = statistics.mean([float(x.compression_time.strip('s')) for x in results['adaptive']])
compression_times_model = statistics.mean([float(x.compression_time.strip('s')) for x in results['model']])
compression_times_adaptive_model = statistics.mean([float(x.compression_time.strip('s')) for x in results['adaptive_model']])

decompression_times_baseline = statistics.mean([float(x.decompression_time.strip('s')) for x in results['baseline']])
decompression_times_adaptive = statistics.mean([float(x.decompression_time.strip('s')) for x in results['adaptive']])
decompression_times_model = statistics.mean([float(x.decompression_time.strip('s')) for x in results['model']])
decompression_times_adaptive_model = statistics.mean([float(x.decompression_time.strip('s')) for x in results['adaptive_model']])

bpp_baseline = statistics.mean([float(x.bpp.strip('s')) for x in results['baseline']])
bpp_adaptive = statistics.mean([float(x.bpp.strip('s')) for x in results['adaptive']])
bpp_model = statistics.mean([float(x.bpp.strip('s')) for x in results['model']])
bpp_adaptive_model = statistics.mean([float(x.bpp.strip('s')) for x in results['adaptive_model']])

print(f"Compression time (s)           & {compression_times_baseline:.3f} & {compression_times_adaptive:.3f} & {compression_times_model:.3f} & {compression_times_adaptive_model:.3f} \\\\")
print(f"Decompression time (s)         & {decompression_times_baseline:.3f} & {decompression_times_adaptive:.3f} & {decompression_times_model:.3f} & {decompression_times_adaptive_model:.3f} \\\\")
print(f"Bits per pixel (bpp)       & {bpp_baseline:.3f} & {bpp_adaptive:.3f} & {bpp_model:.3f} & {bpp_adaptive_model:.3f} \\\\", end='')

print(table_3_footer)