#pragma once

#include <ncore/kernel/random.h>
#include <ncore/modules/ecs/ecs_world.h>
#include <ncore/modules/service_locator.h>
#include <ncore/modules/video/render_service.h>

#include <microcosmos/SpeciesRegistry.h>
#include <microcosmos/components/OrganismAIComponent.h>
#include <microcosmos/components/OrganismComponent.h>
#include <microcosmos/components/SpeciesComponent.h>

namespace OrganismFactory {

inline OrganismComponent& create(ncore::EcsWorld& world, SpeciesRegistry& reg, SpeciesComponent* species)
{
    auto instance = world.create_entity();

    float spawn_x = 0.0f;
    float spawn_y = 0.0f;

    // auto renderer = world.get_services().resolve<ncore::IRenderService>();
    //  if (renderer) {
    //      auto camera = renderer->get_main_camera().lock();
    //      if (camera) {
    //          auto cam_pos = camera->get_position();
    //          spawn_x = cam_pos.x + ncore::Random::rand_float(-3.0f, 3.0f);
    //          spawn_y = cam_pos.y + ncore::Random::rand_float(-3.0f, 3.0f);
    //      }
    //  }

    // auto &transform = world.emplace<ncore::TransformComponent>(instance, ncore::Vec2(spawn_x, spawn_y), 0.0f,
    //                                                            ncore::Vec2(species->genes.size,
    //                                                            species->genes.size));
    // world.emplace<ncore::RigidbodyComponent>(instance);

    // auto &organism = world.emplace<OrganismComponent>(instance, species);
    // organism.entity = &instance;
    // organism.species_id = species->entity->id;
    // organism.cur_energy = organism.genome.energy_capacity;
    // organism.fitness = 0;
    // reg.track_organism(organism);
    // species->population_count++;

    // auto &ai = world.emplace<OrganismAIComponent>(instance, organism.genome.speed, 1.5f);
    // ai.entity = &instance;

    // auto &circle = world.emplace<ncore::TempCircleComponent>(instance);
    // circle.radius = organism.genome.size / 2.0f;
    // circle.color = organism.genome.membrane_color;
    // circle.filled = true;
    // circle.edge = false;

    // return organism;
    OrganismComponent organism{};
    return organism;
}

} // namespace OrganismFactory
