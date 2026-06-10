#include "NutrientSystem.h"

#include <Entity.h>
#include <Logger.h>
#include <Random.h>
#include <RigidBodyComponent.h>
#include <Services.h>
#include <SpriteComponent.h>
#include <TransformComponent.h>
#include <Viewport.h>
#include <World.h>

#include "MicrocosmWorld.h"
#include "components/NutrientComponent.h"

bool NutrientSystem::OnInit(Aeon::World &world) {
    return true;
}

void NutrientSystem::OnFixedUpdate(Aeon::World &world, double fixedDelta) {
    auto *viewport = world.GetMainLoop().GetServices().TryGet<Aeon::Viewport2D>();

    int count = 0;
    for (const auto &entityPtr: world.GetGroup(MicrocosmWorld::GroupLabels::NutrientsGroup)) {
        if (!entityPtr->IsEnabled())
            continue;

        if (entityPtr->HasComponent<NutrientComponent>()) {
            auto &nutrient = entityPtr->GetComponent<NutrientComponent>();

            // Destroy if depleted
            if (nutrient.curEnergy < 0) {
                entityPtr->Destroy();
            }
        }

        count++;
    }

    auto spawnIntervalFactor = Aeon::Random::RandomFloat(0.5f, 1.0f);
    m_spawnTimer += fixedDelta;
    if (m_spawnTimer >= m_spawnInterval * spawnIntervalFactor) {
        m_spawnTimer = 0.0f;
        auto spawnFactor = Aeon::Random::RandomFloat(0.03f, 0.07f);
        int amountToSpawn = 0;
        while (count + amountToSpawn < m_maxAmountOfNutrients && amountToSpawn < m_maxAmountOfNutrients * spawnFactor) {
            amountToSpawn++;
        }
        HandleSpawnNutrients(world, amountToSpawn);
    }
}

void NutrientSystem::HandleSpawnNutrients(Aeon::World &world, int amountToSpawn) {
    for (int i = 0; i < amountToSpawn; i++) {
        auto &nutrient(world.CreateEntity());

        float size = Aeon::Random::RandomFloat(0.2f, 0.6f);

        float spawnX = Aeon::Random::RandomFloat(-m_spawnArea.x, m_spawnArea.x);
        float spawnY = Aeon::Random::RandomFloat(-m_spawnArea.y, m_spawnArea.y);

        nutrient.AddComponent<Aeon::TransformComponent>(Aeon::Vector2D(spawnX, spawnY), 0.0f,
                                                        Aeon::Vector2D(size, size));
        nutrient.AddComponent<Aeon::RigidBodyComponent>();
        nutrient.AddComponent<NutrientComponent>(5.0f);
        nutrient.AddComponent<Aeon::SpriteComponent>("assets/nutrient.webp");
        nutrient.AddGroup(MicrocosmWorld::GroupLabels::NutrientsGroup);
    }

    LOG_INFO(Aeon::Log::Game, "Spawned {} nutrients to the environment", amountToSpawn);
}

