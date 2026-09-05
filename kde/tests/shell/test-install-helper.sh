#!/usr/bin/env bash
# Tests for kde/resources/numi-kde-install-update.sh (the polkit helper).
#
# curl, rpm and dnf are replaced by stubs on PATH so nothing is downloaded or
# installed. Every case asserts the exit code AND that dnf was not invoked
# unless the input was fully verified.
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)
HELPER_SRC="$ROOT/kde/resources/numi-kde-install-update.sh"

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
stubs="$work/bin"
mkdir -p "$stubs" "$work/release"
export STUB_DIR="$work"
# The helper reads the project key from /etc/pki/rpm-gpg; the test copy points at a fixture instead.
cp "$ROOT/packaging/RPM-GPG-KEY-numi-kde" "$work/RPM-GPG-KEY-numi-kde"
HELPER="$work/helper.sh"
sed "s|^PUBLIC_KEY=.*|PUBLIC_KEY=\"$work/RPM-GPG-KEY-numi-kde\"|" "$HELPER_SRC" > "$HELPER"

passed=0
failed=0
check() { # name condition got want
  if [[ "$2" == "true" ]]; then
    printf '  PASS  %s\n' "$1"; passed=$((passed + 1))
  else
    printf '  FAIL  %s\n        got:      "%s"\n        expected: "%s"\n' "$1" "$3" "$4"; failed=$((failed + 1))
  fi
}

# ── Stubs ───────────────────────────────────────────────────────────────────
# curl -o <file> <url>: copies the matching file from $STUB_DIR/release, 404 otherwise.
cat > "$stubs/curl" <<'EOF'
#!/usr/bin/env bash
out=""; url=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    -o) out="$2"; shift 2 ;;
    http*) url="$1"; shift ;;
    *) shift ;;
  esac
done
name=$(basename "$url")
[[ -f "$STUB_DIR/release/$name" ]] || exit 22
cp "$STUB_DIR/release/$name" "$out"
EOF
# Fake rpm layout: line 1 = package name, line 2 = signer key id ("unsigned" for none).
# rpm -q --qf %{VERSION} numi-kde → installed version
# rpm -qp --qf %{NAME} <file>      → line 1;  rpm -qp --qf %{RSAHEADER:pgpsig} → "RSA/SHA256, ..., Key ID <line 2>"
cat > "$stubs/rpm" <<'EOF'
#!/usr/bin/env bash
if [[ "$1" == "-qp" ]]; then
  file="${@: -1}"
  if [[ "$*" == *RSAHEADER* ]]; then
    signer=$(sed -n 2p "$file")
    [[ "$signer" == "unsigned" ]] && { printf '(none)'; exit 0; }
    printf 'RSA/SHA256, Fri Sep  5 2026, Key ID %s' "$signer"; exit 0
  fi
  head -n1 "$file"; exit 0
