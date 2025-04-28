from PIL import Image
import sys
import os

def image_to_raw_grayscale(input_path, output_path):
    # Open the image
    img = Image.open(input_path)
    # Convert to grayscale
    gray_img = img.convert('L')  # 'L' mode is 8-bit pixels, black and white
    
    # Get pixel data
    pixels = list(gray_img.getdata())
    width, height = gray_img.size
    
    # Write raw pixel data to file
    with open(output_path, 'wb') as f:
        for y in range(height):
            # Extract the row of pixels
            row_pixels = pixels[y * width:(y + 1) * width]
            # Write the row to file
            f.write(bytearray(row_pixels))
    print(f"Saved raw grayscale image to {output_path}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python script.py <input_image> <output_raw_file>")
    else:
        input_image = sys.argv[1]
        output_raw_arg = sys.argv[2]
        # Open image to get width
        with Image.open(input_image) as img:
            width, _ = img.size
        # Construct output filename
        output_filename = f"{width}-{output_raw_arg}.raw"
        # Call the function
        image_to_raw_grayscale(input_image, output_filename)