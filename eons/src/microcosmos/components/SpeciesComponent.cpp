#include "SpeciesComponent.h"

#include <utility>

std::string SpeciesComponent::get_name_formatted(const bool identifier) const {
    std::string result = genus + " " + epithet;
    if (identifier)
        result += " (" + name + ")";
    return result;
}
