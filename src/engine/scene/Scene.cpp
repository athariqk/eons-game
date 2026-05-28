#include "Scene.h"

void Scene::_OnUpdate(double delta, uint64_t ticks) {
    m_entityManager.Refresh();
	m_entityManager.Update(delta);
    OnUpdate(delta, ticks);
}

void Scene::_OnRender() {
	m_entityManager.Draw();
    OnRender();
}

void Scene::_OnFinish() {
	m_entityManager.Clear();
    OnFinish();
}

Entity &Scene::CreateEntity() { return m_entityManager.AddEntity(); }
