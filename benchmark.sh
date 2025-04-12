#!/bin/bash

# --- Default Settings ---
default_bench_filename="benchmark/shp1.raw"
# List of files to benchmark when --all is used
all_bench_files=(
    "benchmark/cb.raw"
    "benchmark/cb2.raw"
    "benchmark/df1h.raw"
    "benchmark/df1v.raw"
    "benchmark/df1hvx.raw"
    "benchmark/nk01.raw"
    "benchmark/shp.raw"
    "benchmark/shp1.raw"
    "benchmark/shp2.raw"
    "benchmark/512x250.raw"
)
size=512
adaptive_flag=""
model_flag=""
run_all_flag=0 # Flag to indicate if --all was passed

# --- Timing & Stats Variables ---
total_compress_time_sec=0.0
total_decompress_time_sec=0.0
successful_compress_count=0
successful_decompress_count=0
total_bits_used=0
total_original_size=0
successful_bpc_count=0 # Count runs successful for BPC calculation

# Function to parse time output (e.g., "real 0m1.234s") and return seconds
parse_time() {
    local time_output="$1"
    # Extract the line containing 'real', then the time value (e.g., 0m1.234s)
    local real_time_str=$(echo "$time_output" | grep '^real' | awk '{print $2}')
    if [[ -z "$real_time_str" ]]; then
        echo "0.0" # Could not parse
        return
    fi
    # Remove trailing 's'
    real_time_str=${real_time_str%s}
    # Check if 'm' (minutes) is present
    if [[ "$real_time_str" == *"m"* ]]; then
        local minutes=$(echo "$real_time_str" | cut -d'm' -f1)
        local seconds=$(echo "$real_time_str" | cut -d'm' -f2)
        echo "scale=4; $minutes * 60 + $seconds" | bc
    else
        # Only seconds present
        echo "scale=4; $real_time_str" | bc
    fi
}

usage() {
  echo "Usage: $0 [--all] [-a] [-m] [input_file]"
  echo "  --all      : Benchmark all predefined files (${all_bench_files[*]})."
  echo "               If specified, [input_file] is ignored."
  echo "  -a         : Enable adaptive mode for lz_codec."
  echo "  -m         : Enable model mode for lz_codec."
  echo "  [input_file] : Optional. Specific file to benchmark if --all is not used."
  echo "                 Defaults to '$default_bench_filename' if omitted."
  exit 1
}

# --- Parse Command-Line Options ---
# Manual parsing for --all first, as getopt doesn't handle long options easily
new_args=()
for arg in "$@"; do
  case $arg in
    --all)
      run_all_flag=1
      shift # Remove --all from arguments
      ;;
    *)
      new_args+=("$arg") # Keep other args
      ;;
  esac
done
# Reset positional parameters for getopt
set -- "${new_args[@]}"

# Parse short options using getopt
while getopts "am" opt; do
  case $opt in
    a)
      adaptive_flag="-a"
      ;;
    m)
      model_flag="-m"
      ;;
    \?) # Handle invalid options
      echo "Invalid option: -$OPTARG" >&2
      usage
      ;;
  esac
done
shift $((OPTIND-1))

