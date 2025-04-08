#!/usr/bin/env python3

import os
import subprocess
import shutil
import sys
import glob # Import glob for finding files

# --- Configuration ---
build_dir = "build"
benchmark_dir = "benchmark"
generated_dir = "generated" # Added directory for generated files
tmp_dir = "tmp"
lz_codec_exe = os.path.join(build_dir, "lz_codec")
# Assuming 'size' is an executable in the current directory or PATH
# Adjust path if necessary, e.g., os.path.join(build_dir, "size")
size_exe = "./size"
convert_script = "convert.py"

# --- Helper Function ---
def run_command(cmd_list, check=True, capture=True, show_output=True):
    """
    Runs a command, prints it, captures output, and checks for errors.
    Controls whether captured output is displayed.
    """
    separator = "-" * 60 # Use a slightly longer separator
    print(separator)
    # Use subprocess.list2cmdline for a more shell-like representation on Windows
    # On Unix, ' '.join is usually sufficient for display purposes
    cmd_str = (
        subprocess.list2cmdline(cmd_list)
        if os.name == "nt"
        else " ".join(cmd_list)
    )
    print(f"Running: {cmd_str}")

    try:
        result = subprocess.run(
            cmd_list,
            check=check,
            text=True,
            capture_output=capture,
        )
        # Determine if output should be shown
        should_show_output = show_output or (
            not check and result.returncode != 0
        )

        if should_show_output:
            if result.stdout:
                print("--- STDOUT ---")
                print(result.stdout.strip())
                print("--------------")
            # Print stderr even if check=True, as warnings might be printed
            if result.stderr:
                print("--- STDERR ---", file=sys.stderr)
                print(result.stderr.strip(), file=sys.stderr)
                print("--------------", file=sys.stderr)

        print(separator)
        return result  # Return the completed process object

    except subprocess.CalledProcessError as e:
        print(f"\nError running command: {cmd_str}", file=sys.stderr)
        print(f"Return code: {e.returncode}", file=sys.stderr)
        # Always show output on error
        if e.stdout:
            print("--- STDOUT ---", file=sys.stderr)
            print(e.stdout.strip(), file=sys.stderr)
            print("--------------", file=sys.stderr)
        if e.stderr:
            print("--- STDERR ---", file=sys.stderr)
            print(e.stderr.strip(), file=sys.stderr)
            print("--------------", file=sys.stderr)
        print(separator)
        return None # Indicate failure

    except FileNotFoundError:
        print(
            f"\nError: Command or file not found: {cmd_list[0]}", file=sys.stderr
        )
        print(
            "Please ensure make, lz_codec, size, and python are installed and in your PATH"
        )
        print(f"Attempted command: {cmd_str}")
        print(separator)
        sys.exit(1) # Exit here, as it's a setup issue

# --- Processing Function ---
def process_file(input_file_path, width="512"):
    """Compresses, decompresses, and verifies a single file."""
    print(f"\n{'='*20} Processing file: {input_file_path} {'='*20}")
    base_name = os.path.basename(input_file_path)
    name_without_ext = os.path.splitext(base_name)[0]

    # Construct temporary file paths based on input filename
    encoded_file = os.path.join(tmp_dir, f"{name_without_ext}.enc")
    decoded_file = os.path.join(tmp_dir, f"{name_without_ext}.dec")
    golden_png = os.path.join(tmp_dir, f"{name_without_ext}_golden.png")
    output_png = os.path.join(tmp_dir, f"{name_without_ext}_output.png")

    # if the extension is .bin get the name
    if input_file_path.endswith(".bin"):
        name_without_ext = os.path.splitext(base_name)[0]
        # Set the width to 512 if the file is .bin
        width = name_without_ext

    # 1. Compress
    print("\n[Compress]")
    compress_cmd = [
        lz_codec_exe,
        "-c",
        "-i",
        input_file_path,
        "-o",
        encoded_file,
        "-w",
        width,
        "-a",
    ]
    # Show lz_codec output
    if run_command(compress_cmd, show_output=True) is None:
        print(f"!!! Compression failed for {input_file_path} !!!")
        return False # Command failed

    # 2. Decompress
    print("\n[Decompress]")
    decompress_cmd = [
        lz_codec_exe,
        "-d",
        "-i",
        encoded_file,
        "-o",
        decoded_file,
        "-a",
    ]
    # Show lz_codec output
    if run_command(decompress_cmd, show_output=True) is None:
        print(f"!!! Decompression failed for {encoded_file} !!!")
        return False # Command failed

    # 3. Get sizes
    print("\n[Verify Size]")
    # Show size output
    if run_command([size_exe, input_file_path], show_output=True) is None:
        return False # Command failed
    if run_command([size_exe, encoded_file], show_output=True) is None:
        return False # Command failed

    # 4. Compare original and decoded files
    print("\n[Verify Content]")
    # Don't show cmp output unless it fails (non-zero return code)
    compare_result = run_command(
        ["cmp", "-s", input_file_path, decoded_file],
        check=False,
        show_output=False,
    )
    if compare_result is None: # cmp command itself failed (unlikely)
        print("!!! Error running cmp command. !!!", file=sys.stderr)
        return False

    files_match = compare_result.returncode == 0

    if files_match:
        print(">>> Success: Files match!")
    else:
        print(
            f"!!! Error: Files do not match for {input_file_path}! !!!",
            file=sys.stderr,
        )
        # Don't generate PNGs if files don't match, as comparison is the goal
        print(f"{'='*20} Finished processing {input_file_path} (FAILED) {'='*20}")
        return False

    # 5. Generate PNGs using the python script (Optional, only if files match)
    print("\n[Generate PNGs]")
    convert_golden_cmd = [
        sys.executable, # Use sys.executable to ensure using the same python
        convert_script,
        input_file_path,
        width,
        "-o",
        golden_png,
    ]
    # Show convert.py output
    if run_command(convert_golden_cmd, show_output=True) is None:
        print(
            f"Warning: Failed to generate golden PNG for {input_file_path}",
            file=sys.stderr,
        )
        # Continue even if PNG generation fails, but report success based on cmp

    convert_output_cmd = [
        sys.executable,
        convert_script,
        decoded_file,
        width,
        "-o",
        output_png,
    ]
    # Show convert.py output
    if run_command(convert_output_cmd, show_output=True) is None:
        print(
            f"Warning: Failed to generate output PNG for {decoded_file}",
            file=sys.stderr,
        )
        # Continue even if PNG generation fails

    print(f"\n{'='*20} Finished processing {input_file_path} (Success) {'='*20}")
    return files_match # Return True only if files matched

