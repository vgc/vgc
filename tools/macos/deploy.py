#!/usr/bin/env python3
#
# This script creates macOS appication bundles and packages.
#
# Versioning
# ----------
#
# CFBundleShortVersion and CFBundleVersion are in theory different, where the
# former is more like a public version, and the latter is more like an internal
# build number. See:
#
# https://stackoverflow.com/questions/19726988
#
# However, in our case, all alpha versions are publicly released, so we just use
# the same value for both. Here are the limitations on the format:
#
#   The build version number should be a string comprised of three non-negative,
#   period-separated integers with the first integer being greater than zero, for
#   example, 3.1.2. The string should only contain numeric (0-9) and period (.) characters.
#   Leading zeros are truncated from each integer and will be ignored (that is,
#   1.02.3 is equivalent to 1.2.3). The meaning of each element is as follows:
#
#   The first number represents the most recent major release and is limited to a maximum length of four digits.
#   The second number represents the most recent significant revision and is limited to a maximum length of two digits.
#   The third number represents the most recent minor bug fix and is limited to a maximum length of two digits.
#
# To work with these limitations, we use the following versioning scheme:
#
# - stable: fourDigitReleaseYear.minor
# - beta/alpha: (twoDigitCommitYear * 100 + commitMonth).commitDay.commitIndex
#
# Compared to the versioning scheme we use on Windows, this only supports commitIndex
# up to 99, instead of up to 999. This should be enough anyway.
#

from pathlib import Path
import argparse
import os
import re
import shutil
import subprocess

# Returns a value from an INI-formatted file. Does not support comments, nor
# spaces surrounding the equal size.
#
# Example:
# s = "a=Hello\nb=World"
# getIniValue(s, "b")      # -> "World"
#
def getIniValue(ini, key):
    return re.search("^" + key + "=(.*)$", ini, flags = re.MULTILINE).group(1)

# Deletes the symlink, regular file, or directory at the given path.
# If path is a symlink, the symlink itself is deleted (not whatever it points to).
# If path is a regular file, it is deleted.
# If path is a directory, it is recursively deleted.
# If path is another type of file (e.g.: socket, mount point, FIFO, block device, char device), an error is raised.
# If path doesn't exist, nothing happens.
#
def delete(path):
    if path.is_symlink() or path.is_file():
        path.unlink()
    elif path.is_dir():
        shutil.rmtree(str(path))
    elif path.exists():
        raise OSError('Cannot delete file: unsupported file type (e.g.: socket, mount point, FIFO, block device, char device)')

# Creates a directory at the given path.
# If path is already a directory, or a symlink to a directory, nothing happens.
# If path is a symlink (except to a directory), or another type of file, an error is raised,
# unless you specify force=True, in which case the symlink or file is deleted instead.
# Missing parent directories are automatically created.
#
def make_dir(path, *, force=False):
    if not path.is_dir():
        if path.is_symlink() or path.exists():
            if force:
                delete(path)
            else:
                raise FileExistsError('Cannot create directory: another file type already exists at the given path. Use force=True if you want to overwrite.')
        path.mkdir(parents=True, exist_ok=True)

# Return an integer representing whether the given path:
#  0: doesn't exist at all
#  1: is a symlink, including a broken symlink or symlink to directory
#  2: is a directory
#  3: is a regular file
#  4: is another type of file (socket, etc.)
#
def path_type(path):
    if path.is_symlink():
        return 1
    elif not path.exists():
        return 0
    elif path.is_dir():
        return 2
    elif path.is_file():
        return 3
    else:
        return 4

# Copy a symlink, regular file, or directory from src to dst.
#
# If src is a real directory (not a symlink), the copy is recursive.
#
# Missing parent directories of dst are automatically created.
#
# If dst already exists, then:
# - If overwrite=None, an error occurs.
# - If overwrite=SameType:
#   - If src and dst have the same type (e.g., regular file), dst is overwritten.
#   - If src and dst have different types, an error occurs. This prevents for
#     example a file to overwrite a whole directory by mistake.
# - If overwrite=All, dst is overwritten
#
# If src and dst are both real directories, then the meaning of "overwriting"
# depends on dirpolicy:
# - If dirpolicy=Merge, the content of src is recursively copied into dst. Files
#   and directories in src overwrite files and directories in dst (if allowed by
#   the overwrite policy), but files and directories in dst which don't exist in
#   src are preserved.
# - If dirpolicy=Replace, dst is completely deleted before the copy starts, such
#   that dst becomes an exact copy of src.
#
def copy(src, dst, *,
         overwrite="SameType",  # None | SameType | All
         dirpolicy="Merge"):    # Merge | Replace

    srcType = path_type(src)
    dstType = path_type(dst)

    if srcType == 0:
        raise FileNotFoundError('Cannot copy ' + str(src) + ': not found.')
    elif srcType == 4:
        raise OSError('Cannot copy ' + str(src) + ': unsupported file type (e.g.: socket).')
    elif dstType == 4:
        raise FileExistsError('Cannot copy as ' + str(dst) + ': unsupported file type (e.g.: socket) already exists.')

    if dstType > 0:
        if overwrite == "SameType" and srcType != dstType:
            raise FileExistsError('Cannot copy as ' + str(dst) + ': file or directory of different type already exists. Use overwrite="All" to overwrite.')
        elif overwrite == "None":
            raise FileExistsError('Cannot copy as ' + str(dst) + ': file or directory already exists. Use overwrite="SameType" to overwrite.')

        if dstType != 2 or srcType != dstType or dirpolicy == "Replace":
            delete(dst)

    if srcType == 2:
        make_dir(dst)
        for item in os.listdir(src):
            s = src / item
            d = dst / item
            copy(s, d, overwrite=overwrite, dirpolicy=dirpolicy)
    else:
        make_dir(dst.parent)
        shutil.copy2(str(src), str(dst))

