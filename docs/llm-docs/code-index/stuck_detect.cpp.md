# stuck_detect.cpp (136 lines)

Source: `patches/Accessibility/stuck_detect.cpp`

Movement verdicts — see stuck_detect.h.

## Declarations (in source order)

- L29 `const StuckWindow::Sample* StuckWindow::Reference(uint64_t now) const`: The newest sample at least kWindowMs old, so the judged span stays near the window instead of growing with the buffer; the oldest when none is that old yet.
- L39 `StuckWindow::Step StuckWindow::Push(const Vector& pos, uint64_t ms)`
- L77 `bool NoProgressTrigger::Update(const Vector& pos, uint64_t ms, bool walking)`
- L100 `bool CirclingTrigger::Update(const Vector& pos, uint64_t ms, bool walking)`
