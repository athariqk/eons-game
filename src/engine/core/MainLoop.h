#pragma once

#include <memory>
#include <AudioManager.h>
#include <Physics.h>
#include <Window.h>
#include <Scene.h>
#include <Viewport.h>
#include <Event.h>

enum SDL_Scancode;
class IGraphicsContext;

class MainLoop {
public:
	MainLoop(Window &p_window);
	~MainLoop();

	void SetCurrentScene(std::unique_ptr<Scene> scene);
	Scene *GetCurrentScene();

	void Init();
	void HandleEvents();
	void Update(double p_delta, uint64_t p_ticks);
	void Render();
	void Clean();

	bool running() const { return m_running; }

	uint64_t &GetTick() { return m_ticks; }

private:
	bool m_running = false;

	Window &m_mainWindow;

	std::unique_ptr<Scene> m_currentScene;

	Viewport2D m_viewport2d;
	Physics2D m_physics2d{};
	AudioManager m_audio{};

	uint64_t m_ticks = 0;

	KeyEvent::Key MapSDLKeyToKey(SDL_Scancode scancode);
};
