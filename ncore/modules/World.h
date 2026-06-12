#pragma once

#include <algorithm>
#include <array>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <modules/ecs/Entity.h>
#include <modules/ecs/System.h>

namespace ncore {

class MainLoop;
class Gui;
class IGraphicsContext;

/**
 * @brief The World class manages entities and systems, driven by the MainLoop.
 * It serves as the "core" of the ECS architecture
 * and context bridge between the engine and the game.
 */
class World {
public:
    World() = default;
    virtual ~World() = default;

    // Internal, to be called by MainLoop
    void on_init_();
    void on_fixed_update_(double p_delta, uint64_t ticks);
    void on_variable_update_(double p_delta);
    void on_post_update_(double p_delta);
    void on_render_(IGraphicsContext &p_graphics);
    void on_gui_render_();
    void on_finish_();

    template<std::derived_from<System> T, typename... TArgs>
    T &add_system(TArgs &&...args) {
        auto system = std::make_unique<T>(std::forward<TArgs>(args)...);
        T *ptr = system.get();
        systems_reg.emplace_back(std::move(system));

        // Sort systems by priority (lower priority = earlier execution)
        std::sort(systems_reg.begin(), systems_reg.end(),
                  [](const auto &a, const auto &b) { return a->get_priority() < b->get_priority(); });

        return *ptr;
    }

    template<typename T>
    T *get_system() {
        for (auto &system: systems_reg) {
            if (auto *ptr = dynamic_cast<T *>(system.get())) {
                return ptr;
            }
        }
        return nullptr;
    }

    // Access to the main loop
    void set_main_loop(MainLoop &p_main_loop) { main_loop = &p_main_loop; }
    MainLoop &get_main_loop() {
        NC_ASSERT(main_loop != nullptr, "MainLoop not set in World!");
        return *main_loop;
    }

    Entity &create_entity();
    void add_to_group(Entity *mEntity, Group mGroup);
    std::vector<Entity *> &get_group(Group mGroup);
    inline const auto &get_entities() const { return entities; }
    Entity *get_entity_by_id(EntityID p_id);

public:
    // Custom overridables
    virtual void on_init() = 0;
    virtual void on_update(double delta) = 0;
    virtual void on_render(IGraphicsContext &graphics) = 0;
    virtual void on_gui_render() = 0;
    virtual void on_finish() = 0;

private:
    std::vector<std::unique_ptr<Entity>> entities;
    std::unordered_map<EntityID, Entity *> entity_id_reg;
    std::array<std::vector<Entity *>, max_groups> grouped_entities_reg;
    std::vector<std::unique_ptr<System>> systems_reg;
    MainLoop *main_loop = nullptr;
};

} // namespace ncore
