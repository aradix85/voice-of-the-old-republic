using System.CommandLine;
using System.CommandLine.Invocation;

namespace Kdev.Commands;

public static class StatusCommand
{
    public static Command Build()
    {
        var cmd = new Command("status", "Validate config, paths, and dev environment health.");
        cmd.SetHandler((InvocationContext context) => context.ExitCode = Run());
        return cmd;
    }

    // A game that is not installed is a fact, not a problem: only the game a
    // command acts on has to exist (KdevConfig.ResolveTarget).
    private static string Installed(string? install, string? exe) =>
        install is null ? "(not configured)"
        : File.Exists(exe) ? install
        : $"{install} (not installed)";

    internal static int Run()
    {
        KdevConfig config;
        try
        {
            config = KdevConfig.Load();
        }
        catch (KdevConfigException ex)
        {
            Console.Error.WriteLine($"ERROR: {ex.Message}");
            return 2;
        }

        Console.WriteLine("kdev status");
        Console.WriteLine("===========");
        Console.WriteLine();
        Console.WriteLine("Resolved configuration:");
        Console.WriteLine($"  game.install         = {Installed(config.GameInstall, config.GameExe)}");
        Console.WriteLine($"  game.install_k2      = {Installed(config.GameInstallK2, config.GameExeK2)}");
        Console.WriteLine($"  patch_manager.release= {config.PatchManagerRelease}");
        Console.WriteLine($"  upstream.clone       = {config.UpstreamClone}");
        Console.WriteLine($"  accessibility.source = {config.AccessibilitySource}");
        Console.WriteLine($"  accessibility.build  = {config.BuildOutput}");
        Console.WriteLine($"  accessibility.id     = {config.PatchId}");
        Console.WriteLine();

        var problems = config.Validate();
        if (problems.Count == 0)
        {
            Console.WriteLine("All configured paths exist. No problems detected.");
            return 0;
        }

        Console.Error.WriteLine($"Found {problems.Count} problem(s):");
        for (var i = 0; i < problems.Count; i++)
        {
            Console.Error.WriteLine($"  {i + 1}. {problems[i]}");
        }
        return 3;
    }
}
