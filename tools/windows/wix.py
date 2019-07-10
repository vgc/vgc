#!/usr/bin/env python3
#
# Useful resources on Windows Installer and WiX
# ---------------------------------------------
#
# - https://docs.microsoft.com/en-us/windows/desktop/Msi/windows-installer-portal
# - https://www.firegiant.com/wix/tutorial/
# - https://stackoverflow.com/questions/471424/wix-tricks-and-tips
#
# Other useful resources or inspiration sources
# ---------------------------------------------
#
# - https://wiki.qt.io/Branch_Guidelines
#
# Suite
# -----
#
# We call a "suite" a group of related apps which are designed to be installed
# together and share resources, such as DLLs, images, fonts, etc.
#
# For example, "Adobe CC" and "Open Office" are suites. In our case, we are
# developing a suite named "VGC".
#
# Version type
# ------------
#
# We call "version type" one of the following:
#
# - Stable:
#     Build based on a specific, hand-picked commit on a release branch, deemed
#     sufficiently stable for production use.
#
# - Beta:
#     Build based on a commit in a release branch, for example the "2020"
#     branch. Release branches are created after a feature freeze, therefore
#     only bug fixes go to such branches, which means that beta versions
#     stabilize over time.
#
# - Alpha:
#     Build based on a commit in the "master" branch.
#
# - Branch:
#     Any build based on a commit in neither of the previous branches.
#
# - Local:
#     Any build which is neither of the previous types, for example because the
#     version control system is not available.
#
# Builds of VGC with different version types are always considered to be
# different programs, and are installed side-by-side in different folders.
# For example, with upgradePolicy = "all" (see next section), the default
# install folders for stable, beta, alpha, branch, and local versions are:
#
# - C:/Program Files/VGC/
# - C:/Program Files/VGC Beta/
# - C:/Program Files/VGC Alpha/
# - C:/Program Files/VGC Branch <branch-name>/
# - C:/Program Files/VGC Local <build-date>.<random-id>/
#
# Where <random-id> is a random sequence of alphanumeric characters long enough
# to make collisions unlikely.
#
# Upgrade policy
# --------------
#
# The "upgrade policy" determines whether installing a newer version of VGC
# suite replaces the existing version, or if instead both versions are installed
# side-by-side. There are three possible upgrade policies:
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
# The upgrade policy also affects which name is given to the install folder, so
# it makes sense that it cannot be changed after installation. For example, any
# "stable" version with the "all" upgrade policy is installed (by default) to:
#
#   C:/Program Files/VGC/
#
# But "stable" versions with the "minor" upgrade policy are installed to a folder
# indicating its major version, in order to allow different major versions to be
# installed side-by-side, for example:
#
#   C:/Program Files/VGC 2020/
#   C:/Program Files/VGC 2021/
#   ...
#
# Below is a table giving examples of what the default install folder name
# looks like based on the version type and the upgrade policy:
#
#            |    "all"          |     "minor"     |          "none"
# -----------+-------------------+-----------------+------------------------------
# "Stable"   |  VGC              |  VGC 2020       |  VGC 2020.0
# "Beta"     |  VGC Beta         |  VGC 2020 Beta  |  VGC 2020 Beta 2019-06-28.3
# "Alpha"    |  VGC Alpha        |  N/A            |  VGC Alpha 2019-06-28.3
# "Branch"   |  VGC Branch gh62  |  N/A            |  VGC Branch gh62 2019-07-06.2
# "Local"    |  N/A              |  N/A            |  VGC Local 2019-07-06.s9wj3g
#
# We note that the "minor" upgrade policy doesn't make sense for alpha and
# branch versions, since the concept of minor vs. major versions doesn't exist
# for these version types. Also, builds which aren't version controlled are
# non-upgradable so only the "none" upgrade policy is possible.
#
# In order to help the user choose the desired upgrade policy, the Download page
# may look like the following:
#
# [Download VGC 2020]      <-- link to VGC 2020 Installer.exe
#
# + More options           <-- dropdown menu revealing more options on click
# |
# |- VGC 2020 Installer.exe:
# |    This is the default and most recommended option, which includes a built-in
# |    system to easily update to any newer version if/when desired. By default,
# |    it installs VGC 2020 and subsequent minor and major updates to:
# |        C:/Program Files/VGC/
# |
# |- VGC 2020 Installer (minor updates only).exe:
# |    This is for advanced users who would like the possibility to install
# |    different major versions of VGC side-by-side (example: VGC 2020 and
# |    VGC 2021), while still keeping the ability to easily update to newer
# |    minor versions, if any (example: VGC 2020 to VGC 2020.1). By default,
# |    it installs VGC 2020 and subsequent minor updates to:
# |        C:/Program Files/VGC 2020/
# |
# |- VGC 2020 Installer (no updates).exe:
# |    This is for advanced users who would like the possibility to install
# |    any versions of VGC side-by-side, including different minor versions.
# |    However, it doesn't include any system to easily update to newer
# |    versions. By default, it installs VGC 2020 to:
# |        C:/Program Files/VGC 2020.0/
# |
# |- VGC 2020.zip:
#      This is a portable installation which you can unzip and run from
#      anywhere. However, it doesn't include any system to easily update
#      to newer versions, and it doesn't create convenient shortcuts in your
#      Start Menu or Desktop (you'll have to do this manually if desired).
#      You may need to manually run the included vc_redist.x64.exe installer
#      if your system doesn't already have the appropriate Visual Studio
#      redistributables installed.
#
# MSI family
# ----------
#
# We call "MSI family" a set of installers which can be upgraded from
# one another or share MSI components. Two installers belongs to the same
# MSI family if and only if all the following are true:
# - they are from the same suite
# - they have the same version type, and:
#   - if versionType = "Branch", they are from the same branch
#   - if versionType = "Local", they are from the same build
# - they have the same upgrade policy, and:
#   - if upgradePolicy = "minor", they have the same major version
#   - if upgradePolicy = "none", they are built from the same commit
# - they are targeting the same architecture (x86 or x64)
#
# We conveniently use the name of the default install folder as an identifier
# for the family.
#
# For example, all installers of "Stable" versions of VGC with the "all"
# upgrade policy are part of the same MSI family called "VGC". They
# all get installed by default to "C:/Program Files/VGC/". Below are
# examples of a few installers belonging to this MSI family:
#
# - "VGC 2020 Installer.exe"
# - "VGC 2021 Installer.exe"
# - "VGC Illustration 2020 Installer.exe"
#
# Examples of installers belonging to the "VGC Beta" MSI family, i.e.,
# versionType = "Beta" and upgradePolicy = "all":
#
# - "VGC 2020 Beta 2019-06-28.3 Installer.exe"
# - "VGC 2020 Beta 2019-06-29 Installer.exe"
# - "VGC 2021 Beta 2020-02-01 Installer.exe"
# - "VGC Illustration 2020 Beta 2020-06-29 Installer.exe"
#
# Examples of installers belonging to the "VGC Beta 2020" MSI family, i.e.,
# versionType = "Beta", upgradePolicy = "minor", and majorVersion = 2020:
#
# - "VGC 2020 Beta 2019-06-28.3 Installer (minor updates only).exe"
# - "VGC 2020 Beta 2019-06-29 Installer (minor updates only).exe"
# - "VGC Illustration 2020 Beta 2020-06-29 Installer (minor updates only).exe"
#
# Examples of installers belonging to the "VGC Alpha 2019-06-28.3" MSI
# family, i.e., versionType = "Alpha", upgradePolicy = "none", and built
# from the fourth commit of 2019-06-28:
#
# - "VGC Alpha 2019-06-28.3 Installer (no updates).exe"
# - "VGC Illustration Alpha 2019-06-28.3 Installer (no updates).exe"
#
# Suite installers vs app installers
# ----------------------------------
#
# Within each MSI family, there are two "installer types":
# - suite installers: they install all VGC apps
# - app installers: they only install a specific VGC app
#
# Suite installers and app installers do not share the same MSI upgradeCode, but
# they do share components and are installed in the same folder.
#
# All the suite installers of a given MSI family share the same upgradeCode.
#
# All the app installers of a given MSI family share the same upgradeCode if
# and only if the are installers for the same app. For example, "VGC Illustration
# 2020.exe" and "VGC Animation 2020.exe" have a different upgradeCode, but "VGC
# Illustration 2020.exe" and "VGC Illustration 2021.exe" have the same
# upgradeCode.
#
# MSI Version
# -----------
#
# Within each MSI family, we use a version numbering scheme designed to work
# with the following constraints:
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
# - Stable: releaseYear.minor.0
#     Example 1:
#       commit: first stable version on the 2020 release branch
#       msiVersion: 20.0.0
#       msiFamily:
#         if upgradePolicy = "all":   VGC
#         if upgradePolicy = "minor": VGC 2020
#         if upgradePolicy = "none":  VGC 2020.0
#     Example 2:
#       commit: second stable version on the 2020 release branch
#       msiVersion: 20.1.0
#       msiFamily:
#         if upgradePolicy = "all":   VGC
#         if upgradePolicy = "minor": VGC 2020
#         if upgradePolicy = "none":  VGC 2020.1
#
# - Beta: commitYear.commitMonth.(commitDay * 1000 + numCommitForThisDay)
#     Example 1:
#       commit: first commit of 2019-06-21 in the 2020 release branch
#       msiVersion: 19.6.21000
#       msiFamily:
#         if upgradePolicy = "all":   VGC Beta
#         if upgradePolicy = "minor": VGC 2020 Beta
#         if upgradePolicy = "none":  VGC 2020 Beta 2019-06-21
#     Example 2:
#       commit: second commit of 2019-06-21 in the 2020 release branch
#       msiVersion: 19.6.21001
#       msiFamily:
#         if upgradePolicy = "all":   VGC Beta
#         if upgradePolicy = "minor": VGC 2020 Beta
#         if upgradePolicy = "none":  VGC 2020 Beta 2019-06-21.1
#
# - Alpha/Branch: commitYear.commitMonth.(commitDay * 1000 + numCommitForThisDay)
#     Example 1:
#       commit: first commit of 2019-06-21 in the master branch
#       msiVersion: 19.6.21000
#       msiFamily:
#         if upgradePolicy = "all": VGC Alpha
#         if upgradePolicy = "none": VGC Alpha 2019-06-21
#     Example 2:
#       commit: second commit of 2019-06-21 in the gh62 branch
#       msiVersion: 19.6.21001
#       msiFamily:
#         if upgradePolicy = "all":  VGC Branch gh62
#         if upgradePolicy = "none": VGC Branch gh62 2019-06-21.1
#
# - Local: 1.0.0
#     Example:
#       msiVersion: 1.0.0
#       msiFamily:
#         if upgradePolicy = "none": VGC Local 2019-07-06 s9wj3g
#
# Additional version types
# ------------------------
#
# In the future, we may also want to consider implementing the following version
# types, although most are probably noth worth the added complexity:
#
# - RC (Release Candidate):
#     Manually selected version on a release branch, between beta and stable.
#     Version scheme: releaseYear.minor.revision
#     Example 1:
#       commit: first RC version before the 2020.0 release
#       msiVersion: 20.0.0
#       msiFamily:
#         if upgradePolicy = "all":   VGC RC
#         if upgradePolicy = "minor": VGC 2020 RC
#         if upgradePolicy = "none":  VGC 2020.0 RC
#     Example 1:
#       commit: second RC version before the 2020.0 release
#       msiVersion: 20.0.1
#       msiFamily:
#         if upgradePolicy = "all":   VGC RC
#         if upgradePolicy = "minor": VGC 2020 RC
#         if upgradePolicy = "none":  VGC 2020.0 RC 2
#     Example 2:
#       commit: first RC version before the 2020.1 release
#       msiVersion: 20.1.0
#       msiFamily:
#         if upgradePolicy = "all":   VGC RC
#         if upgradePolicy = "minor": VGC 2020 RC
#         if upgradePolicy = "none":  VGC 2020.1 RC
#
# - Nightly:
#     Like Alpha but only the last commit of the day. This may be useful because
#     often in the same day, there is a broken commit immediately followed by a
#     fix. So Nightly versions may be slightly more reliable than Alpha versions.
#     Version scheme: commitYear.commitMonth.commitDay
#     Example:
#       commit: last commit of 2019-06-21 in the master branch
#       msiVersion: 19.6.21
#       msiFamily:
#         if upgradePolicy = "all": VGC Nightly
#         if upgradePolicy = "none": VGC Nightly 2019-06-21
#
# - Debug/MinSizeRel/RelWithDebInfo:
#     Same as the others but using a different config.
#     Example:
#       commit: second commit of 2019-06-21 in the gh62 branch
#       config: RelWithDebInfo
#       msiVersion: 19.6.21001
#       msiFamily:
#         if upgradePolicy = "all":  VGC Branch gh62 RelWithDebInfo
#         if upgradePolicy = "none": VGC Branch gh62 2019-06-21.1 RelWithDebInfo
#     Note:
#       It might be simpler to simply choose a different suite name for handling
#       this case, such as "VGCd" for debug builds. This method could be applied
#       for other variations such as choosing different compile options, or
#       using different versions of third-party libraries.
#
# - Uncommited
#     Build based on an uncommited change from a given commit. Similar to
#     "Local" but adds more info since we know which commit this is a change
#     from. We could even include the diff in the About dialog.
#     Version scheme: 1.0.0
#     Example:
#       source: uncommited change from second commit of 2019-06-21 in the gh62
#               branch, with md5 hash of diff = abcdef123456
#       msiVersion: 1.0.0
#       msiFamily:
#         if upgradePolicy = "none": VGC Uncommited gh62 2019-06-21.1.abcdef123456
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
# are part of the MSI family or even the same build.
#
# Component GUID
# --------------
#
# We change component GUIDs the same way that we change product codes, except
# that we use the same component GUID across several .msi files for shared
# components on the same build. For example, the two following .msi files:
#
# - vgc-illustration-2020.msi
# - vgc-animation-2020.msi
#
# have different product codes. However, they are from the same build with
# exactly the same version type and config. Therefore, they both have the exact
# same component vgccore.dll, with the same GUID, which is installed at the same
# location:
#
#   C:/Program Files/VGC/2020/bin/vgccore.dll
#
# The equivalent for this type of sharing on macOS would be to use frameworks:
#
# https://www.bignerdranch.com/blog/it-looks-like-you-are-trying-to-use-a-framework
#
# but it is unclear whether all of this is worth it on macOS, where users are
# used to simply drag and drop the bundle in the Application folder. TODO: Look
# at what Open Office does, which is a similar use case as ours.
#

