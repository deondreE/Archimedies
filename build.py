import subprocess
import os
import sys
import multiprocessing
import shutil

def run_command(command, msg):
    print(f"Executing: {msg}")
    try:
        subprocess.check_call(command, shell=True)
    except subprocess.CalledProcessError as e:
        print(f"Error during {msg}: {e}")
        sys.exit(1)

def main():
    build_dir = "build"
    cores = multiprocessing.cpu_count()

    print("--- Archimedes Python Build System ---")

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