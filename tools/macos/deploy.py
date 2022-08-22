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
import io
import json
import mimetypes
import os
import re
import shutil
import subprocess
import sys
import urllib.parse
import urllib.request
import uuid

# We use dmgbuild (and its dependencies ds_store and mac_alias) to generate our DMG files.
# This is a non-standard packages which can be installed via pip, for example:
#
# /Library/Frameworks/Python.framework/Versions/3.9/bin/pip3 install dmgbuild
#
# Note that starting with Python 3.9, the old API of plistlib is removed, and
# thus you need dmgbuild >= 1.4.1 which uses the new API.
#
import dmgbuild

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
            print("             Deleting symlink " + str(path) + "\n", flush=True)
        path.unlink()
    elif path.is_file():
        if verbose:
            print("        Deleting regular file " + str(path) + "\n", flush=True)
        path.unlink()
    elif path.is_dir():
        if verbose:
            print("Recusively deleting directory " + str(path) + "\n", flush=True)
        shutil.rmtree(str(path))
    elif path.exists():
        raise OSError("Cannot delete file " + str(path) + ": unsupported file type (e.g.: socket, mount point, FIFO, block device, char device)")

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
                raise FileExistsError("Cannot create directory " + str(path) + ": another file type already exists at the given path. Use force=True if you want to overwrite.")
        if verbose:
            print("           Creating directory " + str(path) + "\n", flush=True)
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
            delete(dst, verbose=verbose)

    if srcType == 2:
        if verbose and not recursiveVerbose:
            print("          Recursively copying " + str(src) + "\n" +
                  "                           to " + str(dst) + "\n", flush=True)
        newVerbose = verbose and recursiveVerbose
        make_dir(dst, verbose=newVerbose)
        for item in os.listdir(src):
            s = src / item
            d = dst / item
            copy(s, d, overwrite=overwrite, dirpolicy=dirpolicy, verbose=newVerbose, recursiveVerbose=recursiveVerbose)
    else:
        # Notes:
        # - Like ditto, copy2 copies over file permissions.
        # - Unlike ditto, copy2 does not copy owner/group, which is better for
        #   example when copying the Python framework, because it may have
        #   an undesirable "admin" group and unusual permissions, preventing
        #   altering the rpaths and lib paths
        # - While preserving permissions to some extent, we manually add user
        #   write, and remove group/other write, for similar reasons as above.
        make_dir(dst.parent, verbose=verbose)
        if verbose:
            print("                      Copying " + str(src) + "\n" +
                  "                           to " + str(dst), flush=True) # no final new line (see below)
        shutil.copy2(str(src), str(dst), follow_symlinks=False)
        oldPermissions = os.stat(dst, follow_symlinks=False).st_mode & 0o777
        newPermissions = oldPermissions
        newPermissions |= 0o200
        newPermissions &= ~0o022
        if newPermissions != oldPermissions:
            if verbose:
                print("                              " +
                      "(changed permissions from " + oct(oldPermissions)[2:] +
                      " to " + oct(newPermissions)[2:] + ")", flush=True) # no final new line (see below)
            os.chmod(dst, newPermissions, follow_symlinks=False)
        if verbose:
            print("", flush=True) # final newline

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

# One entry of an `otool -l` output.
# You shouldn't typically create a LibEntry yourself: use LibInfo info instead.
#
class LibEntry:

    def __init__(self, title):
        self.raw_data = ""
        self.title = title
        if re.match("Mach header", self.title):
            self.type = "mach_header"
        elif re.match("Sdection", self.title):
            self.type = "section"
        elif re.match("Load command", self.title):
            self.type = "load_command"
        else:
            self.type = "unknown"

    def add_data_line(self, line):
        self.raw_data += line + "\n"
        if self.type == "load_command":
            line_ = line.split()
            if line_[0] == "time" and line_[1] == "stamp":
                line_ = ["timestamp"] + line_[2:]
            attrname = line_[0]
            attrvalue = " ".join(line_[1:])
            hasoffset = re.match(r"(.*) \(offset (\d+)\)", attrvalue)
            if hasoffset:
                attrvalue = hasoffset.group(1)
                self.offset = hasoffset.group(2)
            setattr(self, attrname, attrvalue)

