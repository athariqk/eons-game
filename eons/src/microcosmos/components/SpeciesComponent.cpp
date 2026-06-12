#include "SpeciesComponent.h"

#include <iostream>
#include <utility>

#include <modules/Services.h>
#include <modules/audio/AudioManager.h>
#include <utils/Random.h>

#include <microcosmos/MicrocosmWorld.h>
#include <microcosmos/components/OrganismComponent.h>

SpeciesComponent::SpeciesComponent(std::string name, std::string genus, std::string epithet) :
    name(std::move(name)), genus(std::move(genus)), epithet(std::move(epithet)) {}

std::string SpeciesComponent::get_name_formatted(const bool identifier) const {
    std::string result = genus + " " + epithet;
    if (identifier)
        result += " (" + name + ")";
    return result;
}
