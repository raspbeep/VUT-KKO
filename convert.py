#!/usr/bin/env python3

import numpy as np
from PIL import Image
import argparse
import os
import sys

def convert_raw_to_image(raw_file_path, n_dim, output_file_path=None):
    """
    Converts a raw binary file containing N*N 8-bit grayscale pixel values
    into a viewable image file (e.g., PNG).

    Args:
        raw_file_path (str): Path to the input raw binary file.
        n_dim (int): The dimension N of the N*N image.
        output_file_path (str, optional): Path to save the output image.
                                         If None, defaults to the input filename
                                         with a .png extension.

    Returns:
        bool: True if conversion was successful, False otherwise.
    """
    if n_dim <= 0:
        print(f"Error: Dimension N must be a positive integer. Got {n_dim}")
        return False

    if not os.path.exists(raw_file_path):
        print(f"Error: Input file not found: {raw_file_path}")
        return False

    expected_size = n_dim * n_dim
    actual_size = os.path.getsize(raw_file_path)

    if actual_size != expected_size:
        print(f"Error: File size mismatch for {n_dim}x{n_dim} image.")
        print(f"Expected {expected_size} bytes, but file contains {actual_size} bytes.")
        return False

    try:
        # Read the raw binary data
        with open(raw_file_path, 'rb') as f:
            raw_data = f.read()

        # Convert the raw bytes to a NumPy array of unsigned 8-bit integers
        # uint8 corresponds to 8-bit grayscale (0-255)
        image_array = np.frombuffer(raw_data, dtype=np.uint8)

        # Reshape the 1D array into an N x N matrix
        image_array = image_array.reshape((n_dim, n_dim))

        # Create a PIL Image object from the NumPy array
        # 'L' mode is for 8-bit grayscale
        img = Image.fromarray(image_array, mode='L')

        # Determine the output file path
        if output_file_path is None:
            base_name = os.path.splitext(os.path.basename(raw_file_path))[0]
            output_file_path = f"{base_name}.png"
        elif not output_file_path.lower().endswith(('.png', '.jpg', '.jpeg', '.bmp', '.tiff', '.gif')):
             # Add a default extension if none is provided in the output path
             print(f"Warning: Output path '{output_file_path}' has no standard image extension. Saving as PNG.")
             output_file_path += ".png"


        # Save the image
        img.save(output_file_path)
        print(f"Successfully converted '{raw_file_path}' ({n_dim}x{n_dim}) to '{output_file_path}'")
        return True

    except FileNotFoundError:
        # This case should be caught by os.path.exists, but included for safety
        print(f"Error: Input file not found: {raw_file_path}")
        return False
    except IOError as e:
        print(f"Error reading or writing file: {e}")
        return False
    except ValueError as e:
        # Could happen if reshape fails, though size check should prevent this
        print(f"Error processing image data: {e}")
        return False
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        return False

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert an N*N raw 8-bit grayscale image file to a standard image format (e.g., PNG)."
    )
    parser.add_argument(
        "filepath", help="Path to the raw input image file."
    )
    parser.add_argument(
        "n", type=int, help="Dimension N of the square image (N*N)."
    )
    parser.add_argument(
        "-o",
        "--output",
        help="Optional output file path. If omitted, saves as <input_filename>.png in the current directory.",
        default=None,
    )

    args = parser.parse_args()

    if not convert_raw_to_image(args.filepath, args.n, args.output):
        sys.exit(1) # Exit with a non-zero status to indicate failure
