#pragma once

#include <Vector2D.h>
#include <World.h>
#include <memory>
#include <string>
#include <vector>

namespace Aeon {
class Gui;
}

class SpeciesComponent;
class OrganismComponent;
class Genes;

class MicrocosmWorld : public Aeon::World {
public:
    MicrocosmWorld();
    ~MicrocosmWorld() {}

    void OnInit() override;
    void OnUpdate(double p_delta) override;
    void OnRender(Aeon::IGraphicsContext &graphics) override {}
    void OnGuiRender() override;
    void OnFinish() override;

    void AddSpeciesToEnvironment(const std::string &name, const std::string &genus, const std::string &epithet);

    //! \brief Spawn given amount of food to be randomly scattered around the environment
    void SpawnNutrients(int amount);

    //! \brief Make this species cease to exist
    void MakeExtinct(SpeciesComponent *species);

    inline auto &GetAllSpecies() { return m_speciesEntities; }
    inline auto &GetAllOrganisms() { return m_organismEntities; }

    //! \brief Returns species by its entity ID
    SpeciesComponent *GetSpeciesById(size_t entityId);

    SpeciesComponent *GetSpecies(const SpeciesComponent *species);
    std::string GetSpeciesName(OrganismComponent *organism);

    int GetSpeciesIndex(const SpeciesComponent *species) const;

    //! \brief Spawn a single organism with given genes and randomly mutate it if set true
    OrganismComponent &AddOrganism(SpeciesComponent *species);

    //! \brief Destroy a single given organism of a species
    void DeleteOrganism(OrganismComponent *organism);

    //! \brief Destroy all members of this species
    void ClearOrganisms(SpeciesComponent *species);

    enum GroupLabels : std::size_t { NutrientsGroup, SpeciesGroup, OrganismsGroup, Other };

private:
    std::vector<SpeciesComponent *> m_speciesEntities;
    std::vector<OrganismComponent *> m_organismEntities;

    // Event subscription IDs
    size_t m_mouseMotionSub = 0;
    size_t m_keyboardSub = 0;
    size_t m_mouseButtonSub = 0;
    size_t m_mouseWheelSub = 0;

    void UpdateCameraMovement(double p_delta);
    void RegisterSystems();
    void SubscribeToEvents();

    // Keyboard movement
    float acceleration = 40.0f;
    float friction = 10.0f; // exponential decay coefficient
    float maxSpeed = 20.0f;
    Aeon::Vector2D m_cameraVelocity{0.0f, 0.0f};

    // Mouse drag / swipe
    float swipeSensitivity = 0.8f; // multiplier on raw pixel delta
    float swipeFriction = 12.0f; // faster decay for mouse-flick coast
    bool m_isDragging = false;
    Aeon::Vector2D m_swipeVelocity{0.0f, 0.0f};

    bool debugMode = false;
    char inputGenus[255];
    char inputEpithet[255];

    bool isInputEmpty(const char *str) const {
        static const unsigned char zero_buffer[255] = {0};
        return memcmp(str, str, 255) == 0;
    }

    void emptyInput() {
        memset(inputGenus, 0, sizeof(inputGenus));
        memset(inputEpithet, 0, sizeof(inputEpithet));
    }
};
