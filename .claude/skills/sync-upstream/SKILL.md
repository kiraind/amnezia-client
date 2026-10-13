---
name: sync-upstream
description: Merge a new upstream AmneziaVPN release into the Лёша VPN fork, resolve conflicts, keep fork changes, bump versions
---

# Sync upstream AmneziaVPN into the Лёша VPN fork

Read `FORK.md` first: it lists every fork change grouped by feature, with files and how to resolve
conflicts in each. Resolve conflicts **by intent** (keep upstream's new behaviour, re-apply the fork's change).

## Never
- Never rebase, squash or drop fork commits; integrate upstream with `git merge` only.
- Never force-push `dev` (or any shared branch). All work goes through a PR to `dev`.
- Never take upstream's branding/ids wholesale: app name, `org.amnezia.*` ids, IPC/socket/pipe names,
  service names, installer GUIDs, Apple team id, logo/wordmark images (see FORK.md §5, §6).
- Never take upstream's CI workflows wholesale (`deploy.yml` is fork-owned; `tag-deploy.yml`/`tag-upload.yml` stay deleted).
- Never bring back the app update checker, settings backup/restore or Amnezia gateway/commercial features
  (VPN by Amnezia, subscriptions/purchases, Amnezia site links, news/promo from Amnezia servers): they are
  deleted from the fork (FORK.md §4, §9.3).
- Never change `LESHA_VERSION` semantics; never reuse a released tag.

## 1. Prepare remotes
```bash
git remote get-url upstream 2>/dev/null || git remote add upstream https://github.com/amnezia-vpn/amnezia-client.git
git fetch upstream --tags
git fetch origin
```