# Get information about a given executable or shared library.
#
# Under the hood, this calls `otool -l` and splits the output into a list of
# "entries" (accessible via libinfo.entries), some of which are "load commands"
# (accessible via libinfo.load_commands).
#
# The load commands are those containing info about dependent shared libraries
# and rpaths. Each load command lc has an attribute lc.cmd describing which type
# of command it is, as well as other attributes, based on the command type. Note
# that unlike in the raw `otool -l` output, the offset of the 'name' or 'path'
# attribute is stored as a separate attribute called 'offset'.
#
# Here are two typical examples of load commands:
#
#   cmd             LC_LOAD_DYLIB
#   cmdsize         72
#   name            @rpath/QtCore.framework/Versions/5/QtCore
#   offset          24
#   timestamp       2 Thu Jan  1 01:00:02 1970
#   current         version 5.12.5
#   compatibility   version 5.12.0
#
#   cmd      LC_RPATH
#   cmdsize  48
#   path     /Users/boris/Qt/5.12.5/clang_64/lib
#   offset   12
#
class LibInfo:

    def __init__(self, filename):
        self.filename = filename
        self.raw_info = subprocess.check_output(["otool", "-l", str(self.filename)]).decode('utf-8')
        lines = self.raw_info.splitlines()
        self.name = lines[0][:-1]
        self.entries = []
        self.load_commands = []
        entry = None
        for line in lines[1:]:
            if re.match("(Mach header)|(Section)|(Load command)", line):
                entry = LibEntry(line)
                self.entries.append(entry)
                if entry.type == "load_command":
                    self.load_commands.append(entry)
            elif entry:
                entry.add_data_line(line)
            else:
                # This can occur for *.a files which have more headers.
                # We just ignore these headers.
                pass

# Returns the list of rpaths of given executable.
#
def get_rpaths(filename):
    info = LibInfo(filename)
    res = []
    for lc in info.load_commands:
        if lc.cmd == "LC_RPATH":
            res.append(lc.path)
    return res

# Changes a given rpath to another.
#
def change_rpath(filename, old, new, *, verbose=False):
    if verbose:
        print("            Changing rpath in " + str(filename) + "\n" +
              "                         from " + str(old) + "\n" +
              "                           to " + str(new) + "\n", flush=True)
    subprocess.run(["install_name_tool", "-rpath", str(old), str(new), str(filename)])

# Adds the given rpath.
#
def add_rpath(filename, new, *, verbose=False):
    if str(new) not in get_rpaths(filename):
        if verbose:
            print("                 Adding rpath " + str(new) + "\n" +
                  "                           in " + str(filename) + "\n", flush=True)
        subprocess.run(["install_name_tool", "-add_rpath", str(new), str(filename)])

# Changes the given rpath to another.
#
def delete_rpath(filename, old, *, verbose=False):
    if str(old) in get_rpaths(filename):
        if verbose:
            print("               Deleting rpath " + str(old) + "\n" +
                  "                           in " + str(filename) + "\n", flush=True)
        subprocess.run(["install_name_tool", "-delete_rpath", str(old), str(filename)])

# Get the shared libraries referenced by a given binary file.
#
def get_libs(filename):
    info = LibInfo(filename)
    res = []
    for lc in info.load_commands:
        if lc.cmd == "LC_LOAD_DYLIB":
            res.append(lc.name)
    return res

# Change the path of a referenced shared libraries.
#
def change_lib(filename, old, new, *, verbose=False):
    if verbose:
        print("     Changing library path in " + str(filename) + "\n" +
              "                         from " + str(old) + "\n" +
              "                           to " + str(new) + "\n", flush=True)
    subprocess.run(["install_name_tool", "-change", str(old), str(new), str(filename)])

# Get the ID of this shared library.
# Returns an empty string if this file has no library ID.
#
def get_lib_id(filename):
    info = LibInfo(filename)
    res = ""
    for lc in info.load_commands:
        if lc.cmd == "LC_ID_DYLIB":
            if len(res) > 0:
                print("Warning: file " + str(filename) + " has more than one LC_ID_DYLIB load command. All IDs after the first are ignored.", flush=True)
                break
            else:
                res = lc.name
    return res

