#pragma once

#include <Vector2D.h>
#include <World.h>
#include <memory>
#include <string>
#include <vector>

#include <utils/Config.h>

namespace Aeon {
class Gui;
class InputSystem;
class ICamera;
class Viewport2D;
class RenderSystem;
} // namespace Aeon

class SpeciesComponent;
class OrganismComponent;
class Genes;

namespace Config {

struct MicrocosmCamera {
    float Acceleration = 80.0f;
    float Friction = 8.0f;
    float MaxSpeed = 30.0f;

    /**
     * @brief Defines how "tight" the camera follows the mouse.
     *
	 * Higher number (e.g., 50) = tighter, more robotic.
     * Lower number (e.g., 10) = looser, more floaty/springy.
     */
    float DragSensitivity = 25.0f;

    float ZoomSensitivity = 2.0f;
    float ZoomFriction = 10.0f;
    float MinZoom = 0.1f;
    float MaxZoom = 4.0f;

    DEFINE_CONFIG_MAP_FIELDS(MicrocosmCamera, Acceleration, Friction, MaxSpeed, DragSensitivity, ZoomSensitivity,
                             ZoomFriction, MinZoom, MaxZoom)
};

} // namespace Config

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
    std::string m_configFileName = "game.ini";
    Aeon::ConfigMap m_config;

    std::vector<SpeciesComponent *> m_speciesEntities;
    std::vector<OrganismComponent *> m_organismEntities;

    void UpdateCameraControl(double p_delta);
    void UpdateCameraMovement(double p_delta);
    void RegisterSystems();

    Aeon::InputSystem *m_inputSystem;
    Aeon::ICamera *m_mainCamera;
    Aeon::Viewport2D *m_viewport;
    Aeon::RenderSystem *m_renderSystem;

    Config::MicrocosmCamera m_cameraConf;
    bool m_isDragging = false;
    Aeon::Vector2D m_camInputDir;
    Aeon::Vector2D m_camVelocity;
    float m_zoomInput;
    float m_zoomVelocity;

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
