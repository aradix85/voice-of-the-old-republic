# save_points.cpp (85 lines)

Source: `patches/Accessibility/save_points.cpp`

(From save_points.h.) Spots on the floor plan as the exit memory keeps them in save variables: whole metres, "x,y" or "x,y=value", joined by ';'. ACC_VISIT_<area> holds the centres of visited regions, ACC_EXIT_<area> door spots with the module each door leads to. Engine-free, so the unit tests cover the format.

## Declarations (in source order)

- L9 `Spot At(const Vector& p, const std::string& value)`
- L17 `std::vector<Spot> Parse(const char* text)`
- L45 `std::string Format(const std::vector<Spot>& spots)`
- L55 `const Spot* Nearest(const std::vector<Spot>& spots, const Vector& p, float maxM)`
- L69 `void Put(std::vector<Spot>& spots, const Spot& s, long sameM, size_t max)`
