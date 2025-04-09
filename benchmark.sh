#!/bin/bash

bench_filename="cb.raw"
size=512

make -B -j4
rm -rf tmp
mkdir tmp
echo "Running benchmark..."
echo "------------------------------------------------------"
echo "./build/lz_codec -c -i benchmark/$bench_filename -o tmp/cb.enc -w $size -a -m"
time ./build/lz_codec -c -i benchmark/$bench_filename -o tmp/cb.enc -w $size -a -m
echo "------------------------------------------------------"
echo "./build/lz_codec -d -i tmp/cb.enc -o tmp/cb.dec -w $size -a -m"
time ./build/lz_codec -d -i tmp/cb.enc -o tmp/cb.dec -a -m
echo "------------------------------------------------------"
./size benchmark/$bench_filename
./size tmp/cb.enc

if ! cmp -s benchmark/$bench_filename tmp/cb.dec; then
    echo "Error: Files do not match!" >&2
else
    echo "Success: Files match!"
fi

python convert.py benchmark/$bench_filename $size -o tmp/cb_golden.png
python convert.py tmp/cb.dec $size -o tmp/cb.png