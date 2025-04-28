#!/usr/bin/env python3

import numpy as np
from PIL import Image
import argparse
import os
import sys

def convert_raw_to_image(raw_file_path, width, output_file_path=None):
    """
    Converts a raw binary file containing grayscale pixel values into a viewable image file.
    The width is specified, and height is calculated based on file size.

    Args:
        raw_file_path (str): Path to the input raw binary file.
        width (int): The width of the image.
        output_file_path (str, optional): Path to save the output image.
                                         If None, defaults to the input filename
                                         with a .png extension.

    Returns:
        bool: True if conversion was successful, False otherwise.
    """
    if width <= 0:
        print(f"Error: Width must be a positive integer. Got {width}")
        return False

    if not os.path.exists(raw_file_path):
        print(f"Error: Input file not found: {raw_file_path}")
        return False

    actual_size = os.path.getsize(raw_file_path)

    if actual_size % width != 0:
        print(f"Error: File size {actual_size} bytes is not divisible by width {width}.")
        print("Cannot determine a proper height for the image.")
        return False

    height = actual_size // width

    try:
        # Read the raw binary data
        with open(raw_file_path, 'rb') as f:
            raw_data = f.read()

        # Convert the raw bytes to a NumPy array of unsigned 8-bit integers
        image_array = np.frombuffer(raw_data, dtype=np.uint8)

        # Reshape the array into height x width
        image_array = image_array.reshape((height, width))

        # Create a PIL Image object from the NumPy array
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
        print(f"Successfully converted '{raw_file_path}' ({width}x{height}) to '{output_file_path}'")
        return True

    except FileNotFoundError:
        print(f"Error: Input file not found: {raw_file_path}")
        return False
    except IOError as e:
        print(f"Error reading or writing file: {e}")
        return False
    except ValueError as e:
        print(f"Error processing image data: {e}")
        return False
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        return False

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert a raw 8-bit grayscale binary file to an image, specifying width only."
    )
    parser.add_argument(
        "filepath", help="Path to the raw input image file."
    )
    parser.add_argument(
        "width", type=int, help="Width of the image."
    )
    parser.add_argument(
        "-o",
        "--output",
        help="Optional output file path. If omitted, saves as <input_filename>.png in the current directory.",
        default=None,
    )

    args = parser.parse_args()

    if not convert_raw_to_image(args.filepath, args.width, args.output):
        sys.exit(1)
