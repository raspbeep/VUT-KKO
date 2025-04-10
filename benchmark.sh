#!/bin/bash

# --- Default Settings ---
bench_filename="cb.raw"
size=512
adaptive_flag=""
model_flag=""

usage() {
  echo "Usage: $0 [-a] [-m]"
  echo "  -a : Enable adaptive mode for lz_codec"
  echo "  -m : Enable model mode for lz_codec (boolean flag)"
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
shift $((OPTIND-1))

make
if [ $? -ne 0 ]; then
    echo "Error: make failed!" >&2
    exit 1
fi

rm -rf tmp
mkdir tmp

echo "Running benchmark..."
echo "------------------------------------------------------"
echo "./build/lz_codec -c -i benchmark/$bench_filename -o tmp/tmp.enc -w $size ${adaptive_flag:+"$adaptive_flag"}${model_flag:+" $model_flag"}"

time ./build/lz_codec -c \
    -i "benchmark/$bench_filename" \
    -o "tmp/tmp.enc" \
    -w "$size" \
    $adaptive_flag \
    $model_flag

compress_status=$?
echo "------------------------------------------------------"

if [ $compress_status -ne 0 ]; then
    echo "Error: Compression failed!" >&2
    exit 1
fi

echo "./build/lz_codec -d -i tmp/tmp.enc -o tmp/tmp.dec ${adaptive_flag:+"$adaptive_flag"}${model_flag:+" $model_flag"}"

time ./build/lz_codec -d \
    -i "tmp/tmp.enc" \
    -o "tmp/tmp.dec" \
    $adaptive_flag \
    $model_flag

decompress_status=$?
echo "------------------------------------------------------"

if [ $decompress_status -ne 0 ]; then
    echo "Error: Decompression failed!" >&2
    exit 1
fi

original_size=$(stat -c %s "benchmark/$bench_filename" 2>/dev/null || ./size "benchmark/$bench_filename")
compressed_size=$(stat -c %s "tmp/tmp.enc" 2>/dev/null || ./size "tmp/tmp.enc")

echo "Original size: $original_size bytes"
echo "Compressed size: $compressed_size bytes"

if [ "$original_size" -gt 0 ]; then
  space_saved=$(echo "scale=2; (1 - $compressed_size / $original_size) * 100" | bc)
  echo "Space saved: $space_saved%"
else
  echo "Error: Original size is zero or invalid."
fi

# --- Write bits per char of original file (compressed_size * 8) / original_size ---
if [ "$original_size" -gt 0 ]; then
  bits_per_char=$(echo "scale=2; ($compressed_size * 8) / $original_size" | bc)
  echo "Bits per char: $bits_per_char"
else
  echo "Error: Original size is zero or invalid."
fi


if ! cmp -s "benchmark/$bench_filename" "tmp/tmp.dec"; then
    echo "Error: Files do not match!" >&2
else
    echo "Success: Files match!"
fi

# # --- Image Conversion (Optional) ---
# if command -v python &> /dev/null && [ -f convert.py ]; then
#     echo "Converting files to images..."
#     python convert.py "benchmark/$bench_filename" "$size" -o "tmp/cb_golden.png"
#     python convert.py "tmp/tmp.dec" "$size" -o "tmp/cb.png"
# else
#     echo "Skipping image conversion (python or convert.py not found)."
# fi

echo "Benchmark finished."
