#pragma once

#include <Scene.h>
#include <Vector2D.h>
#include <set>
#include <string>
#include <vector>

#include "Gui.h"

class Species;

class MicrocosmScene : public Scene {
public:
    MicrocosmScene() {}
    ~MicrocosmScene() {}

    void OnInit();
    void OnEvent(std::shared_ptr<BaseEvent> event);
    void OnUpdate(double p_delta, uint64_t p_ticks);
    void OnRender();
    void OnFinish();

    void AddSpeciesToEnvironment(const std::string &name, const std::string &genus, const std::string &epithet);

    //! \brief Spawn given amount of food to be randomly
    //! scattered around the environment
    void SpawnNutrients(int amount);

    //! \brief Make this species cease to exist
    void MakeExtinct(Species *species);

    std::vector<Species *> &GetAllSpecies();

    //! \brief Returns species by object
    Species *GetSpecies(const Species *species);

    //! \brief Returns species by its name (identifier)
    Species *GetSpecies(std::string name);

    int GetSpeciesIndex(const Species *species) const;

    enum GroupLabels : std::size_t { NutrientsGroup, SpeciesGroup, OrganismsGroup, Other };

private:
    std::vector<Species *> m_speciesInEnvironment;

    std::set<int> m_pressedKeys;

private:
    GUI gui;

    void UpdateCameraMovement(double p_delta);

    const float camSpeed = 150.0f; // per second
};
