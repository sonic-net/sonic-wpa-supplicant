# Adapted from sonic-fips/openssl/package_test.py for the WPA package test.
# Copyright 2016-present Microsoft, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Inspect the WPA archive and verify its debug symbols."""

from pathlib import PurePosixPath
import re
import struct
import subprocess
import tarfile
import zlib


def run(*args):
    return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT)


def unpack_files(archive, destination):
    """Read package files without creating the installed system's absolute links."""
    entries = {}
    with tarfile.open(archive) as source:
        for member in source:
            path = PurePosixPath(member.name)
            if path.is_absolute() or ".." in path.parts:
                raise AssertionError(f"Unsafe package entry: {member.name}")
            name = str(path)
            if any(str(parent) in entries and entries[str(parent)].issym()
                   for parent in path.parents):
                raise AssertionError(f"Package file is below a link: {name}")
            if name in entries and not member.isdir():
                raise AssertionError(f"Duplicate package file: {name}")
            entries[name] = member
            if member.isfile():
                target = destination / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(source.extractfile(member).read())
                target.chmod(member.mode)
            elif not (member.isdir() or member.issym()):
                raise AssertionError(f"Unexpected package entry type: {name}")
    return entries


def build_id(path):
    return re.search(r"Build ID: ([0-9a-f]+)", run("readelf", "-n", str(path))).group(1)


def check_symbols(binary, symbols, scratch):
    identifier = build_id(binary)
    debug = symbols / "usr/lib/debug/.build-id" / identifier[:2] / (identifier[2:] + ".debug")
    assert debug.is_file(), (binary, identifier)
    assert build_id(debug) == identifier
    assert ".debug_info" not in run("readelf", "-SW", str(binary))
    assert ".debug_info" in run("readelf", "-SW", str(debug))
    section = scratch / "debuglink"
    run("objcopy", "--dump-section", f".gnu_debuglink={section}", str(binary), str(scratch / "copy"))
    data = section.read_bytes()
    end = data.index(b"\0")
    assert data[:end].decode() == debug.name
    offset = (end + 1 + 3) & ~3
    assert struct.unpack_from("<I", data, offset)[0] == zlib.crc32(debug.read_bytes())
    dynamic = run("readelf", "-d", str(binary))
    assert not re.search(r"(?:RPATH|RUNPATH).*?(?:bazel-out|execroot|build_tmpdir)", dynamic), dynamic


def check_gdb(binary, symbols, function, source):
    output = run(
        "gdb", "--batch", "--nx", "--quiet",
        "-ex", "set debuginfod enabled off",
        "-ex", f"set debug-file-directory {symbols / 'usr/lib/debug'}",
        "-ex", f"file {binary}",
        "-ex", f"info line {function}",
    )
    assert re.search(r"Line [0-9]+ of .*" + re.escape(source), output), output
