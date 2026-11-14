"""Validate the installed WPA payload, source-owned libraries and debug pairs."""

import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tarfile
import tempfile
import time

from openssl.package_test import check_gdb, check_symbols, unpack_files

BINARIES = {"usr/sbin/wpa_supplicant", "usr/sbin/wpa_cli", "usr/bin/wpa_passphrase"}
EPOCH = 1598266417


def canonical(path):
    path = Path(path).as_posix().rstrip("/")
    return "usr/" + path if path.startswith(("sbin/", "lib/")) else path


def inventory(source, generated):
    """Read the existing Debian payload declarations; reject unknown syntax."""
    result = {}
    for line in (source / "debian/wpasupplicant.install").read_text().splitlines():
        fields = line.split()
        if not fields or fields[0].startswith("#"):
            continue
        assert len(fields) == 2, fields
        if fields[0].startswith("wpa_supplicant/systemd/"):
            matches = [path for name, path in generated.items() if name.startswith("wpa_supplicant") and name.endswith(".service")]
        elif fields[0].startswith("wpa_supplicant/dbus/fi."):
            matches = [path for name, path in generated.items() if name.startswith("fi.") and name.endswith(".service")]
        elif fields[0] in {"wpa_supplicant/" + Path(name).name for name in BINARIES}:
            matches = [generated[Path(fields[0]).name]]
        else:
            matches = list(source.glob(fields[0]))
        assert matches and all(path.is_file() for path in matches), fields
        for path in matches:
            result[canonical(fields[1] + "/" + path.name)] = path
    result["etc/dbus-1/system.d/wpa_supplicant.conf"] = source / "wpa_supplicant/dbus/dbus-wpa_supplicant.conf"
    for suffix, directory in [("docs", "usr/share/doc/wpasupplicant"), ("examples", "usr/share/doc/wpasupplicant/examples")]:
        for pattern in (source / ("debian/wpasupplicant." + suffix)).read_text().splitlines():
            if not pattern.strip() or pattern.lstrip().startswith("#"):
                continue
            matches = list(source.glob(pattern.strip()))
            assert matches, pattern
            for path in matches:
                result[directory + "/" + path.name] = path
    for filename, origin in {
        "copyright": "debian/copyright",
        "README.Debian": "debian/wpasupplicant.README.Debian",
        "changelog.Debian": "debian/changelog",
        "changelog": "wpa_supplicant/ChangeLog",
    }.items():
        result["usr/share/doc/wpasupplicant/" + filename] = source / origin
    for pattern in (source / "debian/wpasupplicant.manpages").read_text().splitlines():
        name = Path(pattern).name
        result["usr/share/man/man" + name[-1] + "/" + name] = source / pattern
    links = {}
    for line in (source / "debian/wpasupplicant.links").read_text().splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        target, destination = line.split()
        links[canonical(destination)] = canonical(target)
    return result, links


