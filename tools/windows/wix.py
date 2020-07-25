#!/usr/bin/env python3
#
# This script creates windows installers from a given build. Please also read
# the documentation of vgc/tools/version.py for a definition of some of the
# versioning terms we use in this script.
#
# Useful resources on Windows Installer and WiX
# ---------------------------------------------
#
# - https://docs.microsoft.com/en-us/windows/desktop/Msi/windows-installer-portal
# - https://www.firegiant.com/wix/tutorial/
# - https://wixtoolset.org/documentation/manual/v3/wixui/wixui_customizations.html
# - https://stackoverflow.com/questions/471424/wix-tricks-and-tips
#
# Other useful resources or inspiration sources
# ---------------------------------------------
#
# - https://wiki.qt.io/Branch_Guidelines
#
# Upgrade policy
# --------------
#
# The "upgrade policy" determines whether installing a newer version of VGC
# replaces the existing version, or if instead both versions are installed
# side-by-side. For stable versions, there are three possible upgrade
# policies:
# - "all": Always replaces the existing installation.
# - "minor": Replaces the existing installation only for minor updates.
# - "none": Never replaces the existing installation.
#
# Unfortunately, the upgrade policy must be hard-coded in the installer (or at
# the very least, in the MSI package, and we could choose which msi to install
# based on user input in the installer), and cannot be changed by the user
# after installation. This is because the upgrade policy is determined by the
# MSI upgrade code which is hard-coded in the MSI package.
#
# The upgrade policy also affects which name is given to the install folder,
# so it makes sense that it cannot be changed after installation. For example,
# with the "all" upgrade policy, stable versions are by default installed to:
#
#   C:/Program Files/VGC/
#
# While with the "minor", their are installed to a more specific folder to
# allow side-by-side installation:
#
#   C:/Program Files/VGC 2020/
#   C:/Program Files/VGC 2021/
#
# Below is a table documenting:
# - which upgrade policies are supported for each version type
# - what the default install folder looks like
#
# -----------------------------------------------------------------
# Version type   Upgrade policy   Default install folder
# -----------------------------------------------------------------
# "stable"       "all"            VGC
#                "minor"          VGC 2020
#                "none"           VGC 2020.0
# -----------------------------------------------------------------
# "beta"         "minor"          VGC 2020 Beta[-gh62]
#                "none"           VGC 2020 Beta[-gh62] 2019-07-14.1
# -----------------------------------------------------------------
# "alpha"        "minor"          VGC Alpha[-gh62]
#                "none"           VGC Alpha[-gh62] 2019-07-14.1
# -----------------------------------------------------------------
#
# The optional branch name (e.g., "gh62") is only appended if the commitBranch
# is not "master" for alpha versions, or "{versionMajor}" (e.g., "2020") for
# beta versions. The commitBranch is ignored for stable versions, since we
# already have all the info we need for versioning (major.minor), and we want
# to support creating installers for stable versions without access to git.
#
# For alpha versions, the concept of "minor" vs "major" updates doesn't make
# sense, so there's just one type of possible update, which we call "minor".
#
# For beta versions, the concept of "minor" vs "major" updates does make some
# sense, and we could envision an "all" upgrade policy that would install "VGC
# Beta" and would allow both minor and major updates. Unfortunately, such
# installations would suffer from a sudden decrease of stability when updating
# from the last commit of the 2020 branch (= stable quality) to the first
# commit of the 2021 branch (= alpha quality), so we decided not to support
# it. The UI may still invite users of VGC 2020 Beta to try out VGC 2021 Beta,
# but it would install it side-by-side, to keep the more stable version
# available. This decision also makes the offer much less confusing: alpha
# versions are for trying the very latest just-implemented features, while
# beta versions are for testing a specific upcoming major version.
#
# Note that alpha and beta versions require a valid commitBranch, commitDate,
# and commitIndex in order to be able to create installers, since we need this
# info for versioning (unlike stable versions, their "minor version number" is
# not hard-coded in the source code).
#
# In order to help the user choose the desired upgrade policy, the Download
# page may look like the following:
#
# [Download VGC 2020]      <-- link to VGC 2020 Installer.exe
#
# + More options           <-- dropdown menu revealing more options on click
# |
# |- VGC 2020 Installer.exe:
# |    This is our recommended installer, which allows to easily update to any
# |    newer version. By default, this installs VGC 2020 to:
# |        C:\Program Files\VGC\
# |
# |- VGC 2020 Installer - Minor Updates Only.exe:
# |    This is for users who prefer to only apply minor updates to their VGC
# |    installation. Using this installer, major updates will be installed
# |    side-by-side rather than replacing the older version. By default, this
# |    installs VGC 2020 to:
# |        C:\Program Files\VGC 2020\
# |
# |- VGC 2020 Installer - No Updates.exe:
# |    This is for users who really don't like updates at all! Using this
# |    installer, both minor and major updates are installed side-by-side
# |    rather than replacing the older version. By default, this installs
# |    VGC 2020 to:
# |        C:\Program Files\VGC 2020.0\
# |
# |- VGC 2020.zip:
#      This is a portable installation which you can unzip and run from
#      anywhere. However, it doesn't include any mechanism to easily update
#      to newer versions, and it doesn't create convenient shortcuts in your
#      Start Menu or Desktop (you'll have to do this manually if desired).
#      You may need to manually run the included vc_redist.x64.exe installer
#      if your system doesn't already have the appropriate Visual Studio
#      redistributables installed.
#
# Install family
# --------------
#
# We call "install family" a set of installers which can be upgraded from
# one another or share components. Two installers belong to the same
# install family if and only if all the following are true:
# - they are from the same manufacturer
# - they are from the same suite
# - they have the same version type ("vt" for short)
# - they have the same upgrade policy ("up" for short)
# - they are targeting the same architecture (x86 or x64)
# - if vt = "stable" and up = "minor": they have the same major version
# - if vt = "stable" and up = "none": they have the same minor and major version
# - if vt = "alpha" or "beta", they are from the same branch
# - if vt = "alpha" or "beta", and up = "none": they are from the same commit
#
# We conveniently use the name of the default install folder as an identifier
# for the family.
#
# For example, all installers of "stable" versions of VGC with the "all"
# upgrade policy are part of the same install family called "VGC". They all
# get installed by default to "C:/Program Files/VGC/". Below are examples of a
# few installers belonging to this install family:
#
# - "VGC 2020 Installer.exe"
# - "VGC Illustration 2020 Installer.exe"
# - "VGC 2020.1 Installer.exe"
# - "VGC 2021 Installer.exe"
#
# Examples of installers belonging to the "VGC 2020" install family, i.e.,
# versionType = "stable", upgradePolicy = "minor", and majorVersion = 2020:
#
# - "VGC 2020 Installer - Minor Updates Only.exe"
# - "VGC Illustration 2020 Installer - Minor Updates Only.exe"
# - "VGC 2020.1 Installer - Minor Updates Only.exe"
#
# Examples of installers belonging to the "VGC 2020.0" install family, i.e.,
# versionType = "stable", upgradePolicy = "none", and version = 2020.0:
#
# - "VGC 2020 Installer - No Updates.exe"
# - "VGC Illustration 2020 Installer - No Updates.exe"
#
# Examples of installers belonging to the "VGC 2020 Beta" install family, i.e.,
# versionType = "beta", majorVersion = "2020", and upgradePolicy = "minor",
#
# - "VGC 2020 Beta 2019-06-28.3 Installer.exe"
# - "VGC Illustration 2020 Beta 2019-06-28.3 Installer.exe"
# - "VGC 2020 Beta 2019-06-29.0 Installer.exe"
#
# Examples of installers belonging to the "VGC 2020 Beta 2019-06-28.3" install family, i.e.,
# versionType = "beta", majorVersion = "2020", upgradePolicy = "none", and commitDateAndIndex = "2019-06-28.3":
#
# - "VGC 2020 Beta 2019-06-28.3 Installer - No Updates.exe"
# - "VGC Illustration 2020 Beta 2019-06-28.3 Installer - No Updates.exe"
#
# Examples of installers belonging to the "VGC Alpha" install family, i.e.,
# versionType = "alpha", upgradePolicy = "minor"
#
# - "VGC Alpha 2019-06-28.3 Installer.exe"
# - "VGC Illustration Alpha 2019-06-28.3 Installer.exe"
# - "VGC Alpha 2019-06-29.0 Installer.exe"
#
# Examples of installers belonging to the "VGC Alpha 2019-06-28.3" install family, i.e.,
# versionType = "alpha", upgradePolicy = "none", and commitDateAndIndex = "2019-06-28.3":
#
# - "VGC Alpha 2019-06-28.3 Installer - No Updates.exe"
# - "VGC Illustration Alpha 2019-06-28.3 Installer - No Updates.exe"
#
# Install version
# ---------------
#
# Within each install family, we use a MSI version numbering scheme designed
# to work with the following MSI constraints:
#
#   https://docs.microsoft.com/en-us/windows/desktop/Msi/productversion
#
#   « The format of the string is as follows:
#
#       major.minor.build
#
#     The first field is the major version and has a maximum value of 255.
#     The second field is the minor version and has a maximum value of 255.
#     The third field is called the build version or the update version and
#     has a maximum value of 65,535. »
#
# The versionning scheme used depends on the version type:
#
# - stable: releaseYear.minor.0
#     Example 1:
#       versionMajor = 2020
#       versionMinor = 0
#       installVersion: 20.0.0
#       installFamily:
#         if upgradePolicy = "all":   VGC
#         if upgradePolicy = "minor": VGC 2020
#         if upgradePolicy = "none":  VGC 2020.0
#     Example 2:
#       versionMajor = 2020
#       versionMinor = 1
#       installVersion: 20.1.0
#       installFamily:
#         if upgradePolicy = "all":   VGC
#         if upgradePolicy = "minor": VGC 2020
#         if upgradePolicy = "none":  VGC 2020.1
#
# - beta: commitYear.commitMonth.(commitDay * 1000 + numCommitForThisDay)
#     Example 1:
#       versionMajor = 2020
#       commit: first commit of 2019-06-21 in the "2020" branch
#       installVersion: 19.6.21000
#       installFamily:
#         if upgradePolicy = "minor": VGC 2020 Beta
#         if upgradePolicy = "none":  VGC 2020 Beta 2019-06-21
#     Example 2:
#       versionMajor = 2020
#       commit: second commit of 2019-06-21 in the "gh62" branch
#       installVersion: 19.6.21001
#       installFamily:
#         if upgradePolicy = "minor": VGC 2020 Beta-gh62
#         if upgradePolicy = "none":  VGC 2020 Beta-gh62 2019-06-21.1
#
# - alpha: commitYear.commitMonth.(commitDay * 1000 + numCommitForThisDay)
#     Example 1:
#       commit: first commit of 2019-06-21 in the "master" branch
#       installVersion: 19.6.21000
#       installFamily:
#         if upgradePolicy = "all": VGC Alpha
#         if upgradePolicy = "none": VGC Alpha 2019-06-21
#     Example 2:
#       commit: second commit of 2019-06-21 in the "gh62" branch
#       installVersion: 19.6.21001
#       installFamily:
#         if upgradePolicy = "all":  VGC Alpha-gh62 gh62
#         if upgradePolicy = "none": VGC Alpha-gh62 gh62 2019-06-21.1
#
# Suite installers vs app installers
# ----------------------------------
#
# Within each install family, there are two kinds of installers:
# - suite installers: they install all VGC apps
# - app installers: they only install a specific VGC app
#
# Suite installers and app installers do not share the same MSI upgradeCode,
# but they do share components and are installed in the same folder.
#
# All the suite installers of a given install family share the same upgradeCode.
#
# All the app installers of a given install family share the same upgradeCode
# if and only if the are installers for the same app. For example, "VGC
# Illustration 2020.exe" and "VGC Animation 2020.exe" have a different
# upgradeCode, but "VGC Illustration 2020.exe" and "VGC Illustration 2021.exe"
# have the same upgradeCode.
#
# Product code
# ------------
#
# https://docs.microsoft.com/en-us/windows/desktop/msi/changing-the-product-code
#
# « The product code must be changed if any of the following are true for the
#   update:
#
#   - Coexisting installations of both original and updated products on the same
#     system must be possible.
#   - The name of the .msi file has been changed.
#   - The component code of an existing component has changed.
#   - A component is removed from an existing feature.
#   - An existing feature has been made into a child of an existing feature.
#   - An existing child feature has been removed from its parent feature. »
#
# In other words, all installers must have a unique product code, even if they
# are part of the install family or even the same build.
#
# Component GUID
# --------------
#
# Component GUIDs should be unique for each file and for each install family.
# However, it is shared across app/suite installers, and doesn't change from
# version to version. For example, the file "bin/vgcillustration.exe" has the
# same GUID in all those .msi files:
#
# - VGC 2020.msi
# - VGC Illustration 2020.msi
# - VGC 2020.1.msi
# - VGC 2021.msi
#
# If we would change the GUID from version to version, the file would not be
# properly updated during an upgrade. For example, if the upgrade is scheduled
# late (e.g., Schedule="afterInstallExecute"), then this is what would happen:
# - Install v2:
#     Component myfile.txt (v2) is installed. This overrides myfile.txt on disk.
# - Uninstall v1:
#     Component myfile.txt (v1) is uninstalled, since Windows Installer believes
#     that this component only exists in v1 and not v2, since they don't have
#     the same GUID. This deletes myfile.txt on disk.
# - End result:
#     The file myfile.txt is deleted instead of being updated.
#
# Note: On macOS, the equivalent of sharing component GUIDs across different
# apps would be to use frameworks:
#
# https://www.bignerdranch.com/blog/it-looks-like-you-are-trying-to-use-a-framework
#
# but it is unclear whether all of this is worth it on macOS, where users are
# used to simply drag and drop the bundle in the Application folder. TODO: Look
# at what Open Office does, which is a similar use case as ours.
#