# Determine which files to process
files_to_process=()
if [ "$run_all_flag" -eq 1 ]; then
  files_to_process=("${all_bench_files[@]}")
  echo "Benchmarking all predefined files..."
  if [ $# -gt 0 ]; then
      echo "Warning: Input file '$1' ignored because --all was specified." >&2
  fi
else
  # If --all is not used, check for a positional argument (input file)
  if [ $# -gt 0 ]; then
    files_to_process=("$1")
    echo "Benchmarking specified file: $1"
    shift # Consume the filename argument
  else
    # Otherwise, use the default
    files_to_process=("$default_bench_filename")
    echo "Benchmarking default file: $default_bench_filename"
  fi
  # Check for any remaining unexpected arguments
  if [ $# -gt 0 ]; then
      echo "Error: Unexpected arguments: $@" >&2
      usage
  fi
fi


# --- Build Step (only once) ---
echo "Running make..."
make
if [ $? -ne 0 ]; then
    echo "Error: make failed!" >&2
    exit 1
fi
echo "Make successful."
echo "------------------------------------------------------"


# --- Main Benchmark Loop ---
overall_status=0 # Track if any benchmark failed

for bench_filename in "${files_to_process[@]}"; do
    echo "======================================================"
    echo "Benchmarking: $bench_filename"
    echo "======================================================"

    if [ ! -f "$bench_filename" ]; then
        echo "Error: Input file '$bench_filename' not found. Skipping." >&2
        overall_status=1 # Mark as failed
        continue # Skip to the next file
    fi

    # Clean and create tmp directory for each file
    rm -rf tmp
    mkdir tmp
    if [ $? -ne 0 ]; then
        echo "Error: Failed to create tmp directory for '$bench_filename'. Skipping." >&2
        overall_status=1
        continue
    fi

    echo "Running benchmark..."
    echo "------------------------------------------------------"
    compress_cmd="./build/lz_codec -c -i \"$bench_filename\" -o \"tmp/tmp.enc\" -w \"$size\" ${adaptive_flag:+"$adaptive_flag"}${model_flag:+" $model_flag"}"
    echo "$compress_cmd"

    # Capture time output (stderr) into a variable
    # Use a subshell (...) and redirect stderr (2>&1) to stdout for capture
    compress_time_output=$( ( time eval "$compress_cmd" ) 2>&1 )
    compress_status=$?
    # Print the command's stdout/stderr (which is now in the variable)
    echo "$compress_time_output"
    echo "------------------------------------------------------"

    if [ $compress_status -ne 0 ]; then
        echo "Error: Compression failed for '$bench_filename'!" >&2
        overall_status=1
        continue # Skip to the next file
    else
        # Parse time and add to total if successful
        compress_time_sec=$(parse_time "$compress_time_output")
        total_compress_time_sec=$(echo "scale=4; $total_compress_time_sec + $compress_time_sec" | bc)
        successful_compress_count=$((successful_compress_count + 1))
        echo "Compression time (real): ${compress_time_sec}s"
        echo "------------------------------------------------------"
    fi

    decompress_cmd="./build/lz_codec -d -i \"tmp/tmp.enc\" -o \"tmp/tmp.dec\" ${adaptive_flag:+"$adaptive_flag"}${model_flag:+" $model_flag"}"
    echo "$decompress_cmd"

    # Capture time output
    decompress_time_output=$( ( time eval "$decompress_cmd" ) 2>&1 )
    decompress_status=$?
    # Print the command's stdout/stderr
    echo "$decompress_time_output"
    echo "------------------------------------------------------"

    if [ $decompress_status -ne 0 ]; then
        echo "Error: Decompression failed for '$bench_filename'!" >&2
        overall_status=1
        continue # Skip to the next file
    else
        # Parse time and add to total if successful
        decompress_time_sec=$(parse_time "$decompress_time_output")
        total_decompress_time_sec=$(echo "scale=4; $total_decompress_time_sec + $decompress_time_sec" | bc)
        successful_decompress_count=$((successful_decompress_count + 1))
        echo "Decompression time (real): ${decompress_time_sec}s"
        echo "------------------------------------------------------"
    fi

    # --- Calculate and Display Stats ---
    original_size=$(stat -c %s "$bench_filename" 2>/dev/null) || original_size=$(./size "$bench_filename" 2>/dev/null) || original_size=0
    compressed_size=$(stat -c %s "tmp/tmp.enc" 2>/dev/null) || compressed_size=$(./size "tmp/tmp.enc" 2>/dev/null) || compressed_size=0
    sizes_valid=0 # Flag to track if sizes were determined correctly for BPC calc

    echo "Original size: $original_size bytes"
    echo "Compressed size: $compressed_size bytes"

    if [ "$original_size" -gt 0 ] && [ "$compressed_size" -ge 0 ]; then
      sizes_valid=1 # Mark sizes as valid for this file
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

    # --- Verify Correctness ---
    if ! cmp -s "$bench_filename" "tmp/tmp.dec"; then
        echo "Error: Files do not match for '$bench_filename'!" >&2
        overall_status=1
    else
        echo "Success: Files match for '$bench_filename'!"
        # Accumulate for average BPC calculation ONLY if sizes were valid AND files match
        if [ "$sizes_valid" -eq 1 ]; then
            current_bits_used=$(echo "$compressed_size * 8" | bc)
            total_bits_used=$(echo "scale=0; $total_bits_used + $current_bits_used" | bc)
            total_original_size=$(echo "scale=0; $total_original_size + $original_size" | bc)
            successful_bpc_count=$((successful_bpc_count + 1))
        fi
    fi

done # End of loop for files_to_process

echo "======================================================"
echo "Benchmark Summary"
echo "======================================================"

# --- Calculate and Print Average Times ---
if [ "$successful_compress_count" -gt 0 ]; then
    avg_compress_time=$(echo "scale=4; $total_compress_time_sec / $successful_compress_count" | bc)
    printf "Average Compression Time (real): %.4fs (%d successful runs)\n" "$avg_compress_time" "$successful_compress_count"
else
    echo "Average Compression Time (real): N/A (0 successful runs)"
fi

if [ "$successful_decompress_count" -gt 0 ]; then
    avg_decompress_time=$(echo "scale=4; $total_decompress_time_sec / $successful_decompress_count" | bc)
    printf "Average Decompression Time (real): %.4fs (%d successful runs)\n" "$avg_decompress_time" "$successful_decompress_count"
else
    echo "Average Decompression Time (real): N/A (0 successful runs)"
fi

# --- Calculate and Print Average Bits Per Char ---
if [ "$successful_bpc_count" -gt 0 ] && [ "$total_original_size" -gt 0 ]; then
    avg_bits_per_char=$(echo "scale=4; $total_bits_used / $total_original_size" | bc)
    printf "Average Bits Per Char:         %.4f (%d successful runs)\n" "$avg_bits_per_char" "$successful_bpc_count"
elif [ "$successful_bpc_count" -gt 0 ]; then
     echo "Average Bits Per Char:         N/A (Total original size was zero for successful runs)"
else
    echo "Average Bits Per Char:         N/A (0 successful runs with valid sizes and matching files)"
fi


echo "------------------------------------------------------"
if [ "$overall_status" -eq 0 ]; then
    echo "All benchmarks completed successfully."
else
    echo "One or more benchmarks encountered errors." >&2
fi
echo "Benchmark finished."

exit $overall_status
