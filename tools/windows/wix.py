#!/usr/bin/env python3
#
# WiX resources
# -------------
#
# - https://stackoverflow.com/questions/471424/wix-tricks-and-tips
#
# Version naming scheme and install location
# -----------------------------------------
#
# Our release cycle uses the following "version type": Dev, Alpha, Beta, Release
# Candidate (RC), and Stable. Each of these version types have independent
# upgrade codes and are seen as completely different applications, which means
# that you cannot upgrade from Beta to Stable, for example.
#
# Each of these version types also have independent version numbering scheme,
# which are designed to work with the following constraints:
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
# Note: For now, only the Dev version type is implemented.
#
# Here are the full details for each version type:
#
# - Dev:
#     CI builds from the master branch.
#     Most unstable, untested.
#     Version scheme: commitYear.commitMonth.(commitDay * 1000 + numCommitForThisDay)
#     Example 1:
#       commit: first commit of 2019-06-21 in the master branch
#       version: 19.6.21000
#       install: Program Files/VGC/Dev 2019-06-21/bin/vgcillustration.exe
#     Example 2:
#       commit: second commit of 2019-06-21 in the master branch
#       version: 19.6.21001
#       install: Program Files/VGC/Dev 2019-06-21.1/bin/vgcillustration.exe
#
# - Alpha:
#     CI builds from a release branch, that is, after a feature freeze.
#     Barely tested, but only bug fixes allowed on a release branch, so more stable than Dev builds.
#     Version scheme: commitYear.commitMonth.(commitDay * 1000 + numCommitForThisDay)
#     Example:
#       commit: second commit of 2019-06-21 in the 2020 release branch
#       version: 19.6.21001
#       install: Program Files/VGC/2020 Alpha 2019-06-21.1/bin/vgcillustration.exe
#
# - Beta:
#     Manually selected version on a release branch. ~3-5 before each stable version.
#     More tested, should be quite stable.
#     Version scheme: release.minor.revision
#     Example 1:
#       commit: second selected beta in the 2020 release branch, before any stable version
#       version: 20.0.2
#       install: Program Files/VGC/2020 Beta 2/bin/vgcillustration.exe
#     Example 2:
#       commit: first selected beta in the 2020 release branch, after the first stable version
#       version: 20.1.1
#       install: Program Files/VGC/2020.1 Beta 1/bin/vgcillustration.exe
#
# - RC:
#     Manually selected version on a release branch, between beta and stable. ~1 before each stable version.
#     Most tested. We hope that no modification is required before stable version.
#     Version scheme: release.minor.revision
#     Example 1:
#       commit: second selected RC in the 2020 release branch, before any stable version
#       version: 20.0.2
#       install: Program Files/VGC/2020 RC 2/bin/vgcillustration.exe
#     Example 2:
#       commit: first selected RC in the 2020 release branch, after the first stable version
#       version: 20.1.1
#       install: Program Files/VGC/2020.1 RC 1/bin/vgcillustration.exe
#
# - Stable
#     Manually selected version on a release branch. ~1-2 per release branch
#     This is what you get when clicking the big "download VGC" button
#     Version scheme: release.minor.0
#     Example 1:
#       commit: first stable version on the 2020 release branch
#       version: 20.0.0
#       install: Program Files/VGC/2020/bin/vgcillustration.exe
#     Example 2:
#       commit: first stable version on the 2020 release branch
#       version: 20.1.0
#       install: Program Files/VGC/2020.1/bin/vgcillustration.exe
#
# We may also want to consider the following:
#
# - Nightly:
#     Like Dev but only the last commit of the day. This is useful because often
#     in the same day, there is a broken commit immediately followed by a fix.
#     So Nightly versions may be slightly more reliable than Dev versions.
#     Version scheme: commitYear.commitMonth.commitDay
#     Example:
#       commit: last commit of 2019-06-21 in the master branch
#       version: 19.6.21
#       install: Program Files/VGC/Nightly 2019-06-21/bin/vgcillustration.exe
#
# - Branch XYZ:
#     A commit from any branch which is neither the master branch nor a release
#     branch. It may be a feature branch or a fix branch, and we may want users
#     to test the fix before merging with master, for example.
#     Version scheme: commitYear.commitMonth.(commitDay * 1000 + numCommitForThisDay)
#     Example:
#       commit: second commit of 2019-06-21 in the gh62 branch
#       version: 19.6.21001
#       install: Program Files/VGC/Branch-gh62 2019-06-21.1/bin/vgcillustration.exe
#
# - Debug/MinSizeRel/RelWithDebInfo:
#     Same as the others but using a different config.
#     Example:
#       commit: second commit of 2019-06-21 in the gh62 branch
#       version: 19.6.21001
#       install: Program Files/VGC/Branch-gh62 RelWithDebInfo 2019-06-21.1/bin/vgcillustration.exe
#
# - Uncommited
#     Built from a change which hasn't been commited yet.
#     It would be nice to include the diff within the UI in the About page.
#     It's not clear whether the ability to create installers from those is useful.
#     It might be simpler to distribute a zip file with the standalone binaries in it.
#     Example:
#       source: change from second commit of 2019-06-21 in the gh62 branch, with md5 hash of diff = abcdef123456
#       version:19.6.21001
#       install: Program Files/VGC/Branch-gh62 2019-06-21.1.abcdef123456/bin/vgcillustration.exe
#       Version scheme: commitYear.commitMonth.(commitDay * 1000 + numCommitForThisDay)
#
# Product code
# ------------
#
# Changing any of the following changes the product code:
# - Commit from which the build is based
# - Release type
# - Architecture (x86 vs x64)
# - Version of Qt, Python, etc.
#
# See:
#
# https://docs.microsoft.com/en-us/windows/desktop/msi/changing-the-product-code
#
#   The product code must be changed if any of the following are true for the
#   update:
#
#   - Coexisting installations of both original and updated products on the same
#     system must be possible.
#   - The name of the .msi file has been changed.
#   - The component code of an existing component has changed.
#   - A component is removed from an existing feature.
#   - An existing feature has been made into a child of an existing feature.
#   - An existing child feature has been removed from its parent feature.
#
# Note: VGC Illustration, VGC Animation, and VGC Suite (= both) have different
# product codes, but they share components.
#
# Component GUID
# --------------
#
# We change component GUIDs the same way that we change product codes.
#
# However, note that we use the same component GUID across several .msi files
# for shared components. For example, the two following .msi files:
#
# - vgc-illustration-2020-x64.msi
# - vgc-animation-2020-x64.msi
#
# have different product codes. However, they are from the same build with
# exactly the same version type and config. Therefore, they both have the exact
# same component vgccore.dll, with the same GUID, which is installed at the same
# location:
#
#   Program Files/VGC/2020/bin/vgccore.dll
#
# The equivalent for this type of sharing on macOS would be to use frameworks:
#
# https://www.bignerdranch.com/blog/it-looks-like-you-are-trying-to-use-a-framework
#
# but it is unclear whether all of this is worth it on macOS, where users are
# used to simply drag and drop the bundle in the Application folder. TODO: Look
# at what Open Office does, which is a similar use case as ours.
#