## 2. Pick the upstream release
- Prefer release tags. Inspect naming first: `git tag -l --sort=-v:refname | head -20`
  (upstream uses plain versions like `4.8.x.y` / `5.x.y.z`; check, don't assume). Cross-check with
  `gh release list -R amnezia-vpn/amnezia-client` if available.
- Current base: `git merge-base origin/dev upstream/dev` and the `AMNEZIAVPN_VERSION` in root `CMakeLists.txt`.
- Pick the newest release tag newer than the base. If there is no tag, use the upstream commit the user names
  (or the release commit on `upstream/dev`) — state which in the PR.
- Read what changes: `git log --oneline <base>..<tag>`, `git diff --stat <base>..<tag>`.

## 3. Branch and merge
```bash
git checkout -b sync/upstream-<version> origin/dev
git merge --no-ff <tag> -m "Merge upstream AmneziaVPN <version>"
git status   # list conflicts
```

## 4. Resolve conflicts (by FORK.md group)
- **Coordinator (§1):** take upstream file, re-add fork hunks (repo accessors, `Coordinator/token` encrypted key,
  coreController members/registration, signal handler, sources.cmake entries, pageEnum/qrc, PageSettings entry, tests CMake).
  If upstream refactored APIs the coordinator uses, adapt coordinator code.
- **MTU (§2):** keep "issuer MTU if smaller than default" in `processAmneziaConfig`.
- **Connection screen (§3):** take upstream, then keep only `coordinatorLogin`, `fileOpen`, `qrScan`; drop new Amnezia/gateway options.
- **App updates (§4) / gateway, news, purchases, backup (§9.3):** keep the deleted files deleted; drop upstream hunks that
  call them; any new update/gateway/purchase path is not added to the build.
- **Rebrand (§5):** upstream logic + branding variables/macros. Upstream edits to files the fork turned into `.in`
  templates → port into the template, delete the literal file.
- **Icons (§6):** binary conflicts → `git checkout --ours <file>`; new/changed upstream logo images → re-flip (rule in FORK.md §6).
- **CI (§7):** `git checkout --ours .github/workflows/deploy.yml`; `git rm` tag-deploy/tag-upload if they reappear;
  port useful toolchain bumps (Qt version, conan, packages) by hand.
- **Translations:** take upstream's `client/translations/*.ts` (`git checkout --theirs`). Fork strings are Russian
  source text in `qsTr()`; re-add `.ts` entries only if a fork commit had added any.
- **Submodules / conan / recipes:** take theirs.
- Anything not in FORK.md: take theirs.

## 5. Versions
- `AMNEZIAVPN_VERSION` and `APP_ANDROID_VERSION_CODE` in root `CMakeLists.txt`: take **upstream's** values.
- Keep the fork's `LESHA_*` block (FORK.md §8) and bump `LESHA_VERSION` by 1 for this release. Never reset it to 1: the Android versionCode is `upstream code * 100 + LESHA_VERSION` and must keep growing. Configure fails at 100; if that happens, stop and ask the user.
- Check the configure log line `-- Version <base>-lesha.<N> (numeric ..., android code ..., apple build ...)`: the android code must be greater than the last released one.
- Full version is `<AMNEZIAVPN_VERSION>-lesha.<LESHA_VERSION>`; the release tag `v<full>` (exactly, or with `-rcN`/`-betaN`) is created after the PR is merged, only if the user asks. A mismatched tag fails the release job.

## 6. Re-audit new upstream code for identities and features
Over the merged range (`git diff <old-base>..<tag>` and the merged tree) check:
```bash
git grep -n "org\.amnezia" -- ':!client/android/src' ':!client/translations'   # bundle/app ids, groups (Kotlin package org.amnezia.vpn is intentionally kept)
git grep -n "AmneziaVPN" -- '*.cpp' '*.h' '*.mm' '*.swift' '*.kt' '*.qml' '*.cmake' '*.in' '*.js' '*.xml' '*.plist' ':!client/translations'
git grep -nE "IpcInterface|QLocalServer|pipe|/var/run|amnvpn" -- client service ipc deploy   # socket/pipe/service names
git grep -nE "gateway|updateController|checkForUpdates|api\.amnezia|amnezia\.org" -- client
git diff <old-base>..<tag> --stat -- client/ui/qml/Pages2/PageSetupWizard* client/images client/android/res client/ios client/macos deploy
```
Fix: user-visible names → `DISPLAY_NAME`/`Qt.application.displayName`; ids/paths/sockets/services → branding vars;
new connection-screen options → remove; new update checks or gateway calls → remove; new logo images → re-flip.
Commit fixes on the sync branch as separate commits (`fix: rebrand ... from upstream <version>`).

## 7. Verify
- **Preferred: CI.** Push the branch (`git push -u origin sync/upstream-<version>`) and open the PR; `deploy.yml`
  builds Linux/Windows/macOS/Android and runs tests on Linux. All jobs must be green.
- **Local build (optional, cloud container):** distro Qt is usually too old; install Qt 6.10.x via conda-forge
  (`conda create -n qt -c conda-forge qt6-main=6.10 qt6-...` matching the modules in `deploy.yml`: remoteobjects,
  5compat, shadertools, plus ninja/cmake/conan), point `CMAKE_PREFIX_PATH` at the env, then
  `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DAMNEZIA_BUILD_TESTS=ON && cmake --build build`.
  Conan pulls deps from Amnezia's public remote (`cmake/recipes_bootstrap.cmake`). If conda/Qt setup fails, rely on CI.
- `ctest --test-dir build --output-on-failure` — must include `CoordinatorControllerTest`.
- Smoke run: `QT_QPA_PLATFORM=offscreen ./build/client/LeshaVPN` for ~10 s; no QML errors in the log
  (missing `CoordinatorController`, unknown `PageEnum`, missing images), app name is LeshaVPN.

## 8. PR
Open a PR `sync/upstream-<version>` → `dev` titled `Merge upstream AmneziaVPN <version>` with:
- upstream range and tag, summary of notable upstream changes;
- every conflict and how it was resolved (per FORK.md group);
- re-audit findings and fixes (new ids, gated features, re-flipped icons);
- version: `AMNEZIAVPN_VERSION` old→new, `APP_ANDROID_VERSION_CODE`, `LESHA_VERSION` N→N+1;
- test/CI status.
Update `FORK.md` if any fork change moved files or a new fork hunk was needed.
