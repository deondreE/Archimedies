import os
import subprocess
import platform

if platform.system == "Windows":
    print("\nRunning Premake...")

    subprocess.call([os.path.abspath("./scripts/generate.bat")])

    print("\n Setup completed")