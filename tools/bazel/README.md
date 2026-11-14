# Native AMD64 WPA source build

Bazel 8.5.1 builds this repository's WPA 2.9.0 sources directly with native
`rules_cc` targets. The fixed source lists and configuration preserve
`debian/config/wpasupplicant/linux`, including the Linux and SONiC MACsec drivers.
The three installed programs use `cc_binary`; the SONiC C++ driver support uses a
private `cc_library`. Bazel selects the declared GCC toolchain and compiles the
original checkout files without invoking the production Makefiles. The supported
platform is native AMD64 on Debian Trixie.

```sh
bazel build //:wpasupplicant_pkg //:wpasupplicant_pkg.debug_symbols //:runtime_packages.json
bazel test //:package_test //:upstream_test
```

The normal `.bazelrc` uses the maintained SONiC registry and BCR. Common, patched
libnl and OpenSSL compile in their source-owning repositories. This frontend
consumes their existing C++ providers and shared-library outputs. Target D-Bus,
readline, PCSC and terminal ABI libraries remain allowed Debian inputs. Declared
execution tools come from the shared infrastructure package set. The root RC
retains its targeted kernel-tool download policy so cached Perl directory aliases
are materialized before local metadata actions use them. Ordinary repository and
action cache reads remain enabled. Both CI jobs fetch the immutable shared
source-mirror configuration and verify its SHA-256 before Bazel uses it.

The runtime TAR contains all three programs, service and D-Bus units, ifupdown
hooks, example configurations and documentation. Six manuals, changelogs and
selected documentation larger than 4 KiB follow Debian's `dh_compress`
convention, including `README.modes.gz` pointing to `README.Debian.gz`.
Compression uses public `rules_gzip` and the shared no-name toolchain. The separate
debug TAR contains matching non-executable, root-owned symbols with mode `0644`.
`runtime_packages.json` derives package records from the existing Debian recipe
and selected source and ABI dependencies. The build does not generate DEBs or run
maintainer scripts.

`package_test` checks exact source and generated payload bytes, modes, links and
epoch; three AMD64 PIE/RELRO/NOW ELF/debug pairs; GDB source lookup for the C and
SONiC C++ entry points; confined source-owned dynamic libraries; the IEEE PBKDF2
vector; and a private supplicant/control-client PING/PONG exchange. It does not
claim physical MACsec link coverage.

The upstream test graph preserves twelve maintained test programs and the
separate source lists and compile settings from `src/utils/Makefile`,
`src/crypto/Makefile` and `src/tls/Makefile`. Native `cc_binary` targets compile all
twelve. `upstream_test` runs the nine self-contained suites, including six Base64
vectors. The HTTPS and X.509 programs retain compile-only coverage because their
external fixtures are not supplied. The legacy Make source behavior stays
unchanged; its retired Bazel frontend and response-file transport are removed.

The AMD64 CI job audits its configured graph before execution, retains the runtime
and debug TARs and test results, and checks a no-op build, an observable
`wpa_passphrase.c` edit and exact byte restoration using the same output base.
Native compile actions provide file-level rebuild boundaries. The receipts bind
the tested commit to the resolved source archives and installed output hashes.
Raw action and build-event logs stay outside the public artifact upload.

CodeQL scans Python and native C++ on this revision. A fresh output base first
prepares declared tools and dependencies with the production build, before CodeQL
injects its tracer. Both commands use foreground batch mode. Private native
targets with the same sources and settings compile the three WPA programs and the
SONiC driver library under tracing. These owner actions have distinct outputs;
prepared local dependency actions remain cached while disk and remote action
cache reads stay disabled. The finalized database must contain the C program
entry points and SONiC `sonic_db_set` definition, with archived source bytes equal
to this checkout. The evidence helper uses the action-provided pinned
`CODEQL_DIST` bundle. Hardware-backed authentication, cross compilation and full
installer validation are outside this source validation.
