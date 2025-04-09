#!/bin/bash

# --- Default Settings ---
bench_filename="cb.raw"
size=512
adaptive_flag="" # Will be set to "-a" if -a is provided
model_flag=""    # Will be set to "-m <model>" if -m is provided

# --- Function to display usage ---
usage() {
  echo "Usage: $0 [-a] [-m <model_name>]"
  echo "  -a : Enable adaptive mode for lz_codec"
  echo "  -m : Specify the model for lz_codec"
  exit 1
}

# --- Parse Command-Line Options ---
while getopts "am:" opt; do
  case $opt in
    a)
      adaptive_flag="-a"
      ;;
    m)
      model_flag="-m"
      ;;
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
# Construct the command string for clarity in echo
codec_cmd_compress="./build/lz_codec -c -i benchmark/$bench_filename -o tmp/cb.enc -w $size $adaptive_flag $model_flag"
echo "$codec_cmd_compress"
# Execute the command
time $codec_cmd_compress
compress_status=$?
echo "------------------------------------------------------"

if [ $compress_status -ne 0 ]; then
    echo "Error: Compression failed!" >&2
    exit 1
fi

codec_cmd_decompress="./build/lz_codec -d -i tmp/cb.enc -o tmp/cb.dec $adaptive_flag $model_flag"
echo "$codec_cmd_decompress"
# Execute the command
time $codec_cmd_decompress
decompress_status=$?
echo "------------------------------------------------------"

if [ $decompress_status -ne 0 ]; then
    echo "Error: Decompression failed!" >&2
    exit 1
fi

# --- Verification and Size ---
./size benchmark/$bench_filename
./size tmp/cb.enc

if ! cmp -s benchmark/$bench_filename tmp/cb.dec; then
    echo "Error: Files do not match!" >&2
else
    echo "Success: Files match!"
fi

# --- Image Conversion (Optional) ---
echo "Converting files to images..."
python convert.py benchmark/$bench_filename $size -o tmp/cb_golden.png
python convert.py tmp/cb.dec $size -o tmp/cb.png

echo "Benchmark finished."
