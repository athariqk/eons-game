#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <memory>
#include <vector>

/* Source: https://github.com/carlbirch/BirchEngine */

class Component;
class Entity;
class EntityManager;
class Scene;

using ComponentID = std::size_t;
using Group = std::size_t;

inline ComponentID getNewComponentTypeID() {
    static ComponentID lastID = 0u;
    return lastID++;
}

template<typename T>
inline ComponentID getComponentTypeID() noexcept {
    static_assert(std::is_base_of_v<Component, T>, "");
    static ComponentID typeID = getNewComponentTypeID();
    return typeID;
}

constexpr std::size_t maxComponents = 32;
constexpr std::size_t maxGroups = 32;

using ComponentBitSet = std::bitset<maxComponents>;
using GroupBitset = std::bitset<maxGroups>;
using ComponentArray = std::array<Component *, maxComponents>;

class Component {
public:
    Entity *entity{};

    virtual void OnInit() {}

    virtual void OnUpdate(float delta) {}

    virtual void OnDraw() {}

    virtual void OnEvent() {}

    virtual void OnClear() {}

    virtual ~Component() {}
};

class Entity {
public:
    Entity(EntityManager &mManager, size_t p_id) : manager(mManager), m_id(p_id) {}

    void OnUpdate(float delta) {
        const auto componentCount = components.size();
        for (std::size_t i = 0; i < componentCount && enabled; i++) {
            auto &c = components[i];
            c->OnUpdate(delta);
        }
    }

    void OnDraw() {
        if (!enabled) {
            return;
        }

        for (auto &c: components)
            c->OnDraw();
    }

    void OnEvent() {
        if (!enabled) {
            return;
        }

        for (auto &c: components)
            c->OnEvent();
    }

    void OnClear() {
        for (auto &c: components)
            c->OnClear();
    }

    bool isEnabled() const { return enabled; }
    void Destroy() { enabled = false; }

    bool hasGroup(Group mGroup) { return groupBitset[mGroup]; }

    void AddGroup(Group mGroup);

    void DelGroup(Group mGroup) { groupBitset[mGroup] = false; }

    template<typename T>
    bool hasComponent() const {
        return componentBitSet[getComponentTypeID<T>()];
    }

    template<typename T, typename... TArgs>
    T &AddComponent(TArgs &&...mArgs) {
        T *c(new T(std::forward<TArgs>(mArgs)...));
        c->entity = this;
        std::unique_ptr<Component> uPtr{c};
        components.emplace_back(std::move(uPtr));

        componentArray[getComponentTypeID<T>()] = c;
        componentBitSet[getComponentTypeID<T>()] = true;

        c->OnInit();
        return *c;
    }

    template<typename T>
    T &GetComponent() const {
        auto ptr(componentArray[getComponentTypeID<T>()]);
        assert(ptr != nullptr && "Entity does not have requested component");
        return *static_cast<T *>(ptr);
    }

    EntityManager &GetManager() { return manager; }

	size_t GetID() const { return m_id; }

private:
    EntityManager &manager;
    bool enabled = true;
    std::vector<std::unique_ptr<Component>> components;

    ComponentArray componentArray{};
    ComponentBitSet componentBitSet{};
    GroupBitset groupBitset{};

	size_t m_id;
};

class EntityManager {
public:
    void SetScene(Scene *scene) { m_scene = scene; }
    Scene *GetScene() const { return m_scene; }

    void Update(float delta) {
        const auto entityCount = entities.size();
        for (std::size_t i = 0; i < entityCount; i++) {
            auto &e = entities[i];
            if (e->isEnabled()) {
                e->OnUpdate(delta);
            }
        }
    }

    void Draw() {
        const auto entityCount = entities.size();
        for (std::size_t i = 0; i < entityCount; i++) {
            auto &e = entities[i];
            if (e->isEnabled()) {
                e->OnDraw();
            }
        }
    }

    void Event() {
        const auto entityCount = entities.size();
        for (std::size_t i = 0; i < entityCount; i++) {
            auto &e = entities[i];
            if (e->isEnabled()) {
                e->OnEvent();
            }
        }
    }

    void Clear() {
        for (auto &e: entities)
            e->OnClear();
    }

    void Refresh() {
        for (auto i(0u); i < maxGroups; i++) {
            auto &v(groupedEntities[i]);
            std::erase_if(v, [i](Entity *mEntity) { return !mEntity->isEnabled() || !mEntity->hasGroup(i); });
        }

        std::erase_if(entities, [](const std::unique_ptr<Entity> &mEntity) { return !mEntity->isEnabled(); });
    }

    void AddToGroup(Entity *mEntity, Group mGroup) { groupedEntities[mGroup].emplace_back(mEntity); }

    std::vector<Entity *> &GetGroup(Group mGroup) { return groupedEntities[mGroup]; }

    Entity &AddEntity() {
        auto e = new Entity(*this, entities.size()); // TODO: proper id generation
        std::unique_ptr<Entity> uPtr{e};
        entities.emplace_back(std::move(uPtr));
        return *e;
    }

    inline const auto &GetEntities() const { return entities; }

private:
    Scene *m_scene = nullptr;
    std::vector<std::unique_ptr<Entity>> entities;
    std::array<std::vector<Entity *>, maxGroups> groupedEntities;
};
