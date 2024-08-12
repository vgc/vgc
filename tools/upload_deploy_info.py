#!/usr/bin/env python3

from pathlib import Path
import argparse
import json
import os
import re
import sys
import time
import urllib.parse
import urllib.request

# Prints with flush=True by default, preventing truncattion/order issues in CI logs.
#
def print_(*objects, sep=' ', end='\n', file=sys.stdout, flush=True):
    print(*objects, sep=sep, end=end, file=file, flush=flush)


# Returns whether the given 'name' is likely to refer to sensitive information.
#
def is_sensitive(name):
    name = name.lower()
    for s in ['pass', 'key', 'token']:
        if s in name:
            return True
    return False


# Returns a copy of the data but with sensitive information replaced
# with asterisks.
#
def hide_sensitive(data: dict):
    res = {}
    for key, value in data.items():
        if isinstance(value, dict):
            res[key] = hide_sensitive(v)
        elif value != "" and is_sensitive(key) :
            res[key] = '******'
        else:
             res[key] = value
    return res


# Makes a POST request to the given URL with the given data.
# The given data should be a Python dictionary, which this function
# automatically encodes as JSON. Finally, the JSON response is decoded
# and returned as a python dictionary.
#
def post_json(url, urlargs, jsondata):

    jsontext = json.dumps(jsondata)
    jsonbytes = jsontext.encode("utf-8")

    print_(f"POST {url}")
    print_(f"  args={hide_sensitive(urlargs)}")
    print_(f"  json={hide_sensitive(jsondata)}")

    urlWithArgs = url + "?" + urllib.parse.urlencode(urlargs)
    request = urllib.request.Request(urlWithArgs)
    request.method = "POST"
    request.add_header("Content-Type", "application/json; charset=utf-8")
    request.add_header("Connection", "close")
    with urllib.request.urlopen(request, jsonbytes) as response:
        encoding = response.info().get_param("charset") or "utf-8"
        responsedata = json.loads(response.read().decode(encoding))
        print_(f"  response={hide_sensitive(responsedata)}")
        print_("  Done.")
        return responsedata


def github_upload(version, files, uploadUrl):

    if uploadUrl[-1] != '/':
        uploadUrl += '/'

    urlargs = {}
    eventName = os.getenv("GITHUB_EVENT_NAME")

    if eventName == "pull_request":
        ref = os.getenv("GITHUB_REF")
        assert ref, "Missing GITHUB_REF."
        pr = ref.split('/')[2]
        assert pr.isdigit(), "Failed to extract PR number from GITHUB_REF (={ref})."
        urlargs['key'] = ""
        urlargs['pr'] =  pr

    elif eventName == "push":
        key = os.getenv("VGC_GITHUB_KEY")
        assert key, "Missing key: cannot upload artifact"
        urlargs['key'] = key
        urlargs['pr'] = "false"

    else:
        print_("Skipping upload as this does not appear to be a pull request or push.")
        return

    url = "https://webhooks.vgc.io/github"
    response = post_json(url, urlargs, version)
    urlargs['releaseId'] = response["releaseId"]
    for filename in files:
        urlargs['filename'] = filename
        urlargs['externalUrl'] = uploadUrl + urllib.parse.quote(filename)
        post_json(url, urlargs, {})

    # TODO: Improve API so we can do a single request with version + files + uploadUrl,
    #       instead of one request per uploaded file


def main(args):

    # Get file/dirs
    file = Path(args.file)
    deployDir = file.parent

    # Parse JSON file
    infoText = file.read_text()
    print_(f"{file.name} = {infoText}")
    info = json.loads(infoText)
    version = info['version']
    files = info['files']

    # Upload deploy info
    if os.getenv("GITHUB_REPOSITORY") == "vgc/vgc":
        github_upload(version, files, args.url)
    else:
        print_("Skipping upload as this does not appear to be an official VGC repository.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("file", help="Path to the info.json file.")
    parser.add_argument("url", help="Root URL where the files where uploaded.")
    args = parser.parse_args()
    main(args)
