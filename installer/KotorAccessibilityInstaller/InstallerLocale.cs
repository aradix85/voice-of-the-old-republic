using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Text;
using System.Text.Json;

namespace KotorAccessibilityInstaller
{
    /// <summary>
    /// The installer's texts: the flat JSON file Locales/en.json, embedded as a
    /// resource. A missing key reads as the key name.
    /// </summary>
    public static class InstallerLocale
    {
        private static Dictionary<string, string> _strings = new Dictionary<string, string>();

        public static void Initialize()
        {
            _strings = LoadEmbedded("locale.en.json");
            Logger.Info($"[InstallerLocale] Loaded {_strings.Count} strings");
        }

        public static string Get(string key) =>
            _strings.TryGetValue(key, out string val) ? val : key;

        /// <summary>
        /// Convert the texts' <c>\n</c> line breaks to <c>\r\n</c>.
        ///
        /// <para>Required for any multiline <see cref="System.Windows.Forms.TextBox"/>:
        /// it only breaks lines on <c>\r\n</c> and renders a lone <c>\n</c> as
        /// nothing, so a bulleted list collapses onto one line. MessageBox and
        /// Label both handle <c>\n</c> fine.</para>
        ///
        /// <para>Idempotent: an existing <c>\r\n</c> is not doubled.</para>
        /// </summary>
        public static string NormalizeLineBreaks(string text) =>
            text == null ? null : text.Replace("\r\n", "\n").Replace("\n", "\r\n");

        public static string Format(string key, params object[] args)
        {
            string template = Get(key);
            try { return string.Format(template, args); }
            catch (FormatException) { return template; }
        }

        private static Dictionary<string, string> LoadEmbedded(string resourceName)
        {
            try
            {
                using var stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(resourceName);
                if (stream == null)
                {
                    Logger.Warning($"[InstallerLocale] Resource not found: {resourceName}");
                    return new Dictionary<string, string>();
                }
                using var reader = new StreamReader(stream, Encoding.UTF8);
                return JsonSerializer.Deserialize<Dictionary<string, string>>(reader.ReadToEnd())
                       ?? new Dictionary<string, string>();
            }
            catch (Exception ex)
            {
                Logger.Warning($"[InstallerLocale] Error loading {resourceName}: {ex.Message}");
                return new Dictionary<string, string>();
            }
        }
    }
}
