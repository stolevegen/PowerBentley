#!/usr/bin/env python3

Import("env")
import subprocess
import os
from SCons.Script import ARGUMENTS

def create_qemu_firmware(source, target, env):
    """Custom build step to create QEMU firmware binary"""
    
    # Get build directory from environment
    build_dir = env.subst("$BUILD_DIR")
    
    # Define paths
    bootloader_bin = os.path.join(build_dir, "bootloader.bin")
    partitions_bin = os.path.join(build_dir, "partitions.bin")
    firmware_bin = os.path.join(build_dir, "firmware.bin")
    qemu_firmware_bin = os.path.join(build_dir, "qemu_firmware.bin")
    
    # Check if required files exist
    required_files = [bootloader_bin, partitions_bin, firmware_bin]
    for file_path in required_files:
        if not os.path.exists(file_path):
            print(f"Warning: Required file not found: {file_path}")
            return
    
    # Build esptool command
    cmd = [
        "esptool.py",
        "--chip", "esp32",
        "merge_bin",
        "--flash_mode", "dio",
        "--flash_freq", "40m",
        "--flash_size", "4MB",
        "--fill-flash-size", "4MB",
        "-o", qemu_firmware_bin,
        "0x1000", bootloader_bin,
        "0x8000", partitions_bin,
        "0x10000", firmware_bin
    ]
    
    try:
        print("Creating QEMU firmware binary...")
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        print(f"QEMU firmware binary created: {qemu_firmware_bin}")
    except subprocess.CalledProcessError as e:
        print(f"Error creating QEMU firmware: {e}")
        print(f"stderr: {e.stderr}")
    except FileNotFoundError:
        print("Error: esptool.py not found. Make sure it's installed.")

# Add custom target that runs after build
env.AddPostAction("$BUILD_DIR/firmware.bin", create_qemu_firmware)