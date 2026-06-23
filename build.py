import subprocess
import os
import sys
import multiprocessing
import argparse
import glob

def run_command(command, msg):
    print(f"Executing: {msg}")
    try:
        subprocess.check_call(command, shell=True)
    except subprocess.CalledProcessError as e:
        print(f"Error during {msg}: {e}")
        sys.exit(1)

def run_format():
    """Runs clang-format while ignoring the vendor directory."""
    print("--- Formatting source code (ignoring vendor/) ---")
    extensions = ("**/*.cpp", "**/*.hpp", "**/*.c", "**/*.h")
    all_files = []
    for ext in extensions:
        all_files.extend(glob.glob(ext, recursive=True))

    # Filter out files that reside in the 'vendor' directory
    files_to_format = [
        f for f in all_files 
        if not f.startswith("vendor" + os.sep) and os.sep + "vendor" + os.sep not in f
    ]

    if not files_to_format:
        print("No source files found to format (excluding vendor).")
        return

    format_cmd = f"clang-format -i -style=file {' '.join(files_to_format)}"
    run_command(format_cmd, f"Formatting {len(files_to_format)} files")

def main():
    parser = argparse.ArgumentParser(description="Archimedes Build System")
    parser.add_argument(
        "--format",
        action="store_true",
        help="Run clang-format on the source code (ignoring vendor/)"
    )
    args = parser.parse_args()

    build_dir = "build"
    cores = multiprocessing.cpu_count()

    print("--- Archimedes Python Build System ---")

    if args.format:
        run_format()

    if not os.path.exists(build_dir):
        config_cmd = (
            f"cmake -G Ninja -B {build_dir} "
            f"-DCMAKE_C_COMPILER=clang "
            f"-DCMAKE_CXX_COMPILER=clang++ "
            f"-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
        )
        run_command(config_cmd, "Initial CMake Configuration")

    build_cmd = f"cmake --build {build_dir} -j {cores}"
    run_command(build_cmd, "Compilation")

    print(f"\n--- Build Complete [using {cores} cores] ---")

if __name__ == "__main__":
    main()