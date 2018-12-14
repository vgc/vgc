#!/bin/bash
#
# Usage:
#
#     ./linuxdeploy.sh <build-dir> <src-dir> <qmake-executable>
#
# While you can run this script manually, the intended usage is to call:
#
#     make linuxdeploy
#
# which calls this script with the appropriate arguments.
#

# Get input arguments
BUILD_DIR="$1"
SRC_DIR="$2"
QMAKE_EXECUTABLE="$3"

echo "Running vgc/tools/linuxdeploy.sh with:"
echo "  BUILD_DIR = $BUILD_DIR"
echo "  SRC_DIR = $SRC_DIR"
echo "  QMAKE_EXECUTABLE = $QMAKE_EXECUTABLE"

# Download linuxdeployqt, unless we already have it.
# In the future, we might want to allow clients to use their own version of
# linuxdeployqt, or ship it with vgc.
LINUXDEPLOYQT_NAME=linuxdeployqt-continuous-x86_64.AppImage
if [ ! -e $LINUXDEPLOYQT_NAME ]; then
    wget "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/$LINUXDEPLOYQT_NAME"
    chmod a+x $LINUXDEPLOYQT_NAME
fi
LINUXDEPLOYQT_EXECUTABLE="./$LINUXDEPLOYQT_NAME"

# We only have one app for now, so we hard-code its name.
# We'll improve that once we start VGC Animation.
SHORT_APP_NAME=illustration
APP_NAME=vgcillustration

# Path to AppDir. This is a temporary folder used as input for linuxdeployqt.
APP_DIR="$BUILD_DIR/$APP_NAME.appdir"

# Remove $APP_DIR if any
rm -r "$APP_DIR"

# Create usr folder
mkdir -p "$APP_DIR/usr/"

# Copy VGC executable
mkdir -p "$APP_DIR/usr/bin"
cp "$BUILD_DIR/bin/$APP_NAME" "$APP_DIR/usr/bin"

# Copy VGC libraries
cp -r "$BUILD_DIR/lib/" "$APP_DIR/usr/"

# Copy VGC python bindings
#
# Note: The Python library itself (e.g., libpython3.6m.so.1.0) is automatically
# copied by linuxdeployqt into "$APP_DIR/lib/". However, we probably also need
# to copy Python's site-packages in order to get access to python built-in packages.
#
# Example of how to their locations:
# >>> import sys
# >>> print(sys.path)
# ['', '/usr/lib/python36.zip', '/usr/lib/python3.6', '/usr/lib/python3.6/lib-dynload', '/home/boris/.local/lib/python3.6/site-packages', '/usr/local/lib/python3.6/dist-packages', '/usr/lib/python3/dist-packages']
#
cp -r "$BUILD_DIR/python/" "$APP_DIR/usr/"

# Copy VGC resources
cp -r "$BUILD_DIR/resources/" "$APP_DIR/usr/"

# Copy icons and desktop file
cp -r "$SRC_DIR/apps/$SHORT_APP_NAME/share" "$APP_DIR/usr/"

# Cleanup environment variabes. Not sure if all this is necessary.
#unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH

# Set version name. This is used by linuxdeployqt for naming the output AppImage file.
#
# We may improve this later, e.g.:
#     export VERSION=$(git rev-parse --short HEAD)
#
# However, maybe it's a little bit wasteful to upload to github every single version.
# The AppImage is around 30MB, so after 100 commits that's 3GB of data.
#
# For now, let's just not set version at all.

# Create AppImage
$LINUXDEPLOYQT_EXECUTABLE "$APP_DIR/usr/share/applications/$APP_NAME.desktop" -qmake="$QMAKE_EXECUTABLE" -appimage