from pathlib import Path
from xml.dom.minidom import getDOMImplementation
from datetime import datetime, timezone
import hashlib
import os
import re
import subprocess
import uuid
import string
import random

# Returns a random alphanumeric string. The string may contain uppercase and
# lowercase letters of the latin alphabet, as well as digits (i.e., it uses
# a 62-char alphabet).
#
def randomAlphanumString(size):
    urandom = random.SystemRandom()
    alphabet = string.ascii_lowercase + string.ascii_uppercase + string.digits
    return ''.join(urandom.choice(alphabet) for _ in range(size))

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
    # Example:
    #     suiteName     = "VGC"
    #     appName       = "Illustration"
    #     architecture  = "x86"
    #     versionType   = "Beta"
    #     upgradePolicy = "minor"
    #     commitBranch  = "2020"
    #     commitDate    = "2019-07-08"
    #     commitNumber  = 1
    #
    def __init__(
            self, wixDir,
            manufacturer, suiteName, appName, architecture,
            versionType, upgradePolicy,
            commitBranch, commitDate, commitNumber):

        self.wixDir = wixDir
        self.manufacturer = manufacturer
        self.suiteName = suiteName
        self.appName = appName
        self.architecture = architecture
        self.versionType = versionType
        self.upgradePolicy = upgradePolicy
        self.commitBranch = commitBranch
        self.commitDate = commitDate
        self.commitNumber = commitNumber

        # Set appOrSuiteName. Examples:
        # - for suite installers: "VGC"
        # - for app installers: "VGC Illustration", "VGC Animation", etc.
        #
        self.appOrSuiteName = self.suiteName
        if self.appName:
            self.appOrSuiteName += " " + self.appName

        # Check versionType
        allowedVersionTypes = ["Stable", "Beta", "Alpha", "Branch", "Local"]
        if not self.versionType in allowedVersionTypes:
            raise ValueError(
                'Unknown versionType: "' + self.versionType + '".' +
                ' Allowed values are: "' + '", "'.join(allowedVersionTypes) + '".')

        # Check upgradePolicy
        allowedUpgradePolicies = {
            "Stable": ["all", "minor", "none"],
            "Beta":   ["all", "minor", "none"],
            "Alpha":  ["all", "none"],
            "Branch": ["all", "none"],
            "Local":  ["none"],
        }
        allowedUpgradePolicies_ = allowedUpgradePolicies[self.versionType]
        if not self.upgradePolicy in allowedUpgradePolicies_:
            raise ValueError(
                'Unknown upgradePolicy: "' + self.upgradePolicy + '".' +
                ' Allowed values when versionType = "' + self.versionType + '"'
                ' are: ' + '", "'.join(allowedUpgradePolicies_) + '".')

        # Check architecture
        allowedArchitectures = ["x86", "x64"]
        if not self.architecture in allowedArchitectures:
            raise ValueError(
                'Unknown architecture: "' + self.versionType + '".' +
                ' Allowed values are: "' + '", "'.join(allowedArchitectures) + '".')

        # Set release year
        # Example: "2020"
        if self.versionType in ["Stable", "Beta"]:
            releaseYear = self.commitBranch[0:4]

        # Set commitDateAndNumber
        # Example: "2019-07-08.1"
        if self.versionType in ["Beta", "Alpha", "Branch"]:
            commitDateAndNumber = self.commitDate
            if self.commitNumber > 0:
                commitDateAndNumber += ".{}".format(self.commitNumber)

        # Set msiVersion
        # Example: "19.7.08001"
        if self.versionType == "Stable":
            major = int(self.commitBranch[2:4])
            minor = int(self.commitBranch[5:])
            build = 0
        elif self.versionType in ["Beta", "Alpha", "Branch"]:
            major = int(self.commitDate[2:4])
            minor = int(self.commitDate[5:7])
            build = int(self.commitDate[8:10]) * 1000 + self.commitNumber
        elif self.versionType == "Local":
            major = 1
            minor = 0
            build = 0
        self.msiVersion = "{}.{}.{}".format(major, minor, build)

        # Set msiFamilySuffix and fullVersion (architecture added just after)
        # Example:
        #   msiFamilySuffix = " 2020 Beta 32-bit"
        #   fullVersion     = "2020 Beta 2019-07-08.1 32-bit"

        vt = self.versionType
        up = self.upgradePolicy
        datenowutc = datetime.now(timezone.utc).date().isoformat()
        randomid = randomAlphanumString(6)

        if vt == "Stable":
            if   up == "all":   p = ""
            elif up == "minor": p = " {}"
            elif up == "none":  p = " {}.{}"
            self.msiFamilySuffix = p.format(releaseYear, minor)
            self.fullVersion = releaseYear
            if minor > 0:
                self.fullVersion += "." + str(minor)

        elif vt == "Beta":
            if   up == "all":   p = " Beta"
            elif up == "minor": p = " {} Beta"
            elif up == "none":  p = " {} Beta {}"
            q                     =  "{} Beta {}"
            self.msiFamilySuffix = p.format(releaseYear, commitDateAndNumber)
            self.fullVersion     = q.format(releaseYear, commitDateAndNumber)

        elif vt == "Alpha":
            if   up == "all":   p = " Alpha"
            elif up == "none":  p = " Alpha {}"
            q                     =  "Alpha {}"
            self.msiFamilySuffix = p.format(commitDateAndNumber)
            self.fullVersion     = q.format(commitDateAndNumber)

        elif vt == "Branch":
            if   up == "all":   p = " Branch {}"
            elif up == "none":  p = " Branch {} {}"
            q                     =  "Branch {} {}"
            self.msiFamilySuffix = p.format(self.commitBranch, commitDateAndNumber)
            self.fullVersion     = q.format(self.commitBranch, commitDateAndNumber)

        elif vt == "Local":
            if   up == "none":  p = " Local {}.{}"
            q                     =  "Local {}.{}"
            self.msiFamilySuffix = p.format(datenowutc, randomid)
            self.fullVersion     = q.format(datenowutc, randomid)

        # Set architecture
        if self.architecture == "x64":
            self.win64yesno = "yes"
            self.programFilesFolder = "ProgramFiles64Folder"
        elif self.architecture == "x86":
            self.win64yesno = "no"
            self.programFilesFolder = "ProgramFilesFolder"
            self.msiFamilySuffix += " 32-bit"
            self.fullVersion     += " 32-bit"

        # Set msiFamily
        # Example: "VGC 2020 Beta 32-bit"
        self.msiFamily = self.suiteName + self.msiFamilySuffix

        # Set installerName and msiName
        #
        if up == "all":   up_ = ""
        if up == "minor": up_ = " (minor updates only)"
        if up == "none":  up_ = " (no updates)"
        self.msiName       = self.appOrSuiteName + " " + self.fullVersion + up_
        self.installerName = self.appOrSuiteName + " " + self.fullVersion + " Installer" + up_

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
            ("Name", self.appOrSuiteName + self.msiFamilySuffix),
            ("Id", self.dynamicGuid("Product/ProductCode/" + self.appOrSuiteName)),
            ("UpgradeCode", self.staticGuid("Product/UpgradeCode/" + self.appOrSuiteName)),
            ("Language", "1033"),
            ("Codepage", "1252"),
            ("Version", self.msiVersion),
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

        # Add basic directory structure.
        self.targetDirectory = self.product.createDirectory("SourceDir", "TARGETDIR")
        self.programFilesDirectory = self.targetDirectory.createDirectory("PFiles", self.programFilesFolder)
        self.installDirectory = self.programFilesDirectory.createDirectory(self.msiFamily, "INSTALLDIR")
        self.startMenuDirectory = self.targetDirectory.createDirectory("Programs", "ProgramMenuFolder")
        self.desktopDirectory = self.targetDirectory.createDirectory("Desktop", "DesktopFolder")

    # Generates a deterministic GUID based on the msiFamily, the msiVersion, and
    # the given string identifier "sid". This GUID changes from version to
    # version but doesn't depend on the appName, which makes it suitable for
    # shared components. You must manually include the appName in the sid if you
    # wish this GUID to depend on the appName.
    #
    def dynamicGuid(self, sid):
        u = uuid.uuid5(uuid.NAMESPACE_URL,
                    "http://dynamicguid.wix.vgc.io" +
                    "/" + self.msiFamily +
                    "/" + self.msiVersion +
                    "/" + sid)
        return str(u).upper()

    # Generates a deterministic GUID based on the msiFamily and the given string
    # identifier "sid". This GUID doesn't change from version to version (as
    # long at it belongs to the same msiFamily) neither to the appName. You must
    # manually include the appName in the sid if you wish this GUID to depend on
    # the appName, e.g., for the upgradeCode.
    #
    def staticGuid(self, sid):
        u = uuid.uuid5(uuid.NAMESPACE_URL,
                    "http://staticguid.wix.vgc.io" +
                    "/" + self.msiFamily +
                    "/" + sid)
        return str(u).upper()

    # Generates the setup file
    #
    def makeSetup(self, deployDir, icon, logo):
        msi_wxs             = deployDir / (self.msiName + ".wxs")
        msi_wixobj          = deployDir / (self.msiName + ".wixobj")
        msi                 = deployDir / (self.msiName + ".msi")
        bundle_template_wxs = deployDir / "bundle.wxs"
        bundle_wxs          = deployDir / (self.installerName + ".wxs")
        bundle_wixobj       = deployDir / (self.installerName + ".wixobj")
        installer_exe       = deployDir / (self.installerName + ".exe")
        vcredist            = deployDir / "vc_redist.x64.exe"
        vcredistVersion     = (deployDir / "vc_redist.x64.exe.version").read_text().strip()
        binDir = self.wixDir / "bin"

        # Generate the .wxs file of the MsiPackage
        msi_wxs.write_bytes(self.domDocument.toprettyxml(encoding='windows-1252'))

        # Generate the .wxsobj file of the MsiPackage
        # -sw1091: Silence warning CNDL1091: The Package/@Id attribute has been set.
        subprocess.run([
            str(binDir / "candle.exe"), str(msi_wxs),
            "-sw1091",
            "-arch", self.architecture,
            "-out", str(msi_wixobj)])

        # Generate the .msi file of the MsiPackage
        # ICE07/ICE60: Remove warnings about font files. See:
        # https://stackoverflow.com/questions/13052258/installing-a-font-with-wix-not-to-the-local-font-folder
        subprocess.run([
            str(binDir / "light.exe"), str(msi_wixobj),
            "-sice:ICE07", "-sice:ICE60",
            "-out", str(msi)])

        # Generate the .wxs file of the Bundle
        bundle_text = bundle_template_wxs.read_text()
        bundle_wxs.write_text(replace_all(bundle_text, {
            "$(var.name)":            self.msiFamily,
            "$(var.manufacturer)":    self.manufacturer,
            "$(var.upgradeCode)":     self.staticGuid("Bundle/UpgradeCode/" + self.appOrSuiteName),
            "$(var.version)":         self.msiVersion,
            "$(var.icon)":            str(icon),
            "$(var.logo)":            str(logo),
            "$(var.msi)":             str(msi),
            "$(var.vcredist)":        str(vcredist),
            "$(var.vcredistVersion)": vcredistVersion}))

        # Generate the .wxsobj file of the Bundle
        subprocess.run([
            str(binDir / "candle.exe"), str(bundle_wxs),
            "-ext", "WixBalExtension",
            "-ext", "WixUtilExtension",
            "-arch", self.architecture,
            "-out", str(bundle_wixobj)])

        # Generate the .exe file of the Bundle
        subprocess.run([
            str(binDir / "light.exe"), str(bundle_wixobj),
            "-ext", "WixBalExtension",
            "-ext", "WixUtilExtension",
            "-out", str(installer_exe)])

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
        iconId = encodeId("Icon" + basename) + suffix
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
            dirId = encodeId(self.dirId + '/' + name)
        child = self.createChild("Directory", [("Id", dirId), ("Name", name)])
        child.dirId = dirId
        child.files = {}
        return child

    # Creates a shortcut to this file using the given icon
    #
    def createShortcut(self, directory, name, icon, workingDirectory = None):
        if workingDirectory is None:
            workingDirectory = self.wix.installDirectory
        return self.createChild("Shortcut", [
            ("Id", encodeId("Shortcut" + directory.dirId + "/" + name)),
            ("Directory", directory.dirId),
            ("Name", name),
            ("WorkingDirectory", workingDirectory.dirId),
            ("Icon", icon.iconId),
            ("IconIndex", "0"),
            ("Advertise", "yes")])

    # Creates a submenu of the startup menu.
    # It has to be associated with a feature so that it is deleted when the
    # feature is uninstalled.
    #
    def createSubMenu(self, name, feature):
        subMenuDirectory = self.createDirectory(name)
        componentId = encodeId("SubMenuComponent" + subMenuDirectory.dirId)
        subMenuComponent = subMenuDirectory.createChild("Component", [
            ("Id", componentId),
            ("Guid", self.wix.dynamicGuid(componentId))])
        subMenuComponent.createChild("RemoveFolder", [
            ("Id", encodeId("RemoveFolder" + componentId)),
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
        componentId = encodeId("FileComponent" + self.dirId + "/" + name)
        fileComponent = self.createChild("Component", [
            ("Id", componentId),
            ("Guid", self.wix.dynamicGuid(componentId)),
            ("Win64", self.wix.win64yesno)])
        file = fileComponent.createChild("File", [
            ("Id", encodeId("File" + componentId)),
            ("Name", name),
            ("DiskId", "1"),
            ("Source", str(srcFile)),
            ("KeyPath", "yes")])
        self.files[name] = file
        feature.createChild("ComponentRef", [("Id", componentId)])
        return file

    # Returns a previously created file
    #
    def getFile(self, name):
        return self.files[name]

    # Recursively add the given directory and all its files to this directory
    # for the given feature
    #
    def addDirectory(self, srcDir, feature):
        destDir = self.createDirectory(srcDir.name)
        for child in srcDir.iterdir():
            if child.is_file():
                destDir.createFile(str(child), child.name, feature)
            elif child.is_dir():
                destDir.addDirectory(child, feature)
        return destDir

# Generates an MSI file for the given suite
#
def makeSuiteInstaller(
        buildDir, configDir, deployDir, wixDir,
        manufacturer, suiteName, appNames, architecture,
        versionType, upgradePolicy,
        commitBranch, commitDate, commitNumber):

    # General configuration
    appName = None
    wix = Wix(
        wixDir,
        manufacturer, suiteName, appName, architecture,
        versionType, upgradePolicy,
        commitBranch, commitDate, commitNumber)
    appFeature = wix.createFeature("Complete")

    # Add 'bin', 'python', and 'resources' directories
    wixBinDir = wix.installDirectory.addDirectory(configDir / "bin", appFeature)
    wix.installDirectory.addDirectory(configDir / "python", appFeature)
    wix.installDirectory.addDirectory(configDir / "resources", appFeature)

    # Create Desktop and Start Menu shortcuts
    for appName in appNames:
        appExecutableBasename = "vgc" + appName.lower()
        appExecutableName = appExecutableBasename + ".exe"
        appExecutable = wixBinDir.getFile(appExecutableName)
        appIconPath = buildDir / (appExecutableBasename + ".ico")
        appShortcutName = suiteName + " " + appName + wix.msiFamilySuffix
        appIcon = wix.createIcon(appIconPath)
        appExecutable.createShortcut(wix.startMenuDirectory, appShortcutName, appIcon)
        appExecutable.createShortcut(wix.desktopDirectory, appShortcutName, appIcon)

    # Generate the Setup file
    setupIcon = appIconPath
    setupLogo = setupIcon.with_suffix(".png")
    wix.makeSetup(deployDir, setupIcon, setupLogo)

# Converts the given "datetime with UTC offset" to the date it was in the
# UTC timezone at that exact moment.
#
# Example:
# format = '%Y-%m-%d %H:%M:%S %z'
# utcdate('2019-06-23 18:00:45 -0800', format)
#
# Output: '2019-06-24'
#
def utcdate(dateString, format):
    d = datetime.strptime(dateString, format)
    return d.astimezone(timezone.utc).date().isoformat()

# Returns whether the string is the name of a release branch,
# that is, a four-digit integer, like 2020.
#
def isReleaseBranch(branchName):
    if re.fullmatch(r"[0-9]{4}", branchName):
        return True
    else:
        return False

# Generates an MSI file from the build
#
def run(buildDir, config, wixDir):

    # Get and create useful directories
    buildDir = Path(buildDir)
    configDir = buildDir / config
    deployDir = configDir / "deploy"
    deployDir.mkdir(parents=True, exist_ok=True)
    wixDir = Path(wixDir)

    # General configuration
    manufacturer = "VGC Software"
    suiteName = "VGC"
    appNames = [ "Illustration" ]
    architecture = "x64"

    # Get git info
    try:
        gitInfo = (buildDir / "git_info.txt").read_text()
        commitBranch = re.search(r"^branch: (.*)$", gitInfo, flags = re.MULTILINE).group(1)
        format = '%Y-%m-%d %H:%M:%S %z'
        dates = re.search(r"^latest-commit-dates: (.*)", gitInfo, flags = re.MULTILINE | re.DOTALL).group(1).splitlines()
        dates = [utcdate(date, format) for date in dates if date]
        commitDate = dates[0]
        commitNumber = dates.count(commitDate) - 1
    except:
        commitBranch = None
        commitDate = None
        commitNumber = None

    # Deduct version type from git info
    # TODO: deduct "Stable" version type based on either branch or tag name.
    if commitBranch:
        if commitBranch == "master":
            versionType = "Alpha"
        elif isReleaseBranch(commitBranch):
            versionType = "Beta"
        else:
            versionType = "Branch"
    else:
        versionType = "Local"

    # Set upgrade policy. For now, it is hard-coded to be the the most
    # upradable policy.
    if versionType == "Local":
        upgradePolicy = "none"
    else:
        upgradePolicy = "all"

    # Create an installer for the suite, containing all apps.
    makeSuiteInstaller(
        buildDir, configDir, deployDir, wixDir,
        manufacturer, suiteName, appNames, architecture,
        versionType, upgradePolicy,
        commitBranch, commitDate, commitNumber)

    # TODO: also create a ZIP for portable installation (like Blender
    # does), with updates disabled .
