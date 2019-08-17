#!/usr/bin/env python3
#
# Generates a file called version.txt with versioning info. Example:
#
#   manufacturer=VGC Software
#   suite=VGC
#   versionType=alpha
#   versionMajor=
#   versionMinor=
#   versionName=Alpha 2019-07-10
#   commitRepository=https://github.com/vgc/vgc
#   commitBranch=master
#   commitHash=09793d56c9b4046e4507b5d13f06cd1772ce8cab
#   commitDate=2019-07-10
#   commitTime=12:16:35
#   commitIndex=0
#   buildCompiler=MSVC
#   buildCompilerVersion=19.16.27031.1
#   buildArchitecture=x64
#   buildConfig=Release
#   buildDate=2019-07-15
#   buildTime=09:11:12
#
# We detail below some of the less obvious values.
#
# Suite
# -----
#
# We call a "suite" a group of related apps which are designed to be installed
# together and share resources, such as DLLs, images, fonts, etc.
#
# For example, "Adobe CC" and "Open Office" are suites. In our case, we are
# developing a suite named "VGC". You can change this name if you are forking
# the project.
#
# Version type
# ------------
#
# We call "version type" one of the following: "stable", "beta", and "alpha".
#
# Builds of VGC with different version types are considered to be different
# programs, and are installed side-by-side in different folders, e.g.:
#
# - C:/Program Files/VGC/
# - C:/Program Files/VGC Beta/
# - C:/Program Files/VGC Alpha/
#
# Note that other things determine whether different versions can be installed
# side-by-side, such as the "upgrade policy". See vgc/tools/windows/wix.py for
# more details.
#
# Version number (major and minor)
# --------------------------------
#
# Alpha versions don't have any version number associated with them. Both
# versionMajor and versionMinor are empty strings.
#
# Beta versions only have a major version number associated with them, e.g.,
# "2020". versionMinor is an empty string.
#
# Stable versions have both a major and minor version number, e.g., 2020.0.
#
# Commit branch
# -------------
#
# This is the Git branch this commit belongs to.
#
# But let's clarify this because it's not as obvious as it seems. In theory,
# in Git, a commit doesn't "belong to" any branch. A commit is just a node of
# a directed acyclic graph, and a branch is just a reference to one such node.
# In other words, a branch is not a set of commits, it's just one commit. Most
# commits don't have any branch associated with them, as far as Git is
# concerned.
#
# However, in practice, and regardless of how Git works internally, most
# projects are following a branching model to organize development. In such
# models, the word "branch" usually does not mean a reference to a single
# commit, but rather means the "whole branch" more similarly to what a branch
# means for real living trees. This is what we mean by "commit branch", and it
# is quite well defined, even though git isn't aware a it. Here is our
# definition:
#
#   For any given commit, we define its "commit branch" to be the name of the
#   checked out branch when this commit was created.
#
# Indeed, when a commit is first created, there is one and only one branch
# that references this commit, which is the branch currently checked out. This
# is what we call the commit branch, and it never changes. Let's take the
# following sequence of commands as examples:
#
# - git checkout master:
#     HEAD is a commit (let's call it c1) belonging to the master branch.
#
#       --o--o c1 (master)
#
# - git checkout -b 2020:
#     A new release branch is created, but for now it still references the same
#     commit c1, belonging to master.
#
#       --o--o c1 (master, 2020)
#
# - bump_version.sh && git commit:
#     Now, a new commit is created, let call it c2. This new commit belongs to
#     the branch "2020".
#
#              o c2 (2020)
#             /
#       --o--o c1 (master)
#
# - git checkout master && git checkout -b gh62:
#     We've created a new topic branch "gh62" off master, say, to fix a bug.
#     The new topic branch still points to c1, belonging to master.
#
#              o c2 (2020)
#             /
#       --o--o c1 (master, gh62)
#
# - fix_bug.sh && git commit:
#     A new commit is created, let call it c3, fixing the bug. This new commit
#     belongs to the branch "gh62".
#
#              o c2 (2020)
#             /
#       --o--o c1 (master)
#             \
#              o c3 (gh62)
#
# - fix_bug_2.sh && git commit:
#     A new commit is created, let call it c4, improving the bug fix based on
#     feedback from code review. This new commit belongs to the branch "gh62".
#
#              o c2 (2020)
#             /
#       --o--o c1 (master)
#             \
#              o--o c4 (gh62)
#
# - git checkout master && git merge --squash gh62:
#     The bug-fix is squash-merged to master. This creates a new commit c5,
#     containing all the changes introduced in gh62. c5 belongs to master.
#
#              o c2 (2020)
#             /
#       --o--o--o c5 (master)
#             \
#              o--o c4 (gh62)
#
#     (Alternatively, one could also use "git merge --no-ff gh62", which would
#      also create a new commit c5 belonging to master)
#
# - git branch -D gh62:
#     The branch gh62 is deleted. The commits c3 and c4 are now orphaned and
#     can be considered deleted.
#
#              o c2 (2020)
#             /
#       --o--o--o c5 (master)
#             \
#           (  o--o c4  ) <- orphaned
#
# - git checkout 2020 && git cherry-pick c5:
#     The bug-fix is cherry-picked for inclusion in the 2020 release. This
#     creates a new commit c6, containing all the changes introduced by c5
#     (originally introduced by gh62). c6 belongs to 2020.
#
#              o--o c6 (2020)
#             /
#       --o--o--o c5 (master)
#             \
#           (  o--o c4  ) <- orphaned
#
# In summary, any commit can be attributed a "commit branch":
#
# master: c1, c5
# 2020: c2, c6
# gh62: c3, c4 (now orphaned)
#
# Commit index
# ------------
#
# Count how many ancestors of this commit have the same commitDate as this
# commit. This makes it possible to uniquely identify a commit in a
# human-friendly way:
#
#   commitBranch.commitDate.commitIndex   (example: master.2019-07-10.0)
#
# However, note that topic branches are short-lived and eventually deleted, so
# for commits on a topic branch, this can only be a short-term identifier and
# does not entirely replace the commit hash.
#