def main():
    runtime, debug, control, dependencies, filter_file, *inputs = map(Path, sys.argv[1:])
    source = control.parent.parent
    generated = {path.name: path for path in inputs[:8]}
    manuals, owner_tars = inputs[8:13], inputs[13:]
    assert len(manuals) == 5 and owner_tars
    expected, expected_links = inventory(source, generated)
    for manual in manuals:
        destination = "usr/share/man/man" + manual.name[-1] + "/" + manual.name
        assert destination in expected and manual.is_file(), manual
        expected[destination] = manual
    compressed = {name for name, path in expected.items()
                  if name.startswith("usr/share/man/")
                  or (name.startswith("usr/share/doc/") and Path(name).name != "copyright"
                      and (path.stat().st_size > 4096 or Path(name).name.startswith("changelog")))}
    expected = {name + ".gz" if name in compressed else name: path
                for name, path in expected.items()}
    expected_links = {name + ".gz" if target in compressed else name:
                      target + ".gz" if target in compressed else target
                      for name, target in expected_links.items()}
    spec = importlib.util.spec_from_file_location("common_runtime_filter", filter_file)
    common_runtime = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(common_runtime)
    with tempfile.TemporaryDirectory(dir=os.environ.get("TEST_TMPDIR")) as temporary, \
            tempfile.TemporaryDirectory(prefix="wpa-", dir="/tmp") as socket_directory:
        work = Path(temporary)
        root, symbols = work / "root", work / "symbols"
        entries = unpack_files(runtime, root)
        debug_entries = unpack_files(debug, symbols)
        files = {name for name, member in entries.items() if member.isfile()}
        assert files == set(expected), sorted(files ^ set(expected))
        links = {name: member for name, member in entries.items() if member.issym()}
        assert set(links) == set(expected_links), (links, expected_links)
        for name, member in links.items():
            resolved = os.path.normpath(str(Path(name).parent / member.linkname))
            assert resolved == expected_links[name], (name, resolved, expected_links[name])
        for name, member in entries.items():
            assert (member.uid, member.gid, member.mtime) == (0, 0, EPOCH), name
            if member.isfile():
                executable = name in BINARIES or name.startswith("etc/wpa_supplicant/") or name in {
                    "usr/sbin/wpa_action", "usr/lib/pm-utils/sleep.d/60_wpa_supplicant",
                }
                assert member.mode == (0o755 if executable else 0o644), (name, member.mode)
                if name in BINARIES:
                    continue
                data = (root / name).read_bytes()
                if name.removesuffix(".gz") in compressed:
                    assert data[:2] == b"\x1f\x8b" and data[4:8] == b"\0" * 4 and not data[3] & 8, name
                    data = gzip.decompress(data)
                assert data == expected[name].read_bytes(), name
        actual_elf = {name for name in files if (root / name).read_bytes().startswith(b"\x7fELF")}
        assert actual_elf == BINARIES
        for name in sorted(BINARIES):
            binary = root / name
            assert struct.unpack_from("<HH", binary.read_bytes(), 16) == (3, 62), name
            dynamic = subprocess.check_output(["readelf", "-dW", binary], text=True)
            program = subprocess.check_output(["readelf", "-lW", binary], text=True)
            assert "BIND_NOW" in dynamic and "GNU_RELRO" in program, name
            assert re.search(r"GNU_STACK\s+.*\sRW\s", program), name
            check_symbols(binary, symbols, work, prefix_mapped=True)
            check_gdb(binary, symbols, "main", {"wpa_supplicant": "main.c", "wpa_cli": "wpa_cli.c", "wpa_passphrase": "wpa_passphrase.c"}[binary.name])
        check_gdb(root / "usr/sbin/wpa_supplicant", symbols, "sonic_db_set", "sonic_operators.cpp")
        debug_files = [m for m in debug_entries.values() if m.isfile()]
        assert len(debug_files) == 3
        for member in debug_files:
            assert (member.mode, member.uid, member.gid) == (0o644, 0, 0), (member.name, member.mode)

        # Reuse Common's confined Debian-link extraction. APT dependencies are
        # staged first; source owners then provide the exact selected libraries.
        for archive in [dependencies, *owner_tars]:
            with tarfile.open(archive) as payload:
                payload.extractall(root, filter=common_runtime.runtime_member_filter)
        owners = {}
        for archive in owner_tars:
            with tarfile.open(archive) as payload:
                for member in payload:
                    if member.isfile() and ".so" in member.name:
                        data = payload.extractfile(member).read()
                        if data.startswith(b"\x7fELF"):
                            destination = root / member.name.lstrip("./")
                            assert destination.read_bytes() == data, (archive, member.name)
                            owners[str(destination.resolve())] = hashlib.sha256(data).hexdigest()
        loader = root / "usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2"
        assert loader.is_file()
        library_dirs = sorted({str(p.parent.resolve()) for p in root.rglob("*.so*") if p.is_file()})
        prefix = [str(loader), "--library-path", ":".join(library_dirs)]
        environment = {k: v for k, v in os.environ.items() if not k.startswith("LD_")}
        environment.update(HOME=temporary, LC_ALL="C")
        loaded = {}
        for name in sorted(BINARIES):
            listing = subprocess.check_output(prefix + ["--list", str(root / name)], text=True, env=environment)
            assert "not found" not in listing, listing
            actual = re.findall(r"=>\s+(/\S+)", listing)
            assert actual and all(Path(p).resolve().is_relative_to(root) for p in actual), listing
            loaded[name] = {Path(p).name: str(Path(p).resolve().relative_to(root)) for p in actual}
        assert "libswsscommon.so.0" in loaded["usr/sbin/wpa_supplicant"]
        assert any(name.startswith("libnl-3.so") for name in loaded["usr/sbin/wpa_supplicant"])
        assert "libcrypto.so.3" in loaded["usr/bin/wpa_passphrase"]
        for binary, libraries in loaded.items():
            for name, relative in libraries.items():
                if name.startswith(("libswsscommon.so", "libnl-3.so", "libnl-genl-3.so", "libnl-route-3.so", "libcrypto.so", "libssl.so")):
                    assert str((root / relative).resolve()) in owners, (binary, name, relative)

        def run(binary, *args):
            return subprocess.check_output(prefix + [str(root / binary), *args], text=True, stderr=subprocess.STDOUT, env=environment, timeout=15)
        help_text = run("usr/sbin/wpa_supplicant", "-h")
        assert "macsec_linux" in help_text and "macsec_sonic" in help_text and "nl80211" in help_text, help_text
        passphrase = run("usr/bin/wpa_passphrase", "IEEE", "password")
        expected_psk = hashlib.pbkdf2_hmac("sha1", b"password", b"IEEE", 4096, 32).hex()
        assert "\tpsk=" + expected_psk in passphrase, passphrase
        # Linux sockaddr_un is limited to 108 bytes; Bazel TEST_TMPDIR is not.
        socket = Path(socket_directory) / "control.sock"
        log = work / "supplicant.log"
        with log.open("w") as output:
            process = subprocess.Popen(prefix + [str(root / "usr/sbin/wpa_supplicant"), "-g", str(socket)], stdout=output, stderr=subprocess.STDOUT, env=environment)
            try:
                for _ in range(100):
                    assert process.poll() is None, log.read_text()
                    if socket.exists():
                        break
                    time.sleep(0.05)
                assert socket.exists(), log.read_text()
                assert run("usr/sbin/wpa_cli", "-g", str(socket), "ping").strip() == "PONG"
            finally:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
        print(json.dumps({"files": len(files), "links": len(links), "elf_debug_pairs": 3, "gdb_sources": 4, "source_libraries": owners, "loaded_libraries": loaded, "private_control_ping": True, "pbkdf2_vector": True, "macsec_drivers": ["macsec_linux", "macsec_sonic"]}, indent=2))


if __name__ == "__main__":
    main()
