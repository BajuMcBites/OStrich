#!/bin/bash

# MAY NOT WORK ON WINDOWS.
# make sure to chmod +x gen_sd_image.sh if needed.
# Usage: ./gen_sd_image.sh <size_in_MB>
# Example: ./gen_sd_image.sh 8 => 'sdcard_8MB.dd'

# Check if size argument is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <size_in_MB>"
    exit 1
fi

size=$1

# Check if size is a power of 2
if ! ((size > 0)) || ((size & (size - 1))); then
    echo "Error: Size must be a power of 2"
    exit 1
fi

filename="sdcard_${size}MB.dd"

# Create empty file of specified size
dd if=/dev/zero of=$filename bs=1M count=$((size))

echo "Created $filename of size ${size}MB."
