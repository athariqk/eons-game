#include "organism.h"

#include <SDL3/SDL_render.h>

#include <IGraphicsContext.h>
#include <Scene.h>
#include <Viewport.h>
#include "organismAI.h"
#include "species.h"

#include "PrimitiveShape.h"
#include "Random.h"
#include "RigidBodyComponent.h"
#include "TransformComponent.h"

OrganismComponent::OrganismComponent(Species *m_species) : species(m_species) { genome = m_species->genes; }

void OrganismComponent::OnInit() {
    id = Random::RandomInt(0, 20);

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

    // Add all the necessary components
    transform = &entity->AddComponent<TransformComponent>(spawnX, spawnY, genome.size, genome.size);
    rb = &entity->AddComponent<RigidBodyComponent>();
    ai = &entity->AddComponent<OrganismAI>(genome.speed, 50);

    membraneColour = genome.membraneColour;
    curEnergy = genome.energyCapacity;
    fitness = 0;
}

void OrganismComponent::OnUpdate(float delta) {
    curEnergy -= 0.02f;
    fitness -= 0.005f;

    if (curEnergy == 0 || curEnergy < 0) {
        species->deleteOrganism(this);
    }

    if (curEnergy < 0)
        curEnergy = 0;

    if (fitness < 0)
        fitness = 0;

    if (curEnergy > genome.energyCapacity || fitness > 100) {
        curEnergy = genome.energyCapacity;
        fitness = 100;
    }
}

void OrganismComponent::OnDraw() {
    Scene *scene = entity->GetManager().GetScene();
    auto ctx = scene->GetViewport()->GetGraphicsContext();
    if (scene && ctx) {
        SDL_Renderer *renderer = static_cast<SDL_Renderer *>(ctx->GetNativeHandle());
        if (renderer) {
            // Transform world position to screen position using viewport
            Viewport2D *viewport = scene->GetViewport();
            Vector2D screenPos = transform->position;

            if (viewport) {
                screenPos = viewport->WorldToScreen(transform->position);
            }

            PrimitiveShape::DrawCircle(renderer, screenPos.x, screenPos.y, transform->width / 2, membraneColour, true,
                                       true);
        }
    }
}

std::string OrganismComponent::getSpeciesName() const { return species->genus + " " + species->epithet; }

size_t OrganismComponent::getID() const { return id; }
