# speech_clipboard.cpp (176 lines)

Source: `patches/Accessibility/speech_clipboard.cpp`

user32.lib for the clipboard API. The patch build links only sqlite3 by default; pragma keeps the link config local to the TU that needs it.

## Declarations (in source order)

- L53 `void SpeakFeedback(S id)`: Speak mod feedback without letting it become the thing Ctrl+R copies next time.
- L63 `bool CopyToClipboard(const char* text)`: Publish `text` (game codepage) as CF_UNICODETEXT.
- L113 `DWORD WINAPI RunCopy(LPVOID param)`: Worker entry.
- L128 `void PollWin32()`
