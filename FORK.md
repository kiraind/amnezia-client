# Лёша VPN fork of AmneziaVPN

This repo (`kiraind/amnezia-client`, default branch `dev`) is a fork of
[amnezia-vpn/amnezia-client](https://github.com/amnezia-vpn/amnezia-client). It ships as
**Лёша VPN** (`LeshaVPN`, app id `ru.kiraind.vpn`) and installs side by side with the official
AmneziaVPN. Its purpose: log in to a self-hosted VPN coordinator (Telegram-backed account) and get
auto-updated AmneziaWG configs, with Amnezia's own commercial/gateway features hidden or disabled.

Upstream base of the fork: `f1af8ef`. Inventory: `git log --oneline f1af8ef..HEAD`,
`git diff --stat f1af8ef..HEAD` (after each upstream sync, the base becomes the merged upstream
tag; use `git log --oneline --no-merges upstream/<tag>..dev` to list fork-only commits).

To integrate a new upstream release follow `.claude/skills/sync-upstream/SKILL.md`. When a merge
conflicts, find the file below and resolve **by intent**: keep upstream's new behaviour, then
re-apply the fork's change described here.

---

## 1. Coordinator client (fork feature)
Commits: `c1dde1e`, `6818032`, `e70a501`.

Fork-only files (never conflict; upstream doesn't have them, but check that APIs they call still exist):
- `client/core/controllers/coordinator/coordinatorApi.{h,cpp}` — HTTP client for the coordinator API
- `client/core/controllers/coordinator/coordinatorController.{h,cpp}` — login, config sync, import of managed configs into the servers repo
- `client/ui/controllers/coordinator/coordinatorUiController.{h,cpp}` — QML-facing controller (`CoordinatorController` in QML)
- `client/ui/qml/Pages2/PageCoordinatorLogin.qml`, `PageCoordinatorConfigs.qml`
- `client/tests/testCoordinatorController.cpp` (mock API test, `CoordinatorControllerTest`)

Fork hunks in upstream files (conflict-prone):
- `client/core/repositories/secureAppSettingsRepository.{h,cpp}` — `coordinatorBaseUrl/Token/User/Configs` accessors + `clearCoordinatorAccount()`; not part of app backups.
- `client/secureQSettings.cpp` — `"Coordinator/token"` added to `encryptedKeys`; keychain default derived from `APPLICATION_NAME`.
- `client/core/controllers/coreController.{h,cpp}` — members `m_coordinatorApi`, `m_coordinatorController`, `m_coordinatorUiController`; construction and QML context registration.
- `client/core/controllers/coreSignalHandlers.cpp` — `CoordinatorUiController::errorOccurred` -> `PageController::showErrorMessage(QString)`.
- `client/cmake/sources.cmake` — coordinator `.h/.cpp` in HEADERS/SOURCES; `ui/controllers/coordinator/*` in the UI_CONTROLLERS globs.
- `client/ui/utils/pageEnum.h` — `PageCoordinatorLogin`, `PageCoordinatorConfigs`; `client/ui/qml/qml.qrc` — the two pages.
- `client/ui/qml/Pages2/PageSettings.qml` — `coordinatorAccount` entry ("Аккаунт VPN").
- `client/CMakeLists.txt` — `COORDINATOR_API_URL` cache var -> `-DCOORDINATOR_API_URL`; `deploy/build.sh` / `deploy/build.bat` pass it through.
- `client/tests/CMakeLists.txt` — `test_coordinator_controller` target + `add_test`.

How to resolve: take upstream's version of the file, then re-add the fork hunk(s) above. If upstream
renamed/refactored something the coordinator uses (servers repository, import controller,
PageController, settings repo), adapt the coordinator code to the new API rather than keeping the old
upstream code. `CoordinatorControllerTest` must pass afterwards.

## 2. MTU handling on import
File: `client/core/controllers/selfhosted/importController.cpp`, `ImportController::processAmneziaConfig`.
Fork change: instead of always overwriting `mtu` with the platform default, keep the issuer's MTU if it
is smaller than the default (coordinator configs use 1280 due to double encapsulation).
How to resolve: keep upstream's surrounding logic; the rule "mtu = issuer MTU if 0 < issuerMtu <= default, else default" must survive.

## 3. Connection-screen options removed
File: `client/ui/qml/Pages2/PageSetupWizardConfigSource.qml` (commits `50d8b95`, `0f472e8`, `8419f3b`).
Fork state: `variants` = `coordinatorLogin`, `fileOpen`, `qrScan` only. Removed: "VPN by Amnezia"
(`amneziaVpn`), self-hosted setup (`selfHostVpn`), `backupRestore`, `restorePurchases`, `siteLink`,
and the "Site Amnezia" footer link.
How to resolve: take upstream's file for new/changed code, then delete the removed variants again and
keep the fork's list. Any NEW upstream option that goes to Amnezia's gateway/shop/site must also be dropped.
Related: `PageSettingsAbout.qml`, `PageSettingsApplication.qml` (update UI gated by `IsAppUpdatesEnabled`),
`PageSettings.qml` ("О приложении %1" with `Qt.application.displayName`).

## 4. App updates disabled
- `client/cmake/branding/common.cmake` — `CLIENT_ENABLE_APP_UPDATES` defaults to `0`.
- `client/amneziaApplication.cpp` — QML context property `IsAppUpdatesEnabled`; update check under `#if CLIENT_ENABLE_APP_UPDATES`.
- `client/core/controllers/updateController.cpp` — `#if !CLIENT_ENABLE_APP_UPDATES` early-out.
- QML: update buttons `visible: IsAppUpdatesEnabled`.
How to resolve: keep the gating. If upstream adds a new update/check path (new controller, new QML button,
new gateway "latest version" call), wrap it in the same `CLIENT_ENABLE_APP_UPDATES` / `IsAppUpdatesEnabled` gate.

## 5. Rebrand (names, ids, IPC, sockets, installers)
Commit `7ddc416`. Single source of truth: `client/cmake/branding/*.cmake`:
- `common.cmake`: `CLIENT_TARGET_NAME`/`CLIENT_APPLICATION_NAME` = `LeshaVPN`, `CLIENT_DISPLAY_NAME` = `Лёша VPN`
  (also emitted as `DISPLAY_NAME` UTF-8 escapes in `version.h`), `CLIENT_APP_DOMAIN` = `ru.kiraind.vpn`,
  `CLIENT_SERVICE_NAME` = `LeshaVPN-service`, `CLIENT_ORGANIZATION_NAME` = `kiraind.ru`,
  `CLIENT_APP_INSTANCE_NAME` = `LeshaVPNInstance`, keychain `${APPLICATION_NAME}-Keychain`.
- `android.cmake`: `CLIENT_ANDROID_PACKAGE` = `ru.kiraind.vpn`. `apple.cmake`: bundle ids from `CLIENT_APP_DOMAIN`,
  group `group.ru.kiraind.vpn`, empty `BUILD_VPN_DEVELOPMENT_TEAM`. `ios.cmake`, `macos.cmake`: same idea.
Code that now derives from these macros instead of hard-coded `AmneziaVPN`/`org.amnezia`:
- `ipc/ipc.h` (`IPC_SERVICE_URL = "local:" APPLICATION_NAME "IpcInterface"`), `client/daemon/daemonlocalserver.cpp`,
  `client/mozilla/localsocketcontroller.cpp` (daemon socket names), `client/main.cpp`, `client/amneziaApplication.cpp`,
  `client/core/utils/migrations.cpp`, `client/ui/utils/{notificationHandler,systemTrayNotificationHandler,qAutoStart}.cpp`,
  `client/platforms/windows/**` (service/tunnel/firewall/split-tunnel names), `client/platforms/macos/**`,
  `client/platforms/ios/{Log,NELogController}.swift`, `client/core/protocols/ikev2VpnProtocolWindows.h`,
  `service/server/{CMakeLists.txt,systemservice.cpp,amneziavpn-service.rc.in}`, `client/platforms/windows/amneziavpn.rc.in`.
- Root `CMakeLists.txt`: includes `client/cmake/clientBrandingDefaults.cmake` early; macOS service group `AMN_SERVICE_GROUP` = lowercased app name.
- `client/CMakeLists.txt` / `client/cmake/macos.cmake`: bundle path `${PROJECT}.app`, `MACOSX_BUNDLE_GUI_IDENTIFIER`.
- Android: `client/android/{AndroidManifest.xml,build.gradle,build.gradle.kts}` (applicationId `ru.kiraind.vpn`; Kotlin
  namespace/package `org.amnezia.vpn` is intentionally unchanged), `res/values*/strings.xml` (app name), `src/org/amnezia/vpn/*.kt`
  (notification channel/tile/auth labels), `protocolApi/.../Protocol.kt`.
- Apple: `client/ios/app/{Info.plist.in,main.entitlements}`, `client/ios/networkextension/*`, `client/macos/app/{Info.plist.in,app.entitlements}`,
  `client/macos/networkextension/*.entitlements` (app group written literally — keep in sync with `apple.cmake`).
- Packaging: `cmake/CPack.cmake` (names from branding, own installer GUIDs/ids, WiX culture ru-RU for the Cyrillic name),
  `cmake/CPackOptions.cmake` (macOS pkg signing identity only when set, so unsigned CI builds work), `cmake/sign_binaries.cmake`,
  `deploy/installer/qif/{componentscript.js,controlscript.js}`, deploy data turned into templates:
  `deploy/data/linux/{app.desktop.in,app.service.in,post_install.sh.in,post_uninstall.sh.in}`,
  `deploy/data/macos/{post_install.sh.in,post_uninstall.sh.in,service.plist.in}`,
  `deploy/data/windows/{post_install.cmd.in,post_uninstall.cmd.in}`, `deploy/installer/**/{close_client_patch,service_install_patch}.xml.in`.
How to resolve: never take upstream's literal names/ids. Take upstream's logic, then replace any literal
`AmneziaVPN`, `AmneziaVPN-service`, `AmneziaVPN.ORG`, `org.amnezia.AmneziaVPN`, `amnvpn`, pipe/socket names, GUIDs
with the branding variable/macro. If upstream changed a deploy file the fork renamed to `.in` (e.g. `deploy/data/linux/AmneziaVPN.desktop`),
git shows modify/delete or rename conflicts: port upstream's change into the `.in` template, keep the `@VAR@` placeholders, delete the upstream literal file.
Translated strings mentioning "AmneziaVPN" in user-visible UI should use `Qt.application.displayName` / `DISPLAY_NAME`.

## 6. Flipped logo icons and "Лёша VPN" wordmarks (binary assets)
Commits `9df8e81`, `440d780`. Files: `client/images/{icon.png,app.ico,app.icns,AmneziaVPN.png,amneziaBigLogo.png,AmneziaVPN_Full_logo.svg}`,
`client/images/controls/amnezia.svg`, `client/images/tray/*.png`, `client/android/res/drawable*/logo.png`,
`client/android/res/drawable/ic_amnezia_round.xml`, `client/android/res/mipmap-*/{icon,icon_round,ic_launcher_foreground,ic_banner}.png`,
`client/ios/app/Media.xcassets/AppIcon.appiconset/*.png`, `client/ios/app/launch.png`,
`client/macos/app/Images.xcassets/AppIcon.appiconset/*.png`, `deploy/data/linux/AmneziaVPN.png`, `metadata/en-US/images/icon.png`.
Rule (generator scripts are not in the repo): the logo glyph is **rotated 180° about its mask centroid** (visual center of the
non-background pixels), background/shape kept as is. Wordmarks (`AmneziaVPN_Full_logo.svg`, `amneziaBigLogo.png`, `ic_banner`, launch screen)
show the text **"Лёша VPN"** set in `client/fonts/pt-root-ui_vf.ttf`, next to the flipped logo.
How to resolve: binary conflicts — keep ours (`git checkout --ours <file>`). If upstream changed/added an icon that contains the logo,
take upstream's new image and re-apply the rule (Python + Pillow: compute alpha/mask centroid, rotate 180° around it, composite onto the original background;
SVG: wrap the glyph in `transform="rotate(180 cx cy)"`). Never ship the upright Amnezia logo or the "AmneziaVPN" wordmark.

## 7. CI workflow
Commit `affb205`. `.github/workflows/deploy.yml` is fully rewritten: builds LeshaVPN for Linux, Windows, macOS, Android
(iOS optional via `vars.ENABLE_IOS`), runs tests on Linux, publishes a GitHub Release on `v*` tags, uses repo var `COORDINATOR_API_URL`
and optional signing secrets, never uploads to Amnezia's infra. Upstream's `tag-deploy.yml` and `tag-upload.yml` are deleted.
How to resolve: keep ours (`git checkout --ours .github/workflows/deploy.yml`; `git rm` the tag-* files if upstream modified them).
Port only useful upstream changes manually (Qt version bump, new conan/toolchain steps, new required modules/packages).
New upstream workflows: delete them unless they are generic and safe (no Amnezia secrets/servers).

## 8. Versioning
- `AMNEZIAVPN_VERSION` (root `CMakeLists.txt`) = the upstream base version the fork is built on (e.g. `5.0.3.1`); `APP_ANDROID_VERSION_CODE` likewise comes from upstream. `APP_VERSION` (upstream's) is still what goes to Amnezia's gateway code paths.
- `LESHA_VERSION` = fork release counter (positive integer, CACHE, default 1), bumped by 1 for every fork release and **never reset**, not even on an upstream merge. Configure fails if it reaches 100.
- Derived in one block of root `CMakeLists.txt` (the only place they are computed), e.g. for `5.0.3.1` + `1`:
  - `LESHA_FULL_VERSION` `5.0.3.1-lesha.1`: About page, `X-App-Version` (via the `APP_FULL_VERSION` define in `version.h.in`), Android versionName, package file names.
  - `LESHA_NUMERIC_VERSION` `5.0.3001` (MAJOR.MINOR.PATCH*1000+N): MSI/CPack version. `LESHA_RC_VERSION` `5,0,3001,1`: Windows `.rc` files.
  - `LESHA_ANDROID_VERSION_CODE` = upstream code (with offset) * 100 + N, e.g. `217101`. `LESHA_APPLE_BUILD` = TWEAK*1000 + N, e.g. `1001`.
- Release tag must be exactly `v<LESHA_FULL_VERSION>` (optionally `-rcN`/`-betaN` for a prerelease); the release job in `deploy.yml` checks it against `CMakeLists.txt` and fails otherwise.
- Files: root `CMakeLists.txt`, `version.h.in`, `cmake/CPack.cmake`, `client/cmake/{android,ios,macos,macos_ne}.cmake`, both `networkextension/CMakeLists.txt`, `client/platforms/windows/amneziavpn.rc.in`, `service/server/amneziavpn-service.rc.in`, `client/main.cpp`, settings controllers (About page), `coordinatorApi.cpp`, `.github/workflows/deploy.yml`.
How to resolve: for `AMNEZIAVPN_VERSION` and `APP_ANDROID_VERSION_CODE` take **theirs**; keep the fork's `LESHA_*` block and the places that use `LESHA_*`/`APP_FULL_VERSION` instead of upstream's version vars; bump `LESHA_VERSION` by 1. If upstream adds a new place that stamps a version (a new platform plist, installer, manifest), point it at the `LESHA_*` values too.

## Translations
Fork UI strings are Russian source text inside `qsTr(...)` (e.g. "Аккаунт VPN"). The fork does not edit `client/translations/*.ts`,
so on conflict take upstream's `.ts` files. Only if a fork commit ever added entries to them, re-add those entries.
