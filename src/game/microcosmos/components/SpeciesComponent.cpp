#include "SpeciesComponent.h"

#include <iostream>
#include <utility>

#include <AudioManager.h>
#include <Logger.h>
#include <Random.h>
#include <Services.h>

#include "MicrocosmWorld.h"
#include "OrganismComponent.h"

SpeciesComponent::SpeciesComponent(std::string name, std::string genus, std::string epithet) :
    name(std::move(name)), genus(std::move(genus)), epithet(std::move(epithet)) {}

std::string SpeciesComponent::get_name_formatted(const bool identifier) const {
    std::string result = genus + " " + epithet;
    if (identifier)
        result += " (" + name + ")";
    return result;
}
