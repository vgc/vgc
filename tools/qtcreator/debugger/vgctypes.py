# How to use this file?
#
# In Qt Creator, indicate the path to this file in:
#
#   Tools > Options > Debugger > Locals & Expressions > Extra Debugging Helper
#
# During a debugging session, you should now see more convenient values
# printed in the "Value" column for VGC types (e.g., vgc::core::Array,
# vgc::core::StringId, etc.)
#
#
# Implementation notes
#
# In order to pretty-print a given type vgc::core::Foo in the Qt Creator
# debugger, we need to define the following function:
#
# qdump__vgc__core__Foo(d, value):
#     # Our implementation here
#     # d is of type 'Dumper'
#     # value is of type 'Dumper.Value'
#
# See https://doc.qt.io/qtcreator/creator-debugging-helpers.html for a very
# incomplete and partially broken/out-of-date documentation of the API.
#
# See Qt/Tools/QtCreator/share/qtcreator/debugger/dumper.py for the full API,
# almost undocumented but at least complete.
#
# The Dumper class is basically a "JSON Stream Writer". For example, if we
# call d.put("something"), it simply writes "something" to d.output. There
# are convenient higher-level functions to simplify implementation (such as
# putValue(), putType(), putArrayData(), etc.), but at the end of the day,
# they simply write strings to d.output. See official documentation for what
# the final JSON should look like.
#
# Some "put" functions write directly to d.output, but other "put" functions
# defer the writing and instead store it temporarily in d.currentValue and
# d.currentType, and only later write it to the output string. This makes it
# possible to modify these ourselves. Note that the Python type of
# d.currentValue and d.currentType is 'ReportItem', implemented as:
#
# class ReportItem():
#     """
#     Helper structure to keep temporary 'best' information about a value
#     or a type scheduled to be reported. This might get overridden be
#     subsequent better guesses during a putItem() run.
#     """
#
#     def __init__(self, value=None, encoding=None, priority=-100, elided=None):
#         self.value = value
#         self.priority = priority
#         self.encoding = encoding
#         self.elided = elided
#
#     def __str__(self):
#         return 'Item(value: %s, encoding: %s, priority: %s, elided: %s)' \
#             % (self.value, self.encoding, self.priority, self.elided)
#
# When we call d.putValue("something"), it simply does:
#   d.currentValue = ReportItem("something", None)
#
# That is, the value is written as a str with no encoding. It will create
# errors if your string contains special characters not allowed in JSON
# strings. You can either escape those characters with json.dumps(myString)
# [1:-1], or encode the string as an UTF-8 hex-encoded string, with
# d.hexencode(myString) and put the value with d.putValue
# (myHexEncodedString, 'utf8'). However, keep in mind that for each
# utf8-encoded value like this, Qt Creator will automatically surround your
# given string with double quotes. If you don't want those, you need to use
# the JSON-escaping approach.
#  
# Note that when errors are encountered, you basically get no useful output.
# The only way we found to debug pretty-printers is to write to a file. You
# may find the following snippet useful:
#
# import os
#
# qdump__vgc__core__Foo(d, value):
#     d.putItem(value)
#     file_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'debug.txt')
#     file = open(file_path, "w")
#     file.write("output:\n\n")
#     file.write(d.output)
#     file.write("\n\ncurrent value:\n\n")
#     file.write(str(d.currentValue))
#     file.write("\n\ncurrent type:\n\n")
#     file.write(str(d.currentType))
#     file.close()
#

import json

# Notes:
# - We can't use putItemCount(), because it would override our putValue()
# - We need to explicitly cast value["data_"] to an integer, otherwise
#   the check self.checkIntType(base) inside putArrayData() fails.
# - value.type[0] is of type Dumper.Type and represents the first
#   template parameter of the class template.
#
def qdump__vgc__core__Array(d, value):
    length = value["length_"].integer()
    reserved = value["reservedLength_"].integer()
    data = value["data_"].pointer()
    d.putValue(f"<{length} items / {reserved} reserved>")
    d.putNumChild(length)
    if d.isExpanded():
        d.putArrayData(data, length, value.type[0])


def qdump__vgc__core__ObjPtr(d, value):
    obj = hex(value["obj_"].pointer())
    refCount = value["obj_"]["refCount_"].integer()
    d.putValue(f"{{refCount={refCount} obj={obj}}}")
    d.putPlainChildren(value["obj_"].dereference())


def qdump__vgc__core__StringId(d, value):
    # Pretend we're an std::string
    d.putItem(value["stringPtr_"].dereference())

    # Modify the "Value" column
    encoding = d.currentValue.encoding            # utf8
    old_value = d.currentValue.value              # 48656C6C6F20776F726C64
    decoded = d.hexdecode(old_value, encoding)    # Hello world
    address = hex(value["stringPtr_"].pointer())  # 0x1f37fa27490
    desired = f'"{decoded}" (id={address})'       # "Hello world" (id=0x1f37fa27490)
    jsonified = json.dumps(desired)               # "\"Hello world\" (id=0x1f37fa27490)"
    new_value = jsonified[1:-1]                   # \"Hello world\" (id=0x1f37fa27490)
    new_encoding = None                           # Ensures QtCreator don't add extra quotes
    d.putValue(new_value, new_encoding)

    # Modify the "Type" column
    d.putType("vgc::core::StringId")


def qdump__vgc__core__Vec2f(d, value):
    x = value["data_"][0].value()
    y = value["data_"][1].value()
    d.putValue(f"({x}, {y})")
    d.putPlainChildren(value)


def qdump__vgc__core__Vec2d(d, value):
    x = value["data_"][0].value()
    y = value["data_"][1].value()
    d.putValue(f"({x}, {y})")
    d.putPlainChildren(value)
