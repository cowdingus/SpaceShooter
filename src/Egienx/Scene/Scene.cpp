#include "Scene/Scene.hpp"

#include "Utility/Box2dDebugDraw.hpp"
#include "Core/Random.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Systems.hpp"

#include "Scene/Components/Components.hpp"
#include "Scene/Components/RigidBodyComponent.hpp"
#include "Scene/Scripts/ShipScript.hpp"

#include <entt/entt.hpp>
#include <spdlog/spdlog.h>
#include <imgui.h>

#include <iostream>

namespace enx
{

Scene::Scene(sf::RenderTarget& mainWindow)
	: m_physicsWorld({0,0})
	, m_worldView(mainWindow.getDefaultView())
	, m_mainWindow(mainWindow)
	, m_box2dDebugDraw(mainWindow)
{
	m_registry.set<GameStateComponent>();

	m_munroFont.loadFromFile("assets/munro.ttf");

	m_physicsWorld.SetDebugDraw(&m_box2dDebugDraw);

	m_worldView.setCenter(0,0);

	m_registry.on_destroy<NativeScriptComponent>().connect<&Scene::deallocateNscInstance>();
	m_registry.on_destroy<RigidBodyComponent>().connect<&Scene::deallocateB2BodyInstance>();
}

Entity Scene::createEntity()
{
	return Entity{ m_registry.create(), m_registry };
}

void Scene::handleEvent(const sf::Event& event)
{
	auto view = m_registry.view<NativeScriptComponent>();

	for (auto entity : view)
	{
		auto& nsc = view.get<NativeScriptComponent>(entity);

		if (nsc.instance)
		{
			nsc.instance->onEvent(event);
		}
	}
}

void Scene::update(float deltaTime)
{
	auto view = m_registry.view<NativeScriptComponent>();

	for (auto entity : view)
	{
		auto& nsc = view.get<NativeScriptComponent>(entity);

		if (!nsc.instance)
		{
			nsc.instance = nsc.instantiateScript();
			nsc.instance->m_entity = Entity{ entity, m_registry };
			nsc.instance->onCreate();
		}

		nsc.instance->onUpdate(deltaTime);
	}

	displayComponentInspector(m_registry);
	displayEntityList(m_registry);
}

void Scene::fixedUpdate(float deltaTime)
{
	auto view = m_registry.view<NativeScriptComponent>();

	for (auto entity : view)
	{
		auto& nsc = view.get<NativeScriptComponent>(entity);

		if (nsc.instance)
		{
			nsc.instance->onFixedUpdate(deltaTime);
		}
	}

	m_physicsWorld.Step(deltaTime, m_velocityIterations, m_positionIterations);
}

void Scene::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	sf::View lastView = target.getView();

	target.setView(m_worldView);

	drawEntities(m_registry, target);

	m_physicsWorld.DebugDraw();

	target.setView(lastView);
}

void Scene::deallocateNscInstance(entt::registry& registry, entt::entity entity)
{
	auto& nsc = registry.get<NativeScriptComponent>(entity);

	if (nsc.instance)
	{
		nsc.instance->onDestroy();
		nsc.destroyScript(&nsc);
	}
}

void Scene::deallocateB2BodyInstance(entt::registry& registry, entt::entity entity)
{
	auto& rb = registry.get<RigidBodyComponent>(entity);

	if (rb.body)
	{
		if (rb.body->GetUserData().pointer)
		{
			delete (Entity*) rb.body->GetUserData().pointer;
			spdlog::info("A b2Body instance was destroyed with an userdata attached.");
		}
		else
		{
			spdlog::info("A b2Body instance was destroyed without an userdata attached.");
		}

		b2World* world = rb.body->GetWorld();
		world->DestroyBody(rb.body);
	}
}

Scene::~Scene()
{
	m_registry.clear();
}

}