from pathlib import Path
from urllib.request import urlopen
from xml.dom.minidom import getDOMImplementation
import argparse
import base64
import datetime
import hashlib
import json
import os
import re
import subprocess
import sys
import time
import uuid

# Converts any string identifier into a valid WiX Id, that is:
# - only contain ASCII characters A-Z, a-z, digits, underscores, or period,
# - begin with either a letter or an underscore
# - is 72 characters or less
# - is 57 characters or less if it is an Icon Id.
#
# For now we do this by computing a md5 hash (prepended with an underscore
# otherwise the hash may start with a digit which is forbidden). In the
# future, to make the ID more human friendly, we may use a combination
# of hash (for uniqueness), but still with the input ID as intact as possible.
#
def encodeId(s):
    return "_" + hashlib.md5(s.encode('utf-8')).hexdigest()

# Returns a copy of the text, with all occurrences of "key" replaced by
# replacements["key"], for all keys in the replacements dictionary.
#
# Example:
#
#   s1 = "Hello world 1, Hello world 2"
#   s2 = replace_all(s, {"Hello": "Hi", "world": "World"})
#   print(s2)
#   # => "Hi World 1, Hi World 2"
#
def replace_all(text, replacements):
    replacements = dict((re.escape(k), v) for k, v in replacements.items())
    pattern = re.compile("|".join(replacements.keys()))
    return pattern.sub(lambda m: replacements[re.escape(m.group(0))], text)

