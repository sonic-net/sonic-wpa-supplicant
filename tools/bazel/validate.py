#!/usr/bin/env python3
"""Retain native source-TAR acceptance and test a restored source edit."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tarfile
import time
from urllib.parse import unquote, urlparse

ROOT = Path(__file__).resolve().parents[2]
OUTPUTS = {"//:wpasupplicant_pkg": "wpasupplicant.tar",
           "//:wpasupplicant_pkg.debug_symbols": "wpasupplicant.debug.tar.gz",
           "//:runtime_packages.json": "runtime_packages.json"}
TESTS = ["//:package_test", "//:upstream_test"]


def require(value, message):
    if not value:
        raise AssertionError(message)


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def checkout_identity(workspace, head_sha="", base_sha=""):
    subprocess.run(["git", "diff", "--exit-code", "HEAD"], cwd=workspace, check=True)
    require(not subprocess.check_output(["git", "status", "--porcelain"], cwd=workspace, text=True).strip(),
            "Acceptance requires a clean committed source checkout")
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=workspace, text=True).strip()
    headers = subprocess.check_output(["git", "cat-file", "-p", "HEAD"], cwd=workspace, text=True).split("\n\n", 1)[0]
    # History traversal hides parents at a shallow checkout boundary; the raw
    # commit still binds GitHub's tested merge to both requested revisions.
    parents = [line.removeprefix("parent ") for line in headers.splitlines() if line.startswith("parent ")]
    if head_sha and revision != head_sha:
        require(head_sha in parents and base_sha and base_sha in parents,
                "Tested merge does not contain the requested PR head and base")
    return {"revision": revision, "parents": parents, "requested_head": head_sha,
            "requested_base": base_sha}


def retain_test_results(events, destination):
    """Copy logs from this accepted invocation, not a stale output symlink."""
    results = {}
    for line in events.read_text().splitlines():
        event = json.loads(line)
        label = event.get("id", {}).get("testResult", {}).get("label")
        if label in TESTS:
            require(label not in results, "Unexpected repeated test attempt: " + label)
            result = event["testResult"]
            require(result.get("status") == "PASSED", "Test result was not passed: " + label)
            files = {}
            for item in result.get("testActionOutput", []):
                if item.get("name") in ("test.log", "test.xml"):
                    uri = urlparse(item["uri"])
                    require(uri.scheme == "file" and not uri.netloc, "Expected a local test artifact")
                    files[item["name"]] = Path(unquote(uri.path))
            require(set(files) == {"test.log", "test.xml"}, "Missing accepted test logs: " + label)
            results[label] = files
    require(set(results) == set(TESTS), "Missing accepted test results")
    for label, files in results.items():
        directory = destination / label.removeprefix("//:")
        directory.mkdir(parents=True, exist_ok=True)
        for name, source in files.items():
            require(source.is_file(), "Missing accepted test artifact: " + str(source))
            shutil.copyfile(source, directory / name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("stage", choices=["audit", "build", "incremental", "collect"])
    parser.add_argument("--output", type=Path, default=ROOT / "bazel-evidence")
    parser.add_argument("--output-user-root", type=Path)
    parser.add_argument("--codeql", action="store_true", help="Include the private same-source traced build in the audit")
    parser.add_argument("--head-sha", default=os.environ.get("CI_HEAD_SHA", ""))
    parser.add_argument("--base-sha", default=os.environ.get("CI_BASE_SHA", ""))
    args = parser.parse_args()
    require(platform.machine() == "x86_64", "Native AMD64 execution is required")
    output = args.output.resolve()
    private = output / "private"
    private.mkdir(parents=True, exist_ok=True)
    prefix = ["bazel"]
    if args.output_user_root:
        prefix += ["--output_user_root=" + str(args.output_user_root.resolve())]
    flags = ["--jobs=2", "--local_test_jobs=1"]

    def bazel(command, *arguments, log=None):
        started = time.monotonic()
        if log:
            with (private / log).open("w") as stream:
                result = subprocess.run([*prefix, command, *arguments], cwd=ROOT, stdout=stream,
                                        stderr=subprocess.STDOUT)
            if result.returncode:
                print((private / log).read_text(errors="replace")[-20000:], file=sys.stderr)
                result.check_returncode()
            return time.monotonic() - started
        return subprocess.check_output([*prefix, command, *arguments], cwd=ROOT, text=True)


    def resolved_inputs():
        graph = bazel("mod", "graph")
        require("sonic-build-infra@0.0.34-a4723268cd72a573ed316ab75347e698cc3f86ab" in graph,
                "Selected shared tooling version differs from the reviewed source")
        infra = bazel("mod", "show_repo", "@sonic_build_infra")
        require("http_archive(" in infra and "a4723268cd72a573ed316ab75347e698cc3f86ab" in infra
                and "sha256-/jYt+Q3QG7UIx1FdyA1VgHZsaYtYD09X7B+KPqrh98M=" in infra,
                "Actual tooling source is not the reviewed immutable archive; local overrides are not acceptance")
        libnl = bazel("mod", "show_repo", "@libnl3")
        require("libnl3@3.7.0.sonic.7" in graph and "http_archive(" in libnl
                and "0001-build-libnl3-with-shared-native-rules.patch" in libnl
                and "0002-match-shared-output-sonames.patch" in libnl,
                "Actual libnl repository lacks the reviewed owner interface")
        common = bazel("mod", "show_repo", "@sonic_swss_common")
        require("sonic-swss-common@0.0.9-334389eae54afd1f4f5ee69f89c3a7ad2062ee61" in graph
                and "http_archive(" in common and "334389eae54afd1f4f5ee69f89c3a7ad2062ee61" in common
                and "sha256-MX76U2Z8y01QmyNoO2ZVqIXvpWySdJuyovJ/unKqzuc=" in common,
                "Common differs from the registered owner source")
        fips = bazel("mod", "show_repo", "@sonic_fips")
        require("sonic-fips@0.0.6-18a778db501d455f8a1b6d1e3ef8e17121661177" in graph
                and "http_archive(" in fips and "18a778db501d455f8a1b6d1e3ef8e17121661177" in fips
                and "sha256-vzJY61s9LR7vXQTgw+wtr4G5uiFmJWglig8h8PiNKRs=" in fips,
                "OpenSSL differs from the registered owner source")
        (output / "fips-repository.txt").write_text(fips)
        (output / "common-repository.txt").write_text(common)
        record = {"fips_repository_sha256": hashlib.sha256(fips.encode()).hexdigest(),
                  "common_repository_sha256": hashlib.sha256(common.encode()).hexdigest(),
                  "module_graph_sha256": hashlib.sha256(graph.encode()).hexdigest(),
                  "infra_repository_sha256": hashlib.sha256(infra.encode()).hexdigest(),
                  "libnl_repository_sha256": hashlib.sha256(libnl.encode()).hexdigest()}
        (output / "resolved-modules.txt").write_text(graph)
        (output / "infra-repository.txt").write_text(infra)
        (output / "libnl-repository.txt").write_text(libnl)
        return record

    def paths():
        result = {}
        # Bind output lookup to the accepted invocation. Bazel info does not
        # reliably resolve apparent platform repository names under Bzlmod.
        execution_roots = set()
        for line in (private / "build-events.jsonl").read_text().splitlines():
            event = json.loads(line)
            if "workspaceInfo" in event:
                execution_roots.add(Path(event["workspaceInfo"]["localExecRoot"]))
        require(len(execution_roots) == 1, "Expected one accepted execution root")
        execution_root = execution_roots.pop()
        for label, name in OUTPUTS.items():
            files = bazel("cquery", "--output=files", "config(" + label + ", target)").splitlines()
            require(len(files) == 1, "Expected one public output: " + label)
            result[name] = execution_root / files[0]
            require(result[name].is_file(), "Missing public output: " + name)
        return result

    def snapshot(files):
        return {name: sha(path) for name, path in files.items()}

    def binaries(archive):
        with tarfile.open(archive) as source:
            return {m.name.lstrip("./"): hashlib.sha256(source.extractfile(m).read()).hexdigest()
                    for m in source if m.isfile() and m.name.lstrip("./") in {
                        "usr/sbin/wpa_supplicant", "usr/sbin/wpa_cli", "usr/bin/wpa_passphrase"}}

    identity = checkout_identity(ROOT, args.head_sha, args.base_sha)
    if args.stage != "audit":
        require(json.loads((output / "source-identity.json").read_text()) == identity,
                "Source checkout changed since the accepted action audit")

    if args.stage == "audit":
        (output / "source-identity.json").write_text(json.dumps(identity, indent=2) + "\n")
        (output / "resolved-inputs.json").write_text(json.dumps(resolved_inputs(), indent=2) + "\n")
        graph_path = private / "configured-actions.json"
        targets = [*OUTPUTS, *TESTS] + (["//:codeql_build"] if args.codeql else [])
        expression = "deps(set(" + " ".join(targets) + "))"
        graph_path.write_text(bazel("aquery", "--output=jsonproto", "--include_commandline",
                                    *(["--compilation_mode=dbg"] if args.codeql else []), expression))
        graph = json.loads(graph_path.read_text())
        fragments = {item["id"]: item for item in graph.get("pathFragments", [])}

        def path(identifier):
            item = fragments[identifier]
            return (path(item["parentId"]) + "/" if item.get("parentId") else "") + item["label"]

        artifacts = {item["id"]: path(item["pathFragmentId"]) for item in graph.get("artifacts", [])}
        outputs = {artifacts[identifier] for action in graph["actions"] for identifier in action.get("outputIds", [])}
        require(not any(name.endswith((".deb", ".udeb", ".ddeb")) for name in outputs), "DEB output in configured graph")
        for action in graph["actions"]:
            command = " ".join(action.get("arguments", []))
            require(not re.search(r"dpkg-buildpackage|dpkg-deb\s+(?:--build|-b)(?:\s|$)|pkg_deb|p4rt_(?:dbg_)?deb", command),
                    "DEB-producing command in configured graph")
        targets_by_id = {item["id"]: item["label"] for item in graph["targets"]}
        compiled = {}
        for action in graph["actions"]:
            label = targets_by_id[action["targetId"]]
            if not label.startswith(("//:", "//tests:")):
                continue
            require(action["mnemonic"] != "ForeignCc", "WPA compilation must use native actions: " + label)
            if action["mnemonic"] == "CppCompile":
                arguments = action.get("arguments", [])
                require("-c" in arguments, "Missing native source compile input: " + label)
                compiled.setdefault(label, set()).add(arguments[arguments.index("-c") + 1])
        expected = {"wpa_supplicant": 226, "sonic_operators": 1, "wpa_cli": 8, "wpa_passphrase": 11}
        for prefix in (["", "codeql_"] if args.codeql else [""]):
            for name, count in expected.items():
                label = "//:" + ("" if name == "sonic_operators" else "bin/") + prefix + name
                require(len(compiled.get(label, set())) == count, "Native source coverage differs: " + label)
        test_counts = {"utils": 14, "crypto": 45, "tls": 17}
        test_counts.update({name: 1 for name in [
            "test-aes", "test-base64", "test-list", "test-md4", "test-milenage", "test-rc4",
            "test-rsa-sig-ver", "test-sha1", "test-sha256", "test-x509v3", "test-https", "test-https_server",
        ]})
        for name, count in test_counts.items():
            require(len(compiled.get("//tests:" + name, set())) == count,
                    "Upstream native source coverage differs: " + name)
        (output / "native-sources.json").write_text(json.dumps(
            {label: sorted(sources) for label, sources in sorted(compiled.items())}, indent=2) + "\n")
        record = {"status": "passed", "architecture": "amd64", "actions": len(graph["actions"]),
                  "outputs": len(outputs), "deb_outputs": 0, "targets": targets}
        (output / "action-audit.json").write_text(json.dumps(record, indent=2) + "\n")
    elif args.stage == "build":
        require(json.loads((output / "action-audit.json").read_text())["status"] == "passed", "Audit required")
        elapsed = bazel("test", *flags, "--test_timeout=600", "--test_output=errors",
                        "--build_event_json_file=" + str(private / "build-events.jsonl"),
                        *OUTPUTS, *TESTS, log="build.log")
        (output / "build.json").write_text(json.dumps({"status": "passed", "seconds": elapsed,
                                                       "tests": TESTS}, indent=2) + "\n")
    elif args.stage == "incremental":
        require(json.loads((output / "build.json").read_text())["status"] == "passed", "Accepted build required")
        files = paths()
        original = snapshot(files)
        native = binaries(files["wpasupplicant.tar"])
        require(len(native) == 3, "Expected all three installed source binaries")
        noop = bazel("build", *flags, *OUTPUTS, log="noop.log")
        require(snapshot(files) == original, "No-op changed exported outputs")
        source = ROOT / "wpa_supplicant/wpa_passphrase.c"
        before = source.read_bytes()
        token = b'printf("network={\\n");'
        require(before.count(token) == 1, "Source edit anchor changed")
        try:
            source.write_bytes(before.replace(token, b'printf("network={ # bazel-rebuild-probe\\n");'))
            changed_time = bazel("build", *flags, *OUTPUTS, log="changed.log")
            changed = snapshot(files)
            changed_native = binaries(files["wpasupplicant.tar"])
            require(changed["wpasupplicant.tar"] != original["wpasupplicant.tar"] and
                    changed["wpasupplicant.debug.tar.gz"] != original["wpasupplicant.debug.tar.gz"],
                    "Source edit did not change runtime/debug artifacts")
            require(changed_native["usr/bin/wpa_passphrase"] != native["usr/bin/wpa_passphrase"], "Source edit missed installed program")
            require(all(changed_native[name] == value for name, value in native.items()
                        if name != "usr/bin/wpa_passphrase"), "Unrelated source binaries changed")
        finally:
            source.write_bytes(before)
            restored_time = bazel("build", *flags, *OUTPUTS, log="restored.log")
            require(snapshot(files) == original, "Restored source did not restore original output bytes")
        record = {"status": "passed", "seconds": {"noop": noop, "changed": changed_time, "restored": restored_time},
                  "original": original, "changed": changed, "restored": snapshot(files),
                  "changed_installed_program": "usr/bin/wpa_passphrase",
                  "scope": "Same retained output base; native source compile/link/package actions. No remote-cache-hit claim."}
        (output / "incremental.json").write_text(json.dumps(record, indent=2) + "\n")
    else:
        require(json.loads((output / "build.json").read_text())["status"] == "passed", "Accepted build required")
        incremental = json.loads((output / "incremental.json").read_text())
        require(incremental["status"] == "passed", "Accepted incremental check required")
        require(resolved_inputs() == json.loads((output / "resolved-inputs.json").read_text()),
                "Actual resolved source changed since the action audit")
        files = paths()
        require(snapshot(files) == incremental["original"] == incremental["restored"],
                "Exported artifacts differ from the accepted original/restored outputs")
        for name, source in files.items():
            shutil.copyfile(source, output / name)
        retain_test_results(private / "build-events.jsonl", output / "tests")
        lock = ROOT / "MODULE.bazel.lock"
        require(lock.is_file(), "Resolved module lock is missing")
        shutil.copyfile(lock, output / lock.name)
        revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
        record = {**identity, "architecture": "amd64", "artifacts": snapshot(files),
                  "resolved_inputs": json.loads((output / "resolved-inputs.json").read_text()),
                  "registry": "https://raw.githubusercontent.com/securely1g/sonic-bazel-registry/main",
                  "infra_source": "a4723268cd72a573ed316ab75347e698cc3f86ab"}
        (output / "provenance.json").write_text(json.dumps(record, indent=2) + "\n")


if __name__ == "__main__":
    main()
