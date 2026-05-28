#pragma once

#include "Genes.h"

#include "EntitySystem.h"

#include <string>

class Species;
class OrganismAI;
class TransformComponent;
class RigidBodyComponent;

class OrganismComponent : public Component {
public:
    OrganismComponent(Species *m_species);

    OrganismComponent(Species *m_species, const Genes &genes) : species(m_species), genome(genes) {}

    OrganismComponent(const OrganismComponent &organism) : species(organism.species) {}

    ~OrganismComponent() override {}

    void OnInit() override;

    void OnUpdate(float delta) override;

    void OnDraw() override;

    Species *species{};
    OrganismAI *ai{};
    Genes genome{};
    double fitness{};

    std::string getSpeciesName() const;

    //! \todo Implement proper ID counting
    //! instead of just random numbers
    size_t getID() const;

    float curEnergy{};

private:
    /* Components */
    TransformComponent *transform{};
    RigidBodyComponent *rb{};

    size_t id = 0;

    SDL_Color membraneColour{};
};