# Writes the given text in the given file.
# Missing parent directories are automatically created.
#
def write_text(text, file):
    make_dir(file.parent)
    file.write_text(text)

# Returns a string of whitespaces for indentation purposes, where indent must be an
# integer specifying the desired indent level.
#
def plist_indent(indent):
    return "    " * indent

# Returns the given Python plist data as an XML string.
#
def plist_to_text(plist, indent=0):
    if isinstance(plist, dict):
        text = plist_indent(indent) + "<dict>\n"
        for k, v in plist.items():
            text += plist_indent(indent + 1) + "<key>" + k + "</key>\n"
            text += plist_to_text(v, indent + 1)
        text += plist_indent(indent) + "</dict>\n"
    elif isinstance(plist, list):
        text = plist_indent(indent) + "<array>\n"
        for v in plist:
            text += plist_to_text(v, indent + 1)
        text += plist_indent(indent) + "</array>\n"
    elif isinstance(plist, str):
        text = plist_indent(indent) + "<string>" + plist + "</string>\n"
    else:
        raise ValueError("plist type not supported.")
    return text

# Writes the given Python plist data to a file.
#
def write_plist(plist, file):
    text = '<?xml version="1.0" encoding="UTF-8"?>\n'
    text += '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n'
    text += '<plist version="1.0">\n'
    text += plist_to_text(plist)
    text += '</plist>\n'
    write_text(text, file)

