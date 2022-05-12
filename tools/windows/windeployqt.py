#!/usr/bin/env python3
#
# Runs windeployqt.exe whenever necessary to copy all required Qt dependencies
# of a given app.
#

from pathlib import Path
import argparse
import errno
import subprocess
import time


# Implements thread locking via the creation of a temporary file.
#
# Inspired by https://github.com/dmfrey/FileLock
#
class FileLock(object):
    def __init__(self, file, timeout=30, delay=1):
        self.isLocked = False
        self.file = file
        self.timeout = timeout
        self.delay = delay

    def acquire(self):
        startTime = time.time()
        while True:
            try:
                self.fd = self.file.open('x')
                self.isLocked = True
                break
            except OSError as e:
                if e.errno != errno.EEXIST:
                    raise
                if (time.time() - startTime) >= self.timeout:
                    raise RuntimeError(f"FileLock: Could not acquire {self.file}.")
                time.sleep(self.delay)

    def release(self):
        if self.isLocked:
            self.fd.close()
            self.file.unlink()
            self.isLocked = False

    def __enter__(self):
        if not self.isLocked:
            self.acquire()
        return self

    def __exit__(self, type, value, traceback):
        if self.isLocked:
            self.release()

    def __del__(self):
        self.release()


# Returns whether we need to run windeployqt.exe.
#
# For now we simply run windeployqt.exe exactly once for each app. More
# precisely, we run it after the first build of the app, but we don't re-run
# it for subsequent builds.
#
# In the future, we may want to improve this behavior and re-run
# windeployqt.exe if any of the dependencies of the app changed, potentially
# including version changes or if the location of the dependency changed.
#
def needsToRun(configDir, appName):
    windeployqtAppFile = configDir / f'windeployqt_{appName}'
    try:
        with windeployqtAppFile.open('x'):
            # It's the first time we run windeployqt for this app.
            # For now we just create an empty file and return True.
            # In the future, we may add dependency info to the file.
            return True
    except FileExistsError:
        # It's not the first time we run windeployqt for this app. For now we
        # just return False. In the future we may want to also check whether
        # dependencies have changed, in which case we'd return True and update
        # the file.
        return False


# Script entry point.
#
if __name__ == "__main__":

    # Parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument("srcDir", help="path to the source directory")
    parser.add_argument("buildDir", help="path to the build directory")
    parser.add_argument("config", help="build configuration (e.g.: 'Release', 'Debug')")
    parser.add_argument("windeployqtExe", help="path to windeployqt.exe")
    parser.add_argument("appName", help="name of the app (e.g.: 'illustration'")
    args = parser.parse_args()

    # Get and create useful directories
    config = args.config
    srcDir = Path(args.srcDir)
    buildDir = Path(args.buildDir)
    configDir = buildDir / config
    binDir = configDir / "bin"
    deployDir = buildDir / "deploy" / config

    appName = args.appName
    appFileName = f"vgc{appName}.exe"
    if needsToRun(configDir, appName):
        with FileLock(configDir / "windeployqt.lock"):
            print(f"Running windeployqt.exe for {appFileName}", flush=True)
            appFile = binDir / appFileName
            subprocess.run([args.windeployqtExe, str(appFile)])
            deployDir.mkdir(parents=True, exist_ok=True)
            vcredistFileName = "vc_redist.x64.exe"
            vcredistBinFile = binDir / vcredistFileName
            vcredistDeployFile = deployDir / vcredistFileName
            if vcredistBinFile.is_file():
                if vcredistDeployFile.is_file():
                    vcredistBinFile.unlink()
                else:
                    vcredistBinFile.rename(vcredistDeployFile)
