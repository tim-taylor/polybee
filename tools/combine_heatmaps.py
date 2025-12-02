#!/usr/bin/env python3
"""
Combine multiple heatmap PNG files into a single A4 page with grid layout.

The script takes multiple PNG files (e.g., from visualize_heatmap.py) and arranges
them in a grid that fits on a single A4 page. The grid dimensions are automatically
calculated based on the number of input images.

Dependencies:
    pip install matplotlib pillow numpy

Or on Ubuntu/Debian:
    sudo apt install python3-matplotlib python3-pil python3-numpy

Usage:
    ./combine_heatmaps.py output.pdf image1.png image2.png image3.png ...
    ./combine_heatmaps.py output.png *.png
"""

import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
from PIL import Image
import numpy as np
import sys
import argparse
import os
import math

# A4 dimensions in inches (portrait orientation)
A4_WIDTH_INCHES = 8.27
A4_HEIGHT_INCHES = 11.69

def calculate_grid_dimensions(num_images):
    """
    Calculate optimal grid dimensions (rows x cols) to maximize individual image size.
    Takes into account A4 page aspect ratio to determine the best layout.

    Args:
        num_images: Number of images to arrange

    Returns:
        Tuple of (rows, cols)
    """
    if num_images == 1:
        return (1, 1)

    # A4 aspect ratio (portrait)
    page_aspect_ratio = A4_WIDTH_INCHES / A4_HEIGHT_INCHES  # ~0.71

    best_rows = 1
    best_cols = num_images
    best_size = 0.0

    # Try all possible grid configurations
    for cols in range(1, num_images + 1):
        rows = math.ceil(num_images / cols)

        # Calculate the size each image would have with this grid
        # (accounting for margins and spacing)
        available_width = A4_WIDTH_INCHES * 0.9  # 90% for margins
        available_height = A4_HEIGHT_INCHES * 0.9  # 90% for margins

        # Account for spacing between images
        width_per_image = available_width / (cols + 0.05 * (cols - 1))
        height_per_image = available_height / (rows + 0.25 * (rows - 1))

        # The limiting dimension determines the actual size
        # (assuming square-ish images, use the smaller dimension)
        image_size = min(width_per_image, height_per_image)

        # Prefer layouts that better match the page aspect ratio
        # and maximize the image size
        if image_size > best_size:
            best_size = image_size
            best_rows = rows
            best_cols = cols

    return (best_rows, best_cols)

def extract_datetime_uid(filename):
    """
    Extract the datetime and UID portion from a filename.

    Expected format: prefix_YYYYMMDD-HHMMSS_UID.ext
    Returns: "YYYYMMDD-HHMMSS_UID" or None if not found

    Args:
        filename: Filename to parse

    Returns:
        String with datetime and UID, or None
    """
    import re
    # Match pattern: YYYYMMDD-HHMMSS_UID
    pattern = r'(\d{8}-\d{6}-[a-f0-9]+)'
    match = re.search(pattern, filename)
    if match:
        return match.group(1)
    return None

def extract_uid(filename):
    """
    Extract just the UID portion from a filename.

    Expected format: prefix_YYYYMMDD-HHMMSS_UID.ext
    Returns: just the UID portion (hex string after last dash)

    Args:
        filename: Filename to parse

    Returns:
        String with UID, or None if not found
    """
    import re
    # Match pattern: YYYYMMDD-HHMMSS-UID
    pattern = r'\d{8}-\d{6}-([a-f0-9]+)'
    match = re.search(pattern, filename)
    if match:
        return match.group(1)
    return None

def find_run_info_file(image_path):
    """
    Find the associated run-info txt file for a given image file.

    Args:
        image_path: Path to the PNG image file

    Returns:
        Path to run-info file, or None if not found
    """
    # Extract datetime and UID from the image filename
    datetime_uid = extract_datetime_uid(os.path.basename(image_path))
    if not datetime_uid:
        print(f"  Could not extract datetime/UID from: {os.path.basename(image_path)}")
        return None

    #print(f"  Looking for run-info with datetime/UID: {datetime_uid}")

    # Look in the same directory as the image
    directory = os.path.dirname(image_path)
    if not directory:
        directory = '.'

    # Search for run-info file with matching datetime and UID
    try:
        for filename in os.listdir(directory):
            if 'run-info' in filename and datetime_uid in filename and filename.endswith('.txt'):
                #print(f"  Found run-info file: {filename}")
                return os.path.join(directory, filename)
    except Exception as e:
        print(f"  Error searching directory: {e}")
        pass

    print(f"  No matching run-info file found in {directory}")
    return None

def read_final_emd(run_info_path):
    """
    Read the final EMD value from a run-info txt file.

    Args:
        run_info_path: Path to the run-info txt file

    Returns:
        EMD value as float, or None if not found
    """
    if not run_info_path or not os.path.isfile(run_info_path):
        return None

    try:
        with open(run_info_path, 'r') as f:
            for line in f:
                if line.startswith('Final EMD'):
                    # Get the last item on the line
                    parts = line.strip().split()
                    if parts:
                        try:
                            return float(parts[-1])
                        except ValueError:
                            pass
    except Exception as e:
        print(f"Warning: Could not read EMD from {run_info_path}: {e}", file=sys.stderr)

    return None