# Change the id of the shared library
#
def set_lib_id(filename, id, *, verbose=False):
    if verbose:
        print("        Setting library ID of " + str(filename) + "\n" +
              "                           to " + str(id) + "\n", flush=True)
    subprocess.run(["install_name_tool", "-id", str(id), str(filename)])

# Computes new path for the given lib.
#
def compute_lib_replacement(lib):

    # Keep paths already relative to @rpath as is
    if lib.startswith("@rpath/"):
        return lib

    # Change "@executable_path/../Frameworks/" to "@rpath/"
    elif lib.startswith("@executable_path/../Frameworks/"):
        return "@rpath" + lib[len("@executable_path/../Frameworks"):]

    # Keep system frameworks and system libraries as is
    if lib.startswith("/System/Library/Frameworks/") or lib.startswith("/usr/lib/"):
        return lib

    # Ship things that are user-installed, for example via Homebrew.
    # Examples:
    #   /Library/Frameworks/Python.framework/Versions/3.7/Python
    #   /usr/local/Cellar/python/3.7.5/Frameworks/Python.framework/Versions/3.7/Python
    #   /usr/local/opt/python/Frameworks/Python.framework/Versions/3.7/Python
    #   /usr/local/opt/freetype/lib/libfreetype.6.dylib
    elif (lib.startswith("/Library/Frameworks/") or
          lib.startswith("/usr/local/Cellar/") or
          lib.startswith("/usr/local/opt/")):
        i = lib.find(".framework")
        if i != -1:
            while lib[i] != "/":
                i -= 1
            return "@rpath" + lib[i:] # @rpath/Python.framework/Versions/3.7/Python
        else:
            return "@rpath/" + Path(lib).name # @rpath/libfreetype.6.dylib

    # Ship VGC libraries (obviously)
    elif "libvgc" in lib:
        return "@rpath/" + Path(lib).name # @rpath/libvgccore.dylib

    # GitHub Actions Python
    # Note that libpython3.7m.dylib is both in
    #   Frameworks/libpython3.7m.dylib
    # and in
    #   Frameworks/Python/lib/libpython3.7m.dylib
    # XXX Remove the duplicate
    elif (lib.startswith("/Users/runner/hostedtoolcache/Python") and
          ("x64/lib/libpython" in lib)):
        return "@rpath/" + Path(lib).name # @rpath/libpython3.7m.dylib

    # Print a warning if it's not one of the above known categories
    else:
        print('\033[33m' + "    Warning: suspicious library path: " + lib + '\033[0m', flush=True)
        return lib


# Returns all the binaries in the app bundle
#
def get_binaries(bundle):

    # Find all libraries
    tmp = []
    tmp.extend(bundle.glob("**/*.dylib"))
    tmp.extend(bundle.glob("**/*.so"))
    tmp.extend(bundle.glob("**/*.a"))
    for framework in bundle.glob("**/*.framework"):
        name = framework.stem
        tmp.extend(framework.glob("**/" + name))
        # Note: the above also finds "*/Python.framework/*/Python.app/*/Python"

    # Find all executables
    tmp.extend((bundle / "Contents/MacOS/bin").glob("vgc*"))
    for app in bundle.glob("**/*.app"):
        name = app.stem
        tmp.extend(app.glob("**/" + name))

    # Remove duplicates and sort
    res = set()
    for path in tmp:
        if not path.is_symlink() and not path.is_dir():
            res.add(path)
    return sorted(res)

# Returns all the executables in the app bundle
#
def get_executables(bundle):

    # Find all executables
    tmp = []
    for x in (bundle / "Contents/MacOS/bin").glob("vgc*"):
        if x.name != "vgc.conf":
            tmp.append(x)
    for app in bundle.glob("**/*.app"):
        name = app.stem
        tmp.extend(app.glob("**/" + name))
    res = []

    # Remove duplicates and sort
    res = set()
    for path in tmp:
        if not path.is_symlink() and not path.is_dir():
            res.add(path)
    return sorted(res)