# A class to perform code signing. We encapsulate the class in a resource
# manager to ensure that the certificate file is properly deleted when the
# script exits or an exception is raised. See the following link for a
# description of this pattern: https://stackoverflow.com/a/865272
#
class CodeSignerResource:
    def __init__(self, buildDir):
        self.buildDir = buildDir

    def __enter__(self):
        class CodeSigner:
            def __init__(self, buildDir, codeSignUrl, codeSignUrlKey):
                self.certificate = None
                self.password = None
                self.lastSignTime = None
                self.delayBetweenSigns = 20 # Delay in seconds to avoid surcharching server
                self.dualSign = False
                if codeSignUrl is not None and codeSignUrl != "":
                    print("Obtaining code signing certificate... ", end='')
                    try:
                        data = json.load(urlopen(codeSignUrl + "?key=" + codeSignUrlKey))
                        certbytes = base64.b64decode(data['certificate'])
                        self.certificate = buildDir / "codesign.pfx"
                        self.certificate.write_bytes(certbytes)
                        self.password = data['password']
                        print("OK.", flush=True)
                    except:
                        self.cleanup()
                        print("FAIL.", flush=True)

            def cleanup(self):
                # Delete certificate file if any
                if self.certificate and self.certificate.exists():
                    self.certificate.unlink()
                self.certificate = None
                self.password = None

            def sign_(self, file, fd="sha256", dualSign=True, verbose=False):
                if (self.certificate):
                    args = ["signtool.exe", "sign", "/f", str(self.certificate), "/p", self.password]
                    if fd == "sha256":
                        args += ["/fd", fd, "/tr", "http://timestamp.comodoca.com/?td=" + fd, "/td", fd]
                    else:
                        args += ["/t", "http://timestamp.comodoca.com"] # XXX shouldn't it be http://timestamp.comodoca.com/authenticode ?
                    if dualSign:
                        args.append("/as")
                    if verbose:
                        args.append("/v")
                    args.append(str(file))
                    if self.lastSignTime != None:
                        elapsed = int(time.time()) - self.lastSignTime
                        if elapsed < self.delayBetweenSigns:
                            s = self.delayBetweenSigns - elapsed
                            t = datetime.datetime.utcnow().isoformat()
                            print(f"{t}: Waiting {s} seconds to avoid surcharging timestamp server...", flush=True)
                            time.sleep(s)
                    t = datetime.datetime.utcnow().isoformat()
                    print(f"{t}: Signing with an {fd} timestamp...", flush=True)
                    subprocess.run(args)
                    self.lastSignTime = int(time.time())

            # Signs the given file. If dualSign = True and the file is an EXE or
            # a DLL, then we dual-sign it with both SHA-1 and SHA-256 for
            # compatibility with Windows Vista. If it is an MSI file, we only
            # sign it with the newer SHA-256 since MSI files do not support
            # dual-signing.
            #
            # Note: https://support.ksoftware.net/support/solutions/articles/17133-what-is-a-timestamp-
            #
            #  As of May 30th 2020, SHA1 timestamping is effectively deprecated
            #  as the SHA1 roots have expired. Use only the SHA256 timestamp
            #  server from now on.
            #
            def sign(self, file):
                if (self.certificate):
                    if not self.dualSign or file.suffix == ".msi":
                        self.sign_(file, fd="sha256", dualSign=False)
                    else:
                        self.sign_(file, fd="sha1", dualSign=False)
                        self.sign_(file, fd="sha256", dualSign=True)
                else:
                    print(f"Skipping signature step (no certificate provided)", flush=True)

        codeSignUrl = os.getenv("VGC_CODESIGN_URL")
        codeSignUrlKey = os.getenv("VGC_APPVEYOR_KEY")
        self.codeSigner = CodeSigner(self.buildDir, codeSignUrl, codeSignUrlKey)
        return self.codeSigner

    def __exit__(self, exc_type, exc_value, traceback):
        self.codeSigner.cleanup()