def load_images(image_files):
    """
    Load PNG images from files and find associated EMD values.

    Args:
        image_files: List of image file paths

    Returns:
        List of tuples (filepath, PIL Image, EMD value or None, UID or None)
    """
    images = []
    for filepath in image_files:
        if not os.path.isfile(filepath):
            print(f"Warning: File not found, skipping: {filepath}", file=sys.stderr)
            continue

        try:
            img = Image.open(filepath)
            # Convert to RGB if needed (in case of RGBA)
            if img.mode == 'RGBA':
                img = img.convert('RGB')

            # Find and read associated run-info file
            run_info_file = find_run_info_file(filepath)
            #print("Looking for run-info file for:", filepath)
            emd_value = read_final_emd(run_info_file)

            # Extract UID from filename
            uid = extract_uid(os.path.basename(filepath))

            #if run_info_file and emd_value is not None:
                #print(f"  Found EMD {emd_value:.4f} for {os.path.basename(filepath)}")

            images.append((filepath, img, emd_value, uid))
        except Exception as e:
            print(f"Warning: Could not load image {filepath}: {e}", file=sys.stderr)
            continue

    return images

def create_combined_figure(images, output_file):
    """
    Create a combined figure with all images arranged in a grid on A4 page.

    Args:
        images: List of tuples (filepath, PIL Image, EMD value or None, UID or None)
        output_file: Output file path
    """
    num_images = len(images)

    if num_images == 0:
        print("Error: No valid images to combine", file=sys.stderr)
        sys.exit(1)

    # Sort by EMD value (smallest first), putting images without EMD at the end
    # Keep all data: filepath, img, emd_value, uid
    sorted_images = sorted(images,
                          key=lambda x: (x[2] is None, x[2] if x[2] is not None else float('inf')))

    # Calculate grid dimensions
    rows, cols = calculate_grid_dimensions(num_images)

    print(f"Arranging {num_images} images in a {rows}x{cols} grid (sorted by EMD)")

    # Create figure with A4 dimensions
    fig = plt.figure(figsize=(A4_WIDTH_INCHES, A4_HEIGHT_INCHES))

    # Add a main title
    fig.suptitle('Combined Heatmaps (sorted by EMD)', fontsize=16, fontweight='bold', y=0.98)

    # Create grid spec with spacing to prevent text overlap between rows
    # hspace needs to be large enough for two-line titles (index + EMD)
    gs = gridspec.GridSpec(rows, cols, figure=fig,
                          left=0.05, right=0.95,
                          top=0.95, bottom=0.05,
                          wspace=0.05, hspace=0.25)

    # Add each image to the grid
    for grid_idx, (filepath, img, emd_value, uid) in enumerate(sorted_images):
        if grid_idx >= rows * cols:
            print(f"Warning: Too many images for grid, truncating at {rows * cols}", file=sys.stderr)
            break

        row = grid_idx // cols
        col = grid_idx % cols

        ax = fig.add_subplot(gs[row, col])

        # Display the image
        ax.imshow(np.array(img))

        # Use UID as title, or filename if UID not found
        if uid:
            title_text = uid
        else:
            # Fallback to filename if UID extraction failed
            title_text = os.path.splitext(os.path.basename(filepath))[0]

        if emd_value is not None:
            title_text += f"\nEMD: {emd_value:.4f}"
        ax.set_title(title_text, fontsize=8, fontweight='bold', pad=2)

        # Remove axis ticks and labels for cleaner appearance
        ax.set_xticks([])
        ax.set_yticks([])

        # Keep axis frame for better visual separation
        for spine in ax.spines.values():
            spine.set_edgecolor('gray')
            spine.set_linewidth(0.5)

    # Hide any unused subplots
    total_slots = rows * cols
    for idx in range(num_images, total_slots):
        row = idx // cols
        col = idx % cols
        ax = fig.add_subplot(gs[row, col])
        ax.axis('off')

    # Save the figure
    plt.savefig(output_file, dpi=150, bbox_inches='tight',
                pad_inches=0.1, format=None)  # format=None auto-detects from extension
    print(f"Combined figure saved to: {output_file}")

    plt.close()

def main():
    parser = argparse.ArgumentParser(
        description='Combine multiple heatmap PNG files into a single A4 page grid layout.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s output.pdf image1.png image2.png image3.png
  %(prog)s output.png target-heatmaps/*.png
  %(prog)s combined.pdf *.png

Output format is determined by the file extension (.png, .pdf, .jpg, etc.)
        """
    )

    parser.add_argument('output_file',
                       help='Output file (e.g., output.pdf or output.png)')
    parser.add_argument('input_files', nargs='+',
                       help='Input PNG files to combine')

    args = parser.parse_args()

    # Check output file extension
    output_ext = os.path.splitext(args.output_file)[1].lower()
    if not output_ext:
        print("Warning: No extension specified for output file, using .pdf", file=sys.stderr)
        args.output_file += '.pdf'

    # Load all images
    print(f"Loading {len(args.input_files)} image(s)...")
    images = load_images(args.input_files)

    if len(images) == 0:
        print("Error: No valid images found", file=sys.stderr)
        sys.exit(1)

    print(f"Successfully loaded {len(images)} image(s)")

    # Create combined figure
    create_combined_figure(images, args.output_file)

if __name__ == '__main__':
    main()
