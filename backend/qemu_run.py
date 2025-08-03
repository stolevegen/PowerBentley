#!/usr/bin/env python3

import subprocess
import sys
import os
import argparse

def run_qemu(firmware_path, extra_args=None):
    """Run QEMU with the ESP32 firmware"""
    
    if not os.path.exists(firmware_path):
        print(f"Error: Firmware file not found: {firmware_path}")
        sys.exit(1)
    
    print(f"Starting QEMU with firmware: {firmware_path}")
    print("Press Ctrl+A then X to exit QEMU")
    print("=" * 50)
    
    # Base QEMU command
    qemu_cmd = [
        "qemu-system-xtensa",
        "-nographic",
        "-machine", "esp32",
        "-serial", "mon:stdio",
        "-drive", f"file={firmware_path},if=mtd,format=raw"
    ]
    
    # Add any extra arguments
    if extra_args:
        qemu_cmd.extend(extra_args)
    
    try:
        # Run QEMU interactively
        subprocess.run(qemu_cmd)
    except KeyboardInterrupt:
        print("\nQEMU interrupted by user")
    except FileNotFoundError:
        print("Error: qemu-system-xtensa not found.")
        print("Make sure QEMU is installed and in your PATH.")
        print("On Ubuntu/Debian: apt install qemu-system-misc")
        print("On macOS: brew install qemu")
        sys.exit(1)
    except Exception as e:
        print(f"Error running QEMU: {e}")
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(description="Run ESP32 firmware in QEMU")
    parser.add_argument("firmware", nargs="?", 
                       default=".pio/build/esp32-qemu/qemu_firmware.bin",
                       help="Path to firmware binary (default: .pio/build/esp32-qemu/qemu_firmware.bin)")
    parser.add_argument("--debug", action="store_true",
                       help="Enable QEMU debugging")
    parser.add_argument("--gdb", action="store_true",
                       help="Wait for GDB connection on port 1234")
    
    args = parser.parse_args()
    
    extra_args = []
    
    if args.debug:
        extra_args.extend(["-d", "guest_errors,unimp"])
    
    if args.gdb:
        extra_args.extend(["-s", "-S"])
        print("QEMU will wait for GDB connection on port 1234")
        print("In another terminal, run: xtensa-esp32-elf-gdb")
        print("Then in GDB: target remote :1234")
    
    run_qemu(args.firmware, extra_args)

if __name__ == "__main__":
    main()