fi
[[ -n "${STUB_INSTALLED:-}" ]] || exit 1
printf '%s' "$STUB_INSTALLED"
EOF
# rpmkeys --import <key> → records; rpmkeys --checksig <rpm> → "signatures OK" only for signed fakes.
cat > "$stubs/rpmkeys" <<'EOF'
#!/usr/bin/env bash
if [[ "$1" == "--import" ]]; then echo "import $2" >> "$STUB_DIR/rpmkeys.log"; exit 0; fi
file="${@: -1}"
if [[ "$(sed -n 2p "$file")" == "unsigned" ]]; then echo "$file: digests OK"; else echo "$file: digests signatures OK"; fi
EOF
cat > "$stubs/dnf" <<'EOF'
#!/usr/bin/env bash
printf '%s\n' "$*" >> "$STUB_DIR/dnf.log"
EOF
chmod +x "$stubs"/*
export PATH="$stubs:$PATH"

arch=$(uname -m)
GOOD_KEY="411c68b856cec16e"
make_release() { # version name-inside-rpm [break-checksum|unsigned|wrongkey]
  local name="$2" rpm="numi-kde-$1-$arch.rpm" signer="$GOOD_KEY"
  [[ "${3:-}" == "unsigned" ]] && signer="unsigned"
  [[ "${3:-}" == "wrongkey" ]] && signer="deadbeefcafef00d"
  rm -f "$work/release"/*
  printf '%s\n%s\nfake rpm payload\n' "$name" "$signer" > "$work/release/$rpm"
  ( cd "$work/release" && sha256sum "$rpm" > SHA256SUMS )
  if [[ "${3:-}" == "break-checksum" ]]; then
    echo "tampered" >> "$work/release/$rpm"
  fi
}
run_helper() { rm -f "$work/dnf.log"; set +e; bash "$HELPER" "$@" >"$work/out" 2>"$work/err"; rc=$?; set -e; }
dnf_called() { [[ -s "$work/dnf.log" ]] && echo yes || echo no; }

# ── Cases ───────────────────────────────────────────────────────────────────
echo "=== numi-kde install helper ==="
export STUB_INSTALLED="0.1.81"

run_helper "/tmp/evil.rpm"
check "path argument is rejected (exit 2, no dnf)" "$([[ $rc -eq 2 && $(dnf_called) == no ]] && echo true || echo false)" "rc=$rc dnf=$(dnf_called)" "rc=2 dnf=no"

run_helper "1.2"
check "non-semver version is rejected" "$([[ $rc -eq 2 && $(dnf_called) == no ]] && echo true || echo false)" "rc=$rc dnf=$(dnf_called)" "rc=2 dnf=no"

run_helper ""
check "empty version is rejected" "$([[ $rc -eq 2 ]] && echo true || echo false)" "rc=$rc" "rc=2"

make_release 0.1.80 numi-kde
run_helper "0.1.80"
check "downgrade is refused (exit 3, no dnf)" "$([[ $rc -eq 3 && $(dnf_called) == no ]] && echo true || echo false)" "rc=$rc dnf=$(dnf_called)" "rc=3 dnf=no"

make_release 0.1.81 numi-kde
run_helper "v0.1.81"
check "same version is refused" "$([[ $rc -eq 3 && $(dnf_called) == no ]] && echo true || echo false)" "rc=$rc dnf=$(dnf_called)" "rc=3 dnf=no"

rm -f "$work/release"/*
run_helper "0.1.90"
check "missing release asset fails before dnf (exit 4)" "$([[ $rc -eq 4 && $(dnf_called) == no ]] && echo true || echo false)" "rc=$rc dnf=$(dnf_called)" "rc=4 dnf=no"

make_release 0.1.90 numi-kde break-checksum
run_helper "0.1.90"
check "checksum mismatch is refused (exit 5, no dnf)" "$([[ $rc -eq 5 && $(dnf_called) == no ]] && echo true || echo false)" "rc=$rc dnf=$(dnf_called)" "rc=5 dnf=no"

make_release 0.1.90 numi-kde
rm -f "$work/release/SHA256SUMS"; ( cd "$work/release" && echo "0000 other.rpm" > SHA256SUMS )
run_helper "0.1.90"
check "rpm not listed in SHA256SUMS is refused (exit 5)" "$([[ $rc -eq 5 && $(dnf_called) == no ]] && echo true || echo false)" "rc=$rc dnf=$(dnf_called)" "rc=5 dnf=no"

make_release 0.1.90 evil-package
run_helper "0.1.90"
check "foreign package name is refused (exit 6, no dnf)" "$([[ $rc -eq 6 && $(dnf_called) == no ]] && echo true || echo false)" "rc=$rc dnf=$(dnf_called)" "rc=6 dnf=no"

make_release 0.1.90 numi-kde unsigned
run_helper "0.1.90"
check "unsigned rpm is refused (exit 7, no dnf)" "$([[ $rc -eq 7 && $(dnf_called) == no ]] && echo true || echo false)" "rc=$rc dnf=$(dnf_called)" "rc=7 dnf=no"

make_release 0.1.90 numi-kde wrongkey
run_helper "0.1.90"
check "rpm signed by a foreign key is refused (exit 8, no dnf)" "$([[ $rc -eq 8 && $(dnf_called) == no ]] && echo true || echo false)" "rc=$rc dnf=$(dnf_called)" "rc=8 dnf=no"

make_release 0.1.90 numi-kde
run_helper "0.1.90"
dnf_args=$(cat "$work/dnf.log" 2>/dev/null || true)
ok=false
[[ $rc -eq 0 && "$dnf_args" == install\ -y\ /var/tmp/numi-kde-update.*/numi-kde-0.1.90-$arch.rpm ]] && ok=true
check "verified, signed release is installed without --nogpgcheck" "$ok" "rc=$rc dnf='$dnf_args'" "rc=0 dnf='install -y /var/tmp/numi-kde-update.*/numi-kde-0.1.90-$arch.rpm'"
check "project key was imported before checking the signature" "$([[ -s "$work/rpmkeys.log" ]] && echo true || echo false)" "$(cat "$work/rpmkeys.log" 2>/dev/null)" "import <key>"

leftovers=$(find /var/tmp -maxdepth 1 -name "numi-kde-update.*" 2>/dev/null | wc -l)
check "temporary download directory is cleaned up" "$([[ $leftovers -eq 0 ]] && echo true || echo false)" "$leftovers dirs left" "0 dirs left"

unset STUB_INSTALLED
make_release 0.1.90 numi-kde
run_helper "0.1.90"
check "fresh install (package not yet installed) is allowed" "$([[ $rc -eq 0 && $(dnf_called) == yes ]] && echo true || echo false)" "rc=$rc dnf=$(dnf_called)" "rc=0 dnf=yes"

printf '\n=== Results: %d passed, %d failed ===\n' "$passed" "$failed"
[[ $failed -eq 0 ]]
