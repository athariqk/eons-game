#include "scene.h"

#include "Simulation/environment.h"
#include "Simulation/organism.h"

#include "gui.h"

#include "Camera.h"
#include "Engine/AudioManager.h"
#include "Logger.h"
#include "Physics/Physics2D.h"

EntityManager entityManager;
Physics2D physics2d;
AudioManager audio;
GUI gui;
Environment env;
Scene *Scene::staticInstance = nullptr;
SDL_Event Scene::m_event;
Camera mainCamera;

Scene::Scene(Window &m_mainWindow) : mainWindow(m_mainWindow) { staticInstance = this; }

Scene::~Scene() { staticInstance = nullptr; }

Scene *Scene::Get() { return staticInstance; }

void Scene::Init() {
    // Start the game loop
    m_running = true;

    // Initialize ImGui
    gui.OnInit();

    // Initialize physics
    physics2d.Init();

    // Initialize audio
    audio.Init();

    env.spawnNutrients(30);

    /* Initial species */
    env.addSpeciesToEnvironment("Primum", "Primus", "specium");
}

void Scene::HandleEvents() {
    while (SDL_PollEvent(&m_event)) {
        entityManager.Event();
        gui.OnImGuiEvent();

        switch (m_event.type) {
            case SDL_EVENT_QUIT: {
                m_running = false;
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED: {
                SDL_SetWindowSize(mainWindow.GetWindow(), m_event.window.data1, m_event.window.data2);
                LOG_INFO("Window resolution changed: {} x {}", m_event.window.data1, m_event.window.data2);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                if (m_event.button.button == SDL_BUTTON_LEFT) {
                    isPanning = true;
                }
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                if (m_event.button.button == SDL_BUTTON_LEFT) {
                    isPanning = false;
                }
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                if (isPanning) {
                    mainCamera.position.x -= m_event.motion.xrel * cameraSpeed;
                    mainCamera.position.y -= m_event.motion.yrel * cameraSpeed;
                }
                break;
            }
            default: {
                break;
            }
        }
    }
}

void Scene::Update(const double p_delta, const uint64_t p_ticks) {
    entityManager.Refresh();
    physics2d.Step();
    entityManager.Update(p_delta);

    HandleCameraMovement(p_delta);

    ticks = p_ticks;
}

void Scene::Render() const {
    SDL_Renderer *renderer = mainWindow.GetRenderer();

    /* Solid background color (RGB: 70, 130, 180) */
    SDL_SetRenderDrawColor(renderer, 70, 130, 180, 255);
    SDL_RenderClear(renderer);

    entityManager.Draw();
    gui.OnImGuiRender();

    SDL_RenderPresent(renderer);
}

Window &Scene::GetWindow() const { return mainWindow; }

EntityManager &Scene::GetEntityManager() { return entityManager; }

Environment &Scene::GetEnvironment() { return env; }

GUI &Scene::GetGUI() { return gui; }

Physics2D &Scene::GetPhysics() { return physics2d; }

AudioManager &Scene::GetAudio() { return audio; }

Camera &Scene::GetCamera() { return mainCamera; }

void Scene::MoveCamera(float x, float y) {
    mainCamera.position.x += x;
    mainCamera.position.y += y;
}

void Scene::Clean() {
    gui.OnImGuiClear();
    entityManager.Clear();
    audio.Clear();

    SDL_Quit();
}

void Scene::HandleCameraMovement(const double p_delta) const {
    const bool *keyboardState = SDL_GetKeyboardState(nullptr);

    if (keyboardState[SDL_SCANCODE_W]) {
        mainCamera.position.y -= cameraSpeed * p_delta;
    }
    if (keyboardState[SDL_SCANCODE_S]) {
        mainCamera.position.y += cameraSpeed * p_delta;
    }
    if (keyboardState[SDL_SCANCODE_A]) {
        mainCamera.position.x -= cameraSpeed * p_delta;
    }
    if (keyboardState[SDL_SCANCODE_D]) {
        mainCamera.position.x += cameraSpeed * p_delta;
    }
}