# Prints the lib ID, rpaths, and lib paths of the given binary.
#
def print_binary_info(filename):
    print(str(filename) + ":", flush=True)
    print("  LC_ID_DYLIB:", flush=True)
    print("    " + get_lib_id(filename), flush=True)
    print("  LC_LOAD_DYLIB:", flush=True)
    for libs in get_libs(filename):
        print("    " + libs, flush=True)
    print("  LC_RPATH:", flush=True)
    for rpaths in get_rpaths(filename):
        print("    " + rpaths, flush=True)
    print("", flush=True)

# Prints the lib IDs, rpaths, and lib paths of all the binaries in the given
# app bundle.
#
def print_binaries_info(bundle):
    for path in get_binaries(bundle):
        print_lib_info(path)

# Prints a summary of the lib IDs, rpaths, and lib paths of all the binaries
# in the given app bundle.
#
def print_binaries_summary(bundle):

    binaries = get_binaries(bundle)

    # Print binaries
    print("Shipped binaries:", flush=True)
    for path in binaries:
        print("  " + str(path.relative_to(bundle)), flush=True)

    # Print libraries
    print("Dynamically linked libraries:", flush=True)
    libs = set()
    for path in binaries:
        for lib in get_libs(path):
            libs.add(lib)
    for lib in sorted(libs):
        print("  " + lib, flush=True)

    # Print details
    print("Details:", flush=True)
    for path in binaries:
        print("  " + str(path.relative_to(bundle)), flush=True)
        id = get_lib_id(path)
        if id != "":
            print('\033[90m' + "    LC_ID_DYLIB:   " + id + '\033[0m', flush=True)
        for rpath in get_rpaths(path):
            print('\033[90m' + "    LC_RPATH:      " + rpath + '\033[0m', flush=True)
        for lib in get_libs(path):
            print('\033[90m' + "    LC_LOAD_DYLIB: " + lib + '\033[0m', flush=True)

# Returns a path relative to another one. This is like
# pathlib.Path.relative_to(), but it also handle the case where path isn't a
# descendant of relto, by adding ".." components.
#
def relative_to(path, relto):
    path = str(path)
    relto = str(relto)
    if path == relto:
        return ""
    if path.startswith(relto + "/"):
        return path[len(relto)+1:]
    else:
        path_components = path.split("/")
        relto_components = relto.split("/")
        common_ancestor = ""
        for p, r in zip(path_components, relto_components):
            if p == r:
                common_ancestor += p + "/"
            else:
                break
        path_remainder = path[len(common_ancestor):]
        relto_remainder = relto[len(common_ancestor):]
        n = str(relto_remainder).count("/") + 1
        res = (n * "../") + path_remainder
        if res.endswith("/"): # example: "../" if path is a parent dir of relto
            res = res[:-1]
        return res

