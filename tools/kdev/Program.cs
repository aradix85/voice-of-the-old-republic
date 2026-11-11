using System.CommandLine;
using System.Globalization;
using System.Text;
using Kdev.Commands;

namespace Kdev;

internal static class Program
{
    public static async Task<int> Main(string[] args)
    {
        // Force invariant culture so command-line numeric arguments and
        // numeric output use "." as decimal separator regardless of the
        // host's regional setting (German `,` would otherwise reparse
        // `0.5` as `5`).
        CultureInfo.DefaultThreadCurrentCulture = CultureInfo.InvariantCulture;
        Thread.CurrentThread.CurrentCulture = CultureInfo.InvariantCulture;

        var root = new RootCommand("Dev CLI for the Voice of the Old Republic accessibility patch.")
        {
            StatusCommand.Build(),
            BuildCommand.Build(),
            CleanCommand.Build(),
            ApplyCommand.Build(),
            LaunchCommand.Build(),
            DevCommand.Build(),
            KillCommand.Build(),
            LogsCommand.Build(),
            AnalyzeDumpCommand.Build(),
            StripDumpCommand.Build(),
            WalkmeshStatsCommand.Build(),
            WalkmeshFaceTypesCommand.Build(),
            WalkmeshGeometryAuditCommand.Build(),
            SoundScoreCommand.Build(),
            ReCommand.Build(),
            DumpTextCommand.Build(),
        };

        return await root.InvokeAsync(args);
    }
}
