#pragma once

#include <ncore/kernel/structures.h>
#include <ncore/runtime/ecs_system.h>

#include <microcosmos/components/OrganismAIComponent.h>

// namespace nc {
// class EcsWorld;
// }
//
// struct OrganismComponent;
//
// class OrganismAISystem : public nc::EcsSystem {
//     NCLASS(OrganismAISystem, nc::EcsSystem)
//
// public:
//     OrganismAISystem() { set_priority(55); }
//
//     void on_init(nc::EcsWorld &world) override;
//     void on_process(nc::EcsWorld &world, double delta_time) override;
//
// private:
//     nc::Vec2 get_rand_dir();
//
//     void change_state(OrganismAIComponent &ai, BehaviourState newState);
//
//     void update_idling(OrganismAIComponent &ai, double delta);
//     void update_run_and_tumble(nc::EcsEntity &entity, OrganismAIComponent &ai, OrganismComponent &organism,
//                                nc::EcsWorld &world, double delta);
//     void update_absorbing(nc::EcsWorld &world, nc::EcsEntity &entity, OrganismAIComponent &ai,
//                           OrganismComponent &organism, double delta);
//     void update_eval(nc::EcsEntity &entity, OrganismAIComponent &ai, OrganismComponent &organism,
//                      nc::EcsWorld &world, double	 delta);
//
//     void absorb_food(nc::EcsWorld &world, nc::EcsEntity &entity, OrganismAIComponent &ai,
//                      OrganismComponent &organism);
//     void reproduce(nc::EcsWorld &world, nc::EcsEntity &entity, OrganismAIComponent &ai,
//                    OrganismComponent &organism);
// };
