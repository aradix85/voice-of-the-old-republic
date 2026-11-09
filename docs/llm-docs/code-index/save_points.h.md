# save_points.h (40 lines)

Source: `patches/Accessibility/save_points.h`

Spots on the floor plan as the exit memory keeps them in save variables: whole metres, "x,y" or "x,y=value", joined by ';'. ACC_VISIT_<area> holds the centres of visited regions, ACC_EXIT_<area> door spots with the module each door leads to. Engine-free, so the unit tests cover the format.

## Declarations (in source order)

- L16 `struct Spot`
- L23 `Spot At(const Vector& p, const std::string& value = std::string())`: `p` rounded to whole metres, with `value`.
- L27 `std::vector<Spot> Parse(const char* text)`: The spots in `text`, up to the first malformed entry.
- L30 `std::string Format(const std::vector<Spot>& spots)`: The text Parse reads back.
- L33 `const Spot* Nearest(const std::vector<Spot>& spots, const Vector& p, float maxM)`: The spot nearest `p` within `maxM` metres, or null.
- L37 `void Put(std::vector<Spot>& spots, const Spot& s, long sameM, size_t max)`: Puts `s` last: drops the spots within `sameM` metres of it, then the oldest ones beyond `max` spots in all.
