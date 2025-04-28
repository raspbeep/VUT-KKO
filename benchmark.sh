#!/bin/bash

# --- Default Settings ---
benchmark_dir="benchmark"
# Use find to get all .raw files in the benchmark directory
all_bench_files=$(find "$benchmark_dir" -name "*.raw" -print)
adaptive_flag=""
model_flag=""
run_all_flag=0 # Flag to indicate if --all was passed
# example additional args to pass to lz_codec
additional_params="--block_size=64"
#--offset_bits=8 --length_bits=10"

# --- Timing & Stats Variables ---
total_compress_time_sec=0.0
total_decompress_time_sec=0.0
successful_compress_count=0
successful_decompress_count=0
total_bits_used=0
total_original_size=0
successful_bpc_count=0
total_images_tested=0

# Function to parse time output (e.g., "real 0m1.234s") and return seconds
parse_time() {
  local time_output="$1"
  local real_time_str=$(echo "$time_output" | grep '^real' | awk '{print $2}')
  if [[ -z "$real_time_str" ]]; then
    echo "0.0"
    return
  fi
  real_time_str=${real_time_str%s}
  if [[ "$real_time_str" == *"m"* ]]; then
    local minutes=$(echo "$real_time_str" | cut -d'm' -f1)
    local seconds=$(echo "$real_time_str" | cut -d'm' -f2)
    echo "scale=4; $minutes * 60 + $seconds" | bc
  else
    echo "scale=4; $real_time_str" | bc
  fi
}

usage() {
  echo "Usage: $0 [--all] [-a] [-m] [input_file]"
  echo "  --all      : Benchmark all .raw files in the benchmark directory."
  echo "               If specified, [input_file] is ignored."
  echo "  -a         : Enable adaptive mode for lz_codec."
  echo "  -m         : Enable model mode for lz_codec."
  echo "  [input_file] : Optional. Specific file to benchmark if --all is not used."
  echo "                 If omitted and --all is not used, it will exit."
  exit 1
}

# --- Parse Command-Line Options ---
new_args=()
for arg in "$@"; do
  case $arg in
    --all)
      run_all_flag=1
      shift
      ;;
    *)
      new_args+=("$arg")
      ;;
  esac
done
set -- "${new_args[@]}"

while getopts "am" opt; do
  case $opt in
    a)
      adaptive_flag="-a"
      ;;
    m)
      model_flag="-m"
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      usage
      ;;
  esac
done
shift $((OPTIND-1))

