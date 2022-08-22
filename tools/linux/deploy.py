#!/usr/bin/env python3
#
# This script creates Linux AppImage packages from a locally built version
# of VGC.
#
# For now, we create one AppImage per app (e.g., Vgc Illustration, VGC
# Animation, etc.). In the future, it may be useful to look at how LibreOffice
# handles Linux packaging to bundle the whole application suite into one
# AppImage:
# - https://github.com/AppImage/AppImageKit/wiki/Bundling-LibreOffice
# - https://www.libreoffice.org/download/appimage/
#
# Versioning
# ----------
#
# For stable versions, we use semantic versioning (https://semver.org):
#   VGC 2020.0  ->  2020.0
#   VGC 2020.1  ->  2020.1  (    binary compatible with VGC 2020.0)
#   VGC 2021.0  ->  2021.0  (not binary compatible with VGC 2020.0)
#
# For alpha versions, we never guarantee binary compatibility. We use the
# following versioning scheme:
#   VGC Alpha 2019-07-08.1  ->  2019.7.8.1
#
# For beta versions, we consider each beta branch (VGC 2020 Beta, VGC 2021 Beta,
# etc.) to be a completely separate library / app. Within each beta branch,
# binary compatibility is preserved *after the first stable version on this
# branch is out*. We use the following versioning scheme:
#   VGC 2020 Beta 2019-07-08.1  ->  2019.7.8.1
#

from pathlib import Path
import argparse
import io
import json
import mimetypes
import os
import re
import shlex
import shutil
import subprocess
import sys
import urllib.parse
import urllib.request
import uuid

# Returns a value from an INI-formatted file. Does not support comments, nor
# spaces surrounding the equal size.
#
# Example:
# s = "a=Hello\nb=World"
# getIniValue(s, "b")      # -> "World"
#
def getIniValue(ini, key):
    return re.search("^" + key + "=(.*)$", ini, flags = re.MULTILINE).group(1)

# Prints with flush=True by default. Using flush=True tends to behave better and
# prevent truncating and ordering issues in TravisCI and perhaps other systems
# which process and redirect the output.
#
def print_(*objects, sep=' ', end='\n', file=sys.stdout, flush=True):
    print(*objects, sep=sep, end=end, file=file, flush=flush)

# Deletes the symlink, regular file, or directory at the given path.
# If path is a symlink, the symlink itself is deleted (not whatever it points to).
# If path is a regular file, it is deleted.
# If path is a directory, it is recursively deleted.
# If path is another type of file (e.g.: socket, mount point, FIFO, block device, char device), an error is raised.
# If path doesn't exist, nothing happens.
#
def delete(path, *, verbose=False):
    if path.is_symlink():
        if verbose:
            print_(f"             Deleting symlink {path}\n")
        path.unlink()
    elif path.is_file():
        if verbose:
            print_(f"        Deleting regular file {path}\n")
        path.unlink()
    elif path.is_dir():
        if verbose:
            print_(f"Recusively deleting directory {path}\n")
        shutil.rmtree(str(path))
    elif path.exists():
        raise OSError(f"Cannot delete file {path}: unsupported file type (e.g.: socket, mount point, FIFO, block device, char device)")

# Creates a directory at the given path.
# If path is already a directory, or a symlink to a directory, nothing happens.
# If path is a symlink (except to a directory), or another type of file, an error is raised,
# unless you specify force=True, in which case the symlink or file is deleted instead.
# Missing parent directories are automatically created.
#
def make_dir(path, *, force=False, verbose=False):
    if not path.is_dir():
        if path.is_symlink() or path.exists():
            if force:
                delete(path, verbose=verbose)
            else:
                raise FileExistsError(f"Cannot create directory {path}: another file type already exists at the given path. Use force=True if you want to overwrite.")
        if verbose:
            print_(f"           Creating directory {path}\n")
        path.mkdir(parents=True)

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