# This is a convenient class to generate a wxs file to be compiled
# with WiX. Traditionally, these are generated by using WiX tools such
# as 'heat', but we prefer the flexibility of a real programming language
# such as Python for this.
#
class Wix:

    # Creates a new Wix XML document with the following automatically
    # generated WixElements:
    # - wix.root: the root Wix element
    # - wix.product: the Product element
    # - wix.package: the Package element
    # - wix.media: the Media element
    #
    # And the following directories:
    # - wix.targetDirectory: the root of the selected install disk (e.g., "C:")
    # - wix.programFilesDirectory: "Program Files" directory
    # - wix.installDirectory:      "Program Files\[appOrSuiteName]"
    # - wix.startMenuDirectory:    Windows' Start Menu
    # - wix.desktopDirectory:      Windows' desktop
    #
    # Use appName = None for a "Suite" installer.
    #
    def __init__(
            self, wixDir,
            manufacturer, suiteName, appName, architecture,
            versionType, versionMajor, versionMinor, upgradePolicy,
            commitBranch, commitDate, commitIndex):

        self.wixDir = wixDir
        self.manufacturer = manufacturer
        self.suiteName = suiteName
        self.appName = appName
        self.architecture = architecture
        self.versionType = versionType
        self.versionMajor = versionMajor
        self.versionMinor = versionMinor
        self.upgradePolicy = upgradePolicy
        self.commitBranch = commitBranch
        self.commitDate = commitDate
        self.commitIndex = commitIndex

        # Check versionType
        allowedVersionTypes = ["stable", "beta", "alpha"]
        if not self.versionType in allowedVersionTypes:
            raise ValueError(
                'Unknown versionType: "' + self.versionType + '".' +
                ' Allowed values are: "' + '", "'.join(allowedVersionTypes) + '".')

        # Check commit information
        if versionType in ["beta", "alpha"]:
            hasCommitInfo = (self.commitBranch and
                             self.commitDate and
                             self.commitIndex)
            if not hasCommitInfo:
                raise ValueError(
                    'Commit info not found while required to deploy when' +
                    ' versionType = "' + versionType + '.' +
                    ' Please compile from a git repository.')

        # Check upgradePolicy
        allowedUpgradePolicies = {
            "stable": ["all", "minor", "none"],
            "beta":   ["minor", "none"],
            "alpha":  ["minor", "none"]
        }
        if not self.upgradePolicy in allowedUpgradePolicies[self.versionType]:
            raise ValueError(
                'Unknown upgradePolicy: "' + self.upgradePolicy + '".' +
                ' Allowed values when versionType = "' + self.versionType + '"'
                ' are: ' + '", "'.join(allowedUpgradePolicies[self.versionType]) + '".')

        # Check architecture
        allowedArchitectures = ["x86", "x64"]
        if not self.architecture in allowedArchitectures:
            raise ValueError(
                'Unknown architecture: "' + self.versionType + '".' +
                ' Allowed values are: "' + '", "'.join(allowedArchitectures) + '".')

        # Set appOrSuiteName. Examples:
        # - for suite installers: "VGC"
        # - for app installers: "VGC Illustration", "VGC Animation", etc.
        self.appOrSuiteName = self.suiteName
        if self.appName:
            self.appOrSuiteName += " " + self.appName

        # Set commitDateAndIndex
        # Example: "2019-07-08.1"
        if self.versionType in ["beta", "alpha"]:
            commitDateAndIndex = self.commitDate + "." + self.commitIndex

        # Set branchSuffix
        # Example: "-gh62"
        branchSuffix = ""
        if ((self.versionType == "beta" and self.commitBranch != self.versionMajor) or
            (self.versionType == "alpha" and self.commitBranch != "master")):
            branchSuffix = "-" + self.commitBranch

        # Set installVersion and installHumanVersion
        # Example: "19.7.08001" / "2019-07-08.1"
        if self.versionType == "stable":
            major = int(self.versionMajor[2:4])
            minor = int(self.versionMinor)
            self.installVersion = "{}.{}".format(major, minor)
            self.installHumanVersion = "{}.{}".format(self.versionMajor, minor)
        elif self.versionType in ["beta", "alpha"]:
            major = int(self.commitDate[2:4])
            minor = int(self.commitDate[5:7])
            build = int(self.commitDate[8:10]) * 1000 + int(self.commitIndex)
            self.installVersion = "{}.{}.{}".format(major, minor, build)
            self.installHumanVersion = commitDateAndIndex

        # Set installFamilySuffix and fullVersion (except the architecture suffix)
        # Example:
        #   installFamilySuffix = " 2020 Beta"
        #   fullVersion     = "2020 Beta 2019-07-08.1"
        if self.versionType == "stable":
            if   self.upgradePolicy == "all":   p = ""
            elif self.upgradePolicy == "minor": p = " {}"
            elif self.upgradePolicy == "none":  p = " {}.{}"
            self.installFamilySuffix = p.format(self.versionMajor, self.versionMinor)
            self.fullVersion = self.versionMajor
            if minor > 0:
                self.fullVersion += "." + self.versionMinor
        elif self.versionType == "beta":
            if   self.upgradePolicy == "minor": p = " {} Beta{}"
            elif self.upgradePolicy == "none":  p = " {} Beta{} {}"
            q                     =  "{} Beta{} {}"
            self.installFamilySuffix = p.format(self.versionMajor, branchSuffix, commitDateAndIndex)
            self.fullVersion         = q.format(self.versionMajor, branchSuffix, commitDateAndIndex)
        elif self.versionType == "alpha":
            if   self.upgradePolicy == "minor": p = " Alpha{}"
            elif self.upgradePolicy == "none":  p = " Alpha{} {}"
            q                     =  "Alpha{} {}"
            self.installFamilySuffix = p.format(branchSuffix, commitDateAndIndex)
            self.fullVersion         = q.format(branchSuffix, commitDateAndIndex)

        # Set architecture and add it as suffix to installFamily and fullVersion
        # Example:
        #   installFamilySuffix = " 2020 Beta 32-bit"
        #   fullVersion     = "2020 Beta 2019-07-08.1 32-bit"
        if self.architecture == "x64":
            self.win64yesno = "yes"
            self.programFilesFolder = "ProgramFiles64Folder"
        elif self.architecture == "x86":
            self.win64yesno = "no"
            self.programFilesFolder = "ProgramFilesFolder"
            self.installFamilySuffix += " 32-bit"
            self.fullVersion     += " 32-bit"

        # Set installFamily
        # Example: "VGC 2020 Beta 32-bit"
        self.installFamily = self.suiteName + self.installFamilySuffix

        # Set installerName and msiName
        if self.versionType == "stable" and self.upgradePolicy == "minor":
            installerTypeSuffix = " - Minor Updates Only"
        elif self.upgradePolicy == "none":
            installerTypeSuffix = " - No Updates"
        else:
            installerTypeSuffix = ""
        self.msiName       = self.appOrSuiteName + " " + self.fullVersion + installerTypeSuffix
        self.installerName = self.appOrSuiteName + " " + self.fullVersion + " Installer" + installerTypeSuffix

        # Create XML document.
        #
        domImplementation = getDOMImplementation()
        self.domDocument = domImplementation.createDocument(None, "Wix", None)

        # Get Wix root element and set its xmlns attribute.
        #
        self.root = WixElement(self.domDocument.documentElement, self, "")
        self.root.setAttribute("xmlns", "http://schemas.microsoft.com/wix/2006/wi")

        # Add product. Note that we regenerate its ProductCode GUID
        # (= 'Id' attribute) when the version change since we consider all
        # changes of version to be "Major Upgrades" in MSI terminology. We need
        # this since components may be added, moved or removed even when going
        # from VGC 2020.0 to VGC 2020.1. Also, we desire that the name of our
        # MSI file changes in this case, and it is a Microsoft requirement to
        # keep the same MSI filename during a "Minor Upgrade" or "Small Upgrade".
        #
        self.product = self.root.createChild("Product", [
            ("Name", self.appOrSuiteName + self.installFamilySuffix),
            ("Id", self.dynamicGuid("Product/ProductCode/" + self.appOrSuiteName)),
            ("UpgradeCode", self.staticGuid("Product/UpgradeCode/" + self.appOrSuiteName)),
            ("Language", "1033"),
            ("Codepage", "1252"),
            ("Version", self.installVersion),
            ("Manufacturer", self.manufacturer)])

        # Add package
        #
        # Note: manual specification of "Platform" is discouraged in favour
        # of using the candle.exe -arch x64/x86 switch, but we use both anyway.
        #
        self.package = self.product.createChild("Package", [
            ("Id", self.dynamicGuid("Package/Id/" + self.appOrSuiteName)),
            ("Keywords", "Installer"),
            ("Description", "Installer of " + self.msiName),
            ("Manufacturer", self.manufacturer),
            ("InstallerVersion", "500"),
            ("InstallPrivileges", "elevated"),
            ("InstallScope", "perMachine"),
            ("Languages", "1033"),
            ("Compressed", "yes"),
            ("SummaryCodepage", "1252"),
            ("Platform", self.architecture)])

        # Add media
        self.media = self.product.createChild("Media", [
            ("Id", "1"),
            ("Cabinet", "Cabinet.cab"),
            ("EmbedCab", "yes")])

        # See: https://stackoverflow.com/questions/2058230/wix-create-non-advertised-shortcut-for-all-users-per-machine
        self.product.createChild("Property", [
            ("Id", "DISABLEADVTSHORTCUTS"),
            ("Value", "1")])

        # Allow major upgrades and prevent downgrades.
        #
        # We use the afterInstallExecute schedule in order not to break user
        # shortcuts in the Desktop or Task Bar, see:
        #     http://windows-installer-xml-wix-toolset.687559.n2.nabble.com/Keep-Desktop-Shortcuts-and-Pinned-Task-Bar-Icons-with-Upgrades-td7599707.html
        #     https://stackoverflow.com/questions/32318662/user-pinned-taskbar-shortcuts-and-major-upgrades
        #
        self.majorUpgrades = self.product.createChild("MajorUpgrade", [
            ("Schedule", "afterInstallExecute"),
            ("DowngradeErrorMessage", "A newer version of [ProductName] is already installed.")])

        # Add basic directory structure.
        self.targetDirectory = self.product.createDirectory("SourceDir", "TARGETDIR")
        self.programFilesDirectory = self.targetDirectory.createDirectory("PFiles", self.programFilesFolder)
        self.installDirectory = self.programFilesDirectory.createDirectory(self.installFamily, "INSTALLFOLDER")
        self.startMenuDirectory = self.targetDirectory.createDirectory("Programs", "ProgramMenuFolder")
        self.desktopDirectory = self.targetDirectory.createDirectory("Desktop", "DesktopFolder")

    # Generates a deterministic GUID based on the installFamily, the
    # installVersion, and the given string identifier "sid". This GUID changes
    # from version to version but doesn't depend on the appName, which makes
    # it suitable for shared components. You must manually include the appName
    # in the sid if you wish this GUID to depend on the appName.
    #
    def dynamicGuid(self, sid):
        u = uuid.uuid5(uuid.NAMESPACE_URL,
                    "http://dynamicguid.msi.vgc.io" +
                    "/" + self.manufacturer +
                    "/" + self.installFamily +
                    "/" + self.installVersion +
                    "/" + sid)
        return str(u).upper()

    # Generates a deterministic GUID based on the installFamily and the given
    # string identifier "sid". This GUID doesn't change from version to
    # version (as long at it belongs to the same installFamily) neither to the
    # appName. You must manually include the appName in the sid if you wish
    # this GUID to depend on the appName, e.g., for the upgradeCode.
    #
    def staticGuid(self, sid):
        u = uuid.uuid5(uuid.NAMESPACE_URL,
                    "http://staticguid.msi.vgc.io" +
                    "/" + self.manufacturer +
                    "/" + self.installFamily +
                    "/" + sid)
        return str(u).upper()

    # Generates the setup file
    #
    def makeSetup(self, srcDir, deployDir, icon, logo, logoside, codeSigner):
        msi_wxs             = deployDir / (self.msiName + ".wxs")
        msi_wixobj          = deployDir / (self.msiName + ".wixobj")
        msi                 = deployDir / (self.msiName + ".msi")
        bundle_template_wxs = srcDir / "tools" / "windows" / "bundle.wxs"
        bundle_wxs          = deployDir / (self.installerName + ".wxs")
        bundle_wixobj       = deployDir / (self.installerName + ".wixobj")
        installer_exe       = deployDir / (self.installerName + ".exe")
        engine_exe          = deployDir / (self.installerName + ".engine.exe")
        vcredist            = deployDir / "vc_redist.x64.exe"
        vcredistVersion     = (deployDir / "vc_redist.x64.exe.version").read_text().strip()
        binDir = self.wixDir / "bin"

        print(f"\n_____________________________________________________________", flush=True)
        print(f"Generating {installer_exe.name}", flush=True)

        # Generate the .wxs file of the MsiPackage
        msi_wxs.write_bytes(self.domDocument.toprettyxml(encoding='windows-1252'))

        # Generate the .wxsobj file of the MsiPackage
        # -sw1091: Silence warning CNDL1091: The Package/@Id attribute has been set.
        print(f"\nStep 1: Running Candle for the MSI:", flush=True)
        subprocess.run([
            str(binDir / "candle.exe"), str(msi_wxs),
            "-sw1091",
            "-arch", self.architecture,
            "-out", str(msi_wixobj)])

        # Generate the .msi file of the MsiPackage
        # ICE07/ICE60: Remove warnings about font files. See:
        # https://stackoverflow.com/questions/13052258/installing-a-font-with-wix-not-to-the-local-font-folder
        print(f"\nStep 2: Running Light for the MSI:", flush=True)
        subprocess.run([
            str(binDir / "light.exe"), str(msi_wixobj),
            "-sice:ICE07", "-sice:ICE60",
            "-out", str(msi)])

        # Sign the MSI
        print(f"\nStep 3: Signing the MSI:", flush=True)
        codeSigner.sign(msi)

        # Generate the .wxs file of the Bundle
        bundle_text = bundle_template_wxs.read_text()
        bundle_wxs.write_text(replace_all(bundle_text, {
            "$(var.name)":                self.msiName,
            "$(var.manufacturer)":        self.manufacturer,
            "$(var.upgradeCode)":         self.staticGuid("Bundle/UpgradeCode/" + self.appOrSuiteName),
            "$(var.version)":             self.installVersion,
            "$(var.icon)":                str(icon),
            "$(var.logo)":                str(logo),
            "$(var.logoside)":            str(logoside),
            "$(var.themeFile)":           str(srcDir / "tools" / "windows" / "VgcTheme.xml"),
            "$(var.localizationFile)":    str(srcDir / "tools" / "windows" / "VgcTheme.wxl"),
            "$(var.installFamily)":       self.installFamily,
            "$(var.installHumanVersion)": self.installHumanVersion,
            "$(var.msi)":                 str(msi),
            "$(var.vcredist)":            str(vcredist),
            "$(var.vcredistVersion)":     vcredistVersion}))

        # Generate the .wxsobj file of the Bundle
        print(f"\nStep 4: Running Candle for the .exe installer:", flush=True)
        subprocess.run([
            str(binDir / "candle.exe"), str(bundle_wxs),
            "-ext", "WixBalExtension",
            "-ext", "WixUtilExtension",
            "-arch", self.architecture,
            "-out", str(bundle_wixobj)])

        # Generate the .exe file of the Bundle
        print(f"\nStep 5: Running Light for the .exe installer:", flush=True)
        subprocess.run([
            str(binDir / "light.exe"), str(bundle_wixobj),
            "-ext", "WixBalExtension",
            "-ext", "WixUtilExtension",
            "-out", str(installer_exe)])

        # Detach and sign the burn engine, then re-attach and sign the installer.
        # See: https://wixtoolset.org/documentation/manual/v3/overview/insignia.html
        # We delete the engine afterwards to avoid deploying it as AppVeyor artifact.
        print(f"\nStep 6: Running Insignia to extract the burn engine:", flush=True)
        subprocess.run([
            str(binDir / "insignia.exe"),
            "-ib", str(installer_exe),
            "-o", str(engine_exe)])
        print(f"\nStep 7: Signing the burn engine:", flush=True)
        codeSigner.sign(engine_exe)
        print(f"\nStep 8: Running Insignia to re-attach the burn engine:", flush=True)
        subprocess.run([
            str(binDir / "insignia.exe"),
            "-ab", str(engine_exe), str(installer_exe),
            "-o", str(installer_exe)])
        print(f"\nStep 9: Signing the .exe installer:", flush=True)
        codeSigner.sign(installer_exe)
        engine_exe.unlink()

    # Creates a new feature. Note: feature names cannot be longer than 38 characters in length.
    #
    def createFeature(self, name, level = 1):
        return self.product.createChild("Feature", [
            ("Id", name),
            ("Level", str(level))])

    # Creates a new icon.
    #
    def createIcon(self, srcFile):
        # Note: icon Ids must end with either .ico or .exe
        # and match the extension of the filename.
        basename = srcFile.with_suffix("").name
        suffix = srcFile.suffix
        iconId = encodeId("Icon/" + basename) + suffix
        icon = self.product.createChild("Icon", [
            ("Id", iconId),
            ("SourceFile", str(srcFile))])
        icon.iconId = iconId
        return icon