from datetime import datetime, timezone
from pathlib import Path
import argparse
import subprocess

# Converts a date given as a string in a given format as a UTC datetime
# object.
#
# Example:
# dateformat = '%Y-%m-%d %H:%M:%S %z'
# datestring = '2019-06-23 18:00:45 -0800'
# d = utcdatetime(datestring, dateformat)   # -> datetime(2019, 6, 24, 2, 0, 45, tzinfo=utc)
# print(d)                                  # -> "2019-06-24 02:00:45+00:00"
#
def utcdatetime(datestring, dateformat):
    return datetime.strptime(datestring, dateformat).astimezone(timezone.utc)

# Script entry point.
#
if __name__ == "__main__":

    # Parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument("srcDir")
    parser.add_argument("buildDir")
    parser.add_argument("gitExecutable")
    parser.add_argument("manufacturer")
    parser.add_argument("suite")
    parser.add_argument("versionType")
    parser.add_argument("versionMajor")
    parser.add_argument("versionMinor")
    parser.add_argument("commitRepository")
    parser.add_argument("commitBranch")
    parser.add_argument("buildCompiler")
    parser.add_argument("buildCompilerVersion")
    parser.add_argument("buildArchitecture")
    parser.add_argument("buildConfig")
    args = parser.parse_args()

    # Import arguments into the global namespace.
    # This allows to more simply write `foo` instead of `args.foo`.
    globals().update(vars(args))

    # The resulting string to write to version.txt
    res = ""

    # Basic version info
    res += "manufacturer=" + manufacturer + "\n"
    res += "suite=" + suite + "\n"
    res += "versionType=" + versionType + "\n"
    res += "versionMajor=" + versionMajor + "\n"
    res += "versionMinor=" + versionMinor + "\n"

    # Get commit info
    commitHash = ""
    commitDate = ""
    commitTime = ""
    commitIndex = ""

    if gitExecutable:

        # Branch
        if not commitBranch:
            p = subprocess.run(
                [gitExecutable, "rev-parse", "--abbrev-ref", "HEAD"],
                cwd=srcDir, encoding='utf-8', stdout=subprocess.PIPE)
            if p.returncode == 0:
                lines = p.stdout.splitlines()
                if len(lines) > 0:
                    commitBranch = lines[0]

        # Date
        p = subprocess.run(
            [gitExecutable, "rev-parse", "HEAD"],
            cwd=srcDir, encoding='utf-8', stdout=subprocess.PIPE)
        if p.returncode == 0:
            lines = p.stdout.splitlines()
            if len(lines) > 0:
                commitHash = lines[0]

        # Date, time, and index
        p = subprocess.run(
            [gitExecutable, "--no-pager", "log", "-100", "--pretty=format:%ci"],
            cwd=srcDir, encoding='utf-8', stdout=subprocess.PIPE)
        if p.returncode == 0:
            lines = p.stdout.splitlines()
            if len(lines) > 0:
                dateformat = '%Y-%m-%d %H:%M:%S %z'
                datetimes = [utcdatetime(s, dateformat) for s in lines]
                isodates = [d.date().isoformat() for d in datetimes]
                commitDate = isodates[0]
                commitTime = datetimes[0].time().isoformat()
                commitIndexInt = isodates.count(commitDate) - 1
                commitIndex = str(commitIndexInt)

    # Human-readable version name
    if versionType == "stable":
        versionName = versionMajor
        if int(versionMinor) > 0:
            versionName += "." + versionMinor
    else:
        if versionType == "beta":
            versionName = versionMajor + " Beta"
        else:
            versionName = "Alpha"
        hasCommitInfo = commitBranch and commitDate and commitIndex
        if hasCommitInfo:
            if ((versionType == "beta"  and commitBranch != versionMajor) or
                (versionType == "alpha" and commitBranch != "master")):
                versionName += "-" + commitBranch
            versionName += " " + commitDate + "." + commitIndex
    res += "versionName=" + versionName + "\n"

    # Note: installFamily, installVersion, and installHumanVersion are not in
    # the version.txt file since they depend on the upgradePolicy, which we
    # cannot know at this time. See vgc/tools/windows/wix.py for details.

    # Write commit info
    res += "commitRepository=" + commitRepository + "\n"
    res += "commitBranch=" + commitBranch + "\n"
    res += "commitHash=" + commitHash + "\n"
    res += "commitDate=" + commitDate + "\n"
    res += "commitTime=" + commitTime + "\n"
    res += "commitIndex=" + commitIndex + "\n"

    # Build info
    now = datetime.now(timezone.utc)
    res += "buildCompiler=" + buildCompiler + "\n"
    res += "buildCompilerVersion=" + buildCompilerVersion + "\n"
    res += "buildArchitecture=" + buildArchitecture + "\n"
    res += "buildConfig=" + buildConfig + "\n"
    res += "buildDate=" + now.date().isoformat() + "\n"
    res += "buildTime=" + now.time().isoformat(timespec="seconds") + "\n"

    # Write to file
    versionDir = Path(buildDir) / buildConfig / "resources" / "core"
    versionDir.mkdir(parents=True, exist_ok=True)
    versionPath = versionDir / "version.txt"
    versionPath.write_text(res)
