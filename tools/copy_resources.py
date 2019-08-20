#!/usr/bin/env python3

from pathlib import Path
import argparse
import shutil

# Returns the last modification time of the given Path.
#
def mtime(path):
    return path.stat().st_mtime

# Copies a file from the given src Path to the given dest Path, creating
# directories as necessary.
#
def copyFile(src, dest):
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(str(src), str(dest))

# Returns resources as a set of strings (= relative paths) from a
# comma-separated text file given by its Path.
#
def getResources(path):
    res = set()
    if path.is_file():
        text = path.read_text().replace("\n", "")
        if text != "":
            res = set(text.split(";"))
    return res

# Deletes the given resource if it exists, and recursively deletes its parent
# directories if they are empty.
#
def removeResource(destPath, r):
    path = destPath / r
    try:
        path.unlink()
        print("Deleted " + r)
        path = path.parent
        while path:
            path.rmdir()
            path = path.parent
    except:
        pass

# Copies the given resource from srcPath to destPath, unless it already exists
# and is not outdated.
#
def updateResource(srcPath, destPath, r):
    src = srcPath / r
    dest = destPath / r
    if not dest.is_file():
        copyFile(src, dest)
        print("Copied " + r)
    elif mtime(dest) < mtime(src):
        copyFile(src, dest)
        print("Updated " + r)

# Script entry point.
#
if __name__ == "__main__":

    # Parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument("srcDir", help="path to the source directory")
    parser.add_argument("buildDir", help="path to the build directory")
    parser.add_argument("config", help="build configuration (e.g., 'Release')")
    parser.add_argument("targetType", help="either 'lib' or 'app'")
    parser.add_argument("targetName", help="the name of this VGC target (e.g., 'core', 'illustration')")
    args = parser.parse_args()

    # Import arguments into the global namespace.
    # This allows to more simply write `foo` instead of `args.foo`.
    globals().update(vars(args))

    # Get directory paths
    if targetType == "lib":
        targetBuildPath = Path(buildDir) / "libs" / "vgc" / targetName
        srcPath = Path(srcDir) / "libs" / "vgc" / targetName
        destPath = Path(buildDir) / config / "resources" / targetName
    elif targetType == "app":
        targetBuildPath = Path(buildDir) / "apps" / targetName
        srcPath = Path(srcDir) / "apps" / targetName
        destPath = Path(buildDir) / config / "resources" / "apps" / targetName
    else:
        raise ValueError(
            'Unknown targetType: "' + targetType + '".' +
            ' Allowed values are "lib" and "app".')

    # Update the build tree by copying all outdated resources.
    ref = targetBuildPath / "resources.txt"
    stamp = targetBuildPath / ("resources_" + config + ".txt")
    if not stamp.is_file() or mtime(stamp) < mtime(ref):
        r1 = getResources(ref)
        r2 = getResources(stamp)
        for r in r2 - r1:
            removeResource(destPath, r)
        for r in r1:
            updateResource(srcPath, destPath, r)
        copyFile(ref, stamp)
