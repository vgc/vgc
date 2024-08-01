#!/usr/bin/env python3

from pathlib import Path
import argparse
import json
import os
import sys
import time
import urllib.parse
import urllib.request

# Prints with flush=True by default. Using flush=True tends to behave better and
# prevent truncating and ordering issues in TravisCI and perhaps other systems
# which process and redirect the output.
#
def print_(*objects, sep=' ', end='\n', file=sys.stdout, flush=True):
    print(*objects, sep=sep, end=end, file=file, flush=flush)

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
    request.add_header("Connection", "close")
    response = urllib.request.urlopen(request, databytes)
    with urllib.request.urlopen(request, databytes) as response:
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

# Script entry point.
#
if __name__ == "__main__":

    # Set verbosity
    verbose = True

    # Parse arguments, and import them into the global namespace
    parser = argparse.ArgumentParser()
    parser.add_argument("file", help="path to the info.json file")
    args = parser.parse_args()
    globals().update(vars(args))

    file = Path(file)
    deployDir = file.parent

    info = json.load(file.read_text())
    version = info['version']
    files = info['files']

    upload = False
    if os.getenv("GITHUB_REPOSITORY") == "vgc/vgc":
        eventName = os.getenv("GITHUB_EVENT_NAME")
        url = "https://webhooks.vgc.io/github"
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
        print_("Uploading commit metadata...")
        response = post_json(
            urlencode(url, {
                "key": key,
                "pr": pr
            }), version)
        print_(" Done.")
        releaseId = response["releaseId"]
        allFilesUploaded = True
        for filename in files:
            response = post_json(
                        urlencode(url, {
                            "key": key,
                            "pr": pr,
                            "releaseId": releaseId,
                            "filename": filename,
                            "externalUrl": f"https://example.com/{filename}"
                        }), {
                    })