# A WixElement represents an XML element in the WiX file.
# It is a wrapper around dom.Element providing more convenient API.
#
class WixElement:

    # Wraps a dom.Element into a WixElement. This is an implementation detail,
    # clients are not supposed to call this themselves, but instead to use the
    # WixElement.createChild() function.
    #
    def __init__(self, domElement, wix, dirId):
        self.domElement = domElement
        self.wix = wix
        self.dirId = dirId

    # Sets the attribute of this WixElement
    #
    def setAttribute(self, name, value):
         self.domElement.setAttribute(name, value)

    # Gets the value of the given attribute of this WixElement
    #
    def getAttribute(self, name):
         self.domElement.getAttribute(name)

    # Creates a new Element with the given tagName, and appends this new
    # Element as a child of the given parent Element. Returns the new Element.
    #
    def createChild(self, tagName, attributes):
        domChild = self.wix.domDocument.createElement(tagName)
        self.domElement.appendChild(domChild)
        child = WixElement(domChild, self.wix, self.dirId)
        for pair in attributes:
            child.setAttribute(pair[0], pair[1])
        return child

    # Creates a new Directory with the given name and the given ID.
    # If no ID is provided, one will be automatically generated.
    #
    def createDirectory(self, name, dirId = None):
        if dirId is None:
            dirId = encodeId("Directory/" + self.dirId + '/' + name)
        child = self.createChild("Directory", [("Id", dirId), ("Name", name)])
        child.dirId = dirId
        child.files = {}
        return child

    # Creates a shortcut.
    #
    # Note: We need to create a registry value because for some reason, a
    # shortcut cannot be used as KeyPath. References:
    # https://wixtoolset.org/documentation/manual/v3/xsd/wix/shortcut.html
    # https://wixtoolset.org/documentation/manual/v3/howtos/files_and_registry/create_start_menu_shortcut.html
    # http://windows-installer-xml-wix-toolset.687559.n2.nabble.com/desktop-shortcut-guidance-td7463861.html
    # https://stackoverflow.com/questions/11868499/create-shortcut-to-desktop-using-wix
    #
    def createShortcut(self, directory, name, workingDirectory, icon):
        shortcutId = encodeId("Shortcut/" + directory.dirId + "/" + name)
        shortcut = self.createChild("Shortcut", [
            ("Id", shortcutId),
            ("Directory", directory.dirId),
            ("Name", name),
            ("WorkingDirectory", workingDirectory.dirId),
            ("Icon", icon.iconId),
            ("IconIndex", "0"),
            ("Advertise", "yes")])
        return shortcut

    # Creates a submenu of the start menu.
    # It has to be associated with a feature so that it is deleted when the
    # feature is uninstalled.
    #
    def createSubMenu(self, name, feature):
        subMenuDirectory = self.createDirectory(name)
        componentId = encodeId("SubMenuComponent/" + subMenuDirectory.dirId)
        subMenuComponent = subMenuDirectory.createChild("Component", [
            ("Id", componentId),
            ("Guid", self.wix.dynamicGuid(componentId))])
        subMenuComponent.createChild("RemoveFolder", [
            ("Id", encodeId("RemoveFolder/" + componentId)),
            ("On", "uninstall")])
        subMenuComponent.createChild("RegistryValue", [
            ("Root", "HKCU"),
            ("Key", "Software\\[Manufacturer]\\[ProductName]"),
            ("Type", "string"),
            ("Value", ""),
            ("KeyPath", "yes")])
        feature.createChild("ComponentRef", [("Id", componentId)])
        return subMenuDirectory

    # Creates a file.
    #
    def createFile(self, srcFile, name, feature):
        componentId = encodeId("FileComponent/" + self.dirId + "/" + name)
        fileId = encodeId("File/" + componentId)
        fileComponent = self.createChild("Component", [
            ("Id", componentId),
            ("Guid", self.wix.staticGuid("Component/Guid/" + componentId)),
            ("Win64", self.wix.win64yesno)])
        file = fileComponent.createChild("File", [
            ("Id", fileId),
            ("Name", name),
            ("DiskId", "1"),
            ("Source", str(srcFile)),
            ("KeyPath", "yes")])
        feature.createChild("ComponentRef", [("Id", componentId)])
        file.fileId = fileId
        file.fileName = name
        self.files[name] = file
        return file

    # Returns a previously created file
    #
    def getFile(self, name):
        return self.files[name]

    # Recursively add the given directory and all its files to this directory
    # for the given feature. Files or directories whose names appear in the
    # `exclude` list are ignored.
    #
    def addDirectory(self, srcDir, feature, *, exclude=[]):
        destDir = self.createDirectory(srcDir.name)
        for child in srcDir.iterdir():
            if child.name in exclude:
                pass
            elif child.is_file():
                destDir.createFile(str(child), child.name, feature)
            elif child.is_dir():
                destDir.addDirectory(child, feature)
        return destDir

