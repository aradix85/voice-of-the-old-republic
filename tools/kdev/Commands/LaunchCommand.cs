using System.CommandLine;
using System.CommandLine.Invocation;
using System.Diagnostics;

namespace Kdev.Commands;

public static class LaunchCommand
{
    public static Command Build()
    {
        var monitor = new Option<bool>(
            name: "--monitor",
            description: "Block until the game exits and report its exit code.");

        var gameOption = KdevConfig.GameOption();
        var cmd = new Command("launch", "Launch the game directly — the dinput8.dll proxy auto-loads the patcher.");
        cmd.AddOption(monitor);
        cmd.AddOption(gameOption);
        cmd.SetHandler((InvocationContext context) =>
        {
            var monitorValue = context.ParseResult.GetValueForOption(monitor);
            context.ExitCode = Run(monitorValue, context.ParseResult.GetValueForOption(gameOption)!);
        });
        return cmd;
    }

    internal static int Run(bool monitor, string game = "k1")
    {
        var config = KdevConfig.LoadOrPrintErrors(out var exitCode);
        if (config is null) return exitCode;

        var target = config.ResolveTarget(game);
        if (target is null) return 2;

        // The proxy loader is what brings KotorPatcher into the process on
        // launch. Without it the game runs unmodded and the dev cycle
        // silently produces a vanilla session — the same death path that
        // KPatchLauncher used to mask by injecting at runtime. Surface it
        // explicitly so 'kdev launch' fails loudly instead of misleading.
        var proxyDll = Path.Combine(target.Install, "dinput8.dll");
        if (!File.Exists(proxyDll))
        {
            Console.Error.WriteLine($"ERROR: dinput8.dll proxy not installed at {proxyDll}");
            Console.Error.WriteLine("       The game would launch unmodded. Run 'kdev apply' first.");
            return 6;
        }

        Console.WriteLine($"Launching {target.Exe}...");

        var psi = new ProcessStartInfo
        {
            FileName = target.Exe,
            WorkingDirectory = target.Install,
            UseShellExecute = true,
        };

        Process proc;
        try
        {
            proc = Process.Start(psi)
                ?? throw new InvalidOperationException("Process.Start returned null");
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"ERROR: failed to launch {target.Exe}: {ex.Message}");
            return 6;
        }

        Console.WriteLine($"Started PID {proc.Id}.");

        // Pin to a single logical core. KOTOR's main loop Sleep(0)-spins and
        // otherwise eats one core at 100% AND fights with itself across cores
        // on modern CPUs; pinning drops power draw and keeps fans quiet.
        // We pin twice because the engine (or a library it loads during init)
        // sometimes resets affinity a few seconds after launch.
        if (OperatingSystem.IsWindows())
        {
            var mask = (IntPtr)1;
            TryPin(proc.Id, mask, "initial");
            Thread.Sleep(TimeSpan.FromSeconds(5));
            TryPin(proc.Id, mask, "after settling delay");
        }

        if (!monitor) return 0;

        Console.WriteLine();
        Console.WriteLine($"Monitoring game (PID {proc.Id}). Ctrl+C stops monitoring (game keeps running).");
        try
        {
            proc.WaitForExit();
            Console.WriteLine($"Game exited with code: {proc.ExitCode}");
            return proc.ExitCode;
        }
        catch (ArgumentException)
        {
            Console.WriteLine("Game process no longer exists.");
            return 0;
        }
    }

    // The IsWindows() guard below looks redundant — both call sites are already
    // inside Run's if (OperatingSystem.IsWindows()) block, and kdev.csproj pins
    // RuntimeIdentifier=win-x64. It is NOT removable: CA1416 does not propagate
    // a caller's platform guard across a method boundary, so deleting it fails
    // the build on Process.ProcessorAffinity below. Leave it.
    private static void TryPin(int pid, IntPtr mask, string label)
    {
        if (!OperatingSystem.IsWindows()) return;
        try
        {
            var p = Process.GetProcessById(pid);
            p.ProcessorAffinity = mask;
            Console.WriteLine($"Pinned PID {pid} ({label}).");
        }
        catch (ArgumentException)
        {
            Console.WriteLine($"Game process {pid} no longer exists ({label}).");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"WARNING: could not pin PID {pid} ({label}): {ex.Message}");
        }
    }
}
