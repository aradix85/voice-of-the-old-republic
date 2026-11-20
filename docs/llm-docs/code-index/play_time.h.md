# play_time.h (38 lines)

Source: `patches/Accessibility/play_time.h`

The play time in a save row's text. Engine-free.

## Declarations (in source order)

- L12 `inline int PlayMinutes(const char* text)`: The minutes of the first " h m" in `text` ("QUICKSAVE - 80h 24m"), or -1 when there is none.