# Script entry point.
#
if __name__ == "__main__":

    # Parse arguments, and import them into the global namespace
    parser = argparse.ArgumentParser()
    parser.add_argument("srcDir", help="path to the source directory")
    parser.add_argument("buildDir", help="path to the build directory")
    parser.add_argument("config", help="build configuration (empty string in mono-config builds, or 'Release', 'Debug', etc. in multi-config builds)")
    parser.add_argument("qtDir", help="path the the Qt library, e.g.: '/Users/boris/Qt/5.12.5/clang_64'")
    args = parser.parse_args()
    globals().update(vars(args))

    # Build and deploy directory structure
    srcDir = Path(srcDir)
    rootBuildDir = Path(buildDir)
    buildDir = rootBuildDir / config
    deployDir = rootBuildDir / "deploy" / config
    qtDir = Path(qtDir)

    # Get version info
    versionFile = buildDir / "resources" / "core" / "version.txt"
    version = versionFile.read_text()
    manufacturer = getIniValue(version, "manufacturer")
    suiteName = getIniValue(version, "suite")
    versionType = getIniValue(version, "versionType")
    versionMajor = getIniValue(version, "versionMajor")
    versionMinor = getIniValue(version, "versionMinor")
    versionName = getIniValue(version, "versionName")
    commitBranch = getIniValue(version, "commitBranch")
    commitDate = getIniValue(version, "commitDate")
    commitIndex = getIniValue(version, "commitIndex")
    architecture = getIniValue(version, "buildArchitecture")

    # Compute branchSuffix
    # Example: "-gh62"
    branchSuffix = ""
    if ((versionType == "beta" and commitBranch != versionMajor) or
        (versionType == "alpha" and commitBranch != "master")):
        branchSuffix = commitBranch

    # Compute bundleVersion. Examples:
    #   "Stable 2019.1" => "2019.1"
    #   "Alpha 2019-07-08.1" => "1907.8.1"
    if versionType == "stable":
        major = int(versionMajor[0:4])
        minor = int(versionMinor)
        bundleVersion = "{}.{}".format(major, minor)
    elif versionType in ["beta", "alpha"]:
        year = int(commitDate[2:4])
        month = int(commitDate[5:7])
        day = int(commitDate[8:10])
        index = int(commitIndex)
        bundleVersion = "{}.{}.{}".format(year * 100 + month, day, index)

    # Identifiers
    baseUrl = "https://www.vgc.io"
    idBase = "io.vgc."
    typeIdBase = idBase
    if versionType == "stable":
        appIdBase = idBase
    elif versionType == "beta":
        appIdBase = idBase + versionMajor + "-beta" + branchSuffix + "."
    elif versionType == "alpha":
        appIdBase = idBase + "alpha" + branchSuffix + "."

    # List of all apps of the suite to install
    appNames = [ "Illustration" ]

    # Create Desktop and Start Menu shortcuts
    for appName in appNames:

        # Various names and version strings
        suiteAppName = suiteName + " " + appName
        bundleDirBasename = suiteAppName + " " + versionName
        bundleDirName = bundleDirBasename + ".app"
        suiteNameLower = suiteName.lower()
        appNameLower = appName.lower()
        fileExtension = suiteNameLower + appNameLower[0]

        # Bundle directory structure
        bundleDir = deployDir / bundleDirName
        bundleContentsDir = bundleDir / "Contents"
        bundleMacOSDir = bundleContentsDir / "MacOS"
        bundleResourcesDir = bundleContentsDir / "Resources"
        infoFile = bundleContentsDir / "Info.plist"

        # Start fresh
        delete(bundleDir)

        # Copy executable
        appExecutableBasename = "vgc" + appNameLower
        bundleExecutable = "bin/" + appExecutableBasename
        copy(buildDir / bundleExecutable, bundleMacOSDir / bundleExecutable)

        # Copy resources
        copy(buildDir / "resources", bundleMacOSDir / "resources")

        # Copy bundle icons
        appIcnsName = appExecutableBasename + ".icns"
        fileExtensionIcnsName = fileExtension + ".icns"
        copy(srcDir / "apps" / appNameLower / appIcnsName, bundleResourcesDir / appIcnsName)
        copy(srcDir / "apps" / appNameLower / fileExtensionIcnsName, bundleResourcesDir / fileExtensionIcnsName)

        # XXX Shouldn't we get rid of the bin folder?
        # XXX Shouldn't the resources be in <name>.app/Contents/Resources?
        #
        # Currently, the executable expects to be in a folder named "bin", and
        # have its resources in a sibling folder called "resources". However, we
        # could have a folder hierarchy more idiomatic to each platform by implementing
        # a "smarter" way to locate resources.

        # Write Info.plist
        info = {
            "CFBundleDevelopmentRegion": "en-US",
            "CFBundleDisplayName": bundleDirBasename,
            "CFBundleDocumentTypes": [
                {
                    "CFBundleTypeExtensions": [
                        fileExtension
                    ],
                    "CFBundleTypeIconFile": fileExtensionIcnsName,
                    "CFBundleTypeName": suiteAppName + " file",
                    "CFBundleTypeRole": "Editor",
                    "LSItemContentTypes": [
                        typeIdBase + fileExtension
                    ],
                    "LSHandlerRank": "Owner",
                }
            ],
            "CFBundleExecutable": bundleExecutable,
            "CFBundleIconFile": appIcnsName,
            "CFBundleIdentifier": appIdBase + appNameLower,
            "CFBundleInfoDictionaryVersion": "6.0",
            "CFBundleName": suiteAppName,
            "CFBundlePackageType": "APPL",
            "CFBundleShortVersionString": bundleVersion,
            "CFBundleSignature": "????", # TODO: sign the packages
            "CFBundleVersion": bundleVersion,
            "UTExportedTypeDeclarations": [
                {
                    "UTTypeIdentifier": typeIdBase + fileExtension,
                    "UTTypeTagSpecification": {
                        "public.filename-extension": [
                            fileExtension
                        ],
                    },
                    "UTTypeConformsTo": [
                        "public.xml"
                    ],
                    "UTTypeIconFile": fileExtensionIcnsName,
                    "UTTypeDescription": suiteAppName + " file",
                    "UTTypeReferenceUrl": baseUrl, # TODO: have a more specific url where the file type is described
                }
            ]
        }
        write_plist(info, infoFile)

        # Execute macdeployqt
        #
        # Note: for now, in my personal machine, there is the following error:
        #
        #   ERROR: no file at "/usr/local/opt/python/lib/Python.framework/Versions/3.7/Python"
        #
        # Indeed, Python is at:
        #   /usr/local/Frameworks/Python.framework/Versions/3.7/Python,
        # Or equivalently at (symlink):
        #   /usr/local/opt/python/Frameworks/Python.framework/Versions/3.7
        # not at the path above.
        #
        # This seems to be an issue with macdeployqt which doesn't corresctly identitfy
        # the correct path for python.
        #
        # See the following for potential solutions or workarounds:
        #   https://forum.qt.io/topic/76932/unable-to-shipping-with-python/12
        #   https://stackoverflow.com/questions/2809930/macdeployqt-and-third-party-libraries
        #   https://stackoverflow.com/questions/35612687/cmake-macos-x-bundle-with-bundleutiliies-for-qt-application
        #   https://github.com/bvschaik/julius/issues/100
        #   https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/platform_dependent_issues/Bundles-And-Frameworks
        #   https://github.com/Homebrew/homebrew-core/issues/3219
        #
        subprocess.run([
            str(qtDir / "bin" / "macdeployqt"),
            str(bundleDir),
            "-always-overwrite"])

        # Add correct rpath to the executable.
        # By default, macdeployqt adds @executable_path/../Frameworks, but it is incorrect since
        # our executable is nested one folder deeper. You can run "otool -l <executable>" and look
        # for "LC_RPATH" in order to find all the paths added to the rpath.
        #
        subprocess.run([
            "install_name_tool", "-add_rpath",
            "@executable_path/../../Frameworks",
            str(bundleMacOSDir / bundleExecutable)])
