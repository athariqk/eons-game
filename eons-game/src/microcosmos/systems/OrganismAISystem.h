#pragma once

#include <ncore/kernel/structures.h>
#include <ncore/modules/ecs/ecs_system.h>

#include <microcosmos/components/OrganismAIComponent.h>

// namespace ncore {
// class EcsWorld;
// }
//
// struct OrganismComponent;
//
// class OrganismAISystem : public ncore::EcsSystem {
//     NCLASS(OrganismAISystem, ncore::EcsSystem)
//
// public:
//     OrganismAISystem() { set_priority(55); }
//
//     void on_init(ncore::EcsWorld &world) override;
//     void on_process(ncore::EcsWorld &world, double delta_time) override;
//
// private:
//     ncore::Vec2 get_rand_dir();
//
//     void change_state(OrganismAIComponent &ai, BehaviourState newState);
//
//     void update_idling(OrganismAIComponent &ai, double delta);
//     void update_run_and_tumble(ncore::EcsEntity &entity, OrganismAIComponent &ai, OrganismComponent &organism,
//                                ncore::EcsWorld &world, double delta);
//     void update_absorbing(ncore::EcsWorld &world, ncore::EcsEntity &entity, OrganismAIComponent &ai,
//                           OrganismComponent &organism, double delta);
//     void update_eval(ncore::EcsEntity &entity, OrganismAIComponent &ai, OrganismComponent &organism,
//                      ncore::EcsWorld &world, double	 delta);
//
//     void absorb_food(ncore::EcsWorld &world, ncore::EcsEntity &entity, OrganismAIComponent &ai,
//                      OrganismComponent &organism);
//     void reproduce(ncore::EcsWorld &world, ncore::EcsEntity &entity, OrganismAIComponent &ai,
//                    OrganismComponent &organism);
// };
