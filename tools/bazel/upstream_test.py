"""Run the maintained upstream crypto and container test executables."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile

# These are the seven programs run by tests/Makefile's maintained run-tests.
# The remaining upstream tools need external TLS/X.509 fixtures and are built
# as compile coverage only. Base64/RC4 have self-contained test vectors too.
EXPECTED = {"test-aes", "test-base64", "test-list", "test-md4", "test-milenage", "test-rc4", "test-rsa-sig-ver", "test-sha1", "test-sha256"}


def main():
    binaries = [Path(value).absolute() for value in sys.argv[1:]]
    assert {p.name for p in binaries} == EXPECTED
    for binary in binaries:
        if binary.name == "test-base64":
            # The maintained executable is a conversion tool, so exercise its
            # real encode/decode interface with the RFC 4648 short vectors.
            vectors = [(b"f", b"Zg=="), (b"fo", b"Zm8="), (b"foo", b"Zm9v"),
                       (b"foob", b"Zm9vYg=="), (b"fooba", b"Zm9vYmE="),
                       (b"foobar", b"Zm9vYmFy")]
            with tempfile.TemporaryDirectory(dir=os.environ.get("TEST_TMPDIR")) as name:
                root = Path(name)
                for plain, encoded in vectors:
                    (root / "plain").write_bytes(plain)
                    subprocess.run([binary, "encode", root / "plain", root / "encoded"], check=True, timeout=90)
                    assert (root / "encoded").read_bytes().strip() == encoded
                    subprocess.run([binary, "decode", root / "encoded", root / "decoded"], check=True, timeout=90)
                    assert (root / "decoded").read_bytes() == plain
            print("== test-base64 ==\n6 encode/decode vectors passed", flush=True)
            continue
        result = subprocess.run([binary], capture_output=True, text=True, timeout=90)
        print(f"== {binary.name} ==\n{result.stdout}{result.stderr}", flush=True)
        assert result.returncode == 0, (binary.name, result.returncode)
    print(f"{len(binaries)} upstream executable test suites passed")


if __name__ == "__main__":
    main()
