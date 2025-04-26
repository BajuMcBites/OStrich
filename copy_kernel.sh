#!/bin/bash

# Call the remove script to delete any existing kernel8.img
./remove_kernel_image.sh

# Define potential directories
DIR1="/run/media/diego/bootfs"
DIR2="/run/media/diego/bootfs1"

# Path to the kernel image in the build directory
KERNEL_IMAGE="build/kernel8.img"

# Function to copy kernel8.img to the directory
copy_kernel_image() {
  if [ -d "$1" ]; then
    echo "Copying $KERNEL_IMAGE to $1"
    cp "$KERNEL_IMAGE" "$1/"
    echo "Copied $KERNEL_IMAGE to $1"
  else
    echo "Directory $1 does not exist"
  fi
}

# Check and copy kernel8.img to both directories
copy_kernel_image "$DIR1"
copy_kernel_image "$DIR2"

# Eject the SD card
sudo eject /dev/sda  # Replace with the correct device identifier for your SD card
