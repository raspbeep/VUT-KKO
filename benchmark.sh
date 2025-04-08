#!/bin/bash

bench_filename="nk01.raw"

make -B -j4
rm -rf tmp
mkdir tmp
echo "Running benchmark..."
echo "------------------------------------------------------"
echo "./build/lz_codec -c -i benchmark/$bench_filename -o tmp/cb.enc -w 512 -a"
./build/lz_codec -c -i benchmark/$bench_filename -o tmp/cb.enc -w 512 -a
echo "------------------------------------------------------"
echo "./build/lz_codec -d -i tmp/cb.enc -o tmp/cb.dec -w 512 -a"
./build/lz_codec -d -i tmp/cb.enc -o tmp/cb.dec -a
echo "------------------------------------------------------"
./size benchmark/$bench_filename
./size tmp/cb.enc



if ! cmp -s benchmark/$bench_filename tmp/cb.dec; then
    echo "Error: Files do not match!" >&2
else
    echo "Success: Files match!"
fi



python convert.py benchmark/$bench_filename 512 -o tmp/cb_golden.png
python convert.py tmp/cb.dec 512 -o tmp/cb.png