# Generates an MSI file for the given suite
#
def makeSuiteInstaller(
        srcDir, buildDir, configDir, deployDir, wixDir,
        manufacturer, suiteName, appNames, architecture,
        versionType, versionMajor, versionMinor, upgradePolicy,
        commitBranch, commitDate, commitIndex,
        codeSigner):

    # General configuration
    appName = None
    wix = Wix(
        wixDir,
        manufacturer, suiteName, appName, architecture,
        versionType, versionMajor, versionMinor, upgradePolicy,
        commitBranch, commitDate, commitIndex)
    appFeature = wix.createFeature("Complete")

    # Add 'bin', 'python', and 'resources' directories
    wixBinDir = wix.installDirectory.addDirectory(configDir / "bin", appFeature, exclude=["vgc.conf"])
    wix.installDirectory.addDirectory(configDir / "python", appFeature)
    wix.installDirectory.addDirectory(configDir / "resources", appFeature)

    # Create Desktop and Start Menu shortcuts
    for appName in appNames:
        appNameLower = appName.lower()
        appExecutableBasename = "vgc" + appNameLower
        appExecutableName = appExecutableBasename + ".exe"
        appExecutable = wixBinDir.getFile(appExecutableName)
        appShortcutName = suiteName + " " + appName + wix.installFamilySuffix
        workingDirectory = wixBinDir
        appIconPath = srcDir / "apps" / appNameLower / (appExecutableBasename + ".ico")
        appIcon = wix.createIcon(appIconPath)
        appExecutable.createShortcut(wix.startMenuDirectory, appShortcutName, workingDirectory, appIcon)
        appExecutable.createShortcut(wix.desktopDirectory, appShortcutName, workingDirectory, appIcon)

    # Generate the Setup file
    setupIcon = appIconPath # TODO: use non-app-specific VGC icon instead
    setupLogo = srcDir / "tools" / "windows" / "setuplogo.png"
    setupLogoSide = srcDir / "tools" / "windows" / "setuplogoside.png"
    wix.makeSetup(srcDir, deployDir, setupIcon, setupLogo, setupLogoSide, codeSigner)

