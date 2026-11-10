using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Principal;
using System.Text;
using System.Windows.Forms;
using Microsoft.Win32;

namespace KotorAccessibilityInstaller
{
    // Install-flow orchestration.
    //
    // Split out of Program.cs by the Phase-1 structure pass (refactoring
    // candidate 17). Program.Main dispatches a mode; these drive it -
    // the KOTOR 1 full install, the KOTOR 2 preparation and TSLRCM
    // sub-flows, and the standalone maintenance actions (log collection,
    // spatial-audio toggle).
    static class InstallFlow
    {
        internal static void RunFullInstallFlow(string gamePath, string pathArgOverride, string latestVersion, string localKpatchPath)
        {
            var welcomeForm = new WelcomeForm { LatestModVersion = latestVersion };
            Application.Run(welcomeForm);
            if (!welcomeForm.ProceedWithInstall)
            {
                Logger.Info("Installation cancelled from welcome dialog");
                return;
            }

            // Which game(s) to install into. Both are fully supported; each gets
            // its own MainForm pass at the end of this method.
            var gameVersionForm = new GameVersionSelectionForm();
            Application.Run(gameVersionForm);
            if (!gameVersionForm.ProceedWithInstall)
            {
                Logger.Info("Installation cancelled from game-version selection");
                return;
            }

            // Resolve each selected game's folder up front. Detection knows the
            // Steam and GOG registry entries; when it still comes up empty the
            // user gets a folder picker BEFORE we conclude the game is absent —
            // an unregistered GOG or manually moved copy is only findable by
            // its owner, and treating it as "not installed" sent a GOG user
            // through a buy-the-game screen into an installer that then closed
            // on them.
            string k1Path = null;
            string k2Path = null;
            var missingGames = new List<GameTarget>();
            if (gameVersionForm.Selection.Kotor1)
            {
                k1Path = pathArgOverride ?? GamePathDetector.Detect(GameTarget.Kotor1)
                         ?? PromptForGamePath(GameTarget.Kotor1);
                if (k1Path == null) missingGames.Add(GameTarget.Kotor1);
            }
            if (gameVersionForm.Selection.Kotor2)
            {
                k2Path = GamePathDetector.Detect(GameTarget.Kotor2)
                         ?? PromptForGamePath(GameTarget.Kotor2);
                if (k2Path == null) missingGames.Add(GameTarget.Kotor2);
            }

            // Offer store links for whatever is genuinely missing — and skip
            // the screen entirely when everything is present. Deliberately
            // after the selection: a "where to buy" page shown before the user
            // has chosen a game can only guess which game to advertise, and
            // the old one always guessed KOTOR 1.
            if (!GameStoreLinksForm.ShowIfNeeded(missingGames))
            {
                Logger.Info("Installation cancelled from the game-store-links screen");
                return;
            }

            // The store-links screen invites installing the game before
            // pressing Continue — honour that by re-checking what was missing.
            if (missingGames.Contains(GameTarget.Kotor1))
                k1Path = GamePathDetector.Detect(GameTarget.Kotor1);
            if (missingGames.Contains(GameTarget.Kotor2))
                k2Path = GamePathDetector.Detect(GameTarget.Kotor2);

            if (gameVersionForm.Selection.Kotor2)
            {
                Logger.Info($"KOTOR 2 selected; install: {k2Path ?? "(not found)"}");

                // KOTOR 2's game-content mods run BEFORE our own install, not
                // after. TSLRCM is a third-party Inno installer that writes
                // freely into the game folder (and replaces dialog.tlk
                // outright), so anything of ours already sitting there would be
                // at its mercy. KOTOR 1's optional mods have the same ordering,
                // just expressed differently: they run inside MainForm, but
                // still after nothing of ours is at risk — they are
                // HoloPatcher payloads that only touch game data.
                RunKotor2ModFlow(k2Path);
            }

            string resolvedK1Path = null;
            ModSelection k1Mods = null;

            if (gameVersionForm.Selection.Kotor1)
            {
                // Base-components info: accessibility mod + NVDA controller client + widescreen (always installed).
                var infoForm = new ModdingInfoForm();
                Application.Run(infoForm);
                if (!infoForm.ProceedWithInstall)
                {
                    Logger.Info("Installation cancelled from base-components screen");
                    return;
                }

                // Never null: MainForm always opens for a selected KOTOR 1,
                // with its editable path box and Browse button — a wrong
                // prefill just leaves Install disabled until corrected.
                resolvedK1Path = k1Path ?? gamePath ?? GamePathDetector.DefaultGamePath;

                // Optional mods: K1CP / cut content / companion + swoop. All default on.
                var selectionForm = new ModSelectionForm();
                Application.Run(selectionForm);
                if (!selectionForm.ProceedWithInstall)
                {
                    Logger.Info("Installation cancelled from mod-selection screen");
                    return;
                }
                k1Mods = selectionForm.Selection;
            }

            // Make Windows Error Reporting capture minidumps under
            // %LOCALAPPDATA%\CrashDumps next time either game faults. Without
            // this, beta testers' "collect logs" zips contain no crash dump.
            // Idempotent + best-effort; failure does not block install.
            WerLocalDumps.Enable();

            // One MainForm pass per selected game. Sequential rather than a
            // single multi-target form: each pass has its own path to validate,
            // its own progress to announce and its own completion summary, and
            // a screen-reader user needs those separated rather than interleaved.
            // The cost is one extra .kpatch download when both are selected.
            if (resolvedK1Path != null)
            {
                Application.Run(new MainForm(GameTarget.Kotor1, resolvedK1Path,
                    modSelection: k1Mods,
                    localKpatchPath: localKpatchPath));
            }

            if (gameVersionForm.Selection.Kotor2)
            {
                if (k2Path == null)
                {
                    // Nothing to install into. Said plainly rather than silently
                    // skipped — the user explicitly asked for KOTOR 2.
                    MessageBox.Show(
                        InstallerLocale.Get("K2Prep_NotDetected"),
                        InstallerLocale.Get("K2Prep_Title"),
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Warning);
                }
                else
                {
                    Application.Run(new MainForm(GameTarget.Kotor2, k2Path,
                        localKpatchPath: localKpatchPath));
                }
            }
        }