# Makes a POST request to the given URL with the given data.
# The given data should be a Python dictionary, which this function
# automatically encodes as JSON. Finally, the JSON response is decoded
# and returned as a python dictionary.
#
def post_json(url, data):
    databytes = json.dumps(data).encode('utf-8')
    request = urllib.request.Request(url)
    request.method = 'POST'
    request.add_header('Content-Type', 'application/json; charset=utf-8')
    response = urllib.request.urlopen(request, databytes)
    encoding = response.info().get_param('charset') or 'utf-8'
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
    bBoundary = boundary.encode('utf-8')
    data = io.BytesIO()
    for name, value in fields.items():
        bName = name.encode('utf-8')
        bValue = value.encode('utf-8')
        data.write(b'--' + bBoundary + b'\r\n')
        data.write(b'Content-Disposition: form-data; name="' + bName + b'"\r\n')
        data.write(b'\r\n')
        data.write(bValue)
        data.write(b'\r\n')
    for name, filepath in files.items():
        filename = filepath.name
        mimetype = mimetypes.guess_type(filename)[0] or 'application/octet-stream'
        bName = name.encode('utf-8')
        bFilename = filename.encode('utf-8')
        bMimetype = mimetype.encode('utf-8')
        data.write(b'--' + bBoundary + b'\r\n')
        data.write(b'Content-Disposition: form-data; name="' + bName + b'"; filename="' + bFilename + b'"\r\n')
        data.write(b'Content-Type: ' + bMimetype + b'\r\n')
        data.write(b'\r\n')
        data.write(filepath.read_bytes())
        data.write(b'\r\n')
    data.write(b'--' + bBoundary + b'--\r\n')
    databytes = data.getvalue()
    request = urllib.request.Request(url)
    request.method = 'POST'
    request.add_header('Content-Type', 'multipart/form-data; boundary=' + boundary)
    request.add_header('Content-Length', len(databytes))
    response = urllib.request.urlopen(request, databytes)
    encoding = response.info().get_param('charset') or 'utf-8'
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

    # Create macOS app bundles and dmg images
    filesToUpload = []
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
        bundleFrameworksDir = bundleContentsDir / "Frameworks"
        infoFile = bundleContentsDir / "Info.plist"

        # Start fresh
        delete(bundleDir, verbose=verbose)

        # Copy executable
        appExecutableBasename = "vgc" + appNameLower
        executableRelPath = "bin/" + appExecutableBasename
        bundleExecutableRelPath = executableRelPath
        executable = buildDir / executableRelPath
        bundleExecutable = bundleMacOSDir / bundleExecutableRelPath
        copy(executable, bundleExecutable, verbose=verbose)

        # Copy VGC shared libraries
        vgcLibDir = buildDir / "lib"
        bundleVgcLibDir = bundleFrameworksDir
        vgcLibs = [ lib for lib in vgcLibDir.iterdir() ]
        bundleVgcLibs = [ (bundleVgcLibDir / lib.name) for lib in vgcLibs ]
        for lib, bundleLib in zip(vgcLibs, bundleVgcLibs):
            copy(lib, bundleLib, verbose=verbose)

        # Find Python framework and determine new location
        #
        # On Travis, the Python structure looks like:
        #   -- [Python]
        #   -- Found PythonInterp: /usr/local/bin/python3 (found suitable version "3.9.1", minimum required is "3.6")
        #   -- Found PythonLibs: /usr/local/Frameworks/Python.framework/Versions/3.9/lib/libpython3.9.dylib (found suitable version "3.9.1", minimum required is "3.6")
        #   -- Version: 3.9.1
        #   -- PYTHON_PREFIX: /usr/local/Cellar/python@3.9/3.9.1/Frameworks/Python.framework/Versions/3.9
        #   -- PYTHON_EXEC_PREFIX: /usr/local/Cellar/python@3.9/3.9.1/Frameworks/Python.framework/Versions/3.9
        #   -- PYTHON_HOME: /usr/local/Cellar/python@3.9/3.9.1/Frameworks/Python.framework/Versions/3.9
        #
        # On GitHub Actions, it looks like:
        #   -- [Python]
        #   -- Found PythonInterp: /Users/runner/hostedtoolcache/Python/3.7.9/x64/bin/python3 (found suitable version "3.7.9", minimum required is "3.6")
        #   -- Found PythonLibs: /Users/runner/hostedtoolcache/Python/3.7.9/x64/lib/libpython3.7m.dylib (found suitable version "3.7.9", minimum required is "3.6")
        #   -- Version: 3.7.9
        #   -- PYTHON_PREFIX: /Users/runner/hostedtoolcache/Python/3.7.9/x64
        #   -- PYTHON_EXEC_PREFIX: /Users/runner/hostedtoolcache/Python/3.7.9/x64
        #   -- PYTHON_HOME: /Users/runner/hostedtoolcache/Python/3.7.9/x64
        #
        #   /Users/runner/hostedtoolcache/Python/3.7.9/x64
        #   ├── Python-3.7.9.tgz
        #   ├── bin
        #   │   ├── python -> python3.7
        #   │   ├── python3 -> python3.7
        #   │   ├── python3.7
        #   │   ...
        #   ├── include
        #   │   └── python3.7m
        #   │       ├── Python.h
        #   │       ...
        #   ├── lib
        #   │   ├── libpython3.7m.dylib
        #   │   ├── pkgconfig
        #   │   │   ├── python-3.7.pc
        #   │   │   ├── python-3.7m.pc -> python-3.7.pc
        #   │   │   └── python3.pc -> python-3.7.pc
        #   │   └── python3.7
        #   │       ├── __future__.py
        #   │       ...
        #   ├── python -> ./bin/python3.7
        #   ├── python-3.7.9-darwin-x64.tar.gz
        #   ├── share
        #   │   └── man
        #   │       └── man1
        #   │           ├── python3.1 -> python3.7.1
        #   │           └── python3.7.1
        #   └── tools_structure.txt
        #
        print_binary_info(bundleExecutable)
        for lib in get_libs(bundleExecutable):
            i = lib.find("Python.framework")
            if i != -1:
                pythonOldRef = lib                         # Example: /usr/local/opt/python/Frameworks/Python.framework/Versions/3.7/Python
                pythonFrameworkOldParent = Path(lib[:i-1]) # Example: /usr/local/opt/python/Frameworks
                pythonLibRelPath = Path(lib[i:])           # Example: Python.framework/Versions/3.7/Python
                print("                Found library " + lib + "\n" +
                      "                referenced in " + str(bundleExecutable) + "\n", flush=True)
                pythonFrameworkOldPath = pythonFrameworkOldParent / "Python.framework"
                pythonFrameworkPath = bundleFrameworksDir / "Python.framework"
                pythonHome = (bundleFrameworksDir / pythonLibRelPath).parent
                break
            i = lib.find("x64/lib/libpython3")
            if i != -1:
                pythonOldRef = lib                            # Example: /Users/runner/hostedtoolcache/Python/3.7.9/x64/lib/libpython3.7m.dylib
                pythonFrameworkOldParent = Path(lib[:i-1])    # Example: /Users/runner/hostedtoolcache/Python/3.7.9
                pythonLibRelPath = Path("Python" + lib[i+3:]) # Example: Python/x64/lib/libpython3.7m.dylib
                print("                Found library " + lib + "\n" +
                      "                referenced in " + str(bundleExecutable) + "\n", flush=True)
                pythonFrameworkOldPath = pythonFrameworkOldParent / "x64"
                pythonFrameworkPath = bundleFrameworksDir / "Python"
                pythonHome = pythonFrameworkPath
                break

        # Copy Python framework to our bundle
        copy(pythonFrameworkOldPath, pythonFrameworkPath, verbose=verbose)

        # Delete Python stuff we don't need.
        # Sizes are given sometimes for the official 64bit-only installation of Python 3.7.4 from www.python.org,
        # and sometimes for the Python distribution provided by GitHub Actions (GA).
        # We don't keep __pycache__ folders for two reasons:
        # 1. They contain non-relocatable file paths which I don't know how to change
        # 2. The take a lot of space
        # 3. They can be generated at runtime anyway (either preemptively at install time,
        #    or automatically when importing a module at runtime)
        xDotY = f"{sys.version_info.major}.{sys.version_info.minor}"
        pythonXdotY = "python" + xDotY
        pythonLibDir = pythonHome / "lib"
        pythonXdotYDir = pythonLibDir / pythonXdotY
        delete(pythonFrameworkPath / "Python", verbose=verbose)    # broken symlink: points to itself (XXX should we instead make it point to Versions/X.Y/Python ? QtCore, QtGui, etc. do this)
        delete(pythonFrameworkPath / "Resources", verbose=verbose) # broken symlink: points to itself (XXX should we instead make it point to Versions/X.Y/Resources ?)
        delete(pythonFrameworkPath / "Headers", verbose=verbose)   # broken symlink: points to itself + we delete header files anyway
        delete(pythonHome / "bin", verbose=verbose)                # 2to3, easy_install, idle, pip, pydoc, python, pyvenv (56 kB)
        delete(pythonHome / "Headers", verbose=verbose)            # symlink to include/pythonX.Ym
        delete(pythonHome / "include", verbose=verbose)            # *.h files (0.9 MB)
        delete(pythonHome / "share", verbose=verbose)              # doc and examples (2.3 MB)
        for x in (pythonHome / "Resources").glob('*.lproj'):       # documentation (46 MB)
            delete(x, verbose=verbose)
        delete(pythonXdotYDir / "test")                            # tests (23 MB) (+ 25 MB of __pycache__)
        for x in pythonXdotYDir.glob("**/__pycache__"):            # Python bytecode (58.5 MB) (including tests)
            delete(x, verbose=verbose)
        delete(pythonXdotYDir / "site-packages")                   # Manually installed packages (GA: 51MB). Examples: Crypto, pip, setuptools, dmgbuild, mac_alias, ds_store, etc.
        for x in pythonXdotYDir.glob("**/*.profclangr"):           # Profiling files in unittest folder (GA: 10 MB)
            delete(x, verbose=verbose)
        for x in pythonHome.glob("*.tgz"):                         # Compressed copy of python src (GA: 22MB)
            delete(x, verbose=verbose)
        for x in pythonHome.glob("*.tar.gz"):                      # Compressed copy of python distribution (GA: 71MB)
            delete(x, verbose=verbose)
        delete(pythonXdotYDir / f"config-{xDotY}m-darwin")         # Config stuff (GA: 18MB)

        # Copy VGC Python modules to the Python framework
        copy(buildDir / "python/vgc", pythonXdotYDir / "vgc", verbose=verbose)

        # Copy vgc resources
        copy(buildDir / "resources", bundleMacOSDir / "resources", verbose=verbose)

        # Copy bundle icons
        appIcnsName = appExecutableBasename + ".icns"
        fileExtensionIcnsName = fileExtension + ".icns"
        copy(srcDir / "apps" / appNameLower / appIcnsName, bundleResourcesDir / appIcnsName, verbose=verbose)
        copy(srcDir / "apps" / appNameLower / fileExtensionIcnsName, bundleResourcesDir / fileExtensionIcnsName, verbose=verbose)

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
            "CFBundleExecutable": bundleExecutableRelPath,
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

        # Execute macdeployqt.
        # We need to do this after the Info.plist file is generated, so that it can find the executable.
        # We need to do this before updating python paths, otherwise macdeployqt will mess them up again.
        #
        print("Executing macdeployqt...", flush=True)
        print("(note: an error about Python is expected, it's macdeployqt not being smart enough. We fix it afterwards.)", flush=True)
        subprocess.run([
            str(qtDir / "bin" / "macdeployqt"),
            str(bundleDir), "-always-overwrite", "-verbose=1"])
        print("Done.", flush=True)

        # Create qt.conf file alongside executable.
        # See https://github.com/vgc/vgc/issues/218
        #
        qtconf = bundleExecutable.parent / "qt.conf"
        executable_ = str(bundleExecutable.relative_to(bundleContentsDir))
        plugins_ = "PlugIns"
        up_ = "../" * executable_.count("/")
        qtconfText = "[Paths]\n"
        qtconfText += f"Plugins = {up_}{plugins_}\n"
        qtconf.write_text(qtconfText)
        print(f"\n{qtconf}:\n{qtconfText}", flush=True)

        # Create vgc.conf file alongside executable.
        #
        vgcconf = bundleExecutable.parent / "vgc.conf"
        pythonHome_ = str(pythonHome.relative_to(bundleContentsDir))
        vgcconfText = "BasePath = ..\n"
        vgcconfText += f"PythonHome = {up_}{pythonHome_}\n"
        vgcconf.write_text(vgcconfText)
        print(f"\n{vgcconf}:\n{vgcconfText}", flush=True)

        # Update rpaths, lib paths, and id of all binaries
        print("Fixing library paths...", flush=True)
        binaries = get_binaries(bundleDir)
        executables = get_executables(bundleDir)
        replacements = {}
        for x in binaries:
            print("  " + str(x), flush=True)
            # Delete rpaths
            for rpath in get_rpaths(x):
                delete_rpath(x, rpath)
            # Update lib id path
            id = get_lib_id(x)
            if id != "":
                newid = "@rpath/" + relative_to(x, bundleFrameworksDir)
                print('\033[90m' + "    ID = " + id, end='')
                if newid != id:
                    set_lib_id(x, newid)
                    print('\033[32m' + " -> " + newid + '\033[0m', flush=True)
                else:
                    print('\033[0m', flush=True)
            # Update loaded lib paths
            for lib in get_libs(x):
                print('\033[90m' + "    " + lib, end='')
                if lib not in replacements:
                    newlib = compute_lib_replacement(lib)
                    replacements[lib] = newlib
                newlib = replacements[lib]
                if newlib != lib:
                    change_lib(x, lib, newlib)
                    print('\033[32m' + " -> " + newlib + '\033[0m', flush=True)
                else:
                    print('\033[0m', flush=True)
                if newlib.startswith("@rpath/"):
                    dst = bundleFrameworksDir / newlib[len("@rpath/"):]
                    if not dst.is_file():
                        if lib.startswith("/"):
                            src = Path(lib).resolve()
                            copy(src, dst, verbose=True)
                            binaries.append(dst) # find all missing libraries recursively
                    if not dst.is_file():
                        print('\033[33m' + "    Warning: file " + newlib + " does not exist" + '\033[0m', flush=True)

        # Add rpaths
        for x in executables:
            y = x.relative_to(bundleContentsDir)
            n = str(y).count("/")
            rpath = "@executable_path/" + (n * "../") + "Frameworks"
            add_rpath(x, rpath)

        print("Done.", flush=True)
        print_binaries_summary(bundleDir)

        # Generate the DMG file.
        #
        dmgFile = deployDir / (bundleDirBasename + ".dmg")
        dmgFilename = str(dmgFile)
        dmgVolumeName = bundleDirBasename
        dmgSettings = {
            'filename': dmgFilename,
            'volume_name': dmgVolumeName,
            'format': 'UDBZ',
            'size': None,
            'files': [ str(bundleDir) ],
            'symlinks': { 'Applications': '/Applications' },
            'badge_icon': str(bundleResourcesDir / appIcnsName),
            'icon_locations': {
                bundleDirName:  (120, 135),
                'Applications': (470, 135),
                '.background.tiff': (2000, 135)
            },
            'background': str(srcDir / "tools" / "macos" / "dmg-background.png"),
            # Note: dmgbuild will also automatically find "dmg-background@2x.png",
            # and merge both 1x and 2x versions into a single .tiff image.
            'show_status_bar': False,
            'show_tab_view': False,
            'show_toolbar': False,
            'show_pathbar': False,
            'show_sidebar': False,
            'sidebar_width': 180,
            'window_rect': ((0, 0), (600, 300 + 22)), # 22px = window title height
            'default_view': 'icon-view',
            'show_icon_preview': False,
            'include_icon_view_settings': 'auto',
            'include_list_view_settings': 'auto',
            'arrange_by': None,
            'grid_offset': (0, 0),
            'grid_spacing': 100,
            'scroll_position': (0, 0),
            'label_pos': 'bottom',
            'text_size': 14,
            'icon_size': 128,
            'list_icon_size': 16,
            'list_text_size': 12,
            'list_scroll_position': (0, 0),
            'list_sort_by': 'name',
            'list_use_relative_dates': True,
            'list_calculate_all_sizes': False,
            'list_columns': ('name', 'date-modified', 'size', 'kind', 'date-added'),
            'list_column_widths': {
                'name': 300,
                'date-modified': 181,
                'date-created': 181,
                'date-added': 181,
                'date-last-opened': 181,
                'size': 97,
                'kind': 115,
                'label': 100,
                'version': 75,
                'comments': 300,
                },
            'list_column_sort_directions': {
                'name': 'ascending',
                'date-modified': 'descending',
                'date-created': 'descending',
                'date-added': 'descending',
                'date-last-opened': 'descending',
                'size': 'descending',
                'kind': 'ascending',
                'label': 'ascending',
                'version': 'ascending',
                'comments': 'ascending',
                }
        }
        print("Creating image " + dmgFilename + "...", flush=True)
        dmgbuild.build_dmg(dmgFilename, dmgVolumeName, settings=dmgSettings)
        print("Done.", flush=True)
        filesToUpload.append(dmgFile)
        print("File size: " + str(dmgFile.stat().st_size) + "B", flush=True)



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