# Determine files to process
files_to_process=()
if [ "$run_all_flag" -eq 1 ]; then
  # Split the newline-separated string into an array
  IFS=$'\n' read -r -d '' -a files_to_process <<< "$all_bench_files"
  echo "Benchmarking all .raw files in $benchmark_dir..."
  if [ $# -gt 0 ]; then
    echo "Warning: Input file '$1' ignored because --all was specified." >&2
  fi
else
  if [ $# -gt 0 ]; then
    files_to_process=("$1")
    echo "Benchmarking specified file: $1"
    shift
  else
    echo "Error: No input file specified and --all not used." >&2
    usage
  fi
  if [ $# -gt 0 ]; then
    echo "Error: Unexpected arguments: $@" >&2
    usage
  fi
fi

# Run make once
echo "Running make..."
make
if [ $? -ne 0 ]; then
  echo "Error: make failed!" >&2
  exit 1
fi
echo "Make successful."
echo "------------------------------------------------------"

# Main benchmarking loop
overall_status=0

for bench_filename in "${files_to_process[@]}"; do
  total_images_tested=$((total_images_tested + 1))
  echo "======================================================"
  echo "Benchmarking: $bench_filename"
  echo "======================================================"

  if [ ! -f "$bench_filename" ]; then
    echo "Error: Input file '$bench_filename' not found. Skipping." >&2
    overall_status=1
    continue
  fi

  # Extract number from filename (e.g., 736 from 736-pukeko.raw)
  filename_base=$(basename "$bench_filename")
  # Use regex to extract leading number
  if [[ "$filename_base" =~ ^([0-9]+) ]]; then
    size_param="${BASH_REMATCH[1]}"
  else
    echo "Warning: Could not extract size from filename '$filename_base'. Using default size 0."
    size_param=0
  fi

  # Prepare tmp directory
  rm -rf tmp
  mkdir tmp
  if [ $? -ne 0 ]; then
    echo "Error: Failed to create tmp directory for '$bench_filename'. Skipping." >&2
    overall_status=1
    continue
  fi

  echo "Running benchmark..."
  echo "------------------------------------------------------"
  # Pass size_param as -w argument
  compress_cmd="./build/lz_codec -c -i \"$bench_filename\" -o \"tmp/tmp.enc\" -w \"$size_param\" ${adaptive_flag:+"$adaptive_flag"}${model_flag:+" $model_flag"} ${additional_params:+" $additional_params"}"
  echo "$compress_cmd"

  # Capture time output
  compress_time_output=$((time eval "$compress_cmd") 2>&1)
  compress_status=$?
  echo "$compress_time_output"
  echo "------------------------------------------------------"

  if [ $compress_status -ne 0 ]; then
    echo "Error: Compression failed for '$bench_filename'!" >&2
    overall_status=1
    continue
  else
    compress_time_sec=$(parse_time "$compress_time_output")
    total_compress_time_sec=$(echo "scale=4; $total_compress_time_sec + $compress_time_sec" | bc)
    successful_compress_count=$((successful_compress_count + 1))
    echo "Compression time (real): ${compress_time_sec}s"
    echo "------------------------------------------------------"
  fi

  decompress_cmd="./build/lz_codec -d -i \"tmp/tmp.enc\" -o \"tmp/tmp.dec\""
  echo "$decompress_cmd"

  decompress_time_output=$((time eval "$decompress_cmd") 2>&1)
  decompress_status=$?
  echo "$decompress_time_output"
  echo "------------------------------------------------------"

  if [ $decompress_status -ne 0 ]; then
    echo "Error: Decompression failed for '$bench_filename'!" >&2
    overall_status=1
    continue
  else
    decompress_time_sec=$(parse_time "$decompress_time_output")
    total_decompress_time_sec=$(echo "scale=4; $total_decompress_time_sec + $decompress_time_sec" | bc)
    successful_decompress_count=$((successful_decompress_count + 1))
    echo "Decompression time (real): ${decompress_time_sec}s"
    echo "------------------------------------------------------"
  fi

  # Get original size
  original_size=$(stat -c %s "$bench_filename" 2>/dev/null) || original_size=0
  compressed_size=$(stat -c %s "tmp/tmp.enc" 2>/dev/null) || compressed_size=0

  echo "Original size: $original_size bytes"
  echo "Compressed size: $compressed_size bytes"

  if [ "$original_size" -gt 0 ] && [ "$compressed_size" -ge 0 ]; then
    sizes_valid=1
    space_saved=$(echo "scale=2; (1 - $compressed_size / $original_size) * 100" | bc)
    echo "Space saved: $space_saved%"
    bits_per_char=$(echo "scale=2; ($compressed_size * 8) / $original_size" | bc)
    echo "Bits per char: $bits_per_char"
  elif [ "$original_size" -eq 0 ]; then
    echo "Warning: Original size is zero. Cannot calculate ratios."
  else
    echo "Error: Could not determine file sizes accurately."
    overall_status=1
  fi

  # Verify correctness
  if ! cmp -s "$bench_filename" "tmp/tmp.dec"; then
    echo "Error: Files do not match for '$bench_filename'!" >&2
    overall_status=1
  else
    echo "Success: Files match for '$bench_filename'!"
    if [ "$sizes_valid" -eq 1 ]; then
      current_bits_used=$(echo "$compressed_size * 8" | bc)
      total_bits_used=$(echo "scale=0; $total_bits_used + $current_bits_used" | bc)
      total_original_size=$(echo "scale=0; $total_original_size + $original_size" | bc)
      successful_bpc_count=$((successful_bpc_count + 1))
    fi
  fi
done

# Summary
echo "======================================================"
echo "Benchmark Summary ($total_images_tested images)"
echo "======================================================"

if [ "$successful_compress_count" -gt 0 ]; then
  avg_compress_time=$(echo "scale=4; $total_compress_time_sec / $successful_compress_count" | bc)
  printf "Average Compression Time (real): %.4fs (%d/%d successful runs)\n" "$avg_compress_time" "$successful_compress_count" "$total_images_tested"
else
  echo "Average Compression Time (real): N/A (0/$total_images_tested successful runs)"
fi

if [ "$successful_decompress_count" -gt 0 ]; then
  avg_decompress_time=$(echo "scale=4; $total_decompress_time_sec / $successful_decompress_count" | bc)
  printf "Average Decompression Time (real): %.4fs (%d/%d successful runs)\n" "$avg_decompress_time" "$successful_decompress_count" "$total_images_tested"
else
  echo "Average Decompression Time (real): N/A (0/$total_images_tested successful runs)"
fi

if [ "$successful_bpc_count" -gt 0 ] && [ "$total_original_size" -gt 0 ]; then
  avg_bits_per_char=$(echo "scale=4; $total_bits_used / $total_original_size" | bc)
  printf "Average Bits Per Char:         %.4f (%d/%d successful runs)\n" "$avg_bits_per_char" "$successful_bpc_count" "$total_images_tested"
elif [ "$successful_bpc_count" -gt 0 ]; then
  echo "Average Bits Per Char:         N/A (Total original size was zero for successful runs)"
else
  echo "Average Bits Per Char:         N/A (0/$total_images_tested successful runs with valid sizes and matching files)"
fi

echo "------------------------------------------------------"
if [ "$overall_status" -eq 0 ]; then
  echo "All benchmarks completed successfully."
else
  echo "One or more benchmarks encountered errors." >&2
fi
echo "Benchmark finished."

exit $overall_status
