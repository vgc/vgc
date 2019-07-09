#!/usr/bin/env python3

from pathlib import Path
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

# Updates the build tree by copying all outdated resources.
#
def run(srcDir, buildDir, config, libName):
    libBuildPath = Path(buildDir) / "libs" / "vgc" / libName
    ref = libBuildPath / "resources.txt"
    stamp = libBuildPath / ("resources_" + config + ".txt")
    if not stamp.is_file() or mtime(stamp) < mtime(ref):
        srcPath = Path(srcDir) / "libs" / "vgc" / libName
        destPath = Path(buildDir) / config / "resources" / libName
        r1 = getResources(ref)
        r2 = getResources(stamp)
        for r in r2 - r1:
            removeResource(destPath, r)
        for r in r1:
            updateResource(srcPath, destPath, r)
        copyFile(ref, stamp)