# Determine whether follow_symlinks=False is supported for stat and chmod.
#
# Note that follow_symlinks=True is always supported, that is, it is always
# possible to get/set permissions of the targeted file. However, it is not
# always possible to get/set permissions of the symlink itself, since different
# platforms support different symlink features. In particular, Linux doesn't
# support setting permissions for symlinks, while macOS does. See:
#
# https://superuser.com/questions/303040/how-do-file-permissions-apply-to-symlinks
#
_stat_supports_symlinks = os.stat in os.supports_follow_symlinks
_chmod_supports_symlinks = os.chmod in os.supports_follow_symlinks

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
         dirpolicy="Merge",     # Merge | Replace
         verbose=False,
         recursiveVerbose=False):

    srcType = path_type(src)
    dstType = path_type(dst)

    if srcType == 0:
        raise FileNotFoundError(f"Cannot copy {src}: not found.")
    elif srcType == 4:
        raise OSError(f"Cannot copy {src}: unsupported file type (e.g.: socket).")
    elif dstType == 4:
        raise FileExistsError(f"Cannot copy as {dst}: unsupported file type (e.g.: socket) already exists.")

    if dstType > 0:
        if overwrite == "SameType" and srcType != dstType:
            raise FileExistsError(f"Cannot copy as {dst}: file or directory of different type already exists. Use overwrite=\"All\" to overwrite.")
        elif overwrite == "None":
            raise FileExistsError(f"Cannot copy as {dst}: file or directory already exists. Use overwrite=\"SameType\" to overwrite.")

        if dstType != 2 or srcType != dstType or dirpolicy == "Replace":
            delete(dst, verbose=verbose)

    if srcType == 2:
        if verbose and not recursiveVerbose:
            print_(f"          Recursively copying {src}\n" +
                   f"                           to {dst}\n")
        newVerbose = verbose and recursiveVerbose
        make_dir(dst, verbose=newVerbose)
        for item in os.listdir(src):
            s = src / item
            d = dst / item
            copy(s, d, overwrite=overwrite, dirpolicy=dirpolicy, verbose=newVerbose, recursiveVerbose=recursiveVerbose)
    else:
        # Notes:
        # - Does not preserve owner/group
        # - Applies u+w,go-w to file permissions (unless we can't, e.g., symlinks on Linux)
        make_dir(dst.parent, verbose=verbose)
        if verbose:
            print_(f"                      Copying {src}\n" +
                   f"                           to {dst}")
        shutil.copy2(str(src), str(dst), follow_symlinks=False)

        # Change permissions (unless we're copying a symlink, in which case we
        # only set permissions if supported)
        notSymlink = (srcType != 1)
        symlinksSupportPermissions = (_stat_supports_symlinks and _chmod_supports_symlinks)
        follow_symlinks = not symlinksSupportPermissions
        if notSymlink or symlinksSupportPermissions:
            oldPermissions = os.stat(dst, follow_symlinks=follow_symlinks).st_mode & 0o777
            newPermissions = oldPermissions
            newPermissions |= 0o200  # u+w
            newPermissions &= ~0o022 # go-w
            if newPermissions != oldPermissions:
                if verbose:
                    old = oct(oldPermissions)[2:]
                    new = oct(newPermissions)[2:]
                    print_(f"                              (changing permissions from {old} to {new})")
                os.chmod(dst, newPermissions, follow_symlinks=follow_symlinks)
            if verbose:
                print_("") # final newline

# Makes a POST request to the given URL with the given data.
# The given data should be a Python dictionary, which this function
# automatically encodes as JSON. Finally, the JSON response is decoded
# and returned as a python dictionary.
#
def post_json(url, data):
    databytes = json.dumps(data).encode("utf-8")
    request = urllib.request.Request(url)
    request.method = "POST"
    request.add_header("Content-Type", "application/json; charset=utf-8")
    response = urllib.request.urlopen(request, databytes)
    encoding = response.info().get_param("charset") or "utf-8"
    return json.loads(response.read().decode(encoding))

