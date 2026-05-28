#include "Microcosm.h"

#include <vector>

#include <SDL3/SDL_mouse.h>

#include <Logger.h>
#include <Viewport.h>

#include "Nutrient.h"
#include "Organism.h"
#include "Species.h"

void MicrocosmScene::OnInit() {
    // Initialize ImGui
    gui.OnInit(GetViewport()->GetWindow());

    SpawnNutrients(30);

    /* Initial species */
    AddSpeciesToEnvironment("Primum", "Primus", "specium");
}

void MicrocosmScene::OnEvent(std::shared_ptr<BaseEvent> event) {
    gui.OnEvent(event);

    // Get viewport's main camera
    auto *viewport = GetViewport();
    auto *camera = viewport ? viewport->GetMainCamera() : nullptr;

    // Handle mouse events
    if (auto me = dynamic_cast<MouseEvent *>(event.get())) {
        // Apply camera panning when mouse is being dragged
        if (me->m_isPanning && camera && !me->m_relativeMotion.IsZero()) {
            Vector2D motion = me->m_relativeMotion * -(camSpeed / 100.0f);

            auto newPos = camera->GetPosition();
            newPos.x += motion.x;
            newPos.y += motion.y;
            camera->SetPosition(newPos);
        }
    }

    // Handle keyboard events
    if (auto ke = dynamic_cast<KeyEvent *>(event.get())) {
        int keyCode = static_cast<int>(ke->m_key);

        if (ke->m_type == KeyEvent::Type::KeyDown) {
            m_pressedKeys.insert(keyCode);
        } else if (ke->m_type == KeyEvent::Type::KeyUp) {
            m_pressedKeys.erase(keyCode);
        }
    }
}

void MicrocosmScene::OnUpdate(const double p_delta, const uint64_t p_ticks) { UpdateCameraMovement(p_delta); }

void MicrocosmScene::OnRender() { gui.OnRender(this, GetViewport()->GetWindow()); }

void MicrocosmScene::OnFinish() { gui.OnClear(); }

void MicrocosmScene::UpdateCameraMovement(const double p_delta) {
    auto *viewport = GetViewport();
    auto *camera = viewport ? viewport->GetMainCamera() : nullptr;
    if (!camera)
        return;

    float movement = camSpeed * static_cast<float>(p_delta);

    auto pos = camera->GetPosition();

    // Check pressed keys and move camera
    if (m_pressedKeys.count(static_cast<int>(KeyEvent::Key::W))) {
        pos.y -= movement;
    }
    if (m_pressedKeys.count(static_cast<int>(KeyEvent::Key::S))) {
        pos.y += movement;
    }
    if (m_pressedKeys.count(static_cast<int>(KeyEvent::Key::A))) {
        pos.x -= movement;
    }
    if (m_pressedKeys.count(static_cast<int>(KeyEvent::Key::D))) {
        pos.x += movement;
    }

    camera->SetPosition(pos);
}

void MicrocosmScene::AddSpeciesToEnvironment(const std::string &name, const std::string &genus,
                                             const std::string &epithet) {
    auto &instance(CreateEntity());
    instance.AddComponent<Species>(name, genus, epithet);
    m_speciesInEnvironment.push_back(&instance.GetComponent<Species>());

    auto &species = instance.GetComponent<Species>();

    LOG_INFO("Added species {} to the environment, with following traits:\n speed {}, energy capacity {}, size {}",
             species.getFormattedName(true), species.genes.speed, species.genes.energyCapacity, species.genes.size);

    species.addOrganism();
}

Species *MicrocosmScene::GetSpecies(const Species *species) {
    for (auto &s: m_speciesInEnvironment) {
        if (s == species)
            return s;
    }

    LOG_ERROR("Species is not found!");
    return nullptr;
}

Species *MicrocosmScene::GetSpecies(std::string name) {
    for (auto &s: m_speciesInEnvironment) {
        if (s->name == name) {
            return s;
        }
    }

    LOG_ERROR("Not found species with name {}", name);
    return nullptr;
}

std::vector<Species *> &MicrocosmScene::GetAllSpecies() { return m_speciesInEnvironment; }

void MicrocosmScene::MakeExtinct(Species *species) {
    if (species == nullptr) {
        LOG_ERROR("Tried to make a null species extinct!");
        return;
    }

    const auto it = std::ranges::find(m_speciesInEnvironment, species);
    if (it == m_speciesInEnvironment.end()) {
        LOG_ERROR("Species {} is not found!", species->getID());
        return;
    }

    const auto formattedName = species->getFormattedName(false);

    species->entity->Destroy();
    species->clearOrganisms();
    m_speciesInEnvironment.erase(it);

    LOG_INFO("Species {} has gone extinct!", formattedName);
}

int MicrocosmScene::GetSpeciesIndex(const Species *species) const {
    if (const auto it = std::ranges::find(m_speciesInEnvironment, species); it != m_speciesInEnvironment.end()) {
        return static_cast<int>(std::distance(m_speciesInEnvironment.begin(), it));
    }

    LOG_ERROR("Index of species {} is not found!", species != nullptr ? species->getID() : 0);
    return -1;
}

void MicrocosmScene::SpawnNutrients(int amount) {
    for (int i = 0; i < amount; i++) {
        auto &nutrient(CreateEntity());
        nutrient.AddComponent<Nutrient>(30);
        nutrient.AddGroup(GroupLabels::NutrientsGroup);
    }
    LOG_INFO("Spawned {} nutrients to the environment", amount);
}