        /// <summary>
        /// Detection came up empty for a selected game: offer a folder picker
        /// before concluding it is absent. This is the road an unregistered GOG
        /// or manually moved copy takes on its first install — the registry
        /// lookups cannot see it, and only its owner knows where it lives.
        /// Returns a validated folder, or null when the user declines.
        /// </summary>
        private static string PromptForGamePath(GameTarget target)
        {
            var offer = MessageBox.Show(
                InstallerLocale.Format("GameNotFound_BrowsePrompt_Format", target.DisplayName),
                target.DisplayName,
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Question);
            if (offer != DialogResult.Yes)
            {
                Logger.Info($"{target.DisplayName} not detected; user declined to browse for it");
                return null;
            }

            while (true)
            {
                using var dialog = new FolderBrowserDialog
                {
                    Description = InstallerLocale.Format("GameNotFound_BrowseDialog_Format",
                                                         target.DisplayName, target.ExeName),
                    ShowNewFolderButton = false
                };

                if (dialog.ShowDialog() != DialogResult.OK)
                {
                    Logger.Info($"{target.DisplayName} folder selection cancelled");
                    return null;
                }

                if (GamePathDetector.IsValidGamePath(target, dialog.SelectedPath))
                {
                    Logger.Info($"{target.DisplayName} folder selected by the user: {dialog.SelectedPath}");
                    return dialog.SelectedPath;
                }

                Logger.Info($"Selected folder has no {target.ExeName}: {dialog.SelectedPath}");
                MessageBox.Show(
                    InstallerLocale.Format("GameNotFound_InvalidFolder_Format",
                                           dialog.SelectedPath, target.ExeName),
                    target.DisplayName,
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning);
            }
        }

