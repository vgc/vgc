#!/usr/bin/env python3
#
# This script assumes that an "embedded" version of Python is installed in the
# "embed" subdirectory of the current Python installation. This is not the case
# by default: you must manually download the embedded version and extract the
# ZIP. In the future, we may want to automatically download it. For example, the
# address for Python 3.7.3 is the following:
#
# https://www.python.org/ftp/python/3.7.3/python-3.7.3-embed-amd64.zip
#
# Note that we do not deploy pip as part of the installation, since it does take
# quite a bit of space (> 8 MB), and using pip together with an "embedded Python"
# isn't officially supported. If we do want to ship pip in a future version,
# please see the following as a starting point on how to do this:
#
# https://stackoverflow.com/questions/42666121/pip-with-embedded-python
#
# Note that instead of "shipping VGC with pip", one option could be to ship a
# script that downloads get-pip.py, runs it, and adds to pythonXY._pth:
#
#   Lib/site-packages
#   import site
#

import sys
from pathlib import Path
import shutil
import zipfile
import urllib.request

# Copies a file from the given src Path to the given dest Path, creating
# directories as necessary.
#
def copyFile(src, dest):
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(str(src), str(dest))

# Updates the build tree by copying all outdated resources.
#
def run(buildDir, config):
    buildDir = Path(buildDir)
    pythonDir = Path(sys.executable).parent
    embedDir = pythonDir / "embed"
    binDir = buildDir / config / "bin"

    # Get version-dependent python name, e.g.:
    #   pythonXY   = "python37"
    #   versionXYZ = "3.7.4"
    version = sys.version_info
    pythonXY = "python{}{}".format(version.major, version.minor)
    versionXYZ = "{}.{}.{}".format(version.major, version.minor, version.micro)

    # We first look inside the python installation to see if the embed folder is
    # there. If not, we download the embed folder directly in the build dir.
    if not embedDir.exists():
        embedDir = buildDir / "embed"
        if not embedDir.exists():
            embedZipFileName = "python-{}-embed-amd64.zip".format(versionXYZ)
            embedZipUrl = "https://www.python.org/ftp/python/{}/{}".format(versionXYZ, embedZipFileName)
            embedZipPath = buildDir / embedZipFileName
            if not embedZipPath.exists():
                print("Downloading {}".format(embedZipUrl))
                print("         to {}".format(str(embedZipPath)))
                urllib.request.urlretrieve(embedZipUrl, str(embedZipPath))
                print("Done.")
            embedZipFile = zipfile.ZipFile(str(embedZipPath), 'r')
            embedZipFile.extractall(str(embedDir))
            embedZipFile.close()

    # Extract Python built-in libraries to <build>/<config>/python
    # We don't keep them zipped for runtime performance
    zipPath = embedDir / (pythonXY + ".zip")
    zipFile = zipfile.ZipFile(str(zipPath), 'r')
    zipFile.extractall(str(buildDir / config / "python"))
    zipFile.close()

    # Copy all other files to <build>/<config>/python
    for child in embedDir.iterdir():
        if child != zipPath and child.is_file():
            copyFile(child, binDir / child.name)

    # Modify the default pythonXY._pth file:
    # - add correct location of python libs
    # - uncomment 'import site' so that functions like exit() and help() work
    pthPath = binDir / (pythonXY + "._pth")
    pthPath.write_text("../python\n.\nimport site")