from xml.dom.minidom import getDOMImplementation
import uuid
import subprocess
import hashlib
import os
from pathlib import Path
import re

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
    # - wix.manufacturerDirectory: "Program Files\[Manufacturer]"
    # - wix.installDirectory:      "Program Files\[Manufacturer]\[ProductName]"
    # - wix.startMenuDirectory:    Windows' Start Menu
    # - wix.desktopDirectory:      Windows' desktop
    #
    def __init__(
            self, wixDir,
            manufacturer, suiteName, appName, architecture,
            versionType, commitBranch, commitDate, commitNumber):

        self.wixDir = wixDir
        self.manufacturer = manufacturer
        self.suiteName = suiteName
        self.appName = appName
        self.architecture = architecture
        self.versionType = versionType
        self.commitDate = commitDate
        self.commitNumber = commitNumber

        # Versioning.
        #
        # The short version is a standard MSI-compliant version in the form
        # "major.minor.build". It is used for formal versionning of products
        # sharing the same upgrade code. Since builds with different version
        # types do not share the same upgrade code, the version type is not part
        # of the short version.
        #
        # The full version is a human-readable version including the version
        # type, such as "Dev 2019-06-21.1". Together with the suiteName and
        # architecture, it uniquely identifies this suite build. It is used
        # in product names and install directories, as well as for generating
        # static and dynamic GUIDs.
        #
        if self.versionType == "Dev":
            # Example:
            #   shortVersion = "19.6.21001"
            #   fullVersion = "Dev 2019-06-21.1"
            year = int(self.commitDate[2:4])
            month = int(self.commitDate[5:7])
            day = int(self.commitDate[8:10])
            build = 1000*day + self.commitNumber
            if self.commitNumber == 0:
                subDate = ""
            else:
                subDate = ".{}".format(self.commitNumber)
            self.shortVersion = "{}.{}.{}".format(year, month, build)
            self.fullVersion = "{} {}{}".format(self.versionType, self.commitDate, subDate)
        else:
            # TODO: implement version numbering scheme for version types other than Dev
            raise ValueError('Currently, "Dev" is the only supported versionType ("' + self.versionType + '" provided)')

        # Architecture
        if self.architecture == "x64":
            self.win64yesno = "yes"
            self.programFilesFolder = "ProgramFiles64Folder"
        else:
            self.win64yesno = "no"
            self.programFilesFolder = "ProgramFilesFolder"

        # Human-readable app name as should appear in shortcut and list of installed programs
        self.appFullName = "{} {} {}".format(self.suiteName, self.appName, self.fullVersion)
        if self.architecture == "x86":
            self.appFullName += " (32-bit)"

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
        # keep the same MSI filename during a "Minor Upgrade" or "Small Uprade".
        #
        self.product = self.root.createChild("Product", [
            ("Name", self.appFullName),
            ("Id", self.dynamicGuid("Product/ProductCode/" + self.appName)),
            ("UpgradeCode", self.staticGuid("Product/UpgradeCode/" + self.appName)),
            ("Language", "1033"),
            ("Codepage", "1252"),
            ("Version", self.shortVersion),
            ("Manufacturer", self.manufacturer)])

        # Add package
        #
        # Note: manual specification of "Platform" is discouraged in favour
        # of using the candle.exe -arch x64/x86 switch, but we use both anyway.
        #
        self.package = self.product.createChild("Package", [
            ("Id", self.dynamicGuid("Package/Id/" + self.appName)),
            ("Keywords", "Installer"),
            ("Description", "Installer of " + self.appFullName),
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

        # Add basic directory structure. As far as I understand, a lot of these
        # Name and Id attribute are "magic names" recognized by either WiX or
        # Windows Installer, and can't be changed.
        self.targetDirectory = self.product.createDirectory("SourceDir", "TARGETDIR")
        self.programFilesDirectory = self.targetDirectory.createDirectory("PFiles", self.programFilesFolder)
        self.manufacturerDirectory = self.programFilesDirectory.createDirectory(self.suiteName)
        self.installDirectory = self.manufacturerDirectory.createDirectory(self.fullVersion, "INSTALLDIR")
        self.startMenuDirectory = self.targetDirectory.createDirectory("Programs", "ProgramMenuFolder")
        self.desktopDirectory = self.targetDirectory.createDirectory("Desktop", "DesktopFolder")

    # Generates a deterministic GUID based on the architecture, suiteName,
    # fullVersion, and the given string identifier "sid". This GUID changes
    # from version to version but doesn't depend on the appName, which makes
    # it suitable for shared components. You must manually include the appName
    # in the sid if you wish this GUID to depend on the appName.
    #
    def dynamicGuid(self, sid):
        u = uuid.uuid5(uuid.NAMESPACE_URL,
                    "http://dynamicguid.wix.vgc.io" +
                    "/" + self.architecture +
                    "/" + self.suiteName +
                    "/" + self.fullVersion +
                    "/" + sid)
        return str(u).upper()

    # Generates a deterministic GUID based on the architecture, suiteName,
    # versionType, and the given string identifier "sid". This GUID doesn't
    # change from version to version (of the same versionType), and doesn't
    # depend on the appName.
    #
    def staticGuid(self, sid):
        u = uuid.uuid5(uuid.NAMESPACE_URL,
                    "http://dynamicguid.wix.vgc.io" +
                    "/" + self.architecture +
                    "/" + self.suiteName +
                    "/" + self.versionType +
                    "/" + sid)
        return str(u).upper()

    # Generates the setup file
    #
    def makeSetup(self, deployDir, icon, logo):
        basename = "{} {} {}".format(self.suiteName, self.appName, self.fullVersion)
        if self.architecture != "x64":
            basename += "-" + self.architecture
        msi_wxs             = deployDir / (basename + ".wxs")
        msi_wixobj          = deployDir / (basename + ".wixobj")
        msi                 = deployDir / (basename + ".msi")
        bundle_template_wxs = deployDir / "bundle.wxs"
        bundle_wxs          = deployDir / (basename + " Bundle.wxs")
        bundle_wixobj       = deployDir / (basename + " Bundle.wixobj")
        setup_exe           = deployDir / (basename + " Setup.exe")
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
        setup_text = bundle_template_wxs.read_text()
        bundle_wxs.write_text(replace_all(setup_text, {
            "$(var.name)":            self.appFullName,
            "$(var.manufacturer)":    self.manufacturer,
            "$(var.upgradeCode)":     self.staticGuid("Bundle/UpgradeCode/" + self.appName),
            "$(var.version)":         self.shortVersion,
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
            "-out", str(setup_exe)])

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

# Generates an MSI file for the given application
#
def makeAppSetup(
        buildDir, configDir, deployDir, wixDir,
        manufacturer, suiteName, appName, architecture,
        versionType, commitBranch, commitDate, commitNumber):

    # General configuration
    wix = Wix(
        wixDir,
        manufacturer, suiteName, appName, architecture,
        versionType, commitBranch, commitDate, commitNumber)
    appFeature = wix.createFeature(appName)

    # Add 'bin', 'python', and 'resources' directories
    wixBinDir = wix.installDirectory.addDirectory(configDir / "bin", appFeature)
    wix.installDirectory.addDirectory(configDir / "python", appFeature)
    wix.installDirectory.addDirectory(configDir / "resources", appFeature)

    # Create Desktop and Start Menu shortcuts
    appExecutableBasename = "vgc" + appName.lower()
    appExecutableName = appExecutableBasename + ".exe"
    appExecutable = wixBinDir.getFile(appExecutableName)
    appIconPath = buildDir / (appExecutableBasename + ".ico")
    appIcon = wix.createIcon(appIconPath)
    appExecutable.createShortcut(wix.startMenuDirectory, wix.appFullName, appIcon)
    appExecutable.createShortcut(wix.desktopDirectory, wix.appFullName, appIcon)

    # Generate the Setup file
    setupIcon = appIconPath
    setupLogo = setupIcon.with_suffix(".png")
    wix.makeSetup(deployDir, setupIcon, setupLogo)

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
    versionType = "Dev"         # TODO should be a CMake cache variable
    commitBranch = "master"     # TODO should be automatically retrieved from git log
    commitDate = "2019-05-27"   # TODO should be automatically retrieved from git log
    commitNumber = 0            # TODO should be automatically retrieved from git log

    # Create an .msi and a setup.exe for each app.
    # TODO: also create a "Suite" .msi and setup.exe installing all the apps.
    appNames = [ "Illustration" ]
    for appName in appNames:
        makeAppSetup(
            buildDir, configDir, deployDir, wixDir,
            manufacturer, suiteName, appName, architecture,
            versionType, commitBranch, commitDate, commitNumber)
