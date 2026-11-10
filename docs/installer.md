# Installer

`installer/KotorAccessibilityInstaller/` is the end-user installer: one download, one run, a working game, without asking a blind player to find mods on DeadlyStream or drive TSLPatcher. It is a .NET 8 WinForms project published as one self-contained single-file EXE, and it references `third_party/Kotor-Patch-Manager/src/KPatchCore/` directly, so the same install logic as `kdev apply` runs on the player's machine. English only.

The original mod's design notes, research and history (the mod survey, the localized TSLRCM editions, the self-updater) are in `archiev/installer-design-original.md`.

## Where the mod comes from

`.github/workflows/build.yml` builds `Accessibility.kpatch` and the installer on every push to `bridge` and re-creates the pre-release `bridge-latest` with `accessibility.dll`, `Accessibility.kpatch` and `VoiceOfTheOldRepublicInstaller.exe`.

- The installer downloads `Accessibility.kpatch` from `bridge-latest` (`Config.ModRepositoryUrl`, `Config.ReleaseTag`), through the github.com release-download URL first and the API asset endpoint as a fallback.
- A build is named by its commit: `GitHubClient.GetModBuildAsync` reads the release's `target_commitish` and returns `bridge-<first 7 characters>`. That id is written to the uninstall entry; at the next start, an installed copy whose id differs from the published one is offered an update. Ids are compared for equality only.
- There is no self-update: the installer EXE is replaced by downloading it again from `bridge-latest`.

## Flows

At start (`Program.cs`): log, load the texts (`Locales/en.json`), load the third-party pins (`SourcePins`), require administrator rights, refuse while a game is running. Then:

- **Uninstall** (`/uninstall`, from the uninstall entry; `/quiet` for no UI): `UninstallForm` / `UninstallFlow` remove only this mod's files.
- **Installed and out of date**: `UpdateAvailableForm` — update the mod only, full install, toggle spatial audio, collect logs.
- **Installed and current**: `InstalledOptionsForm` — full install, toggle spatial audio, collect logs.
- **Not installed**: the full install.

The full install (`InstallFlow.RunFullInstallFlow`):

1. `WelcomeForm` — what the mod is, the latest build, then which game.
2. `GameVersionSelectionForm` — KOTOR 1, KOTOR 2 or both; `GameStoreLinksForm` for a game that is not found, and a folder picker before giving up on detection.
3. KOTOR 2 first, because TSLRCM writes freely into the game folder: `Kotor2ModSelectionForm` (TSLRCM, K2CP, Tweak Pack, Thematic Companions), a warning when Steam Workshop items are subscribed (they override directory-installed mods), TSLRCM from DeadlyStream (`TslrcmInstallForm`: silent Inno install verified by the `dialog.tlk` fingerprint, the visible wizard or a manual download as fallbacks), then K2CP, Tweak Pack and Thematic Companions (`Kotor2ModsInstallForm`) only when TSLRCM is in place. A spoken summary ends the step.
4. KOTOR 1: `ModdingInfoForm` (what is always installed), then `ModSelectionForm` (K1 Community Patch, on by default; Thematic Companions, off by default because it changes game balance).
5. `MainForm` once per selected game: path check, download, staging the patch runtime, KPatchCore install of the mod with the bundled engine patches the executable supports, the loader, the Visual C++ runtime, NVDA's controller client, the minigame cue assets and priority group, `swkotor.ini` accessibility and key-map defaults, optionally skipping the intro movies, the optional mods through HoloPatcher, the uninstaller and the uninstall entry. The end screen offers the README and starting the game.

Command-line arguments: `/uninstall`, `/quiet`, `--game k1|k2`, a game path, and for development `--local-kpatch <file>` (install a local build instead of downloading) and `--dev-gog-only`.

## What lands in the game folder

- `swkotor.exe`, with a backup, when a bundled engine patch rewrites it
- `KotorPatcher.dll`, `sqlite3.dll`, the address database and `patch_config.toml` (the Kotor-Patch-Manager runtime)
- `dinput8.dll`, the proxy loader: the game imports it, so it starts the runtime
- `msvcp140.dll`, `vcruntime140.dll` (see below)
- `patches/accessibility.dll` (from the `.kpatch`) and `patches/nvdaControllerClient32.dll` with its license; old `prism.dll` and `Tolk.dll` are removed
- the minigame cue files and the extended `prioritygroups.2da` in `Override`
- `KotorAccessibility_Uninstaller.exe`, a copy of the installer, so the uninstall entry keeps working after the download is deleted

Optional, from the maintenance screens: dsoal and OpenAL Soft for spatial game audio (`SpatialAudioManager`).

## Bundled and downloaded mods

The filter: no visual mods, no new story content, nothing that drifts far from vanilla balance; bug fixes that prevent soft locks are worth the most to a blind player.

- **K1 Community Patch** — `tslpatchdata/` fetched from the KOTORCommunityPatches GitHub repository at a pinned commit, installed with HoloPatcher. English.
- **Widescreen**, and for KOTOR 2 **4GB** and **borderless fullscreen** — engine patches bundled as `.kpatch` files, installed in the same KPatchCore transaction only on executables they declare.
- **TSLRCM 1.8.6, K2CP, Tweak Pack** (KOTOR 2) — from DeadlyStream, the English editions.
- **Thematic Companions** (both games, opt-in) — the authors' GitHub release assets, spoiler-free variant.

## Keeping the third-party pins alive

- **`Resources/sources.json`** (read through `SourcePins`) holds every pin: hashes, sizes, page URLs, commits. It is embedded and also read from this fork's `bridge` branch at start; the remote copy wins when it parses with a known schema version, and any failure falls back to the embedded one.
- **DeadlyStream mods** (TSLRCM, K2CP, Tweak Pack): a matching hash installs silently; a different hash is treated as an upstream update, checked structurally (readable archive, the expected folders, a real executable) and installed with a notice. A file that fails the structural check sends the player to the guided manual download.
- **GitHub tagged assets** (Thematic Companions) never relax: a mismatch fails. Pinned commits (K1CP) do not change.
- **Guided manual download** (`ManualDownloadForm`): when automatic download fails, the installer says why, opens the official page, and takes the file the player downloaded (file picker, or one key when a plausible file is already in Downloads). A user-supplied file is never deleted.

## HoloPatcher and the Visual C++ runtime ship inside the installer

- **HoloPatcher** (`Resources/HoloPatcher.exe`, GPL-3.0, run as a separate process) drives every TSLPatcher-style mod. It is bundled because the player cannot fetch it by hand and nothing installs without it.
- **`msvcp140.dll` and `vcruntime140.dll`** go to the game folder, unconditionally: `accessibility.dll` and `KotorPatcher.dll` import them, and an older system runtime crashes on the mod's first global mutex. The game folder is searched before System32 for the executable's imports, so the copies there win. Re-copy both from the MSVC redistributable folder when the build toolset moves past them.

## Building

- `dotnet build installer/KotorAccessibilityInstaller/KotorAccessibilityInstaller.csproj -c Release` for a quick check (the only warnings are in the vendored KPatchCore).
- The published EXE: `dotnet publish … -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -p:EnableCompressionInSingleFile=true`, as `build.yml` does.
- The files in `Resources/` are checked in by hand; nothing refreshes them automatically.
