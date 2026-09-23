#pragma once
#include <string>
#include <vector>

namespace io {

struct OpeningInfo {
    std::string eco;
    std::string name;
    std::string variation;
    std::string moves;
};

// Given a sequence of SAN moves (without move numbers), return the deepest known opening.
OpeningInfo detect_opening(const std::vector<std::string>& san_moves);

} // namespace io