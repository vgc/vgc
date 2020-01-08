#!/bin/sh
#
# Use this script to update all the copyright years in the source code and
# documentation.
#
# Usage:
# cd vgc
# ./tools/update_copyright_year.sh <previous-year> <current-year>
#
# Note: individual authors should not forget to update their lines in the
# COPYRIGHT file whenever they contribute new code in a given year.

grep --exclude-dir=.git -rnwl . -e "Copyright $1 The VGC Developers" |
while read filename
do
    sed -i "s/Copyright $1 The VGC Developers/Copyright $2 The VGC Developers/g" $filename
    echo $filename
done
