#!/bin/bash

# --- Default Settings ---
bench_filename="cb.raw"
size=512
adaptive_flag="" # Will be set to "-a" if -a is provided
model_flag=""    # Will be set to "-m" if -m is provided

# --- Function to display usage ---
usage() {
  # Updated usage message
  echo "Usage: $0 [-a] [-m]"
  echo "  -a : Enable adaptive mode for lz_codec"
  echo "  -m : Enable model mode for lz_codec (boolean flag)" # Clarified
  exit 1
}

# --- Parse Command-Line Options ---
# Remove the colon after 'm' - it does not expect an argument
while getopts "am" opt; do
  case $opt in
    a)
      adaptive_flag="-a"
      ;;
    m)
      # Just set the flag, no OPTARG involved
      model_flag="-m"
      ;;
    \?) # Handle invalid options
      echo "Invalid option: -$OPTARG" >&2
      usage
      ;;
    # No need for the ':' case for missing arguments for -m
  esac
done
shift $((OPTIND-1)) # Remove parsed options from arguments

# --- Main Script Logic ---
make -j4
if [ $? -ne 0 ]; then
    echo "Error: make failed!" >&2
    exit 1
fi

rm -rf tmp
mkdir tmp

echo "Running benchmark..."
echo "------------------------------------------------------"
# Construct the command string *only* for echoing
# Use parameter expansion ${var:+" $var"} to add a leading space only if the variable exists
echo "./build/lz_codec -c -i benchmark/$bench_filename -o tmp/cb.enc -w $size ${adaptive_flag:+"$adaptive_flag"}${model_flag:+" $model_flag"}"

# Execute the command directly, letting the shell handle arguments
# Quote arguments that might contain spaces or special characters
# Unset variables expand to nothing, which is what we want
time ./build/lz_codec -c \
    -i "benchmark/$bench_filename" \
    -o "tmp/cb.enc" \
    -w "$size" \
    $adaptive_flag \
    $model_flag # Simply include the model_flag variable

compress_status=$?
echo "------------------------------------------------------"

if [ $compress_status -ne 0 ]; then
    echo "Error: Compression failed!" >&2
    exit 1
fi

# Construct the command string *only* for echoing
echo "./build/lz_codec -d -i tmp/cb.enc -o tmp/cb.dec ${adaptive_flag:+"$adaptive_flag"}${model_flag:+" $model_flag"}"

# Execute the command directly
time ./build/lz_codec -d \
    -i "tmp/cb.enc" \
    -o "tmp/cb.dec" \
    $adaptive_flag \
    $model_flag # Simply include the model_flag variable

decompress_status=$?
echo "------------------------------------------------------"

if [ $decompress_status -ne 0 ]; then
    echo "Error: Decompression failed!" >&2
    exit 1
fi

# --- Verification and Size ---
echo "Original size:"
stat -c %s "benchmark/$bench_filename" 2>/dev/null || ./size "benchmark/$bench_filename" # Try stat, fallback to ./size
echo "Compressed size:"
stat -c %s "tmp/cb.enc" 2>/dev/null || ./size "tmp/cb.enc" # Try stat, fallback to ./size


if ! cmp -s "benchmark/$bench_filename" "tmp/cb.dec"; then
    echo "Error: Files do not match!" >&2
else
    echo "Success: Files match!"
fi

# --- Image Conversion (Optional) ---
if command -v python &> /dev/null && [ -f convert.py ]; then
    echo "Converting files to images..."
    python convert.py "benchmark/$bench_filename" "$size" -o "tmp/cb_golden.png"
    python convert.py "tmp/cb.dec" "$size" -o "tmp/cb.png"
else
    echo "Skipping image conversion (python or convert.py not found)."
fi

echo "Benchmark finished."