# Returns a value from an INI-formatted file. Does not support comments, nor
# spaces surrounding the equal size.
#
# Example:
# s = "a=Hello\nb=World"
# getIniValue(s, "b")      # -> "World"
#
def getIniValue(ini, key):
    return re.search("^" + key + "=(.*)$", ini, flags = re.MULTILINE).group(1)

# Script entry point.
#
if __name__ == "__main__":

    # Parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument("srcDir", help="path to the source directory")
    parser.add_argument("buildDir", help="path to the build directory")
    parser.add_argument("config", help="build configuration (e.g.: 'Release', 'Debug')")
    parser.add_argument("wixDir", help="path to the directory where WiX is installed")
    args = parser.parse_args()

    # Import arguments into the global namespace.
    # This allows to more simply write `foo` instead of `args.foo`.
    globals().update(vars(args))

    # Get and create useful directories
    srcDir = Path(srcDir)
    buildDir = Path(buildDir)
    configDir = buildDir / config
    deployDir = buildDir / "deploy" / config
    deployDir.mkdir(parents=True, exist_ok=True)
    wixDir = Path(wixDir)

    # Get version info
    versionFile = configDir / "resources" / "core" / "version.txt"
    version = versionFile.read_text()
    manufacturer = getIniValue(version, "manufacturer")
    suiteName = getIniValue(version, "suite")
    versionType = getIniValue(version, "versionType")
    versionMajor = getIniValue(version, "versionMajor")
    versionMinor = getIniValue(version, "versionMinor")
    commitBranch = getIniValue(version, "commitBranch")
    commitDate = getIniValue(version, "commitDate")
    commitIndex = getIniValue(version, "commitIndex")
    architecture = getIniValue(version, "buildArchitecture")

    # List of all apps of the suite to install
    appNames = [ "Illustration" ]

    # List all possible upgrade policies for the given version type.
    if versionType == "stable":
        upgradePolicies = ["all", "minor", "none"]
    else:
        upgradePolicies = ["minor", "none"]

    # Get CodeSigner resource and proceed to the creation of installers
    #
    with CodeSignerResource(buildDir) as codeSigner:

        # Create one suite installer for each upgrade policy.
        # For now, we don't create app installers.
        for upgradePolicy in upgradePolicies:
            makeSuiteInstaller(
                srcDir, buildDir, configDir, deployDir, wixDir,
                manufacturer, suiteName, appNames, architecture,
                versionType, versionMajor, versionMinor, upgradePolicy,
                commitBranch, commitDate, commitIndex,
                codeSigner)

        # TODO: also create a ZIP for portable installation (like Blender
        # does), with updates disabled.
