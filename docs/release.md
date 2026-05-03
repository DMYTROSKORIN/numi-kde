# Release and Contribution Rules

This project is public. Keep commits, releases, and metadata clean.

## Authorship

All commits must use the project owner as both author and committer:

```sh
git config user.name "DMYTROSKORIN"
git config user.email "DMYTROSKORIN@users.noreply.github.com"
```

Do not add `Co-Authored-By` lines for AI systems or individual models. Multiple AI
tools may help develop the project, but GitHub contributors must reflect the
repository owner only.

Before committing, verify:

```sh
git log -1 --format='%an <%ae> / %cn <%ce>'
git shortlog -sne --all
```

## Semgrep

Use the tracked hook path:

```sh
git config core.hooksPath .githooks
```

The pre-commit hook runs `scripts/check-semgrep.sh` with the repository-local
`.semgrep.yml`. This avoids network-dependent `semgrep --config=auto` behavior in
normal commits.

For manual checks:

```sh
scripts/check-semgrep.sh
```

Use `semgrep --config=auto` only as an optional manual or CI check where network
access is expected.

## Release Checklist

1. Fix the issue and keep unrelated refactors out.
2. Bump `project(... VERSION X.Y.Z ...)` in `kde/CMakeLists.txt`.
3. Add a top entry to `CHANGELOG.md`.
4. Run:

```sh
cmake --build build/kde --target numi-kde numi-kde-tests
ctest --test-dir build/kde --output-on-failure
cmake --build build/kde-release -j$(nproc)
./build/kde-release/numi-kde --probe
./build/kde-release/numi-kde-tests
cpack -G RPM --config build/kde-release/CPackConfig.cmake
```

5. Generate `SHA256SUMS` for the release assets.
6. Commit without AI co-author metadata.
7. Create and push tag `vX.Y.Z`.
8. Create GitHub Release with RPM, `install.sh`, `uninstall.sh`, and `SHA256SUMS`.
