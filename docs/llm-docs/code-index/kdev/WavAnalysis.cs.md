# WavAnalysis.cs (854 lines)

Source: `tools/kdev/Core/WavAnalysis.cs`

## Declarations (in source order)

- L18 `internal static class WavAnalysis`: WAV reader + short-time FFT + acoustic feature extraction.
- L23 `public static SoundScore? Score(string path)`
- L168 `public static int DescribeLoopability(string path)`: Single-file loopability inspector.
- L345 `public static int MakeLoop(string inPath, string outPath, double startSec,`: Build a seamless continuous-loop WAV.
- L452 `public static int RepeatWav(string inPath, string outPath, int reps)`: Concatenate a WAV back-to-back N times.
- L489 `private static void FlattenRegion(double[] x, int from, int to, int sampleRate)`: Flatten the amplitude envelope of x over [from, to) to constant level by dividing out a smoothed (30 ms) RMS envelope.
- L543 `private static void WriteWavMono16(string path, double[] mono, int sampleRate)`
- L572 `private static double SliceCentroid(double[] x, int center, int sampleRate)`
- L593 `private static double Dbfs(double lin)`
- L595 `private static double ZcrHz(double[] x, int a, int b, int sampleRate)`
- L605 `private static int NextZeroCrossing(double[] x, int from, int limit)`
- L612 `private static int PrevZeroCrossing(double[] x, int from, int lowLimit)`
- L619 `private static double OnsetRate(List<double> flux, double seconds)`
- L639 `private static double[] HannWindow(int n)`
- L647 `private static double Clamp01(double v)`
- L648 `private static double Clamp(double v, double lo, double hi)`
- L652 `private static void Fft(double[] re, double[] im)`: --- Iterative radix-2 Cooley-Tukey FFT (in place).
- L770 `private static double ReadSample(byte[] b, int i, int bits, int format)`
- L816 `private static double[] DecodeImaAdpcmMono(byte[] b, int dataOffset, int dataLength, int blockAlign)`