# Makes a multipart POST request to the given URL with the given
# fields and files. The JSON response is decoded
# and returned as a python dictionary.
#
# fields should be a dictionary of the form:
# {
#     "name1": "value1",
#     "name2": "value2"
# }
#
# and files should be a dictionary of the form:
# {
#     "name3": filepath3,
#     "name4": filepath4
# }
#
# where filepaths are of type pathlib.Path.
#
def post_multipart(url, fields, files):
    boundary = uuid.uuid4().hex
    bBoundary = boundary.encode("utf-8")
    data = io.BytesIO()
    for name, value in fields.items():
        bName = name.encode("utf-8")
        bValue = value.encode("utf-8")
        data.write(b"--" + bBoundary + b"\r\n")
        data.write(b"Content-Disposition: form-data; name=\"" + bName + b"\"\r\n")
        data.write(b"\r\n")
        data.write(bValue)
        data.write(b"\r\n")
    for name, filepath in files.items():
        filename = filepath.name
        mimetype = mimetypes.guess_type(filename)[0] or "application/octet-stream"
        bName = name.encode("utf-8")
        bFilename = filename.encode("utf-8")
        bMimetype = mimetype.encode("utf-8")
        data.write(b"--" + bBoundary + b"\r\n")
        data.write(b"Content-Disposition: form-data; name=\"" + bName + b"\"; filename=\"" + bFilename + b"\"\r\n")
        data.write(b"Content-Type: " + bMimetype + b"\r\n")
        data.write(b"\r\n")
        data.write(filepath.read_bytes())
        data.write(b"\r\n")
    data.write(b"--" + bBoundary + b"--\r\n")
    databytes = data.getvalue()
    request = urllib.request.Request(url)
    request.method = "POST"
    request.add_header("Content-Type", f"multipart/form-data; boundary={boundary}")
    request.add_header("Content-Length", len(databytes))
    response = urllib.request.urlopen(request, databytes)
    encoding = response.info().get_param("charset") or "utf-8"
    return json.loads(response.read().decode(encoding))

# Contructs a URL by concatenating the given base url with
# the given query parameters.
#
# Example:
# urlencode("http://www.example.com", {"key": "hello", "password": "world"})
#
# Output:
# "http://www.example.com?key=hello&password=world"
#
def urlencode(url, data):
    return url + "?" + urllib.parse.urlencode(data)

# Convenient wrapper around subprocess.check_output(), taking a string as input
# rather than a list, and returning the output as a decoded UTF-8 string.
#
def check_output(cmd, **kwargs):
    return subprocess.check_output(shlex.split(cmd), **kwargs).decode('utf-8')

# Prints the file path, rpaths, runpaths, and dependent shared libraries of the
# given binary file. If relative_to is not None, then the file path is printed
# relative to the given path. If the given file is not an ELF file, then the
# file is silently ignored (nothing is printed, no error raised).
#
def print_binary_info(file, *, relative_to):
    filetype = check_output(f"file {file}")
    if "ELF" in filetype:
        if relative_to:
            path = "./" + str(file.relative_to(relative_to))
        else:
            path = str(file)
        print_(f"\n{path}:")
        dump = check_output(f"objdump -x {file}")
        for line in dump.splitlines():
            if (("NEEDED" in line) or
                ("RPATH" in line) or
                ("RUNPATH" in line)):
                print_(line)

# Prints the file paths, rpaths, runpaths, and dependent shared libraries of all
# the binaries in the given appdir.
#
def print_binaries_info(appdir):
    print_(f"\nBinary files in {appdir}:")
    for file in sorted(appdir.glob("**/bin/*"), key=lambda v: str(v).upper()):
        print_binary_info(file, relative_to=appdir)
    for file in sorted(appdir.glob("**/*.so*"), key=lambda v: str(v).upper()):
        print_binary_info(file, relative_to=appdir)
    print_("") # newline