# --- Main Script Logic ---
overall_success = True
failed_files = []

try:
    # 1. Compile using make (once at the start)
    print(">>> Compiling project...")
    if run_command(["make", "-B", "-j4"], show_output=True) is None:
        print("!!! Make command failed. Exiting. !!!", file=sys.stderr)
        sys.exit(1)
    print(">>> Compilation complete.")

    # 2. Setup temporary directory (once at the start)
    print(f"\n>>> Setting up temporary directory: {tmp_dir}")
    if os.path.exists(tmp_dir):
        print(f"Removing existing directory: {tmp_dir}")
        shutil.rmtree(tmp_dir)
    os.makedirs(tmp_dir)
    print(f"Created directory: {tmp_dir}")

    # --- Phase 1: Process .raw files in benchmark directory ---
    print(f"\n{'#'*15} Phase 1: Processing {benchmark_dir}/*.raw files {'#'*15}")
    benchmark_files = sorted(glob.glob(os.path.join(benchmark_dir, "*.raw"))) # Sort for consistent order
    if not benchmark_files:
        print(f"Warning: No .raw files found in {benchmark_dir}", file=sys.stderr)
    else:
        print(f"Found {len(benchmark_files)} .raw files to process.")
        for raw_file in benchmark_files:
            if not process_file(raw_file):
                overall_success = False
                failed_files.append(raw_file)

    # --- Phase 2: Process .bin files in generated directory ---
    # print(f"\n{'#'*15} Phase 2: Processing {generated_dir}/*.bin files {'#'*15}")
    # if not os.path.isdir(generated_dir):
    #     print(
    #         f"Warning: Directory {generated_dir} not found. Skipping Phase 2.",
    #         file=sys.stderr,
    #     )
    # else:
    #     generated_files = sorted(glob.glob(os.path.join(generated_dir, "*.bin"))) # Sort for consistent order
    #     if not generated_files:
    #         print(
    #             f"Warning: No .bin files found in {generated_dir}", file=sys.stderr
    #         )
    #     else:
    #         print(f"Found {len(generated_files)} .bin files to process.")
    #         for bin_file in generated_files:
    #             if not process_file(bin_file):
    #                 overall_success = False
    #                 failed_files.append(bin_file)

    # --- Final Summary ---
    print("\n" + "#" * 60)
    print(f"{'#'*20} Final Summary {'#'*22}")
    print("#" * 60)
    if overall_success:
        print("\n>>> SUCCESS: All processed files were successfully compressed and decompressed correctly!")
    else:
        print(
            "\n!!! FAILURE: One or more files failed the compression/decompression test. !!!",
            file=sys.stderr,
        )
        print("Failed files:", file=sys.stderr)
        for f in failed_files:
            print(f"  - {f}", file=sys.stderr)
        print("#" * 60)
        sys.exit(1) # Exit with non-zero status if any file failed

except Exception as e:
    print(f"\nAn unexpected error occurred: {e}", file=sys.stderr)
    import traceback
    traceback.print_exc() # Print stack trace for unexpected errors
    sys.exit(1)

print("\n>>> Benchmark finished successfully.")
print("#" * 60)
sys.exit(0) # Explicitly exit with 0 on success
