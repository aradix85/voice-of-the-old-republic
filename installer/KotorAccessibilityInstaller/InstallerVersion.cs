using System;
using System.Reflection;

namespace KotorAccessibilityInstaller
{
    /// <summary>
    /// The installer's own version, read off the assembly. Three components at
    /// minimum ("0.8.0", not "0.8"); a fourth only when it is not zero.
    /// </summary>
    public static class InstallerVersion
    {
        public static string Number()
        {
            var v = Assembly.GetExecutingAssembly().GetName().Version ?? new Version(0, 0, 0);
            string number = $"{Math.Max(0, v.Major)}.{Math.Max(0, v.Minor)}.{Math.Max(0, v.Build)}";
            if (v.Revision > 0) number += $".{v.Revision}";
            return number;
        }
    }
}
