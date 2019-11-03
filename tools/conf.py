#!/usr/bin/env python3
#
# Generates a file called vgc.conf, telling VGC executables about the location
# of various runtime resources.
#
# This makes it possible to abstract away differences in folder hierarchies
# depending on the platform, allows users to customize these in case of unusual
# configurations, and makes it easier to create relocatable packages.
#
# Alternatives are either to bake these paths into the binary files (hard to
# inspect and change), or use obscure and complex heuristic to try to
# automatically guess these paths at runtime (error-prone and non-generic).
#
# "Explicit is better than implicit."
#                    -- Zen of Python
#

from pathlib import Path
import argparse

# Script entry point.
#
if __name__ == "__main__":

    # Parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument("srcDir")
    parser.add_argument("buildDir")
    parser.add_argument("buildConfig")
    parser.add_argument("pythonHome")
    args = parser.parse_args()

    # Import arguments into the global namespace.
    # This allows to more simply write `foo` instead of `args.foo`.
    globals().update(vars(args))

    # Create the vgc.conf string.
    #
    # Note that we use forward slashes for paths even on Windows, otherwise
    # QSettings can't read it. See https://doc.qt.io/qt-5/qsettings.html:
    #
    #   QSettings always treats backslash as a special character and provides no
    #   API for reading or writing such entries.
    #
    res = ""
    buildDir = buildDir.replace("\\", "/");
    pythonHome = pythonHome.replace("\\", "/");
    res += f"BasePath = {buildDir}/{buildConfig}\n"
    res += f"PythonHome = {pythonHome}\n"

    # Write to file
    binDir = Path(buildDir) / buildConfig / "bin"
    binDir.mkdir(parents=True, exist_ok=True)
    confPath = binDir / "vgc.conf"
    confPath.write_text(res)

    # Print to console
    print(f"\n{confPath}:\n{res}", flush=True)
