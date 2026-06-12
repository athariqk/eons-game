#pragma once

#include <memory>
#include <string>
#include <vector>

#include <modules/Config.h>
#include <modules/World.h>
#include <modules/utils/Structures.h>

namespace ncore {
class Gui;
class InputSystem;
class ICamera;
class Viewport2D;
class RenderSystem;
} // namespace ncore

class SpeciesComponent;
class OrganismComponent;
class Genes;

namespace cfg {

struct MicrocosmCamera {
    float Acceleration = 80.0f;
    float Friction = 8.0f;
    float MaxSpeed = 50.0f;

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

    NC_DEF_CFG_MAP(MicrocosmCamera, Acceleration, Friction, MaxSpeed, DragSensitivity, ZoomSensitivity, ZoomFriction,
                   MinZoom, MaxZoom)
};

} // namespace cfg

class MicrocosmWorld : public ncore::World {
public:
    MicrocosmWorld();
    ~MicrocosmWorld() {}

    void on_init() override;
    void on_update(double p_delta) override;
    void on_render(ncore::IGraphicsContext &graphics) override {}
    void on_gui_render() override;

    /**
     * @brief Cleanup phase. Called before systems cleanup
     */
    void on_finish() override;

    void add_species(const std::string &name, const std::string &genus, const std::string &epithet);

    //! \brief Make this species cease to exist (extinct)
    void remove_species(SpeciesComponent *species);

    inline auto &get_species_reg() { return species_reg; }
    inline auto &get_organism_reg() { return organism_reg; }

    //! \brief Returns species by its entity ID
    SpeciesComponent *get_species_by_id(size_t entityId);

    SpeciesComponent *get_species(const SpeciesComponent *species);
    std::string get_species_name(OrganismComponent *organism);

    int get_species_idx(const SpeciesComponent *species) const;

    //! \brief Spawn a single organism with given genes and randomly mutate it if set true
    OrganismComponent &add_organism(SpeciesComponent *species);

    //! \brief Destroy a single given organism of a species
    void remove_organism(OrganismComponent *organism);

    //! \brief Destroy all members of this species
    void clear_organism_reg(SpeciesComponent *species);

    enum GroupLabels : std::size_t { NUTRIENTS_GROUP, SPECIES_GROUP, ORGANISM_GROUP, OTHER };

private:
    void update_cam_ctrl(double p_delta);
    void update_cam_movement(double p_delta);
    void register_systems();

    bool get_is_inputy_empty(const char *str) const {
        static const unsigned char zero_buffer[255] = {0};
        return memcmp(str, str, 255) == 0;
    }

    void set_input_empty() {
        memset(inputGenus, 0, sizeof(inputGenus));
        memset(inputEpithet, 0, sizeof(inputEpithet));
    }

    std::string cfg_filename = "game.ini";
    ncore::ConfigMap cfg_map;
    std::vector<SpeciesComponent *> species_reg;
    std::vector<OrganismComponent *> organism_reg;
    ncore::InputSystem *inputs;
    ncore::ICamera *main_cam;
    ncore::Viewport2D *viewport;
    ncore::RenderSystem *rendering;
    cfg::MicrocosmCamera cfg_cam;
    bool is_dragging = false;
    ncore::Vec2 cam_input_dir;
    ncore::Vec2 cam_velocity;
    float cam_input_zoom;
    float cam_velocity_zoom;
    char inputGenus[255];
    char inputEpithet[255];
};