# Script entry point.
#
if __name__ == "__main__":

    # Set verbosity
    verbose = True

    # Parse arguments, and import them into the global namespace
    parser = argparse.ArgumentParser()
    parser.add_argument("srcDir", help="path to the source directory")
    parser.add_argument("buildDir", help="path to the build directory")
    parser.add_argument("config", help="build configuration (empty string in mono-config builds, or 'Release', 'Debug', etc. in multi-config builds)")
    parser.add_argument("qtDir", help="path to the Qt library, e.g.: '/Users/boris/Qt/5.12.5/clang_64'")
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
    commitRepository = getIniValue(version, "commitRepository")
    commitBranch = getIniValue(version, "commitBranch")
    commitHash = getIniValue(version, "commitHash")
    commitDate = getIniValue(version, "commitDate")
    commitTime = getIniValue(version, "commitTime")
    commitIndex = getIniValue(version, "commitIndex")
    architecture = getIniValue(version, "buildArchitecture")

    # Compute dot-separated version. We actually don't use this yet, but it may
    # be useful for some package managers in the future.
    # Examples:
    #   "Stable 2020.1"           ->  "2020.1"
    #   "2020 Beta 2019-07-08.1"  ->  "2019.7.8.1"
    #   "Alpha 2019-07-08.1"      ->  "2019.7.8.1"
    if versionType == "stable":
        major = int(versionMajor)
        minor = int(versionMinor)
        dotVersion = f"{major}.{minor}"
    elif versionType in ["beta", "alpha"]:
        year = int(commitDate[0:4])
        month = int(commitDate[5:7])
        day = int(commitDate[8:10])
        index = int(commitIndex)
        dotVersion = f"{year}.{month}.{day}.{index}"

    # Compute dash-separated version.
    # Examples:
    #   "Stable 2020.1"           ->  "2020.1"
    #   "2020 Beta 2019-07-08.1"  ->  "2020-beta-2019-07-08.1"
    #   "Alpha 2019-07-08.1"      ->  "alpha-2019-07-08.1"
    dashVersion = versionName.lower().replace(" ", "-")

    # Download linuxdeployqt, unless we already have it.
    linuxdeployqtName = f"linuxdeployqt-continuous-{architecture}.AppImage"
    linuxdeployqtPath = deployDir / linuxdeployqtName
    if not linuxdeployqtPath.exists():
        make_dir(linuxdeployqtPath.parent)
        linuxdeployqtUrl = "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/" + linuxdeployqtName
        print_(f"Downloading {linuxdeployqtUrl}")
        print_(f"         to {linuxdeployqtPath}")
        urllib.request.urlretrieve(linuxdeployqtUrl, str(linuxdeployqtPath))
        print_("Done.")
        linuxdeployqtPath.chmod(0o755)

    # List of all apps of the suite to install
    appNames = [ "Illustration" ]

    # Create AppImages
    filesToUpload = []
    for appName in appNames:

        # AppDir structure
        suiteNameLower = suiteName.lower()
        appNameLower = appName.lower()
        exeName = f"{suiteNameLower}{appNameLower}"
        pkgName = f"{exeName}-{dashVersion}-{architecture}"
        appdir = deployDir / (pkgName + ".AppDir")

        # Start fresh
        delete(appdir, verbose=verbose)

        # Copy VGC executable
        copy(buildDir / "bin" / exeName,
             appdir / "usr" / "bin" / exeName,
             verbose=verbose)

        # Create vgc.conf file
        vgcconf = appdir / "usr" / "bin" / "vgc.conf"
        vgcconfText = (
            "BasePath = ..\n" +
            "PythonHome = ..\n")
        vgcconf.write_text(vgcconfText)
        print_(f"\n{vgcconf}:\n{vgcconfText}")

        # Copy VGC shared libraries
        copy(buildDir / "lib",
             appdir / "usr" / "lib",
             verbose=verbose)

        # Copy the Python standard library.
        #
        # Note: the following shared libraries are automatically copied by
        # linuxdeployqt into appdir/usr/lib/:
        #
        # - Python itself (e.g., libpython3.6m.so.1.0).
        #
        # - The shared libraries that libpythonX.Y.so.Z depends on.
        #
        # - The shared libraries that pythonX.Y/**/*.so depends on,
        #   for all *.so files in the Python standard library.
        #
        print_(f"sys.prefix = {sys.prefix}")
        print_(f"sys.exec_prefix = {sys.exec_prefix}")
        pythonHome = Path(sys.prefix)
        pythonXdotY = f"python{sys.version_info.major}.{sys.version_info.minor}"
        copy(pythonHome / "lib" / pythonXdotY,
             appdir / "usr" / "lib" / pythonXdotY,
             verbose=verbose)

        # Copy VGC python bindings
        #
        copy(buildDir / "python",
             appdir / "usr" / "lib" / pythonXdotY,
             verbose=verbose)

        # Copy VGC resources
        copy(buildDir / "resources",
             appdir / "usr" / "resources",
             verbose=verbose)

        # Copy icons and desktop file
        copy(srcDir / "apps" / appNameLower / "share",
             appdir / "usr" / "share",
             verbose=verbose)

        # Print binaries info
        #
        print_binaries_info(appdir)

        # Create AppImage by calling linuxdeployqt.
        #
        # We set the VERSION environment variable to explicitly tell
        # appimagetool (used by linuxdeployqt under the hood) about the app
        # version. Otherwise, appimagetool would attempt to guess the version
        # based on `git rev-parse --short HEAD`, which would raise an error if
        # running outside from a git repo, and would be less predictable.
        #
        # But in fact, we actually don't care about VERSION: appimagetool only
        # uses this information to set the filename of the AppImage output,
        # which we change anyway.
        #
        # Also, note that executables and shared libraries on Linux don't have
        # any version info embedded in them, unlike on Windows and macOS.
        #
        # For more info about how appimagetool uses VERSION or other environment
        # variables, you can search for `getenv` in:
        #
        # https://github.com/AppImage/AppImageKit/blob/master/src/appimagetool.c
        #
        # Note that FreeType and HarfBuzz are intentionally not bundled by
        # linuxdeployqt, with the following given reason:
        #
        #   Those seem to cause major problems on systems with never versions of
        #   libfontconfig (such as Fedora, and recently Arch and their
        #   derivatives). As they're most likely not direct dependencies, not
        #   bundling them shouldn't break anything.
        #
        #   https://github.com/AppImage/pkg2appimage/pull/323
        #
        # However, in our case, they *are* direct dependencies, so I'm not sure
        # not bundling them is a good idea. For now, let's try as is (that is,
        # not including them as per linuxdeployqt advice), and if problems arise
        # we can try to bundle them.
        #
        env = os.environ.copy()
        env["VERSION"] = dashVersion
        env["LD_LIBRARY_PATH"] = f":{pythonHome}/lib"
        env["ARCH"] = architecture
        subprocess.run([
            f"{linuxdeployqtPath}",
            f"{appdir}/usr/share/applications/{exeName}.desktop",
            f"-qmake={qtDir}/bin/qmake",
            f"-appimage"],
            env=env)

        # Print binaries info
        #
        print_binaries_info(appdir)

        # Move and rename AppImage output to desired location and name. Note:
        # we'd rather tell linuxdeployqt where to generate this output, but at
        # the time of writing this isn't possible. See:
        #
        # https://github.com/probonopd/linuxdeployqt/issues/146
        #
        defaultOutput = Path(f"{suiteName}_{appName}-{dashVersion}-{architecture}.AppImage")
        desiredOutput = deployDir / (pkgName + ".AppImage")
        defaultOutput.rename(desiredOutput)

        # Add to list of files to upload
        filesToUpload.append(desiredOutput)

    # Upload artifacts if this is a Travis of GitHub Action build.
    #
    # We check for TRAVIS_REPO_SLUG rather than simply TRAVIS to prevent users
    # from mistakenly attempting to upload artifacts to our VGC servers if they
    # fork VGC and setup their own Travis builds.
    #
    # We have other security measures in place to prevent intentional harm from
    # malicious users.
    #
    # Note that VGC_TRAVIS_KEY is a secure environment variable not defined
    # for pull requests.
    #
    upload = False
    if os.getenv("TRAVIS_REPO_SLUG") == "vgc/vgc":
        upload = True
        key = os.getenv("VGC_TRAVIS_KEY", default="")
        pr = os.getenv("TRAVIS_PULL_REQUEST", default="false")
        url = "https://webhooks.vgc.io/travis"
        commitMessage = os.getenv("TRAVIS_COMMIT_MESSAGE")
    elif os.getenv("GITHUB_REPOSITORY") == "vgc/vgc":
        eventName = os.getenv("GITHUB_EVENT_NAME")
        url = "https://webhooks.vgc.io/github"
        commitMessage = os.getenv("COMMIT_MESSAGE")
        if eventName == "pull_request":
            upload = True
            key = ""
            ref = os.getenv("GITHUB_REF") # Example: refs/pull/461/merge
            pr = ref.split('/')[2]        # Example: 461
        elif eventName == "push":
            upload = True
            key = os.getenv("VGC_GITHUB_KEY")
            assert key, "Missing key: cannot upload artifact"
            pr = "false"
        else:
            upload = False
    if upload:
        print_("Uploading commit metadata...", end="")
        response = post_json(
            urlencode(url, {
                "key": key,
                "pr": pr
            }), {
            "versionType": versionType,
            "versionMajor": versionMajor,
            "versionMinor": versionMinor,
            "versionName": versionName,
            "commitRepository": commitRepository,
            "commitBranch": commitBranch,
            "commitHash": commitHash,
            "commitDate": commitDate,
            "commitTime": commitTime,
            "commitIndex": commitIndex,
            "commitMessage": commitMessage
        })
        print_(" Done.")
        releaseId = response["releaseId"]
        for file in filesToUpload:
            print_(f"Uploading {file}...", end="")
            response = post_multipart(
                urlencode(url, {
                    "key": key,
                    "pr": pr,
                    "releaseId": releaseId
                }), {}, {
                "file": file
            })
            print_(" Done.")
