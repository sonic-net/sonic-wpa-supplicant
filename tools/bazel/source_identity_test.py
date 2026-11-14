"""Exercise source identity checks against a real shallow Git merge checkout."""
import importlib.util
import json
import sys
import zipfile
from pathlib import Path
import subprocess
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("wpa_validate", Path(__file__).with_name("validate.py"))
validate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validate)

class SourceIdentityTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="wpa-identity-")
        self.addCleanup(self.temporary.cleanup)
        root = Path(self.temporary.name)
        origin = root / "origin"
        origin.mkdir()
        def git(where, *args, input=None):
            return subprocess.check_output(["git", "-c", "user.name=SONiC test", "-c", "user.email=test@example.invalid", "-c", "commit.gpgSign=false", "-c", "core.hooksPath=/dev/null", "-C", str(where), *args], input=input, text=True, stderr=subprocess.PIPE).strip()
        git(origin, "init", "--initial-branch=main")
        (origin / "source.c").write_text("int baseline;\n")
        git(origin, "add", "source.c")
        git(origin, "commit", "-m", "baseline")
        self.base = git(origin, "rev-parse", "HEAD")
        git(origin, "switch", "-c", "feature")
        (origin / "source.c").write_text("int feature;\n")
        git(origin, "commit", "-am", "feature")
        self.head = git(origin, "rev-parse", "HEAD")
        self.merge = git(origin, "commit-tree", "HEAD^{tree}", "-p", self.base, "-p", self.head, input="tested merge\n")
        git(origin, "update-ref", "refs/heads/ci", self.merge)
        self.checkout = root / "checkout"
        git(root, "clone", "--depth=1", "--single-branch", "--branch=ci", origin.as_uri(), str(self.checkout))
        self.assertEqual(git(self.checkout, "show", "-s", "--format=%P", "HEAD"), "")

    def test_shallow_merge_preserves_both_actual_parent_identities(self):
        identity = validate.checkout_identity(self.checkout, self.head, self.base)
        self.assertEqual(identity["revision"], self.merge)
        self.assertEqual(identity["parents"], [self.base, self.head])

    def test_unrelated_head_and_base_are_rejected(self):
        for head, base in [("0" * 40, self.base), (self.head, "0" * 40)]:
            with self.subTest(head=head, base=base), self.assertRaises(AssertionError):
                validate.checkout_identity(self.checkout, head, base)

    def test_dirty_source_cannot_be_attributed_to_the_merge(self):
        (self.checkout / "unexpected.c").write_text("untracked source\n")
        with self.assertRaises(AssertionError):
            validate.checkout_identity(self.checkout, self.head, self.base)

class ArtifactEvidenceTest(unittest.TestCase):
    def test_action_bundle_does_not_require_cli_on_path(self):
        sys.path.insert(0, str(Path(__file__).parent))
        import codeql_coverage
        with tempfile.TemporaryDirectory(prefix="wpa-codeql-bundle-") as temporary:
            bundle = Path(temporary)
            cli = bundle / "codeql"
            cli.write_text("#!/bin/sh\nexit 0\n")
            cli.chmod(0o755)
            self.assertEqual(codeql_coverage.initialized_cli({"CODEQL_DIST": str(bundle)}), cli.resolve())
            with self.assertRaises(AssertionError):
                codeql_coverage.initialized_cli({"CODEQL_DIST": str(bundle / "absent")})

    def test_owner_definition_and_archived_bytes_are_both_required(self):
        sys.path.insert(0, str(Path(__file__).parent))
        import codeql_coverage
        with tempfile.TemporaryDirectory(prefix="wpa-extracted-") as temporary:
            root = Path(temporary)
            rows = []
            with zipfile.ZipFile(root / "src.zip", "w") as archive:
                for relative, function in codeql_coverage.OWNER_FUNCTIONS.items():
                    path = root / relative
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_text("/* " + relative + " */")
                    archive.writestr("compiled/" + relative, path.read_bytes())
                    rows.append([function, "/compiled/" + relative])
            self.assertEqual(len(codeql_coverage.verify_owner(rows, root / "src.zip", root)), 4)
            with self.assertRaises(AssertionError):
                codeql_coverage.verify_owner(rows[:-1], root / "src.zip", root)
            (root / "src/drivers/sonic_operators.cpp").write_text("changed source")
            with self.assertRaises(AssertionError):
                codeql_coverage.verify_owner(rows, root / "src.zip", root)

    def test_failed_or_missing_test_result_cannot_be_collected(self):
        with tempfile.TemporaryDirectory(prefix="wpa-results-") as temporary:
            root = Path(temporary)
            events = root / "events.jsonl"
            events.write_text(json.dumps({"id": {"testResult": {"label": validate.TESTS[0]}},
                                          "testResult": {"status": "FAILED"}}))
            with self.assertRaises(AssertionError):
                validate.retain_test_results(events, root / "results")
            events.write_text("")
            with self.assertRaises(AssertionError):
                validate.retain_test_results(events, root / "results")

if __name__ == "__main__":
    unittest.main()
