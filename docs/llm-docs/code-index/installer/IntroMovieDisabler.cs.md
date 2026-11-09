# IntroMovieDisabler.cs (180 lines)

Source: `installer/KotorAccessibilityInstaller/IntroMovieDisabler.cs`

## Declarations (in source order)

- L33 `public static class IntroMovieDisabler`: Rename the launch-time intro .bik files aside (the per-game list is ) so the engine's PlayMoviesAsync queue fails to open them and falls through to the main menu.
- L38 `public sealed class Result`
- L53 `public static Result DisableIntros(GameTarget target, string gameDir)`: Rename each intro file to &lt;name&gt;.bik.disabled .
- L62 `public static Result RestoreIntros(GameTarget target, string gameDir)`: Reverse .
- L83 `private static Result MoveIntros(GameTarget target, string gameDir, bool disable)`: The rename, in whichever direction.
