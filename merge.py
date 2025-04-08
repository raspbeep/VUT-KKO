import argparse
import glob
import os
import sys
from typing import List, Set


def merge_files(
    dirs_to_search: List[str],
    exclude_files: Set[str],
    output_filename: str,
) -> bool:
    """
    Finds all .cpp and .hpp files in the specified directories, merges them
    into a single output file, adding comments indicating the source file,
    and skipping any excluded files.

    Args:
        dirs_to_search: A list of directory paths to search for files.
        exclude_files: A set of absolute, normalized paths to exclude.
        output_filename: The path for the merged output file.

    Returns:
        True if merging was successful (or no files found), False on error.
    """
    files_to_merge = []
    processed_dirs = set()

    print("Searching for .cpp and .hpp files...")

    # Normalize the output file path once for exclusion check
    abs_output_path = os.path.abspath(output_filename)
    norm_output_path = os.path.normpath(abs_output_path)

    for directory in dirs_to_search:
        norm_dir = os.path.normpath(directory)
        if not os.path.isdir(norm_dir):
            print(f"Warning: Directory not found: '{directory}'. Skipping.")
            continue

        if norm_dir in processed_dirs:
            print(f"Warning: Directory '{directory}' specified multiple times. Skipping duplicate.")
            continue
        processed_dirs.add(norm_dir)

        print(f"  Scanning directory: '{norm_dir}'")
        # Find .cpp files
        cpp_pattern = os.path.join(norm_dir, "*.cpp")
        found_cpp = glob.glob(cpp_pattern)

        # Find .hpp files
        hpp_pattern = os.path.join(norm_dir, "*.hpp")
        found_hpp = glob.glob(hpp_pattern)

        # Combine and filter
        for filepath in found_cpp + found_hpp:
            abs_filepath = os.path.abspath(filepath)
            norm_filepath = os.path.normpath(abs_filepath)

            # Check against exclusion list
            if norm_filepath in exclude_files:
                print(f"    Excluding specified file: {filepath}")
                continue

            # Check against output file path
            if norm_filepath == norm_output_path:
                print(f"    Excluding target output file: {filepath}")
                continue

            files_to_merge.append(filepath) # Store original path for comment

    # Sort files alphabetically by their original path for predictable order
    files_to_merge.sort()

    if not files_to_merge:
        print("\nNo matching files found to merge (after exclusions).")
        return True  # No action needed, considered success

    print(f"\nFound {len(files_to_merge)} files to merge:")
    for f in files_to_merge:
        print(f"  - {f}")

    try:
        # Open the output file in write mode ('w') with UTF-8 encoding
        print(f"\nWriting to {output_filename}...")
        with open(output_filename, "w", encoding="utf-8") as outfile:
            for filename in files_to_merge:
                # Use the original (potentially relative) filename for the comment
                relative_filename = os.path.relpath(filename) # More readable comment
                print(f"  Adding: {relative_filename}")
                try:
                    # Write the comment header
                    outfile.write(f"// {relative_filename.replace(os.sep, '/')}\n") # Use forward slash in comment

                    # Open the current source file in read mode ('r')
                    with open(filename, "r", encoding="utf-8") as infile:
                        content = infile.read()
                        outfile.write(content)

                    # Add separation
                    outfile.write("\n\n")

                except IOError as e:
                    print(f"    Error reading file {filename}: {e}. Skipping.")
                except UnicodeDecodeError as e:
                    print(
                        f"    Error decoding file {filename} (not UTF-8?): {e}. Skipping."
                    )
                except Exception as e:
                    print(
                        f"    An unexpected error occurred with file {filename}: {e}. Skipping."
                    )

        print(
            f"\nSuccessfully merged {len(files_to_merge)} files into {output_filename}"
        )
        return True  # Indicate success

    except IOError as e:
        print(f"Error opening or writing to the output file {output_filename}: {e}")
        return False
    except Exception as e:
        print(f"An unexpected error occurred during the process: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Merge .cpp and .hpp files from specified directories into a single file.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument(
        "--dirs",
        required=True,
        help="Comma-separated list of directories to search for .cpp and .hpp files.",
    )
    parser.add_argument(
        "--exclude-file",
        action="append", # Allows specifying multiple times: --exclude-file a --exclude-file b
        default=[],
        help="Specify a file path to exclude (can be used multiple times). Paths are matched exactly.",
    )
    parser.add_argument(
        "-o",
        "--output",
        default="merged.cpp",
        help="Name of the output file to create.",
    )

    args = parser.parse_args()

    # Process directories
    dirs_list = [d.strip() for d in args.dirs.split(",") if d.strip()]
    if not dirs_list:
        print("Error: No valid directories specified with --dirs.", file=sys.stderr)
        sys.exit(1)

    # Process exclusions: normalize paths for reliable matching
    normalized_exclusions = set()
    for excl_path in args.exclude_file:
        try:
            abs_path = os.path.abspath(excl_path)
            norm_path = os.path.normpath(abs_path)
            normalized_exclusions.add(norm_path)
            print(f"Exclusion rule added for: {norm_path}")
        except Exception as e:
            print(f"Warning: Could not process exclusion path '{excl_path}': {e}")


    # Run the merge operation
    if merge_files(dirs_list, normalized_exclusions, args.output):
        sys.exit(0)  # Exit with success code
    else:
        sys.exit(1)  # Exit with error code


if __name__ == "__main__":
    main()
