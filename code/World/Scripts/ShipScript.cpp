#include "World/Scripts/ShipScript.hpp"

#include "Utility/Keyboard.hpp"
#include "Utility/VectorConverter.hpp"
#include "World/World.hpp"
#include "World/Entity.hpp"
#include "World/UnitScaling.hpp"
#include "World/Components/RigidBodyComponent.hpp"
#include "World/CoordinateSpaceMapper.hpp"

#include <SFML/Window/Mouse.hpp>
#include <Thor/Math.hpp>
#include <box2d/box2d.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

namespace astro
{

ShipScript::ShipScript(const CoordinateSpaceMapper& coordinateMapper, World& world, b2World& physicsWorld)
	: m_coordinateMapper(&coordinateMapper)
	, m_bulletBuilder(world, physicsWorld)
{
}

void ShipScript::onCreate()
{
	m_body = getComponent<RigidBodyComponent>().body;
	m_bulletBuilder.setSize({5.f, 10.f});
	m_bulletBuilder.setSpeed(200.f);
}

void ShipScript::onDestroy()
{
}

void ShipScript::onUpdate(float deltaTime)
{
	const auto& velocity = m_body->GetLinearVelocity();
	const auto& center = m_body->GetLocalCenter();

	if (isTimeToShoot(deltaTime))
	{
		m_bulletBuilder.setAngle(m_body->GetAngle() - 1.57079632679f)
			.setPosition(calculateBulletStartPosition())
			.spawn();
		m_shouldShoot = false;
	}
}

void ShipScript::onFixedUpdate(float deltaTime)
{
	using Key = sf::Keyboard::Key;

	m_direction = {0,0};

	if (Keyboard::isKeyPressed(Key::Up))
	{
		m_direction.y += -1;
	}

	if (Keyboard::isKeyPressed(Key::Down))
	{
		m_direction.y += 1;
	}

	if (Keyboard::isKeyPressed(Key::Left))
	{
		m_direction.x += -1;
	}

	if (Keyboard::isKeyPressed(Key::Right))
	{
		m_direction.x += 1;
	}

	m_direction.Normalize();

	const b2Vec2 shipLinearVelocity = m_body->GetLinearVelocity();
	const b2Vec2 shipPosition = m_body->GetPosition();
	const float shipAngularVelocity = m_body->GetAngularVelocity();
	const float shipAngle = m_body->GetAngle();

	// Friction effect
	const b2Vec2 friction { -shipLinearVelocity.x * m_frictionConstant, -shipLinearVelocity.y *  m_frictionConstant };
	m_body->ApplyForceToCenter(friction, false);

	// Forward force effect
	const b2Vec2 force { m_direction.x * m_maxSpeed, m_direction.y * m_maxSpeed };
	m_body->ApplyForceToCenter(force, true);

	// Rotate effect
	m_body->SetAngularVelocity(0);

	const b2Vec2 lookAtVector = m_pointToLookAt - shipPosition;
	const float desiredAngle = std::atan2(lookAtVector.y, lookAtVector.x) + thor::toRadian(90.f);

	m_body->SetTransform(shipPosition, desiredAngle);
}

void ShipScript::onEvent(sf::Event event)
{
	switch (event.type)
	{
		case sf::Event::MouseMoved:
		{
			const auto x = event.mouseMove.x;
			const auto y = event.mouseMove.y;
			m_pointToLookAt = toMeters(m_coordinateMapper->mapToViewSpace({x, y}));
			break;
		}
		case sf::Event::MouseButtonPressed:
		{
			if (event.mouseButton.button == sf::Mouse::Left) {
				m_shouldShoot = true;
			}
			break;
		}
		default:
			break;
	}
}

bool ShipScript::isTimeToShoot(float deltaTime)
{
	// m_weaponTimer += deltaTime;

	// if (m_weaponTimer >= m_weaponReloadSpeed)
	// {
	// 	m_weaponTimer -= m_weaponReloadSpeed;
	// 	return true;
	// }

	// return false;
	return m_shouldShoot;
}

sf::Vector2f ShipScript::calculateBulletStartPosition() const
{
	return toPixels(m_body->GetWorldPoint({0.f, -0.63f}));
}

} // namespace astro
