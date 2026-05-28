#include "Nutrient.h"

#include <Scene.h>
#include <Viewport.h>
#include "Random.h"

#include "RigidBodyComponent.h"
#include "SpriteComponent.h"
#include "TransformComponent.h"

void Nutrient::OnInit() {
    id = Random::RandomInt(5, 10);
    int size = Random::RandomInt(10, 20);

    float spawnX = 0;
    float spawnY = 0;

    // Get camera position from viewport
    Scene *scene = entity->GetManager().GetScene();
    if (scene) {
        Viewport2D *viewport = scene->GetViewport();
        if (viewport) {
            ICamera *camera = viewport->GetMainCamera();
            if (camera) {
                const auto cameraPos = camera->GetPosition();
                spawnX = cameraPos.x + Random::RandomFloat(-100, 100);
                spawnY = cameraPos.y + Random::RandomFloat(-100, 100);
            }
        }
    }

    transform = &entity->AddComponent<TransformComponent>(spawnX, spawnY, size, size);

    sprite = &entity->AddComponent<SpriteComponent>("assets/nutrient.png");
    rb = &entity->AddComponent<RigidBodyComponent>();
}

void Nutrient::OnUpdate(float delta) {
    if (curEnergy < 0)
        entity->Destroy();
}