        /// <summary>
        /// KOTOR 2's game-content mods, run before our own install (see the
        /// caller for why that order). Sequence: show
        /// <see cref="Kotor2ModSelectionForm"/> (TSLRCM / K2CP / Tweak Pack —
        /// all on by default), install TSLRCM via
        /// <see cref="TslrcmInstallForm"/> (silent Inno; visible wizard as
        /// fallback), then run the K2CP + Tweak Pack pipeline via
        /// <see cref="Kotor2ModsInstallForm"/> — gated on TSLRCM being
        /// installed (this run or detected via <see cref="TslrcmDetector"/>)
        /// so the community-mandated order TSLRCM → K2CP → Tweak Pack always
        /// holds. Ends with one spoken per-mod summary box.
        ///
        /// The engine patches are NOT applied here. They rewrite swkotor2.exe,
        /// so they install in the same KPatchCore transaction as the
        /// accessibility patch during the MainForm pass — applying them in a
        /// transaction of their own would change the executable hash that the
        /// accessibility patch's own version gate then reads.
        /// </summary>
        internal static void RunKotor2ModFlow(string k2Path)
        {
            string statusLine = k2Path != null
                ? InstallerLocale.Format("K2Prep_Detected_Format", k2Path)
                : InstallerLocale.Get("K2Prep_NotDetected");

            var selectionForm = new Kotor2ModSelectionForm(statusLine);
            Application.Run(selectionForm);
            if (!selectionForm.ProceedWithInstall)
            {
                Logger.Info("KOTOR 2 mod selection cancelled");
                return;
            }

            // Subscribed Workshop items override everything we are about to
            // install. Ask before, not after.
            if (k2Path != null && !ConfirmNoWorkshopContent(k2Path)) return;

            var summary = new StringBuilder();
            summary.AppendLine(InstallerLocale.Get("ModInstall_SummaryHeading"));
            const string tslrcmName = "TSLRCM 1.8.6";

            bool installTslrcm = selectionForm.InstallTslrcm;
            bool tslrcmPresent;
            if (installTslrcm)
            {
                // Three signals, strongest first.
                //
                // 1. The silent install verifies ITSELF: TslrcmInstallForm
                //    checks dialog.tlk and, when that is unchanged, Setup's own
                //    log — so a reinstall over an existing TSLRCM reports
                //    success rather than a scary false failure.
                // 2. Otherwise the uninstall entry, which is all we have after
                //    the visible wizard.
                // 3. If neither fires, ASK. The alternative is to conclude
                //    "not installed" from the absence of a signal we are not
                //    sure exists, and then silently skip K2CP and the Tweak
                //    Pack — the failure that leaves a player with a mod set
                //    they believe is complete and is not.
                var outcome = RunTslrcmInstall(k2Path);

                if (outcome == TslrcmOutcome.SilentInstalled)
                {
                    tslrcmPresent = true;
                    Logger.Info("TSLRCM present: verified by the silent install");
                }
                else if (TslrcmDetector.IsInstalled())
                {
                    tslrcmPresent = true;
                }
                else if (outcome == TslrcmOutcome.DownloadedOnly ||
                         outcome == TslrcmOutcome.SilentInstallFailed)
                {
                    // The user was handed the wizard. Whether they completed it
                    // is something only they know.
                    tslrcmPresent = MessageBox.Show(
                        InstallerLocale.Get("K2Tslrcm_ConfirmInstalled_Text"),
                        InstallerLocale.Get("K2Prep_Title"),
                        MessageBoxButtons.YesNo,
                        MessageBoxIcon.Question) == DialogResult.Yes;
                    Logger.Info($"TSLRCM presence answered by the user: {tslrcmPresent}");
                }
                else
                {
                    tslrcmPresent = false;
                }

                summary.AppendLine(tslrcmPresent
                    ? InstallerLocale.Format(
                        _tslrcmWasAlreadyInstalled ? "ModInstall_SummaryAlready_Format"
                                                   : "ModInstall_SummaryOk_Format", tslrcmName)
                    : InstallerLocale.Format("ModInstall_SummaryFailed_Format", tslrcmName, "(not installed)"));
            }
            else
            {
                // The user unticked TSLRCM, which in practice means "it is
                // already there" or "not this time". The registry only knows an
                // installer-made TSLRCM, so on a miss we ask rather than silently
                // skipping the mods that build on it.
                if (TslrcmDetector.IsInstalled())
                {
                    tslrcmPresent = true;
                }
                else
                {
                    int patchedModules = TslrcmDetector.CountPatchedModules(k2Path);
                    Logger.Info($"TSLRCM not in the registry; asking the user " +
                                $"({patchedModules} patched module file(s) found)");

                    tslrcmPresent = MessageBox.Show(
                        InstallerLocale.Format("K2Tslrcm_AskAlreadyInstalled_Format", patchedModules),
                        InstallerLocale.Get("K2Prep_Title"),
                        MessageBoxButtons.YesNo,
                        MessageBoxIcon.Question) == DialogResult.Yes;
                    Logger.Info($"TSLRCM presence answered by the user: {tslrcmPresent}");
                }

                summary.AppendLine(InstallerLocale.Format("ModInstall_SummarySkipped_Format", tslrcmName));
            }

            if (selectionForm.Selection.K2cp || selectionForm.Selection.TweakPack ||
                selectionForm.Selection.ThematicCompanionsK2)
            {
                if (k2Path == null || !tslrcmPresent)
                {
                    // Order gate: without a known install dir, or without
                    // TSLRCM in place, installing K2CP / Tweak Pack / Thematic
                    // Companions now would bake in the wrong order (TSLRCM must
                    // come first, and all three build on it).
                    if (k2Path != null)
                    {
                        MessageBox.Show(
                            InstallerLocale.Get("K2Mods_NoTslrcm_Text"),
                            InstallerLocale.Get("K2Prep_Title"),
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Warning);
                    }
                    Logger.Info($"K2CP/Tweak Pack/Thematic Companions skipped " +
                                $"(k2Path={(k2Path ?? "null")}, tslrcmPresent={tslrcmPresent})");
                    if (selectionForm.Selection.K2cp)
                        summary.AppendLine(InstallerLocale.Format("ModInstall_SummarySkipped_Format", "k2cp"));
                    if (selectionForm.Selection.TweakPack)
                        summary.AppendLine(InstallerLocale.Format("ModInstall_SummarySkipped_Format", "tweakpack"));
                    if (selectionForm.Selection.ThematicCompanionsK2)
                        summary.AppendLine(InstallerLocale.Format(
                            "ModInstall_SummarySkipped_Format", "thematic-companions-k2"));
                }
                else
                {
                    var installForm = new Kotor2ModsInstallForm(k2Path, selectionForm.Selection);
                    Application.Run(installForm);
                    foreach (var r in installForm.Results)
                    {
                        if (r.Skipped)
                            summary.AppendLine(InstallerLocale.Format("ModInstall_SummarySkipped_Format", r.Id));
                        else if (r.Success)
                            summary.AppendLine(InstallerLocale.Format("ModInstall_SummaryOk_Format", r.Id));
                        else
                            summary.AppendLine(InstallerLocale.Format("ModInstall_SummaryFailed_Format", r.Id, r.Error ?? "(no detail)"));
                    }
                }
            }

            // TSLRCM's readme buries this in an English document in the game
            // folder; a one-click install would never surface it. Speak it in
            // the summary: old saves are incompatible, a fresh game is required.
            if (installTslrcm && tslrcmPresent)
            {
                summary.AppendLine();
                summary.AppendLine(InstallerLocale.Get("K2Mods_NewGameHint"));
            }

            MessageBox.Show(
                summary.ToString(),
                InstallerLocale.Get("K2Prep_Title"),
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
        }

        /// <summary>
        /// Set by <see cref="RunTslrcmInstall"/> when Setup reported success
        /// without changing anything, i.e. TSLRCM was already there. Only
        /// affects the wording of the summary line.
        /// </summary>
        private static bool _tslrcmWasAlreadyInstalled;

        /// <summary>
        /// TSLRCM step: silent install when the KOTOR 2 folder is known,
        /// visible wizard as fallback (no folder detected, or the silent run
        /// failed and the user accepts the fallback). Returns the form's
        /// outcome so the caller can tell a self-verified silent install from
        /// a wizard handoff whose result only the user knows.
        /// </summary>
        private static TslrcmOutcome RunTslrcmInstall(string k2Path)
        {
            string statusLine = k2Path != null
                ? InstallerLocale.Format("K2Prep_Detected_Format", k2Path)
                : InstallerLocale.Get("K2Prep_NotDetected");

            var form = new TslrcmInstallForm(k2Path);
            Application.Run(form);
            _tslrcmWasAlreadyInstalled = form.AlreadyInstalled;

            switch (form.Outcome)
            {
                case TslrcmOutcome.SilentInstalled:
                    TryDeleteTempFile(form.InstallerPath);
                    break;

                case TslrcmOutcome.DownloadedOnly:
                    // No KOTOR 2 folder detected — silent /DIR would be a guess,
                    // so let TSLRCM's own wizard do its game detection visibly.
                    RunTslrcmWizardInteractive(form.InstallerPath, statusLine);
                    break;

                case TslrcmOutcome.SilentInstallFailed:
                    var fallback = MessageBox.Show(
                        InstallerLocale.Format("K2Tslrcm_SilentFailed_Format", form.FailureReason),
                        InstallerLocale.Get("K2Prep_Title"),
                        MessageBoxButtons.YesNo,
                        MessageBoxIcon.Warning);
                    if (fallback == DialogResult.Yes)
                        RunTslrcmWizardInteractive(form.InstallerPath, statusLine);
                    else
                        TryDeleteTempFile(form.InstallerPath);
                    break;

                case TslrcmOutcome.VerificationFailed:
                    // The download completed but is not a usable installer
                    // (HTML page, truncation — see ProbeInstallerExe). Say
                    // what was wrong, then let the user fetch it themselves.
                    return OfferManualTslrcm(k2Path, statusLine,
                        InstallerLocale.Format("ManualDownload_Reason_Failed_Format",
                                               form.FailureReason));

                case TslrcmOutcome.DownloadFailed:
                    return OfferManualTslrcm(k2Path, statusLine,
                        InstallerLocale.Format("ManualDownload_Reason_Failed_Format",
                                               form.FailureReason));

                case TslrcmOutcome.Cancelled:
                default:
                    // User cancelled — the manual link is on the selection
                    // form's footnote, so stay quiet.
                    break;
            }

            return form.Outcome;
        }

        /// <summary>
        /// Automatic acquisition of TSLRCM did not work. Hand the job to the
        /// user — open the download page, take the file they fetched — and then
        /// continue the install exactly as if we had downloaded it ourselves.
        ///
        /// <para>This is the path that still works when the pins are years out
        /// of date or DeadlyStream has changed its download flow entirely, and
        /// it is why a stale pin is an inconvenience rather than a dead end.
        /// Returns the outcome of the resumed install, or the original failure
        /// when the user skips.</para>
        /// </summary>
        private static TslrcmOutcome OfferManualTslrcm(string k2Path, string statusLine, string reason)
        {
            string supplied = ManualDownloadForm.Ask(
                "TSLRCM " + SourcePins.Tslrcm.DisplayVersion,
                reason,
                Config.TslrcmDownloadPageUrl,
                Config.TslrcmInstallerFileName,
                ManualDownloadForm.FileKind.Executable,
                Config.TslrcmInstallerSha256);

            if (supplied == null)
            {
                Logger.Info("User skipped the manual TSLRCM download");
                return TslrcmOutcome.DownloadFailed;
            }

            if (k2Path == null)
            {
                // No install dir to point a silent /DIR at — let TSLRCM's own
                // wizard do its game detection, same as the DownloadedOnly path.
                RunTslrcmWizardInteractive(supplied, statusLine, deleteAfter: false);
                return TslrcmOutcome.DownloadedOnly;
            }

            var form = new TslrcmInstallForm(k2Path, suppliedInstallerPath: supplied);
            Application.Run(form);

            if (form.Outcome == TslrcmOutcome.SilentInstallFailed)
            {
                var fallback = MessageBox.Show(
                    InstallerLocale.Format("K2Tslrcm_SilentFailed_Format", form.FailureReason),
                    InstallerLocale.Get("K2Prep_Title"),
                    MessageBoxButtons.YesNo,
                    MessageBoxIcon.Warning);
                if (fallback == DialogResult.Yes)
                    RunTslrcmWizardInteractive(supplied, statusLine, deleteAfter: false);
            }

            // The user's own download is never deleted — it is in their
            // Downloads folder and theirs to keep.
            return form.Outcome;
        }

        /// <summary>
        /// Steam Workshop pre-flight. A subscribed Workshop copy of TSLRCM (or
        /// any other Workshop item) installs into a separate content folder that
        /// the game overlays on top of everything else, which breaks the
        /// directory-installed mods we are about to lay down — the community
        /// builds all require unsubscribing first. We cannot unsubscribe on the
        /// user's behalf, so this warns and lets them choose.
        ///
        /// Detection is deliberately shallow: the existence of any subdirectory
        /// under the KOTOR 2 workshop content path. Reading which items they are
        /// would mean parsing Steam's manifests for no gain — any Workshop
        /// content at all is worth the warning.
        ///
        /// Returns false when the user chose to stop.
        /// </summary>
        private static bool ConfirmNoWorkshopContent(string k2Path)
        {
            string workshopDir = FindWorkshopContentDir(k2Path);
            if (workshopDir == null) return true;

            Logger.Warning($"Steam Workshop content present for KOTOR 2: {workshopDir}");
            var choice = MessageBox.Show(
                InstallerLocale.Format("K2Workshop_Warning_Format", workshopDir),
                InstallerLocale.Get("K2Prep_Title"),
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Warning);

            if (choice != DialogResult.Yes)
            {
                Logger.Info("User stopped at the Steam Workshop warning");
                return false;
            }
            return true;
        }

        /// <summary>
        /// <c>&lt;library&gt;/steamapps/workshop/content/208580</c> if it exists and
        /// holds at least one item, else null. Derived from the game path
        /// (<c>steamapps/common/&lt;game&gt;</c>) so it follows a game installed to a
        /// secondary Steam library rather than assuming the default one.
        /// </summary>
        private static string FindWorkshopContentDir(string k2Path)
        {
            try
            {
                // …/steamapps/common/<game>  ->  …/steamapps
                var common = Directory.GetParent(k2Path);
                var steamapps = common?.Parent;
                if (steamapps == null) return null;

                string workshop = Path.Combine(steamapps.FullName, "workshop", "content",
                                               Config.Kotor2WorkshopAppId);
                if (!Directory.Exists(workshop)) return null;

                return Directory.EnumerateFileSystemEntries(workshop).Any() ? workshop : null;
            }
            catch (Exception ex)
            {
                Logger.Warning($"Could not probe for Workshop content: {ex.Message}");
                return null;
            }
        }

        /// <summary>
        /// Visible-wizard fallback: announce the handoff (so an unannounced
        /// English wizard doesn't steal focus from the screen reader), launch
        /// TSLRCM's Inno wizard, block until it exits, delete the temp exe.
        /// </summary>
        /// <param name="deleteAfter">
        /// False when the exe is the user's own download sitting in their
        /// Downloads folder. Deleting that would be us throwing away a file we
        /// did not create and they may want to keep — or re-use, if they have
        /// to run this again.
        /// </param>
        private static void RunTslrcmWizardInteractive(string installerExe, string statusLine, bool deleteAfter = true)
        {
            MessageBox.Show(
                InstallerLocale.Format("K2Tslrcm_WizardIntro_Format", statusLine),
                InstallerLocale.Get("K2Prep_Title"),
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);

            try
            {
                Logger.Info($"Launching TSLRCM installer: {installerExe}");
                using var proc = Process.Start(new ProcessStartInfo
                {
                    FileName = installerExe,
                    UseShellExecute = true
                });
                proc?.WaitForExit();
                Logger.Info($"TSLRCM installer exited with code {proc?.ExitCode}");
            }
            catch (Exception ex)
            {
                Logger.Error("Could not launch TSLRCM installer", ex);
                MessageBox.Show(
                    InstallerLocale.Format("K2Tslrcm_Failed_Format",
                        ex.Message, Config.TslrcmDownloadPageUrl),
                    InstallerLocale.Get("K2Prep_Title"),
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning);
            }
            finally
            {
                if (deleteAfter) TryDeleteTempFile(installerExe);
            }
        }

        private static void TryDeleteTempFile(string path)
        {
            if (string.IsNullOrEmpty(path)) return;
            try { if (File.Exists(path)) File.Delete(path); }
            catch (Exception ex) { Logger.Warning($"Could not delete temp file {path}: {ex.Message}"); }
        }

        /// <summary>
        /// Build a beta-test archive (.7z, LZMA2; .zip fallback) in the user's
        /// Downloads folder containing the newest patch log and crash dump of
        /// every installed game, the installer log, and a system-info summary.
        /// Opens Explorer with the archive selected so the user can attach it
        /// to a bug report directly.
        ///
        /// <para><paramref name="target"/> no longer decides which game is
        /// collected — <see cref="LogCollector"/> covers them all — but is still
        /// passed so a caller-supplied game path survives detection.</para>
        /// </summary>
        internal static void CollectLogsAndReport(GameTarget target, string gamePath)
        {
            // Make sure WER will actually capture dumps from now on, even if
            // nothing was set up earlier in the install. Idempotent.
            WerLocalDumps.Enable();

            var result = LogCollector.Collect(target, gamePath);
            if (!result.Success)
            {
                MessageBox.Show(
                    InstallerLocale.Format("CollectLogs_Error_Format", result.Error ?? "(unknown)"),
                    InstallerLocale.Get("CollectLogs_Error_Title"),
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return;
            }

            string message = InstallerLocale.Format(
                "CollectLogs_Success_Format",
                result.ArchivePath,
                result.LogCount,
                result.DumpCount);
            MessageBox.Show(
                message,
                InstallerLocale.Get("CollectLogs_Success_Title"),
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);

            LogCollector.RevealInExplorer(result.ArchivePath);
        }

        /// <summary>
        /// Flip dsoal on or off based on its current state, then show a
        /// MessageBox so the screen reader announces the new state and reminds
        /// the user to restart the game.
        /// </summary>
        internal static void ToggleSpatialAudioAndReport(string gamePath)
        {
            bool wasEnabled = SpatialAudioManager.IsEnabled(gamePath);
            var result = wasEnabled
                ? SpatialAudioManager.Disable(gamePath)
                : SpatialAudioManager.Enable(gamePath);

            if (!result.Success)
            {
                MessageBox.Show(
                    InstallerLocale.Format("SpatialAudio_Error_Format", result.Error ?? "(unknown)"),
                    InstallerLocale.Get("SpatialAudio_Error_Title"),
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return;
            }

            string messageKey = result.NowEnabled
                ? "SpatialAudio_EnabledMessage"
                : "SpatialAudio_DisabledMessage";
            MessageBox.Show(
                InstallerLocale.Get(messageKey),
                InstallerLocale.Get("SpatialAudio_Result_Title"),
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
        }

    }
}
