#!/usr/bin/env python3
"""Prove the finalized C++ database contains WPA's actual compiled definitions."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import zipfile
from validate import checkout_identity

OWNER_FUNCTIONS = {
    "wpa_supplicant/main.c": "main",
    "wpa_supplicant/wpa_cli.c": "main",
    "wpa_supplicant/wpa_passphrase.c": "main",
    "src/drivers/sonic_operators.cpp": "sonic_db_set",
}
QUERY = '''import cpp
from Function f, File source
where source = f.getDefinition().getFile() and
  (source.getAbsolutePath().matches("%/wpa_supplicant/main.c") or
   source.getAbsolutePath().matches("%/wpa_supplicant/wpa_cli.c") or
   source.getAbsolutePath().matches("%/wpa_supplicant/wpa_passphrase.c") or
   source.getAbsolutePath().matches("%/src/drivers/sonic_operators.cpp"))
select f.getName(), source.getAbsolutePath()
'''


def verify_owner(rows, source_zip, workspace):
    checked = {}
    with zipfile.ZipFile(source_zip) as archive:
        names = set(archive.namelist())
        for relative, function in OWNER_FUNCTIONS.items():
            files = {path for name, path in rows if name == function and path.endswith("/" + relative)}
            if not files:
                raise AssertionError("Missing extracted function " + function + " in " + relative)
            source = (workspace / relative).read_bytes()
            for path in files:
                member = path.lstrip("/")
                if member not in names or archive.read(member) != source:
                    raise AssertionError("Extracted source bytes differ from the checkout: " + relative)
            checked[relative] = {"function": function, "sha256": hashlib.sha256(source).hexdigest(),
                                 "extracted_definitions": len(files)}
    return checked


def initialized_cli(environment):
    # github/codeql-action supplies the pinned bundle location but does not
    # promise to put its CLI on PATH in a container job.
    bundle = environment.get("CODEQL_DIST")
    selected = Path(bundle) / "codeql" if bundle else Path(shutil.which("codeql") or "")
    cli = selected.resolve()
    if not cli.is_file():
        raise AssertionError("The initialized CodeQL CLI is unavailable")
    return cli


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--database", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    workspace = Path(__file__).resolve().parents[2]
    identity = checkout_identity(workspace, os.environ.get("CI_HEAD_SHA", ""), os.environ.get("CI_BASE_SHA", ""))
    cli = initialized_cli(os.environ)
    version = json.loads(subprocess.check_output([str(cli), "version", "--format=json"], text=True))
    if version.get("version") != "2.27.0":
        raise AssertionError("CodeQL differs from the workflow's pinned bundle")
    bundle = cli.parent
    packs = json.loads(subprocess.check_output([str(cli), "resolve", "qlpacks", "--format=json"], text=True))
    candidates = [Path(path).resolve() for path in packs.get("codeql/cpp-all", [])]
    candidates = [path for path in candidates if path.is_relative_to(bundle)]
    versions = {re.search(r"^version: (\S+)$", (path / "qlpack.yml").read_text(), re.M).group(1)
                for path in candidates}
    if versions != {"12.1.0"}:
        raise AssertionError("Expected cpp-all 12.1.0 from the immutable CodeQL 2.27.0 bundle")
    with tempfile.TemporaryDirectory(prefix="wpa-codeql-query-") as temporary:
        root = Path(temporary)
        (root / "qlpack.yml").write_text("name: sonic/wpa-coverage\nversion: 0.0.0\ndependencies:\n  codeql/cpp-all: 12.1.0\n")
        query = root / "owner.ql"
        query.write_text(QUERY)
        result = root / "owner.bqrs"
        subprocess.run([str(cli), "query", "run", "--database=" + str(args.database),
                        "--search-path=" + str(bundle), "--threads=2", "--ram=2048",
                        "--output=" + str(result), str(query)], check=True)
        decoded = json.loads(subprocess.check_output([str(cli), "bqrs", "decode", "--format=json", str(result)], text=True))
        checked = verify_owner(decoded["#select"]["tuples"], args.database / "src.zip", workspace)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps({"status": "passed", "codeql": version["version"],
                                      "cpp_all": "12.1.0", "source_identity": identity, "owner_sources": checked}, indent=2) + "\n")


if __name__ == "__main__":
    main()
