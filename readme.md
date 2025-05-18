# LZSS Compressor and Decompressor

A C++ implementation of the LZSS compression and decompression algorithm.

## Features

*   **Compression & Decompression:** Compresses input files using LZSS and decompresses them back to their original form.
*   **Adaptive Block Strategy (`-a`):**
    *   Optionally divides the input data into blocks (configurable size via `--block_size`).
    *   For each block, it tries both horizontal and vertical serialization and picks the one that yields better compression. Useful for 2D data like raw images.
*   **Model Preprocessing (`-m`):**
    *   Optionally applies a data transformation *before* LZSS compression to potentially improve ratios.
    *   Currently supports Delta transform (default) or Move-To-Front (MTF). The choice is a compile-time option via the `MTF` macro in `include/common.hpp`.
*   **Customizable LZSS Parameters:**
    *   Allows specifying the number of bits for offset (`--offset_bits`) and length (`--length_bits`) in coded tokens as well as custom block size for adaptive mode (`--block_size`)
*   **Bit Packing:** Writes compressed data efficiently using bit-level packing.
*   **Unsuccessful Compression Handling:** If compression doesn't reduce file size, the original file is copied to the output, prefixed with a `0x00` byte.

## Dependencies

*   A C++11 (or later) compatible compiler (e.g., g++, clang++).
*   The `argparse.hpp` library (https://github.com/p-ranav/argparse) - Assumed to be available in the `include` directory or system include paths.

## Building

Compile the source files using your C++ compiler. Example using g++:

```bash
make
```

## Usage
```bash
./lz_codec [options]
```

**Options:**

*   `-c`: Enable Compression mode.
*   `-d`: Enable Decompression mode.
*   `-i <file>`: Specify the input file (Required).
*   `-o <file>`: Specify the output file (Required).
*   `-a`: Use the adaptive block strategy.
*   `-m`: Use model preprocessing (Delta/MTF) before compression.
*   `-w <width>`: Specify the width of the input data (used for calculating height, important for non-adaptive or 2D data). Defaults to 1.
*   `--block_size <size>`: Set the block size for adaptive mode (Default: 16).
*   `--offset_bits <bits>`: Set the number of bits for the offset part of a coded token (Default: 8).
*   `--length_bits <bits>`: Set the number of bits for the length part of a coded token (Default: 10).
*   `--help`: Display help message.

## Author

*   Pavel Kratochvil (xkrato61@fit.vutbr.cz)
*   Faculty of Information Technology, Brno University of Technology