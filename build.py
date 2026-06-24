import subprocess
import os
import sys
import multiprocessing
import argparse
from pathlib import Path

def run_command(command, msg, shell=False):
    """Executes a command and handles errors."""
    print(f"Executing: {msg}")
    try:
        # shell=False is safer and avoids issues with spaces in paths
        subprocess.check_call(command, shell=shell)
    except subprocess.CalledProcessError as e:
        print(f"Error during {msg}: {e}")
        sys.exit(1)
    except FileNotFoundError:
        print(f"Error: Command not found. Ensure required tools are installed.")
        sys.exit(1)

def run_format():
    """Runs clang-format while ignoring the vendor directory."""
    print("--- Formatting source code (ignoring vendor/) ---")
    
    extensions = ("*.cpp", "*.hpp", "*.c", "*.h")
    all_files = []
    
    # Use pathlib for more robust path globbing
    for ext in extensions:
        all_files.extend(Path(".").rglob(ext))

    # Filter out files in 'vendor' directory
    files_to_format = [
        str(f) for f in all_files 
        if "vendor" not in f.parts
    ]

    if not files_to_format:
        print("No source files found to format (excluding vendor).")
        return

    # Use batching to prevent 'Argument list too long' errors
    # Process 50 files at a time
    batch_size = 50
    for i in range(0, len(files_to_format), batch_size):
        batch = files_to_format[i : i + batch_size]
        format_cmd = ["clang-format", "-i", "-style=file"] + batch
        run_command(format_cmd, f"Formatting batch {i//batch_size + 1}")

def main():
    parser = argparse.ArgumentParser(description="Archimedes Build System")
    parser.add_argument(
        "--format",
        action="store_true",
        help="Run clang-format on the source code (ignoring vendor/)"
    )
    args = parser.parse_args()

    build_dir = "build"
    cores = str(multiprocessing.cpu_count())

    print("--- Archimedes Python Build System ---")

    if args.format:
        run_format()

    ninja_path = "/opt/homebrew/bin/ninja"
    if not os.path.exists(build_dir):
        config_cmd = [
            "cmake", 
            "-G", "Ninja", 
            "-B", build_dir,
            f"-DCMAKE_MAKE_PROGRAM={ninja_path}", # This forces use of the system PATH
            "-DCMAKE_C_COMPILER=clang",
            "-DCMAKE_CXX_COMPILER=clang++",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
        ]
        run_command(config_cmd, "Initial CMake Configuration")
    else:
        reconfig_cmd = [
            "cmake",
            "-B", build_dir,
            f"-DCMAKE_MAKE_PROGRAM={ninja_path}"
        ]
        run_command(reconfig_cmd, "UPDATING NINJA PATH")
    # Compilation
    build_cmd = ["cmake", "--build", build_dir, "-j", cores]
    run_command(build_cmd, "Compilation")

    print(f"\n--- Build Complete [using {cores} cores] ---")

if __name__ == "__main__":
    main()
