// ============================================================================
// c90.hpp - native (C++) Erweiterung des C-Subsets.
// siehe c90.cpp
// ============================================================================
#pragma once
#include <string>

namespace c90 {
// Wandelt erweiterte C-Syntax in das Kern-Subset um. Ist keine Erweiterung
// enthalten, wird der Quelltext unveraendert zurueckgegeben (Paritaet).
std::string maybe_expand(const std::string& src);
}  // namespace c90
